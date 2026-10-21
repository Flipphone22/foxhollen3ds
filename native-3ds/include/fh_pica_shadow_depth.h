#ifndef FH_PICA_SHADOW_DEPTH_H
#define FH_PICA_SHADOW_DEPTH_H
/* Match Camera_ApplyDecalViewport: source near=-0.05, far=1. Reverse-Z PICA
 * maps that to scale=near-far=-1.05, offset=1-far=0, as for ground decals.
 * A fixed four-D24-quantum epsilon loses the source's depth-relative lift:
 * separately authored collision receivers can then fight visible terrain.
 * Keep receiver positions/edge coverage unchanged, GEQUAL, and no depth writes.
 * This is the source decal tolerance, not permission to ignore foreground
 * occlusion. Physical temporal stability still needs image acceptance.
 */
#define FH_PICA_SHADOW_DEPTH_SCALE (-1.05f)
static inline void fh_pica_shadow_depth_bind(void){
    C3D_DepthMap(true,FH_PICA_SHADOW_DEPTH_SCALE,0);
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
}
#endif
