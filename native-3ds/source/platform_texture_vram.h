/* Hot sampled images, promoted only between completed game frames.
 * Consulted pinned Melee texture_vram.h, including its hardware-disable warning.
 * Departure: copies finish in a separate, non-presenting batch BEFORE pointer
 * replacement. Original linear images remain available for cold demotion.
 * No deferred frees, reverse copies or copying into running queues.
 * Included after acquire(); the main producer is the only cache owner. */
enum { BASE_VRAM_LIMIT=3*1024*1024,BASE_VRAM_RESERVE=1024*1024,
       BASE_VRAM_BATCH=256*1024,BASE_VRAM_COPIES=8,BASE_VRAM_COLD_FRAMES=120 };
static unsigned base_vram_enabled=0,base_vram_bytes,base_vram_peak;
static unsigned base_vram_start_frame=3; /* Development comparison; normal default is three. */
static unsigned base_vram_promotions,base_vram_releases,base_vram_batches;
static unsigned base_vram_failures,base_vram_hits,base_linear_hits;
static unsigned base_vram_demotions,base_vram_pressure,base_vram_shadow_bytes,base_vram_shadow_peak;
static u64 base_vram_copy_bytes,base_vram_ticks;
static int base_on_vram(const BaseTexture *t){
    return (uintptr_t)t->gpu.data>=0x1f000000u && (uintptr_t)t->gpu.data<0x1f600000u;
}
static void base_vram_forget(BaseTexture *t){
    if(base_on_vram(t)){
        if(base_vram_bytes<t->gpu_bytes)fh_platform_panic("Native VRAM accounting underflow");
        base_vram_bytes-=t->gpu_bytes;base_vram_releases++;
    }
    if(t->linear_shadow){
        if(base_vram_shadow_bytes<t->gpu_bytes)fh_platform_panic("Native VRAM shadow accounting underflow");
        base_vram_shadow_bytes-=t->gpu_bytes;linearFree(t->linear_shadow);t->linear_shadow=NULL;
    }
}
static void base_vram_report(void){
    char line[640];snprintf(line,sizeof(line),
        "Native texture VRAM: enabled=%u bytes=%u peak=%u limit=%u reserve=%u promotions=%u releases=%u batches=%u failures=%u hits=%u linear_hits=%u copy_bytes=%llu ticks=%llu demotions=%u pressure=%u shadow_bytes=%u shadow_peak=%u\n",
        base_vram_enabled,base_vram_bytes,base_vram_peak,BASE_VRAM_LIMIT,BASE_VRAM_RESERVE,
        base_vram_promotions,base_vram_releases,base_vram_batches,base_vram_failures,
        base_vram_hits,base_linear_hits,(unsigned long long)base_vram_copy_bytes,
        (unsigned long long)base_vram_ticks,base_vram_demotions,base_vram_pressure,
        base_vram_shadow_bytes,base_vram_shadow_peak);fh_platform_log(line);
}
/* Called only at an empty, retired frame boundary. Unlike Melee's cold-entry
 * deletion, retain the exact converted image: no decoding/SD reload on return.
 * Unlike reverse TextureCopy, this introduces no GPU->FCRAM transfer. Linear
 * images remain bounded by BASE_GPU_LIMIT (4MiB New / 2MiB Old), exactly as
 * with VRAM disabled. Additional residents remain capped at3MiB in VRAM. */
static int base_vram_demote_one(void){
    BaseTexture *oldest=NULL;
    for(unsigned i=0;i<BASE_TEXTURES;i++){
        BaseTexture *t=base_textures+i;
        if(!t->gpu_bytes || !base_on_vram(t) || !t->linear_shadow ||t->used_frame>fh_minimal_frames ||
           fh_minimal_frames-t->used_frame<BASE_VRAM_COLD_FRAMES)continue;
        if(!oldest || t->used_frame<oldest->used_frame)oldest=t;
    }
    if(!oldest)return 0;
    void *vram=oldest->gpu.data,*linear=oldest->linear_shadow;
    oldest->linear_shadow=NULL; /* Preserve this allocation across forget(). */
    if(base_vram_shadow_bytes<oldest->gpu_bytes)fh_platform_panic("Native VRAM demotion accounting underflow");
    base_vram_shadow_bytes-=oldest->gpu_bytes;
    base_vram_forget(oldest);oldest->gpu.data=linear;vramFree(vram);
    base_vram_demotions++;return 1;
}
static void base_vram_begin(void){
    if(!base_vram_enabled || fh_minimal_frames<base_vram_start_frame)return;
    /* begin() reaches here once per real frame, after acquire() retired old
     * GPU commands. present() joined every borrowed CPU descriptor already. */
    if(prepared_on_worker() || ui_count ||
       __atomic_load_n(&prepared_published,__ATOMIC_ACQUIRE)!=
       __atomic_load_n(&prepared_completed,__ATOMIC_ACQUIRE))
        fh_platform_panic("VRAM promotion has outstanding CPU readers");
    gxCmdQueue_s *q=&C3Di_GetContext()->gxQueue;
    if(q->numEntries || q->curEntry || q->lastEntry)
        fh_platform_panic("VRAM promotion requires an empty retired GPU queue");
    BaseTexture *chosen[BASE_VRAM_COPIES];void *dest[BASE_VRAM_COPIES];
    unsigned count=0,bytes=0,releases=0;u64 start=svcGetSystemTick();
    for(unsigned n=0;n<BASE_VRAM_COPIES;n++){
        BaseTexture *best=NULL,*blocked=NULL;unsigned free_vram=vramSpaceFree();
        for(unsigned i=0;i<BASE_TEXTURES;i++){
            BaseTexture *t=base_textures+i;
            if(!t->gpu_bytes || !t->valid || base_on_vram(t) || t->stable_frames<3 ||
               t->used_frame!=fh_minimal_frames || t->gpu_bytes>BASE_VRAM_BATCH-bytes ||
               t->gpu_bytes>BASE_VRAM_LIMIT)continue;
            /* Physical 20261008-141810 used L4 masks on TEXUNIT1 in VRAM,
             * matching the documented PICA corruption/hang hazard. Cache
             * entries can later serve any sampler: retain L4/A4 in linear
             * memory for their entire lifetime, regardless of first use.
             * ETC1 block compression is a different format, not L4/A4. */
            if(t->gpu.fmt==GPU_L4 || t->gpu.fmt==GPU_A4)continue;
            unsigned duplicate=0;for(unsigned j=0;j<count;j++)duplicate|=chosen[j]==t;
            if(duplicate)continue;
            if(t->gpu_bytes>BASE_VRAM_LIMIT-base_vram_bytes-bytes ||
               free_vram<BASE_VRAM_RESERVE+t->gpu_bytes){
                if(!blocked || t->gpu_bytes>blocked->gpu_bytes)blocked=t;
            }else if(!best || t->gpu_bytes>best->gpu_bytes)best=t;
        }
        if(!best)best=blocked;
        if(!best)break;
        while(best->gpu_bytes>BASE_VRAM_LIMIT-base_vram_bytes-bytes ||
              vramSpaceFree()<BASE_VRAM_RESERVE+best->gpu_bytes){
            base_vram_pressure++;
            if(releases++==BASE_VRAM_COPIES || !base_vram_demote_one())break;
        }
        if(best->gpu_bytes>BASE_VRAM_LIMIT-base_vram_bytes-bytes ||
           vramSpaceFree()<BASE_VRAM_RESERVE+best->gpu_bytes)break;
        void *v=vramAlloc(best->gpu_bytes);
        if(!v){base_vram_failures++;break;}
        /* Only registered linear texture allocations -> new VRAM. This does
         * not widen the existing depth/readback transfer contract. */
        const char *error=fh_gpu_texture_copy_layout_error((uintptr_t)best->gpu.data,
            best->gpu_bytes,0,(uintptr_t)v,best->gpu_bytes,0,best->gpu_bytes,FH_GPU_COPY_CONTIGUOUS);
        if(error)fh_platform_panic(error);
        extern u32 __ctru_linear_heap,__ctru_linear_heap_size;
        uintptr_t source=(uintptr_t)best->gpu.data;
        if(source<__ctru_linear_heap || source-__ctru_linear_heap>__ctru_linear_heap_size ||
            best->gpu_bytes>__ctru_linear_heap_size-(source-__ctru_linear_heap) ||
            (uintptr_t)v<0x1f000000u ||
            (uintptr_t)v>=0x1f600000u || best->gpu_bytes>0x1f600000u-(uintptr_t)v)
            fh_platform_panic("VRAM promotion allocation domain");
        chosen[count]=best;dest[count++]=v;bytes+=best->gpu_bytes;
    }
    if(!count)return;
    for(unsigned i=0;i<count;i++){
        BaseTexture *t=chosen[i];
        /* The immutable linear image was flushed by its conversion producer. */
        C3D_SyncTextureCopy(t->gpu.data,0,dest[i],0,t->gpu_bytes,FH_GPU_COPY_CONTIGUOUS);
    }
    /* No target was marked used in this fresh frame. End publishes copies;
     * acquire joins both GPU completion and SDK callbacks, and resets queue.
     * A stalled transfer retains BOTH allocations and every original binding.
     * Successful copies keep the immutable source as a bounded linear shadow. */
    fh_early_queue_defer();C3D_FrameEnd(GX_CMDLIST_FLUSH);acquire();
    for(unsigned i=0;i<count;i++){
        BaseTexture *t=chosen[i];void *old=t->gpu.data;
        if(t->linear_shadow)fh_platform_panic("VRAM promotion already owns a shadow");
        t->linear_shadow=old;t->gpu.data=dest[i];
        base_vram_shadow_bytes+=t->gpu_bytes;
        base_vram_bytes+=t->gpu_bytes;base_vram_promotions++;
    }
    if(base_vram_bytes>base_vram_peak)base_vram_peak=base_vram_bytes;
    if(base_vram_shadow_bytes>base_vram_shadow_peak)base_vram_shadow_peak=base_vram_shadow_bytes;
    base_vram_batches++;base_vram_copy_bytes+=bytes;base_vram_ticks+=svcGetSystemTick()-start;
    /* Every submit binds copied sampler values afresh; no pointer-only bind cache. */
}
