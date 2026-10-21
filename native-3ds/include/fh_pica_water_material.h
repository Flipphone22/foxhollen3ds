#ifndef FH_PICA_WATER_MATERIAL_H
#define FH_PICA_WATER_MATERIAL_H
#include <stdint.h>
#include <citro3d.h>
/* Native reflection-compositing kernel, compiled outside the draw loop.
 * TEX0/TEX1 are the source's two projected/distorted scene samples.
 * PRIMARY.rgb is original C2 (ambient >>3, or heavy-fog RGB).
 * PRIMARY.a is A1 * normalized indirect bump alpha. Supplying that alpha and
 * the two coordinates is the sheet renderer's responsibility; this module
 * does not claim to implement indirect texture sampling or frame capture.
 * Stages 2..5 remain owned by later effects. No GPU state is published here. */
void fh_pica_water_material_compile(C3D_TexEnv stages[2], unsigned heavy_fog,
                                    uint32_t reflection_mix);
/* Two ordered native passes, each sampling its projected reflection on TEX0.
 * Both receive the SAME PRIMARY.a = A1 * normalized bump alpha.
 * Pass0: SRC_ALPHA, ONE_MINUS_SRC_ALPHA. Pass1: SRC_ALPHA, ONE.
 * Neither may write depth; apply the same depth/coverage tests to both.
 * Distance fog: ordinary fog color in pass0, BLACK fog color in pass1.
 * Alpha-channel blending must preserve destination alpha in pass1.
 * K's three color channels are independent. No GPU publication here.
 * This removes the need for two simultaneous projective samplers, NOT the
 * need for native indirect-coordinate and filtered bump-alpha handling. */
void fh_pica_water_split_compile(C3D_TexEnv stages[2][2], unsigned heavy_fog,
                                 uint32_t reflection_mix);
#endif
