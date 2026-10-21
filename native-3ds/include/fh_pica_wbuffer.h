#pragma once
#include <math.h>
/* Prototype: derive linear eye-distance storage from the ORIGINAL GX clip
 * projection. No vertex Z rewrite, so source near/far clipping is unchanged.
 * Intended SDK call: C3D_DepthMap(false, out[0], out[1]). Not wired into the
 * gameplay frame owner: every depth-sharing perspective pipeline must move
 * together, and orthographic/custom projections require explicit treatment.
 */
static inline int fh_pica_wbuffer_parameters(float out[4],const float projection[4][4],float near_z,float far_z){
    if(projection[2][0]!=0 || projection[2][1]!=0 || projection[3][0]!=0 ||
       projection[3][1]!=0 || projection[3][2]!=-1 || projection[3][3]!=0 ||
       !isfinite(near_z) || !isfinite(far_z))return 0;
    double a=projection[2][2],b=projection[2][3];
    if(!isfinite(a) || !isfinite(b) || a>=0 || b>=0)return 0;
    double near=b/(a-1),far=b/a;
    if(!(near>0 && far>near) || !isfinite(far))return 0;
    double scale=-((1-near_z)*far-(1-far_z)*near)/(near*far);
    double offset=(near_z-far_z-scale*near)/(far-near);
    out[0]=(float)scale;out[1]=(float)offset;out[2]=(float)near;out[3]=(float)far;
    return 1;
}
