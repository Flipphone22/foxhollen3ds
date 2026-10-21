#include "fh_graphics_options.h"
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include "fh_pica_hologram.h"
#include "fh_pica_image.h"
#include "fh_pica_morph.h"
#include "fh_pica_morph_layout.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"

static shaderProgram_s program;
static C3D_TexEnv stages[2];
static unsigned draws,indices;
void fh_pica_hologram_init(void *entry){
    shaderProgramInit(&program);shaderProgramSetVsh(&program,(DVLE_s*)entry);
    if(shaderInstanceGetUniformLocation(program.vertexShader,"bones")!=7 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"decode")!=73 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"projected")!=76 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"normal_uv")!=79)
        fh_platform_panic("PICA hologram uniform ABI");
    /* Original zero indirect matrix contributes no distortion. RGB is the
     * projected base; alpha is clamp(channelAlpha - mask * konstAlpha). */
    for(unsigned i=0;i<2;i++)C3D_TexEnvInit(stages+i);
    C3D_TexEnvSrc(stages,C3D_RGB,GPU_TEXTURE0,0,0);
    C3D_TexEnvFunc(stages,C3D_RGB,GPU_REPLACE);
    C3D_TexEnvSrc(stages,C3D_Alpha,GPU_TEXTURE1,GPU_CONSTANT,0);
    C3D_TexEnvFunc(stages,C3D_Alpha,GPU_MODULATE);
    C3D_TexEnvSrc(stages+1,C3D_Alpha,GPU_CONSTANT,GPU_PREVIOUS,0);
    C3D_TexEnvFunc(stages+1,C3D_Alpha,GPU_SUBTRACT);
}
unsigned fh_pica_hologram_draw(const FhSceneAsset *asset,const FhPicaModelPlan *plan,
    const float *poses,unsigned joints,const float world[3][4],const float view[3][4],
    const float projection[4][4],const unsigned clip[4],const FhPicaHologram *state,unsigned *out_indices){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    *out_indices=0;
    if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_HOLOGRAM))return 0;
    unsigned total=0;
    if(!asset || !plan || !asset->compact)fh_platform_panic("PICA hologram persistent storage");
    float mask_scale[2];unsigned intensity;
    const C3D_Tex *mask=fh_pica_hud_sampler(state->mask_page,mask_scale,&intensity);
    const void *morph_view=fh_pica_morph_view(asset,poses);
    for(unsigned i=0;i<plan->count;i++){
        const FhPicaModelDraw *d=plan->draw+i;
        fh_pica_scene_reserve();fh_pica_scene_invalidate();
        /* Two small menu meshes. Publish after any host retirement, never
         * retain a stack sampler or command referencing transient vertices. */
        C3D_BindProgram(&program);C3D_LightEnvBind(NULL);
        C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
        C3D_TexEnvBufUpdate(C3D_Both,0);
        C3D_SetTexEnv(0,stages);C3D_SetTexEnv(1,stages+1);
        for(unsigned s=2;s<6;s++)C3D_TexEnvInit(C3D_GetTexEnv(s));
        C3D_TexEnvColor(C3D_GetTexEnv(0),state->mask_alpha<<24);
        C3D_TexEnvOpAlpha(C3D_GetTexEnv(0),intensity?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,0);
        C3D_TexEnvColor(C3D_GetTexEnv(1),state->alpha<<24);
        float uv_scale[2];const C3D_Tex *base=fh_pica_material_projected_sampler(&plan->materials,d->material,uv_scale);
        C3D_TexBind(0,(C3D_Tex*)base);C3D_TexBind(1,(C3D_Tex*)mask);C3D_TexBind(2,NULL);
        C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
        C3D_CullFace(state->cull_front?GPU_CULL_BACK_CCW:GPU_CULL_FRONT_CCW);
        C3D_DepthMap(true,-1,0);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
        C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
        for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0],view[r][1],view[r][2],view[r][3]);
        for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
        for(unsigned b=0;b<d->bones;b++){
            unsigned bone=d->bone[b];float rows[3][4];
            if(bone>joints)fh_platform_panic("PICA hologram bone index");
            fh_pica_model_pose(rows,bone<joints?poses+bone*16:&world[0][0],(const float*)(asset->meta+asset->tails_at)+bone*3);
            for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,7+b*3+r,rows[r][0],rows[r][1],rows[r][2],rows[r][3]);
        }
        float ps=d->format==7?1.f:d->format==5?1.f/8:1.f/256;
        C3D_FVUnifSet(GPU_VERTEX_SHADER,73,ps,1,asset->meta[7]=='B'?1.f/128:.25f,1.f/64);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,74,uv_scale[0],uv_scale[1],0,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,75,mask_scale[0],mask_scale[1],0,0);
        for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,76+r,state->projected[r][0],state->projected[r][1],state->projected[r][2],state->projected[r][3]);
        for(unsigned r=0;r<2;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,79+r,state->normal_uv[r][0],state->normal_uv[r][1],state->normal_uv[r][2],state->normal_uv[r][3]);
        C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
        AttrInfo_AddLoader(a,0,GPU_SHORT,4);AttrInfo_AddLoader(a,1,GPU_UNSIGNED_BYTE,4);
        AttrInfo_AddLoader(a,2,GPU_SHORT,4);AttrInfo_AddLoader(a,3,GPU_BYTE,4);
        AttrInfo_AddLoader(a,4,GPU_UNSIGNED_BYTE,4);AttrInfo_AddLoader(a,5,GPU_FLOAT,3);
        C3D_BufInfo *v=C3D_GetBufInfo();BufInfo_Init(v);
        const void *positions=fh_pica_morph_positions(morph_view,i);
        if(positions){
            if(!fh_pica_morph_bind(v,positions,fh_pica_morph_attributes(morph_view,i)))
                fh_platform_panic("PICA hologram morph layout");
        }else if(BufInfo_Add(v,asset->gpu+d->vertex_offset,40,6,0x543210)<0)
            fh_platform_panic("PICA hologram vertex layout");
        C3D_DrawElements(GPU_TRIANGLES,d->indices,C3D_UNSIGNED_SHORT,asset->gpu+d->index_offset);
        draws++;indices+=d->indices;total+=d->indices;
    }
    *out_indices=total;return plan->count;
}
void fh_pica_hologram_shutdown(void){
    char line[128];snprintf(line,sizeof(line),"PICA hologram: draws=%u indices=%u CPU_vertices=0\n",draws,indices);fh_platform_log(line);
    shaderProgramFree(&program);
}
