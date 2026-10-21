#pragma once
#include <citro3d.h>
/* Original gRcpTevSwapTable: alpha never participates in the RGB permutation.
 * Three disjoint channel masks reconstruct TEX0, then the fourth stage applies
 * the admitted original base RGB operation. No new image, UV or geometry. */
static const unsigned fh_pica_texture_swap_masks[24][3]={
 {0xffu,0xff00u,0xff0000u},
 {0xffu,0xff0000u,0xff00u},
 {0xff00u,0xffu,0xff0000u},
 {0xff00u,0xff0000u,0xffu},
 {0xff0000u,0xffu,0xff00u},
 {0xff0000u,0xff00u,0xffu},
 {0xffffu,0x0u,0xff0000u},
 {0xff00ffu,0x0u,0xff00u},
 {0xffff00u,0x0u,0xffu},
 {0xffffu,0xff0000u,0x0u},
 {0xff00ffu,0xff00u,0x0u},
 {0xffff00u,0xffu,0x0u},
 {0x0u,0xffffu,0xff0000u},
 {0x0u,0xff00ffu,0xff00u},
 {0x0u,0xffff00u,0xffu},
 {0xff0000u,0xffffu,0x0u},
 {0xff00u,0xff00ffu,0x0u},
 {0xffu,0xffff00u,0x0u},
 {0xff0000u,0x0u,0xffffu},
 {0xff00u,0x0u,0xff00ffu},
 {0xffu,0x0u,0xffff00u},
 {0x0u,0xff0000u,0xffffu},
 {0x0u,0xff00u,0xff00ffu},
 {0x0u,0xffu,0xffff00u}
};
static inline unsigned fh_pica_texture_swizzle_color(unsigned selector,unsigned stage,unsigned color){
    if(stage>=3)return color;
    return (color&0xff000000u)|fh_pica_texture_swap_masks[selector][stage];
}
/* Selectors 6..23 use only two input channels. This leaves stage0 available
 * for the existing clamped environment-light sum and its RGB buffer. */
static inline unsigned fh_pica_texture_swizzle_missing(unsigned selector){
    for(unsigned i=0;i<3;i++)if(!fh_pica_texture_swap_masks[selector][i])return i;
    return 3;
}
static inline unsigned fh_pica_texture_swizzle_environment_color(unsigned selector,unsigned stage,unsigned color,unsigned strength){
    if(!stage)return 0xff000000u|strength*0x010101u;
    if(stage==3)return color;
    unsigned missing=fh_pica_texture_swizzle_missing(selector),channel=stage-1;
    if(channel>=missing)channel++;
    return (color&0xff000000u)|fh_pica_texture_swap_masks[selector][channel];
}
static inline int fh_pica_texture_swizzle_compile(C3D_TexEnv result[4],const C3D_TexEnv base[4]){
    /* Only independent single-base programs. Environment/C2, masks and layers
     * need their own composition; do not overwrite their live stages. */
    if(base[0].funcRgb!=GPU_MODULATE || base[0].opRgb || base[0].scaleRgb!=GPU_TEVSCALE_1 ||
       ((base[0].srcRgb>>4)&15)!=GPU_TEXTURE0 ||
       ((base[0].srcRgb&15)!=GPU_PRIMARY_COLOR && (base[0].srcRgb&15)!=GPU_CONSTANT))return 0;
    /* REPLACE consumes only its first source/operand. The existing unlit
     * compiler rewrites unused PRIMARY_COLOR slots too; those are harmless. */
    for(unsigned i=1;i<4;i++)if((base[i].srcRgb&15)!=GPU_PREVIOUS ||
        (base[i].srcAlpha&15)!=GPU_PREVIOUS || (base[i].opRgb&15) || (base[i].opAlpha&15) ||
        base[i].funcRgb!=GPU_REPLACE || base[i].funcAlpha!=GPU_REPLACE ||
        base[i].scaleRgb!=GPU_TEVSCALE_1 || base[i].scaleAlpha!=GPU_TEVSCALE_1)return 0;
    const GPU_TEVOP_RGB channel[3]={GPU_TEVOP_RGB_SRC_R,GPU_TEVOP_RGB_SRC_G,GPU_TEVOP_RGB_SRC_B};
    for(unsigned i=0;i<4;i++)C3D_TexEnvInit(result+i);
    for(unsigned i=0;i<3;i++){
        C3D_TexEnvSrc(result+i,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
        C3D_TexEnvOpRgb(result+i,channel[i],GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvFunc(result+i,C3D_RGB,i?GPU_MULTIPLY_ADD:GPU_MODULATE);
    }
    result[0].srcAlpha=base[0].srcAlpha;result[0].opAlpha=base[0].opAlpha;
    result[0].funcAlpha=base[0].funcAlpha;result[0].scaleAlpha=base[0].scaleAlpha;
    result[3].srcRgb=(base[0].srcRgb&~0xf0u)|(GPU_PREVIOUS<<4);
    result[3].opRgb=base[0].opRgb;result[3].funcRgb=base[0].funcRgb;
    result[3].scaleRgb=base[0].scaleRgb;
    return 1;
}
/* RGB reconstruction and the source's separate opacity plane occupy independent
 * combiner lanes. Preserve the prepared alpha sequence, including replacement
 * of base alpha by KAlpha * mask. Do not admit an unrelated layer operation. */
static inline int fh_pica_texture_swizzle_opacity_compile(C3D_TexEnv result[4],const C3D_TexEnv base[4]){
    if(fh_pica_texture_swizzle_compile(result,base))return 1;
    const C3D_TexEnv *mask=base+1;
    if((mask->srcAlpha&255)!=(GPU_PREVIOUS|(GPU_TEXTURE1<<4)) ||
       (mask->opAlpha&15) || (((mask->opAlpha>>4)&15)!=GPU_TEVOP_A_SRC_ALPHA &&
                            ((mask->opAlpha>>4)&15)!=GPU_TEVOP_A_SRC_R) ||
       mask->funcAlpha!=GPU_MODULATE || mask->scaleAlpha!=GPU_TEVSCALE_1)return 0;
    C3D_TexEnv plain[4];for(unsigned i=0;i<4;i++)plain[i]=base[i];
    C3D_TexEnvSrc(plain+1,C3D_Alpha,GPU_PREVIOUS,0,0);
    C3D_TexEnvOpAlpha(plain+1,0,0,0);C3D_TexEnvFunc(plain+1,C3D_Alpha,GPU_REPLACE);
    if(!fh_pica_texture_swizzle_compile(result,plain))return 0;
    result[1].srcAlpha=mask->srcAlpha;result[1].opAlpha=mask->opAlpha;
    result[1].funcAlpha=mask->funcAlpha;result[1].scaleAlpha=mask->scaleAlpha;
    return 1;
}
static inline int fh_pica_texture_swizzle_environment_compile(C3D_TexEnv result[4],const C3D_TexEnv base[4],const C3D_TexEnv *lighting,unsigned missing){
    if(missing>=3 || !fh_pica_texture_swizzle_opacity_compile(result,base) ||
       (base[0].srcRgb&15)!=GPU_PRIMARY_COLOR)return 0;
    result[0]=*lighting;
    const GPU_TEVOP_RGB channel[3]={GPU_TEVOP_RGB_SRC_R,GPU_TEVOP_RGB_SRC_G,GPU_TEVOP_RGB_SRC_B};
    for(unsigned i=1;i<4;i++)C3D_TexEnvInit(result+i);
    for(unsigned i=0;i<2;i++){
        unsigned c=i>=missing?i+1:i;
        C3D_TexEnvSrc(result+i+1,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
        C3D_TexEnvOpRgb(result+i+1,channel[c],GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvFunc(result+i+1,C3D_RGB,i?GPU_MULTIPLY_ADD:GPU_MODULATE);
    }
    result[1].srcAlpha=base[0].srcAlpha;result[1].opAlpha=base[0].opAlpha;
    result[1].funcAlpha=base[0].funcAlpha;result[1].scaleAlpha=base[0].scaleAlpha;
    result[2].srcAlpha=base[1].srcAlpha;result[2].opAlpha=base[1].opAlpha;
    result[2].funcAlpha=base[1].funcAlpha;result[2].scaleAlpha=base[1].scaleAlpha;
    C3D_TexEnvSrc(result+3,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,0);
    C3D_TexEnvFunc(result+3,C3D_RGB,GPU_MODULATE);
    return 1;
}
/* Existing diffuse-map sampler ownership: map=0, base=1. Remap the prepared
 * descriptor once at load; UV routing and map lifetimes stay with their owner. */
static inline void fh_pica_texture_swizzle_diffuse_compile(C3D_TexEnv result[4],const C3D_TexEnv base[4],unsigned environment){
    for(unsigned i=0;i<4;i++){
        result[i]=base[i];
        unsigned sources[2]={base[i].srcRgb,base[i].srcAlpha};
        for(unsigned c=0;c<2;c++)for(unsigned n=0;n<3;n++){
            unsigned shift=n*4,src=(sources[c]>>shift)&15;
            if(environment && !i && !c && src==GPU_FRAGMENT_PRIMARY_COLOR)src=GPU_TEXTURE0;
            else if(src==GPU_TEXTURE0)src=GPU_TEXTURE1;
            else if(src==GPU_TEXTURE1)src=GPU_TEXTURE2;
            sources[c]=(sources[c]&~(15u<<shift))|(src<<shift);
        }
        result[i].srcRgb=sources[0];result[i].srcAlpha=sources[1];
    }
}
