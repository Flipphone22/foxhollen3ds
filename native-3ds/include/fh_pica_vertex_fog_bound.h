#pragma once
#include <math.h>
/* Prototype admission bound, not yet used by gameplay.
 * Positive perspective-correct barycentric weights form a convex combination
 * of camera distances. For a triangle wholly inside the exponential interval,
 * linear interpolation error <= max(|f''|)*(hi-lo)^2/8. A crossing clamp kink
 * uses the global Lipschitz bound L*(hi-lo)/2 instead. Constant tails are exact.
 * The returned transmission error excludes raster/TEV quantization and shader
 * float24 error. Callers must budget those separately and provide bounds on
 * LIVE transformed geometry, not just the undeformed bind-pose mesh.
 */
static inline float fh_pica_vertex_fog_bound(float lo,float hi,float start,float end){
    if(!isfinite(lo)||!isfinite(hi)||!isfinite(start)||!isfinite(end)||
       hi<lo||!(end>start))return 1;
    if(hi<=start||lo>=end||hi==lo)return 0;
    /* Round slope upward. No per-object exp/pow or per-vertex CPU work. */
    const float k=5.545178f;
    float span=(hi-lo)/(end-start);
    if(!isfinite(span))return 1;
    float bound=k*span*.5f;
    if(lo>=start&&hi<=end)bound=fminf(bound,k*k*span*span*.125f);
    return fminf(1,bound+2.e-7f);
}
