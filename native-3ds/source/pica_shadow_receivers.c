#include "fh_pica_shadow_receivers.h"
#include <math.h>
#include <string.h>
static int inside(const FhSceneAsset *a,const void *p,unsigned count,unsigned stride){
    uintptr_t base=(uintptr_t)a->gpu,at=(uintptr_t)p;
    return at>=base && at-base<=a->gpu_bytes && count<=(a->gpu_bytes-(unsigned)(at-base))/stride;
}
static float coordinate(const FhPicaWorldDraw*d,unsigned vertex,unsigned lane,unsigned compact){
    if(compact){
        int16_t value;memcpy(&value,(const unsigned char*)d->vertex+vertex*20+lane*2,2);
        float scale=d->format==7?1.f:d->format==5?1.f/8:1.f/256;
        return value*scale;
    }
    float value;memcpy(&value,(const unsigned char*)d->vertex+vertex*64+lane*4,4);return value;
}
unsigned fh_pica_shadow_receivers_compile(const FhSceneAsset*a,const FhPicaWorldPlan*p,FhPicaShadowReceiver*out,unsigned capacity){
    if(!a || !p || !a->block || !a->direct || !a->gpu || !p->count || p->count>a->groups || (out && capacity<p->count))return 0;
    for(unsigned i=0;i<p->count;i++){
        const FhPicaWorldDraw*d=p->draw+i;
        if(!d->vertices || !d->indices || d->indices%3 || ((uintptr_t)d->index&1) ||
           (d->format!=3 && d->format!=5 && d->format!=6 && d->format!=7) ||
           !inside(a,d->vertex,d->vertices,a->compact?20:64) || !inside(a,d->index,d->indices,2))return 0;
        for(unsigned j=0;j<d->indices;j++){
            if(d->index[j]>=d->vertices)return 0;
            for(unsigned k=0;k<3;k++)if(!isfinite(coordinate(d,d->index[j],k,a->compact)))return 0;
        }
    }
    if(out)for(unsigned i=0;i<p->count;i++){
        const FhPicaWorldDraw*d=p->draw+i;out[i].draw=d;
        for(unsigned k=0;k<3;k++)out[i].bounds.lo[k]=out[i].bounds.hi[k]=coordinate(d,d->index[0],k,a->compact);
        for(unsigned j=1;j<d->indices;j++)for(unsigned k=0;k<3;k++){
            float x=coordinate(d,d->index[j],k,a->compact);
            if(x<out[i].bounds.lo[k])out[i].bounds.lo[k]=x;
            if(x>out[i].bounds.hi[k])out[i].bounds.hi[k]=x;
        }
    }
    return p->count;
}
void fh_pica_shadow_receiver_planes(FhWorldPlane out[4],const float uv[2][4]){
    float planes[4][4];
    for(unsigned i=0;i<2;i++)for(unsigned k=0;k<4;k++){
        planes[i*2][k]=uv[i][k];planes[i*2+1][k]=-uv[i][k]+(k==3?1.f:0.f);
    }
    fh_world_planes_prepare(out,planes);
}
static unsigned aligned(unsigned n){return (n+15)&~15u;}
unsigned fh_pica_shadow_batches_compile(const FhSceneAsset*a,const FhPicaWorldPlan*p,FhPicaWorldPlan*out,void*gpu,unsigned capacity,unsigned*out_count){
    if(!a || !p || !p->count || !out_count || fh_pica_shadow_receivers_compile(a,p,NULL,0)!=p->count)return 0;
    if(out){
        unsigned count=0,needed=fh_pica_shadow_batches_compile(a,p,NULL,NULL,0,&count);
        if(!gpu || !needed || capacity<needed || *out_count<count)return 0;
    }
    unsigned bytes=0,batches=0;
    for(unsigned i=0;i<p->count;){
        const FhPicaWorldDraw*first=p->draw+i;unsigned end=i,nv=0,ni=0;
        while(end<p->count && end-i<4){
            const FhPicaWorldDraw*d=p->draw+end;
            if(d->format!=first->format || d->cull!=first->cull || nv+d->vertices>65535)break;
            nv+=d->vertices;ni+=d->indices;end++;
        }
        if(end==i)return 0;
        unsigned stride=a->compact?8:16,ib=aligned(nv*stride),size=ib+aligned(ni*2);
        if(out){
            if(bytes>capacity || size>capacity-bytes || !gpu)return 0;
            FhPicaWorldDraw*d=out->draw+batches;*d=*first;
            d->vertex=(unsigned char*)gpu+bytes;d->index=(uint16_t*)((unsigned char*)gpu+bytes+ib);d->vertices=nv;d->indices=ni;
            unsigned v=0,index=0;uint16_t*dst=(uint16_t*)d->index;
            for(unsigned j=i;j<end;j++){
                const FhPicaWorldDraw*source=p->draw+j;
                for(unsigned k=0;k<source->vertices;k++)memcpy((unsigned char*)d->vertex+(v+k)*stride,(const unsigned char*)source->vertex+k*(a->compact?20:64),stride);
                for(unsigned k=0;k<source->indices;k++)dst[index++]=source->index[k]+v;
                v+=source->vertices;
            }
        }
        bytes+=size;batches++;i=end;
    }
    if(out){
        /* Bounds use source positions, since packed position stride differs. */
        FhPicaShadowReceiver*receiver=(FhPicaShadowReceiver*)(out->draw+batches);
        unsigned source=0;
        for(unsigned i=0;i<batches;i++){
            receiver[i].draw=out->draw+i;
            unsigned remaining=out->draw[i].vertices,started=0;
            while(remaining){
                const FhPicaWorldDraw*d=p->draw+source++;
                for(unsigned j=0;j<d->indices;j++)for(unsigned k=0;k<3;k++){
                    float v=coordinate(d,d->index[j],k,a->compact);
                    if(!started || v<receiver[i].bounds.lo[k])receiver[i].bounds.lo[k]=v;
                    if(!started || v>receiver[i].bounds.hi[k])receiver[i].bounds.hi[k]=v;
                    if(k==2)started=1;
                }
                remaining-=d->vertices;
            }
        }
        out->count=batches;
    }
    *out_count=batches;return bytes;
}
