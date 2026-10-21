#ifndef FH_PICA_RECEIVER_PLANE_H
#define FH_PICA_RECEIVER_PLANE_H
#include <math.h>
typedef struct { float equation[4],depth[2]; } FhPicaReceiverPlane;
/* Rank-two projected caster -> perspective depth affine in screen XY.
 * The quad's clip depth is normalized into [-.9,-.1]; SDK DepthMap reverses
 * that normalization. Thus near/far clipping never leaves stale stencil,
 * including planes crossing the horizon. No vertex sampling or CPU clipping. */
static inline int fh_pica_receiver_plane(FhPicaReceiverPlane *out,
    const float view[3][4],const float p[4][4],float near_z,float far_z){
    if(p[0][3]!=0 || p[1][3]!=0 || p[2][0]!=0 || p[2][1]!=0 ||
       p[3][0]!=0 || p[3][1]!=0 || p[3][3]!=0 || !(far_z>near_z))return 0;
    float determinant=p[0][0]*p[1][1]-p[0][1]*p[1][0];
    if(!(fabsf(determinant)>1e-8f) || !(fabsf(p[3][2])>1e-8f))return 0;
    float best=0,n[3]={0};
    for(unsigned a=0;a<3;a++){
        unsigned b=(a+1)%3;
        float q[3]={view[1][a]*view[2][b]-view[2][a]*view[1][b],
            view[2][a]*view[0][b]-view[0][a]*view[2][b],
            view[0][a]*view[1][b]-view[1][a]*view[0][b]};
        float length=q[0]*q[0]+q[1]*q[1]+q[2]*q[2];
        if(length>best){best=length;for(unsigned k=0;k<3;k++)n[k]=q[k];}
    }
    if(!isfinite(best) || best<1e-12f)return 0;
    float d=n[0]*view[0][3]+n[1]*view[1][3]+n[2]*view[2][3],length=sqrtf(best);
    if(!isfinite(d) || fabsf(d)<.001f*length)return 0;
    d+=copysignf(.25f*length,d);
    float x=(n[0]*p[1][1]-n[1]*p[1][0])/determinant;
    float y=(n[1]*p[0][0]-n[0]*p[0][1])/determinant;
    float k=p[2][3]/(p[3][2]*d);
    float a=k*x*p[3][2],b=k*y*p[3][2];
    float c=p[2][2]/p[3][2]+k*(n[2]-x*p[0][2]-y*p[1][2]);
    float radius=fabsf(a)+fabsf(b),span=fmaxf(2*radius,1.f/65536),maximum=c+radius;
    out->equation[0]=a*.8f/span;out->equation[1]=b*.8f/span;
    out->equation[2]=0;out->equation[3]=(c-maximum)*.8f/span-.1f;
    out->depth[0]=(near_z-far_z)*span/ .8f;
    out->depth[1]=1-far_z+(near_z-far_z)*(maximum+span*.125f);
    for(unsigned i=0;i<4;i++)if(!isfinite(out->equation[i]))return 0;
    return isfinite(out->depth[0]) && isfinite(out->depth[1]);
}
void fh_pica_receiver_init(void);
/* Selects caster mode in the same resident program; no code replacement. */
void *fh_pica_receiver_caster_program(void);
void *fh_pica_receiver_project_program(void);
void fh_pica_receiver_clear_image(void);
void fh_pica_receiver_draw(const FhPicaReceiverPlane*,const unsigned clip[4]);
void fh_pica_receiver_shutdown(void);
#endif
