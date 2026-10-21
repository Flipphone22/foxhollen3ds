/* Exact pinned SDK binders + real libctru serializer, executed on ARM11.
 * This isolates wire publication, not a GPU or SDK-owner handoff test. */
#include "fh_pica_draw_stream.h"
#include "internal.h"
#include <c3d/base.h>
#include <string.h>
volatile unsigned test_checks,test_failure;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){test_failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);++test_checks;}while(0)
C3D_Context __C3D_Context;
void C3Di_UpdateContext(void){} /* State is explicitly published below. */
u32 osConvertVirtToPhys(const void *p){return (u32)p;}
void svcBreak(UserBreakType reason){(void)reason;test_fail(__LINE__);}
static u32 original[512],actual[512],saved[512];
static unsigned seed=123;
static unsigned rnd(void){return seed=1664525u*seed+1013904223u;}
static void effect_copy(C3D_Effect *dst,const FhPicaStreamEffect *src){
#define COPY(x) dst->x=src->x
    COPY(zBuffer);COPY(zScale);COPY(zOffset);COPY(cullMode);
    COPY(alphaTest);COPY(stencilMode);COPY(stencilOp);COPY(depthTest);
    COPY(blendClr);COPY(alphaBlend);COPY(clrLogicOp);COPY(fragOpMode);COPY(fragOpShadow);
    COPY(earlyDepth);COPY(earlyDepthFunc);COPY(earlyDepthRef);
#undef COPY
}
static void reference(const FhPicaDrawDescription *s,const FhPicaDrawLive *live,C3D_Tex textures[2]){
    C3D_Context *ctx=&__C3D_Context;ctx->attrInfo=s->attributes;ctx->bufInfo=s->buffers;
    effect_copy(&ctx->effect,&s->effect);
    C3Di_AttrInfoBind(&ctx->attrInfo);
    C3Di_BufInfoBind(&ctx->bufInfo);
    C3Di_EffectBind(&ctx->effect);
    for(unsigned i=0;i<2;i++)if(s->texture_units&(1u<<i))C3Di_SetTex(i,textures+i);
    GPUCMD_AddMaskedWrite(GPUREG_TEXUNIT_CONFIG,11,live->texture_config|(1u<<16));
    GPUCMD_AddMaskedWrite(GPUREG_TEXUNIT_CONFIG,4,1u<<16);
    GPUCMD_AddWrite(GPUREG_TEXUNIT0_SHADOW,live->texture_shadow);
    GPUCMD_AddMaskedWrite(GPUREG_TEXENV_UPDATE_BUFFER,7,live->environment_buffer);
    GPUCMD_AddWrite(GPUREG_TEXENV_BUFFER_COLOR,live->environment_color);
    GPUCMD_AddWrite(GPUREG_FOG_COLOR,live->fog_color);
    for(unsigned i=0;i<4;i++){
        C3D_TexEnv env=s->stages[i];env.color=live->colors[i];C3Di_TexEnvBind(i,&env);
    }
    GPUCMD_AddWrite(GPUREG_VSH_FLOATUNIFORM_CONFIG,0x80000007u);
    GPUCMD_AddWrites(GPUREG_VSH_FLOATUNIFORM_DATA,live->material_uniforms,12);
    C3D_DrawElements(GPU_TRIANGLES,s->index_count,C3D_UNSIGNED_SHORT,(void *)s->index_address);
}
typedef struct {uint32_t regs[1024],uniforms[384],next,cache_clears,draws;} Device;
static void execute(Device *d,const uint32_t *w,unsigned words){
    for(unsigned at=0;at<words;){
        uint32_t h=w[at+1],count=((h>>20)&255)+1,id=h&0xffff,mask=0;
        for(unsigned j=0;j<4;j++)if(h&(1u<<(16+j)))mask|=255u<<(8*j);
        CHECK(at+((count+2)&~1u)<=words);
        for(unsigned j=0;j<count;j++){
            unsigned r=id+((h>>31)?j:0);uint32_t v=w[at+(j?j+1:0)];CHECK(r<1024);
            if(r==GPUREG_VSH_FLOATUNIFORM_CONFIG){CHECK(v&0x80000000u);d->next=(v&255)*4;}
            else if(r==GPUREG_VSH_FLOATUNIFORM_DATA){CHECK(d->next<384);d->uniforms[d->next++]=v;}
            else{
                if(r==GPUREG_TEXUNIT_CONFIG && (v&mask&(1u<<16))){d->cache_clears++;v&=~(1u<<16);}
                d->regs[r]=(d->regs[r]&~mask)|(v&mask);
                if(r==GPUREG_DRAWELEMENTS)d->draws++;
            }
        }
        at+=(count+2)&~1u;
    }
}
static uint32_t arena_words[8192];
static FhPicaDrawDescription descriptions[16];
static FhPicaResidentDraw resident[16];
static Device reference_device,native_device;
volatile unsigned resident_full_words,resident_emitted_words,resident_arena_words;
const unsigned test_sizes[]={sizeof(FhPicaDrawCommands),sizeof(FhPicaResidentDraw),
    sizeof(FhPicaStreamState),sizeof(FhPicaStreamArena),sizeof(FhPicaSamplerCommands)};
static void test_resident(void){
    FhPicaStreamArena arena;CHECK(fh_pica_stream_arena_init(&arena,arena_words,8192));
    for(unsigned i=0;i<16;i++){
        FhPicaDrawDescription *s=descriptions+i;
        AttrInfo_Init(&s->attributes);AttrInfo_AddLoader(&s->attributes,0,GPU_SHORT,3);
        AttrInfo_AddLoader(&s->attributes,1,GPU_UNSIGNED_BYTE,4);
        if(i&4)AttrInfo_AddLoader(&s->attributes,2,GPU_SHORT,2);
        BufInfo_Init(&s->buffers);s->buffers.bufCount=i%3+1;
        for(unsigned j=0;j<(unsigned)s->buffers.bufCount;j++){
            s->buffers.buffers[j].offset=0x1000u+i*0x1000u+j*0x100;
            s->buffers.buffers[j].flags[0]=0x10;s->buffers.buffers[j].flags[1]=(16<<16)|(2u<<28);
        }
        s->effect=(FhPicaStreamEffect){.zBuffer=1,.zScale=0xbf0000,.cullMode=i%2,
            .depthTest=0x1f01,.alphaBlend=0x01010000,.fragOpMode=0xe40100};
        for(unsigned j=0;j<4;j++){C3D_TexEnvInit(s->stages+j);s->stages[j].srcRgb=(i&2)?0x300:0x500;}
        s->texture_units=i%4;s->index_address=0x18020000u+1024*i;s->index_count=3+3*i;
        CHECK(fh_pica_stream_resident_compile(&arena,resident+i,s));
        unsigned used=arena.used;FhPicaResidentDraw duplicate;
        CHECK(fh_pica_stream_resident_compile(&arena,&duplicate,s));
        CHECK(arena.used==used && !memcmp(&duplicate,resident+i,sizeof duplicate));
    }
    resident_arena_words=arena.used;
    /* All materials/geometry remain resident. No runtime arena scan or hash. */
    FhPicaSamplerCommands samplers[8][2];
    for(unsigned f=0;f<8;f++)for(unsigned unit=0;unit<2;unit++){
        FhPicaStreamSampler s={0x12345678,0x00400040,0,0,0x18200000+f*0x1000+unit*0x800,0};
        CHECK(fh_pica_stream_sampler(&samplers[f][unit],unit,&s));
    }
    FhPicaStreamState state={0};fh_pica_stream_state_reset(&state);FhPicaDrawLive live={0};
    for(unsigned step=0;step<1024;step++){
        unsigned index=((step/3)*7)%16;
        const FhPicaResidentDraw *d=resident+index;
        FhPicaDrawCommands full;CHECK(fh_pica_stream_compile(&full,descriptions+index));
        for(unsigned i=0;i<2;i++)live.samplers[i]=&samplers[(step/11)%8][i];
        live.texture_config=d->texture_units;
        if(step%7==0){
            for(unsigned i=0;i<4;i++)live.colors[i]=rnd();
            for(unsigned i=0;i<12;i++)live.material_uniforms[i]=rnd();
        }
        if(step%19==0){live.environment_buffer=rnd();live.environment_color=rnd();live.fog_color=rnd();}
        if(step%37==0){
            fh_pica_stream_state_reset(&state);
            memset(&reference_device,0,sizeof reference_device);memset(&native_device,0,sizeof native_device);
        }
        unsigned reference_at=0,at=0;memset(original,0xa5,sizeof original);memset(actual,0xa5,sizeof actual);
        unsigned full_words=fh_pica_stream_append(original,512,&reference_at,&full,&live);CHECK(full_words);
        FhPicaStreamState saved_state=state;
        unsigned flush=step%13==0;
        unsigned emitted=fh_pica_stream_resident_append(actual,512,&at,&state,d,&live,flush);
        CHECK(emitted && emitted==at && emitted<=full_words);
        CHECK(!memcmp(actual+emitted-30,original+full_words-30,30*4));
        execute(&reference_device,original,full_words);execute(&native_device,actual,emitted);
        CHECK(!memcmp(reference_device.regs,native_device.regs,sizeof native_device.regs));
        CHECK(!memcmp(reference_device.uniforms,native_device.uniforms,sizeof native_device.uniforms));
        CHECK(reference_device.draws==native_device.draws);
        unsigned texture_expected=!saved_state.valid || flush ||
            saved_state.texture_config!=(live.texture_config&~(1u<<16)) ||
            saved_state.texture_shadow!=live.texture_shadow;
        for(unsigned i=0;i<2;i++)if((d->texture_units&(1u<<i)) && saved_state.samplers[i]!=live.samplers[i])texture_expected=1;
        /* Count the cache clears in just this segment independently. */
        Device segment={0};execute(&segment,actual,emitted);CHECK(segment.cache_clears==texture_expected);
        resident_full_words+=full_words;resident_emitted_words+=emitted;
        state=saved_state;memcpy(saved,actual,sizeof actual);at=0;
        at=1;CHECK(!fh_pica_stream_resident_append(actual,512,&at,&state,d,&live,flush));
        CHECK(at==1 && !memcmp(&state,&saved_state,sizeof state) && !memcmp(saved,actual,sizeof actual));at=0;
        CHECK(!fh_pica_stream_resident_append(actual,emitted-1,&at,&state,d,&live,flush));
        CHECK(!at && !memcmp(&state,&saved_state,sizeof state) && !memcmp(saved,actual,sizeof actual));
        CHECK(fh_pica_stream_resident_append(actual,emitted,&at,&state,d,&live,flush)==emitted);
        CHECK(!memcmp(saved,actual,sizeof actual));
    }
    CHECK(resident_emitted_words<resident_full_words);
    /* Failed load admission cannot poison the shared arena or publish a plan. */
    FhPicaStreamArena before=arena;FhPicaResidentDraw before_draw=resident[0];
    FhPicaDrawDescription other=descriptions[0];other.index_count=123456;
    arena.capacity=arena.used;
    CHECK(!fh_pica_stream_resident_compile(&arena,resident,&other));
    CHECK(arena.used==before.used && !memcmp(arena.buckets,before.buckets,sizeof arena.buckets));
    CHECK(!memcmp(resident,&before_draw,sizeof before_draw));
}
void resident_test_main(void){test_resident();test_pass();}
void test_main(void){
    FhPicaDrawDescription s={0};FhPicaDrawLive live={0};
    FhPicaDrawCommands plan,before;FhPicaSamplerCommands samplers[2];C3D_Tex textures[2];
    for(unsigned sample=0;sample<512;sample++){
        AttrInfo_Init(&s.attributes);
        unsigned attrs=sample%12+1;
        for(unsigned i=0;i<attrs;i++)CHECK(AttrInfo_AddLoader(&s.attributes,i,GPU_SHORT,i%4+1)>=0);
        BufInfo_Init(&s.buffers);s.buffers.bufCount=sample%12+1;
        for(unsigned i=0;i<12;i++){
            s.buffers.buffers[i].offset=rnd()&0xffffff;
            s.buffers.buffers[i].flags[0]=rnd();s.buffers.buffers[i].flags[1]=rnd();
        }
        s.effect=(FhPicaStreamEffect){sample&1,rnd()&0xffffff,rnd()&0xffffff,sample%3,
            rnd(),rnd(),rnd(),rnd(),rnd(),rnd(),rnd(),rnd(),rnd(),(sample>>1)&1,sample%4,rnd()};
        for(unsigned i=0;i<4;i++){
            C3D_TexEnvInit(s.stages+i);s.stages[i].srcRgb=rnd()&0xfff;
            s.stages[i].srcAlpha=rnd()&0xfff;s.stages[i].opAll=rnd();
            s.stages[i].color=rnd();s.stages[i].funcRgb=sample%8;s.stages[i].funcAlpha=sample%8;
        }
        s.texture_units=sample%4;s.index_address=0x18010000u+sample*64;s.index_count=sample*3+3;
        CHECK(fh_pica_stream_compile(&plan,&s));before=plan;
        CHECK(plan.count==168+8*__builtin_popcount(s.texture_units));
        for(unsigned frame=0;frame<3;frame++){
            /* Change live values, including addresses, with the same immutable
             * descriptor. Source scrolling/animation must never be frozen. */
            for(unsigned i=0;i<2;i++){
                FhPicaStreamSampler sampler={rnd(),rnd(),rnd()&0x0fffffff,rnd(),
                    0x18200000u+((rnd()&4095)<<3),sample%14};
                CHECK(fh_pica_stream_sampler(samplers+i,i,&sampler));live.samplers[i]=samplers+i;
                textures[i]=(C3D_Tex){0};textures[i].border=sampler.border;textures[i].dim=sampler.dim;
                textures[i].param=sampler.param;textures[i].lodParam=sampler.lod;
                textures[i].data=(void *)sampler.address;textures[i].fmt=sampler.format;
            }
            live.texture_config=(rnd()&0xfffefff8u)|s.texture_units;
            live.texture_shadow=rnd();live.environment_buffer=rnd();live.environment_color=rnd();live.fog_color=rnd();
            for(unsigned i=0;i<4;i++)live.colors[i]=rnd();
            for(unsigned i=0;i<12;i++)live.material_uniforms[i]=rnd();
            memset(original,0x5a,sizeof original);memset(actual,0x5a,sizeof actual);
            gpuCmdBuf=original;gpuCmdBufSize=512;gpuCmdBufOffset=8;
            reference(&s,&live,textures);unsigned cursor=8;
            CHECK(fh_pica_stream_append(actual,512,&cursor,&plan,&live)==plan.count);
            CHECK(cursor==gpuCmdBufOffset);
            CHECK(!memcmp(original,actual,sizeof original));
            CHECK(!memcmp(&plan,&before,sizeof plan));
            memcpy(saved,actual,sizeof actual);
            cursor=9;CHECK(!fh_pica_stream_append(actual,512,&cursor,&plan,&live));
            CHECK(cursor==9 && !memcmp(saved,actual,sizeof actual));
            cursor=8;CHECK(!fh_pica_stream_append(actual,8+plan.count-1,&cursor,&plan,&live));
            CHECK(cursor==8 && !memcmp(saved,actual,sizeof actual));
            cursor=8;CHECK(fh_pica_stream_append(actual,8+plan.count,&cursor,&plan,&live)==plan.count);
            CHECK(cursor==8u+plan.count && !memcmp(saved,actual,sizeof actual));
            cursor=513;CHECK(!fh_pica_stream_append(actual,512,&cursor,&plan,&live));
            CHECK(cursor==513 && !memcmp(saved,actual,sizeof actual));
            live.texture_config^=4;cursor=8;
            CHECK(!fh_pica_stream_append(actual,512,&cursor,&plan,&live));
            CHECK(cursor==8 && !memcmp(saved,actual,sizeof actual));live.texture_config^=4;
            if(s.texture_units){
                unsigned i=(s.texture_units&1)?0:1;live.samplers[i]=NULL;
                CHECK(!fh_pica_stream_append(actual,512,&cursor,&plan,&live));
                CHECK(cursor==8 && !memcmp(saved,actual,sizeof actual));
            }
        }
        FhPicaDrawDescription bad=s;bad.index_address=s.buffers.base_paddr-2;
        CHECK(!fh_pica_stream_compile(&plan,&bad));CHECK(!memcmp(&plan,&before,sizeof plan));
        bad=s;bad.texture_units=4;CHECK(!fh_pica_stream_compile(&plan,&bad));
        CHECK(!memcmp(&plan,&before,sizeof plan));
    }
    test_resident();test_pass();
}
