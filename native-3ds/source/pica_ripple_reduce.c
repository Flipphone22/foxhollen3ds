#include "fh_pica_ripple_reduce.h"
#include "fh_install_simplify.h"
#include <stdlib.h>
#include <string.h>

/* Geometry remains the original planar UV domain. Only the persistent index
 * topology changes; exact surviving UV/displacement/padding bytes are kept. */
static int64_t twice_area(const FhWaterGridVertex *v,unsigned a,unsigned b,unsigned c){
    int64_t abx=(int)v[b].uv_delta[0]-v[a].uv_delta[0];
    int64_t aby=(int)v[b].uv_delta[1]-v[a].uv_delta[1];
    int64_t acx=(int)v[c].uv_delta[0]-v[a].uv_delta[0];
    int64_t acy=(int)v[c].uv_delta[1]-v[a].uv_delta[1];
    return abx*acy-aby*acx;
}
int fh_pica_ripple_reduce(FhWaterGridVertex *v,uint16_t *ids,
                         FhWaterGridSize *size,size_t *scratch_peak){
    if(!v || !ids || !size || !scratch_peak)return 0;
    unsigned nv=size->vertices,ni=size->indices;
    if(!nv || nv>4356 || !ni || ni>25350 || ni%3 ||
       size->vertex_bytes!=nv*sizeof(*v) || size->index_bytes!=ni*sizeof(*ids))return 0;
    int64_t original_area=0;
    for(unsigned i=0;i<nv;i++){
        if(v[i].uv_delta[0]<0 || v[i].uv_delta[0]>16384 ||
           v[i].uv_delta[1]<0 || v[i].uv_delta[1]>16384 ||
           v[i].uv_delta[2]<-2048 || v[i].uv_delta[2]>2032 ||
           v[i].uv_delta[3]<-2048 || v[i].uv_delta[3]>2032 ||
           v[i].padding[0] || v[i].padding[1])return 0;
    }
    for(unsigned i=0;i<ni;i++)if(ids[i]>=nv)return 0;
    for(unsigned i=0;i<ni;i+=3){
        int64_t area=twice_area(v,ids[i],ids[i+1],ids[i+2]);
        if(area<=0)return 0;
        original_area+=area;
    }
    /* Bounded arithmetic above: <=201588 B caller scratch, plus <=101400 B
     * kernel output and its fixed1MiB arena. No resident/per-frame allocation. */
    size_t bytes=(size_t)nv*(5*sizeof(float)+sizeof(uint16_t)+1)+(size_t)ni*sizeof(unsigned);
    unsigned char *memory=(unsigned char*)malloc(bytes);
    if(!memory)return 0;
    float *position=(float*)memory,*attribute=position+3*nv;
    unsigned *source=(unsigned*)(attribute+2*nv);
    uint16_t *remap=(uint16_t*)(source+ni);
    unsigned char *locks=(unsigned char*)(remap+nv);
    for(unsigned i=0;i<nv;i++){
        position[3*i]=v[i].uv_delta[0]/16384.f;
        position[3*i+1]=v[i].uv_delta[1]/16384.f;position[3*i+2]=0;
        attribute[2*i]=v[i].uv_delta[2]/4096.f;attribute[2*i+1]=v[i].uv_delta[3]/4096.f;
    }
    for(unsigned i=0;i<ni;i++)source[i]=ids[i];
    memset(locks,0,nv);
    const float weights[2]={1,1};FhInstallSimplification reduced={0};
    int ok=fh_install_simplify(&reduced,source,ni,position,nv,attribute,2,weights,locks,
        .1f,.002f,0,FH_RIPPLE_REDUCE_SCRATCH);
    if(ok){
        int64_t area=0;
        for(unsigned i=0;i<reduced.count;i+=3){
            int64_t triangle=twice_area(v,reduced.indices[i],reduced.indices[i+1],reduced.indices[i+2]);
            if(triangle<0){ok=0;break;}
            area+=triangle;
        }
        if(!reduced.count || reduced.count>ni || area!=original_area)ok=0;
    }
    if(ok){
        memset(remap,255,nv*sizeof(*remap));
        for(unsigned i=0;i<reduced.count;i++)remap[reduced.indices[i]]=0;
        unsigned kept=0;
        for(unsigned i=0;i<nv;i++)if(remap[i]!=UINT16_MAX){remap[i]=(uint16_t)kept;v[kept++]=v[i];}
        for(unsigned i=0;i<reduced.count;i++)ids[i]=remap[reduced.indices[i]];
        size->vertices=kept;size->indices=reduced.count;
        size->vertex_bytes=kept*sizeof(*v);size->index_bytes=reduced.count*sizeof(*ids);
        /* Budget the full reserved arena, not merely meshopt's used prefix. */
        *scratch_peak=bytes+(size_t)ni*sizeof(unsigned)+FH_RIPPLE_REDUCE_SCRATCH+15;
    }
    fh_install_simplify_close(&reduced);free(memory);return ok;
}
