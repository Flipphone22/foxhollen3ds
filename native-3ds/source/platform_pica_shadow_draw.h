/* Scene owner only. The same resident VSH and shadow raster state, compact
 * load-compiled position/skin inputs. No material lookup in this loop. */
#include "fh_pica_shadow_plan.h"
static unsigned compact_shadow_objects,compact_shadow_draws,compact_shadow_saved;
static C3D_AttrInfo shadow_attributes;
static C3D_TexEnv shadow_material[4];
static void shadow_material_init(void){
    for(unsigned i=0;i<4;i++)C3D_TexEnvInit(shadow_material+i);
    C3D_TexEnvSrc(shadow_material,C3D_Both,GPU_CONSTANT,0,0);
    C3D_TexEnvFunc(shadow_material,C3D_Both,GPU_REPLACE);
}
static void shadow_pipeline_init(void){
    AttrInfo_Init(&shadow_attributes);AttrInfo_AddLoader(&shadow_attributes,0,GPU_SHORT,4);
    for(unsigned n=1;n<4;n++)AttrInfo_AddFixed(&shadow_attributes,n);
    AttrInfo_AddLoader(&shadow_attributes,4,GPU_UNSIGNED_BYTE,4);
    AttrInfo_AddLoader(&shadow_attributes,5,GPU_FLOAT,3);shadow_material_init();
}
static void shadow_raster(void){
    /* Capture has no receiver depth/stencil attachment. Publish an opaque
     * white union; the later receiver pass supplies attenuation and blending. */
    if(shadow_active==2){
        C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
        C3D_CullFace(GPU_CULL_NONE);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
        C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        C3D_LightEnvBind(NULL);C3D_TexEnvBufUpdate(C3D_Both,0);
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_CONSTANT,0,0);
        C3D_TexEnvColor(C3D_GetTexEnv(0),0xffffffff);
        fh_pica_fog_bind(0);return;
    }
    C3D_StencilTest(true,GPU_EQUAL,2,3,1);
    C3D_StencilOp(GPU_STENCIL_KEEP,GPU_STENCIL_KEEP,GPU_STENCIL_INCR);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ZERO,GPU_ONE);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,lighting_uniform,false);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,environment_uniform,false);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uv_uniform,false);
    C3D_LightEnvBind(NULL);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned s=0;s<4;s++)C3D_SetTexEnv(s,shadow_material+s);
    C3D_TexEnvColor(C3D_GetTexEnv(0),shadow_alpha<<24);fh_pica_fog_bind(1);
}
static unsigned shadow_compact_draw(const FhSceneAsset *asset,const FhPicaShadowPlan *plan,const float *poses,unsigned joints,
    const float world[3][4],const float view[3][4],const float projection[4][4],const unsigned clip[4],float near_z,float far_z,unsigned *out_indices){
    unsigned total=0;
    if(shadow_active!=2)fh_platform_panic("PICA compact mask used outside capture");
    for(unsigned i=0;i<plan->count;i++){
        const FhPicaShadowDraw *d=plan->draw+i;
        fh_pica_scene_reserve();unsigned publish=!i || !bound;bind_scene_pipeline();
        if(publish){
            scene_depth(near_z,far_z);C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uniform,true);
            C3D_BoolUnifSet(GPU_VERTEX_SHADER,fur_uniform,false);
            scene_camera(view,projection);
            C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
            C3D_SetAttrInfo(&shadow_attributes);
            for(unsigned n=1;n<4;n++)C3D_FixedAttribSet(n,0,0,0,0);
            shadow_raster();fh_pica_color_fade_bind(0);actor_state_publications++;
        }
        float ps=d->format==7?1.f:d->format==5?1.f/8:1.f/256;
        C3D_FVUnifSet(GPU_VERTEX_SHADER,76,ps,1,asset->meta[7]=='B'?1.f/128:.25f,1);
        for(unsigned b=0;b<d->bones;b++){
            /* Load-time slot identity is sufficient only inside this call.
             * First draw and command splits always republish the live pose. */
            if(!publish && !(d->palette_changed&(1u<<b)))continue;
            unsigned bone=d->bone[b];if(bone>joints)fh_platform_panic("PICA compact shadow bone");
            const float *matrix=bone<joints?poses+bone*16:&world[0][0];
            const float *tail=(const float*)(asset->meta+asset->tails_at)+bone*3;float rows[3][4];
            fh_pica_model_pose(rows,matrix,tail);
            for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,10+b*3+r,rows[r][0],rows[r][1],rows[r][2],rows[r][3]);
            actor_bones_written++;actor_bones++;
        }
        C3D_BufInfo *v=C3D_GetBufInfo();BufInfo_Init(v);
        if(BufInfo_Add(v,(const char*)plan->gpu+d->vertex_offset,sizeof(FhPicaShadowVertex),3,0x540)<0)fh_platform_panic("PICA compact shadow buffer");
        C3D_DrawElements(GPU_TRIANGLES,d->indices,C3D_UNSIGNED_SHORT,(const char*)plan->gpu+d->index_offset);total+=d->indices;
    }
    *out_indices=total;compact_shadow_objects++;compact_shadow_draws+=plan->count;
    return plan->count;
}
