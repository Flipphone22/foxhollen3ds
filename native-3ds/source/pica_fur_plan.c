#include "fh_pica_fur_plan.h"
#include <math.h>
#include <string.h>
static unsigned big32(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static float bigfloat(const unsigned char*p){unsigned u=big32(p);float f;memcpy(&f,&u,4);return f;}
unsigned fh_pica_fur_compile(const FhSceneAsset*a,const FhPicaModelPlan*p,
    const void*source,unsigned bytes,FhPicaFurShape*out,unsigned capacity){
    if(!a || !p || !source || a->block || a->meta[7]=='B' || bytes<252 ||
       !a->bones || a->bones>256)return 0;
    const unsigned char *raw=source;unsigned joints=raw[0xf3],at=big32(raw+0x40);
    if(big32(raw+0xa4) || !at || at>bytes || joints*16>bytes-at || a->bones<joints || a->bones>joints+1)return 0;
    unsigned char used[256]={0};unsigned draws=0;
    for(unsigned i=0;i<p->count;i++){
        const FhPicaModelDraw*d=p->draw+i;if(!(d->flags&512))continue;
        if(!d->bones || d->bones>16)return 0;
        for(unsigned b=0;b<d->bones;b++){if(d->bone[b]>=a->bones)return 0;used[d->bone[b]]=1;}
        draws++;
    }
    if(!draws || (out && capacity<a->bones))return 0;
    FhPicaFurShape prepared[256]={0};
    for(unsigned b=0;b<joints;b++)if(used[b]){
        const unsigned char *shape=raw+at+b*16;float radius=bigfloat(shape+12);
        if(!isfinite(radius) || radius<=0)return 0;
        for(unsigned c=0;c<3;c++){
            float pivot=bigfloat(shape+c*4),tail;memcpy(&tail,a->meta+a->tails_at+(b*3+c)*4,4);
            if(!isfinite(pivot) || !isfinite(tail))return 0;
            prepared[b].pivot_tail[c]=pivot+tail;
            if(!isfinite(prepared[b].pivot_tail[c]))return 0;
        }
        for(unsigned step=1;step<=2;step++){
            float growth=log2f(1.f+step*(.08f/radius));
            if(!isfinite(growth))return 0;
            prepared[b].log_growth[step-1]=growth;
        }
    }
    if(out)memcpy(out,prepared,a->bones*sizeof(*out));
    return a->bones;
}
