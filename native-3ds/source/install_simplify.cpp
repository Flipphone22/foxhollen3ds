/* The pinned meshoptimizer simplifier uses this bounded startup allocator.
 * Compile simplifier.cpp and this unit with MESHOPTIMIZER_ALLOC_EXPORT and
 * exceptions enabled. Do not link upstream allocator.cpp or host adapters. */
#include "fh_install_simplify.h"
#include "meshoptimizer.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
struct Exhausted {};
struct Arena {unsigned char*base;size_t capacity,used,peak;bool invalid;};
struct Header {size_t before,end;uintptr_t tag;};
static_assert(sizeof(Header)<=32,"Startup arena header");
Arena*active;
void* arena_allocate(size_t bytes){
    if(!active||bytes>active->capacity||active->used>active->capacity||32>active->capacity-active->used||bytes>active->capacity-active->used-32)throw Exhausted();
    size_t end=(active->used+32+bytes+15)&~size_t(15);
    if(end>active->capacity)throw Exhausted();
    Header*h=reinterpret_cast<Header*>(active->base+active->used);*h=Header{active->used,end,uintptr_t(0x46484c44)};
    void*p=active->base+active->used+32;active->used=end;if(end>active->peak)active->peak=end;return p;
}
void arena_release(void*p){
    if(!active)return;
    uintptr_t at=reinterpret_cast<uintptr_t>(p),base=reinterpret_cast<uintptr_t>(active->base);
    if(at<base+32||at>base+active->capacity||(at-base)%16){active->invalid=true;return;}
    Header*h=reinterpret_cast<Header*>(at-32);
    if(h->tag!=uintptr_t(0x46484c44)||h->end!=active->used||h->before!=at-base-32){active->invalid=true;return;}
    active->used=h->before;h->tag=0;
}
int reject(FhInstallSimplification*out,const char*text){std::free(out->indices);out->indices=nullptr;out->count=0;out->error=0;std::snprintf(out->message,sizeof(out->message),"%s",text);return 0;}
}
meshopt_Allocator::Storage& meshopt_Allocator::storage(){static Storage s={arena_allocate,arena_release};return s;}
extern "C" void fh_install_simplify_close(FhInstallSimplification*out){if(out){std::free(out->indices);std::memset(out,0,sizeof(*out));}}
extern "C" int fh_install_simplify(FhInstallSimplification*out,const unsigned*indices,unsigned count,const float*positions,unsigned vertices,
    const float*attributes,unsigned channels,const float*weights,const unsigned char*locks,float ratio,float error,unsigned permissive,size_t capacity){
    if(!out)return 0;fh_install_simplify_close(out);
    if(active)return reject(out,"Another native simplification is active.");
    if(!indices||!positions||!locks||!count||count%3||count>32768*3||!vertices||vertices>32768||channels>32||
       (channels&&(!attributes||!weights))||!std::isfinite(ratio)||ratio<=0||ratio>=1||!std::isfinite(error)||error<=0||error>.1f||
       permissive>1||capacity<1024||capacity>FH_INSTALL_SIMPLIFY_SCRATCH)return reject(out,"Invalid native simplification request.");
    for(unsigned i=0;i<count;i++)if(indices[i]>=vertices)return reject(out,"Simplification index exceeds source vertices.");
    for(unsigned i=0;i<vertices*3;i++)if(!std::isfinite(positions[i])||std::fabs(positions[i])>1e12f)return reject(out,"Invalid simplification position.");
    for(unsigned i=0;i<vertices*channels;i++)if(!std::isfinite(attributes[i])||std::fabs(attributes[i])>1e6f)return reject(out,"Invalid simplification attribute.");
    for(unsigned i=0;i<channels;i++)if(!std::isfinite(weights[i])||weights[i]<0||weights[i]>1e3f)return reject(out,"Invalid simplification attribute weight.");
    for(unsigned i=0;i<vertices;i++)if(locks[i]>1)return reject(out,"Invalid simplification vertex lock.");
    void*memory=std::malloc(capacity+15);out->indices=static_cast<unsigned*>(std::malloc(count*sizeof(unsigned)));
    if(!memory||!out->indices){std::free(memory);return reject(out,"Not enough startup memory for simplification.");}
    Arena arena={reinterpret_cast<unsigned char*>((reinterpret_cast<uintptr_t>(memory)+15)&~uintptr_t(15)),capacity,0,0,false};active=&arena;
    const char*failure=nullptr;
    try{
        size_t target=static_cast<size_t>(count*ratio)/3*3;if(target<3)target=3;
        out->count=meshopt_simplifyWithAttributes(out->indices,indices,count,positions,vertices,12,
            attributes,channels*sizeof(float),weights,channels,locks,target,error,
            meshopt_SimplifyLockBorder|(permissive?meshopt_SimplifyPermissive:0),&out->error);
    }catch(const Exhausted&){failure="Simplification exceeds its bounded startup arena.";}
    catch(...){failure="Native simplification failed before candidate publication.";}
    active=nullptr;out->scratch_peak=arena.peak;std::free(memory);
    if(arena.invalid||arena.used)failure="Native simplifier allocation retirement failed.";
    if(failure)return reject(out,failure);
    if(out->count>count||out->count%3||!std::isfinite(out->error)||out->error<0)return reject(out,"Invalid native simplification result.");
    for(unsigned i=0;i<out->count;i++)if(out->indices[i]>=vertices)return reject(out,"Simplifier returned an invalid source vertex.");
    return 1;
}
