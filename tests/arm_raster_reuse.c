/* Execute the real context publisher/binders and compare masked GPU registers
 * after every update. No framebuffer or texture-cache side effect may vanish. */
#include FH_RASTER_SOURCE
#include "../reference/citro3d/source/effect.c"
#include "../reference/citro3d/source/texenv.c"
#include "../reference/citro3d/source/attribs.c"
volatile unsigned test_checks,test_failure,test_words[2],test_side[2];
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){test_failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);++test_checks;}while(0)
static u32 registers[1024],reference[1800][1024];
static unsigned variant;
static unsigned boundary_test,split_calls,expected_splits;
static shaderProgram_s plain_program,geometry_program;
static shaderInstance_s dummy_geometry;
void C3D_FrameSplit(u8 flags){
    CHECK(flags==GX_CMDLIST_FLUSH);
    CHECK(C3Di_GetContext()->flags&C3DiF_DrawUsed);
    /* Actual list finalization clears DrawUsed; no GPU wait is modelled. */
    C3Di_GetContext()->flags&=~C3DiF_DrawUsed;
    ++split_calls;
}
void GPUCMD_Add(u32 header,const u32 *p,u32 count){
    unsigned reg=header&1023;u32 mask=0;
    for(unsigned j=0;j<4;j++)if(header&(1u<<(16+j)))mask|=255u<<(8*j);
    test_words[variant]+=(count+2)&~1u;
    for(unsigned i=0;i<count;i++){
        unsigned r=reg+((header>>31)?i:0);CHECK(r<1024);
        registers[r]=(registers[r]&~mask)|(p[i]&mask);
        if(r==GPUREG_FRAMEBUFFER_FLUSH || r==GPUREG_EARLYDEPTH_CLEAR ||
           (r==GPUREG_TEXUNIT_CONFIG && (mask&(1u<<16)) && (p[i]&(1u<<16))))++test_side[variant];
    }
}
void C3Di_FrameBufBind(C3D_FrameBuf *f){(void)f;GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_DIM,0x1234);}
void C3Di_BufInfoBind(C3D_BufInfo *b){(void)b;GPUCMD_AddWrite(GPUREG_ATTRIBBUFFERS_LOC,0x5678);}
void fh_pica_buffers_invalidate(void){}
void C3Di_SetTex(int unit,C3D_Tex *tex){(void)unit;(void)tex;GPUCMD_AddWrite(GPUREG_TEXUNIT0_BORDER_COLOR,0x11223344);}
Result shaderProgramConfigure(shaderProgram_s *p,bool v,bool g){
    if(boundary_test)CHECK(split_calls==expected_splits);
    (void)p;(void)v;(void)g;GPUCMD_AddWrite(GPUREG_VSH_INPUTBUFFER_CONFIG,0xdeadcafe);return 0;
}
void C3D_UpdateUniforms(GPU_SHADER_TYPE t){(void)t;}
void C3Di_DirtyUniforms(GPU_SHADER_TYPE t){(void)t;}
void C3D_ImmSendAttrib(float x,float y,float z,float w){(void)x;(void)y;(void)z;(void)w;}
void C3Di_RenderQueueEnableVBlank(void){}
void C3Di_RenderQueueDisableVBlank(void){}
void C3Di_RenderQueueWaitDone(void){}
static unsigned seed;
static unsigned rnd(void){return seed=seed*1664525u+1013904223u;}
static void test_geometry_edges(void){
    C3D_Context *ctx=C3Di_GetContext();memset(ctx,0,sizeof(*ctx));
    geometry_program.geometryShader=&dummy_geometry;
    fh_geometry_device_enabled=false;boundary_test=1;
    ctx->program=&geometry_program;ctx->flags=C3DiF_Active|C3DiF_Program;
    C3Di_UpdateContext();CHECK(fh_geometry_device_enabled);
    /* A pending GS->plain->GS sequence emitted no plain shader/draw. */
    ctx->program=&plain_program;ctx->program=&geometry_program;
    ctx->flags=C3DiF_Active|C3DiF_Program|C3DiF_DrawUsed;
    C3Di_UpdateContext();CHECK(split_calls==0);
    /* Critical edge with framebuffer invalidation at the same time. The split
     * must occur before that path clears DrawUsed and before shader upload. */
    ctx->program=&plain_program;expected_splits=1;
    ctx->flags=C3DiF_Active|C3DiF_Program|C3DiF_DrawUsed|C3DiF_FrameBuf;
    C3Di_UpdateContext();CHECK(split_calls==1);CHECK(!fh_geometry_device_enabled);
    for(unsigned i=0;i<64;i++){
        ctx->program=&geometry_program;ctx->flags=C3DiF_Active|C3DiF_Program|C3DiF_DrawUsed;
        C3Di_UpdateContext();
        ctx->program=&plain_program;ctx->flags=C3DiF_Active|C3DiF_Program;
        if(i&1){ctx->flags|=C3DiF_DrawUsed;++expected_splits;}
        C3Di_UpdateContext();CHECK(split_calls==expected_splits);
    }
    CHECK(fh_geometry_boundary_splits==33);
    CHECK(fh_geometry_boundary_separated==32);
    CHECK(fh_geometry_boundary_edges==65);
    /* Program dirty restoration without a new edge is not another split. */
    ctx->flags=C3DiF_Active|C3DiF_Program|C3DiF_DrawUsed;
    C3Di_UpdateContext();CHECK(split_calls==33);
    boundary_test=0;
}
void test_main(void){
    test_geometry_edges();
    for(variant=0;variant<2;variant++){
        C3D_Context *ctx=C3Di_GetContext();memset(ctx,0,sizeof(*ctx));
        ctx->program=&plain_program;fh_geometry_device_enabled=false;
        memset(registers,0xa5,sizeof(registers));fh_raster_invalidate();
        fh_raster_reuse_enabled=variant;seed=1234;
        for(unsigned step=0;step<1800;step++){
            ctx->flags=C3DiF_Active|C3DiF_Viewport|C3DiF_Scissor|C3DiF_Effect|
                C3DiF_AttrInfo|C3DiF_TexEnvBuf|C3DiF_TexEnvAll|C3DiF_TexAll;
            if(step%7==0){
                ctx->viewport[0]=rnd();ctx->viewport[4]=rnd();ctx->scissor[0]=rnd()%3;
                ctx->scissor[1]=rnd();ctx->scissor[2]=rnd();
                ctx->effect.alphaTest=rnd();ctx->effect.depthTest=rnd();ctx->effect.alphaBlend=rnd();
                ctx->attrInfo.attrCount=1+rnd()%8;ctx->attrInfo.flags[0]=rnd();ctx->attrInfo.permutation=rnd();
                for(unsigned k=0;k<6;k++){u32 *p=(u32*)&ctx->texEnv[k];p[(step/7)% (sizeof(C3D_TexEnv)/4)]=rnd();}
                ctx->texEnvBuf=(rnd()&~7u)|GPU_NO_FOG;ctx->texEnvBufClr=rnd();ctx->fogClr=rnd();
            }
            if(step%31==0)ctx->flags|=C3DiF_FrameBuf|C3DiF_DrawUsed;
            if(step%19==0)ctx->flags|=C3DiF_Program;
            if(step%97==0){memset(registers,0xcd,sizeof(registers));C3Di_AptEventHook(APTHOOK_ONRESTORE,0);}
            C3Di_UpdateContext();
            if(!variant)memcpy(reference[step],registers,sizeof(registers));
            else CHECK(!memcmp(reference[step],registers,sizeof(registers)));
        }
    }
    CHECK(test_words[1]<test_words[0]);CHECK(test_side[1]==test_side[0]);test_pass();
}
