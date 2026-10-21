#ifndef FH_PICA_SHADOW_PROJECTION_H
#define FH_PICA_SHADOW_PROJECTION_H
#include <math.h>
#include <string.h>
typedef struct {float capture[3][4],projection[4][4],texture[3][4];} FhPicaShadowProjection;
/* Native square orthographic capture, original 400-unit light-eye offset and
 * 1..1025 depth extent. Texture rows project onto real receiver positions;
 * row2 is the original depth-ramp coordinate relative to the caster root.
 * A common basis for capture and lookup allows native framebuffer orientation
 * without modifying the game's camera, poses, RNG or simulation state. */
static inline int fh_pica_shadow_projection(FhPicaShadowProjection*out,const float center[3],const float light[3],float extent,float fade){
    if(!out || !isfinite(extent) || !isfinite(fade) || extent<=0 || fade<=0)return 0;
    float length=0;
    for(unsigned k=0;k<3;k++){if(!isfinite(center[k]) || !isfinite(light[k]))return 0;length+=light[k]*light[k];}
    if(!isfinite(length) || length<1e-12f)return 0;
    float l[3],u[3],v[3];length=1.f/sqrtf(length);
    for(unsigned k=0;k<3;k++)l[k]=light[k]*length;
    if(fabsf(l[1])<.9f){u[0]=l[2];u[1]=0;u[2]=-l[0];}
    else{u[0]=0;u[1]=-l[2];u[2]=l[1];}
    length=1.f/sqrtf(u[0]*u[0]+u[1]*u[1]+u[2]*u[2]);
    for(unsigned k=0;k<3;k++)u[k]*=length;
    v[0]=l[1]*u[2]-l[2]*u[1];v[1]=l[2]*u[0]-l[0]*u[2];v[2]=l[0]*u[1]-l[1]*u[0];
    FhPicaShadowProjection p={0};
    for(unsigned row=0;row<3;row++){
        const float *axis=row==0?u:row==1?v:l;
        float d=0,scale=row==2?1.f/fade:.5f/extent;
        for(unsigned k=0;k<3;k++){p.capture[row][k]=axis[k];p.texture[row][k]=axis[k]*scale;d+=axis[k]*center[k];}
        p.capture[row][3]=-d-(row==2?400.f:0.f);
        p.texture[row][3]=-d*scale+(row==2?0.f:.5f);
    }
    p.projection[0][0]=p.projection[1][1]=1.f/extent;
    p.projection[2][2]=-1.f/1024;p.projection[2][3]=-1025.f/1024;p.projection[3][3]=1;
    /* Separate arrays: do not index across C subobject boundaries. */
    for(unsigned row=0;row<3;row++)for(unsigned k=0;k<4;k++)
        if(!isfinite(p.capture[row][k]) || !isfinite(p.texture[row][k]))return 0;
    if(!isfinite(p.projection[0][0]))return 0;
    *out=p;return 1;
}
static inline void fh_pica_shadow_concat(float out[3][4],const float a[3][4],const float b[3][4]){
    float next[3][4];
    for(unsigned row=0;row<3;row++)for(unsigned col=0;col<4;col++){
        float n=col==3?a[row][3]:0;
        for(unsigned k=0;k<3;k++)n+=a[row][k]*b[k][col];
        next[row][col]=n;
    }
    memcpy(out,next,sizeof(next));
}
/* Collision points include the block's collision Y offset already. Keep
 * the source affine basis, removing only that duplicate translation. Works
 * for moving/rotated blocks; it does not assume an identity or rigid basis. */
static inline void fh_pica_shadow_receiver_world(float out[3][4],const float source[3][4],float offset){
    memcpy(out,source,12*sizeof(float));
    for(unsigned row=0;row<3;row++)out[row][3]-=out[row][1]*offset;
}
/* Once per caster: convert world-space projector to eye-space. Combining this
 * with a live camera*block matrix then gives receiver-local lookup uniforms.
 * Supports affine cameras rather than silently assuming a rigid transform. */
static inline int fh_pica_shadow_to_eye(float out[3][4],const float texture[3][4],const float camera[3][4]){
    float c[3][3];
    for(unsigned row=0;row<3;row++)for(unsigned col=0;col<3;col++){
        unsigned a=(row+1)%3,b=(row+2)%3,x=(col+1)%3,y=(col+2)%3;
        c[row][col]=camera[a][x]*camera[b][y]-camera[a][y]*camera[b][x];
    }
    float det=camera[0][0]*c[0][0]+camera[0][1]*c[0][1]+camera[0][2]*c[0][2];
    if(!isfinite(det) || fabsf(det)<1e-8f)return 0;
    float inverse[3][4];
    for(unsigned row=0;row<3;row++){
        inverse[row][3]=0;
        for(unsigned k=0;k<3;k++){inverse[row][k]=c[k][row]/det;inverse[row][3]-=inverse[row][k]*camera[k][3];}
    }
    float next[3][4];fh_pica_shadow_concat(next,texture,inverse);
    for(unsigned row=0;row<3;row++)for(unsigned k=0;k<4;k++)if(!isfinite(next[row][k]))return 0;
    memcpy(out,next,sizeof(next));return 1;
}
static inline unsigned fh_pica_shadow_source_intensity(unsigned alpha){return (alpha>>1)+(alpha>>2);}
#endif
