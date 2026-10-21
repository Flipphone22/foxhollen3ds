/* Included by the existing scene owner. All mutations happen at load/release
 * boundaries; retirement is the same fence as the parent geometry. */
static unsigned shadow_plan_gpu,shadow_plan_cpu,shadow_plan_peak,shadow_plan_builds,shadow_plan_declined;
static void prepare_shadow_plan(SceneRecord *r,const unsigned char *raw){
    (void)raw;
    if(!r->actor_plan)return;
    unsigned scratch=fh_pica_shadow_scratch(&r->asset,r->actor_plan);
    if(!scratch || !normal_reserve(scratch)){shadow_plan_declined++;return;}
    /* Morph capability does not imply a live deformed stream. The draw owner
     * selects this base plan only when fh_pica_morph_view is NULL; active morphs
     * still consume their fenced native lanes. The same optional budgets apply. */
    FhPicaShadowSize size;
    FhPicaShadowPrepared *prepared=fh_pica_shadow_prepare(&r->asset,r->actor_plan,&size);
    if(!prepared)return;
    if(size.count>=r->actor_plan->count){fh_pica_shadow_prepared_close(prepared);return;}
    unsigned cpu=sizeof(FhPicaShadowPlan)+size.count*sizeof(FhPicaShadowDraw);
    /* Prepared scratch is already allocated and included in current heap use. */
    if(size.bytes>512*1024-shadow_plan_gpu || cpu>64*1024-shadow_plan_cpu || !normal_reserve(cpu) ||
       size.bytes>SCENE_GPU_LIMIT-fh_native_scene_gpu_bytes ||
       !fh_minimal_linear_room(size.bytes) || linearSpaceFree()<size.bytes+SCENE_LINEAR_RESERVE){fh_pica_shadow_prepared_close(prepared);shadow_plan_declined++;return;}
    FhPicaShadowPlan *p=malloc(cpu);void *gpu=linearMemAlign(size.bytes,128);
    if(!p || !gpu){free(p);linearFree(gpu);fh_pica_shadow_prepared_close(prepared);shadow_plan_declined++;return;}
    if(!fh_pica_shadow_emit(prepared,p->draw,size.count,gpu,size.bytes)){
        free(p);linearFree(gpu);fh_pica_shadow_prepared_close(prepared);shadow_plan_declined++;return;
    }
    fh_pica_shadow_prepared_close(prepared);
    p->count=size.count;p->bytes=size.bytes;p->gpu=gpu;r->shadow_plan=p;r->shadow_valid=1;
    fh_gpu_cache_flush(gpu,size.bytes);
    shadow_plan_gpu+=size.bytes;shadow_plan_cpu+=cpu;shadow_plan_builds++;
    if(shadow_plan_gpu>shadow_plan_peak)shadow_plan_peak=shadow_plan_gpu;
    fh_native_scene_gpu_bytes+=size.bytes;fh_native_scene_cpu_bytes+=cpu;
}
static void release_shadow_plan(SceneRecord *r){
    FhPicaShadowPlan *p=r->shadow_plan;if(!p)return;
    unsigned cpu=sizeof(*p)+p->count*sizeof(FhPicaShadowDraw);
    shadow_plan_gpu-=p->bytes;shadow_plan_cpu-=cpu;
    fh_native_scene_gpu_bytes-=p->bytes;fh_native_scene_cpu_bytes-=cpu;
    linearFree(p->gpu);free(p);r->shadow_plan=NULL;r->shadow_valid=0;
}
static void shadow_plans_yield(unsigned gpu){
    if(!shadow_plan_gpu || gpu>SCENE_GPU_LIMIT)return;
    if(gpu<=SCENE_GPU_LIMIT-fh_native_scene_gpu_bytes &&
       linearSpaceFree()>=gpu+SCENE_LINEAR_RESERVE && fh_minimal_linear_room(gpu))return;
    /* Optional batching yields to essential geometry, after its owning fence.
     * Casters then use the same unmerged native mesh; no effect disappears. */
    fh_renderer_scene_storage_join();
    for(unsigned i=0;i<SCENE_RECORDS;i++)release_shadow_plan(records+i);
}
