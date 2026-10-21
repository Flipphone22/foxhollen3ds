/* Command publication only: this does not model asynchronous PICA execution. */
#include FH_RASTER_SOURCE
#include "../reference/citro3d/source/framebuffer.c"
volatile unsigned test_checks,test_failure,test_words[2],test_side[2];
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){test_failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);++test_checks;}while(0)
static unsigned sequence,last_flush,last_cache,last_write,last_target;
static u32 registers[1024];
u32 gpuCmdBufOffset;
void __wrap_GPUCMD_Add(u32 header,const u32 *p,u32 count){
    unsigned reg=header&1023;u32 mask=0;
    for(unsigned j=0;j<4;j++)if(header&(1u<<(16+j)))mask|=255u<<(8*j);
    ++sequence;gpuCmdBufOffset+=2;
    test_words[0]+=(count+2)&~1u;
    for(unsigned i=0;i<count;i++){
        unsigned r=reg+((header>>31)?i:0);CHECK(r<1024);
        registers[r]=(registers[r]&~mask)|(p[i]&mask);
        if(r==GPUREG_FRAMEBUFFER_FLUSH){last_flush=sequence;test_side[0]++;}
        if(r==GPUREG_COLORBUFFER_LOC)last_target=sequence;
        if(r==GPUREG_TEXUNIT_CONFIG && (mask&(1u<<16)) && (p[i]&(1u<<16))){last_cache=sequence;test_side[1]++;}
    }
}
void __wrap_GPUCMD_Split(u32 **p,u32 *n){*p=C3Di_GetContext()->cmdBuf;*n=gpuCmdBufOffset;gpuCmdBufOffset=0;}
void C3D_FrameSplit(u8 flags){u32 *p,n;CHECK(flags==GX_CMDLIST_FLUSH);C3Di_SplitFrame(&p,&n);}
u32 __wrap_osConvertVirtToPhys(const void *p){return (u32)p;}
void fh_pica_buffers_invalidate(void){}
void C3Di_BufInfoBind(C3D_BufInfo *b){(void)b;}
void C3Di_AttrInfoBind(C3D_AttrInfo *a){(void)a;}
void C3Di_EffectBind(C3D_Effect *e){(void)e;}
void C3Di_TexEnvBind(int n,C3D_TexEnv *e){(void)n;(void)e;}
void C3D_UpdateUniforms(GPU_SHADER_TYPE t){(void)t;}
void C3Di_DirtyUniforms(GPU_SHADER_TYPE t){(void)t;}
void C3D_ImmSendAttrib(float x,float y,float z,float w){(void)x;(void)y;(void)z;(void)w;}
void C3Di_RenderQueueEnableVBlank(void){}
void C3Di_RenderQueueDisableVBlank(void){}
void C3Di_RenderQueueWaitDone(void){}
Result shaderProgramConfigure(shaderProgram_s *p,bool v,bool g){(void)p;(void)v;(void)g;return 0;}
#include FH_TEXTURE_FUNCTIONS
enum {FH_SHADOW_IMAGE_SLOTS=3,FH_SHADOW_IMAGE_SIZE=128};
static C3D_Tex images[3];static C3D_FrameBuf targets[3],main_target;
static unsigned initialized,active,valid,slot,updates,draw_count,index_count;
static shaderProgram_s plain_program,geometry_program;
static shaderInstance_s dummy_geometry;
void fh_platform_panic(const char *s){(void)s;test_fail(__LINE__);}
void fh_pica_scene_invalidate(void){}
void fh_minimal_auxiliary_begin(void){}
void fh_minimal_auxiliary_end(unsigned d,unsigned i){(void)d;(void)i;C3D_SetFrameBuf(&main_target);}
#include FH_IMAGE_FUNCTIONS
static void draw(void){C3Di_UpdateContext();C3Di_GetContext()->flags|=C3DiF_DrawUsed;}
void test_main(void){
    C3D_Context *ctx=C3Di_GetContext();memset(ctx,0,sizeof(*ctx));
    ctx->flags=C3DiF_Active;ctx->program=&plain_program;
    geometry_program.geometryShader=&dummy_geometry;
    main_target.colorBuf=(void*)0x18000000;main_target.width=240;main_target.height=400;
    main_target.colorFmt=GPU_RB_RGBA8;main_target.colorMask=15;
    for(unsigned i=0;i<3;i++){
        images[i].data=(void*)(0x18300000+32768*i);images[i].width=images[i].height=128;
        images[i].fmt=GPU_RGB565;images[i].param=GPU_TEXTURE_MODE(GPU_TEX_2D);
        targets[i].colorBuf=images[i].data;targets[i].width=targets[i].height=128;
        targets[i].colorFmt=GPU_RB_RGB565;targets[i].colorMask=15;
    }
    initialized=1;slot=2;
    for(unsigned n=0;n<1024;n++){
        fh_pica_shadow_image_begin();
        ctx->program=&plain_program;ctx->flags|=C3DiF_Program;
        draw();CHECK(registers[GPUREG_COLORBUFFER_LOC]==((u32)images[slot].data>>3));
        CHECK(!(registers[GPUREG_TEXUNIT_CONFIG]&7));
        last_write=++sequence;draw(); /* clear then caster, same target */
        fh_pica_shadow_image_end(2,12);
        ctx->program=&geometry_program;ctx->flags|=C3DiF_Program;
        C3D_TexBind(0,images+slot);draw();
        CHECK(last_flush>last_write);CHECK(last_target>last_flush);CHECK(last_cache>last_target);
        CHECK(registers[GPUREG_COLORBUFFER_LOC]==((u32)main_target.colorBuf>>3));
        CHECK(registers[GPUREG_TEXUNIT0_ADDR1]==((u32)images[slot].data>>3));
        CHECK(registers[GPUREG_TEXUNIT_CONFIG]&1);CHECK(valid&&!active);
        /* Same-address dynamic pixels must still trigger cache publication. */
        unsigned before=last_cache;C3D_TexBind(0,images+slot);draw();CHECK(last_cache>before);
    }
    CHECK(updates==1024);test_pass();
}
