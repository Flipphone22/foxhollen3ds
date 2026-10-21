#include "fh_pica_shadow_plan.h"
#include "fh_pica_index_cache.h"
#include <stdlib.h>
#include <string.h>
struct FhPicaShadowPrepared {
    const FhSceneAsset *asset;
    const FhPicaModelPlan *model;
    FhPicaShadowSize size;
    unsigned vertices,reserved;
    FhPicaShadowDraw draw[];
};
static unsigned align16(unsigned n){return (n+15u)&~15u;}
static unsigned slot(const uint16_t *ids,unsigned n,unsigned bone){
    for(unsigned i=0;i<n;i++)if(ids[i]==bone)return i;
    return n;
}
static int measure(const FhSceneAsset *a,const FhPicaModelPlan *p,FhPicaShadowSize *size,FhPicaShadowDraw *out,unsigned capacity){
    unsigned first=0,count=0,bytes=0;
    if(!a || !p || !size || a->block || !a->compact || !p->count || p->count!=a->groups || p->count>256)return 0;
    while(first<p->count){
        FhPicaShadowDraw draw={0};draw.first_group=first;draw.format=p->draw[first].format;
        for(unsigned i=first;i<p->count;i++){
            const FhPicaModelDraw *d=p->draw+i;
            if((d->vertex_offset&3) || (d->index_offset&1) ||
               (d->format!=3 && d->format!=5 && d->format!=6 && d->format!=7))return 0;
            if(!d->vertices || d->vertices>8192 || !d->indices || d->indices>32766 || d->indices%3 || !d->bones || d->bones>FH_MODEL_BONES)return 0;
            if(d->vertex_offset>a->gpu_bytes || d->vertices>(a->gpu_bytes-d->vertex_offset)/40 ||
               d->index_offset>a->gpu_bytes || d->indices>(a->gpu_bytes-d->index_offset)/2)return 0;
            uint16_t combined[FH_MODEL_BONES];memcpy(combined,draw.bone,sizeof(combined));unsigned n=draw.bones;
            for(unsigned b=0;b<d->bones;b++){
                if(d->bone[b]>=a->bones)return 0;
                if(slot(combined,n,d->bone[b])==n){if(n==FH_MODEL_BONES){n++;break;}combined[n++]=d->bone[b];}
            }
            if(draw.groups && (d->format!=draw.format || n>FH_MODEL_BONES || draw.vertices+d->vertices>32768 || draw.indices+d->indices>32766))break;
            if(n>FH_MODEL_BONES)return 0;
            memcpy(draw.bone,combined,sizeof(combined));draw.bones=n;draw.groups++;draw.vertices+=d->vertices;draw.indices+=d->indices;
        }
        draw.vertex_offset=align16(bytes);draw.index_offset=align16(draw.vertex_offset+draw.vertices*sizeof(FhPicaShadowVertex));
        bytes=align16(draw.index_offset+draw.indices*2);
        if(bytes>4*1024*1024)return 0;
        if(out){
            if(count>=capacity)return 0;
            for(unsigned b=0;b<draw.bones;b++)
                if(!count || b>=out[count-1].bones || draw.bone[b]!=out[count-1].bone[b])
                    draw.palette_changed|=1u<<b;
            out[count]=draw;
        }
        count++;first+=draw.groups;
    }
    *size=(FhPicaShadowSize){count,bytes};return 1;
}
static unsigned hash_slots(unsigned vertices){
    unsigned n=2;while(n<vertices*2)n*=2;return n;
}
static unsigned workspace_words(unsigned vertices,unsigned indices){
    unsigned hashes=hash_slots(vertices),order=(fh_pica_index_cache_scratch(vertices,indices)+indices*2+3)/4;
    return hashes>order?hashes:order;
}
unsigned fh_pica_shadow_scratch(const FhSceneAsset *a,const FhPicaModelPlan *p){
    FhPicaShadowSize size;
    if(!measure(a,p,&size,NULL,0))return 0;
    /* Retain every source-to-output index until emission. Reuse one hash table
     * across <=32768-vertex batches, with no second copy of vertex attributes. */
    unsigned vertices=0,indices=0;
    for(unsigned i=0;i<p->count;i++){vertices+=p->draw[i].vertices;indices+=p->draw[i].indices;}
    return sizeof(FhPicaShadowPrepared)+size.count*sizeof(FhPicaShadowDraw)+
        workspace_words(vertices>32768?32768:vertices,indices>32766?32766:indices)*sizeof(uint32_t)+vertices*sizeof(uint16_t);
}
/* Prefix describes the source groups, independent of deduplicated output.
 * Keys preserve every shader input, including both rebased skin lanes and the
 * complete bind-tail bytes. No epsilon, position-only merge or face sorting. */
static void shadow_vertex(FhPicaShadowVertex *v,const FhSceneAsset *a,const FhPicaModelPlan *p,
        const FhPicaShadowDraw *b,const unsigned prefix[257],unsigned ordinal){
    unsigned low=0,high=b->groups;
    while(low+1<high){unsigned mid=(low+high)/2;if(prefix[mid]<=ordinal)low=mid;else high=mid;}
    const FhPicaModelDraw *d=p->draw+b->first_group+low;
    const unsigned char *s=a->gpu+d->vertex_offset+(ordinal-prefix[low])*40;
    memcpy(v->position,s,8);memcpy(v->skin,s+24,4);memcpy(v->offset,s+28,12);
    for(unsigned k=0;k<2;k++)v->skin[k]=3*slot(b->bone,b->bones,d->bone[v->skin[k]/3]);
}
static unsigned share(const FhSceneAsset *a,const FhPicaModelPlan *p,const FhPicaShadowDraw *b,
        uint32_t *lookup,uint16_t *map){
    unsigned prefix[257],total=0;prefix[0]=0;
    for(unsigned i=0;i<b->groups;i++){total+=p->draw[b->first_group+i].vertices;prefix[i+1]=total;}
    unsigned slots=hash_slots(total),unique=0;memset(lookup,0,slots*sizeof(*lookup));
    for(unsigned i=0;i<total;i++){
        FhPicaShadowVertex v;shadow_vertex(&v,a,p,b,prefix,i);
        const unsigned char *bytes=(const unsigned char*)&v;uint32_t h=2166136261u;
        for(unsigned j=0;j<sizeof(v);j++)h=(h^bytes[j])*16777619u;
        unsigned at=h&(slots-1),original;
        while((original=lookup[at])!=0){
            FhPicaShadowVertex previous;shadow_vertex(&previous,a,p,b,prefix,original-1);
            if(!memcmp(&v,&previous,sizeof(v)))break;
            at=(at+1)&(slots-1);
        }
        if(original)map[i]=map[original-1];
        else{
            lookup[at]=i+1;map[i]=unique++;
        }
    }
    return unique;
}
void fh_pica_shadow_prepared_close(FhPicaShadowPrepared *prepared){free(prepared);}
FhPicaShadowPrepared *fh_pica_shadow_prepare(const FhSceneAsset *a,const FhPicaModelPlan *p,FhPicaShadowSize *size){
    FhPicaShadowSize required;
    if(!size)return 0;
    if(!measure(a,p,&required,NULL,0))return 0;
    /* Validate source indices/bone attributes even for unused vertices. */
    for(unsigned i=0;i<p->count;i++){
        const FhPicaModelDraw *d=p->draw+i;
        const uint16_t *indices=(const uint16_t*)(a->gpu+d->index_offset);
        for(unsigned j=0;j<d->indices;j++)if(indices[j]>=d->vertices)return 0;
        for(unsigned j=0;j<d->vertices;j++){
            const unsigned char *s=a->gpu+d->vertex_offset+j*40+24;
            if(s[0]%3 || s[1]%3 || s[0]/3>=d->bones || s[1]/3>=d->bones)return 0;
        }
    }
    unsigned scratch=fh_pica_shadow_scratch(a,p);if(!scratch)return 0;
    FhPicaShadowPrepared *prepared=malloc(scratch);if(!prepared)return 0;
    FhPicaShadowDraw *layout=prepared->draw;
    if(!measure(a,p,&required,layout,required.count)){free(prepared);return 0;}
    unsigned maximum=0,max_indices=0;
    for(unsigned i=0;i<required.count;i++){
        if(layout[i].vertices>maximum)maximum=layout[i].vertices;
        if(layout[i].indices>max_indices)max_indices=layout[i].indices;
    }
    unsigned reserved=workspace_words(maximum,max_indices);
    uint32_t *lookup=(uint32_t*)(layout+required.count);
    uint16_t *map=(uint16_t*)(lookup+reserved);unsigned end=0,vertices=0;
    for(unsigned i=0;i<required.count;i++){
        FhPicaShadowDraw *b=layout+i;unsigned original=b->vertices;
        b->vertices=share(a,p,b,lookup,map+vertices);vertices+=original;
        b->vertex_offset=align16(end);b->index_offset=align16(b->vertex_offset+b->vertices*sizeof(FhPicaShadowVertex));
        end=align16(b->index_offset+b->indices*2);
    }
    required.bytes=end;
    prepared->asset=a;prepared->model=p;prepared->size=required;
    prepared->vertices=vertices;prepared->reserved=reserved;
    *size=required;return prepared;
}
int fh_pica_shadow_emit(FhPicaShadowPrepared *prepared,FhPicaShadowDraw *out,unsigned capacity,void *gpu,unsigned gpu_bytes){
    if(!prepared || !out || !gpu || capacity<prepared->size.count || gpu_bytes<prepared->size.bytes)return 0;
    const FhSceneAsset *a=prepared->asset;const FhPicaModelPlan *p=prepared->model;
    uint32_t *lookup=(uint32_t*)(prepared->draw+prepared->size.count);
    const uint16_t *map=(const uint16_t*)(lookup+prepared->reserved);
    memset(gpu,0,prepared->size.bytes);
    for(unsigned i=0;i<prepared->size.count;i++){
        const FhPicaShadowDraw *b=prepared->draw+i;
        unsigned prefix[257],total=0,written=0;prefix[0]=0;
        for(unsigned j=0;j<b->groups;j++){total+=p->draw[b->first_group+j].vertices;prefix[j+1]=total;}
        for(unsigned j=0;j<total;j++)if(map[j]==written){
            FhPicaShadowVertex v;shadow_vertex(&v,a,p,b,prefix,j);
            memcpy((unsigned char*)gpu+b->vertex_offset+written*sizeof(v),&v,sizeof(v));written++;
        }
        uint16_t *indices=(uint16_t*)((unsigned char*)gpu+b->index_offset);unsigned at=0;
        for(unsigned j=0;j<b->groups;j++){
            const FhPicaModelDraw *d=p->draw+b->first_group+j;
            const uint16_t *source=(const uint16_t*)(a->gpu+d->index_offset);
            for(unsigned k=0;k<d->indices;k++)indices[at++]=map[prefix[j]+source[k]];
        }
        /* Only mask capture consumes this plan. Reuse dead dedup workspace for
         * both reordering and a temporary input copy; add no resident GPU data.
         * Other passes retain the parent asset's untouched native geometry. */
        unsigned needed=fh_pica_index_cache_scratch(b->vertices,b->indices);
        if(needed+b->indices*2>prepared->reserved*4)return 0;
        uint16_t *original=(uint16_t*)((unsigned char*)lookup+needed);
        memcpy(original,indices,b->indices*2);
        if(!fh_pica_index_cache_order(indices,original,b->vertices,b->indices,16,lookup,needed))return 0;
        map+=total;
    }
    memcpy(out,prepared->draw,prepared->size.count*sizeof(*out));return 1;
}
int fh_pica_shadow_compile(const FhSceneAsset *a,const FhPicaModelPlan *p,FhPicaShadowSize *size,FhPicaShadowDraw *out,unsigned capacity,void *gpu,unsigned gpu_bytes){
    if(!size || ((out || gpu) && (!out || !gpu)))return 0;
    FhPicaShadowSize required;
    FhPicaShadowPrepared *prepared=fh_pica_shadow_prepare(a,p,&required);if(!prepared)return 0;
    int ok=!gpu || fh_pica_shadow_emit(prepared,out,capacity,gpu,gpu_bytes);
    if(ok)*size=required;
    fh_pica_shadow_prepared_close(prepared);return ok;
}
