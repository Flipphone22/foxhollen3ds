#ifndef FH_PICA_GLOW_REFLECTION_MATERIAL_H
#define FH_PICA_GLOW_REFLECTION_MATERIAL_H

#include <3ds.h>
#include <citro3d.h>

/* Load-time material compiler for source setupReflectionIndirectTev.
 * Sampler 0 must supply the projected, distorted reflection RGB.
 * Sampler 1 alpha must supply GX_ALPHA_BUMPN AFTER source filtering and
 * quantization. Ordinary texture alpha is NOT an equivalent substitute.
 * Coordinates, capture ownership and that mask producer are separate work;
 * this compiler alone does not admit the currently unsupported particle path.
 * The caller stores two immutable descriptions (240 CPU bytes with this SDK),
 * and publishes all six stages on owner handoff before optional distance fog.
 * No per-frame compilation, vertices, allocation or GPU commands here. */
static inline void fh_pica_glow_reflection_material_prepare(C3D_TexEnv out[2][6])
{
    for (unsigned override = 0; override < 2; ++override) {
        for (unsigned stage = 0; stage < 6; ++stage)
            C3D_TexEnvInit(&out[override][stage]);
        C3D_TexEnv *e = &out[override][0];
        C3D_TexEnvSrc(e, C3D_RGB, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
        C3D_TexEnvFunc(e, C3D_RGB, override ? GPU_ADD : GPU_MODULATE);
        C3D_TexEnvSrc(e, C3D_Alpha, GPU_TEXTURE1, GPU_PRIMARY_COLOR, 0);
        C3D_TexEnvFunc(e, C3D_Alpha, GPU_MODULATE);
    }
}

#endif
