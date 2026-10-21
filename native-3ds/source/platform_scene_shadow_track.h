/* Included by platform_native_scene.c, with the parent's budgets and fence.
 * Activation awaits the collision receiver consumer; installed rendering is
 * unchanged. No original collision/game state is retained or modified here. */
static unsigned track_gpu,track_cpu,track_peak,track_builds,track_declined,track_invalidated,track_updates,track_changed;
static unsigned long long track_flush_bytes;
enum { TRACK_GPU_LIMIT=1024*1024,TRACK_CPU_LIMIT=256*1024,TRACK_SCRATCH_LIMIT=2*1024*1024 };
static void prepare_shadow_track(SceneRecord*r,const void*raw,unsigned bytes){
    if(!fh_pica_shadow_track_enabled || !r->asset.block || r->track_plan)return;
    /* raw measurement itself needs bounded decoding/hash scratch. Account for
     * that before entering it, not only after measuring the final geometry. */
    if(!normal_reserve(TRACK_SCRATCH_LIMIT))goto declined;
    FhShadowTrackLiveSize size;int16_t offset;
    if(!fh_pica_shadow_track_raw_live_compile(raw,bytes,NULL,0,NULL,0,NULL,0,NULL,0,&size,&offset))goto declined;
    unsigned vb=size.base.corners*sizeof(FhShadowTrackLiveVertex),ib=size.base.corners*sizeof(uint16_t);
    /* A triangle stream can have an odd number of u16 indices. Align draws. */
    unsigned db=size.base.draws*sizeof(FhShadowTrackDraw),draw_at=(vb+ib+3)&~3u,ref_at=draw_at+db,rb=size.references*sizeof(FhShadowTrackLiveRef),temp_bytes=ref_at+rb;
    unsigned cpu=sizeof(FhShadowTrackPlan)+rb;
    if(temp_bytes>TRACK_SCRATCH_LIMIT || size.scratch_bytes>TRACK_SCRATCH_LIMIT-temp_bytes ||
       !normal_reserve(temp_bytes+size.scratch_bytes+cpu))goto declined;
    /* Empty blocks still have a valid plan: absence of surfaces is not a load
     * failure and must not reconnect visible-world geometry as a substitute. */
    unsigned char*temp=malloc(temp_bytes?temp_bytes:4);if(!temp)goto declined;
    FhShadowTrackLiveVertex*v=(void*)temp;uint16_t*ix=(void*)(temp+vb);
    FhShadowTrackDraw*d=(void*)(temp+draw_at);FhShadowTrackLiveRef*refs=(void*)(temp+ref_at);FhShadowTrackRoutes surfaces;
    if(!fh_pica_shadow_track_raw_live_compile(raw,bytes,v,size.base.corners,ix,size.base.corners,d,size.base.draws,refs,size.references,&size,&offset) ||
       !fh_pica_shadow_track_live_routes(ix,size.base.corners,d,size.base.draws,NULL,0,&surfaces)){
        free(temp);goto declined;
    }
    unsigned index_at=(vb+31)&~31u;
    unsigned gpu_bytes=(index_at+ib+surfaces.extra_indices*sizeof(uint16_t)+31)&~31u;
    if(gpu_bytes>TRACK_GPU_LIMIT-track_gpu || cpu>TRACK_CPU_LIMIT-track_cpu ||
       gpu_bytes>SCENE_GPU_LIMIT-fh_native_scene_gpu_bytes ||
       !normal_reserve(cpu) || !fh_minimal_linear_room(gpu_bytes) ||
       linearSpaceFree()<gpu_bytes+SCENE_LINEAR_RESERVE){free(temp);goto declined;}
    FhShadowTrackPlan*p=malloc(cpu);void*gpu=gpu_bytes?linearMemAlign(gpu_bytes,128):NULL;
    if(!p || (gpu_bytes && !gpu)){free(p);linearFree(gpu);free(temp);goto declined;}
    /* All data is private until the flush and final record publication. */
    if(gpu_bytes){
        memset(gpu,0,gpu_bytes);memcpy(gpu,v,vb);
        uint16_t*dst=(void*)((unsigned char*)gpu+index_at);memcpy(dst,ix,ib);
        if(!fh_pica_shadow_track_live_routes(ix,size.base.corners,d,size.base.draws,
                dst+size.base.corners,surfaces.extra_indices,&surfaces)){
            free(p);linearFree(gpu);free(temp);goto declined;
        }
        fh_gpu_cache_flush(gpu,gpu_bytes);
    }
    *p=(FhShadowTrackPlan){.gpu=gpu,.bytes=gpu_bytes,.vertices=size.base.corners,.indices_offset=index_at,.y_offset=offset,.surfaces=surfaces,
        .cpu_bytes=cpu,.source_vertices=be16((const unsigned char*)raw+0x90),.reference_count=size.references};
    memcpy(p->references,refs,rb);
    free(temp);r->track_plan=p;
    track_gpu+=gpu_bytes;track_cpu+=cpu;track_builds++;
    if(track_gpu>track_peak)track_peak=track_gpu;
    fh_native_scene_gpu_bytes+=gpu_bytes;fh_native_scene_cpu_bytes+=cpu;return;
declined:
    track_declined++;
}
static void release_shadow_track(SceneRecord*r){
    FhShadowTrackPlan*p=r->track_plan;if(!p)return;
    /* Parent forget joins once for all resources; shutdown has already drained
     * both owners. Do not reopen a frame while releasing shutdown resources. */
    track_gpu-=p->bytes;track_cpu-=p->cpu_bytes;
    fh_native_scene_gpu_bytes-=p->bytes;fh_native_scene_cpu_bytes-=p->cpu_bytes;
    linearFree(p->gpu);free(p);r->track_plan=NULL;
}

/* Called inside the existing world-position publication boundary, after prior
 * GPU readers have retired. No second fence, allocation or shader CPU math. */
static void refresh_shadow_track(SceneRecord*r,const void*positions,unsigned count){
    FhShadowTrackPlan*p=r->track_plan;if(!p)return;
    if(count!=p->source_vertices){fh_platform_panic("PICA collision source vertex count");return;}
    unsigned first,last;
    unsigned changed=fh_pica_shadow_track_live_positions(p->gpu,p->references,p->reference_count,
        (const int16_t(*)[3])positions,p->y_offset,&first,&last,&p->surfaces);
    if(changed){fh_gpu_cache_flush((unsigned char*)p->gpu+first,last-first);track_flush_bytes+=last-first;}
    track_updates++;track_changed+=changed;r->track_dirty=0;
}
