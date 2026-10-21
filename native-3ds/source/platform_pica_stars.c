#include "fh_graphics_options.h"
/* Persistent star points, expanded/batched by the vertex shader. Original
 * group ordering, RNG and appearance parameters are supplied by game state. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include "fh_pica_stars.h"
#include "fh_pica_scene.h"
#include "fh_pica_hud.h"
#include "fh_pica_image.h"
#include "fh_gpu_cache.h"
#include "fh_minimal_graphics.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"
enum { GROUPS=92, POINTS=50, VERTICES=GROUPS*POINTS*4, INDICES=GROUPS*POINTS*6 };
typedef struct {int16_t position[3];uint8_t group,corner;} Star;
_Static_assert(sizeof(Star)==8,"Compact star vertex ABI");
static Star *vertices;
static uint16_t *indices;
static unsigned colors[GROUPS],sizes[GROUPS];
typedef struct {C3D_Tex texture;unsigned last_used;} Palette;
static Palette palettes[3];
static unsigned palette_upload_bytes,palette_drains;
static shaderProgram_s program;
static int textured_uniform,live_points_uniform;
static unsigned ready,points,frames,draws,submitted;
void fh_pica_stars_init(void *entry){
    shaderProgramInit(&program);shaderProgramSetVsh(&program,(DVLE_s*)entry);
    textured_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"textured");
    live_points_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"live_points");
    if(shaderInstanceGetUniformLocation(program.vertexShader,"sizes")!=7 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"corners")!=23 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"pixels")!=28 || textured_uniform<0 || live_points_uniform<0)
        fh_platform_panic("PICA star uniform ABI");
}
void fh_pica_stars_allocate(void){
    extern u32 __ctru_linear_heap_size;
    if(vertices || indices)fh_platform_panic("PICA stars duplicate initialization");
    const unsigned bytes=VERTICES*sizeof(Star)+INDICES*sizeof(uint16_t)+3*512;
    if(bytes>linearSpaceFree() || linearSpaceFree()-bytes<__ctru_linear_heap_size*15/100)
        fh_platform_panic("PICA star linear reserve");
    vertices=linearMemAlign(VERTICES*sizeof(Star),128);
    indices=linearMemAlign(INDICES*sizeof(uint16_t),128);
    if(!vertices || !indices)fh_platform_panic("PICA star allocation");
    for(unsigned i=0;i<3;i++){
        if(!C3D_TexInit(&palettes[i].texture,16,8,GPU_RGBA8))fh_platform_panic("PICA star palette allocation");
        C3D_TexSetFilter(&palettes[i].texture,GPU_NEAREST,GPU_NEAREST);
        C3D_TexSetWrap(&palettes[i].texture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    }
}
void fh_pica_stars_point(unsigned group,unsigned point,int16_t x,int16_t y,int16_t z){
    if(ready || !vertices || group>=GROUPS || point>=POINTS || points!=group*POINTS+point)
        fh_platform_panic("PICA star preparation order");
    unsigned base=points*4;const unsigned order[6]={0,1,2,2,1,3};
    for(unsigned c=0;c<4;c++)vertices[base+c]=(Star){{x,y,z},group<76?group:group-76,c};
    for(unsigned c=0;c<6;c++)indices[points*6+c]=base+order[c];
    points++;
}
void fh_pica_stars_commit(void){
    if(points!=GROUPS*POINTS || ready)fh_platform_panic("PICA star incomplete preparation");
    fh_gpu_cache_flush(vertices,VERTICES*sizeof(Star));fh_gpu_cache_flush(indices,INDICES*sizeof(uint16_t));ready=1;
}
void fh_pica_stars_group(unsigned group,unsigned rgba,unsigned size){
    if(group>=GROUPS || size>255)fh_platform_panic("PICA star group parameters");
    colors[group]=rgba;sizes[group]=size;
}
/* Native RGBA8 is ABGR byte order in little-endian memory. Two 8x8 tiles
 * hold every group color; only live material parameters change. */
static void fill_palette(unsigned *pixels,const unsigned *rgba){
    for(unsigned group=0;group<GROUPS;group++){
        unsigned x=group&15,y=group/16;
        unsigned at=(x/8)*64+(x&1)+((x&2)<<1)+((x&4)<<2)+((y&1)<<1)+((y&2)<<2)+((y&4)<<3);
        pixels[at]=rgba[group];
    }
}
void fh_pica_stars_draw(unsigned first,unsigned page_a,unsigned page_b,
    const float projection[4][4],const float view[3][4],const float viewport[6],const unsigned scissor[4]){
    if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_SKY))return;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    if(!ready || (first!=0 && first!=76) || !(viewport[2]>0 && viewport[3]>0))
        fh_platform_panic("PICA star draw state");
    fh_pica_hud_flush();unsigned clip[4];if(!fh_pica_hud_target(scissor,clip))return;
    Palette *palette=palettes+frames%3;
    if(palette->last_used){
        if(palette->last_used==fh_minimal_frames+1)palette_drains++;
        fh_minimal_prepare_write(palette->last_used);
    }
    fill_palette(palette->texture.data,colors);
    fh_gpu_cache_flush(palette->texture.data,512);palette_upload_bytes+=512;
    frames++;
    for(unsigned begin=first;begin<GROUPS;){
        unsigned end=76,which=0;
        if(begin<76){}
        else if(begin<84){end=84;which=page_a;}
        else {end=GROUPS;which=page_b;}
        float scale[2]={1,1};unsigned intensity=0;
        const C3D_Tex *texture=which?fh_pica_hud_sampler(which,scale,&intensity):NULL;
        fh_pica_scene_reserve();fh_pica_scene_invalidate();
        C3D_BindProgram(&program);C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,textured_uniform,which!=0);
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,live_points_uniform,false);
        C3D_TexEnvBufUpdate(C3D_Both,0);
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        C3D_TexEnv *e=C3D_GetTexEnv(0);
        C3D_TexEnvSrc(e,C3D_Both,GPU_TEXTURE1,GPU_TEXTURE0,0);
        C3D_TexEnvFunc(e,C3D_Both,which?GPU_MODULATE:GPU_REPLACE);
        if(intensity)C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
        C3D_TexBind(0,(C3D_Tex*)texture);C3D_TexBind(1,&palette->texture);C3D_TexBind(2,NULL);
        C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
        C3D_CullFace(GPU_CULL_NONE);C3D_DepthMap(true,-1,0);
        C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE,GPU_ONE,GPU_ZERO);
        for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0],view[r][1],view[r][2],view[r][3]);
        for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
        if(which)for(unsigned g=begin;g<end;g++)C3D_FVUnifSet(GPU_VERTEX_SHADER,7+g-76,sizes[g],0,0,0);
        const float corners[4][2]={{-.5f,-.5f},{.5f,-.5f},{-.5f,.5f},{.5f,.5f}};
        for(unsigned c=0;c<4;c++)C3D_FVUnifSet(GPU_VERTEX_SHADER,23+c,corners[c][0],corners[c][1],0,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,27,scale[0],scale[1],0,0);
        /* GX point size is sixths of a source viewport pixel. Preserve its
         * normalized size when the framebuffer is scaled to the native LCD. */
        C3D_FVUnifSet(GPU_VERTEX_SHADER,28,1.f/(3*viewport[2]),1.f/(3*viewport[3]),12,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,29,which?76:0,1.f/32,1.f/16,1.f/8);
        C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
        AttrInfo_AddLoader(a,0,GPU_SHORT,3);AttrInfo_AddLoader(a,1,GPU_UNSIGNED_BYTE,2);
        C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
        if(BufInfo_Add(b,vertices,sizeof(Star),2,0x10)<0)fh_platform_panic("PICA star vertex layout");
        unsigned count=(end-begin)*POINTS*6;
        C3D_DrawElements(GPU_TRIANGLES,count,C3D_UNSIGNED_SHORT,indices+begin*POINTS*6);
        palette->last_used=fh_minimal_frames+1;
        fh_pica_hud_account(count);draws++;submitted+=(end-begin)*POINTS;begin=end;
    }
}
void fh_pica_stars_shutdown(void){
    char line[192];snprintf(line,sizeof(line),"PICA stars: frames=%u draws=%u points=%u prepared=%u vertex_bytes=%u index_bytes=%u CPU_vertices=0\n",
        frames,draws,submitted,points,VERTICES*(unsigned)sizeof(Star),INDICES*(unsigned)sizeof(uint16_t));fh_platform_log(line);
    /* Scene shutdown follows the host's completed GPU retirement. */
    linearFree(vertices);linearFree(indices);vertices=NULL;indices=NULL;shaderProgramFree(&program);
    for(unsigned i=0;i<3;i++)C3D_TexDelete(&palettes[i].texture);
    snprintf(line,sizeof(line),"PICA star palette: resident_bytes=1536 upload_bytes=%u overwrite_drains=%u\n",palette_upload_bytes,palette_drains);fh_platform_log(line);
}
