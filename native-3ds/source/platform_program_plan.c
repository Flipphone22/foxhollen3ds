#include "fh_command_copy.h"
#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "fh_platform.h"
#include <citro3d.h>
#include "../vendor/citro3d/internal.h"

/* The SDK program setters publish immutable DVLE/code metadata. Build all
 * upload combinations there, on the existing graphics owner. Stored packets
 * are CPU descriptions, copied into the normal SDK command buffer; the GPU
 * never borrows this allocation. Existing flush/fence/GS boundaries own it. */
enum { FH_PROGRAM_PLAN_SLOTS=64,FH_PROGRAM_PLAN_WORDS=2048,
       FH_PROGRAM_PLAN_BUDGET=256*1024 };
typedef struct {
    shaderProgram_s *owner;
    u32 *words;
    unsigned state,bytes,count[4],offset[4];
} FhProgramPlan;
static FhProgramPlan program_plans[FH_PROGRAM_PLAN_SLOTS];
unsigned fh_program_plan_compiles,fh_program_plan_hits,fh_program_plan_direct;
unsigned fh_program_plan_bytes,fh_program_plan_peak,fh_program_plan_rejected;
void fh_program_plan_report(void){
    char line[240];
    snprintf(line,sizeof line,"Native program plans: compiles=%u hits=%u direct=%u bytes=%u peak=%u limit=%u static=%u rejected=%u\n",
        fh_program_plan_compiles,fh_program_plan_hits,fh_program_plan_direct,
        fh_program_plan_bytes,fh_program_plan_peak,FH_PROGRAM_PLAN_BUDGET,
        (unsigned)sizeof(program_plans),fh_program_plan_rejected);
    fh_platform_log(line);
}
Result __real_shaderProgramConfigure(shaderProgram_s*,bool,bool);
Result __real_shaderProgramInit(shaderProgram_s*);
Result __real_shaderProgramFree(shaderProgram_s*);
Result __real_shaderProgramSetVsh(shaderProgram_s*,DVLE_s*);
Result __real_shaderProgramSetGsh(shaderProgram_s*,DVLE_s*,u8);
Result __real_shaderProgramSetGshInputPermutation(shaderProgram_s*,u64);

static FhProgramPlan *plan_for(shaderProgram_s *p,unsigned create){
    uintptr_t x=(uintptr_t)p;unsigned slot=((x>>2)^(x>>9))&(FH_PROGRAM_PLAN_SLOTS-1);
    FhProgramPlan *vacant=NULL;
    for(unsigned i=0;i<FH_PROGRAM_PLAN_SLOTS;i++){
        FhProgramPlan *e=program_plans+((slot+i)&(FH_PROGRAM_PLAN_SLOTS-1));
        if(e->state==1 && e->owner==p)return e;
        if(e->state!=1 && !vacant)vacant=e;
        if(!e->state)break;
    }
    if(create && vacant){memset(vacant,0,sizeof *vacant);vacant->state=1;vacant->owner=p;return vacant;}
    return NULL;
}
static void invalidate(shaderProgram_s *p){
    FhProgramPlan *e=plan_for(p,0);if(!e)return;
    if(e->words){free(e->words);fh_program_plan_bytes-=e->bytes;}
    memset(e,0,sizeof *e);e->state=2; /* preserve probe chains */
}
static unsigned bounded(shaderInstance_s *s){
    if(!s || !s->dvle || !s->dvle->dvlp)return 0;
    const DVLP_s *d=s->dvle->dvlp;
    /* App admission bounds, not a claim about all possible SDK programs.
     * At most two 512-word code/128-word descriptor uploads fit 2048 words,
     * including SDK chunk headers, padding and configuration. */
    return d->codeSize<=512 && d->opdescSize<=128 &&
        (!d->codeSize || d->codeData) && (!d->opdescSize || d->opcdescData);
}
static Result prepare(shaderProgram_s *p){
    if(!p || !p->vertexShader)return -1;
    FhProgramPlan *e=plan_for(p,1);
    if(!e || !bounded(p->vertexShader) || (p->geometryShader && !bounded(p->geometryShader))){fh_program_plan_rejected++;return 0;}
    /* Load-only 32 KiB scratch. No allocation/compilation at draw time. */
    u32 scratch[4][FH_PROGRAM_PLAN_WORDS];unsigned counts[4],total=0;
    u32 *saved=gpuCmdBuf;u32 capacity=gpuCmdBufSize,offset=gpuCmdBufOffset;
    Result result=0;
    for(unsigned flags=0;flags<4;flags++){
        gpuCmdBuf=scratch[flags];gpuCmdBufSize=FH_PROGRAM_PLAN_WORDS;gpuCmdBufOffset=0;
        result=__real_shaderProgramConfigure(p,(flags&1)!=0,(flags&2)!=0);
        counts[flags]=gpuCmdBufOffset;
        if(R_FAILED(result))break;
        if(!counts[flags] || (counts[flags]&1) || counts[flags]>FH_PROGRAM_PLAN_WORDS)
            fh_platform_panic("Native program upload packet bounds");
        total+=counts[flags];
    }
    gpuCmdBuf=saved;gpuCmdBufSize=capacity;gpuCmdBufOffset=offset;
    if(R_FAILED(result))return result;
    unsigned bytes=total*4;
    if(bytes>FH_PROGRAM_PLAN_BUDGET-fh_program_plan_bytes){fh_program_plan_rejected++;return 0;}
    u32 *storage=malloc(bytes);
    if(!storage){fh_program_plan_rejected++;return 0;}
    unsigned at=0;
    for(unsigned flags=0;flags<4;flags++){
        memcpy(storage+at,scratch[flags],counts[flags]*4);
        e->offset[flags]=at;e->count[flags]=counts[flags];at+=counts[flags];
    }
    e->bytes=bytes;e->words=storage;fh_program_plan_bytes+=bytes;
    if(fh_program_plan_bytes>fh_program_plan_peak)fh_program_plan_peak=fh_program_plan_bytes;
    fh_program_plan_compiles++;return 0;
}
Result __wrap_shaderProgramConfigure(shaderProgram_s *p,bool vertex,bool geometry){
    FhProgramPlan *e=p?plan_for(p,0):NULL;
    if(!e || !e->words || !p->vertexShader){
        fh_program_plan_direct++;return __real_shaderProgramConfigure(p,vertex,geometry);
    }
    unsigned flags=(vertex?1:0)|(geometry?2:0),count=e->count[flags];
    if(!gpuCmdBuf || (gpuCmdBufOffset&1) || gpuCmdBufOffset>gpuCmdBufSize || count>gpuCmdBufSize-gpuCmdBufOffset)
        fh_platform_panic("Native program upload destination capacity");
    fh_command_copy(gpuCmdBuf+gpuCmdBufOffset,e->words+e->offset[flags],count);
    gpuCmdBufOffset+=count;fh_program_plan_hits++;return 0;
}
Result __wrap_shaderProgramInit(shaderProgram_s *p){invalidate(p);return __real_shaderProgramInit(p);}
Result __wrap_shaderProgramFree(shaderProgram_s *p){
    invalidate(p);
    C3D_Context *ctx=C3Di_GetContext();
    if(ctx->program==p)ctx->program=NULL;
    return __real_shaderProgramFree(p);
}
Result __wrap_shaderProgramSetVsh(shaderProgram_s *p,DVLE_s *v){
    invalidate(p);Result status=__real_shaderProgramSetVsh(p,v);if(R_SUCCEEDED(status))prepare(p);return status;
}
Result __wrap_shaderProgramSetGsh(shaderProgram_s *p,DVLE_s *v,u8 stride){
    invalidate(p);Result status=__real_shaderProgramSetGsh(p,v,stride);if(R_SUCCEEDED(status))prepare(p);return status;
}
Result __wrap_shaderProgramSetGshInputPermutation(shaderProgram_s *p,u64 permutation){
    invalidate(p);Result status=__real_shaderProgramSetGshInputPermutation(p,permutation);if(R_SUCCEEDED(status))prepare(p);return status;
}
