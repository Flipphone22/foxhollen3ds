/* Load-compiled native fur coverage and source noise colour perturbation.
 * Coverage uses original indirect alpha without its five-bit quantization.
 * Worst coverage difference is 7/255; indirect wind remains absent. */
#include "fh_pica_fur.h"
static C3D_TexEnv fur_environments[2][3][4];
/* Borrow the resident noise image through a persistent descriptor. Citro3D
 * retains this pointer until its next context publication; never bind a stack
 * copy or alter the HUD owner's shared descriptor. No image copy/allocation. */
static C3D_Tex fur_projected_noise;
static unsigned fur_mask_draws;
void fh_pica_fur_material_init(void){
    for(unsigned lit=0;lit<2;lit++)for(unsigned mask=0;mask<3;mask++){
        C3D_TexEnv *e=fur_environments[lit][mask];
        for(unsigned i=0;i<4;i++)C3D_TexEnvInit(e+i);
        C3D_TexEnvSrc(e,C3D_RGB,lit?GPU_FRAGMENT_PRIMARY_COLOR:GPU_PRIMARY_COLOR,GPU_CONSTANT,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(e,C3D_RGB,lit?GPU_MULTIPLY_ADD:GPU_REPLACE);
        e++;C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE1,GPU_PREVIOUS,0);C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,GPU_TEXTURE2,0);
        C3D_TexEnvFunc(e,C3D_Alpha,mask?GPU_MODULATE:GPU_REPLACE);
        C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,mask==2?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
        e++;C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_CONSTANT,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_SUBTRACT);
        /* Source: clamp(B + .5 - mix(noise, .5, B)). Save lit base B
         * at stage 1; its delayed buffer value is available at stage 3.
         * Both intermediate results fit [0,1], so no signed CPU work or
         * extra pass is needed. Constant .5 rounds to 128/255 on PICA. */
        C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,GPU_TEXTURE0,GPU_PREVIOUS);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
        e++;C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS_BUFFER,GPU_PREVIOUS,0);
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_ADD_SIGNED);
    }
}
int fh_pica_fur_material_bind(const FhPicaMaterialSet *set,unsigned index,unsigned format,unsigned compact,const FhPicaFur *fur){
    if(!set->items || index>=set->count)return 0;
    Material *m=(Material*)set->items+index;
    if(!m->ready || !m->actor)return 0;
    if(m->layers!=1 || m->masked || m->ui.kind || m->movie_pixels || m->moon_alpha)return 2;
    Frame *f=m->frames[0]+m->current[0];
    if(!f->handle || !images[f->handle-1].refs)fh_platform_panic("PICA fur base image lifetime");
    Frame *mask=m->fur_mask;
    /* Texture2 shares UV1. Admit only co-registered padded extents, validated
     * again for animated base frames; never stretch a mismatching coverage map. */
    if(mask && (mask->scale[0]!=f->scale[0] || mask->scale[1]!=f->scale[1]))return 2;
    if(mask && (!mask->handle || !images[mask->handle-1].refs))fh_platform_panic("PICA fur mask lifetime");
    unsigned masked=mask?1+m->fur_mask_intensity:0;
    unsigned lit=!!m->environment;
    unsigned environment_on=lit && !(fh_graphics_options&(FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ENVIRONMENT));
    fh_pica_environment_bind(m->environment_slot,environment_on);
    fh_pica_fog_bind(m->actor_fog);
    unsigned identity=~(lit*3+masked);
    unsigned changed=bound_program!=identity;
    if(changed){C3D_TexEnvBufUpdate(C3D_Alpha,0);C3D_TexEnvBufUpdate(C3D_RGB,2);}
    for(unsigned i=0;i<4;i++){
        unsigned color=i==0?(0xff000000u|(environment_on?m->environment_strength*0x010101u:0)):
            i==2?(((255-fur->alpha)<<24)|0x00808080u):0xffffffffu;
        if(changed){C3D_SetTexEnv(i,fur_environments[lit][masked]+i);C3D_TexEnvColor(C3D_GetTexEnv(i),color);}
        else if(bound_colors[i]!=color)C3D_TexEnvColor(C3D_GetTexEnv(i),color);
        bound_colors[i]=color;
    }
    bound_program=identity;
    float noise_scale[2];unsigned intensity;
    const C3D_Tex *noise=fh_pica_hud_sampler(fur->noise_page,noise_scale,&intensity);
    if(intensity)fh_platform_panic("PICA fur noise requires original alpha");
    C3D_TexBind(1,fh_pica_options_texture(&f->sampler,FH_GRAPHICS_NO_ACTOR_TEX));
    /* Noise controls coverage: ACTOR TEX OFF retains it. */
    fur_projected_noise=*noise;
    fur_projected_noise.param=(noise->param&~GPU_TEXTURE_MODE(7))|GPU_TEXTURE_MODE(GPU_TEX_PROJECTION);
    C3D_TexBind(0,&fur_projected_noise);C3D_TexBind(2,mask?&mask->sampler:NULL);
    fh_pica_texture2_coord1(mask!=NULL);fur_mask_draws+=mask!=NULL;
    float us=compact?(format==5?1.f/256:1.f/1024):1.f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,1,1,1,1.f/us);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,8,37.5f*noise_scale[0],37.5f*noise_scale[1],0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,9,f->scale[0]*us,f->scale[1]*us,0,0);
    fh_pica_scene_actor_second_uv(0);
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ZERO,GPU_ONE);
    return 1;
}

static void fur_material_report(void){
    char line[96];snprintf(line,sizeof(line),"PICA fur coverage: masked_draws=%u\n",fur_mask_draws);fh_platform_log(line);
}
