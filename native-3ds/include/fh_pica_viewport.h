#ifndef FH_PICA_VIEWPORT_H
#define FH_PICA_VIEWPORT_H
/* Native top target is 400x240; original UI canvas is 640x480. Fold its
 * shifted viewport into the projection before the ordinary LCD rotation.
 * This is sixteen scalar matrix operations per object, never a vertex loop. */
static inline void fh_pica_viewport_xy(float out[4][4],const float in[4][4],const float viewport[6]){
    float sx=viewport[2]/640.f,sy=viewport[3]/480.f;
    float ox=(2*viewport[0]+viewport[2]-640.f)/640.f;
    float oy=(480.f-2*viewport[1]-viewport[3])/480.f;
    for(unsigned c=0;c<4;c++){
        out[0][c]=sx*in[0][c]+ox*in[3][c];
        out[1][c]=sy*in[1][c]+oy*in[3][c];
        out[2][c]=in[2][c];out[3][c]=in[3][c];
    }
}
void fh_pica_live_viewport(float[6]);
/* GX near/far are window-depth endpoints. PICA uses reversed window depth
 * and the same [-w,0] clip interval. Keep this out of the projection so the
 * clipping planes remain intact even for a constant or reversed depth range. */
static inline void fh_pica_viewport_depth(float out[2],float near_z,float far_z){
    out[0]=near_z-far_z;out[1]=1.f-far_z;
}
#endif
