/* Actual Citro3D publisher and uniform cache, four physical register banks.
 * The sink models documented VSH broadcast only when GS is disabled.
 * This is a software ownership regression, not physical-device proof. */
#include FH_RASTER_SOURCE
#include FH_UNIFORM_SOURCE
#include "../reference/citro3d/source/effect.c"
#include "../reference/citro3d/source/texenv.c"
#include "../reference/citro3d/source/attribs.c"
volatile unsigned test_checks,test_failure;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){test_failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);++test_checks;}while(0)
static bool geometry;
static u32 bank[4][96][4],row[2],lane[2];
static u32 bool_bank[4];
static u32 pending[2][4];
static unsigned width[2];
void GPUCMD_Add(u32 header,const u32 *values,u32 count){
    unsigned base=header&1023;
    for(unsigned i=0;i<count;i++){
        unsigned reg=base+((header>>31)?i:0),stage;
        if(reg==GPUREG_VSH_BOOLUNIFORM || reg==GPUREG_GSH_BOOLUNIFORM){
            unsigned gs=reg==GPUREG_GSH_BOOLUNIFORM;
            for(unsigned unit=gs?3:0;unit<(gs?4:geometry?3:4);unit++)
                bool_bank[unit]=values[i]&0xffff;
            continue;
        }
        if(reg>=GPUREG_VSH_FLOATUNIFORM_CONFIG && reg<=GPUREG_VSH_FLOATUNIFORM_DATA+7)stage=0;
        else if(reg>=GPUREG_GSH_FLOATUNIFORM_CONFIG && reg<=GPUREG_GSH_FLOATUNIFORM_DATA+7)stage=1;
        else continue;
        unsigned config=stage?GPUREG_GSH_FLOATUNIFORM_CONFIG:GPUREG_VSH_FLOATUNIFORM_CONFIG;
        if(reg==config){row[stage]=values[i]&255;lane[stage]=0;width[stage]=(values[i]>>31)?4:3;continue;}
        CHECK(row[stage]<96);pending[stage][lane[stage]++]=values[i];
        if(lane[stage]==width[stage]){
            if(width[stage]==3)pending[stage][3]=0x24; /* Distinct constant representation. */
            unsigned first=stage?3:0,end=stage?4:geometry?3:4;
            for(unsigned unit=first;unit<end;unit++)memcpy(bank[unit][row[stage]],pending[stage],16);
            row[stage]++;lane[stage]=0;
        }
    }
}
void C3D_FrameSplit(u8 flags){CHECK(flags==GX_CMDLIST_FLUSH);C3Di_GetContext()->flags&=~C3DiF_DrawUsed;}
Result shaderProgramConfigure(shaderProgram_s *p,bool v,bool g){(void)v;(void)g;geometry=p->geometryShader!=NULL;return 0;}
void C3Di_FrameBufBind(C3D_FrameBuf *f){(void)f;}
void C3Di_BufInfoBind(C3D_BufInfo *b){(void)b;}
void C3Di_SetTex(int u,C3D_Tex *t){(void)u;(void)t;}
void C3D_ImmSendAttrib(float x,float y,float z,float w){(void)x;(void)y;(void)z;(void)w;}
void C3Di_RenderQueueEnableVBlank(void){}
void C3Di_RenderQueueDisableVBlank(void){}
void C3Di_RenderQueueWaitDone(void){}
static DVLP_s dvlp;
static DVLE_s vd,gd;
static shaderInstance_s vs,gs;
static shaderProgram_s plain,particle;
static void draw(void){C3Di_UpdateContext();C3Di_GetContext()->flags|=C3DiF_DrawUsed;}
static void same(unsigned reg){
    for(unsigned u=0;u<4;u++)CHECK(!memcmp(bank[u][reg],&C3D_FVUnif[0][reg],16));
}
static void actor_same(void){
    /* Actual resident actor ABI spans camera, decode, palette and lighting.
     * Equal CPU shadows must be republished after a GS draw on every unit. */
    for(unsigned r=0;r<94;r++)same(r);
    for(unsigned u=0;u<4;u++)CHECK(bool_bank[u]==C3D_BoolUnifs[0]);
}
void test_main(void){
    C3D_Context *ctx=C3Di_GetContext();memset(ctx,0,sizeof(*ctx));ctx->flags=C3DiF_Active;
    vd.type=GPU_VERTEX_SHADER;vd.dvlp=&dvlp;gd.type=GPU_GEOMETRY_SHADER;gd.dvlp=&dvlp;
    vs.dvle=&vd;gs.dvle=&gd;plain.vertexShader=&vs;particle.vertexShader=&vs;particle.geometryShader=&gs;
    C3D_BindProgram(&plain);
    for(unsigned r=0;r<94;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,r+1,r+2,r+3,r+4);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,10,1,2,3,4);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,true);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68+2,true);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68+12,true);
    CHECK(C3D_BoolUnifs[GPU_VERTEX_SHADER]==0x1005);
    draw();actor_same();
    for(unsigned n=0;n<64;n++){
        C3D_BindProgram(&particle);
        for(unsigned r=0;r<94;r++)C3D_FVUnifSet(GPU_GEOMETRY_SHADER,r,100+n+r,200,300,400);
        C3D_BoolUnifSet(GPU_GEOMETRY_SHADER,0x68,(n&1)!=0);
        draw();
        CHECK(memcmp(bank[3][10],bank[0][10],16));
        C3D_BindProgram(&plain);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,10,1,2,3,4); /* Equal cached VSH value. */
        if(n&1)ctx->flags&=~C3DiF_DrawUsed; /* Already split is still a rejoin. */
        draw();actor_same();
        /* Bind GS, update it, then bind away WITHOUT drawing. */
        C3D_BindProgram(&particle);C3D_FVUnifSet(GPU_GEOMETRY_SHADER,10,900,901,902,903);
        C3D_BoolUnifSet(GPU_GEOMETRY_SHADER,0x68+2,true);
        C3D_BindProgram(&plain);C3D_FVUnifSet(GPU_VERTEX_SHADER,10,1,2,3,4);draw();actor_same();
        /* Deferred GS values still arrive at their next genuine draw. */
        C3D_BindProgram(&particle);draw();CHECK(!memcmp(bank[3][10],&C3D_FVUnif[1][10],16));
        C3D_BindProgram(&plain);draw();actor_same();
    }
    CHECK(fh_geometry_boundary_edges==128);test_pass();
}
