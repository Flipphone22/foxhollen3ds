#include "fh_gx_dispatch.h"
unsigned fh_gx_dispatch_enabled,fh_gx_dispatch_records,fh_gx_dispatch_scopes,fh_gx_dispatch_waits;
unsigned fh_gx_dispatch_draws,fh_gx_dispatch_direct_draws;
unsigned fh_gx_dispatch_barrier_counts[64];
unsigned fh_gx_state_filter_enabled,fh_gx_state_filtered_records;
unsigned fh_gx_matrix_filter_enabled,fh_gx_matrix_filtered_records,fh_gx_matrix_filter_checks;
unsigned fh_gx_dispatch_batch_enabled,fh_gx_dispatch_batches,fh_gx_dispatch_batch_records;
unsigned fh_gx_matrix_block_enabled,fh_gx_matrix_blocks,fh_gx_matrix_block_matrices;
unsigned fh_gx_matrix_block_records,fh_gx_matrix_block_words;
unsigned fh_gx_state_batch_mode,fh_gx_state_batch_setters,fh_gx_state_batch_packets;
unsigned fh_gx_state_batch_words,fh_gx_state_batch_checks;
unsigned fh_gx_owned_sources_mode,fh_gx_owned_draws,fh_gx_owned_declines;
unsigned fh_gx_owned_source_bytes,fh_gx_owned_source_peak;
unsigned fh_gx_owned_deferred_scopes,fh_gx_owned_deferred_pending,fh_gx_owned_deferred_joins;
unsigned long long fh_gx_owned_copied_bytes,fh_gx_owned_compared_reuse_bytes,fh_gx_owned_shared_bytes;
#ifdef FH_GX_DISPATCH_BUILD
#include "gx_internal.h"
#include "fh_gx_ring.h"
#include "fh_retained_geometry.h"
#include <string.h>
#include "fh_gx_dispatch_ops.h"
#include "fh_source_identity.h"
const FhOwnedSourceLease *fh_owned_active_sources;
#include "fh_gx_state_filter.h"
static FhGxStateFilter state_filter;
#include "fh_gx_matrix_filter.h"
static FhGxMatrixFilter matrix_filter;
_Static_assert(sizeof(FhGxMatrixFilter)==14340,"Matrix mirror BSS accounting");
static int fh_gxd_matrix_repeat(unsigned kind,unsigned id,const void *matrix,unsigned bytes){
    if(!fh_gx_matrix_filter_enabled)return 0;
    ++fh_gx_matrix_filter_checks;
    if(!fh_gx_matrix_filter_repeat(&matrix_filter,kind,id,matrix,bytes))return 0;
    ++fh_gx_matrix_filtered_records;return 1;
}
enum { RING_WORDS=65536,OP_SCOPE_BEGIN=247,OP_SCOPE_END=248,OP_OWNED_LIST=249,OP_STATE_PATCH=250,OP_STATE_CHECK=251,OP_MATRIX_BLOCK=252,OP_CACHE_WRITE=253,OP_LEASED_LIST=254,MAX_LISTS=256 };
static uint32_t storage[RING_WORDS] __attribute__((aligned(32)));
static FhGxRing queue;
static uint32_t *active_matrix_block;
static unsigned fh_gxd_recording,initialized;
static struct {const void *data;unsigned bytes;} leased_lists[MAX_LISTS];
static unsigned leased_count;
static const void *matrix_source;
static unsigned matrix_bytes;
static unsigned registration_end,reader_scope_open;
static unsigned borrowed_since_join,owned_since_join,deferred_end;
#ifdef FH_GX_STATE_BATCH_BUILD
#include "fh_state_delta.h"
static void owned_written(const void *,size_t);
static FhGx state_previous;
enum { STATE_PAGE_BYTES=64,STATE_PAGES=(sizeof(FhGx)+STATE_PAGE_BYTES-1)/STATE_PAGE_BYTES,
       STATE_DIRTY_WORDS=(STATE_PAGES+31)/32 };
static unsigned state_stale,fh_gxd_state_dirty;
static uint32_t state_dirty_pages[STATE_DIRTY_WORDS];
static unsigned state_ends[STATE_PAGES];
static uint32_t state_patch[sizeof(FhGx)/2];
_Static_assert(sizeof(FhGx)%4==0 && sizeof(FhGx)<65536,"GX state patch offset budget");
static inline void fh_gxd_mark_state(const void *field,unsigned bytes){
    unsigned begin=(const unsigned char*)field-(const unsigned char*)&fh_gxp_state;
    unsigned first=begin/STATE_PAGE_BYTES,last=(begin+bytes-1)/STATE_PAGE_BYTES;
    for(unsigned page=first;page<=last;page++)state_dirty_pages[page/32]|=1u<<(page%32);
    fh_gxd_state_dirty=1;
}
#define FH_STATE_MARK(member) fh_gxd_mark_state(&fh_gxp_state.member,sizeof(fh_gxp_state.member))
static void fh_gxd_prepare_state(void){
    if(active_matrix_block)fh_platform_panic("GX command inside unfinished model matrix block");
    if(state_stale){
        /* Only a completed producer barrier sets stale. No worker can be
         * reading or writing the translator while this snapshot is copied. */
        if(fh_gxd_state_dirty)fh_platform_panic("GX state resync lost pending writes");
        fh_gxp_state=fh_gx;state_previous=fh_gx;state_stale=0;
        if(!state_ends[0])for(unsigned i=0;i<STATE_PAGES;i++){
            unsigned end=(i+1)*(STATE_PAGE_BYTES/4);state_ends[i]=end<sizeof(FhGx)/4?end:sizeof(FhGx)/4;
        }
    }
}
static void fh_gxd_flush_state(void);
#endif
extern int fh_gxd_platform_start(void);
int fh_gxd_pending(void);
extern void fh_gxd_platform_wake(void),fh_gxd_platform_wait(void);
static void publish(void){fh_gx_ring_publish(&queue);fh_gxd_platform_wake();}
static uint32_t *fh_gxd_reserve(unsigned words){
    uint32_t *r;
    if(active_matrix_block)fh_platform_panic("GX command inside unfinished model matrix block");
    if(words>RING_WORDS/2)fh_platform_panic("GX state packet exceeds bounded ring");
    while(!(r=fh_gx_ring_reserve(&queue,words))){publish();fh_gxd_platform_wait();}
    return r;
}
static void fh_gxd_commit(unsigned op){
    if(fh_gx_state_filter_enabled && fh_gx_state_filter_repeat(&state_filter,op,
       queue.data+(queue.write&(queue.capacity-1)),queue.reserved)){
        /* A reservation can insert a wrap marker. Keep that valid marker and
         * monotonic write position, but publish no duplicate command. */
        if(!fh_gx_ring_cancel(&queue))fh_platform_panic("GX filtered packet reservation lost");
        ++fh_gx_state_filtered_records;return;
    }
    if(!fh_gx_ring_commit(&queue,op))fh_platform_panic("GX state packet commit");
    ++fh_gx_dispatch_records;
    if(queue.write-__atomic_load_n(&queue.published,__ATOMIC_RELAXED)>=512)publish();
}
static void record_scope_begin(const void *const addresses[4],const unsigned bytes[4],unsigned fpscr){
    uint32_t *r=fh_gxd_reserve(10);r[1]=fpscr;
    for(unsigned i=0;i<4;i++){r[2+2*i]=(uint32_t)(uintptr_t)addresses[i];r[3+2*i]=bytes[i];}
    fh_gxd_commit(OP_SCOPE_BEGIN);
}
#ifdef FH_GX_STATE_BATCH_BUILD
static void fh_gxd_flush_state(void){
    if(!fh_gxd_recording || !fh_gx_state_batch_mode || !fh_gxd_state_dirty)return;
    unsigned words=0;
    for(unsigned i=0;i<STATE_DIRTY_WORDS;i++)if(state_dirty_pages[i]){
        unsigned first=i*32,count=STATE_PAGES-first;if(count>32)count=32;
        words+=fh_state_delta_encode(state_patch+words,sizeof(state_patch)/4-words,&fh_gxp_state,
            &state_previous,first*(STATE_PAGE_BYTES/4),state_ends+first,count,state_dirty_pages[i]);
        state_dirty_pages[i]=0;
    }
    fh_gxd_state_dirty=0;
    if(fh_gx_state_batch_mode==2 && memcmp(&fh_gxp_state,&state_previous,sizeof(FhGx)))
        fh_platform_panic("GX state dirty groups omitted a write");
    if(!words)return;
    uint32_t *r=fh_gxd_reserve(words+1);memcpy(r+1,state_patch,words*4);
    fh_gxd_commit(fh_gx_state_batch_mode==2?OP_STATE_CHECK:OP_STATE_PATCH);
    ++fh_gx_state_batch_packets;fh_gx_state_batch_words+=words;
}
#endif
#define LOAD(name,type) static type fh_gxd_load_##name(const uint32_t*r){type v;memcpy(&v,r,sizeof(v));return v;}
LOAD(f32,f32) LOAD(GXColor,GXColor) LOAD(GXColorS10,GXColorS10)
#undef LOAD
static void fh_gxd_barrier(unsigned reason){
    if(fh_gx_state_filter_enabled)fh_gx_state_filter_reset(&state_filter);
    fh_gx_matrix_filter_reset(&matrix_filter);
    if(fh_gxd_recording)++fh_gx_dispatch_barrier_counts[reason];
    fh_gx_dispatch_barrier();
}
#define FH_GX_DISPATCH_RECORDERS
#include "fh_gx_dispatch_ops.h"
static uint32_t direct_matrix_block[FH_MATRIX_BLOCK_WORDS];
static void apply_matrix_block(const uint32_t *r,unsigned words){
    if(!fh_gx_matrix_block_apply(r,words,fh_gx.position,fh_gx.normal,fh_gx.texture,fh_gx.post))
        fh_platform_panic("Invalid GX model matrix block");
}
uint32_t *fh_gx_matrix_block_begin(unsigned count,unsigned flags){
    if(!fh_gx_matrix_block_enabled || !count || count>FH_MATRIX_BLOCK_MAX || flags>3)return NULL;
    if(active_matrix_block)fh_platform_panic("Nested GX model matrix block");
    unsigned words=2+count*(flags?25:13);
#ifdef FH_GX_STATE_BATCH_BUILD
    if(fh_gxd_recording && fh_gx_state_batch_mode){
        fh_gxd_prepare_state();
        if(fh_gx_state_batch_mode==1){
            active_matrix_block=direct_matrix_block;
            active_matrix_block[1]=count|(flags<<8);return active_matrix_block;
        }
    }
#endif
    if(fh_gxd_recording){
        /* The block overwrites several slots in one transaction. Invalidate
         * producer mirrors before any later individual setter can elide work. */
        fh_gx_state_filter_reset(&state_filter);fh_gx_matrix_filter_reset(&matrix_filter);
        active_matrix_block=fh_gxd_reserve(words);
    }else{
        fh_gx_dispatch_barrier();active_matrix_block=direct_matrix_block;
    }
    active_matrix_block[1]=count|(flags<<8);return active_matrix_block;
}
void fh_gx_matrix_block_end(uint32_t *r){
    if(!r)return;
    if(r!=active_matrix_block)fh_platform_panic("GX model matrix block ownership");
    unsigned count=r[1]&255,flags=r[1]>>8,words=2+count*(flags?25:13);
    ++fh_gx_matrix_blocks;fh_gx_matrix_block_matrices+=count;
    fh_gx_matrix_block_words+=words;
#ifdef FH_GX_STATE_BATCH_BUILD
    if(fh_gxd_recording && fh_gx_state_batch_mode){
        if(!fh_gx_matrix_block_apply(r,words,fh_gxp_state.position,fh_gxp_state.normal,fh_gxp_state.texture,fh_gxp_state.post))
            fh_platform_panic("Invalid producer GX model matrix block");
        for(unsigned i=0;i<count;i++){
            const uint32_t *row=r+2+i*(flags?25:13);unsigned id=row[0]&255,tex=(row[0]>>8)&255;
            FH_STATE_MARK(position[id/3]);
            if(flags&1)FH_STATE_MARK(normal[id/3]);
            if(flags&2){
                if(tex>=64)FH_STATE_MARK(post[(tex-64)/3]);
                else if(tex<30)FH_STATE_MARK(position[tex/3]);
                else FH_STATE_MARK(texture[tex/3]);
            }
        }
        if(fh_gx_state_batch_mode==1){active_matrix_block=NULL;return;}
    }
#endif
    if(fh_gxd_recording){++fh_gx_matrix_block_records;fh_gxd_commit(OP_MATRIX_BLOCK);}
    else apply_matrix_block(r,words);
    active_matrix_block=NULL;
}
void fh_gx_dispatch_matrix_range(const void *matrices,unsigned bytes){
    if(!fh_gxd_recording || fh_gx_dispatch_enabled<2)return;
    if(queue.write!=registration_end)fh_platform_panic("Late GX matrix range registration");
    matrix_source=matrices;matrix_bytes=bytes;
}
int fh_gx_dispatch_cache_write(const void *address,unsigned bytes){
    if(!fh_gxd_recording || fh_gx_dispatch_enabled<2 || !matrix_source || !address || !bytes)return 0;
    uintptr_t a=(uintptr_t)address,b=(uintptr_t)matrix_source;
    if(a<b || a-b>matrix_bytes || bytes>matrix_bytes-(a-b))return 0;
#ifdef FH_GX_STATE_BATCH_BUILD
    /* Matrix metadata writes may alias a registered source. Retire producer
     * references now, preserving every already-published child version. */
    owned_written(address,bytes);
    fh_gxd_flush_state();
#endif
    /* Replay metadata invalidation in order; no reader dereferences these
     * addresses. All GPU matrix inputs already own their copied coefficients. */
    uint32_t*r=fh_gxd_reserve(3);r[1]=(uint32_t)a;r[2]=bytes;fh_gxd_commit(OP_CACHE_WRITE);return 1;
}
void fh_gx_dispatch_lease_list(const void *list,unsigned bytes){
    if(!fh_gxd_recording || fh_gx_dispatch_enabled<2)return;
    /* Called after scope begin and before any GX commands can publish. */
    if(queue.write!=registration_end)fh_platform_panic("Late GX list lease registration");
    if(!list || !bytes || leased_count==MAX_LISTS)fh_platform_panic("Invalid GX list lease");
    leased_lists[leased_count++]=(typeof(leased_lists[0])){list,bytes};
}
#ifdef FH_GX_STATE_BATCH_BUILD
#include "gx_owned_dispatch.h"
#endif
void GXCallDisplayList(const void *list,u32 bytes){
    if(fh_gxd_recording && fh_gx_dispatch_enabled>=2){
        for(unsigned i=0;i<leased_count;i++)if(leased_lists[i].data==list && leased_lists[i].bytes==bytes){
#ifdef FH_GX_STATE_BATCH_BUILD
            if(fh_gx_owned_sources_mode)fh_gxd_prepare_state();
            fh_gxd_flush_state();
            int owned=owned_capture(list,bytes);
            if(owned>=0){uint32_t *r=fh_gxd_reserve(2);r[1]=(unsigned)owned;
                fh_gxd_commit(OP_OWNED_LIST);++owned_since_join;++fh_gx_dispatch_draws;return;}
#endif
            uint32_t*r=fh_gxd_reserve(3);r[1]=(uint32_t)(uintptr_t)list;r[2]=bytes;
            fh_gxd_commit(OP_LEASED_LIST);borrowed_since_join=1;++fh_gx_dispatch_draws;return;
        }
    }
    fh_gxd_barrier(62);++fh_gx_dispatch_direct_draws;fh_gxd_GXCallDisplayList(list,bytes);
}
int fh_gxd_pending(void){return __atomic_load_n(&queue.read,__ATOMIC_ACQUIRE)!=__atomic_load_n(&queue.published,__ATOMIC_ACQUIRE);}
static void consume_record(const uint32_t *r,unsigned words,unsigned op){
    if(op==OP_SCOPE_BEGIN){
        if(words!=10 || reader_scope_open)fh_platform_panic("Invalid GX reader scope begin");
        const void *addresses[4];unsigned sizes[4];
        for(unsigned i=0;i<4;i++){addresses[i]=(const void*)(uintptr_t)r[2+2*i];sizes[i]=r[3+2*i];}
        if(r[1]&~0x03c00000u)fh_platform_panic("Invalid GX reader floating-point control");
#ifdef __arm__
        unsigned fpscr;__asm__ volatile("vmrs %0, fpscr":"=r"(fpscr));
        fpscr=(fpscr&~0x03c00000u)|r[1];__asm__ volatile("vmsr fpscr, %0"::"r"(fpscr));
#endif
        fh_retained_reader_scope_begin(addresses,sizes);reader_scope_open=1;return;
    }
    if(op==OP_SCOPE_END){
        if(words!=1 || !reader_scope_open)fh_platform_panic("Invalid GX reader scope end");
        fh_retained_reader_scope_end();reader_scope_open=0;return;
    }
    if(!reader_scope_open)fh_platform_panic("GX packet outside reader scope");
    if(op==OP_OWNED_LIST){
#ifdef FH_GX_STATE_BATCH_BUILD
        if(words!=2)fh_platform_panic("Invalid owned GX packet");owned_consume(r[1]);
#else
        fh_platform_panic("Owned GX packet unavailable");
#endif
    }else if(op==OP_STATE_PATCH || op==OP_STATE_CHECK){
#ifdef FH_GX_STATE_BATCH_BUILD
        if(words<2 || !fh_state_delta_apply(&fh_gx,sizeof(FhGx)/4,r+1,words-1,op==OP_STATE_CHECK))
            fh_platform_panic("GX state patch differs from checked setters or exceeds state");
        fh_gx_state_batch_checks+=op==OP_STATE_CHECK;
#else
        fh_platform_panic("GX state patch unavailable");
#endif
    }else if(op==OP_MATRIX_BLOCK){
        apply_matrix_block(r,words);
    }else if(op==OP_CACHE_WRITE){
        if(words!=3)fh_platform_panic("Invalid GX cache metadata packet");
        const void *address=(const void*)(uintptr_t)r[1];
        fh_retained_scope_invalidate(address,r[2]);fh_renderer_invalidate(address,r[2]);
    }else if(op==OP_LEASED_LIST){
#ifdef FH_GX_STATE_BATCH_BUILD
        memset(owned_seen_generation,0,sizeof(owned_seen_generation));
#endif
        if(words!=3)fh_platform_panic("Invalid GX leased draw packet");
        /* The callback cannot change these arrays between fur passes. Check
         * every indexed input, including inactive-looking attributes, before
         * handing the renderer ownership. Matrices are already packet copies. */
        for(unsigned a=GX_VA_POS;a<=GX_VA_TEX7;a++)if(fh_gx.descriptors[a]>=GX_INDEX8){
            FhVtxArray *array=fh_gx.arrays+a;
            if(!fh_retained_scope_contains(array->data,array->bytes))fh_platform_panic("GX draw input outside immutable fur lease");
        }
        fh_gxd_GXCallDisplayList((const void*)(uintptr_t)r[1],r[2]);
    }else if(!fh_gxd_replay(op,r,words))fh_platform_panic("Invalid GX state packet");
}
int fh_gxd_consume(void){
    const uint32_t *r;unsigned words,op;int result=fh_gx_ring_peek(&queue,&r,&words,&op);
    if(result==FH_GX_RING_EMPTY)return 0;
    if(result==FH_GX_RING_BAD)fh_platform_panic("Invalid GX state packet");
    consume_record(r,words,op);
    if(!fh_gx_ring_release(&queue,r,words))fh_platform_panic("GX state packet retirement");
    return 1;
}
int fh_gxd_consume_batch(unsigned limit){
    if(!limit || limit>64)fh_platform_panic("Invalid GX worker slice limit");
    unsigned consumed=0;
    if(!fh_gx_dispatch_batch_enabled){while(consumed<limit && fh_gxd_consume())++consumed;return consumed;}
    FhGxRingBatch batch;
    if(!fh_gx_ring_batch_begin(&queue,&batch))fh_platform_panic("Invalid GX batch publication");
    while(consumed<limit){
        const uint32_t *r;unsigned words,op;
        int result=fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op);
        if(result==FH_GX_RING_EMPTY)break;
        if(result==FH_GX_RING_BAD)fh_platform_panic("Invalid GX state batch packet");
        consume_record(r,words,op);
        if(!fh_gx_ring_batch_release(&queue,&batch,r,words))fh_platform_panic("GX batch packet retirement");
        ++consumed;
    }
    if(consumed){++fh_gx_dispatch_batches;fh_gx_dispatch_batch_records+=consumed;}
    if(!fh_gx_ring_batch_finish(&queue,&batch))fh_platform_panic("GX batch retirement");
    return consumed;
}
void fh_gx_dispatch_barrier(void){
    if(active_matrix_block)fh_platform_panic("GX barrier inside unfinished model matrix block");
    if(!initialized)return;
#ifdef FH_GX_STATE_BATCH_BUILD
    fh_gxd_flush_state();
#endif
    /* Melee's acknowledged-prefix fast path: an idle producer has no reader
     * to wake or retire. Acquire the CPU completion cursor before returning.
     * Open/deferred scopes and allocated copied-source pools still take the
     * full path, including all source retirement and state resynchronization. */
    if(!fh_gxd_recording && !deferred_end &&
#ifdef FH_GX_STATE_BATCH_BUILD
       !owned_pool.storage &&
#endif
       queue.write==__atomic_load_n(&queue.read,__ATOMIC_ACQUIRE))return;
    if(fh_gxd_recording)++fh_gx_dispatch_barrier_counts[63];
    publish();
    if(fh_gxd_pending()){++fh_gx_dispatch_waits;fh_gxd_platform_wait();}
    if(deferred_end){++fh_gx_owned_deferred_joins;deferred_end=0;}
    borrowed_since_join=owned_since_join=0;
#ifdef FH_GX_STATE_BATCH_BUILD
    owned_retire();
    owned_model_end();
    memset(owned_seen_generation,0,sizeof(owned_seen_generation));
    if(fh_gxd_recording && fh_gx_state_batch_mode)state_stale=1;
#endif
}
void fh_gx_dispatch_join_deferred(void){if(deferred_end)fh_gx_dispatch_barrier();}
int fh_gx_dispatch_begin(const void *const addresses[4],const unsigned bytes[4]){
    if(!fh_gx_dispatch_enabled || fh_render_floor)return 0;
    if(fh_gxd_recording)fh_platform_panic("Nested GX state command scope");
    /* Until renderer initialization and state resynchronization themselves
     * become ordered jobs, a new scope must join any prior owned scope. */
    fh_gx_dispatch_join_deferred();
    if(fh_gx_dispatch_enabled>=2){
        /* Initialize/reset frame counters on the producer before audio can
         * be serviced there concurrently with draw translation. */
        extern void fh_renderer_dispatch_begin(void);fh_renderer_dispatch_begin();
    }
    if(!initialized){
        if(!fh_gx_ring_init(&queue,storage,RING_WORDS))fh_platform_panic("GX state ring initialization");
        queue.cache_reads=fh_gx_dispatch_batch_enabled!=0;
        if(!fh_gxd_platform_start()){fh_gx_dispatch_enabled=0;return 0;}
        initialized=1;
    }
    leased_count=0;
    fh_gx_state_filter_reset(&state_filter);
    fh_gx_matrix_filter_reset(&matrix_filter);
    matrix_source=NULL;matrix_bytes=0;
#ifdef FH_GX_STATE_BATCH_BUILD
    if(fh_gx_state_batch_mode){state_stale=1;fh_gxd_prepare_state();}
    owned_begin();
#endif
    unsigned scope_fpscr=0;
#ifdef __arm__
    __asm__ volatile("vmrs %0, fpscr":"=r"(scope_fpscr));scope_fpscr&=0x03c00000u;
#endif
    /* The consumer sees the scope's ranges and FP control in queue order.
     * No later producer begin/end can replace metadata for an earlier draw.
     * The marker owns descriptors only: borrowed byte lifetimes still end at
     * the existing CPU join, independently of GPU storage fences. */
    record_scope_begin(addresses,bytes,scope_fpscr);registration_end=queue.write;
    fh_gxd_recording=1;++fh_gx_dispatch_scopes;return 1;
}
void fh_gx_dispatch_end(void){
    if(fh_gxd_recording){
#ifdef FH_GX_STATE_BATCH_BUILD
        fh_gxd_flush_state();
#endif
        (void)fh_gxd_reserve(1);fh_gxd_commit(OP_SCOPE_END);
#ifdef FH_GX_STATE_BATCH_BUILD
        if(fh_gx_owned_sources_mode==2 && !borrowed_since_join && owned_since_join){
            /* Children own immutable copies independently of the producer's
             * model/texture references. Publishing the end marker does not
             * retire their slots or any GPU resource. Original bytes can now
             * change while this reader completes; direct GX access still
             * joins through the existing barrier before using reader state. */
            publish();owned_model_end();fh_gxd_recording=0;deferred_end=1;
            ++fh_gx_owned_deferred_scopes;
            fh_gx_owned_deferred_pending+=fh_gxd_pending()!=0;return;
        }
#endif
    }
    fh_gx_dispatch_barrier();fh_gxd_recording=0;
}
#else
uint32_t *fh_gx_matrix_block_begin(unsigned count,unsigned flags){(void)count;(void)flags;return NULL;}
void fh_gx_matrix_block_end(uint32_t *r){(void)r;}
int fh_gx_dispatch_begin(const void *const addresses[4],const unsigned bytes[4]){(void)addresses;(void)bytes;return 0;}
void fh_gx_dispatch_end(void){}
void fh_gx_dispatch_barrier(void){}
void fh_gx_dispatch_join_deferred(void){}
void fh_gx_dispatch_lease_list(const void *list,unsigned bytes){(void)list;(void)bytes;}
void fh_gx_dispatch_matrix_range(const void *matrices,unsigned bytes){(void)matrices;(void)bytes;}
int fh_gx_dispatch_cache_write(const void *address,unsigned bytes){(void)address;(void)bytes;return 0;}
#endif
