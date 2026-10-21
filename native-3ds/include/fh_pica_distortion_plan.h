#ifndef FH_PICA_DISTORTION_PLAN_H
#define FH_PICA_DISTORTION_PLAN_H
#include <math.h>
#include <stdint.h>

/* Per-call uniforms only. No framebuffer/texture/geometry ownership here.
 * Projection order matches Camera_ProjectWorldSphere's six output arguments.
 * This compiler is not yet connected to a draw consumer. */
typedef struct {
    uint32_t color[4];
    float radial[4]; /* scale U/V, translation U/V */
    float rotation[6]; /* row-major 2x3 texture transform */
    float indirect; /* source matrix entry, before its exponent=1 */
} FhPicaDistortionPlan;

static inline int fh_pica_distortion_plan(FhPicaDistortionPlan *out,
    const float projection[6], float radius, const unsigned char mod[3],
    float angle, float texture_scale, float alpha_radius, float indirect_radius)
{
    if(!out||!projection||!mod||!isfinite(radius)||radius<=0||
       !isfinite(angle)||!isfinite(texture_scale)||!isfinite(alpha_radius)||alpha_radius<0||
       !isfinite(indirect_radius)||indirect_radius<0)return 0;
    for(unsigned i=0;i<6;i++)if(!isfinite(projection[i]))return 0;
    float depth=projection[2]+1.0f;
    if(depth<0||depth>=256||projection[3]==0||projection[4]==0)return 0;
    FhPicaDistortionPlan p={0};
    const unsigned base[4]={66,129,25,16};
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<3;c++)
        p.color[i]|=((base[i]+(mod[c]>>(i==3?3:2)))&255u)<<(8*c);
    p.color[0]|=((uint32_t)(16777216.0f*depth)&0x00ff0000u)<<8;
    float ratio=alpha_radius/radius;
    if(!isfinite(ratio))return 0;
    float fade=ratio>0?sqrtf(ratio):ratio;
    p.color[1]|=(uint32_t)(fade>1?255:255.0f*fade)<<24;
    fade*=2; if(fade>1)fade=1;
    p.color[3]|=(uint32_t)(255.0f*fade)<<24;
    p.radial[0]=texture_scale/projection[3];
    p.radial[1]=texture_scale/projection[4];
    float x=0.5f*(-projection[0])-0.5f,y=0.5f*projection[1]-0.5f;
    p.radial[2]=p.radial[0]*x+0.5f;p.radial[3]=p.radial[1]*y+0.5f;
    float c=cosf(angle),s=sinf(angle);
    p.rotation[0]=c*0.8f;p.rotation[1]=-s*0.8f;
    p.rotation[3]=s*0.8f;p.rotation[4]=c*0.8f;
    p.rotation[2]=c*(0.8f*x)-s*(0.8f*y)+0.5f;
    p.rotation[5]=s*(0.8f*x)+c*(0.8f*y)+0.5f;
    p.indirect=indirect_radius/radius;if(p.indirect>0.5f)p.indirect=0.5f;
    for(unsigned i=0;i<4;i++)if(!isfinite(p.radial[i]))return 0;
    for(unsigned i=0;i<6;i++)if(!isfinite(p.rotation[i]))return 0;
    *out=p;return 1;
}
#endif
