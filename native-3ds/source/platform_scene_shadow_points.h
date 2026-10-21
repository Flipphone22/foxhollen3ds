/* Included by the native scene owner. Same owner retirement and aggregate
 * budgets as essential scene resources. No second renderer/worker/queue. */
#include "fh_pica_shadow_ranges.h"
static unsigned track_gpu,track_cpu,track_peak,track_builds,track_declined,track_invalidated,track_updates,track_changed;
static unsigned long long track_flush_bytes;
static unsigned track_decline_stage[7],track_scratch_peak;
enum { TRACK_GPU_LIMIT=1024*1024,TRACK_CPU_LIMIT=256*1024,TRACK_SCRATCH_LIMIT=2*1024*1024 };
static void prepare_shadow_track(SceneRecord*r,const void*raw,unsigned bytes){
    if(!fh_pica_shadow_track_enabled || !r->asset.block || r->track_plan)return;
    unsigned stage=0;
    unsigned initial_scratch;
    if(!fh_pica_shadow_track_raw_scratch(raw,bytes,&initial_scratch) ||
       initial_scratch>TRACK_SCRATCH_LIMIT || !normal_reserve(initial_scratch))goto declined;
    if(initial_scratch>track_scratch_peak)track_scratch_peak=initial_scratch;
    stage=1;
    FhShadowPointSize size;int16_t offset;unsigned scratch;
    if(!fh_pica_shadow_points_raw_compile(raw,bytes,NULL,0,NULL,0,NULL,0,&size,&scratch,&offset))goto declined;
    unsigned vb=size.gpu_bytes,db=size.draw_bytes,rb=size.reference_bytes;
    unsigned draw_at=vb,ref_at=draw_at+db,temp_bytes=ref_at+rb,cpu=sizeof(FhShadowPointPlan)+rb;
    stage=2;
    if(temp_bytes>TRACK_SCRATCH_LIMIT || scratch>TRACK_SCRATCH_LIMIT-temp_bytes ||
       !normal_reserve(temp_bytes+scratch+cpu))goto declined;
    unsigned char*temp=malloc(temp_bytes?temp_bytes:4);if(!temp)goto declined;
    FhShadowPoint*v=(void*)temp;FhShadowTrackDraw*d=(void*)(temp+draw_at);
    FhShadowTrackLiveRef*refs=(void*)(temp+ref_at);FhShadowPointRoutes surfaces;
    stage=3;
    if(!fh_pica_shadow_points_raw_compile(raw,bytes,v,size.points,d,size.draws,refs,size.points,&size,&scratch,&offset) ||
       !fh_pica_shadow_points_routes(v,size.points,d,size.draws,NULL,0,offset,&surfaces)){
        free(temp);goto declined;
    }
    unsigned index_at=vb; /* Every point is32B, already aligned for u16 indices. */
    unsigned gpu_bytes=(vb+surfaces.extra_indices*sizeof(uint16_t)+31)&~31u;
    unsigned range_offset[2]={0},range_nodes[2]={0};
    cpu=(cpu+3)&~3u;
    for(unsigned route=0;route<2;route++){
        unsigned n=surfaces.route[route].count;if(!n)continue;
        if(route && !surfaces.route[route].indexed && n==surfaces.route[0].count){range_offset[route]=range_offset[0];continue;}
        range_nodes[route]=fh_pica_shadow_range_nodes(n);range_offset[route]=cpu;
        cpu+=range_nodes[route]*sizeof(FhShadowRangeNode);
    }
    unsigned cache_offset=0;
    if(size.points){
        unsigned align=_Alignof(FhShadowRangeCache);
        cache_offset=(cpu+align-1)&~(align-1);cpu=cache_offset+sizeof(FhShadowRangeCache);
    }
    stage=4;
    if(gpu_bytes>TRACK_GPU_LIMIT-track_gpu || cpu>TRACK_CPU_LIMIT-track_cpu ||
       gpu_bytes>SCENE_GPU_LIMIT-fh_native_scene_gpu_bytes || !normal_reserve(cpu) ||
       !fh_minimal_linear_room(gpu_bytes) || linearSpaceFree()<gpu_bytes+SCENE_LINEAR_RESERVE){free(temp);goto declined;}
    stage=5;
    FhShadowPointPlan*p=malloc(cpu);void*gpu=gpu_bytes?linearMemAlign(gpu_bytes,128):NULL;
    if(!p || (gpu_bytes && !gpu)){free(p);linearFree(gpu);free(temp);goto declined;}
    stage=6;
    if(gpu_bytes){
        memset(gpu,0,gpu_bytes);memcpy(gpu,v,vb);
        uint16_t*dst=(void*)((unsigned char*)gpu+index_at);
        if(!fh_pica_shadow_points_routes(v,size.points,d,size.draws,dst,surfaces.extra_indices,offset,&surfaces)){
            free(p);linearFree(gpu);free(temp);goto declined;
        }
    }
    for(unsigned route=0;route<2;route++)if(range_nodes[route]){
        const uint16_t*ix=surfaces.route[route].indexed?(const void*)((unsigned char*)gpu+index_at):NULL;
        FhShadowRangeNode*tree=(void*)((unsigned char*)p+range_offset[route]);
        if(!fh_pica_shadow_range_compile(v,size.points,ix,surfaces.route[route].count,tree,range_nodes[route])){
            free(p);linearFree(gpu);free(temp);goto declined;
        }
    }
    if(cache_offset)memset((unsigned char*)p+cache_offset,0,sizeof(FhShadowRangeCache));
    if(gpu_bytes)fh_gpu_cache_flush(gpu,gpu_bytes);
    *p=(FhShadowPointPlan){.gpu=gpu,.bytes=gpu_bytes,.points=size.points,.indices_offset=index_at,.y_offset=offset,.surfaces=surfaces,
        .cpu_bytes=cpu,.source_vertices=be16((const unsigned char*)raw+0x90),.range_offset={range_offset[0],range_offset[1]},.range_cache_offset=cache_offset};
    memcpy(p->references,refs,rb);free(temp);r->track_plan=p;
    track_gpu+=gpu_bytes;track_cpu+=cpu;track_builds++;if(track_gpu>track_peak)track_peak=track_gpu;
    fh_native_scene_gpu_bytes+=gpu_bytes;fh_native_scene_cpu_bytes+=cpu;return;
declined:
    track_declined++;track_decline_stage[stage]++;
}
static void release_shadow_track(SceneRecord*r){
    FhShadowPointPlan*p=r->track_plan;if(!p)return;
    /* The parent forget/shutdown has already retired readers. */
    track_gpu-=p->bytes;track_cpu-=p->cpu_bytes;
    fh_native_scene_gpu_bytes-=p->bytes;fh_native_scene_cpu_bytes-=p->cpu_bytes;
    linearFree(p->gpu);free(p);r->track_plan=NULL;
}
static void refresh_shadow_track(SceneRecord*r,const void*positions,unsigned count){
    FhShadowPointPlan*p=r->track_plan;if(!p)return;
    if(count!=p->source_vertices){fh_platform_panic("PICA point source vertex count");return;}
    unsigned first,last;
    unsigned changed=fh_pica_shadow_points_update(p->gpu,p->references,p->points,(const int16_t(*)[3])positions,&first,&last);
    if(changed){
        const uint16_t*alt=(const void*)((const unsigned char*)p->gpu+p->indices_offset);
        fh_pica_shadow_points_bounds(p->gpu,alt,p->y_offset,&p->surfaces);
        fh_gpu_cache_flush((unsigned char*)p->gpu+first,last-first);track_flush_bytes+=last-first;
    }
    track_updates++;track_changed+=changed;r->track_dirty=0;
}
