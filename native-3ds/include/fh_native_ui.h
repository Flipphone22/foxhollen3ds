#ifndef FH_NATIVE_UI_H
#define FH_NATIVE_UI_H
#include <math.h>
/* Input uses GX_VTXFMT1 quarter-unit s16 coordinates. Projection includes the
 * native LCD rotation. No divide by W: clipping/perspective stay on the GPU. */
static inline int fh_native_ui_project(float out[4][4],const float projection[4][4],
    const float view[3][4],const float rect[4],float depth){
    static const unsigned cx[4]={0,1,1,0},cy[4]={0,0,1,1};
    for(unsigned i=0;i<4;i++){
        float p[4]={rect[cx[i]*2]*.25f,rect[cy[i]*2+1]*.25f,depth*.25f,1};
        float eye[4]={0,0,0,1};
        for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)eye[r]+=view[r][c]*p[c];
        for(unsigned r=0;r<4;r++){
            out[i][r]=0;
            for(unsigned c=0;c<4;c++)out[i][r]+=projection[r][c]*eye[c];
            if(!isfinite(out[i][r]))return 0;
        }
    }
    return 1;
}
#endif
