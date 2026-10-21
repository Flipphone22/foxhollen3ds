/* Included only by the single-producer GX dispatcher. This first integration
 * retains existing model joins. Cross-model dispatch is not yet enabled. */
#include "fh_owned_draw_sources.h"
enum { OWNED_BYTES=256*1024, OWNED_DRAWS=8, OWNED_EXCLUSIONS=12 };
static FhOwnedSources owned_pool;
static FhOwnedSourceLease owned_model;
static FhOwnedSourceLease owned_textures;
static struct { FhOwnedSourceLease lease;unsigned active,done;const void *list;unsigned bytes; } owned_draws[OWNED_DRAWS];
static struct {const void *address;size_t bytes;} owned_exclusions[OWNED_EXCLUSIONS];
static unsigned owned_exclusion_count;
static unsigned owned_seen_generation[FH_OWNED_SOURCE_SLOTS];
static uintptr_t owned_seen_identity[FH_OWNED_SOURCE_SLOTS];
static unsigned owned_seen_bytes[FH_OWNED_SOURCE_SLOTS];
extern unsigned fh_gx_owned_sources_mode,fh_gx_owned_draws,fh_gx_owned_declines;
extern unsigned fh_renderer_source_exclusions(const void **,size_t *,unsigned);
static void owned_retire(void){
    for(unsigned i=0;i<OWNED_DRAWS;i++)if(owned_draws[i].active && __atomic_load_n(&owned_draws[i].done,__ATOMIC_ACQUIRE)){
        if(!fh_owned_source_release(&owned_pool,&owned_draws[i].lease))fh_platform_panic("Owned GX source retirement failed");
        owned_draws[i].active=0;
    }
}
static void owned_begin(void){
    if(!fh_gx_owned_sources_mode)return;
    if(fh_gx_state_batch_mode!=1)fh_platform_panic("Owned GX sources require producer state");
    owned_retire();
    if(!owned_pool.storage){
        void *storage=fh_renderer_cpu_cache_alloc(OWNED_BYTES);
        if(!storage){fh_gx_owned_sources_mode=0;return;}
        if(!fh_owned_sources_init(&owned_pool,storage,OWNED_BYTES))fh_platform_panic("Owned GX source arena initialization");
        fh_gx_owned_source_bytes=OWNED_BYTES;
    }
    const void *addresses[OWNED_EXCLUSIONS];size_t bytes[OWNED_EXCLUSIONS];
    owned_exclusion_count=fh_renderer_source_exclusions(addresses,bytes,OWNED_EXCLUSIONS);
    if(owned_exclusion_count>OWNED_EXCLUSIONS)fh_platform_panic("Owned GX exclusion budget exceeded");
    for(unsigned i=0;i<owned_exclusion_count;i++)owned_exclusions[i]=(typeof(owned_exclusions[0])){addresses[i],bytes[i]};
}
static int owned_ready(const void *data,size_t bytes,void *context){
    (void)context;
    for(unsigned i=0;i<owned_exclusion_count;i++)if(fh_owned_overlap((uintptr_t)data,bytes,
        (uintptr_t)owned_exclusions[i].address,owned_exclusions[i].bytes))return 0;
    return 1;
}
static int owned_immutable(const void *data,size_t bytes,void *context){
    (void)context;
    if(bytes>UINT32_MAX)return 0;
    unsigned immutable=fh_retained_producer_scope_contains(data,(unsigned)bytes);
    for(unsigned i=0;!immutable && i<leased_count;i++)immutable=data==leased_lists[i].data && bytes==leased_lists[i].bytes;
    if(immutable && fh_retained_scope_validate){
        const void *snapshot=fh_owned_source_resolve(&owned_model,data,bytes);
        if(snapshot && memcmp(snapshot,data,bytes))fh_platform_panic("Owned model immutable source changed");
    }
    return immutable;
}
static void owned_model_end(void){
    if(!fh_owned_source_release(&owned_pool,&owned_model))fh_platform_panic("Owned model source retirement failed");
    if(!fh_owned_source_release(&owned_pool,&owned_textures))fh_platform_panic("Owned texture resource retirement failed");
    fh_gx_owned_copied_bytes=owned_pool.copied_bytes;
    fh_gx_owned_compared_reuse_bytes=owned_pool.reused_bytes;
    fh_gx_owned_shared_bytes=owned_pool.shared_bytes;
}
static void owned_written(const void *address,size_t bytes){
    if(!fh_owned_source_forget(&owned_pool,&owned_textures,address,bytes))fh_platform_panic("Owned texture write retirement failed");
    if(!fh_owned_source_forget(&owned_pool,&owned_model,address,bytes))fh_platform_panic("Owned model write retirement failed");
}
static int owned_capture(const void *list,unsigned bytes){
    if(!fh_gx_owned_sources_mode || !owned_pool.storage)return -1;
    owned_retire();
    for(unsigned i=0;i<OWNED_DRAWS;i++)if(!owned_draws[i].active){
        if(!fh_owned_draw_capture_resources(&owned_pool,&owned_draws[i].lease,&owned_model,owned_immutable,
            &owned_textures,fh_retained_scope_validate,&fh_gxp_state,list,bytes,owned_ready,NULL))break;
        fh_gx_owned_source_peak=owned_pool.peak;
        owned_draws[i].list=list;owned_draws[i].bytes=bytes;
        owned_draws[i].done=0;owned_draws[i].active=1;++fh_gx_owned_draws;return (int)i;
    }
    ++fh_gx_owned_declines;return -1;
}
static void owned_consume(unsigned slot){
    if(slot>=OWNED_DRAWS || !owned_draws[slot].active)fh_platform_panic("Invalid owned GX draw");
    /* Keep original addresses in the GX state/patch stream. Only this draw's
     * synchronous translator sees the remapped sources. Derived draw state
     * produced by translation must survive the source-pointer restoration. */
    const unsigned char *arrays[26];const void *pixels[8],*palettes[8];
    for(unsigned i=0;i<26;i++)arrays[i]=fh_gx.arrays[i].data;
    for(unsigned i=0;i<8;i++){pixels[i]=fh_gx.draw.textures[i].pixels;palettes[i]=fh_gx.draw.textures[i].palette;}
    FhOwnedSourceLease *lease=&owned_draws[slot].lease;
    const void *list=fh_owned_draw_bind(&fh_gx,lease,owned_draws[slot].list,owned_draws[slot].bytes);
    if(!list)fh_platform_panic("Owned GX draw lost a source binding");
    /* Original identity is separate from readable copied bytes. Publish a
     * content change once per copied version, in draw order. CPU array/list
     * caches still verify bytes; texture caches also require this notification. */
    if(fh_owned_active_sources)fh_platform_panic("Nested owned GX source view");
    fh_owned_active_sources=lease;
    for(unsigned i=0;i<lease->count;i++){
        const FhOwnedSource *s=lease->source+i;
        if(owned_seen_generation[s->slot]!=s->generation){
            /* An older pinned version may be consumed again after a newer
             * overlapping version. Invalidate its observation as well. */
            for(unsigned j=0;j<FH_OWNED_SOURCE_SLOTS;j++)if(owned_seen_generation[j] &&
                fh_owned_overlap(s->identity,s->bytes,owned_seen_identity[j],owned_seen_bytes[j]))owned_seen_generation[j]=0;
            fh_renderer_invalidate((const void*)s->identity,s->bytes);
            owned_seen_generation[s->slot]=s->generation;owned_seen_identity[s->slot]=s->identity;owned_seen_bytes[s->slot]=s->bytes;
        }
    }
    fh_gxd_GXCallDisplayList(list,owned_draws[slot].bytes);
    fh_owned_active_sources=NULL;
    for(unsigned i=0;i<26;i++)fh_gx.arrays[i].data=arrays[i];
    for(unsigned i=0;i<8;i++){fh_gx.draw.textures[i].pixels=pixels[i];fh_gx.draw.textures[i].palette=palettes[i];}
    __atomic_store_n(&owned_draws[slot].done,1,__ATOMIC_RELEASE);
}
