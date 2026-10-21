#ifndef FH_PICA_DISTORTION_UNIFORMS_H
#define FH_PICA_DISTORTION_UNIFORMS_H
#include "fh_pica_distortion_plan.h"

/* Seven consecutive vec4 uniforms consumed by pica_distortion.v.pica.
 * Source image dimensions are GX texel dimensions, not native capture dimensions.
 * Mesh offsets store IA8 bytes in A,I order. Aurora's indirect .abg therefore
 * consumes A for S and I for T. The source matrix is diagonal with exponent 1.
 * Source sample rounding and mesh interpolation remain separate error budgets. */
typedef struct { float row[7][4]; } FhPicaDistortionUniforms;
static inline int fh_pica_distortion_uniforms(FhPicaDistortionUniforms *out,
    const FhPicaDistortionPlan *plan, unsigned source_width,
    unsigned source_height, const float viewport[4])
{
    if(!out||!plan||!viewport||!source_width||!source_height)return 0;
    for(unsigned i=0;i<6;i++)if(!isfinite(plan->rotation[i]))return 0;
    for(unsigned i=0;i<4;i++)
        if(!isfinite(plan->radial[i])||!isfinite(viewport[i]))return 0;
    if(!isfinite(plan->indirect)||plan->indirect<0||plan->indirect>0.5f)return 0;
    const float *m=plan->rotation;
    float determinant=m[0]*m[4]-m[1]*m[3];
    if(!isfinite(determinant)||determinant==0)return 0;
    FhPicaDistortionUniforms u={0};
    u.row[0][0]=m[4]/determinant;u.row[0][1]=-m[1]/determinant;
    u.row[1][0]=-m[3]/determinant;u.row[1][1]=m[0]/determinant;
    for(unsigned i=0;i<2;i++)
        u.row[i][2]=-(u.row[i][0]*m[2]+u.row[i][1]*m[5]);
    u.row[2][0]=2*plan->indirect/source_width;
    u.row[3][1]=2*plan->indirect/source_height;
    for(unsigned i=0;i<4;i++){
        u.row[4][i]=plan->radial[i];u.row[5][i]=viewport[i];
    }
    /* Conservative field-space screen rectangle bound. Only the zero-offset
     * outer ring expands; all detailed field vertices remain unchanged.
     * Account for the 128-gon's apothem, not merely its circumradius. */
    float maxx=0,maxy=0;
    for(unsigned y=0;y<2;y++)for(unsigned x=0;x<2;x++){
        float a=fabsf(m[0]*x+m[1]*y+m[2]-0.5f);
        float b=fabsf(m[3]*x+m[4]*y+m[5]-0.5f);
        if(a>maxx)maxx=a;
        if(b>maxy)maxy=b;
    }
    float extent=hypotf(maxx,maxy)/0.7497741f+0.00001f;
    u.row[6][0]=extent>1?extent-1:0;
    u.row[6][1]=0.4f; /* inner radius squared <= .25, outer .5625 */
    u.row[6][2]=0.5f;
    for(unsigned i=0;i<7;i++)for(unsigned j=0;j<4;j++)
        if(!isfinite(u.row[i][j]))return 0;
    *out=u;return 1;
}
#endif
