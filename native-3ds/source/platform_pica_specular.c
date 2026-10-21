/* Original 32x32 light maps, generated on PICA from a persistent source mesh.
 * No CPU rasterization/readback, streamed vertices, texture promotion, depth
 * attachment, or per-frame allocation. Main queue owns all GPU lifetimes. */
#include <3ds.h>
#include <citro3d.h>
#include "fh_pica_actor_stream.h"
static FhPicaSamplerCommands map_commands[3];
#include <string.h>
#include <stdio.h>
#include "fh_pica_specular.h"
#include "fh_pica_specular_map.h"
#include "fh_pica_image.h"
#include "fh_pica_scene.h"
#include "fh_graphics_options.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "pica_specular_map_shader.h"
/* Implemented by the verified frame owner. No auxiliary private queue. */
extern void fh_minimal_auxiliary_begin(void);
extern void fh_minimal_auxiliary_end(unsigned draws,unsigned indices);
static FhPicaSpecularMapMesh *mesh;
static C3D_Tex maps[3],base_sampler;
static C3D_FrameBuf targets[3];
static unsigned base_handle,valid,updates,reuses,passes;
static u64 update_ticks;
static FhPicaSpecularState previous[3];
static DVLB_s *binary;
static shaderProgram_s program;
static C3D_AttrInfo attributes;
static C3D_BufInfo buffers;
static C3D_TexEnv channel_env[2];
/* Optional layered-material copy owner. The ordinary map ABI remains intact.
 * No gameplay consumer requests this owner until its material path is ready. */
typedef struct {
    FhPicaSpecularMapVertex vertices[4];
    u16 indices[6];
} HalfQuad;
static HalfQuad *half_quad;
static C3D_Tex half_maps[3];
static C3D_FrameBuf half_targets[3];
static C3D_BufInfo half_buffers;
static C3D_TexEnv half_env;
static unsigned half_valid,half_passes,half_demand,half_hits,half_misses;
/* SPECULAR_PREFETCH_V1: last-boundary demand predicts the next copy batch.
 * Prediction never substitutes for availability: new demand is resolved before
 * actor uniform publication, in the same ordered GPU queue. */
static void render_maps(unsigned changed,unsigned half_changed,const FhPicaSpecularState state[3]);

void fh_pica_specular_prepare_half(void){
    if(half_quad)return;
    if(!mesh)fh_platform_panic("PICA half specular preparation order");
    extern u32 __ctru_linear_heap_size;
    unsigned bytes=sizeof(*half_quad),available=linearSpaceFree();
    if(available<bytes || (u64)(available-bytes)*100<(u64)__ctru_linear_heap_size*15 ||
       vramSpaceFree()<3*4096+1024*1024)fh_platform_panic("PICA half specular resident budget");
    half_quad=linearMemAlign(bytes,128);
    if(!half_quad)fh_platform_panic("PICA half specular quad allocation");
    const float xy[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}};
    for(unsigned i=0;i<4;i++){
        memcpy(half_quad->vertices[i].position,xy[i],sizeof(xy[i]));
        /* Existing map program emits UV=(normal.xy+1)/2. For a copy,
         * normal.xy follows clip position, NOT the sphere's flipped X. */
        memcpy(half_quad->vertices[i].normal,xy[i],sizeof(xy[i]));
        half_quad->vertices[i].normal[2]=0;
    }
    const u16 indices[6]={0,1,2,0,2,3};memcpy(half_quad->indices,indices,sizeof(indices));
    fh_gpu_cache_flush(half_quad,bytes);
    BufInfo_Init(&half_buffers);
    if(BufInfo_Add(&half_buffers,half_quad->vertices,sizeof(half_quad->vertices[0]),2,0x10)<0)
        fh_platform_panic("PICA half specular quad binding");
    for(unsigned i=0;i<3;i++){
        if(!C3D_TexInitVRAM(half_maps+i,32,32,GPU_RGBA8))fh_platform_panic("PICA half specular map allocation");
        C3D_TexSetFilter(half_maps+i,GPU_LINEAR,GPU_LINEAR);
        C3D_TexSetWrap(half_maps+i,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
        C3D_FrameBufTex(half_targets+i,half_maps+i,GPU_TEXFACE_2D,0);
        C3D_FrameBufDepth(half_targets+i,NULL,GPU_RB_DEPTH24);
    }
    C3D_TexEnvInit(&half_env);
    C3D_TexEnvSrc(&half_env,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvFunc(&half_env,C3D_RGB,GPU_MODULATE);
    C3D_TexEnvSrc(&half_env,C3D_Alpha,GPU_CONSTANT,0,0);
    C3D_TexEnvFunc(&half_env,C3D_Alpha,GPU_REPLACE);
    C3D_TexEnvColor(&half_env,0x80808080u);
    half_valid=0;
}

void fh_pica_specular_prepare(const FhTexture *base){
    if(mesh)fh_platform_panic("PICA specular duplicate preparation");
    extern u32 __ctru_linear_heap_size;
    unsigned bytes=sizeof(*mesh),available=linearSpaceFree();
    if(available<bytes || (u64)(available-bytes)*100<(u64)__ctru_linear_heap_size*15 ||
       vramSpaceFree()<3*4096+1024*1024)fh_platform_panic("PICA specular resident budget");
    base_handle=fh_pica_image_acquire(base);
    if(!base_handle)fh_platform_panic("PICA specular source image");
    float scale[2];fh_pica_image_sampler(base_handle,base,&base_sampler,scale);
    /* Original lookup must cover the complete image, never sample padding. */
    if(scale[0]!=1 || scale[1]!=1)fh_platform_panic("PICA specular padded source");
    mesh=linearMemAlign(bytes,128);
    if(!mesh)fh_platform_panic("PICA specular mesh allocation");
    fh_pica_specular_map_prepare(mesh);fh_gpu_cache_flush(mesh,bytes);
    for(unsigned i=0;i<3;i++){
        if(!C3D_TexInitVRAM(maps+i,32,32,GPU_RGBA8))fh_platform_panic("PICA specular map allocation");
        C3D_TexSetFilter(maps+i,GPU_LINEAR,GPU_LINEAR);
        C3D_TexSetWrap(maps+i,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
        if(!fh_pica_actor_sampler_prepare(map_commands+i,maps+i))fh_platform_panic("PICA light map commands");
        /* Direct framebuffer descriptor avoids a second target owner/list.
         * FrameBufTex establishes native tiled attachment and write mask. */
        C3D_FrameBufTex(targets+i,maps+i,GPU_TEXFACE_2D,0);
        C3D_FrameBufDepth(targets+i,NULL,GPU_RB_DEPTH24);
    }
    binary=DVLB_ParseFile((u32*)pica_specular_map_shader,sizeof(pica_specular_map_shader));
    if(!binary || binary->numDVLE!=1 || binary->DVLP.codeSize>512)fh_platform_panic("PICA specular shader");
    shaderProgramInit(&program);shaderProgramSetVsh(&program,binary->DVLE);
    const char *names[]={"direction","half_direction","attenuation","material","diffuse_alpha"};
    for(unsigned i=0;i<5;i++)if(shaderInstanceGetUniformLocation(program.vertexShader,names[i])!=(int)i)
        fh_platform_panic("PICA specular uniform ABI");
    AttrInfo_Init(&attributes);AttrInfo_AddLoader(&attributes,0,GPU_FLOAT,2);AttrInfo_AddLoader(&attributes,1,GPU_FLOAT,3);
    BufInfo_Init(&buffers);
    if(BufInfo_Add(&buffers,mesh->vertices,sizeof(mesh->vertices[0]),2,0x10)<0)fh_platform_panic("PICA specular mesh binding");
    for(unsigned i=0;i<2;i++){
        C3D_TexEnv *e=channel_env+i;C3D_TexEnvInit(e);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_PRIMARY_COLOR,GPU_PRIMARY_COLOR,i?GPU_CONSTANT:GPU_TEXTURE0);
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA,GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MULTIPLY_ADD);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,0,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);
        C3D_TexEnvColor(e,0);
    }
}
static void uniform(unsigned slot,const float p[4]){C3D_FVUnifSet(GPU_VERTEX_SHADER,slot,p[0],p[1],p[2],p[3]);}
void fh_pica_specular_update(unsigned mask,const FhPicaSpecularState state[3]){
    if(!(fh_graphics_options&FH_GRAPHICS_SPECULAR) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS|FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ACTOR_TEX)))return;
    if(mask&~7u)fh_platform_panic("PICA specular slot mask");
    unsigned changed=0;
    for(unsigned i=0;i<3;i++)if(mask&(1u<<i)){
        if((valid&(1u<<i)) && !memcmp(previous+i,state+i,sizeof(*state)))reuses++;
        else changed|=1u<<i;
    }
    unsigned predicted=half_demand&mask;
    half_demand&=~mask;
    unsigned half_changed=half_quad?predicted&(changed|(valid&~half_valid)):0;
    render_maps(changed,half_changed,state);
}
void fh_pica_specular_require_half(unsigned mask){
    if(mask&~7u)fh_platform_panic("PICA half specular demand mask");
    if(!(fh_graphics_options&FH_GRAPHICS_SPECULAR) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS|FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ACTOR_TEX)))return;
    if(!mask)return;
    if(!half_quad)fh_platform_panic("PICA half specular demand before preparation");
    half_demand|=mask;
    unsigned missing=mask&valid&~half_valid;
    if(missing){half_misses++;render_maps(0,missing,previous);}else half_hits++;
}
static void render_maps(unsigned changed,unsigned half_changed,const FhPicaSpecularState state[3]){
    if(!changed&&!half_changed)return;
    if(!mesh)fh_platform_panic("PICA specular not prepared");
    u64 start=svcGetSystemTick();fh_minimal_auxiliary_begin();
    C3D_BindProgram(&program);C3D_SetAttrInfo(&attributes);C3D_SetBufInfo(&buffers);
    C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    C3D_SetViewport(0,0,32,32);C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
    C3D_CullFace(GPU_CULL_NONE);C3D_DepthMap(true,-1,0);
    C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexBind(0,&base_sampler);C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
    unsigned count=0;
    for(unsigned i=0;i<3;i++)if(changed&(1u<<i)){
        const FhPicaSpecularState *s=state+i;
        C3D_SetFrameBuf(targets+i);uniform(2,s->attenuation);uniform(3,s->material);
        for(unsigned channel=0;channel<2;channel++){
            uniform(0,s->direction[channel]);uniform(1,s->half_direction[channel]);
            C3D_FVUnifSet(GPU_VERTEX_SHADER,4,s->diffuse_alpha[channel],0,0,0);
            C3D_SetTexEnv(0,channel_env+channel);
            /* Sun replaces complete target, including source base texture.
             * Moon adds after interpolation; alpha remains exactly zero. */
            C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,channel?GPU_ONE:GPU_ZERO,GPU_ZERO,GPU_ZERO);
            C3D_DrawElements(GPU_TRIANGLES,FH_SPECULAR_MAP_INDICES,C3D_UNSIGNED_SHORT,mesh->indices);
        }
        previous[i]=*s;valid|=1u<<i;half_valid&=~(1u<<i);count++;
    }
    /* Flush framebuffer writes into the SAME ordered GPU queue before any
     * sampler publication. SDK capacity joins retain every borrowed resource.
     * No CPU wait, transfer engine copy, or sampling an active attachment. */
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    unsigned copied=0;
    if(half_changed){
        /* Full-map clamp/rounding completes before scaling. Rendering both
         * light channels directly at half intensity would change that clamp.
         * Keep each completed target distinct from the texture being read. */
        C3D_SetBufInfo(&half_buffers);C3D_SetTexEnv(0,&half_env);
        /* A half-only update may follow an unrelated program. Its lighting
         * outputs are unused, but all read uniforms must still be defined. */
        for(unsigned i=0;i<5;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,i,0,0,0,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        for(unsigned i=0;i<3;i++)if(half_changed&(1u<<i)){
            C3D_SetFrameBuf(half_targets+i);C3D_TexBind(0,maps+i);
            C3D_DrawElements(GPU_TRIANGLES,6,C3D_UNSIGNED_SHORT,half_quad->indices);
            copied++;half_valid|=1u<<i;
        }
        C3D_FrameSplit(GX_CMDLIST_FLUSH);
    }
    fh_minimal_auxiliary_end(count*2+copied,count*2*FH_SPECULAR_MAP_INDICES+copied*6);
    half_passes+=copied;
    updates+=count;passes+=count*2;update_ticks+=svcGetSystemTick()-start;
}
const C3D_Tex *fh_pica_specular_sampler(unsigned slot){
    if(slot>=3 || !(valid&(1u<<slot)))return NULL;
    return maps+slot;
}
const C3D_Tex *fh_pica_specular_half_sampler(unsigned slot){
    if(slot>=3 || !(half_valid&(1u<<slot)))return NULL;
    return half_maps+slot;
}
void fh_pica_specular_shutdown(void){
    /* Called only after normal frame owner's completion fence and FrameEnd.
     * Release source reference before the shared image manager shuts down. */
    char line[256];snprintf(line,sizeof(line),"PICA specular maps: updates=%u reuses=%u draws=%u submit_ticks=%llu linear_bytes=%u vram_bytes=%u CPU_vertices=0 readbacks=0\n",updates,reuses,passes,(unsigned long long)update_ticks,mesh?(unsigned)sizeof(*mesh):0,mesh?3*4096:0);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA half demand: hits=%u misses=%u\n",half_hits,half_misses);fh_platform_log(line);
    if(!mesh)return;
    if(half_quad){
        snprintf(line,sizeof(line),"PICA half specular maps: draws=%u linear_bytes=%u vram_bytes=%u CPU_vertices=0 readbacks=0\n",half_passes,(unsigned)sizeof(*half_quad),3*4096);fh_platform_log(line);
        for(unsigned i=0;i<3;i++)C3D_TexDelete(half_maps+i);
        linearFree(half_quad);half_quad=NULL;half_valid=half_passes=half_demand=half_hits=half_misses=0;
    }
    for(unsigned i=0;i<3;i++)C3D_TexDelete(maps+i);
    linearFree(mesh);mesh=NULL;
    shaderProgramFree(&program);DVLB_Free(binary);binary=NULL;
    fh_pica_image_release(base_handle);base_handle=0;valid=0;
}

const FhPicaSamplerCommands *fh_pica_specular_commands(unsigned slot){return slot<3 && (valid&(1u<<slot))?map_commands+slot:NULL;}
