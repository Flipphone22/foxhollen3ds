#include "gx_internal.h"
#include <math.h>
#include <string.h>
_Static_assert(sizeof(FhLight)==sizeof(GXLightObj),"Light object storage ABI");
#ifndef FH_GX_PRODUCER_IMPL
int fh_gx_native_lighting(FhNativeLighting *out){
    return fh_native_lighting_pack(out,fh_gx.lights,fh_gx.channel,fh_gx.ambient[0],fh_gx.material[0]);
}
#endif
static FhLight *light(GXLightObj *object) { return (FhLight*)object; }
void GXInitLightAttnA(GXLightObj *object,f32 a,f32 b,f32 c) { float v[]={a,b,c};memcpy(light(object)->a,v,sizeof(v)); }
void GXInitLightAttnK(GXLightObj *object,f32 a,f32 b,f32 c) { float v[]={a,b,c};memcpy(light(object)->k,v,sizeof(v)); }
void GXInitLightAttn(GXLightObj *object,f32 a,f32 b,f32 c,f32 d,f32 e,f32 f) { GXInitLightAttnA(object,a,b,c);GXInitLightAttnK(object,d,e,f); }
void GXGetLightAttnK(GXLightObj *object,f32 *a,f32 *b,f32 *c) { const FhLight *l=(const FhLight*)object;*a=l->k[0];*b=l->k[1];*c=l->k[2]; }
void GXInitLightPos(GXLightObj *object,f32 x,f32 y,f32 z) { float v[]={x,y,z};memcpy(light(object)->position,v,sizeof(v)); }
void GXInitLightDir(GXLightObj *object,f32 x,f32 y,f32 z) { float v[]={-x,-y,-z};memcpy(light(object)->direction,v,sizeof(v)); }
void GXInitSpecularDir(GXLightObj *object,f32 x,f32 y,f32 z) {
    float hx=-x,hy=-y,hz=1-z,length=sqrtf(hx*hx+hy*hy+hz*hz);if(length==0) length=1;
    GXInitLightPos(object,x*1e18f,y*1e18f,z*1e18f);
    light(object)->direction[0]=hx/length;light(object)->direction[1]=hy/length;light(object)->direction[2]=hz/length;
}
void GXInitLightColor(GXLightObj *object,GXColor color) { float *c=light(object)->color;c[0]=color.r/255.f;c[1]=color.g/255.f;c[2]=color.b/255.f;c[3]=color.a/255.f; }
void GXLoadLightObjImm(GXLightObj *object,GXLightID id) {
    fh_gx_require(id && !(id&(id-1)) && (unsigned)id<=128,"Invalid light slot");
    fh_gx.lights[__builtin_ctz((unsigned)id)]=*light(object);
}
void GXInitLightDistAttn(GXLightObj *object,f32 distance,f32 brightness,GXDistAttnFn type) {
    float a=1,b=0,c=0;
    if(distance>0 && brightness>0 && brightness<1) {
        float ratio=(1-brightness)/brightness;
        if(type==GX_DA_GENTLE) b=ratio/distance;
        else if(type==GX_DA_MEDIUM) { b=.5f*ratio/distance;c=.5f*ratio/(distance*distance); }
        else if(type==GX_DA_STEEP) c=ratio/(distance*distance);
    }
    GXInitLightAttnK(object,a,b,c);
}
void GXInitLightSpot(GXLightObj *object,f32 cutoff,GXSpotFn type) {
    float a=1,b=0,c=0;
    if(cutoff<=0 || cutoff>90) type=GX_SP_OFF;
    float cosine=cosf(cutoff*.017453292519943295f),d=(1-cosine)*(1-cosine);
    switch(type) {
    case GX_SP_FLAT:a=-1000*cosine;b=1000;break;
    case GX_SP_COS:a=-cosine/(1-cosine);b=1/(1-cosine);break;
    case GX_SP_COS2:a=0;b=-cosine/(1-cosine);c=1/(1-cosine);break;
    case GX_SP_SHARP:a=cosine*(cosine-2);b=2/d;c=-1/d;break;
    case GX_SP_RING1:a=4*cosine/d;b=4*(1+cosine)/d;c=-4/d;break;
    case GX_SP_RING2:a=1-2*cosine*cosine/d;b=4*cosine/d;c=-2/d;break;
    default:break;
    }
    GXInitLightAttnA(object,a,b,c);
}
void GXSetNumChans(u8 count) { fh_gx_require(count<=2,"Invalid color channel count");fh_gx.channels=count; }
void GXSetChanCtrl(GXChannelID id,GXBool enabled,GXColorSrc ambient,GXColorSrc material,u32 mask,GXDiffuseFn diffuse,GXAttnFn attenuation) {
    if(id==GX_COLOR0A0 || id==GX_COLOR1A1) { GXSetChanCtrl(id&1,enabled,ambient,material,mask,diffuse,attenuation);GXSetChanCtrl((id&1)+2,enabled,ambient,material,mask,diffuse,attenuation);return; }
    /* GX's channel register forces diffuse off for specular attenuation.
     * Match pinned Aurora GXLighting.cpp and Melee's native GXSetChanCtrl. */
    fh_gx_require((unsigned)id<4,"Invalid lighting channel");fh_gx.channel[id]=(FhChannel){enabled,ambient,material,mask,attenuation==GX_AF_SPEC?GX_DF_NONE:diffuse,attenuation};
}
static void set_color(float target[2][4],GXChannelID id,GXColor value) {
    fh_gx_require((unsigned)id<6,"Invalid channel color");float *c=target[id&1];
    if(id<2 || id>=4) { c[0]=value.r/255.f;c[1]=value.g/255.f;c[2]=value.b/255.f; }
    if(id>=2) c[3]=value.a/255.f;
}
void GXSetChanAmbColor(GXChannelID id,GXColor value) { set_color(fh_gx.ambient,id,value); }
void GXSetChanMatColor(GXChannelID id,GXColor value) { set_color(fh_gx.material,id,value); }
