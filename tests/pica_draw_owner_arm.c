/* Actual retained SDK context/buffer/uniform publishers and real serializer.
 * Shader upload and GPU queue/retirement are mocked, not hardware proof. */
#include FH_BASE_SOURCE
#include FH_UNIFORM_SOURCE
#include FH_BUFFER_SOURCE
#include "/work/reference/citro3d/source/attribs.c"
#include "/work/reference/citro3d/source/effect.c"
#include "/work/reference/citro3d/source/texenv.c"
#include "/work/reference/citro3d/source/texture.c"
#include "/work/reference/citro3d/source/framebuffer.c"
#include "/work/reference/citro3d/source/drawElements.c"
volatile unsigned test_checks,test_failure,split_count,draw_count;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){test_failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);++test_checks;}while(0)
void fh_platform_panic(const char *s){(void)s;test_fail(9000);}
void svcBreak(UserBreakType r){(void)r;test_fail(9001);}
u32 osConvertVirtToPhys(const void *p){return (u32)p;}
static u32 commands[4096],registers[1024],uniforms[2][384],saved_regs[1024],saved_uniforms[2][384];
static unsigned uniform_cursor[2],observed_words;
static void execute(const u32 *w,unsigned words){
    for(unsigned at=0;at<words;){
        u32 h=w[at+1],count=((h>>20)&255)+1,id=h&0xffff,mask=0;
        for(unsigned j=0;j<4;j++)if(h&(1u<<(16+j)))mask|=255u<<(8*j);
        CHECK(at+((count+2)&~1u)<=words);
        for(unsigned j=0;j<count;j++){
            unsigned r=id+((h>>31)?j:0);u32 v=w[at+(j?j+1:0)];CHECK(r<1024);
            if(r==GPUREG_VSH_FLOATUNIFORM_CONFIG || r==GPUREG_GSH_FLOATUNIFORM_CONFIG){
                unsigned stage=r==GPUREG_GSH_FLOATUNIFORM_CONFIG;
                CHECK(v&0x80000000u);uniform_cursor[stage]=(v&255)*4;
            }else if(r==GPUREG_VSH_FLOATUNIFORM_DATA || r==GPUREG_GSH_FLOATUNIFORM_DATA){
                unsigned stage=r==GPUREG_GSH_FLOATUNIFORM_DATA;CHECK(uniform_cursor[stage]<384);
                uniforms[stage][uniform_cursor[stage]++]=v;
            }else{
                if(r==GPUREG_TEXUNIT_CONFIG)v&=~(1u<<16);
                registers[r]=(registers[r]&~mask)|(v&mask);
                if(r==GPUREG_DRAWELEMENTS)draw_count++;
            }
        }
        at+=(count+2)&~1u;
    }
}
static void reset_commands(void){
    gpuCmdBuf=commands;gpuCmdBufSize=4096;gpuCmdBufOffset=0;
    observed_words=0;
    __C3D_Context.cmdBuf=commands;__C3D_Context.cmdBufSize=4096;
}
static void drain(void){
    CHECK(gpuCmdBufOffset>=observed_words);
    execute(gpuCmdBuf+observed_words,gpuCmdBufOffset-observed_words);observed_words=gpuCmdBufOffset;
}
void C3D_FrameSplit(u8 flags){
    CHECK(flags==GX_CMDLIST_FLUSH);CHECK(__C3D_Context.flags&C3DiF_DrawUsed);
    u32 *p,n;CHECK(C3Di_SplitFrame(&p,&n));CHECK(!fh_stream_owner);CHECK(n>=observed_words);
    execute(p+observed_words,n-observed_words);reset_commands();split_count++;
}
Result shaderProgramConfigure(shaderProgram_s *p,bool v,bool g){
    (void)v;(void)g;
    GPUCMD_AddWrite(GPUREG_GSH_ENTRYPOINT,p->geometryShader?0x1234:0);
    return 0;
}
void C3Di_RenderQueueEnableVBlank(void){}
void C3Di_RenderQueueDisableVBlank(void){}
void C3Di_RenderQueueWaitDone(void){}
void C3Di_RenderQueueExit(void){CHECK(!fh_stream_owner);}
void linearFree(void *p){CHECK(p==commands);}
void aptUnhook(aptHookCookie *cookie){(void)cookie;}
void C3D_ImmSendAttrib(float x,float y,float z,float w){(void)x;(void)y;(void)z;(void)w;test_fail(9002);}

static int transient(unsigned r){
    const unsigned ids[]={GPUREG_PRIMITIVE_CONFIG,GPUREG_RESTART_PRIMITIVE,GPUREG_INDEXBUFFER_CONFIG,
        GPUREG_NUMVERTICES,GPUREG_VERTEX_OFFSET,GPUREG_GEOSTAGE_CONFIG,GPUREG_GEOSTAGE_CONFIG2,
        GPUREG_START_DRAW_FUNC0,GPUREG_DRAWELEMENTS,GPUREG_VTX_FUNC,GPUREG_FRAMEBUFFER_FLUSH,
        GPUREG_FRAMEBUFFER_INVALIDATE,GPUREG_EARLYDEPTH_CLEAR,GPUREG_FINALIZE};
    for(unsigned i=0;i<sizeof ids/sizeof ids[0];i++)if(r==ids[i])return 1;
    return 0;
}
static void snapshot(void){memcpy(saved_regs,registers,sizeof registers);memcpy(saved_uniforms,uniforms,sizeof uniforms);}
static void verify_restored(void){
    for(unsigned r=0;r<1024;r++)if(!transient(r))CHECK(registers[r]==saved_regs[r]);
    CHECK(!memcmp(uniforms,saved_uniforms,sizeof uniforms));
}
static DVLP_s dvlp;
static DVLE_s vdvle={.type=(DVLE_type)GPU_VERTEX_SHADER,.dvlp=&dvlp},gdvle={.type=(DVLE_type)GPU_GEOMETRY_SHADER,.dvlp=&dvlp};
static shaderInstance_s vshader={.dvle=&vdvle},gshader={.dvle=&gdvle};
static shaderProgram_s plain={.vertexShader=&vshader},geometry={.vertexShader=&vshader,.geometryShader=&gshader};
static C3D_Tex tex[2];
static const FhPicaBufferLayout sdk_layout={1,{{0x10,(16u<<16)|(2u<<28)}}};
static uint32_t arena_words[2048];
static void setup_sdk(void){
    C3D_Context *ctx=&__C3D_Context;memset(ctx,0,sizeof *ctx);reset_commands();
    ctx->flags=C3DiF_Active|C3DiF_Viewport|C3DiF_Scissor|C3DiF_FrameBuf|C3DiF_LightEnv;
    ctx->fb.width=400;ctx->fb.height=240;ctx->fb.colorFmt=GPU_RB_RGBA8;ctx->fb.depthFmt=GPU_RB_DEPTH16;
    ctx->fb.colorBuf=(void *)0x18200000;ctx->fb.depthBuf=(void *)0x18300000;
    C3D_BindProgram(&plain);
    C3D_AttrInfo attributes;AttrInfo_Init(&attributes);AttrInfo_AddLoader(&attributes,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(&attributes,1,GPU_UNSIGNED_BYTE,4);C3D_SetAttrInfo(&attributes);
    const void *data[3]={(void *)0x18010000,0,0};CHECK(fh_pica_buffers_bind(&sdk_layout,data));
    C3D_DepthMap(true,-1,0);C3D_CullFace(GPU_CULL_NONE);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
    C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    for(unsigned i=0;i<2;i++){
        tex[i]=(C3D_Tex){0};tex[i].data=(void *)(0x18400000+i*0x10000);tex[i].width=64;tex[i].height=64;
        tex[i].fmt=GPU_RGBA8;ctx->tex[i]=tex+i;
    }
    ctx->flags|=C3DiF_TexAll;
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned row=0;row<16;row++)C3D_FVUnifSet(GPU_VERTEX_SHADER,row,1+row,2+row,3+row,4+row);
    C3Di_UpdateContext();drain();
}
void test_main(void){
    setup_sdk();C3D_Context *ctx=&__C3D_Context;
    FhPicaDrawDescription description={0};description.attributes=ctx->attrInfo;description.buffers=ctx->bufInfo;
    description.buffers.buffers[0].offset+=0x20000;
    description.buffers.buffers[0].flags[1]^=1u<<16;
    description.effect=(FhPicaStreamEffect){.zBuffer=1,.zScale=0xbf0000,.cullMode=GPU_CULL_FRONT_CCW,
        .alphaTest=0x101,.depthTest=0x0f01,.alphaBlend=0x06070000,.fragOpMode=0xe40100};
    for(unsigned i=0;i<4;i++){C3D_TexEnvInit(description.stages+i);description.stages[i].srcRgb=0x300;}
    description.texture_units=3;description.index_address=0x18030000;description.index_count=12;
    FhPicaStreamArena arena;FhPicaResidentDraw draw;
    CHECK(fh_pica_stream_arena_init(&arena,arena_words,2048));
    CHECK(fh_pica_stream_resident_compile(&arena,&draw,&description));
    FhPicaResidentDraw world;
    CHECK(fh_pica_stream_world_compile(&arena,&world,&description));
    CHECK(world.effect_words==18);
    FhPicaResidentDraw alternative;description.effect.alphaTest^=1;
    CHECK(fh_pica_stream_world_compile(&arena,&alternative,&description));description.effect.alphaTest^=1;
    CHECK(alternative.attributes==world.attributes && alternative.layout==world.layout && alternative.draw==world.draw);
    CHECK(alternative.effect!=world.effect);
    FhPicaSamplerCommands samplers[2];FhPicaDrawLive live={0};
    for(unsigned i=0;i<2;i++){
        FhPicaStreamSampler s={0x87654321,0x00400040,0,0,0x18500000+i*0x10000,0};
        CHECK(fh_pica_stream_sampler(samplers+i,i,&s));live.samplers[i]=samplers+i;
    }
    live.texture_config=3;live.environment_buffer=0x5500;live.environment_color=0x12345678;
    for(unsigned i=0;i<4;i++)live.colors[i]=0x87654321+i;
    for(unsigned i=0;i<12;i++)live.material_uniforms[i]=0x3f000000+i;
    FhPicaStreamState state;
    /* The world lane must preserve inherited stencil/early-depth/shadow and
     * fragment mode state while changing only its native owned raster fields. */
    ctx->effect.stencilMode=0x34567801;ctx->effect.stencilOp=0x123;
    ctx->effect.blendClr=0xdeadbeef;ctx->effect.clrLogicOp=6;
    ctx->effect.fragOpMode=0xe40009;ctx->effect.fragOpShadow=0x456789;
    ctx->effect.earlyDepth=1;ctx->effect.earlyDepthFunc=2;ctx->effect.earlyDepthRef=0x123456;
    ctx->flags|=C3DiF_Effect;
    C3Di_UpdateContext();drain();snapshot();
    CHECK(fh_pica_stream_begin(&state));
    FhPicaDrawLive environment=live;
    CHECK(fh_pica_stream_environment(&environment));
    CHECK(environment.texture_config==ctx->texConfig && environment.texture_shadow==ctx->texShadow);
    CHECK(environment.environment_buffer==ctx->texEnvBuf && environment.environment_color==ctx->texEnvBufClr);
    CHECK(environment.fog_color==ctx->fogClr && !memcmp(environment.material_uniforms,live.material_uniforms,sizeof live.material_uniforms));
    CHECK(fh_pica_stream_draw(&world,&live,0));drain();
    CHECK(registers[GPUREG_STENCIL_TEST]==saved_regs[GPUREG_STENCIL_TEST]);
    CHECK(registers[GPUREG_STENCIL_OP]==saved_regs[GPUREG_STENCIL_OP]);
    CHECK(registers[GPUREG_BLEND_COLOR]==saved_regs[GPUREG_BLEND_COLOR]);
    CHECK(registers[GPUREG_LOGIC_OP]==saved_regs[GPUREG_LOGIC_OP]);
    CHECK((registers[GPUREG_COLOR_OPERATION]&~0xff00u)==(saved_regs[GPUREG_COLOR_OPERATION]&~0xff00u));
    CHECK(registers[GPUREG_FRAGOP_ALPHA_TEST]==description.effect.alphaTest);
    CHECK(registers[GPUREG_FRAGOP_ALPHA_TEST+3]==description.effect.depthTest);
    live.effect=alternative.effect;
    CHECK(fh_pica_stream_draw(&world,&live,0)==48);drain();
    CHECK(registers[GPUREG_FRAGOP_ALPHA_TEST]==(description.effect.alphaTest^1));
    CHECK(fh_pica_stream_draw(&world,&live,0)==30);drain();
    CHECK(registers[GPUREG_FRAGOP_ALPHA_TEST]==(description.effect.alphaTest^1));
    live.effect=NULL;CHECK(fh_pica_stream_draw(&world,&live,0)==48);drain();
    CHECK(registers[GPUREG_FRAGOP_ALPHA_TEST]==description.effect.alphaTest);
    fh_pica_stream_end();C3Di_UpdateContext();drain();verify_restored();reset_commands();
    ctx->effect.stencilMode=0;ctx->effect.stencilOp=0;ctx->effect.blendClr=0;ctx->effect.clrLogicOp=0;
    ctx->effect.fragOpMode=0x100;ctx->effect.fragOpShadow=0;ctx->effect.earlyDepth=0;
    ctx->effect.earlyDepthFunc=0;ctx->effect.earlyDepthRef=0;ctx->flags|=C3DiF_Effect;
    C3Di_UpdateContext();drain();
    for(unsigned round=0;round<60;round++){
        /* Seed the installed-value caches, then interrupt with foreign raw
         * state. Restoring equal CPU values must not be optimized away. */
        const void *data[3]={(void *)0x18010000,0,0};CHECK(fh_pica_buffers_bind(&sdk_layout,data));
        C3Di_UpdateContext();drain();CHECK(fh_cpu_layout==&sdk_layout && fh_gpu_layout==&sdk_layout);
        ctx->flags&=~C3DiF_DrawUsed;snapshot();
        const unsigned untouched=(1u<<0)|(1u<<1)|(1u<<9)|(1u<<10)|(1u<<13);
        unsigned retained=fh_raster.valid&untouched;
        CHECK(fh_pica_stream_begin(&state));CHECK(!fh_pica_stream_begin(&state));
        CHECK(!fh_pica_stream_begin(NULL));
        unsigned flags=ctx->flags,cursor=gpuCmdBufOffset;
        gpuCmdBufSize=cursor+1;FhPicaStreamState before=state;
        CHECK(!fh_pica_stream_draw(&draw,&live,0));
        CHECK(gpuCmdBufOffset==cursor && ctx->flags==flags && !memcmp(&before,&state,sizeof before));
        gpuCmdBufSize=4096;
        CHECK(fh_pica_stream_draw(&draw,&live,0));CHECK(ctx->flags&C3DiF_DrawUsed);drain();
        CHECK(fh_pica_stream_draw(&draw,&live,0)==30);drain();
        CHECK(uniforms[0][7*4]!=saved_uniforms[0][7*4]);
        if(round%3==0){fh_pica_stream_end();CHECK((fh_raster.valid&untouched)==retained);}
        else if(round%3==1)C3D_FrameSplit(GX_CMDLIST_FLUSH);
        /* For the third case UpdateContext itself closes the raw batch. */
        C3Di_UpdateContext();drain();CHECK(!fh_stream_owner);verify_restored();
        CHECK(!fh_pica_stream_draw(&draw,&live,0));
        reset_commands(); /* Mock consumer has retired this complete batch. */
    }
    /* Empty batches must not force dirty state or invalidate installed rows. */
    CHECK(fh_pica_stream_begin(&state));unsigned flags=ctx->flags;
    fh_pica_stream_end();CHECK(ctx->flags==flags);CHECK(!C3D_FVUnifDirty[0][7]);
    /* A pending ordinary SDK writer cannot be silently drawn through. */
    CHECK(fh_pica_stream_begin(&state));C3D_CullFace(GPU_CULL_BACK_CCW);
    unsigned cursor=gpuCmdBufOffset;CHECK(!fh_pica_stream_draw(&draw,&live,0));CHECK(cursor==gpuCmdBufOffset);
    C3Di_UpdateContext();drain();CHECK(!fh_stream_owner);CHECK(registers[GPUREG_FACECULLING_CONFIG]==GPU_CULL_BACK_CCW);
    /* Actual target and APT restoration publishers also terminate the batch. */
    for(unsigned kind=0;kind<2;kind++){
        C3Di_UpdateContext();drain();snapshot();CHECK(fh_pica_stream_begin(&state));
        CHECK(fh_pica_stream_draw(&draw,&live,0));drain();
        if(kind)C3Di_AptEventHook(APTHOOK_ONRESTORE,NULL);
        else ctx->flags|=C3DiF_FrameBuf;
        C3Di_UpdateContext();drain();CHECK(!fh_stream_owner);verify_restored();
    }
    /* Preserve the existing real publication-order GS->VSH boundary. The
     * queue consumer is mocked; the SDK context and split code execute. */
    C3D_BindProgram(&geometry);CHECK(!fh_pica_stream_begin(&state));
    C3D_DrawElements(GPU_TRIANGLES,3,C3D_UNSIGNED_SHORT,(void *)0x18030000);drain();
    unsigned splits=split_count;C3D_BindProgram(&plain);
    CHECK(fh_pica_stream_begin(&state));CHECK(split_count==splits+1);drain();
    CHECK(fh_pica_stream_draw(&draw,&live,0));drain();fh_pica_stream_end();
    C3Di_UpdateContext();drain();
    float24Uniform_s reserved={.id=7};vshader.numFloat24Uniforms=1;vshader.float24Uniforms=&reserved;
    cursor=gpuCmdBufOffset;CHECK(!fh_pica_stream_begin(&state));CHECK(cursor==gpuCmdBufOffset);
    vshader.numFloat24Uniforms=0;vshader.float24Uniforms=NULL;
    flags=ctx->flags;ctx->flags=0;CHECK(!fh_pica_stream_begin(&state));ctx->flags=flags;
    CHECK(fh_pica_stream_begin(&state));C3D_Fini();CHECK(!fh_stream_owner && !ctx->flags);
    CHECK(draw_count==128);test_pass();
}
