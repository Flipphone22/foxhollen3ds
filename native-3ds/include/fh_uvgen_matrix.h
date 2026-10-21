#ifndef FH_UVGEN_MATRIX_H
#define FH_UVGEN_MATRIX_H
#include <math.h>
#include <string.h>
/* One live matrix, never a reassociated product of two transforms. The caller
 * has established ordinary, non-normalized 2x4 texture-coordinate generation.
 * Post-only input has q=1 in GX; require affine output for all three samplers. */
static inline int fh_uvgen_matrix(float out[3][4],const float matrix[3][4],const float post[3][4]) {
    if((matrix!=NULL)==(post!=NULL))return 0;
    const float (*src)[4]=matrix?matrix:post;
    for(unsigned r=0;r<(post?3u:2u);r++)for(unsigned c=0;c<4;c++)
        if(!isfinite(src[r][c]))return 0;
    if(post && (post[2][0]!=0 || post[2][1]!=0 ||
       !((post[2][2]==0 && post[2][3]==1) || (post[2][2]==1 && post[2][3]==0))))return 0;
    memcpy(out,src,32);
    out[2][0]=out[2][1]=out[2][2]=0;out[2][3]=1;
    return 1;
}
#endif
