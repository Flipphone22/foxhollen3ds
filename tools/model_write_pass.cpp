// Native model resource write coverage for the pinned Clang/LLVM 18 pipeline.
// Instrument optimized stores, including aliases that source hooks cannot see.
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <set>
#include <vector>

using namespace llvm;
namespace {
struct Write { Instruction *before; Value *pointer; Value *bytes; };
struct ModelWrites : PassInfoMixin<ModelWrites> {
  PreservedAnalyses run(Module &m, ModuleAnalysisManager &) {
    const DataLayout &layout=m.getDataLayout();
    LLVMContext &ctx=m.getContext();
    Type *word=Type::getInt32Ty(ctx);
    Type *addressType=layout.getIntPtrType(ctx);
    Type *pointerType=PointerType::getUnqual(ctx);
    auto *pageType=ArrayType::get(word,256);
    auto *pages=cast<GlobalVariable>(m.getOrInsertGlobal("fh_model_write_pages",pageType));
    auto hook=m.getOrInsertFunction("fh_model_resources_write_begin",
        FunctionType::get(Type::getVoidTy(ctx),{pointerType,addressType},false));
    std::vector<Write> writes;
    std::set<std::string> external;
    unsigned localStores=0;
    for (Function &fn:m) {
      if (fn.isDeclaration()) continue;
      for (Instruction &i:instructions(fn)) {
        Value *pointer=nullptr,*bytes=nullptr;
        Type *stored=nullptr;
        if (auto *s=dyn_cast<StoreInst>(&i)) {
          pointer=s->getPointerOperand();stored=s->getValueOperand()->getType();
        } else if(auto *s=dyn_cast<AtomicRMWInst>(&i)) {
          pointer=s->getPointerOperand();stored=s->getValOperand()->getType();
        } else if(auto *s=dyn_cast<AtomicCmpXchgInst>(&i)) {
          pointer=s->getPointerOperand();stored=s->getNewValOperand()->getType();
        } else if(auto *s=dyn_cast<MemIntrinsic>(&i)) {
          pointer=s->getRawDest();bytes=s->getLength();
        } else if(auto *call=dyn_cast<CallBase>(&i)) {
          // Calls are recorded separately: this pass does not prove coverage
          // of writes performed by assembly or external libraries.
          if(auto *callee=call->getCalledFunction()) {
            if(callee->isDeclaration() && !callee->isIntrinsic() &&
               !callee->getName().starts_with("fh_model_resource"))
              external.insert(callee->getName().str());
          } else external.insert("<indirect>");
        }
        if(!pointer) continue;
        // Native model allocations never live in an automatic stack slot.
        // A pointer loaded FROM a local variable is not an alloca-derived
        // destination and remains covered.
        if(isa<AllocaInst>(getUnderlyingObject(pointer))) { ++localStores;continue; }
        if(stored) {
          auto size=layout.getTypeStoreSize(stored);
          if(size.isScalable()) report_fatal_error("Scalable model write is unsupported");
          bytes=ConstantInt::get(addressType,size.getFixedValue());
        }
        writes.push_back({&i,pointer,bytes});
      }
    }
    for(auto &w:writes) {
      IRBuilder<> b(w.before);
      Value *address=b.CreatePtrToInt(w.pointer,addressType);
      Value *size=w.bytes;
      if(size->getType()->getIntegerBitWidth()>addressType->getIntegerBitWidth()) {
        Value *max=ConstantInt::get(size->getType(),APInt::getMaxValue(addressType->getIntegerBitWidth()).getZExtValue());
        size=b.CreateSelect(b.CreateICmpUGT(size,max),max,size);
      }
      size=b.CreateZExtOrTrunc(size,addressType);
      Value *end=b.CreateAdd(address,b.CreateSub(size,ConstantInt::get(addressType,1)));
      auto pagePossible=[&](Value *a) {
        Value *page=b.CreateAnd(b.CreateLShr(a,12),8191);
        Value *index=b.CreateLShr(page,5);
        Value *p=b.CreateInBoundsGEP(pageType,pages,{b.getInt32(0),index});
        auto *bits=b.CreateLoad(word,p);
        // Producer owns directory mutation; registration/free already drains
        // readers. Do not turn the page filter into a pointer alias promise.
        Value *bit=b.CreateShl(b.getInt32(1),b.CreateZExtOrTrunc(b.CreateAnd(page,31),word));
        return b.CreateICmpNE(b.CreateAnd(bits,bit),b.getInt32(0));
      };
      Value *possible=b.CreateOr(pagePossible(address),pagePossible(end));
      possible=b.CreateOr(possible,b.CreateICmpUGT(size,ConstantInt::get(addressType,4096)));
      possible=b.CreateOr(possible,b.CreateICmpULT(end,address));
      possible=b.CreateAnd(possible,b.CreateICmpNE(size,ConstantInt::get(addressType,0)));
      Instruction *then=SplitBlockAndInsertIfThen(possible,w.before,false);
      IRBuilder<> call(then);call.CreateCall(hook,{w.pointer,size});
    }
    errs()<<"FH_MODEL_WRITES stores="<<writes.size()<<" local="<<localStores<<" externals=";
    for(const auto &name:external) errs()<<name<<",";
    errs()<<"\n";
    return PreservedAnalyses::none();
  }
};
}
extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION,"FoxhollowModelWrites","1",[](PassBuilder &builder) {
    builder.registerOptimizerLastEPCallback([](ModulePassManager &pm,OptimizationLevel) {
      pm.addPass(ModelWrites());
    });
  }};
}
