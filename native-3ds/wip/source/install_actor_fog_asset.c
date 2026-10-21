#include "fh_install_actor_fog_asset.h"
#include "fh_install_actor_fog_pose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
typedef struct {
    const FhInstallVertexSource *source;
    FhInstallActorFogAsset *out;
    FhInstallSkinWeights *positions,*normals;
    FhInstallActorFogRoute route;
    FhInstallActorFogCoordinate coordinate;
    void *context;
    unsigned char *gpu,*refs;
    unsigned groups,sections,gpu_bytes,ref_bytes,plans,gpu_capacity,ref_capacity;
    unsigned char group[256][240],section[256][32],plan[512][36];
} Builder;
static void put(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static void half(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;}
static unsigned word(const unsigned char*p){return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static unsigned aligned(unsigned n,unsigned alignment){return (n+alignment-1)&~(alignment-1);}
static int fail(Builder*b,const char*why){snprintf(b->out->error,sizeof(b->out->error),"%s",why);return 0;}
/* Load/conversion scratch only: these buffers have never been published to the
 * GPU. Grow with the emitted packet instead of reserving both maximum caps for
 * every model. realloc failure preserves the old allocation for cleanup. */
static int reserve(unsigned char**data,unsigned*capacity,unsigned need,unsigned limit){
    if(need>limit)return 0;
    if(need<=*capacity)return 1;
    unsigned next=*capacity?*capacity:4096;
    while(next<need){
        unsigned step=next/2;
        if(step>limit-next){next=limit;break;}
        next+=step;
    }
    if(next>limit)next=limit;
    unsigned char*p=realloc(*data,next);
    if(!p)return 0;
    *data=p;*capacity=next;return 1;
}
void fh_install_actor_fog_asset_close(FhInstallActorFogAsset*a){if(a){free(a->allocation);memset(a,0,sizeof(*a));}}
static int route(void*context,const FhInstallDrawSource*d,FhPicaActorFogRouting*out){
    Builder*b=context;return b->route(b->context,d,out);
}
static int coordinate(void*context,const FhInstallDrawSource*d,unsigned*out){
    Builder*b=context;return b->coordinate(b->context,d,out);
}
static int group(void*context,const FhInstallActorFogGroup*g){
    Builder*b=context;unsigned primary=aligned(b->gpu_bytes,16),primary_end=primary+g->vertices*40,
        fog=aligned(primary_end,16),index=fog+g->vertices*16,end=index+g->indices*2;
    if(b->groups==256 || g->sections>256-b->sections || end>FH_ACTOR_FOG_GPU_CAP ||
       g->vertices>(FH_ACTOR_FOG_META_CAP-b->ref_bytes)/16)return fail(b,"Actor fog exceeds bounded packet storage.");
    if(!reserve(&b->gpu,&b->gpu_capacity,end,FH_ACTOR_FOG_GPU_CAP) ||
       !reserve(&b->refs,&b->ref_capacity,b->ref_bytes+g->vertices*16,FH_ACTOR_FOG_META_CAP))
        return fail(b,"Cannot grow bounded actor fog staging.");
    memset(b->gpu+b->gpu_bytes,0,primary-b->gpu_bytes);memset(b->gpu+primary_end,0,fog-primary_end);
    if(!fh_install_actor_fog_pack(b->source,b->positions,b->normals,g,
        (FhModelCompactVertex*)(b->gpu+primary),(FhPicaActorFogGpuVertex*)(b->gpu+fog),g->vertices))
        return fail(b,"Cannot pack actor fog GPU streams.");
    unsigned char*p=b->group[b->groups];
    const unsigned fields[]={primary,fog,index,g->vertices,g->indices,g->bone_count,g->fog_count,
        b->sections,g->sections,g->vat,g->flags,g->shader,g->two_uv,b->ref_bytes};
    for(unsigned i=0;i<14;i++)put(p+i*4,fields[i]);
    for(unsigned i=0;i<g->bone_count;i++)half(p+56+i*2,g->bones[i]);
    memcpy(p+100,g->fog,g->fog_count*4);
    for(unsigned i=0;i<g->fog_count;i++){
        unsigned k=0;
        while(k<b->plans && memcmp(b->plan[k],g->fog+i,4))k++;
        if(k==b->plans){
            FhPicaActorFogPosePlan pose;
            if(k==512 || !fh_install_actor_fog_pose_plan(&pose,1,b->source,g->fog+i,1))
                return fail(b,"Cannot resolve actor fog source pose.");
            memcpy(b->plan[k],g->fog+i,4);memcpy(b->plan[k]+4,&pose,32);b->plans++;
        }
        half(p+192+i*2,k);
    }
    memcpy(b->refs+b->ref_bytes,g->ref,g->vertices*16);
    for(unsigned i=0;i<g->indices;i++)half(b->gpu+index+i*2,g->index[i]);
    for(unsigned i=0;i<g->sections;i++){
        const FhInstallSection*s=g->section+i;
        if(s->ordinal!=b->sections || s->stream || s->bit>65535 || s->list>=256 || s->shader>=64)
            return fail(b,"Actor fog source section cannot be represented.");
        const unsigned fields[]={s->list,s->shader,s->stream,s->bit,s->first,s->count,s->ordinal,0};
        for(unsigned k=0;k<8;k++)put(b->section[b->sections]+k*4,fields[k]);
        b->sections++;
    }
    b->groups++;b->gpu_bytes=end;b->ref_bytes+=g->vertices*16;return 1;
}
int fh_install_actor_fog_asset(FhInstallActorFogAsset*out,const void*raw,unsigned bytes,
    unsigned id,unsigned variant,const FhPicaActorFogMatrixBank*initial,unsigned skip,
    FhInstallActorFogRoute resolver,FhInstallActorFogCoordinate selector,void*context){
    if(!out)return 0;
    fh_install_actor_fog_asset_close(out);FhInstallVertexSource source;
    if(id>>31 || !initial || !resolver || !selector || skip>1 ||
       !fh_install_vertex_source(&source,raw,bytes,0)){
        snprintf(out->error,sizeof(out->error),"Invalid actor fog source.");return 0;
    }
    unsigned draws=0;if(fh_install_descriptors(raw,bytes,0,NULL,NULL,&draws)!=1){
        snprintf(out->error,sizeof(out->error),"Invalid actor fog source descriptions.");return 0;
    }
    if(!draws)return 1;
    Builder*b=calloc(1,sizeof(*b));if(!b){snprintf(out->error,sizeof(out->error),"Cannot allocate actor fog builder.");return 0;}
    b->source=&source;b->out=out;b->route=resolver;b->coordinate=selector;b->context=context;
    int ok=0;
    if(source.fixed_world){
        b->positions=calloc(source.positions_count?source.positions_count:1,sizeof(*b->positions));
        b->normals=calloc(source.normals_count?source.normals_count:1,sizeof(*b->normals));
        if(!b->positions || !b->normals ||
           !fh_install_skin_stream(&source,0,b->positions,source.positions_count) ||
           !fh_install_skin_stream(&source,1,b->normals,source.normals_count)){
            fail(b,"Cannot prepare original actor fog skin maps.");goto done;
        }
    }
    if(fh_install_actor_fog_groups(&source,b->positions,b->normals,initial,skip,route,coordinate,group,b,&out->groups)!=1){
        if(!out->error[0])fail(b,out->groups.error);goto done;
    }
    unsigned sections=96+b->groups*240,refs=sections+b->sections*32,
        plans=refs+b->ref_bytes,meta=aligned(plans+b->plans*36,128);
    if(meta>FH_ACTOR_FOG_META_CAP){fail(b,"Actor fog metadata exceeds256 KiB.");goto done;}
    out->allocation=malloc(meta+b->gpu_bytes+127);
    if(!out->allocation){fail(b,"Cannot allocate completed actor fog packet.");goto done;}
    unsigned char*p=(unsigned char*)(((uintptr_t)out->allocation+127)&~(uintptr_t)127);
    memset(p,0,meta);memcpy(p,"FHAFG002",8);
    unsigned source_crc=(unsigned)crc32(0,raw,bytes),bones=source.joints_count?source.joints_count+source.fixed_world:1;
    const unsigned fields[]={meta,b->gpu_bytes,id,bytes,source_crc,variant,b->groups,b->sections,96,sections,refs,
        bones,source.joints_count,source.blends_count,source.positions_count,source.normals_count,
        source.colors_count,source.uvs_count,source.fixed_world,b->ref_bytes,b->plans,plans};
    for(unsigned i=0;i<22;i++)put(p+8+i*4,fields[i]);
    memcpy(p+96,b->group,b->groups*240);memcpy(p+sections,b->section,b->sections*32);
    if(b->ref_bytes)memcpy(p+refs,b->refs,b->ref_bytes);
    memcpy(p+plans,b->plan,b->plans*36);
    for(unsigned i=0;i<b->groups;i++){unsigned char*g=p+96+i*240;put(g+52,refs+word(g+52));}
    if(b->gpu_bytes)memcpy(p+meta,b->gpu,b->gpu_bytes);FhActorFogAsset check;
    if(!fh_native_actor_fog_open(&check,p,meta,p+meta,b->gpu_bytes,id,bytes,source_crc,variant)){
        fail(b,"Native actor fog reader rejected generated packet.");goto done;
    }
    out->metadata=p;out->gpu=p+meta;out->metadata_bytes=meta;out->gpu_bytes=b->gpu_bytes;ok=1;
done:
    free(b->positions);free(b->normals);free(b->gpu);free(b->refs);free(b);
    if(!ok){free(out->allocation);out->allocation=NULL;out->metadata=out->gpu=NULL;out->metadata_bytes=out->gpu_bytes=0;}
    return ok;
}
