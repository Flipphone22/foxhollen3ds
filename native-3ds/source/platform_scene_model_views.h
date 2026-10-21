/* Distinct live source arrays share model topology but not mutable GPU storage.
 * Every selected view still compares source bytes through the existing exact
 * validator. Pointer equality selects storage; it never proves immutability.
 * All allocations remain owned until the existing record-release GPU fence.
 * Adapted from pinned Melee renderer immutable queued-geometry ownership. */
#define MODEL_VIEW_GPU_LIMIT (fh_hardware_large_memory?2u*1024*1024:512u*1024)
static int model_view_matches(const SceneVersion *v,const void *const data[4]){
    for(unsigned i=0;i<4;i++)if(v[i].data!=data[i])return 0;
    return 1;
}
static SceneVersion *model_view_versions(SceneRecord *r){return r->active?r->active->versions:r->versions;}
static SceneSpan *model_view_spans(SceneRecord *r){return r->active?r->active->spans:r->spans;}
static unsigned char *model_view_snapshots(SceneRecord *r){return r->active?r->active->snapshots:r->snapshots;}
static unsigned *model_view_last_used(SceneRecord *r){return r->active?&r->active->last_used:&r->last_used;}
static void model_view_select(SceneRecord *r,const void *const data[4]){
    if(!model_views_mode || !fh_minimal_enabled || !r->asset.compact || r->asset.block)return;
    if(model_view_matches(model_view_versions(r),data))return;
    if(model_view_matches(r->versions,data)){
        r->active=NULL;r->asset.gpu=r->gpu;model_view_hits++;return;
    }
    for(SceneModelView *v=r->views;v;v=v->next)if(model_view_matches(v->versions,data)){
        r->active=v;r->asset.gpu=v->gpu;model_view_hits++;return;
    }
    unsigned gpu_bytes=r->asset.gpu_bytes,bytes=sizeof(SceneModelView);
    for(unsigned i=0;i<4;i++)bytes+=fh_scene_snapshot_space(r->spans[i].bytes);
    if(r->view_count>=7 || gpu_bytes>MODEL_VIEW_GPU_LIMIT-model_view_gpu_bytes ||
       gpu_bytes>SCENE_GPU_LIMIT-fh_native_scene_gpu_bytes || !normal_reserve(bytes) ||
       linearSpaceFree()<gpu_bytes+SCENE_LINEAR_RESERVE)goto fallback;
    if(!fh_hardware_large_memory && !fh_minimal_linear_room(gpu_bytes))goto fallback;
    SceneModelView *v=malloc(bytes);void *gpu=v?linearMemAlign(gpu_bytes,128):NULL;
    if(!v || !gpu){free(v);linearFree(gpu);goto fallback;}
    memset(v,0,sizeof(*v));v->gpu=gpu;v->allocation_bytes=bytes;v->unpublished=1;
    v->snapshots=(unsigned char*)(v+1);unsigned cursor=0;
    SceneSpan *old_spans=model_view_spans(r);unsigned char *old_snapshots=model_view_snapshots(r);
    for(unsigned i=0;i<4;i++){
        unsigned n=old_spans[i].bytes;v->spans[i]=(SceneSpan){cursor,n};
        memcpy(v->snapshots+cursor,old_snapshots+old_spans[i].at,n);
        cursor+=fh_scene_snapshot_space(n);
    }
    /* Old storage may still have GPU readers. CPU reads are safe; no queued
     * address is rebound or overwritten. New bytes are published after update. */
    memcpy(gpu,r->asset.gpu,gpu_bytes);
    v->next=r->views;r->views=v;r->active=v;r->asset.gpu=gpu;r->view_count++;
    fh_native_scene_cpu_bytes+=bytes;fh_native_scene_gpu_bytes+=gpu_bytes;
    model_view_gpu_bytes+=gpu_bytes;if(model_view_gpu_bytes>model_view_peak)model_view_peak=model_view_gpu_bytes;
    model_view_allocations++;return;
fallback:
    /* Keep the current backing and its last-reader token. Existing write
     * fencing remains mandatory when the bounded cache cannot admit a view. */
    model_view_fallbacks++;
}
static void model_view_publish(SceneRecord *r){
    if(fh_minimal_enabled)fh_gpu_cache_flush(r->asset.gpu,r->gpu_used_bytes);
    else GSPGPU_FlushDataCache(r->asset.gpu,r->gpu_used_bytes);
    scene_uploaded_bytes+=r->gpu_used_bytes;
    if(r->active)r->active->unpublished=0;
}
/* Caller has already completed the record's queued GPU readers. */
static void model_views_release(SceneRecord *r){
    SceneModelView *v=r->views;
    while(v){SceneModelView *next=v->next;
        fh_native_scene_cpu_bytes-=v->allocation_bytes;fh_native_scene_gpu_bytes-=r->asset.gpu_bytes;
        model_view_gpu_bytes-=r->asset.gpu_bytes;
        if(v->model_bounds)model_bounds_bytes-=r->asset.groups*sizeof(FhAnimatedBounds);
        free(v->model_bounds);linearFree(v->gpu);free(v);v=next;
    }
    fh_native_scene_cpu_bytes-=r->model_bounds_bytes;model_bounds_bytes-=r->model_bounds_bytes;
    free(r->model_bounds);r->model_bounds=NULL;r->model_bounds_bytes=0;
    r->views=r->active=NULL;r->view_count=0;r->asset.gpu=r->gpu;
}
