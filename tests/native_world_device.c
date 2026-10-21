/* Actual native world submission, isolated from game loading and texture IO. */
#include <3ds.h>
#include <citro3d.h>
#include "../reference/citro3d/source/internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_minimal_graphics.h"
#include "fh_world_index_stream.h"
#include "minimal_world_shader.h"
#include "../native-3ds/vendor/melee/scissor.h"
u32 __ctru_heap_size=8*1024*1024,__ctru_linear_heap_size=8*1024*1024;
unsigned fh_minimal_movie_active,fh_minimal_rejected,fh_minimal_draws,fh_minimal_indices;
FhTexture fh_minimal_movie_texture;
static unsigned initialized=1,world_draws,world_layered_draws,world_deferred_draws,world_movie_draws,world_failures;
static unsigned world_sampler_resets,world_falloff_draws;
static unsigned world_cull_checks,world_culled_indices,world_culled_groups,world_split_draws;
static FhWorldIndexStream world_stream;
static unsigned world_gather_draws,world_gather_fallbacks,world_gather_saved_draws;
static u64 world_gather_bytes;
static unsigned frame_draws,frame_indices,movie_draws;
static u64 world_ticks,submit_ticks;
static shaderProgram_s world_program;
static C3D_RenderTarget *target;
static C3D_Tex texture,*base_pin,*second_pin;
#ifdef FH_WATER_FIXTURE
#include "minimal_water_shader.h"
static C3D_Tex wave;
static shaderProgram_s water_program;
#ifdef FH_WATER_EFFECT_FIXTURE
typedef FhMinimalEffectVertex MinimalUiVertex;
static C3D_Tex foam;
enum { UI_VERTICES=1536 };
static MinimalUiVertex *ui_vertices;
static unsigned ui_used;
#endif
#endif
#ifdef FH_SURFACE_FIXTURE
#include "minimal_ui_shader.h"
static shaderProgram_s copy_program;
static void fh_gpu_cache_flush(const void *p,size_t n){GSPGPU_FlushDataCache(p,n);}
#endif
#ifdef FH_WORLD_FALLOFF_FIXTURE
static C3D_Tex falloff_mask,falloff_ramp;
#endif
static float projection[4][4];
static void fh_platform_panic(const char *s){FILE *f=fopen("sdmc:/3ds/foxhollow/panic.txt","w");if(f){fputs(s,f);fclose(f);}exit(1);}
void fh_early_queue_capacity_completed(u64 wait_ticks){(void)wait_ticks;} /* No early queues. */
static void begin(void){}
static void flush_ui(void){}
void fh_minimal_sync(void){fh_platform_panic("Unexpected fixture flush");}
static C3D_Tex *base_texture(const FhTexture *s){
#ifdef FH_WATER_FIXTURE
if(s==&fh_minimal_water.caustic)return &wave;
#ifdef FH_WATER_EFFECT_FIXTURE
if(s==&fh_minimal_water_effect.mask)return &wave;
if(s==&fh_minimal_splash.mask)return &wave;
if(s==&fh_minimal_splash.foam)return &foam;
#endif
#endif
if(s!=&fh_minimal_movie_texture)fh_platform_panic("Wrong movie source");return &texture;}
static C3D_Tex *surface_texture(const FhTexture *s){
#ifdef FH_WORLD_FALLOFF_FIXTURE
if(s && s->pixels==&falloff_mask)return &falloff_mask;
if(s && s->pixels==&falloff_ramp)return &falloff_ramp;
#endif
return s?&texture:NULL;}
static float base_uv_scale(unsigned logical,unsigned physical){return (float)logical/physical;}
static void fh_native_fog_apply(unsigned enabled){(void)enabled;C3D_FogGasMode(GPU_NO_FOG,0,false);}
void fh_native_scene_used(const FhSceneAsset *asset){(void)asset;}
/* This material fixture has no scene-cache bounds. Culling and stream
 * ownership are exercised separately by their production-loop tests. */
const FhWorldBounds *fh_native_scene_world_bounds(const FhSceneAsset *asset,
    const FhSceneGroup *group,unsigned *first,unsigned *total){
    (void)asset;(void)group;(void)first;(void)total;return NULL;
}
#include "../native-3ds/source/platform_native_world_light.h"
#include "world_submission.inc"
#ifdef FH_SURFACE_FIXTURE
#include "../native-3ds/source/platform_native_surface.h"
#endif
#ifdef FH_WATER_FIXTURE
#include "../native-3ds/source/platform_native_water.h"
#ifdef FH_WATER_EFFECT_FIXTURE
#include "../native-3ds/source/platform_native_water_effect.h"
#endif
#endif
static unsigned morton(unsigned x,unsigned y){unsigned r=0;for(unsigned i=0;i<3;i++)r|=((x>>i)&1)<<(2*i)|((y>>i)&1)<<(2*i+1);return r;}
int main(void){
    gfxInitDefault();gfxSetScreenFormat(GFX_TOP,GSP_RGBA8_OES);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))fh_platform_panic("C3D init");
    DVLB_s *shader=DVLB_ParseFile((u32*)minimal_world_shader,sizeof(minimal_world_shader));
    if(!shader)fh_platform_panic("Shader parse");
    shaderProgramInit(&world_program);shaderProgramSetVsh(&world_program,&shader->DVLE[0]);
#ifdef FH_SURFACE_FIXTURE
    target=native_surface_create();
    DVLB_s *copy_binary=DVLB_ParseFile((u32*)minimal_ui_shader,sizeof(minimal_ui_shader));
    if(!copy_binary)fh_platform_panic("Copy shader parse");
    shaderProgramInit(&copy_program);shaderProgramSetVsh(&copy_program,&copy_binary->DVLE[0]);
#else
    target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    if(target)C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,0);
#endif
    FhModelVertex *floats=linearAlloc(4*sizeof(*floats));
    FhWorldCompactVertex *packed=linearAlloc(4*sizeof(*packed));
    u16 *indices=linearAlloc(12);
    if(!target || !floats || !packed || !indices || !C3D_TexInit(&texture,8,8,GPU_RGBA8))fh_platform_panic("Fixture allocation");
    C3D_TexSetFilter(&texture,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&texture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
#ifdef FH_WATER_FIXTURE
    DVLB_s *water_shader=DVLB_ParseFile((u32*)minimal_water_shader,sizeof(minimal_water_shader));
    if(!water_shader || !C3D_TexInit(&wave,8,8,GPU_RGBA8))fh_platform_panic("Water fixture allocation");
    shaderProgramInit(&water_program);shaderProgramSetVsh(&water_program,&water_shader->DVLE[0]);
    memset(wave.data,255,wave.size);C3D_TexFlush(&wave);
    fh_minimal_water=(FhMinimalWaterMaterial){.caustic={.pixels=&wave,.width=8,.height=8,.format=6},.valid=1};
#ifdef FH_WATER_EFFECT_FIXTURE
    ui_vertices=linearAlloc(UI_VERTICES*sizeof(*ui_vertices));if(!ui_vertices)fh_platform_panic("Effect stream");
    if(!C3D_TexInit(&foam,8,8,GPU_RGBA8))fh_platform_panic("Foam texture");
    C3D_TexSetFilter(&foam,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&foam,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
#endif
#endif
    const u8 colors[4][4]={{216,40,72,255},{24,208,96,255},{40,64,224,255},{232,200,32,255}};
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
        const u8 *c=colors[(x>=4)+2*(y>=4)];u8 *p=(u8*)texture.data+morton(x,y)*4;
        p[0]=c[3];p[1]=c[2];p[2]=c[1];p[3]=c[0];
    }
    C3D_TexFlush(&texture);
#ifdef FH_WORLD_FALLOFF_FIXTURE
    if(!C3D_TexInit(&falloff_mask,128,128,GPU_RGBA8) || !C3D_TexInit(&falloff_ramp,256,8,GPU_RGBA8))fh_platform_panic("Falloff allocation");
    for(unsigned t=0;t<2;t++){
        C3D_Tex *f=t?&falloff_ramp:&falloff_mask;
        C3D_TexSetFilter(f,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(f,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
        for(unsigned y=0;y<f->height;y++)for(unsigned x=0;x<f->width;x++){
            float d=hypotf((x-64.f)/64,(y-64.f)/64);
            unsigned green=t?255-x:d<.5f?160:d>1?0:(unsigned)(320*(1-d));
            unsigned offset=((y/8)*(f->width/8)+x/8)*64+morton(x&7,y&7);
            u8 *pixel=(u8*)f->data+offset*4;pixel[0]=55;pixel[1]=239;pixel[2]=green;pixel[3]=13;
        }
        C3D_TexFlush(f);
    }
#endif
    const float xy[4][2]={{-.8f,-.8f},{.8f,-.8f},{.8f,.8f},{-.8f,.8f}};
    const u16 ix[6]={0,2,1,0,3,2};memcpy(indices,ix,sizeof(ix));
    memset(floats,0,4*sizeof(*floats));memset(packed,0,4*sizeof(*packed));
    for(unsigned i=0;i<4;i++){
        /* Exactly representable positions avoid comparing quantized geometry. */
        packed[i].position[0]=xy[i][0]<0?-192:192;packed[i].position[1]=xy[i][1]<0?-192:192;
        packed[i].position[2]=-128;packed[i].position[3]=1;
        for(unsigned j=0;j<3;j++)floats[i].position[j]=packed[i].position[j]/256.f;floats[i].position[3]=1;
        for(unsigned j=0;j<4;j++){packed[i].color[j]=15;floats[i].color[j]=1;}
        packed[i].uv[0]=(i==1 || i==2)?1024:0;packed[i].uv[1]=i>=2?1024:0;
        for(unsigned j=0;j<2;j++)floats[i].uv[j]=packed[i].uv[j]/1024.f;
    }
    GSPGPU_FlushDataCache(floats,4*sizeof(*floats));GSPGPU_FlushDataCache(packed,4*sizeof(*packed));GSPGPU_FlushDataCache(indices,12);
    for(unsigned i=0;i<4;i++)projection[i][i]=1;
    float view[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};const unsigned scissor[4]={0,0,640,480};
    fh_minimal_movie_texture=(FhTexture){.pixels=colors,.width=8,.height=8,.format=6};
#ifdef FH_SURFACE_FIXTURE
    C3D_Tex original=texture;
#endif
#ifdef FH_WORLD_FALLOFF_FIXTURE
    const unsigned passes=10;
#elif defined(FH_WORLD_LIGHT_FIXTURE)
    const unsigned passes=12;
#elif defined(FH_WATER_EFFECT_FIXTURE)
    const unsigned passes=16;
#elif defined(FH_WATER_FIXTURE)
    const unsigned passes=8;
#else
    const unsigned passes=4;
#endif
    for(unsigned pass=0;pass<passes;pass++){
#ifdef FH_WORLD_FALLOFF_FIXTURE
        unsigned fading=pass>=4 && pass<=8;
        view[0][0]=view[1][1]=fading?50:1;view[2][2]=fading?100:1;
        projection[0][0]=projection[1][1]=fading?.02f:1;projection[2][2]=fading?.01f:1;
        for(unsigned i=0;i<4;i++){packed[i].color[3]=fading?3:15;floats[i].color[3]=fading?.2f:1;}
        GSPGPU_FlushDataCache(floats,4*sizeof(*floats));GSPGPU_FlushDataCache(packed,4*sizeof(*packed));
#endif
        if(!C3D_FrameBegin(0))fh_platform_panic("Frame begin");
        C3D_FrameDrawOn(target);
#ifdef FH_SURFACE_FIXTURE
        native_surface_draw_on(target);
        texture=(pass&1)?native_surface_reflection:original;
        for(unsigned i=0;i<4;i++){
            float u=(pass&1)?(.125f+.75f*(1-floats[i].position[1]/.75f)*.5f):((i==1 || i==2)?1:0);
            float v=(pass&1)?(.125f+.75f*(1-floats[i].position[0]/.75f)*.5f):(i>=2?1:0);
            floats[i].uv[0]=u;floats[i].uv[1]=v;packed[i].uv[0]=u*1024;packed[i].uv[1]=v*1024;
        }
        fh_gpu_cache_flush(floats,4*sizeof(*floats));fh_gpu_cache_flush(packed,4*sizeof(*packed));
        fh_minimal_movie_texture.width=texture.width;fh_minimal_movie_texture.height=texture.height;
#endif
#ifdef FH_WATER_FIXTURE
        projection[3][2]=0;fh_minimal_water.heavy_fog=0;memset(fh_minimal_water.tint,0,4);
        for(unsigned i=0;i<4;i++){
            packed[i].position[0]=xy[i][0]<0?-192:192;packed[i].position[1]=xy[i][1]<0?-192:192;
            packed[i].position[2]=-128;packed[i].color[3]=15;
        }
        fh_gpu_cache_flush(packed,4*sizeof(*packed));
        if(pass==5){const u8 tint[4]={64,96,128,128};memcpy(fh_minimal_water.tint,tint,4);fh_minimal_water.heavy_fog=1;}
        if(pass==7){
            const u8 tint[4]={16,8,24,80};memcpy(fh_minimal_water.tint,tint,4);
            for(unsigned i=0;i<64;i++){u8 *p=(u8*)wave.data+4*i;p[0]=255;p[1]=p[2]=p[3]=128;}
            C3D_TexFlush(&wave);
        }
#endif
        C3D_RenderTargetClear(target,C3D_CLEAR_ALL,0x142030ff+(pass<<24),0);
        FhSceneAsset asset={.block=1,.direct=1,.compact=pass&1};
        FhSceneGroup group={.format=6,.indices=6,.vertex=(const FhModelVertex*)((pass&1)?(void*)packed:(void*)floats),.index=indices};
        fh_minimal_movie_active=pass<2;
#ifndef FH_WATER_FIXTURE
        /* A preceding layered model can leave unit2 enabled. The world
         * shader exports only texcoord0/1: do not inherit its third sampler. */
        C3D_TexBind(2,&texture);
#endif
#ifdef FH_WATER_FIXTURE
#ifdef FH_WATER_EFFECT_FIXTURE
        if(pass>=12 && (pass&1)){
            FhMinimalEffectVertex vertices[630]={0};
            for(unsigned i=0;i<6;i++){
                /* Opposite triangle windings exercise both cull passes. */
                unsigned j=ix[i==3?4:i==4?3:i];
                vertices[i]=(FhMinimalEffectVertex){.position={floats[j].position[0],floats[j].position[1],-.5f,1},
                    .color={1,1,1,128.f/255},.uv={floats[j].uv[0]/12,floats[j].uv[1]}};
            }
            for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
                u8 *p=(u8*)foam.data+4*morton(x,y);
                p[0]=pass==13?64:(x>=4?192:64);p[1]=7;p[2]=241;p[3]=13;
                p=(u8*)wave.data+4*morton(x,y);p[0]=192;p[1]=32;p[2]=64;p[3]=224;
            }
            C3D_TexFlush(&wave);C3D_TexFlush(&foam);
            fh_minimal_splash=(FhMinimalSplashMaterial){.mask={.pixels=&wave,.width=8,.height=8,.format=6},
                .foam={.pixels=&foam,.width=8,.height=8,.format=6},.ambient={16,8,24,255},
                .weight={{128,0,0,255},{0,192,0,255},{0,0,96,255}},.valid=1};
            fh_minimal_splash_draw(vertices,scissor);
        }else if(pass>=8 && (pass&1)){
            FhMinimalEffectVertex vertices[6];
            for(unsigned i=0;i<6;i++){
                unsigned j=ix[i];
                vertices[i]=(FhMinimalEffectVertex){.position={floats[j].position[0],floats[j].position[1],-.5f,1},
                    .color={1,1,1,128.f/255},.uv={0,0}};
            }
            for(unsigned i=0;i<64;i++){u8 *p=(u8*)wave.data+4*i;p[0]=192;p[1]=32;p[2]=64;p[3]=224;}
            C3D_TexFlush(&wave);
            fh_minimal_water_effect=(FhMinimalWaterEffect){.mask={.pixels=&wave,.width=8,.height=8,.format=6},
                .tint={16,8,24,128},.mix={64,128,192,255},.valid=1,.green_mask=pass==9,.heavy_fog=pass==11};
            fh_minimal_water_effect_draw(vertices,view,scissor);
        }else
#endif
        if(pass&1){
            if(pass==3){
                /* Unequal homogeneous W catches vertex-division shortcuts.
                 * Preserve the same projected quad, use original RGBA4 alpha. */
                projection[3][2]=1;
                for(unsigned i=0;i<4;i++){
                    float z=(i<2)?-.5f:-.25f,w=1+z;
                    packed[i].position[0]=(xy[i][0]<0?-192:192)*w;
                    packed[i].position[1]=(xy[i][1]<0?-192:192)*w;
                    packed[i].position[2]=z*256;packed[i].color[3]=8;
                }
                fh_gpu_cache_flush(packed,4*sizeof(*packed));
            }
            if(!fh_minimal_water_group(&asset,&group,view,scissor))fh_platform_panic("Water group");
        }else
#endif
        if(pass<2){if(!fh_minimal_group(&asset,&group,NULL,0,view,1,NULL,NULL,0,0,0,NULL,scissor,NULL,NULL))fh_platform_panic("World movie route");}
        else{FhMinimalWorldMaterial m={.raster={1,1,1,1}};m.texture[0]=fh_minimal_movie_texture;
#ifdef FH_WORLD_LIGHT_FIXTURE
            if(pass>=4 && pass<=10){
                unsigned alpha=(pass==7 || pass==9 || pass==10)?6:15;
                for(unsigned i=0;i<4;i++){
                    packed[i].color[3]=alpha;floats[i].color[3]=alpha/15.f;
                    for(unsigned j=0;j<2;j++){packed[i].second_uv[j]=packed[i].uv[j];floats[i].normal[j]=floats[i].uv[j];}
                }
                GSPGPU_FlushDataCache(floats,4*sizeof(*floats));GSPGPU_FlushDataCache(packed,4*sizeof(*packed));
                m.lighting=(FhMinimalWorldLights){.count=pass==4?1:pass==8 || pass==9?0:2,
                    .mode=pass==6 || pass==8?1:pass==7 || pass==9 || pass==10?2:0,
                    .light={{{.15f,-.12f,-.2f},.8f,{180,80,20,255}},{{-.3f,.2f,-.4f},.6f,{20,90,180,255}}}};
                if(pass==10){m.layered=group.two_uv=1;m.texture[1]=m.texture[0];
                    const u8 k[4]={128,192,64,170};memcpy(m.konst,k,4);}
            }
#endif
#ifdef FH_WORLD_FALLOFF_FIXTURE
            if(fading){m.flags=0x40000000;m.falloff.active=1;m.falloff.distance=pass<=5?60:pass<=7?55:50;
                m.falloff.texture[0]=(FhTexture){.pixels=&falloff_mask,.width=128,.height=128,.format=6};
                m.falloff.texture[1]=(FhTexture){.pixels=&falloff_ramp,.width=256,.height=4,.format=6};}
#endif
            if(!fh_minimal_world_group(&asset,&group,view,&m,scissor))fh_platform_panic("World normal route");}
#ifdef FH_SURFACE_FIXTURE
        if(!(pass&1))native_surface_capture(target,&copy_program);
        native_surface_present();
#endif
#ifndef FH_WATER_FIXTURE
#ifdef FH_WORLD_FALLOFF_FIXTURE
        if(!fading && C3Di_GetContext()->tex[2])fh_platform_panic("World inherited model texture unit 2");
#else
        if(C3Di_GetContext()->tex[2])fh_platform_panic("World inherited model texture unit 2");
#endif
#endif
        C3D_FrameEnd(0);C3D_FrameBegin(0);C3D_FrameEnd(0);
        /* Observe the same top-screen output as game tests. The fixture's
         * former direct VRAM-to-linear readback produced striped files even
         * while the displayed image was correct. No CPU GPU-memory readback. */
        gspWaitForVBlank();gspWaitForVBlank();
        char name[100];snprintf(name,sizeof(name),"sdmc:/3ds/foxhollow/ready-%u.txt",pass);
        FILE *f=fopen(name,"w");if(!f)fh_platform_panic("Ready marker");fputs("ready\n",f);fclose(f);
        snprintf(name,sizeof(name),"sdmc:/3ds/foxhollow/ack-%u.txt",pass);
        u64 start=svcGetSystemTick();
        while(!(f=fopen(name,"r"))){if(svcGetSystemTick()-start>10ULL*SYSCLOCK_ARM11)fh_platform_panic("Capture acknowledgement timeout");svcSleepThread(10000000);}
        fclose(f);
    }
    C3D_FrameBegin(0);C3D_FrameEnd(0);
    FILE *f=fopen("sdmc:/3ds/foxhollow/done.txt","w");if(f){
#ifdef FH_WATER_EFFECT_FIXTURE
fprintf(f,"world=%u movie=%u water=%u effects=%u splashes=%u failed=%u\n",world_draws,world_movie_draws,water_draws,water_effect_draws,splash_draws,world_failures+water_failures+water_effect_failures);
#elif defined(FH_WATER_FIXTURE)
fprintf(f,"world=%u movie=%u water=%u failed=%u\n",world_draws,world_movie_draws,water_draws,world_failures+water_failures);
#else
fprintf(f,"world=%u movie=%u failed=%u\n",world_draws,world_movie_draws,world_failures);
#endif
fclose(f);}
    while(aptMainLoop())svcSleepThread(10000000);
    return 0;
}
