#include "fh_pica_texture_coordinates.h"
#include "fh_pica_options.h"
/* Native HUD: resident image pages, immutable quad geometry, live uniform
 * records batched in original order. No GX interpretation/texture resolution. */
#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include <stdio.h>
#include "fh_pica_hud.h"
#include "fh_pica_scene.h"
#include "fh_pica_image.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_minimal_graphics.h"
#include "fh_replay_timing.h"
enum { PAGES=256, GLYPHS=24, OWNER_BUCKETS=512 };
typedef struct {const void *owner;unsigned image,intensity,mask,mask_intensity;C3D_Tex sampler,mask_sampler;float scale[2],mask_scale[2];} Page;
static Page pages[PAGES];
#include "platform_pica_glow_mask_pages.h"
/* Adapted from pinned Melee texture_index.h: fixed resident slots, chained
 * 16-bit handles. The key here is source ownership, not decoded image content.
 * Update only at load/release/relocation boundaries; never retain freed owners. */
static uint16_t owner_heads[OWNER_BUCKETS],owner_next[PAGES];
static unsigned owner_lookups,owner_probes,owner_linear_equivalent,owner_misses;
static unsigned owner_bucket(const void *owner){
    uintptr_t key=(uintptr_t)owner;
    key^=key>>16;key^=key>>9;
    return key&(OWNER_BUCKETS-1);
}
static void owner_add(unsigned slot){
    unsigned bucket=owner_bucket(pages[slot].owner);
    owner_next[slot]=owner_heads[bucket];owner_heads[bucket]=slot+1;
}
static void owner_remove(unsigned slot){
    uint16_t *link=owner_heads+owner_bucket(pages[slot].owner);
    while(*link){
        unsigned i=*link-1;
        if(i==slot){*link=owner_next[i];owner_next[i]=0;return;}
        link=owner_next+i;
    }
    fh_platform_panic("PICA HUD owner index lifetime");
}
static unsigned initialized;static shaderProgram_s program;
static float (*vertices)[3];
static float parameters[GLYPHS][3][4],matrix[4][4];
static float view[3][4],depth;
static const float identity[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
static unsigned projected,projected_draws,projected_glyphs;
static unsigned count,page,alpha,additive,scissor[4],bound,draws,glyphs,loads,releases,peak,resident;
static unsigned selected_page=~0u,selected_alpha=~0u,selected_additive=~0u;
static unsigned gradient,gradient_b,gradient_old,gradient_phase,gradient_fade,gradient_tint,gradient_draws;
static unsigned ground_shadow,ground_shadow_draws;
static unsigned visibility_bit(void){return ground_shadow?FH_GRAPHICS_NO_GROUND_SHADOWS:gradient?FH_GRAPHICS_NO_SKY:FH_GRAPHICS_NO_HUD;}
unsigned fh_pica_sky_snapshots,fh_pica_sky_snapshot_bytes;
uint64_t fh_pica_sky_snapshot_ticks;
unsigned fh_pica_hud_text_page;
unsigned fh_pica_hud_text_alpha_only;
void fh_pica_hud_invalidate(void){bound=0;}
void fh_pica_hud_init(void *vertex_entry){
    if(!vertex_entry)fh_platform_panic("PICA HUD shader entry");
    shaderProgramInit(&program);shaderProgramSetVsh(&program,(DVLE_s*)vertex_entry);
    initialized=1;
    vertices=linearMemAlign(GLYPHS*6*sizeof(*vertices),128);
    if(!vertices)fh_platform_panic("PICA HUD geometry allocation");
    const unsigned corners[6][2]={{0,0},{1,0},{0,1},{0,1},{1,0},{1,1}};
    for(unsigned i=0;i<GLYPHS;i++)for(unsigned j=0;j<6;j++){
        vertices[i*6+j][0]=corners[j][0];vertices[i*6+j][1]=corners[j][1];vertices[i*6+j][2]=i*3;
    }
    fh_gpu_cache_flush(vertices,GLYPHS*6*sizeof(*vertices));
}
unsigned fh_pica_hud_find(const void *owner){
    if(!owner)return 0;
    owner_lookups++;
    for(unsigned link=owner_heads[owner_bucket(owner)];link;link=owner_next[link-1]){
        owner_probes++;
        if(pages[link-1].owner==owner){owner_linear_equivalent+=link;return link;}
    }
    owner_linear_equivalent+=PAGES;owner_misses++;
    return 0;
}
const C3D_Tex *fh_pica_hud_sampler(unsigned id,float scale[2],unsigned *intensity){
    return fh_pica_hud_sampler_layer(id,0,scale,intensity);
}
const C3D_Tex *fh_pica_hud_sampler_layer(unsigned id,unsigned layer,float scale[2],unsigned *intensity){
    if(!id || id>PAGES || !pages[id-1].owner)fh_platform_panic("PICA HUD borrowed page lifetime");
    if(layer>1)fh_platform_panic("PICA HUD borrowed page layer");
    Page *p=pages+id-1;
    if(layer){
        if(!p->mask)return NULL;
        memcpy(scale,p->mask_scale,sizeof(p->mask_scale));*intensity=p->mask_intensity;return &p->mask_sampler;
    }
    memcpy(scale,p->scale,sizeof(p->scale));*intensity=p->intensity;
    return &p->sampler;
}
unsigned fh_pica_hud_page(const void *owner){
    unsigned found=fh_pica_hud_find(owner);
    if(owner && !found){
        char line[160];snprintf(line,sizeof(line),"PICA HUD missing owner=%p caller=%p resident=%u loads=%u releases=%u\n",
            owner,__builtin_return_address(0),resident,loads,releases);fh_platform_log(line);
        fh_platform_panic("PICA HUD page not prepared");
    }
    return found;
}
void fh_pica_hud_load(const void *owner,const FhTexture *source){
    fh_pica_hud_load_image(owner,source,NULL);
}
void fh_pica_hud_load_image(const void *owner,const FhTexture *source,const FhTexture *mask){
    if(!fh_pica_scene_actors)return;
    if(!initialized || !owner || !source)fh_platform_panic("PICA HUD load boundary");
    if(fh_pica_hud_find(owner))fh_platform_panic("PICA HUD duplicate page load");
    unsigned slot=PAGES;
    for(unsigned i=0;i<PAGES;i++){
        if(!pages[i].owner && slot==PAGES)slot=i;
    }
    if(slot==PAGES)fh_platform_panic("PICA HUD page capacity");
    unsigned image=fh_pica_image_acquire(source);
    if(!image && fh_minimal_linear_room(256*1024))image=fh_pica_image_acquire(source);
    if(!image)fh_platform_panic("PICA HUD image allocation");
    Page *p=pages+slot;p->owner=owner;p->image=image;p->intensity=source->format<=1;
    fh_pica_image_sampler(image,source,&p->sampler,p->scale);
    if(mask){
        p->mask=fh_pica_image_acquire(mask);
        if(!p->mask && fh_minimal_linear_room(256*1024))p->mask=fh_pica_image_acquire(mask);
        if(!p->mask)fh_platform_panic("PICA HUD mask allocation");
        p->mask_intensity=mask->format<=1;
        fh_pica_image_sampler(p->mask,mask,&p->mask_sampler,p->mask_scale);
    }
    owner_add(slot);resident++;loads++;if(resident>peak)peak=resident;
}
void fh_pica_hud_release(const void *owner){
    fh_pica_hud_release_many(&owner,1);
}
void fh_pica_hud_release_many(const void *const *owners,unsigned n){
    if(!fh_pica_scene_actors)return;
    unsigned retired=0;
    for(unsigned k=0;k<n;k++){
        unsigned handle=fh_pica_hud_find(owners[k]);if(!handle)continue;unsigned i=handle-1;
        if(!retired){fh_pica_hud_flush();fh_minimal_sync();retired=1;}
        glow_mask_page_release(i);if(pages[i].mask)fh_pica_image_release(pages[i].mask);
        fh_pica_image_release(pages[i].image);owner_remove(i);memset(pages+i,0,sizeof(Page));
        if(fh_pica_hud_text_page==i+1)fh_pica_hud_text_page=0;
        resident--;releases++;
    }
}
void fh_pica_hud_forget(const void *allocation,unsigned bytes){
    if(!resident || !allocation || !bytes)return;
    uintptr_t start=(uintptr_t)allocation;
    unsigned retired=0;
    for(unsigned i=0;i<PAGES;i++)if(pages[i].owner && (uintptr_t)pages[i].owner>=start && (uintptr_t)pages[i].owner-start<bytes){
        if(!retired){fh_pica_hud_flush();fh_minimal_sync();retired=1;}
        glow_mask_page_release(i);if(pages[i].mask)fh_pica_image_release(pages[i].mask);
        fh_pica_image_release(pages[i].image);owner_remove(i);memset(pages+i,0,sizeof(Page));resident--;releases++;
        if(fh_pica_hud_text_page==i+1)fh_pica_hud_text_page=0;
    }
}
void fh_pica_hud_relocate(const void *old_owner,const void *new_owner){
    unsigned index=fh_pica_hud_find(old_owner);
    if(!index)return;
    if(!new_owner || fh_pica_hud_find(new_owner))fh_platform_panic("PICA HUD relocation collision");
    owner_remove(index-1);
    pages[index-1].owner=new_owner; /* GPU pixels are owned copies, unaffected by source relocation. */
    owner_add(index-1);
}
void fh_pica_hud_flush(void){
    if(!count)return;
    if((fh_graphics_options&FH_GRAPHICS_NO_RENDER) ||
       (fh_graphics_options&visibility_bit())){count=0;return;}
    static unsigned options_seen;
    if(options_seen!=fh_graphics_options){bound=0;options_seen=fh_graphics_options;}
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    unsigned n=count;count=0;unsigned clip[4];
    if(!fh_pica_hud_target(scissor,clip))return;
    fh_pica_scene_reserve();
    if(!bound){
        fh_pica_scene_invalidate();C3D_BindProgram(&program);
        C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
        C3D_TexEnvBufUpdate(C3D_Both,0);
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        for(unsigned i=0;i<3;i++)C3D_TexBind(i,NULL);
        C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddLoader(a,0,GPU_FLOAT,3);
        C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);BufInfo_Add(b,vertices,sizeof(*vertices),1,0);
        C3D_CullFace(GPU_CULL_NONE);C3D_DepthMap(true,-1,0);
        C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
        selected_page=selected_alpha=selected_additive=~0u;bound=1;
    }
    if(selected_page!=page || selected_alpha!=alpha){
        C3D_TexEnv *e=C3D_GetTexEnv(0);C3D_TexEnvInit(e);
        C3D_TexEnvSrc(e,C3D_Both,GPU_PRIMARY_COLOR,page?GPU_TEXTURE0:GPU_PRIMARY_COLOR,0);
        C3D_TexEnvFunc(e,C3D_RGB,page && !(alpha&1)?GPU_MODULATE:GPU_REPLACE);
        C3D_TexEnvFunc(e,C3D_Alpha,page?GPU_MODULATE:GPU_REPLACE);
        C3D_TexEnvScale(e,C3D_Alpha,alpha&2?GPU_TEVSCALE_4:GPU_TEVSCALE_1);
        if(page && pages[page-1].intensity)C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
        C3D_TexBind(0,fh_pica_options_texture(page?&pages[page-1].sampler:NULL,ground_shadow?0:FH_GRAPHICS_NO_HUD_TEX));
        Page *p=page?pages+page-1:NULL;
        C3D_TexEnv *mask=C3D_GetTexEnv(1);C3D_TexEnvInit(mask);
        if(p && p->mask){
            C3D_TexEnvSrc(mask,C3D_RGB,GPU_PREVIOUS,0,0);C3D_TexEnvFunc(mask,C3D_RGB,GPU_REPLACE);
            C3D_TexEnvSrc(mask,C3D_Alpha,GPU_PRIMARY_COLOR,GPU_TEXTURE1,0);C3D_TexEnvFunc(mask,C3D_Alpha,GPU_MODULATE);
            C3D_TexEnvScale(mask,C3D_Alpha,alpha&2?GPU_TEVSCALE_4:GPU_TEVSCALE_1);
            if(p->mask_intensity)C3D_TexEnvOpAlpha(mask,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
        }
        C3D_TexBind(1,fh_pica_options_texture(p && p->mask?&p->mask_sampler:NULL,ground_shadow?0:FH_GRAPHICS_NO_HUD_TEX));
        selected_page=page;selected_alpha=alpha;
    }
    if(selected_additive!=additive){
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,additive?GPU_ONE:GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
        selected_additive=additive;
    }
    if(gradient){
        C3D_TexBind(1,&pages[gradient_b-1].sampler);
        fh_pica_texture2_coord1(0);C3D_TexBind(2,gradient_old?&pages[gradient_old-1].sampler:NULL);
        C3D_TexEnv *e=C3D_GetTexEnv(0);C3D_TexEnvInit(e);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE1,GPU_TEXTURE0,GPU_CONSTANT);
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);C3D_TexEnvColor(e,gradient_phase<<24);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);
        e=C3D_GetTexEnv(1);C3D_TexEnvInit(e);
        if(gradient_old){
            C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE2,GPU_PREVIOUS,GPU_CONSTANT);
            C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);C3D_TexEnvColor(e,gradient_fade<<24);
        }
        e=C3D_GetTexEnv(2);C3D_TexEnvInit(e);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,GPU_PREVIOUS,GPU_CONSTANT);
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);C3D_TexEnvColor(e,gradient_tint);
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        gradient_draws++;
    }
    if(ground_shadow){
        /* Original Camera_ApplyDecalViewport uses near=-.05, far=1. Pinned
         * Melee uses reversed-depth GEQUAL for source LEQUAL, no depth writes. */
        C3D_DepthMap(true,-1.05f,0);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
        ground_shadow_draws++;
    }
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    Page *p=page?pages+page-1:NULL;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,76,p && p->mask?p->mask_scale[0]/p->scale[0]:1,
        p && p->mask?p->mask_scale[1]/p->scale[1]:1,depth*.25f,0);
    for(unsigned i=0;i<4;i++){
        const float *row=matrix[i];float scale=1;
        if(!projected && i==2){row=matrix[3];scale=-.5f;}
        C3D_FVUnifSet(GPU_VERTEX_SHADER,i,row[0]*scale,row[1]*scale,row[2]*scale,row[3]*scale);
    }
    for(unsigned i=0;i<3;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,77+i,view[i][0],view[i][1],view[i][2],view[i][3]);
    for(unsigned i=0;i<n;i++)for(unsigned j=0;j<3;j++){
        float *p=parameters[i][j];C3D_FVUnifSet(GPU_VERTEX_SHADER,4+i*3+j,p[0],p[1],p[2],p[3]);
    }
    C3D_DrawArrays(GPU_TRIANGLES,0,n*6);fh_pica_hud_account(n*6);draws++;glyphs+=n;
    if(projected){projected_draws++;projected_glyphs+=n;}
    if(gradient||ground_shadow)fh_pica_hud_invalidate(); /* Restore ordinary raster/TEV state at next bind. */
}
static void quad(unsigned which,const float transform[4][4],const float modelview[3][4],float z,unsigned perspective,
    const float rect[4],const float uv[4],unsigned rgba,unsigned alpha_only,unsigned add,const unsigned clip[4]){
    if((fh_graphics_options&FH_GRAPHICS_NO_RENDER) ||
       (fh_graphics_options&visibility_bit()))return;
    if(!fh_pica_scene_actors || !(rgba&255))return;
    if(which>PAGES || (which && !pages[which-1].owner))fh_platform_panic("PICA HUD draw page lifetime");
    if(count && (count==GLYPHS || which!=page || alpha_only!=alpha || add!=additive || projected!=perspective || depth!=z ||
        memcmp(matrix,transform,sizeof(matrix)) || memcmp(view,modelview,sizeof(view)) || memcmp(scissor,clip,sizeof(scissor))))fh_pica_hud_flush();
    if(!count){page=which;alpha=alpha_only;additive=add;projected=perspective;depth=z;
        memcpy(matrix,transform,sizeof(matrix));memcpy(view,modelview,sizeof(view));memcpy(scissor,clip,sizeof(scissor));}
    float (*p)[4]=parameters[count++];float sx=which?pages[which-1].scale[0]:1,sy=which?pages[which-1].scale[1]:1;
    p[0][0]=rect[0];p[0][1]=rect[1];p[0][2]=rect[2]-rect[0];p[0][3]=rect[3]-rect[1];
    p[1][0]=uv[0]*sx;p[1][1]=uv[1]*sy;p[1][2]=(uv[2]-uv[0])*sx;p[1][3]=(uv[3]-uv[1])*sy;
    for(unsigned i=0;i<4;i++)p[2][i]=((rgba>>(24-8*i))&255)/255.f;
}
void fh_pica_hud_quad(unsigned which,const float transform[4][4],const float rect[4],const float uv[4],unsigned rgba,unsigned alpha_only,const unsigned clip[4]){
    quad(which,transform,identity,-8,0,rect,uv,rgba,alpha_only,0,clip);
}
void fh_pica_hud_image(unsigned which,const float transform[4][4],const float rect[4],const float uv[4],unsigned rgba,unsigned add,const unsigned clip[4]){
    quad(which,transform,identity,-8,0,rect,uv,rgba,0,add!=0,clip);
}
void fh_pica_hud_colored(unsigned which,const float transform[4][4],const float rect[4],const float uv[4],unsigned rgba,unsigned add,const unsigned clip[4]){
    quad(which,transform,identity,-8,0,rect,uv,rgba,2|(add!=0),add!=0,clip);
}
void fh_pica_hud_projected(unsigned which,const float projection[4][4],const float modelview[3][4],
    const float rect[4],float z,const float uv[4],unsigned rgba,unsigned alpha_only,unsigned add,const unsigned clip[4]){
    quad(which,projection,modelview,z,1,rect,uv,rgba,alpha_only,add!=0,clip);
}
void fh_pica_hud_gradient(unsigned a,unsigned b,unsigned previous,unsigned phase,unsigned fade,unsigned tint,
    const float projection[4][4],const float rect[4],float z,const float uv[4],const unsigned clip[4]){
    fh_pica_hud_flush();
    unsigned ids[3]={a,b,previous};
    for(unsigned i=0;i<3;i++)if(i<2 || ids[i]){
        if(!ids[i] || ids[i]>PAGES || !pages[ids[i]-1].owner)fh_platform_panic("PICA sky page lifetime");
        Page *p=pages+ids[i]-1;
        if(p->mask || p->intensity || memcmp(p->scale,pages[a-1].scale,sizeof(p->scale)))
            fh_platform_panic("PICA sky page layout");
    }
    gradient=1;gradient_b=b;gradient_old=previous;gradient_phase=phase;gradient_fade=fade;gradient_tint=tint;
    quad(a,projection,identity,z,1,rect,uv,0xffffffff,0,0,clip);
    fh_pica_hud_flush();gradient=0;
}
void fh_pica_ground_shadow(unsigned which,const float projection[4][4],const float modelview[3][4],unsigned opacity,const unsigned clip[4]){
    if(!which || !(opacity&255) || (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_GROUND_SHADOWS)))return;
    fh_pica_hud_flush();fh_pica_hud_invalidate();ground_shadow=1;
    /* The shared native HUD shader decodes GX's two fractional position bits.
     * Four input units produce the unit square consumed by the plane matrix. */
    const float rect[4]={0,0,4,4},uv[4]={0,0,1,1};
    quad(which,projection,modelview,0,1,rect,uv,opacity&255,1,0,clip);
    fh_pica_hud_flush();ground_shadow=0;
}
void fh_pica_hud_shutdown(void){
    if(!initialized)return;
    if(count)fh_platform_panic("PICA HUD pending shutdown");
    for(unsigned i=0;i<PAGES;i++)if(pages[i].owner){glow_mask_page_release(i);if(pages[i].mask)fh_pica_image_release(pages[i].mask);fh_pica_image_release(pages[i].image);memset(pages+i,0,sizeof(Page));resident--;}
    memset(owner_heads,0,sizeof(owner_heads));memset(owner_next,0,sizeof(owner_next));
    char line[224];snprintf(line,sizeof(line),"PICA HUD: draws=%u glyphs=%u loads=%u releases=%u peak=%u resident=%u vertex_bytes=%u parameter_bytes=%u\n",draws,glyphs,loads,releases,peak,resident,GLYPHS*6*(unsigned)sizeof(*vertices),(unsigned)sizeof(parameters));fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA HUD projection: draws=%u glyphs=%u view_bytes=%u CPU_vertices=0\n",projected_draws,projected_glyphs,(unsigned)sizeof(view));fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA ground shadows: draws=%u triangles_per_draw=2 extra_linear=0\n",ground_shadow_draws);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA HUD owner index: lookups=%u probes=%u linear_equivalent=%u misses=%u bytes=%u\n",owner_lookups,owner_probes,owner_linear_equivalent,owner_misses,(unsigned)(sizeof(owner_heads)+sizeof(owner_next)));fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA sky: draws=%u snapshots=%u snapshot_bytes=%u snapshot_ticks=%llu\n",gradient_draws,fh_pica_sky_snapshots,fh_pica_sky_snapshot_bytes,(unsigned long long)fh_pica_sky_snapshot_ticks);fh_platform_log(line);
    linearFree(vertices);vertices=NULL;shaderProgramFree(&program);initialized=0;
}
