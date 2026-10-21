#include "fh_pica_water_material.h"

void fh_pica_water_material_compile(C3D_TexEnv stages[2], unsigned heavy_fog,
                                    uint32_t reflection_mix){
    C3D_TexEnvInit(stages);C3D_TexEnvInit(stages+1);
    if(heavy_fog){
        /* GX divides the sum before clamping. ADD then halve on PICA would
         * incorrectly clamp bright reflections first. Interpolation retains
         * the full range; 128/255 instead of 1/2 differs by <=0.5 byte before
         * target quantization. This is explicit, not exact GX half precision. */
        C3D_TexEnvSrc(stages,C3D_RGB,GPU_TEXTURE0,GPU_PRIMARY_COLOR,GPU_CONSTANT);
        C3D_TexEnvFunc(stages,C3D_RGB,GPU_INTERPOLATE);
        C3D_TexEnvColor(stages,0x80808080u);
    }else{
        C3D_TexEnvSrc(stages,C3D_RGB,GPU_TEXTURE0,GPU_PRIMARY_COLOR,0);
        C3D_TexEnvFunc(stages,C3D_RGB,GPU_ADD);
    }
    C3D_TexEnvSrc(stages,C3D_Alpha,GPU_PRIMARY_COLOR,0,0);
    C3D_TexEnvFunc(stages,C3D_Alpha,GPU_REPLACE);
    C3D_TexEnvSrc(stages+1,C3D_RGB,GPU_TEXTURE1,GPU_PREVIOUS,GPU_CONSTANT);
    C3D_TexEnvFunc(stages+1,C3D_RGB,GPU_INTERPOLATE);
    C3D_TexEnvColor(stages+1,reflection_mix);
    /* Init leaves alpha as PREVIOUS: the original opacity product survives. */
}

void fh_pica_water_split_compile(C3D_TexEnv stages[2][2], unsigned heavy_fog,
                                 uint32_t reflection_mix){
    /* Reuse the source first-stage clamp/half-sum. The source's second stage
     * is a convex RGB mix: C0*(1-K)+C1*K. Moving these two contributions into
     * ordered framebuffer blends preserves the original backdrop term once.
     * The second pass must not repeat the destination's (1-opacity) factor. */
    fh_pica_water_material_compile(stages[0],heavy_fog,reflection_mix);
    C3D_TexEnvInit(&stages[0][1]);
    C3D_TexEnvSrc(&stages[0][1],C3D_RGB,GPU_PREVIOUS,GPU_CONSTANT,0);
    C3D_TexEnvFunc(&stages[0][1],C3D_RGB,GPU_MODULATE);
    C3D_TexEnvColor(&stages[0][1],reflection_mix^0x00ffffffu);
    C3D_TexEnvInit(&stages[1][0]);C3D_TexEnvInit(&stages[1][1]);
    C3D_TexEnvSrc(&stages[1][0],C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvFunc(&stages[1][0],C3D_RGB,GPU_MODULATE);
    C3D_TexEnvColor(&stages[1][0],reflection_mix);
    C3D_TexEnvSrc(&stages[1][0],C3D_Alpha,GPU_PRIMARY_COLOR,0,0);
    C3D_TexEnvFunc(&stages[1][0],C3D_Alpha,GPU_REPLACE);
}
