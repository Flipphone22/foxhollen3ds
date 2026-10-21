#include "fh_pica_glow_field.h"
#include <math.h>

static unsigned address(int x,unsigned size,unsigned mode)
{
    if(!mode)return x<0?0:((unsigned)x>=size?size-1:(unsigned)x);
    int period=(int)size*(mode==2?2:1);
    x%=period;if(x<0)x+=period;
    return mode==2&&(unsigned)x>=size?(unsigned)(period-1-x):(unsigned)x;
}
size_t fh_glow_field_bytes(const FhGlowField *f)
{
    if(!f||!f->rgba||!f->width||!f->height||
       f->width>1024||f->height>1024||f->wrap_s>2||f->wrap_t>2||
       f->format>1)return 0;
    if(f->format){
        if(f->stride||(f->width&3)||(f->height&3))return 0;
        size_t required=(size_t)f->width*f->height*2;
        return f->bytes>=required&&(uintptr_t)f->rgba<=UINTPTR_MAX-required?required:0;
    }
    size_t row=(size_t)f->width*4;
    if(f->stride<row||(f->height>1&&f->stride>(SIZE_MAX-row)/(f->height-1)))return 0;
    size_t required=(f->height-1)*f->stride+row;
    return f->bytes>=required&&(uintptr_t)f->rgba<=UINTPTR_MAX-required?required:0;
}
int fh_glow_field_sample_uv(void *context,float u,float v,float output[2])
{
    const FhGlowField *f=context;
    if(!output||!fh_glow_field_bytes(f)||!isfinite(u)||!isfinite(v)||
       u< -256||u>256||v< -256||v>256)return 0;
    float x=u*f->width-.5f,y=v*f->height-.5f;
    int ix=(int)floorf(x),iy=(int)floorf(y);
    float fx=x-ix,fy=y-iy,b=0,g=0;
    for(unsigned dy=0;dy<2;dy++)for(unsigned dx=0;dx<2;dx++){
        unsigned sx=address(ix+(int)dx,f->width,f->wrap_s);
        unsigned sy=address(iy+(int)dy,f->height,f->wrap_t);
        unsigned blue,green;
        if(f->format){
            size_t at=(((size_t)(sy/4)*(f->width/4)+sx/4)*16+(sy%4)*4+sx%4)*2;
            unsigned word=((unsigned)f->rgba[at]<<8)|f->rgba[at+1];
            if(word&0x8000){
                blue=word&31;green=(word>>5)&31;
                blue=(blue<<3)|(blue>>2);green=(green<<3)|(green>>2);
            }else{
                blue=(word&15)*17;green=((word>>4)&15)*17;
            }
        }else{
            const uint8_t *p=f->rgba+(size_t)sy*f->stride+(size_t)sx*4;
            blue=p[2];green=p[1];
        }
        float weight=(dx?fx:1-fx)*(dy?fy:1-fy);
        b+=blue*weight;g+=green*weight;
    }
    output[0]=(floorf(b+.5f)-128)*.125f;
    output[1]=(floorf(g+.5f)-128)*.125f;
    return 1;
}
int fh_glow_field_sample(void *context,float u,float v,float output[2])
{
    if(u<0||u>1||v<0||v>1)return 0;
    return fh_glow_field_sample_uv(context,u,v,output);
}
int fh_glow_field_region_sample(void *context,float u,float v,float output[2])
{
    const FhGlowFieldRegion *r=context;
    if(!r||!isfinite(u)||!isfinite(v)||u<0||u>1||v<0||v>1)return 0;
    float s=(r->uv[0]+((int)r->uv[2]-r->uv[0])*u)*(1.f/128.f);
    float t=(r->uv[1]+((int)r->uv[3]-r->uv[1])*v)*(1.f/128.f);
    return fh_glow_field_sample_uv((void*)&r->field,s,t,output);
}
