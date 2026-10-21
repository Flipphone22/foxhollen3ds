#include "fh_asset_paths.h"
#include <3ds.h>
#include "fh_draw_capture.h"
#include "fh_indirect_lighting.h"
#include <citro3d.h>
#include "fh_early_queue.h"
#include "../vendor/citro3d/internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "fh_memory_bytes.h"
#include "fh_uniform_rows.h"
#include "fh_hash_bytes.h"
#include "fh_alpha_cache.h"
#include "fh_graphics.h"
#include "fh_minimal_graphics.h"
#include "fh_primitive.h"
#include "fh_gpu_transfer.h"
#include "fh_gpu_lighting.h"
#include "fh_gpu_texgen.h"
#include "fh_retained_geometry.h"
#include "fh_gx_dispatch.h"
#include "fh_compiled_geometry.h"
#include "fh_material.h"
#include "fh_material_scope.h"
#include "fh_source_identity.h"
#include "fh_texture_dependencies.h"
#include "fh_texture_pack.h"
#include "fh_fragment.h"
#include "fh_fur_gpu.h"
#include "fh_fragment_raster.h"
#include "fh_fog.h"
#include "fh_vertex_reuse.h"
#include "fh_material_register.h"
#include "fh_platform.h"
#include "fh_diagnostic_buffer.h"
#include "game_shader.h"
#include "sphere_shader.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include "../vendor/melee/alpha_test.h"
#include "../vendor/melee/scissor.h"
#include "../vendor/melee/blend_state.h"
#pragma GCC diagnostic pop

enum { VERTICES=32768,TEXTURES=256,TEXTURE_BUDGET=12*1024*1024,TEXTURE_CACHE_LIMIT=10*1024*1024,PLANS=128,
       MATERIAL_STATE_BYTES=offsetof(FhDrawState,constant_raster_mask)-offsetof(FhDrawState,tev),
       MATERIAL_KEY_BYTES=MATERIAL_STATE_BYTES+36+224+4 };
_Static_assert(TEXTURE_CACHE_LIMIT*100<=TEXTURE_BUDGET*85,"Texture storage reserve must remain at least 15 percent");
typedef FhNativeVertex GpuVertex;
_Static_assert(sizeof(GpuVertex)==88 && sizeof(FhVertex)==148,"GPU vertex/staging layout");
typedef struct { C3D_Tex gpu;FhTexture source,secondary;unsigned frame,bytes,valid,mode,pinned,hash; } Texture;
typedef struct { unsigned hash,valid,key_bytes,fallback_only,stamp;unsigned char key[MATERIAL_KEY_BYTES];FhMaterial plan; } Material;
static Texture textures[TEXTURES];
static FhTexturePack prepared_textures;
static unsigned prepared_textures_checked;
unsigned fh_light_lod_enabled,fh_light_lod_draws,fh_light_lod_source_lights,fh_light_lod_palette_draws;
static int prepared_texture_find(const FhTexture *source,unsigned mode,FhPackedTexture *entry){
    if(mode || source->format!=14)return 0;
    if(!prepared_textures_checked){
        prepared_textures_checked=1;
        extern u32 __ctru_heap_size,__stacksize__;
        uint64_t used=(unsigned)mallinfo().uordblks;
        uint64_t limit=(uint64_t)__ctru_heap_size*85/100;
        uint64_t committed=used+__stacksize__+8192; /* FILE plus allocator headroom */
        unsigned budget=limit>committed?(unsigned)(limit-committed):0;
        if(budget>128*1024)budget=128*1024;
        if(fh_texture_pack_open(&prepared_textures,fh_asset_path(FH_ASSET_TEXTURE),budget))
            fh_platform_log("Prepared compressed textures enabled; exact source/alpha, reduced RGB detail\n");
    }
    return fh_texture_pack_find(&prepared_textures,source->pixels,source->width,source->height,entry);
}
static void efb_color_init(void);
static void efb_color_finish(void);
static Texture *efb_color_lookup(const FhTexture *source,unsigned mode);
static void efb_color_invalidate(const void *address,size_t bytes);
static void efb_shadow_init(void);
static void efb_shadow_finish(void);
static Texture *efb_shadow_lookup(const FhTexture *source,unsigned mode);
static void efb_shadow_materialize(const void *pixels);
static void efb_shadow_invalidate(const void *address,size_t bytes);
static void efb_depth_init(void);
static void efb_depth_finish(void);
static void efb_depth_materialize(const void *pixels);
static void efb_depth_invalidate(const void *address,size_t bytes);
static Texture *efb_depth_lookup(const FhTexture *source,unsigned mode);
static unsigned short texture_recent[256];
static Texture fragment_atlas;
static unsigned atlas_x,atlas_y,atlas_row,fragment_pixels,fragment_triangles;
static Material materials[PLANS];
static Material *last_material;
static unsigned material_clock;
static C3D_Tex bound_gpu[3];
static unsigned bound_used[3];
static unsigned pending_start,pending_count;
static unsigned transform_valid,transform_enabled;
static float transform_matrix[4][4];
static float transform_models[30][4];
static unsigned affine_valid,affine_enabled;
static unsigned lighting_valid,lighting_enabled,prepared_light_count;
static unsigned gpu_lit_vertices,gpu_light_evaluations,gpu_lighting_fallback_vertices,lighting_uniform_vectors;
static unsigned gpu_normal_vertices,gpu_texgen_vertices;
static FhGpuLighting prepared_lighting,uploaded_lighting;
static float prepared_texgen_post[3][4],uploaded_texgen_post[3][4];
static unsigned texgen_valid,texgen_enabled;
static unsigned prepared_texgen_units,uploaded_texgen_units;
enum { NATIVE_GEOMETRY_SLOTS=512,NATIVE_GEOMETRY_BYTES=FH_NATIVE_GEOMETRY_BUDGET,LINEAR_RESERVE=4*1024*1024 };
typedef struct { unsigned raw,normal_mode,width[3],height[3],points,packing;
    unsigned fur_frame,fur;FhTexture fur_textures[2];unsigned fur_order[2][2],fur_scale[2][2];
    unsigned fur_post_valid;float fur_post[3][4];
    int coord[3],primary[4];
    /* Only scalar CPU color packing consumes the full material. Common GPU
     * packing keys end here, so they neither copy nor compare unused TEV state. */
    FhMaterial scalar;
} NativeGeometryKey;
typedef struct {
    uint64_t id;void *storage;NativeGeometryKey *key;unsigned key_bytes;
    unsigned bytes,vertex_offset,index_offset,count,unique,epoch,stamp;
} NativeGeometry;
unsigned fh_native_geometry_key_bytes;
static NativeGeometry native_geometry[NATIVE_GEOMETRY_SLOTS],*native_replay;
static uint64_t native_record_id;
static const FhVertex *native_record_input;
static const FhDrawState *native_record_state;
static const FhCompiledMesh *compiled_mesh;
static FhCompiledDecode compiled_decode;
static unsigned compiled_sphere,compiled_failed,compiled_staged,compiled_native;
static unsigned compiled_compact;
#ifdef FH_COMPACT_VERTEX_BUILD
static FhCompactPacking compact_fixed;
#endif
unsigned fh_compact_packing_checks,fh_compact_draws;
unsigned fh_native_packing_checks;
unsigned fh_indexed_geometry_disabled,fh_indexed_geometry_draws,fh_indexed_geometry_replays;
/* CPU-only staging shared by ordinary indexed meshes and fur. Fur keeps its
 * existing limit; only the ordinary vertex portion grows. Admission below
 * still reserves 15% normal heap, and GPU/CPU cache arenas are unchanged. */
enum { COMPILED_FUR_UNIQUE=1024,COMPILED_FUR_PADDED=1026 };
typedef struct {FhVertex vertices[FH_COMPILED_RETAINED_UNIQUE];float uv[COMPILED_FUR_PADDED][3];} CompiledFur;
static CompiledFur *compiled_fur_storage;
static unsigned compiled_fur;
static unsigned compiled_fur_gpu;
static float compiled_fur_strength,uploaded_fur_strength;
static unsigned fur_strength_valid;
static const FhDrawState *compiled_fur_source;
static const float (*compiled_fur_post)[4];
/* identity, key, resident draw, decode, coefficients, admission, queue setup,
 * new draw, packing, retention, entry, textures, fog/combiner, raster, transform.
 * Several spans overlap; these are attribution counters, not additive costs. */
u64 fh_fur_path_ticks[15];
/* Separate preparation/submission cost by backend, rather than treating all
 * display-list processing as vertex decoding. Six disjoint spans within a
 * direct draw; parent layered calls do not accumulate their child spans. */
u64 fh_backend_ticks[3][6];
unsigned fh_backend_calls[3];
unsigned fh_render_attribution_active;
unsigned fh_gpu_fur_disabled,fh_gpu_fur_draws,fh_gpu_fur_replays;
unsigned fh_fur_coefficients_mode,fh_fur_coefficients_checks;
#include "fh_movie_disjoint.h"
unsigned fh_movie_batch_enabled,fh_movie_batches,fh_movie_batch_triangles;
unsigned fh_movie_disjoint_batches,fh_movie_disjoint_triangles,fh_movie_disjoint_rejections,fh_movie_disjoint_groups;
unsigned fh_ordered_program_chunks,fh_ordered_program_triangles;
unsigned fh_compiled_fur_draws,fh_compiled_fur_unique,fh_compiled_fur_bytes;
static unsigned native_record_count;
static unsigned native_geometry_bytes,native_geometry_epoch=1,native_geometry_clock;
static unsigned native_geometry_hits,native_geometry_vertices;
static void native_geometry_trim(unsigned required);
static NativeGeometry *native_geometry_oldest(void);
static void native_geometry_retire(NativeGeometry *entry);
unsigned fh_texture_allocation_retries,fh_texture_geometry_evictions,fh_texture_cache_evictions;
static float affine_coefficients[4][9];
static unsigned affine_scales[2];
static GpuVertex *vertices;
static u16 *indices;
static unsigned index_count;
static unsigned early_queue_enabled=
#ifdef FH_EMULATOR_TEST
    0;
#else
    1;
#endif
static unsigned early_queue_offset,early_queue_full,early_frame_starts;
static u64 queue_join_ticks;
#ifndef FH_EARLY_QUEUE_BYTES
#define FH_EARLY_QUEUE_BYTES 16384
#endif
/* Melee renderer.c::flush_dynamic: only appended bytes become dirty. Reset
 * after the queue is joined, never merely when commands are submitted. */
static unsigned flushed_vertex_bytes,flushed_index_bytes;
static unsigned stream_flush_bytes,stream_flush_full_bytes,stream_flush_calls;
static u64 stream_flush_ticks;
#ifdef FH_EMULATOR_TEST
static unsigned stream_full_flush;
#endif
static struct { const FhVertex *vertices;const uint32_t *identities;unsigned count; } identified;
unsigned fh_vertex_unique,fh_vertex_reused;
static unsigned frame_unique_base,frame_reused_base;
static C3D_RenderTarget *target;
static C3D_RenderTarget *display_target;
static DVLB_s *shader,*sphere_shader_binary;
static shaderProgram_s program,point_program,sphere_program;
static unsigned sphere_program_wanted,sphere_program_active;
#ifdef FH_EMULATOR_TEST
unsigned fh_gpu_sphere_disabled=1;
#else
unsigned fh_gpu_sphere_disabled;
#endif
unsigned fh_gpu_sphere_draws;
static struct { unsigned active,width,offsets;float texture_offset; } point_context;
static unsigned point_program_active;
static float point_parameters[4];
unsigned fh_gpu_points_disabled,fh_gpu_point_vertices;
static unsigned initialized,open_frame,frame_number,vertex_count,draws,texture_bytes,black,clear_color=255,clear_depth=0xffffff;
static unsigned total_vertices,barriers;
static unsigned command_peak;
unsigned fh_gpu_boot_trace; /* Opt-in verbose startup diagnostics. */
static void boot_queue_trace(void) {
    gxCmdQueue_s *q=&C3Di_GetContext()->gxQueue;
    char line[256];
    snprintf(line,sizeof(line),"boot GPU queue frame=%u entries=%u submitted=%u completed=%u\n",frame_number,q->numEntries,q->curEntry,q->lastEntry);
    fh_platform_log(line);
    for(unsigned i=0;i<q->numEntries;i++) {
        const u32 *v=q->entries[i].data;
        snprintf(line,sizeof(line),"boot GPU command %u: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",i,
                 (unsigned long)v[0],(unsigned long)v[1],(unsigned long)v[2],(unsigned long)v[3],
                 (unsigned long)v[4],(unsigned long)v[5],(unsigned long)v[6],(unsigned long)v[7]);
        fh_platform_log(line);
    }
}
static void boot_gpu_trace(const char *stage) {
    if(!fh_gpu_boot_trace || frame_number>=32)return;
    char line[192];
    snprintf(line,sizeof(line),"boot GPU %s frame=%u draws=%u vertices=%u indices=%u open=%u ticks=%llu\n",
             stage,frame_number,draws,vertex_count,index_count,open_frame,(unsigned long long)svcGetSystemTick());
    fh_platform_log(line);
}
static void bounded_frame_begin(void) {
    u64 started=svcGetSystemTick();
    boot_gpu_trace("wait-begin");
    while(!C3D_FrameBegin(C3D_FRAME_NONBLOCK)) {
        if(svcGetSystemTick()-started>5ULL*SYSCLOCK_ARM11) {
            boot_queue_trace();
            fh_platform_panic("GPU command queue did not complete within five seconds");
        }
        svcSleepThread(1000000);
    }
    queue_join_ticks+=svcGetSystemTick()-started;
    fh_early_queue_retire();early_queue_offset=early_queue_full=0;
    boot_gpu_trace("wait-complete");
}
static u32 *readback;
static unsigned readback_stride,readback_views;
static uint64_t readback_compact_bytes_avoided;
#ifdef FH_EMULATOR_TEST
static unsigned readback_compact_reference;
#endif
/* A 512-column backing has the same tile pitch as the sampled capture.
 * The visible/logic viewport remains 264x400 (rotated 400x264). */
static unsigned efb_storage_width=FH_EFB_NATIVE_HEIGHT;
static unsigned efb_storage_height=400,efb_direct_view,efb_view_active,efb_sampled_views;
static unsigned efb_ordered_view,efb_view_waits_avoided;
static C3D_Tex efb_target_texture;
static unsigned efb_storage_bytes=400*FH_EFB_NATIVE_HEIGHT*4;
static unsigned readback_pending;
static unsigned capture_dma_rows,capture_dma_contiguous;
static void *sampled_capture_storage;
static size_t sampled_capture_bytes;
static void *depth_snapshot_storage;
static size_t depth_snapshot_bytes;
/* Isolation mode waits at each producer/copy boundary. Ordered mode keeps
 * draws, DMA rows and their GPU consumers in the same libctru queue. CPU
 * readback, resource reuse and capacity retirement retain their own joins. */
static unsigned gpu_copy_fences=1,gpu_copy_fence_count;
static void gpu_copy_wait(const char *phase,unsigned serial) {
    gxCmdQueue_s *q=&C3Di_GetContext()->gxQueue;
    u64 started=svcGetSystemTick();
    gxCmdQueueRun(q);
    while(!gxCmdQueueWait(q,0)) {
        if(svcGetSystemTick()-started>5ULL*SYSCLOCK_ARM11) {
            char message[128];
            snprintf(message,sizeof(message),"GPU copy %u %s timeout",serial,phase);
            boot_queue_trace();fh_platform_panic(message);
        }
        svcSleepThread(100000);
    }
    queue_join_ticks+=svcGetSystemTick()-started;
}
const char *fh_renderer_texture_copy_error(uintptr_t source,size_t source_bytes,unsigned input,
        uintptr_t destination,size_t destination_bytes,unsigned output,unsigned bytes,unsigned flags) {
    if(destination>=0x1f000000 && destination<0x1f600000)
        return fh_gpu_texture_copy_error(source,source_bytes,input,destination,destination_bytes,output,bytes,flags);
    return fh_gpu_sampled_copy_error(source,source_bytes,input,destination,destination_bytes,output,bytes,flags,
        (uintptr_t)sampled_capture_storage,sampled_capture_bytes);
}
const char *fh_renderer_texture_copy_submission_error(uintptr_t source,size_t source_bytes,unsigned input,
        uintptr_t destination,size_t destination_bytes,unsigned output,unsigned bytes,unsigned flags){
    const char *error=fh_renderer_texture_copy_error(source,source_bytes,input,destination,destination_bytes,output,bytes,flags);
    if(error)return error;
    if(output>>16)return "Regression: output-gap GPU transfer must be split into contiguous rows";
    if(destination<0x1f000000 || destination>=0x1f600000)
        return "Regression: sampled linear capture must use DMA, not raw TextureCopy";
    if(depth_snapshot_storage && destination>=(uintptr_t)depth_snapshot_storage &&
       destination-(uintptr_t)depth_snapshot_storage<depth_snapshot_bytes)
        return "Regression: depth snapshot must use DMA, not raw TextureCopy";
    return NULL;
}
const char *fh_renderer_capture_dma_error(uintptr_t source,uintptr_t destination,unsigned bytes){
    if(depth_snapshot_storage && destination>=(uintptr_t)depth_snapshot_storage &&
       destination-(uintptr_t)depth_snapshot_storage<depth_snapshot_bytes)
        return fh_gpu_sampled_copy_error(source,bytes,0,destination,bytes,0,bytes,FH_GPU_COPY_CONTIGUOUS,
            (uintptr_t)depth_snapshot_storage,depth_snapshot_bytes);
    return fh_gpu_sampled_copy_error(source,bytes,0,destination,bytes,0,bytes,FH_GPU_COPY_CONTIGUOUS,
        (uintptr_t)sampled_capture_storage,sampled_capture_bytes);
}
static void copy_owned_gpu(u32 *source,size_t source_bytes,unsigned input,u32 *destination,
        size_t destination_bytes,unsigned output,unsigned bytes,unsigned flags) {
    const char *error=fh_renderer_texture_copy_error((uintptr_t)source,source_bytes,input,
        (uintptr_t)destination,destination_bytes,output,bytes,flags);
    if(error)fh_platform_panic(error);
    unsigned serial=++gpu_copy_fence_count;
    if(!gpu_copy_fences){
        /* Split emits the previous draw's framebuffer flush/invalidate before
         * the DMA/PPF command. Queue order, not CPU waiting, owns the source.
         * Bound the SDK's capacity drain before it can wait indefinitely. */
        gxCmdQueue_s *q=&C3Di_GetContext()->gxQueue;
        if(q->numEntries+4>=q->maxEntries)gpu_copy_wait("copy capacity",serial);
        C3D_FrameSplit(GX_CMDLIST_FLUSH);
    }
    if(gpu_copy_fences) {
        C3D_FrameSplit(GX_CMDLIST_FLUSH);
        gpu_copy_wait("producer",serial);
        if(serial<=12) {
            char line[192];
            snprintf(line,sizeof(line),"GPU isolated copy %u frame=%u source=%08lx destination=%08lx bytes=%u dims=%08x/%08x flags=%u\n",
                serial,frame_number,(unsigned long)source,(unsigned long)destination,bytes,input,output,flags);
            fh_platform_log(line);
        }
    }
    if(output>>16){
#ifdef FH_LEGACY_OUTPUT_GAP
#ifndef FH_EMULATOR_TEST
#error Legacy output-gap command is a test-only mutation
#endif
        C3D_SyncTextureCopy(source,input,destination,output,bytes,flags);
        return;
#endif
        unsigned rows=fh_gpu_copy_rows(input,output,bytes);
        if(!rows)fh_platform_panic("Unsupported padded GPU copy layout");
        for(unsigned i=0;i<rows;i++){
            FhGpuCopyRow row=fh_gpu_copy_row(output,i);
            u32 *from=(u32*)((u8*)source+row.source_offset),*to=(u32*)((u8*)destination+row.destination_offset);
            /* Explicit nonzero line widths also avoid relying on emulators'
             * implicit size substitution for a zero-width contiguous copy. */
            unsigned dimensions=GX_BUFFER_DIM(row.bytes/16,0);
            error=fh_renderer_texture_copy_error((uintptr_t)from,source_bytes-row.source_offset,dimensions,
                (uintptr_t)to,destination_bytes-row.destination_offset,dimensions,row.bytes,FH_GPU_COPY_CONTIGUOUS);
            if(error)fh_platform_panic(error);
            if((uintptr_t)to<0x1f000000 || (uintptr_t)to>=0x1f600000){
                error=fh_renderer_capture_dma_error((uintptr_t)from,(uintptr_t)to,row.bytes);
                if(error)fh_platform_panic(error);
#ifdef FH_LEGACY_LINEAR_RAW_COPY
#ifndef FH_EMULATOR_TEST
#error Legacy raw capture is a test-only mutation
#endif
                C3D_SyncTextureCopy(from,dimensions,to,dimensions,row.bytes,FH_GPU_COPY_CONTIGUOUS);
#else
                /* GSP DMA copies the same tiled bytes without the PPF raw-copy
                 * engine that stalled on hardware. The producer precedes the
                 * DMA in this queue; capture has no CPU readers/writers after
                 * its initial flush. */
                /* Match SyncTextureCopy's split: the pinned queue adapter also
                 * reserves capacity here, including when there are no new draws. */
                /* The Melee-derived queue adapter reserves four tail entries.
                 * Bound its capacity wait before FrameSplit joins callbacks
                 * and recycles the queue. Rows have disjoint destinations and
                 * share the preceding draw's image in command order. */
                gxCmdQueue_s *q=&C3Di_GetContext()->gxQueue;
                if(q->numEntries+4>=q->maxEntries)gpu_copy_wait("row capacity",serial);
                C3D_FrameSplit(GX_CMDLIST_FLUSH);
                if(R_FAILED(GX_RequestDma(from,to,row.bytes)))fh_platform_panic("Capture DMA submission failed");
                capture_dma_rows++;
#endif
            }else C3D_SyncTextureCopy(from,dimensions,to,dimensions,row.bytes,FH_GPU_COPY_CONTIGUOUS);
        }
        if(gpu_copy_fences)gpu_copy_wait("row transfer",serial);
    }else if(destination==sampled_capture_storage){
        /* Same registered GPU-owned allocation and DMA engine as the row
         * route; matching tile pitches need just one command. Never use raw
         * TextureCopy for this linear destination. */
        if(input || output)fh_platform_panic("Contiguous capture DMA layout");
        error=fh_renderer_capture_dma_error((uintptr_t)source,(uintptr_t)destination,bytes);
        if(error)fh_platform_panic(error);
        C3D_FrameSplit(GX_CMDLIST_FLUSH);
        if(R_FAILED(GX_RequestDma(source,destination,bytes)))fh_platform_panic("Contiguous capture DMA submission failed");
        capture_dma_contiguous++;
        if(gpu_copy_fences)gpu_copy_wait("capture DMA",serial);
    }else if(destination==depth_snapshot_storage){
        if(input || output)fh_platform_panic("Depth snapshot DMA must be contiguous");
        error=fh_renderer_capture_dma_error((uintptr_t)source,(uintptr_t)destination,bytes);
        if(error)fh_platform_panic(error);
#ifdef FH_LEGACY_DEPTH_SNAPSHOT
#ifndef FH_EMULATOR_TEST
#error Legacy depth snapshot is a test-only mutation
#endif
        C3D_SyncTextureCopy(source,input,destination,output,bytes,flags);
#else
        C3D_FrameSplit(GX_CMDLIST_FLUSH);
        if(R_FAILED(GX_RequestDma(source,destination,bytes)))fh_platform_panic("Depth snapshot DMA submission failed");
#endif
        if(gpu_copy_fences)gpu_copy_wait("depth DMA",serial);
    }else{
        C3D_SyncTextureCopy(source,input,destination,output,bytes,flags);
        if(gpu_copy_fences)gpu_copy_wait("transfer",serial);
    }
}
static unsigned depth_snapshot_valid,depth_snapshot_draws;
unsigned fh_depth_snapshot_reads,fh_depth_snapshot_reuses;
static void queue_readback(u32 *source,unsigned width,unsigned height) {
    depth_snapshot_valid=0;
    if(width!=FH_EFB_NATIVE_HEIGHT || height!=400)fh_platform_panic("Unexpected EFB readback dimensions");
    width=efb_storage_width;
    height=400;
    size_t bytes=(size_t)width*height*4;
    const char *error=fh_gpu_readback_error((uintptr_t)source,bytes,(uintptr_t)readback,
                         efb_storage_bytes,width,height,0);
    if(error)fh_platform_panic(error);
    if(R_FAILED(GX_DisplayTransfer(source,GX_BUFFER_DIM(width,height),readback,GX_BUFFER_DIM(width,height),0)))
        fh_platform_panic("Framebuffer readback submission failed");
    readback_pending=1;
}
static void acquire_readback(void){
    if(!readback_pending)return;
    if(R_FAILED(GSPGPU_InvalidateDataCache(readback,efb_storage_bytes)))fh_platform_panic("EFB readback cache invalidation failed");
    /* The completed DisplayTransfer owns these bytes. Converters read its
     * physical row pitch directly, so ARM never dirties the readback buffer.
     * No compaction or CPU-to-GPU flush is needed before its next GPU write. */
    readback_stride=efb_storage_width;
#ifdef FH_EMULATOR_TEST
    if(readback_compact_reference && efb_storage_width!=FH_EFB_NATIVE_HEIGHT){
        for(unsigned y=1;y<400;y++)memmove(readback+y*FH_EFB_NATIVE_HEIGHT,
            readback+y*efb_storage_width,FH_EFB_NATIVE_HEIGHT*4);
        if(R_FAILED(GSPGPU_FlushDataCache(readback,400*FH_EFB_NATIVE_HEIGHT*4)))fh_platform_panic("Compacted EFB reference visibility");
        readback_stride=FH_EFB_NATIVE_HEIGHT;
    }
#endif
    readback_views++;
    if(readback_stride!=FH_EFB_NATIVE_HEIGHT)readback_compact_bytes_avoided+=399u*FH_EFB_NATIVE_HEIGHT*4;
    readback_pending=0;
}
static FILE *timings;
static FILE *profile;
static FILE *attribution;
static unsigned attribution_period;
/* Like pinned Melee's native_detail_begin, detailed clocks must not execute
 * on every production draw. Sample complete frames so component spans keep
 * their existing meaning. FPS/workload and timeout/audio clocks stay live. */
static unsigned frame_profile_active;
static inline u64 component_tick(void) {
#ifdef FH_EMULATOR_TEST
    return svcGetSystemTick();
#else
    return frame_profile_active?svcGetSystemTick():0;
#endif
}
extern u64 fh_gx_phase_ticks[4],fh_gx_shading_ticks[3];
static struct {
    u64 gx[4],shading[3],backend[3][6];
    unsigned calls[3];
} attribution_previous;
/* Called only after the GX producer's presentation fence has retired CPU
 * readers. Sampling changes at that boundary, never while a draw is running.
 * Keep raw ticks: formatting/division happens outside the measured spans. */
static void attribution_present(unsigned frame) {
    if(!attribution_period)return;
    if(fh_render_attribution_active && attribution){
        fprintf(attribution,"{\"frame\":%u,\"period\":%u,\"ticks_hz\":%u,\"gx\":[",frame,attribution_period,(unsigned)SYSCLOCK_ARM11);
        for(unsigned i=0;i<4;i++)fprintf(attribution,"%s%llu",i?",":"",(unsigned long long)(fh_gx_phase_ticks[i]-attribution_previous.gx[i]));
        fputs("],\"shading\":[",attribution);
        for(unsigned i=0;i<3;i++)fprintf(attribution,"%s%llu",i?",":"",(unsigned long long)(fh_gx_shading_ticks[i]-attribution_previous.shading[i]));
        fputs("],\"backend\":[",attribution);
        for(unsigned k=0;k<3;k++){
            fprintf(attribution,"%s{\"calls\":%u,\"ticks\":[",k?",":"",fh_backend_calls[k]-attribution_previous.calls[k]);
            for(unsigned i=0;i<6;i++)fprintf(attribution,"%s%llu",i?",":"",(unsigned long long)(fh_backend_ticks[k][i]-attribution_previous.backend[k][i]));
            fputs("]}",attribution);
        }
        fputs("]}\n",attribution);
    }
    memcpy(attribution_previous.gx,fh_gx_phase_ticks,sizeof(attribution_previous.gx));
    memcpy(attribution_previous.shading,fh_gx_shading_ticks,sizeof(attribution_previous.shading));
    memcpy(attribution_previous.backend,fh_backend_ticks,sizeof(attribution_previous.backend));
    memcpy(attribution_previous.calls,fh_backend_calls,sizeof(attribution_previous.calls));
    fh_render_attribution_active=frame%attribution_period==0;
}
u64 fh_gpu_begin_ticks;
static u64 material_ticks,texture_ticks,submit_ticks,fragment_ticks,audio_ticks,mesh_ticks,copy_ticks;
#ifdef FH_EMULATOR_TEST
typedef struct {u64 total,transfer,conversion;uint32_t source[4];unsigned width,height,format,flags,calls;} CopyProfile;
static CopyProfile copy_profiles[16];
static unsigned copy_profile_overflow;
static void profile_copy(const uint32_t source[4],unsigned width,unsigned height,unsigned format,unsigned flags,u64 total,u64 transfer,u64 conversion) {
    for(unsigned i=0;i<16;i++) {
        CopyProfile *p=copy_profiles+i;
        if(!p->calls){memcpy(p->source,source,sizeof(p->source));p->width=width;p->height=height;p->format=format;p->flags=flags;}
        if(!memcmp(p->source,source,sizeof(p->source)) && p->width==width && p->height==height && p->format==format && p->flags==flags){
            p->calls++;p->total+=total;p->transfer+=transfer;p->conversion+=conversion;return;
        }
    }
    copy_profile_overflow++;
}
static void copy_profile_finish(void) {
    FILE *f=fopen("sdmc:/3ds/foxhollow/copy-profile.csv","wb");if(!f)return;
    fputs("x,y,source_width,source_height,width,height,format,flags,calls,total_ms,transfer_ms,conversion_ms\n",f);
    for(unsigned i=0;i<16;i++) {
        const CopyProfile *p=copy_profiles+i;if(!p->calls)continue;
        fprintf(f,"%lu,%lu,%lu,%lu,%u,%u,%u,%u,%u,%.3f,%.3f,%.3f\n",(unsigned long)p->source[0],(unsigned long)p->source[1],(unsigned long)p->source[2],(unsigned long)p->source[3],
            p->width,p->height,p->format,p->flags,p->calls,p->total*1000.0/SYSCLOCK_ARM11,p->transfer*1000.0/SYSCLOCK_ARM11,p->conversion*1000.0/SYSCLOCK_ARM11);
    }
    fprintf(f,"# overflow=%u\n",copy_profile_overflow);fclose(f);
}
#endif
static unsigned copy_pixels,gpu_transform_vertices,cpu_transform_fallback_vertices;
static unsigned material_hits,material_misses,texture_uploads,hashed_bytes;
#ifdef FH_EMULATOR_TEST
unsigned fh_renderer_texture_hashed_bytes(void){return hashed_bytes;}
/* Same one-based frame number written by present() to render.csv. */
unsigned fh_renderer_profile_frame(void){return frame_number+1;}
#endif
static unsigned mesh_triangles,mesh_fallbacks;
#ifdef FH_EMULATOR_TEST
/* Tests explicitly select the candidate, retaining the scalar oracle. */
static unsigned fur_mesh_enabled;
#else
/* Console validation candidate: keep every fur shell, with indirect
 * coordinates evaluated at vertices and noise/base sampling on PICA.
 * Unsupported coordinate layouts retain the scalar material fallback. */
static unsigned fur_mesh_enabled=2;
#endif
static uint32_t *fragment_pixels_cpu;
#ifdef FH_EMULATOR_TEST
static unsigned fur_reference_frame;
static unsigned capture_interval=60;
/* Only the synthetic fixture uses this to retain an independent subdivision
 * reference while comparing the new fragment-lighting path in one run. */
unsigned fh_sphere_reference;
unsigned fh_affine_reference;
unsigned fh_affine_vertices;
unsigned fh_vertex_reference;
#endif
static unsigned fur_mode(void) {
#ifdef FH_EMULATOR_TEST
    /* Presented frames are one-based. A bounded full-scene reference avoids
     * repeating minutes of scalar fur before the frame under comparison.
     * Game logic and every draw still execute normally on both routes. */
    if(fur_reference_frame && frame_number+1==fur_reference_frame)return 0;
#endif
    return fur_mesh_enabled;
}
static unsigned fur_surface_enabled,fur_surface_plans;
static unsigned combiner_valid,raster_valid;
static unsigned fog_valid;
static unsigned char last_fog[24];
static C3D_FogLut fog_lut;
static unsigned fog_lut_valid;
static uint32_t fog_lut_key[5];
static FhMaterial last_combiner;
static unsigned last_intensity[3];
static struct { unsigned clip[4],cull,depth,mask,alpha,blend,src,dst; } last_raster;
static u64 frame_start,previous_present_tick;
static u64 audio_service_tick;
static void service_audio(void) {
#ifdef FH_GX_DISPATCH_BUILD
    extern int fh_gx_dispatch_is_worker(void);
    extern int fh_audio_worker_is_current(void);
    if(fh_audio_worker_is_current() || fh_gx_dispatch_is_worker()){
        extern void fh_audio_worker_service_current(void);
        fh_audio_worker_service_current();return;
    }
#endif
    u64 now=svcGetSystemTick();
    /* Movie synth jobs produce one 5-ms block per producer/worker handoff.
     * The old 10-ms bulk-fill cadence would necessarily starve that pipeline.
     * Poll cheaply at 1 ms; the synth try-lock keeps renderer work nonblocking. */
    extern unsigned fh_audio_synth_jobs_enabled;
    unsigned period=fh_audio_synth_jobs_enabled?SYSCLOCK_ARM11/1000:SYSCLOCK_ARM11/100;
    if(now-audio_service_tick>=period){fh_renderer_material_scope_invalidate();fh_audio_pump();audio_service_tick=svcGetSystemTick();audio_ticks+=audio_service_tick-now;}
}
static float cpu_total,gpu_total;
static struct { unsigned valid,frame,draws,vertices,barriers,commands,texture_bytes,pixels,triangles;float wall,cpu,gpu,interval; } pending_timing;
static void finish_timing(void) {
    if(!pending_timing.valid)return;
    if(timings){
        fprintf(timings,"%u,%u,%u,%u,%u,%u,%u,%u,%.3f,%.3f,%.3f,%.3f\n",pending_timing.frame,pending_timing.draws,pending_timing.vertices,pending_timing.barriers,pending_timing.commands,pending_timing.texture_bytes,pending_timing.pixels,pending_timing.triangles,pending_timing.wall,pending_timing.cpu,pending_timing.gpu+C3D_GetDrawingTime(),pending_timing.interval);
    }
    pending_timing.valid=0;
}
static void begin(void);
static void bind_geometry(void);
static void render_direct(const FhVertex *input,unsigned count,const FhDrawState *state,const FhMaterial *plan);
static void render_direct_coordinates(const FhVertex *input,unsigned count,const FhDrawState *state,const FhMaterial *plan,const float uv7[][3]);
static void flush_vertices(void);
static void flush_stream_buffers(void);
static void reserve_commands(void) {
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity || capacity>1024*1024/4) fh_platform_panic("Invalid GPU command buffer position");
    unsigned used=1024*1024-(capacity-offset)*4;
    if(used>command_peak) command_peak=used;
    unsigned limit=1024*1024;
#ifdef FH_GPU_COMMAND_LIMIT
    limit=FH_GPU_COMMAND_LIMIT;
#endif
    if(used+64*1024>limit) fh_renderer_sync();
    else if(early_queue_enabled && !early_queue_full && used>=early_queue_offset+FH_EARLY_QUEUE_BYTES){
        /* Only completed draws and their flushed resources enter the queue.
         * The command/vertex cursors remain append-only until a joined fence. */
        /* Keep the pending compatible draw open: it has no command yet.
         * Submitting a prefix must not destroy adjacent-draw batching. */
        flush_stream_buffers();
        if(fh_early_queue_append()){
            GPUCMD_GetBuffer(NULL,&capacity,&offset);
            early_queue_offset=1024*1024-(capacity-offset)*4;
        }else early_queue_full=1;
    }
}
static void flush_draw(void) {
    if(!pending_count)return;
    C3D_DrawElements(point_program_active?GPU_GEOMETRY_PRIM:GPU_TRIANGLES,pending_count,C3D_UNSIGNED_SHORT,indices+pending_start);draws++;total_vertices+=pending_count;pending_count=0;
}
unsigned fh_uniform_rows_enabled,fh_uniform_rows_requested,fh_uniform_rows_written;
static FhUniformRows uniform_rows;
static void uniform_row(unsigned row,float x,float y,float z,float w){
    float values[4]={x,y,z,w};fh_uniform_rows_requested++;
    if(fh_uniform_rows_enabled && !fh_uniform_row_changed(&uniform_rows,row,values))return;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,row,x,y,z,w);fh_uniform_rows_written++;
}
static void configure_lighting(const FhDrawState *state) {
    unsigned enabled=state && (state->raw_position&2);
    if(!lighting_valid || lighting_enabled!=enabled || (enabled && fh_memory_compare(&prepared_lighting,&uploaded_lighting,sizeof(prepared_lighting)))) {
        unsigned flat=enabled && (prepared_lighting.mask&FH_LIGHT_FLAT);
        flush_draw();C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x6a,enabled && !flat);
        /* model_normal selects the short flat path only when spot_lighting
         * is false. Reuse the existing 16 booleans and uniform allocation. */
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x73,enabled && (flat || (prepared_lighting.mask&FH_LIGHT_MODEL_NORMAL)));
        if(enabled) {
            const float *rows=(const float*)&prepared_lighting;
            for(unsigned i=0;i<44;i++)if(!flat || i<3 || i==43)uniform_row(44+i,rows[i*4],rows[i*4+1],rows[i*4+2],rows[i*4+3]);
            for(unsigned i=0;i<8;i++)C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x6b+i,(prepared_lighting.mask&(1u<<i))!=0);
            uploaded_lighting=prepared_lighting;lighting_uniform_vectors+=flat?4:44;
        }
        lighting_valid=1;lighting_enabled=enabled;
    }
}
static void configure_transform(const FhDrawState *state) {
    /* EFB clear/presentation helpers emit pretransformed vertices and rely on
     * the ordinary shader's raw_transform=false branch. A specialized shader
     * must be unbound even when no normal material draw intervenes. */
    if(!state && sphere_program_active){
        flush_draw();sphere_program_wanted=0;bind_geometry();
    }
    if(compiled_fur_gpu && (!fur_strength_valid || uploaded_fur_strength!=compiled_fur_strength)){
        flush_draw();uniform_row(91,compiled_fur_strength,0,0,0);
        uploaded_fur_strength=compiled_fur_strength;fur_strength_valid=1;
    }
    configure_lighting(state);
    unsigned texgen=state && (state->raw_position&4);
    if(!texgen_valid || texgen_enabled!=texgen || (texgen && (prepared_texgen_units!=uploaded_texgen_units || fh_memory_compare(prepared_texgen_post,uploaded_texgen_post,sizeof(prepared_texgen_post))))) {
        flush_draw();C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x74,texgen!=0);
        if(texgen){
            for(unsigned unit=0;unit<3;unit++)C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x75+unit,(prepared_texgen_units&(1u<<unit))!=0);
            for(unsigned r=0;r<3;r++)uniform_row(88+r,prepared_texgen_post[r][0],prepared_texgen_post[r][1],prepared_texgen_post[r][2],prepared_texgen_post[r][3]);
            memcpy(uploaded_texgen_post,prepared_texgen_post,sizeof(prepared_texgen_post));
            uploaded_texgen_units=prepared_texgen_units;
        }
        texgen_valid=1;texgen_enabled=texgen;
    }
    unsigned enabled=state && state->raw_position;
    float matrix[4][4];
    if(enabled)fh_vertex_pica_projection(matrix,state);
    if(!transform_valid || transform_enabled!=enabled || (enabled && (fh_memory_compare(matrix,transform_matrix,sizeof(matrix)) || fh_memory_compare(state->model_palette,transform_models,sizeof(transform_models))))){
        flush_draw();C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,enabled!=0);
        if(enabled){
            for(unsigned row=0;row<30;row++)uniform_row(row,state->model_palette[row][0],state->model_palette[row][1],state->model_palette[row][2],state->model_palette[row][3]);
            for(unsigned row=0;row<4;row++)uniform_row(30+row,matrix[row][0],matrix[row][1],matrix[row][2],matrix[row][3]);
            memcpy(transform_matrix,matrix,sizeof(matrix));
            memcpy(transform_models,state->model_palette,sizeof(transform_models));
        }
        transform_valid=1;transform_enabled=enabled;
    }
}
static void flush_stream(void *buffer,unsigned bytes,unsigned *flushed,unsigned reference_start) {
    stream_flush_full_bytes+=((bytes+31)&~31u)-(reference_start&~31u);
    unsigned start=*flushed&~31u;
#ifdef FH_EMULATOR_TEST
    if(stream_full_flush)start=reference_start&~31u;
#endif
    if(bytes>*flushed
#ifdef FH_EMULATOR_TEST
       || (stream_full_flush && bytes)
#endif
    ){
        unsigned size=((bytes+31)&~31u)-start;
        u64 before=svcGetSystemTick();
        if(R_FAILED(GSPGPU_FlushDataCache((unsigned char*)buffer+start,size)))fh_platform_panic("Stream cache flush failed");
        stream_flush_ticks+=svcGetSystemTick()-before;
        stream_flush_bytes+=size;stream_flush_calls++;
        /* Keep the byte end, not rounded end: a later append can dirty the
         * same final cache line and must flush that line again. */
        *flushed=bytes;
    }
}
static void flush_stream_buffers(void) {
    flush_stream(vertices,vertex_count*sizeof(*vertices),&flushed_vertex_bytes,0);
    flush_stream(indices,index_count*sizeof(*indices),&flushed_index_bytes,0);
}
static void flush_vertices(void) {flush_draw();flush_stream_buffers();}
static unsigned affine_supported(const FhMaterial *plan) {
    if(!plan || plan->pipeline.count || plan->sphere_lighting || plan->cpu_fragment || plan->movie_layer)return 0;
#ifdef FH_EMULATOR_TEST
    if(fh_affine_reference)return 0;
#endif
    for(unsigned c=0;c<4;c++) {
        float bound=0;for(unsigned j=0;j<9;j++)bound+=fabsf(plan->color[c][j]);
        if(!isfinite(bound) || bound>4)return 0;
    }
    return 1;
}
static unsigned gpu_lighting_supported(const FhMaterial *plan){
    if(!plan)return 0;
    if(plan->pipeline.count){
        if(plan->cpu_fragment || plan->sphere_lighting || plan->movie_layer)return 0;
        for(unsigned c=0;c<3;c++)if(plan->pipeline.primary[c]!=(int)c)return 0;
        return plan->pipeline.primary[3]==3 || plan->pipeline.primary[3]==-1;
    }
    if(!affine_supported(plan))return 0;
    for(unsigned c=0;c<4;c++)for(unsigned j=5;j<9;j++)if(plan->color[c][j]!=0)return 0;
    return 1;
}
static unsigned configure_affine(const FhMaterial *plan) {
    unsigned enabled=affine_supported(plan);
    if(!affine_valid || affine_enabled!=enabled || (enabled &&
       (fh_memory_compare(affine_coefficients,plan->color,sizeof(affine_coefficients)) || fh_memory_compare(affine_scales,plan->scale,sizeof(affine_scales))))) {
        flush_draw();C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x69,enabled!=0);
        if(enabled) {
            for(unsigned c=0;c<4;c++) {
                uniform_row(34+c,plan->color[c][1],plan->color[c][2],plan->color[c][3],plan->color[c][4]);
                uniform_row(38+c,plan->color[c][5],plan->color[c][6],plan->color[c][7],plan->color[c][8]);
            }
            uniform_row(42,plan->color[0][0],plan->color[1][0],plan->color[2][0],plan->color[3][0]);
            float rgb=1.f/plan->scale[0],alpha=1.f/plan->scale[1];
            uniform_row(43,rgb,rgb,rgb,alpha);
            memcpy(affine_coefficients,plan->color,sizeof(affine_coefficients));memcpy(affine_scales,plan->scale,sizeof(affine_scales));
        }
        affine_valid=1;affine_enabled=enabled;
    }
    return enabled;
}
int fh_renderer_ready(void) { return fh_minimal_enabled?fh_minimal_ready():initialized!=0; }
int fh_renderer_transform_available(void) { return 1; }
void *fh_renderer_retained_alloc(unsigned bytes) {
    /* Only ARM reads these compact snapshots; replay copies into separately
     * owned GPU buffers. Pinned Melee also allocates decoded geometry from
     * ordinary heap. Keep scarce linear memory for resident packed meshes. */
    extern u32 __ctru_heap_size,__stacksize__;
    if(bytes!=4*1024*1024)return NULL;
    uint64_t used=(unsigned)mallinfo().uordblks;
    if((used+bytes+__stacksize__+64)*100>(uint64_t)__ctru_heap_size*85)return NULL;
    return memalign(32,bytes);
}
void *fh_renderer_cpu_cache_alloc(unsigned bytes) {
    /* Bounds and their owned source copies are read only by ARM. Melee's
     * geometry cache likewise keeps CPU ownership separate from GPU buffers.
     * Preserve the normal-heap reserve including the reserved main stack;
     * decline the optional cache if admission would consume that headroom. */
    extern u32 __ctru_heap_size,__stacksize__;
    if(bytes!=1024*1024 && bytes!=256*1024)return NULL;
    uint64_t used=(unsigned)mallinfo().uordblks;
    if((used+bytes+__stacksize__+64)*100>(uint64_t)__ctru_heap_size*85)return NULL;
    return memalign(32,bytes);
}
void fh_renderer_init(void) {
    if(fh_minimal_enabled){fh_minimal_init();return;}
    if(fh_render_floor)return;
    if(initialized) return;
    FILE *light_lod_config=fopen("sdmc:/3ds/foxhollow/lighting_lod.txt","rb");
    if(light_lod_config){fh_light_lod_enabled=fgetc(light_lod_config)=='1';fclose(light_lod_config);}
    if(fh_light_lod_enabled)fh_platform_log("Reduced model lighting enabled: two spatial lights plus directional fill\n");
    FILE *reflection_config=fopen("sdmc:/3ds/foxhollow/reflection_lod.txt","rb");
    if(reflection_config){fh_reflective_lod_enabled=fgetc(reflection_config)=='1';fclose(reflection_config);}
    FILE *indirect_config=fopen("sdmc:/3ds/foxhollow/indirect_lighting.txt","rb");
    if(indirect_config){fh_indirect_lighting_enabled=fgetc(indirect_config)=='1';fclose(indirect_config);}
    FILE *surface_config=fopen("sdmc:/3ds/foxhollow/fur_surface.txt","rb");
    if(surface_config){fur_surface_enabled=fgetc(surface_config)=='1';fclose(surface_config);}
    if(fur_surface_enabled)fh_platform_log("Reduced fur surface enabled; direct GPU noise coordinates\n");
    FILE *attribution_config=fopen("sdmc:/3ds/foxhollow/render_attribution.txt","rb");
    if(attribution_config){
        unsigned period=0;
        if(fscanf(attribution_config,"%u",&period)==1 && period<=1024)attribution_period=period;
        fclose(attribution_config);
    }
    if(attribution_period){
        attribution=fopen("sdmc:/3ds/foxhollow/render-attribution.jsonl","wb");
        if(!attribution || fh_diagnostic_buffer(attribution))fh_platform_panic("Render attribution log unavailable");
        /* Leave the partial initialization frame unsampled. */
        fh_render_attribution_active=0;
    }
#ifndef FH_EMULATOR_TEST
    {
        char line[128];snprintf(line,sizeof(line),"Render component timing samples whole frames every %u; FPS/workload clocks remain live\n",attribution_period?attribution_period:16);
        fh_platform_log(line);
    }
#endif
    FILE *fur_config=fopen("sdmc:/3ds/foxhollow/fur_coefficients.txt","rb");
    if(fur_config){int mode=fgetc(fur_config);if(mode>='0' && mode<='2')fh_fur_coefficients_mode=mode-'0';fclose(fur_config);}
    FILE *uniform_config=fopen("sdmc:/3ds/foxhollow/uniform_rows.txt","rb");
    if(uniform_config){fh_uniform_rows_enabled=fgetc(uniform_config)=='1';fclose(uniform_config);}
    FILE *movie_config=fopen("sdmc:/3ds/foxhollow/movie_batch.txt","rb");
    if(movie_config){int mode=fgetc(movie_config);if(mode>='0' && mode<='3')fh_movie_batch_enabled=mode-'0';fclose(movie_config);}
    FILE *early_config=fopen("sdmc:/3ds/foxhollow/early_queue.txt","rb");
    if(early_config){early_queue_enabled=fgetc(early_config)=='1';fclose(early_config);}
    FILE *copy_config=fopen("sdmc:/3ds/foxhollow/queued_gpu_copies.txt","rb");
    if(copy_config){gpu_copy_fences=fgetc(copy_config)!='1';fclose(copy_config);}
    fh_platform_log(gpu_copy_fences?"GPU copies fenced before/after transfer for hardware crash isolation; timings include waits\n":
        "GPU-only copies ordered with draws; CPU readback and resource-reuse fences retained\n");
#ifdef FH_EMULATOR_TEST
    FILE *stream_config=fopen("sdmc:/3ds/foxhollow/stream_full_flush.txt","rb");
    if(stream_config){stream_full_flush=fgetc(stream_config)=='1';fclose(stream_config);}
#endif
#ifdef FH_DRAW_CAPTURE
    fh_capture_draw_init();
#endif
#ifdef FH_EMULATOR_TEST
    FILE *mesh=fopen("sdmc:/3ds/foxhollow/fur_mesh.txt","rb");if(mesh){fur_mesh_enabled=fgetc(mesh)=='v'?2:1;fclose(mesh);}
    FILE *reference=fopen("sdmc:/3ds/foxhollow/fur_reference_frame.txt","r");
    if(reference){if(fscanf(reference,"%u",&fur_reference_frame)!=1)fur_reference_frame=0;fclose(reference);}
    FILE *capture_config=fopen("sdmc:/3ds/foxhollow/capture_interval.txt","r");
    if(capture_config){unsigned n;if(fscanf(capture_config,"%u",&n)==1 && n && n<=600)capture_interval=n;fclose(capture_config);}
#endif
    if(!C3D_Init(1024*1024)) fh_platform_panic("Citro3D initialization failed");
    FILE *layout=fopen("sdmc:/3ds/foxhollow/efb_contiguous.txt","rb");
    if(layout){int mode=fgetc(layout);if(mode=='1' || mode=='2')efb_storage_width=512;
        if(mode=='2'){efb_storage_height=512;efb_direct_view=1;}fclose(layout);}
    FILE *view_config=fopen("sdmc:/3ds/foxhollow/efb_ordered_view.txt","rb");
    if(view_config){efb_ordered_view=efb_direct_view && fgetc(view_config)=='1';fclose(view_config);}
    efb_storage_bytes=efb_storage_width*efb_storage_height*4;
    if(efb_direct_view){
        if(!C3D_TexInitVRAM(&efb_target_texture,512,512,GPU_RGBA8))fh_platform_panic("Sampleable EFB allocation failed");
        target=C3D_RenderTargetCreateFromTex(&efb_target_texture,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=GPU_RB_DEPTH24_STENCIL8});
        /* Texture storage includes unused padding rows. Raster coordinates
         * retain the same 400-row framebuffer as the padded capture route. */
        if(target)target->frameBuf.height=400;
    }else target=C3D_RenderTargetCreate(efb_storage_width,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    display_target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,-1);
    vertices=linearMemAlign(VERTICES*sizeof(*vertices),32);
    indices=linearMemAlign(VERTICES*sizeof(*indices),32);
    readback=linearMemAlign(efb_storage_bytes,4096);
    if(!target || !display_target || !vertices || !indices || !readback) fh_platform_panic("Renderer memory allocation failed");
    /* Discard any cache lines left by the allocator's previous owner before
     * handing this page-aligned allocation to GPU writes for its lifetime. */
    if(R_FAILED(GSPGPU_InvalidateDataCache(readback,efb_storage_bytes)))fh_platform_panic("Initial EFB readback ownership");
#ifdef FH_EMULATOR_TEST
    FILE *compact_reference=fopen("sdmc:/3ds/foxhollow/readback_compact_reference.txt","r");
    if(compact_reference){readback_compact_reference=fgetc(compact_reference)=='1';fclose(compact_reference);}
#endif
    C3D_RenderTargetSetOutput(display_target,GFX_TOP,GFX_LEFT,GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8));
    shader=DVLB_ParseFile((u32*)game_shader,sizeof(game_shader));if(!shader) fh_platform_panic("Vertex shader parse failed");
    shaderProgramInit(&program);shaderProgramSetVsh(&program,&shader->DVLE[0]);
    if(shader->numDVLE<2)fh_platform_panic("Missing point geometry shader");
    shaderProgramInit(&point_program);shaderProgramSetVsh(&point_program,&shader->DVLE[0]);
    shaderProgramSetGsh(&point_program,&shader->DVLE[1],6);
    sphere_shader_binary=DVLB_ParseFile((u32*)sphere_shader,sizeof(sphere_shader));
    if(!sphere_shader_binary)fh_platform_panic("Sphere shader parse failed");
    shaderProgramInit(&sphere_program);shaderProgramSetVsh(&sphere_program,&sphere_shader_binary->DVLE[0]);
    FILE *sphere_config=fopen("sdmc:/3ds/foxhollow/gpu_sphere.txt","rb");
    if(sphere_config){fh_gpu_sphere_disabled=fgetc(sphere_config)!='1';fclose(sphere_config);}
    timings=fopen("sdmc:/3ds/foxhollow/render.csv","wb");
    if(fh_diagnostic_buffer(timings))fh_platform_panic("Render telemetry buffer allocation failed");
    if(timings) fputs("frame,draws,vertices,barriers,command_peak_bytes,textures_bytes,fragment_pixels,fragment_triangles,wall_ms,cpu_ms,gpu_ms,interval_ms\n",timings);
    /* Component timings must also come from the console. Counters already run
     * in both builds; buffer records rather than syncing SD for every frame. */
    profile=fopen("sdmc:/3ds/foxhollow/render-profile.csv","wb");
    if(fh_diagnostic_buffer(profile))fh_platform_panic("Render profile buffer allocation failed");
    if(profile)fputs("frame,material_ms,texture_ms,submit_ms,fragment_ms,material_hits,material_misses,texture_uploads,hashed_bytes,mesh_triangles,mesh_fallbacks,audio_ms,mesh_ms,copy_ms,copy_pixels,gpu_transform_vertices,cpu_transform_fallback_vertices,packed_vertices,reused_vertices,gpu_lit_vertices,gpu_light_evaluations,gpu_lighting_fallback_vertices,lighting_uniform_vectors,gpu_normal_vertices,gpu_texgen_vertices,stream_flush_bytes,stream_flush_full_bytes,stream_flush_calls,stream_flush_ms,queue_join_ms,early_prefixes\n",profile);
    initialized=1;efb_color_init();efb_depth_init();efb_shadow_init();fh_fragment_worker_start();fh_platform_log("PICA renderer initialized\n");
    {char line[160];snprintf(line,sizeof(line),"EFB backing=%ux%u bytes=%u viewport=264x400 display=240x400\n",efb_storage_width,efb_storage_height,efb_storage_bytes);fh_platform_log(line);}
    if(early_queue_enabled)fh_platform_log("Early GPU queue enabled; gpu_ms measures remaining queue tails, not complete GPU execution\n");
    fh_platform_log("Texture storage budget=12582912 cache_limit=10485760 reserve=2097152\n");
}
static void bind_geometry(void) {
    fh_uniform_rows_reset(&uniform_rows);
    combiner_valid=raster_valid=0;
    transform_valid=affine_valid=lighting_valid=texgen_valid=fur_strength_valid=0;
    point_program_active=point_context.active;
    sphere_program_active=!point_context.active && sphere_program_wanted;
    C3D_BindProgram(point_context.active?&point_program:sphere_program_active?&sphere_program:&program);C3D_AttrInfo *attributes=C3D_GetAttrInfo();AttrInfo_Init(attributes);
    AttrInfo_AddLoader(attributes,0,GPU_FLOAT,4);AttrInfo_AddLoader(attributes,1,GPU_FLOAT,4);
    for(unsigned i=2;i<5;i++) AttrInfo_AddLoader(attributes,i,GPU_FLOAT,3);
    AttrInfo_AddLoader(attributes,5,GPU_FLOAT,2);AttrInfo_AddLoader(attributes,6,GPU_FLOAT,3);
#ifdef FH_COMPACT_VERTEX_BUILD
    AttrInfo_AddFixed(attributes,7);C3D_FixedAttribSet(7,1,1,0,0);
#endif
    C3D_BufInfo *buffers=C3D_GetBufInfo();BufInfo_Init(buffers);
    if(BufInfo_Add(buffers,vertices,sizeof(*vertices),7,0x6543210)<0) fh_platform_panic("Vertex buffer binding failed");
    C3D_FrameDrawOn(target);C3D_SetViewport(0,0,FH_EFB_NATIVE_HEIGHT,400);
}
static void vertex_buffer(const GpuVertex *data) {
    C3D_BufInfo *buffers=C3D_GetBufInfo();BufInfo_Init(buffers);
    if(BufInfo_Add(buffers,data,sizeof(*data),7,0x6543210)<0)fh_platform_panic("Retained vertex buffer binding failed");
}
#ifdef FH_COMPACT_VERTEX_BUILD
static void compact_vertex_buffer(const void *data,int compact){
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    AttrInfo_AddLoader(a,0,compact?GPU_SHORT:GPU_FLOAT,4);
    if(compact)AttrInfo_AddFixed(a,1);else AttrInfo_AddLoader(a,1,GPU_FLOAT,4);
    for(unsigned i=2;i<5;i++)AttrInfo_AddLoader(a,i,GPU_FLOAT,3);
    AttrInfo_AddLoader(a,5,compact?GPU_SHORT:GPU_FLOAT,2);
    AttrInfo_AddLoader(a,6,compact?GPU_BYTE:GPU_FLOAT,compact?4:3);
    AttrInfo_AddFixed(a,7);
    /* Melee's explicit permutation avoids SDK int-shift pitfalls. */
    a->permutation=UINT64_C(0x76543210);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,data,compact?sizeof(FhCompactVertex):sizeof(GpuVertex),compact?6:7,
        compact?UINT64_C(0x654320):UINT64_C(0x6543210))<0)fh_platform_panic("Compact vertex binding failed");
    if(compact){
        C3D_FixedAttribSet(1,compact_fixed.color[0],compact_fixed.color[1],compact_fixed.color[2],compact_fixed.color[3]);
        C3D_FixedAttribSet(7,compact_fixed.position_scale,compact_fixed.normal_scale,0,0);
    }else C3D_FixedAttribSet(7,1,1,0,0);
}
#endif
static void configure_fog(const FhDrawState *state) {
    if(fog_valid && !fh_memory_compare(last_fog,&state->fog_type,sizeof(last_fog)))return;
    flush_draw();
    memcpy(last_fog,&state->fog_type,sizeof(last_fog));fog_valid=1;
    unsigned type=state->fog_type&7;
    if(!type){C3D_FogGasMode(GPU_NO_FOG,(GPU_GASMODE)0,false);return;}
    if(type!=2 && type<4)fh_platform_panic("Invalid fog type");
    uint32_t key[5]={state->fog_type};memcpy(key+1,&state->fog_start,16);
    if(!fog_lut_valid || memcmp(key,fog_lut_key,sizeof(key))){
        float data[256],previous=0;
        for(unsigned i=0;i<=128;i++) {
            float value=fh_fog_transmission(state,i/128.f);
            if(i<128)data[i]=value;
            if(i)data[i+127]=value-previous;
            previous=value;
        }
        FogLut_FromArray(&fog_lut,data);memcpy(fog_lut_key,key,sizeof(key));fog_lut_valid=1;
    }
    /* Color and temporary fog disable do not change the transmission curve.
     * Retain exact packed LUT bytes, but keep the existing explicit binding. */
    C3D_FogLutBind(&fog_lut);
    unsigned color=state->fog_color;
    C3D_FogColor((color>>24)|((color>>8)&0xff00)|((color<<8)&0xff0000));
    C3D_FogGasMode(GPU_FOG,(GPU_GASMODE)0,false);
}
static void begin(void) {
    if(!initialized) fh_renderer_init();
    if(open_frame){
        if(point_program_active!=point_context.active){flush_draw();bind_geometry();}
        return;
    }
    fh_fur_cache_invalidate();
    u64 begin_tick=svcGetSystemTick();
    bounded_frame_begin();
    fh_gpu_begin_ticks+=svcGetSystemTick()-begin_tick;
    finish_timing();cpu_total=gpu_total=0;
    material_ticks=texture_ticks=submit_ticks=fragment_ticks=audio_ticks=mesh_ticks=copy_ticks=0;material_hits=material_misses=texture_uploads=hashed_bytes=copy_pixels=0;
    gpu_transform_vertices=cpu_transform_fallback_vertices=0;
    gpu_lit_vertices=gpu_light_evaluations=gpu_lighting_fallback_vertices=lighting_uniform_vectors=0;
    gpu_normal_vertices=gpu_texgen_vertices=0;
    frame_unique_base=fh_vertex_unique;frame_reused_base=fh_vertex_reused;
    frame_start=svcGetSystemTick();vertex_count=index_count=draws=total_vertices=barriers=command_peak=0;open_frame=1;
#ifdef FH_EMULATOR_TEST
    frame_profile_active=1;
#else
    frame_profile_active=frame_number%(attribution_period?attribution_period:16)==0;
#endif
    flushed_vertex_bytes=flushed_index_bytes=0;
    stream_flush_bytes=stream_flush_full_bytes=stream_flush_calls=0;stream_flush_ticks=0;
    queue_join_ticks=0;early_frame_starts=fh_early_queue_starts;
    depth_snapshot_valid=0;
    native_geometry_epoch++;
    fragment_pixels=fragment_triangles=atlas_x=atlas_y=atlas_row=0;
    mesh_triangles=mesh_fallbacks=0;
    C3D_RenderTargetClear(target,C3D_CLEAR_ALL,black?255:clear_color,0xffffff-clear_depth);
    bind_geometry();
}
void fh_renderer_dispatch_begin(void){if(!fh_render_floor)begin();}
void fh_renderer_sync(void) {
    if(!initialized || !open_frame) return;
    boot_gpu_trace("sync-submit");
    flush_vertices();bool used=target->used;target->used=false;
    fh_early_queue_defer();C3D_FrameEnd(GX_CMDLIST_FLUSH);cpu_total+=C3D_GetProcessingTime();
#ifdef FH_EMULATOR_TEST
    if(fh_gpu_boot_trace && frame_number<32)boot_queue_trace();
#endif
    bounded_frame_begin();
    gpu_total+=C3D_GetDrawingTime();
    native_geometry_epoch++;
    target->used=used;vertex_count=index_count=0;barriers++;bind_geometry();
    flushed_vertex_bytes=flushed_index_bytes=0;
}
static int texture_matches(const Texture *entry,const FhTexture *source) {
    return entry->bytes && entry->source.pixels==source->pixels && entry->source.palette==source->palette && entry->source.width==source->width && entry->source.height==source->height && entry->source.format==source->format && entry->source.palette_format==source->palette_format && entry->source.palette_count==source->palette_count;
}
static unsigned texture_hash(const FhTexture *source,unsigned hash) {
    hashed_bytes+=fh_texture_source_bytes(source->format,source->width,source->height)+(source->palette?source->palette_count*2:0);
    hash=fh_hash_bytes(hash,source->pixels,fh_texture_source_bytes(source->format,source->width,source->height));
    if(source->palette)hash=fh_hash_bytes(hash,source->palette,source->palette_count*2);
    return hash;
}
static void retire(Texture *entry) {
    if(!entry->bytes) return;
    C3D_TexDelete(&entry->gpu);texture_bytes-=entry->bytes;memset(entry,0,sizeof(*entry));
}
static void reserve_texture_linear(unsigned bytes) {
    native_geometry_trim(bytes);
    /* The texture byte budget alone cannot account for geometry, DSP and
     * readback storage. Evict only unpinned textures, after GPU completion,
     * when optional geometry cannot provide the shared linear reserve. */
    while(linearSpaceFree()<bytes+LINEAR_RESERVE) {
        Texture *oldest=NULL;
        for(unsigned i=0;i<TEXTURES;i++)if(textures[i].bytes && !textures[i].pinned &&
           (!oldest || textures[i].frame<oldest->frame))oldest=&textures[i];
        if(!oldest)fh_platform_panic("Live textures exceed linear RAM reserve");
        fh_renderer_sync();retire(oldest);
    }
}
static void texture_init_reclaim(C3D_Tex *gpu,unsigned width,unsigned height,GPU_TEXCOLOR format) {
    /* Melee renderer.c retries the real allocation after eviction. Aggregate
     * free space alone cannot establish a contiguous linear allocation. */
    if(width<8 || height<8 || width>1024 || height>1024 || (width&(width-1)) || (height&(height-1)))
        fh_platform_panic("Invalid PICA texture extent");
    for(unsigned attempt=0;attempt<=NATIVE_GEOMETRY_SLOTS+TEXTURES;attempt++) {
        if(C3D_TexInit(gpu,width,height,format))return;
        fh_texture_allocation_retries++;
#ifdef FH_TEXTURE_FRAGMENTATION_NEGATIVE
        fh_platform_panic("Regression: fragmented texture allocation was not reclaimed");
#endif
        NativeGeometry *geometry=native_geometry_oldest();
        if(geometry){
            if(geometry->epoch==native_geometry_epoch)fh_renderer_sync();
            native_geometry_retire(geometry);fh_texture_geometry_evictions++;continue;
        }
        Texture *oldest=NULL;
        for(unsigned i=0;i<TEXTURES;i++)if(textures[i].bytes && !textures[i].pinned &&
            (!oldest || textures[i].frame<oldest->frame))oldest=&textures[i];
        if(!oldest)break;
        fh_renderer_sync();retire(oldest);fh_texture_cache_evictions++;
    }
    char error[192];snprintf(error,sizeof(error),"Texture allocation exhausted: %ux%u format=%u linear_free=%u geometry=%u textures=%u",
        width,height,(unsigned)format,(unsigned)linearSpaceFree(),native_geometry_bytes,texture_bytes);
    fh_platform_panic(error);
}
#include "platform_scene_storage.h"
static int texture_binding_matches(const Texture *entry,const FhTexture *source,unsigned mode,const FhTexture *secondary) {
    if(entry->mode!=mode || !texture_matches(entry,source))return 0;
    if(mode!=1 && mode!=3)return 1;
    const FhTexture *other=&entry->secondary;
    return secondary && other->pixels==secondary->pixels && other->palette==secondary->palette && other->width==secondary->width && other->height==secondary->height && other->format==secondary->format && other->palette_format==secondary->palette_format && other->palette_count==secondary->palette_count && other->wrap_s==secondary->wrap_s && other->wrap_t==secondary->wrap_t && other->mag_filter==secondary->mag_filter;
}
static Texture *load_texture(const FhTexture *source,unsigned mode,const FhTexture *secondary);
static Texture *load_texture_unprofiled(const FhTexture *source,unsigned mode,const FhTexture *secondary) {
    Texture *scene=scene_page_lookup(source,mode);if(scene)return scene;
    Texture *copy=efb_color_lookup(source,mode);if(copy)return copy;
    copy=efb_depth_lookup(source,mode);if(copy)return copy;
    copy=efb_shadow_lookup(source,mode);if(copy)return copy;
    fh_renderer_materialize_texture(source->pixels);
    if(secondary)fh_renderer_materialize_texture(secondary->pixels);
    if(fragment_atlas.bytes && source->pixels==fragment_atlas.gpu.data) return &fragment_atlas;
    FhTexture identity=*source,secondary_identity;fh_texture_identity(&identity);
    const FhTexture *other_identity=NULL;
    if(secondary){secondary_identity=*secondary;fh_texture_identity(&secondary_identity);other_identity=&secondary_identity;}
    unsigned bucket=(((uintptr_t)identity.pixels>>5)^((uintptr_t)identity.palette>>3)^(mode*71)^source->width^(source->height<<3)^source->format)&255;
    unsigned recent=texture_recent[bucket];
    Texture *cached=recent?&textures[recent-1]:NULL;
    if(!cached || !texture_binding_matches(cached,&identity,mode,other_identity)) {
        cached=NULL;
        for(unsigned i=0;i<TEXTURES;i++)if(texture_binding_matches(&textures[i],&identity,mode,other_identity)){cached=&textures[i];break;}
    }
    if(cached)texture_recent[bucket]=(unsigned)(cached-textures)+1;
#ifdef FH_TEXTURE_LOOKUP_VERIFY
    Texture *linear=NULL;
    for(unsigned i=0;i<TEXTURES;i++)if(texture_binding_matches(&textures[i],&identity,mode,other_identity)){linear=&textures[i];break;}
    if(linear!=cached)fh_platform_panic("Texture recent lookup differs from linear lookup");
#endif
    if(cached && cached->valid){cached->frame=frame_number;return cached;}
    unsigned width=8,height=8;while(width<source->width) width*=2;while(height<source->height) height*=2;
    size_t bytes=mode==3?width*height*4:fh_texture_native_bytes(source,width,height),source_bytes=fh_texture_source_bytes(source->format,source->width,source->height);
    if(!bytes || !source_bytes || bytes>TEXTURE_CACHE_LIMIT) fh_platform_panic("Texture exceeds native budget or format support");
    unsigned hash=texture_hash(source,2166136261u);if(mode==1 || mode==3)hash=texture_hash(secondary,hash);
    if(cached && cached->hash==hash){cached->valid=1;cached->frame=frame_number;return cached;}
    if(cached){if(cached->frame==frame_number)fh_renderer_sync();retire(cached);}
    FhPackedTexture prepared;
    unsigned packed=prepared_texture_find(source,mode,&prepared);
    size_t original_bytes=bytes;
    if(packed)bytes=prepared.bytes;
retry_texture_allocation: ;
    Texture *slot=NULL;
    for(;;) {
        if(texture_bytes+bytes<=TEXTURE_CACHE_LIMIT) for(unsigned i=0;i<TEXTURES;i++) if(!textures[i].bytes) { slot=&textures[i];break; }
        if(slot) break;
        Texture *oldest=NULL;for(unsigned i=0;i<TEXTURES;i++) if(textures[i].bytes && !textures[i].pinned && (!oldest || textures[i].frame<oldest->frame)) oldest=&textures[i];
        if(!oldest) fh_platform_panic("Texture cache allocation accounting failed");
        if(oldest->frame==frame_number) fh_renderer_sync();
        retire(oldest);
    }
    reserve_texture_linear(bytes);
    texture_init_reclaim(&slot->gpu,width,height,packed?(GPU_TEXCOLOR)prepared.format:mode==3?GPU_RGBA8:(GPU_TEXCOLOR)fh_texture_native_format(source));
    if(packed && !fh_texture_pack_read(&prepared_textures,&prepared,slot->gpu.data,bytes)){
        /* The candidate was never bound or accounted as live. Corruption/IO
         * failure disables the pack and retries the original conversion. */
        C3D_TexDelete(&slot->gpu);memset(slot,0,sizeof(*slot));
        bytes=original_bytes;packed=0;goto retry_texture_allocation;
    }
    texture_uploads++;
    if(packed) {
        /* Fully checked native payload; the common flush publishes it below. */
    } else if(mode==3) {
        if(!fh_texture_pack_fur(slot->gpu.data,width,height,source,secondary))fh_platform_panic("Fur texture packing rejected source");
    } else if(!fh_texture_convert(slot->gpu.data,bytes,width,height,source,source_bytes)) fh_platform_panic("Texture conversion rejected source");
    if(mode==1 || mode==2) {
        if(slot->gpu.fmt!=GPU_RGBA8 || (mode==1 && (!secondary || secondary->format!=6))) fh_platform_panic("Material texture transform format mismatch");
        for(unsigned y=0;y<height;y++) for(unsigned x=0;x<width;x++) {
            unsigned morton=(x&1)|((y&1)<<1)|((x&2)<<1)|((y&2)<<2)|((x&4)<<2)|((y&4)<<3);
            u32 *pixel=(u32*)slot->gpu.data+((y/8)*(width/8)+x/8)*64+morton;
            if(mode==1) {
                unsigned sx=x<source->width?x:source->width-1,sy=y<source->height?y:source->height-1;
                const unsigned char *p=secondary->pixels;
                unsigned blue=p[((sy/4)*((source->width+3)/4)+sx/4)*64+32+2*((sy%4)*4+sx%4)+1];
                *pixel=(*pixel&0xffffff00)|blue;
            } else *pixel=(((((*pixel>>24)+1)/2)<<24)|(((((*pixel>>16)&255)+1)/2)<<16)|(((((*pixel>>8)&255)+1)/2)<<8)|(*pixel&255));
        }
    }
    C3D_TexFlush(&slot->gpu);slot->source=identity;if(secondary)slot->secondary=secondary_identity;slot->mode=mode;slot->bytes=bytes;slot->frame=frame_number;slot->valid=1;slot->hash=hash;texture_bytes+=bytes;texture_recent[bucket]=(unsigned)(slot-textures)+1;return slot;
}
#ifdef FH_EMULATOR_TEST
typedef struct {const void *pixels;unsigned width,height,format,mode,calls,uploads,hashed;u64 ticks;} TextureProfile;
static TextureProfile texture_profiles[256];
static unsigned texture_profile_checked,texture_profile_first,texture_profile_last,texture_profile_used,texture_profile_dropped;
static void texture_profile_finish(void){
    if(!texture_profile_first)return;
    FILE *f=fopen("sdmc:/3ds/foxhollow/texture-profile.csv","wb");if(!f)fh_platform_panic("Texture profile report");
    fprintf(f,"address,width,height,format,mode,calls,uploads,hashed_bytes,total_ms\n");
    for(unsigned i=0;i<texture_profile_used;i++){
        const TextureProfile *p=texture_profiles+i;
        fprintf(f,"%u,%u,%u,%u,%u,%u,%u,%u,%.6f\n",(unsigned)p->pixels,p->width,p->height,p->format,p->mode,p->calls,p->uploads,p->hashed,p->ticks*1000.0/SYSCLOCK_ARM11);
    }
    fclose(f);f=fopen("sdmc:/3ds/foxhollow/texture-profile-window.txt","wb");
    if(f){fprintf(f,"%u %u %u\n",texture_profile_first,texture_profile_last,texture_profile_dropped);fclose(f);}
}
#endif
static Texture *load_texture(const FhTexture *source,unsigned mode,const FhTexture *secondary){
#ifdef FH_EMULATOR_TEST
    if(!texture_profile_checked){
        texture_profile_checked=1;FILE *f=fopen("sdmc:/3ds/foxhollow/profile-textures.txt","r");
        if(f){if(fscanf(f,"%u %u",&texture_profile_first,&texture_profile_last)!=2 || texture_profile_first>texture_profile_last)texture_profile_first=0;fclose(f);}
    }
    if(texture_profile_first && frame_number>=texture_profile_first && frame_number<=texture_profile_last){
        unsigned uploads=texture_uploads,hashed=hashed_bytes;u64 start=svcGetSystemTick();
        Texture *result=load_texture_unprofiled(source,mode,secondary);u64 elapsed=svcGetSystemTick()-start;
        unsigned i;for(i=0;i<texture_profile_used;i++){
            const TextureProfile *p=texture_profiles+i;
            if(p->pixels==source->pixels && p->width==source->width && p->height==source->height && p->format==source->format && p->mode==mode)break;
        }
        if(i==256){texture_profile_dropped++;return result;}
        TextureProfile *p=texture_profiles+i;
        if(i==texture_profile_used){texture_profile_used++;*p=(TextureProfile){.pixels=source->pixels,.width=source->width,.height=source->height,.format=source->format,.mode=mode};}
        p->calls++;p->ticks+=elapsed;p->uploads+=texture_uploads-uploads;p->hashed+=hashed_bytes-hashed;return result;
    }
#endif
    return load_texture_unprofiled(source,mode,secondary);
}
static const FhMaterial *material_unscoped(const FhDrawState *state,const FhVertex *vertices_in,unsigned count,int probe) {
    if(!state->stages || state->stages>16) fh_platform_panic("Invalid material stage count");
    static const FhMaterial fur={.cpu_fragment=2};
    if(!fur_surface_enabled && fh_material_scope_fur(state) && fh_fragment_supported(state)){material_hits++;return &fur;}
    if(!++material_clock){memset(materials,0,sizeof(materials));last_material=NULL;material_clock=1;}
    unsigned hash=2166136261u;
    unsigned char key[MATERIAL_KEY_BYTES];unsigned used=0;
    FhRegisterMaterial registers;
    unsigned register_material=fh_material_register_key(&registers,state);
    if(register_material){
        /* Exact complete register plans depend on only these canonical values.
         * Keep them in the existing owned cache: texture materialization can
         * recursively draw, so a shared mutable temporary plan is unsafe. */
        memcpy(key,&registers,sizeof(registers));used=sizeof(registers);
    }else{
    unsigned active_bytes=state->stages*sizeof(FhTevStage);memcpy(key,state->tev,active_bytes);used+=active_bytes;
    memcpy(key+used,state->tev_colors,128);used+=128;
    memcpy(key+used,state->swaps,72);used+=72;
    memcpy(key+used,&state->constant_raster_mask,36);used+=36;
    for(unsigned i=0;i<8;i++) {
        memcpy(key+used,&state->textures[i].width,12);used+=12;
        memcpy(key+used,&state->textures[i].wrap_s,16);used+=16;
    }
    unsigned raster[]={state->blend_mode,state->alpha_write,state->color_write,state->depth_test,state->depth_write,state->depth_compare};
    memcpy(key+used,raster,sizeof(raster));used+=sizeof(raster);
    memcpy(key+used,state->alpha_compare,20);used+=20;
    memcpy(key+used,&state->indirect_count,36);used+=36;
    }
    memcpy(key+used,&fh_reflective_lod_enabled,4);used+=4;
    const unsigned char *p=key;
    if(last_material && last_material->valid && (!last_material->fallback_only || probe) && last_material->key_bytes==used && !fh_memory_compare(last_material->key,p,used)){
        last_material->stamp=material_clock;
        material_hits++;return probe && (last_material->fallback_only || last_material->plan.cpu_fragment)?NULL:&last_material->plan;
    }
    hash=fh_hash_bytes(hash,p,used);
    hash^=hash>>16;hash*=0x85ebca6bu;hash^=hash>>13;hash*=0xc2b2ae35u;hash^=hash>>16;
    /* Melee's four-way material cache avoids direct-map eviction between
     * unrelated materials without increasing the bounded plan capacity. */
    Material *set=&materials[(hash%(PLANS/4))*4],*entry=set;
    for(unsigned i=0;i<4;i++){
        Material *candidate=set+i;
        if(candidate->valid && candidate->hash==hash && candidate->key_bytes==used && !fh_memory_compare(candidate->key,p,used)){
            entry=candidate;
            if(!entry->fallback_only || probe){
                last_material=entry;entry->stamp=material_clock;
                material_hits++;return probe && (entry->fallback_only || entry->plan.cpu_fragment)?NULL:&entry->plan;
            }
            break;
        }
        if(candidate->stamp<entry->stamp)entry=candidate;
    }
    last_material=entry;entry->stamp=material_clock;
    material_misses++;
    entry->fallback_only=0;
    if(fh_reflective_lod_enabled && fh_reflective_lod_pipeline(&entry->plan,state))fh_reflective_lod_plans++;
    else if(fur_surface_enabled && fh_fur_surface_pipeline(&entry->plan,state))fur_surface_plans++;
    else if(register_material)fh_material_register_plan(&entry->plan,&registers);
    else if(!fh_material_compile(&entry->plan,state) && !fh_pipeline_compile(&entry->plan,state) && !fh_layered_material_compile(&entry->plan,state)) {
        if(probe)entry->fallback_only=1;
        else if(fh_fragment_supported(state)) {
            memset(&entry->plan,0,sizeof(entry->plan));entry->plan.cpu_fragment=fh_fur_material(state)?2:1;
        } else {
        FILE *file=fopen("sdmc:/3ds/foxhollow/material-failure.bin","wb");if(file) { fwrite(state,1,sizeof(*state),file);fclose(file); }
        file=fopen("sdmc:/3ds/foxhollow/material-vertices.bin","wb");if(file) { fwrite(vertices_in,sizeof(*vertices_in),count,file);fclose(file); }
        file=fopen("sdmc:/3ds/foxhollow/material-layout.json","wb");if(file){fprintf(file,"{\"state_bytes\":%u,\"vertex_bytes\":%u,\"vertex_count\":%u}\n",(unsigned)sizeof(*state),(unsigned)sizeof(*vertices_in),count);fclose(file);}
#ifdef FH_DRAW_CAPTURE
        fh_capture_failed_draw(frame_number,vertices_in,count,state);
#endif
        char message[160];snprintf(message,sizeof(message),"TEV material unsupported: %s (%lu stages)",entry->plan.failure,(unsigned long)state->stages);fh_platform_panic(message);
        }
    }
    memcpy(entry->key,p,used);entry->key_bytes=used;entry->hash=hash;entry->valid=1;
    return probe && (entry->fallback_only || entry->plan.cpu_fragment)?NULL:&entry->plan;
}
static const FhMaterial *material(const FhDrawState *state,const FhVertex *vertices_in,unsigned count,int probe){
    return fh_material_scope_get(state,vertices_in,count,probe,material_unscoped);
}
static unsigned byte(float value) { return value<0?0:value>1?255:(unsigned)(value*255+.5f); }
static GPU_TESTFUNC alpha_comparison(unsigned comparison) { static const GPU_TESTFUNC values[]={GPU_NEVER,GPU_LESS,GPU_EQUAL,GPU_LEQUAL,GPU_GREATER,GPU_NOTEQUAL,GPU_GEQUAL,GPU_ALWAYS};return values[comparison&7]; }
const FhGpuLighting *fh_renderer_prepared_lighting(void) {return &prepared_lighting;}
int fh_renderer_prepare_lighting(const FhDrawState *state,const FhGpuLighting *lighting) {
    const FhMaterial *plan=material(state,NULL,0,1);
    if(lighting->mask&FH_LIGHT_SPHERE){
        if(fh_gpu_sphere_disabled || !plan || !plan->sphere_lighting)return 0;
        prepared_lighting=*lighting;prepared_light_count=6;return 1;
    }
    /* Fur's generated GPU combiner consumes primary RGB unchanged. If its
     * per-vertex coordinate checks fail, resolve lighting before ARM fallback. */
    if(!gpu_lighting_supported(plan) && !(plan && plan->cpu_fragment==2 && fur_mode()==2))return 0;
    prepared_lighting=*lighting;
    if((lighting->mask&FH_LIGHT_FLAT) && !(lighting->mask&FH_LIGHT_LIVE_ALPHA))prepared_lighting.ambient[3]=1.f;
    prepared_lighting.lights[0][3][3]=(lighting->mask&FH_LIGHT_LIVE_MATERIAL)?1.f:0.f;
    prepared_lighting.lights[0][4][3]=(lighting->mask&FH_LIGHT_NORMAL_PALETTE)?1.f:0.f;
    prepared_lighting.lights[0][0][3]=(lighting->mask&FH_LIGHT_DIFFUSE_FILL)?1.f:0.f;
    prepared_light_count=__builtin_popcount(lighting->mask&255);return 1;
}
int fh_renderer_prepare_texgen(const FhDrawState *state,const float post[3][4]) {
    const FhMaterial *plan=material(state,NULL,0,1);
    if(fur_surface_enabled && plan && fh_material_scope_fur(state)){
        /* Normal-derived coordinate zero remains sampler two, just as in
         * the full fur path. Coordinates two/four are still supplied live. */
        prepared_texgen_units=4;memcpy(prepared_texgen_post,post,sizeof(prepared_texgen_post));return 1;
    }
    if(plan && plan->cpu_fragment==2 && fur_mode()==2){
        /* Fur's sphere coordinate is unit two after its three-sampler remap.
         * Indirect sample coordinates remain CPU-visible and unchanged. */
        prepared_texgen_units=4;memcpy(prepared_texgen_post,post,sizeof(prepared_texgen_post));return 1;
    }
    if(!plan || !plan->pipeline.count || plan->cpu_fragment || plan->sphere_lighting || plan->movie_layer ||
       !plan->pipeline.textures || plan->pipeline.textures>3)return 0;
    unsigned mask=0;
    for(unsigned unit=0;unit<plan->pipeline.textures;unit++){
        if(plan->pipeline.coord[unit]==0)mask|=1u<<unit;
        else if(plan->pipeline.coord[unit]!=1)return 0;
    }
    if(!mask)return 0;
    prepared_texgen_units=mask;memcpy(prepared_texgen_post,post,sizeof(prepared_texgen_post));return 1;
}
int fh_renderer_texgens_unused(const FhDrawState *state) {
    const FhMaterial *plan=material(state,NULL,0,1);
    return plan && !plan->cpu_fragment && !plan->sphere_lighting && !plan->movie_layer &&
        (plan->pipeline.count?!plan->pipeline.textures:plan->map<0);
}
int fh_renderer_prepare_uvgen(const FhDrawState *state,const float matrix[3][4]) {
    const FhMaterial *plan=material(state,NULL,0,1);
    if(!plan || plan->cpu_fragment || plan->sphere_lighting || plan->movie_layer)return 0;
    unsigned units=plan->pipeline.count?plan->pipeline.textures:plan->map>=0?1:0;
    if(!units || units>3)return 0;
    for(unsigned i=0;i<units;i++)if((plan->pipeline.count?plan->pipeline.coord[i]:plan->coord)!=0)return 0;
    prepared_texgen_units=(1u<<units)-1;
    memcpy(prepared_texgen_post,matrix,sizeof(prepared_texgen_post));return 1;
}
const float (*fh_renderer_prepared_texgen(void))[4] {return prepared_texgen_post;}
static int native_geometry_key(NativeGeometryKey *key,const FhDrawState *state,const FhMaterial *plan) {
    if(!state->raw_position || !plan || plan->cpu_fragment || plan->sphere_lighting || plan->movie_layer)return 0;
    unsigned packing=affine_supported(plan)?1:plan->pipeline.count?2:0;
    memset(key,0,packing?offsetof(NativeGeometryKey,scalar):sizeof(*key));key->raw=state->raw_position;
    if(compiled_fur_gpu){
        /* The zero second-matrix family bakes a different sample coefficient.
         * Distinguish it even when all vertex and texture identities match. */
        key->fur=1|(fh_fur_gpu_zero_second(compiled_fur_source)<<1);key->fur_frame=frame_number;
        if(compiled_fur_post){key->fur_post_valid=1;memcpy(key->fur_post,compiled_fur_post,sizeof(key->fur_post));}
        for(unsigned i=0;i<2;i++){key->fur_textures[i]=compiled_fur_source->textures[compiled_fur_source->indirect_order[i][1]];fh_texture_identity(&key->fur_textures[i]);}
        memcpy(key->fur_order,compiled_fur_source->indirect_order,sizeof(key->fur_order));
        memcpy(key->fur_scale,compiled_fur_source->indirect_scale,sizeof(key->fur_scale));
    }
    /* Match only dependencies of emit_vertices. TEV constants and affine
     * coefficients are live GPU state: star twinkle must not rebuild centers.
     * Scalar color evaluation still requires the complete material key. */
    key->packing=packing;
    if(!packing)key->scalar=*plan;
    else if(packing==2)memcpy(key->primary,plan->pipeline.primary,sizeof(plan->pipeline.primary));
    key->points=point_context.active;
    key->normal_mode=(state->raw_position&2)?prepared_lighting.mask&FH_LIGHT_MODEL_NORMAL:0;
    unsigned units=plan->pipeline.count?plan->pipeline.textures:plan->map>=0?1:0;
    for(unsigned i=0;i<units;i++){
        unsigned map=plan->pipeline.count?plan->pipeline.map[i]:plan->map;
        key->coord[i]=plan->pipeline.count?plan->pipeline.coord[i]:plan->coord;
        if(!packing)key->scalar.pipeline.coord[i]=key->coord[i];
        key->width[i]=state->textures[map].width;key->height[i]=state->textures[map].height;
    }
    return 1;
}
static unsigned native_geometry_key_size(const NativeGeometryKey *key){return key->packing?offsetof(NativeGeometryKey,scalar):sizeof(*key);}
static NativeGeometry *native_geometry_oldest(void) {
    NativeGeometry *oldest=NULL;
    for(unsigned i=0;i<NATIVE_GEOMETRY_SLOTS;i++){
        NativeGeometry *e=native_geometry+i;
        if(e->storage && e!=native_replay && (!oldest || e->stamp<oldest->stamp))oldest=e;
    }
    return oldest;
}
static void native_geometry_retire(NativeGeometry *entry) {
    /* The key shares the fenced linear allocation with its GPU payload. */
    fh_native_geometry_key_bytes-=entry->key_bytes;
    linearFree(entry->storage);native_geometry_bytes-=entry->bytes;memset(entry,0,sizeof(*entry));
}
static void native_geometry_trim(unsigned required) {
    /* Texture correctness takes precedence over optional cached geometry.
     * A queued buffer cannot be freed until the owning queue is completed. */
    while(native_geometry_bytes && linearSpaceFree()<required+LINEAR_RESERVE){
        NativeGeometry *oldest=native_geometry_oldest();if(!oldest)break;
        if(oldest->epoch==native_geometry_epoch)fh_renderer_sync();
        native_geometry_retire(oldest);
    }
}
static NativeGeometry *native_geometry_find(uint64_t id,unsigned count,const NativeGeometryKey *key) {
    unsigned set=(unsigned)(id^(id>>8)^(id>>21))&(NATIVE_GEOMETRY_SLOTS/4-1);
    for(unsigned way=0;way<4;way++){
        NativeGeometry *e=native_geometry+set*4+way;
        if(e->id==id && e->count==count && e->storage && e->key_bytes==native_geometry_key_size(key) && !fh_memory_compare(e->key,key,e->key_bytes))return e;
    }
    return NULL;
}
int fh_renderer_replay_retained(uint64_t id,unsigned count,const FhDrawState *state) {
    if(!id || native_replay)return 0;
#ifdef FH_DRAW_CAPTURE
    if(fh_capture_draw_pending(frame_number,count,0))return 0;
#endif
    const FhMaterial *plan=material(state,NULL,0,1);NativeGeometryKey key;
    if(!native_geometry_key(&key,state,plan))return 0;
    NativeGeometry *entry=native_geometry_find(id,count,&key);if(!entry)return 0;
    /* The caller already verified all source bytes. Raster, textures, palette,
     * lighting and texgen uniforms still pass through the normal renderer. */
    native_replay=entry;fh_renderer_draw(NULL,count,state);native_replay=NULL;return 1;
}
void fh_renderer_draw_retained(uint64_t id,const FhVertex *v,unsigned count,const FhDrawState *state,const uint32_t *identities) {
    uint64_t previous=native_record_id;const FhVertex *old_input=native_record_input;
    const FhDrawState *old_state=native_record_state;unsigned old_count=native_record_count;
    native_record_id=id;native_record_input=v;native_record_state=state;native_record_count=count;
    fh_renderer_draw_identified(v,count,state,identities);
    native_record_id=previous;native_record_input=old_input;native_record_state=old_state;native_record_count=old_count;
}
int fh_renderer_replay_retained_points(uint64_t id,unsigned count,const FhDrawState *state,unsigned width,unsigned offsets,float texture_offset) {
    if(!width || !count)return 1;
    if(width>255 || point_context.active || !fh_renderer_points_available(state))return 0;
    point_context.active=1;point_context.width=width;point_context.offsets=offsets;point_context.texture_offset=texture_offset;
    int hit=fh_renderer_replay_retained(id,count,state);
    point_context.active=0;if(hit)fh_gpu_point_vertices+=count;return hit;
}
void fh_renderer_draw_retained_points(uint64_t id,const FhVertex *v,unsigned count,const FhDrawState *state,unsigned width,unsigned offsets,float texture_offset) {
    uint64_t previous=native_record_id;const FhVertex *old_input=native_record_input;
    const FhDrawState *old_state=native_record_state;unsigned old_count=native_record_count;
    native_record_id=id;native_record_input=v;native_record_state=state;native_record_count=count;
    fh_renderer_draw_points(v,count,state,width,offsets,texture_offset);
    native_record_id=previous;native_record_input=old_input;native_record_state=old_state;native_record_count=old_count;
}
static void native_geometry_record(const FhVertex *input,unsigned count,const FhDrawState *state,const FhMaterial *plan,unsigned first_vertex,unsigned first_index,unsigned first_barrier) {
    if(!native_record_id || native_replay || input!=native_record_input || state!=native_record_state || count!=native_record_count ||
       barriers!=first_barrier || index_count-first_index!=count)return;
    NativeGeometryKey key;if(!native_geometry_key(&key,state,plan))return;
    if(!compiled_fur_gpu && !compiled_staged)for(unsigned i=0;i<count;i++)if(input[i].matrix_index<0)return;
    unsigned unique=vertex_count-first_vertex,key_bytes=native_geometry_key_size(&key);
    unsigned vertex_offset=(key_bytes+31)&~31u;
    unsigned index_offset=(vertex_offset+unique*sizeof(GpuVertex)+31)&~31u,bytes=(index_offset+count*2+31)&~31u;
    if(!unique || bytes>NATIVE_GEOMETRY_BYTES)return;
    unsigned set=(unsigned)(native_record_id^(native_record_id>>8)^(native_record_id>>21))&(NATIVE_GEOMETRY_SLOTS/4-1);
    NativeGeometry *entry=NULL;
    for(unsigned way=0;way<4;way++){
        NativeGeometry *e=native_geometry+set*4+way;
        if(!e->storage){entry=e;break;}
        if(e->epoch!=native_geometry_epoch && (!entry || e->stamp<entry->stamp))entry=e;
    }
    if(!entry)return; /* Every way is still referenced by this GPU queue. */
    if(entry->storage)native_geometry_retire(entry);
    while(native_geometry_bytes+bytes>NATIVE_GEOMETRY_BYTES || linearSpaceFree()<bytes+LINEAR_RESERVE){
        NativeGeometry *oldest=native_geometry_oldest();
        if(!oldest || oldest->epoch==native_geometry_epoch)return;
        native_geometry_retire(oldest);
    }
    /* Melee keeps metadata in its fixed geometry directory. SFA's variable
     * material keys instead share the existing linear cache allocation:
     * admitting GPU payload must not depend on an additional ordinary-heap
     * allocation. Include the key/padding in the unchanged cache budget and
     * linear reserve; retire the whole allocation only after its GPU fence.
     * This does not relax the ordinary-heap reserve or enlarge either pool. */
    void *storage=linearMemAlign(bytes,32);if(!storage)return;
    NativeGeometryKey *owned_key=storage;
    memcpy(owned_key,&key,key_bytes);memcpy((unsigned char*)storage+vertex_offset,vertices+first_vertex,unique*sizeof(GpuVertex));
    u16 *out=(u16*)((unsigned char*)storage+index_offset);
    for(unsigned i=0;i<count;i++)out[i]=indices[first_index+i]-first_vertex;
    if(R_FAILED(GSPGPU_FlushDataCache(storage,bytes))){linearFree(storage);return;}
    *entry=(NativeGeometry){.id=native_record_id,.storage=storage,.key=owned_key,.key_bytes=key_bytes,.bytes=bytes,.vertex_offset=vertex_offset,.index_offset=index_offset,.count=count,.unique=unique,.epoch=0,.stamp=++native_geometry_clock};
    fh_native_geometry_key_bytes+=key_bytes;
    native_geometry_bytes+=bytes;
}
static void native_geometry_submit(const FhDrawState *state) {
    NativeGeometry *entry=native_replay;flush_draw();
    vertex_buffer((const GpuVertex*)((unsigned char*)entry->storage+entry->vertex_offset));
    C3D_DrawElements(point_context.active?GPU_GEOMETRY_PRIM:GPU_TRIANGLES,entry->count,C3D_UNSIGNED_SHORT,(unsigned char*)entry->storage+entry->index_offset);
    draws++;total_vertices+=entry->count;gpu_transform_vertices+=entry->unique;
    if(state->raw_position&2){gpu_lit_vertices+=entry->unique;gpu_light_evaluations+=entry->unique*prepared_light_count;
        if(prepared_lighting.mask&FH_LIGHT_MODEL_NORMAL)gpu_normal_vertices+=entry->unique;}
    if(state->raw_position&4)gpu_texgen_vertices+=entry->unique;
    entry->epoch=native_geometry_epoch;entry->stamp=++native_geometry_clock;native_geometry_hits++;native_geometry_vertices+=entry->count;
    vertex_buffer(vertices);
}
void fh_renderer_native_geometry_stats(unsigned *hits,unsigned *count,unsigned *bytes) {*hits=native_geometry_hits;*count=native_geometry_vertices;*bytes=native_geometry_bytes;}
static void pipeline_combiner(const FhPipeline *plan,Texture *const textures_in[3]) {
    static const GPU_TEVSRC sources[]={GPU_PRIMARY_COLOR,GPU_TEXTURE0,GPU_TEXTURE1,GPU_TEXTURE2,GPU_PREVIOUS,GPU_CONSTANT,GPU_PREVIOUS_BUFFER};
    static const GPU_COMBINEFUNC operations[]={GPU_REPLACE,GPU_MODULATE,GPU_ADD,GPU_ADD_SIGNED,GPU_INTERPOLATE,GPU_SUBTRACT,GPU_MULTIPLY_ADD};
    static const GPU_TEVOP_RGB rgb_ops[]={GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA,GPU_TEVOP_RGB_SRC_R,GPU_TEVOP_RGB_SRC_G,GPU_TEVOP_RGB_SRC_B};
    static const GPU_TEVOP_A alpha_ops[]={GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,GPU_TEVOP_A_SRC_G,GPU_TEVOP_A_SRC_B};
    for(unsigned i=0;i<6;i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvBufUpdate(C3D_RGB,plan->buffer_updates[0]);C3D_TexEnvBufUpdate(C3D_Alpha,plan->buffer_updates[1]);
    for(unsigned i=0;i<plan->count;i++) {
        const FhCombiner *stage=&plan->stages[i];C3D_TexEnv *env=C3D_GetTexEnv(i);u32 constant=0;
        for(unsigned c=0;c<4;c++) constant|=byte(stage->constant[c])<<(8*c);
        C3D_TexEnvColor(env,constant);
        for(unsigned alpha=0;alpha<2;alpha++) {
            const FhCombinerChannel *ch=&stage->channel[alpha];C3D_TexEnvMode mode=alpha?C3D_Alpha:C3D_RGB;
            C3D_TexEnvSrc(env,mode,sources[ch->source[0]],sources[ch->source[1]],sources[ch->source[2]]);
            unsigned operand[3];memcpy(operand,ch->operand,sizeof(operand));
            for(unsigned j=0;j<3;j++) if(ch->source[j]>=FH_TEXTURE0 && ch->source[j]<=FH_TEXTURE2 && (operand[j]&7)==1) {
                const Texture *t=textures_in[ch->source[j]-FH_TEXTURE0];
                if(t && t->mode!=3 && (t->source.format==0 || t->source.format==1)) operand[j]=(operand[j]&8)|2;
            }
            if(alpha) C3D_TexEnvOpAlpha(env,(GPU_TEVOP_A)(alpha_ops[operand[0]&7]+(operand[0]>>3)),(GPU_TEVOP_A)(alpha_ops[operand[1]&7]+(operand[1]>>3)),(GPU_TEVOP_A)(alpha_ops[operand[2]&7]+(operand[2]>>3)));
            else C3D_TexEnvOpRgb(env,(GPU_TEVOP_RGB)(rgb_ops[operand[0]&7]+(operand[0]>>3)),(GPU_TEVOP_RGB)(rgb_ops[operand[1]&7]+(operand[1]>>3)),(GPU_TEVOP_RGB)(rgb_ops[operand[2]&7]+(operand[2]>>3)));
            C3D_TexEnvFunc(env,mode,operations[ch->operation]);
            C3D_TexEnvScale(env,mode,ch->scale==4?GPU_TEVSCALE_4:ch->scale==2?GPU_TEVSCALE_2:GPU_TEVSCALE_1);
        }
    }
}
static void combiner(const FhMaterial *plan,const Texture *texture) {
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnv *env=C3D_GetTexEnv(0);
    if(plan->sphere_lighting) {
        C3D_TexEnvSrc(env,C3D_Both,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(env,C3D_Both,GPU_REPLACE);
        C3D_TexEnvScale(env,C3D_RGB,GPU_TEVSCALE_2);
        env=C3D_GetTexEnv(1);C3D_TexEnvSrc(env,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE0,0);C3D_TexEnvFunc(env,C3D_RGB,GPU_ADD);
        return;
    }
    u32 constant=0;
    for(unsigned c=0;c<4;c++) constant|=byte(plan->bias[c]/plan->scale[c==3])<<(8*c);
    C3D_TexEnvColor(env,constant);
    for(unsigned alpha=0;alpha<2;alpha++) {
        unsigned component=alpha?3:0;C3D_TexEnvMode mode=alpha?C3D_Alpha:C3D_RGB;
        C3D_TexEnvSrc(env,mode,GPU_PRIMARY_COLOR,plan->textured[component]?GPU_TEXTURE0:GPU_PRIMARY_COLOR,GPU_CONSTANT);
        int bias=alpha?plan->bias[3]!=0:plan->bias[0]!=0 || plan->bias[1]!=0 || plan->bias[2]!=0;
        C3D_TexEnvFunc(env,mode,!plan->textured[component]?GPU_REPLACE:bias?GPU_MULTIPLY_ADD:GPU_MODULATE);
        C3D_TexEnvScale(env,mode,plan->scale[alpha]==4?GPU_TEVSCALE_4:plan->scale[alpha]==2?GPU_TEVSCALE_2:GPU_TEVSCALE_1);
    }
    if(texture && (texture->source.format==0 || texture->source.format==1)) C3D_TexEnvOpAlpha(env,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,GPU_TEVOP_A_SRC_ALPHA);
}
typedef struct { const FhDrawState *state;const FhMaterial *plan;Texture *textures[3];unsigned coord[3],batch_start,affine;const float (*uv7)[3];const uint32_t *identities;float uv_scale[3][2],sx,sy,ox,oy,sz,oz;FhVertexReuse reuse; } VertexSink;
static void submit_vertices(VertexSink *sink) {
    unsigned count=index_count-sink->batch_start;
    if(!count) return;
    if(pending_count && pending_start+pending_count!=sink->batch_start)flush_draw();
    if(!pending_count)pending_start=sink->batch_start;
    pending_count+=count;sink->batch_start=index_count;
}
static void emit_vertices(const FhVertex *input,unsigned count,void *context) {
    VertexSink *sink=context;const FhMaterial *plan=sink->plan;
    if(vertex_count+count>VERTICES || index_count+count>VERTICES) {
        submit_vertices(sink);fh_renderer_sync();configure_transform(sink->state);configure_affine(plan);
        if(point_context.active)C3D_FVUnifSet(GPU_GEOMETRY_SHADER,0,point_parameters[0],point_parameters[1],point_parameters[2],point_parameters[3]);
        sink->batch_start=0;memset(&sink->reuse,0,sizeof(sink->reuse));
    }
    for(unsigned i=0;i<count;i++) {
        const FhVertex *s=&input[i];unsigned index=vertex_count;
#ifdef FH_EMULATOR_TEST
        if(!fh_vertex_reference)
#endif
        if(sink->identities)index=fh_vertex_reuse(&sink->reuse,sink->identities[i],vertex_count);
        indices[index_count++]=index;
        if(index!=vertex_count){
            ++fh_vertex_reused;
            continue;
        }
        ++fh_vertex_unique;
        GpuVertex *v=&vertices[vertex_count++];float w=s->position[3];
        v->matrix_index=sink->state->raw_position?s->matrix_index:-1;
        v->generated_normal_uv=0;
        if(sink->state->raw_position&2){if(v->matrix_index>=0){gpu_lit_vertices++;gpu_light_evaluations+=prepared_light_count;}else gpu_lighting_fallback_vertices++;}
        if((sink->state->raw_position&2) && (prepared_lighting.mask&FH_LIGHT_MODEL_NORMAL) && v->matrix_index>=0)gpu_normal_vertices++;
        if(v->matrix_index>=0){
            if(v->matrix_index>27 || ((unsigned)v->matrix_index)%3)fh_platform_panic("GPU matrix palette index invalid");
            memcpy(v->position,s->position,sizeof(v->position));gpu_transform_vertices++;
        }
        else {v->position[0]=s->position[1]*sink->sy+w*sink->oy;v->position[1]=-(s->position[0]*sink->sx+w*sink->ox);
        v->position[2]=s->position[2]*sink->sz+w*sink->oz;v->position[3]=w;}
        if(sink->affine)memcpy(v->color,s->color[0],sizeof(v->color));
        else if(plan->pipeline.count) for(unsigned c=0;c<4;c++) v->color[c]=plan->pipeline.primary[c]<0?0:((const float*)s->color)[plan->pipeline.primary[c]];
        else fh_material_color(v->color,plan,s->color);
        if(sink->state->raw_position&16)memcpy(v->normal,s->uv[0],sizeof(v->normal));
        else if(sink->state->raw_position&2)memcpy(v->normal,s->uv[7],sizeof(v->normal));else memset(v->normal,0,sizeof(v->normal));
        memset(v->uv,0,sizeof(v->uv));
        for(unsigned unit=0;unit<3;unit++) if(sink->textures[unit]) {
            unsigned coord=sink->coord[unit];
            const float *uv=coord==7 && sink->uv7?sink->uv7[i]:s->uv[coord];
            float q=uv[2];if(fabsf(q)<1e-20f) q=1;
            float u=uv[0],t=uv[1];
            if(unit){u/=q;t/=q;}
            v->uv[unit][0]=u*sink->uv_scale[unit][0];
            v->uv[unit][1]=(unit?1:q)-t*sink->uv_scale[unit][1];
            v->uv[unit][2]=unit?1:q;
        }
        if((sink->state->raw_position&4) && s->uv[6][0] && v->matrix_index>=0){
            gpu_texgen_vertices++;
            v->generated_normal_uv=(sink->state->raw_position&16)?-1:1;
            for(unsigned unit=0;unit<3;unit++)if(prepared_texgen_units&(1u<<unit)){
                v->uv[unit][0]=sink->uv_scale[unit][0];v->uv[unit][1]=sink->uv_scale[unit][1];
            }
        }
        if(sink->affine) {
            memcpy(v->uv[1],s->color[1],3*sizeof(float));v->uv[2][0]=s->color[1][3];
        }
        if(compiled_fur_gpu){
            v->generated_normal_uv=2;
            float q=s->uv[4][2];
            v->uv[1][2]=compiled_fur_storage->uv[i][0]*q*sink->uv_scale[0][0];
            v->uv[2][2]=compiled_fur_storage->uv[i][1]*q*sink->uv_scale[0][1];
        }
    }
}
static void emit_triangle(const FhVertex input[3],void *context){emit_vertices(input,3,context);}
static int emit_native_compiled(VertexSink *sink){
    if(!compiled_native || !compiled_mesh->decode_native || (!sink->affine && !sink->plan->pipeline.count))return 0;
    FhNativePacking packing={.units=sink->plan->pipeline.count?sink->plan->pipeline.textures:sink->plan->map>=0?1:0,
        .generated_units=(sink->state->raw_position&4)?prepared_texgen_units:0,
        .affine=sink->affine,.sphere=compiled_sphere && !(sink->state->raw_position&64)};
    memcpy(packing.primary,sink->plan->pipeline.primary,sizeof(packing.primary));
    memcpy(packing.coord,sink->coord,sizeof(packing.coord));
    memcpy(packing.scale,sink->uv_scale,sizeof(packing.scale));
    unsigned result=compiled_mesh->decode_native(vertices+vertex_count,compiled_mesh,&packing);
    if(!result)return 0;
    unsigned n=compiled_mesh->unique;
    if(result==2){
        /* Compare against the real scalar decoder and the real renderer sink,
         * not another copy of the candidate's arithmetic. Nothing is submitted
         * until every candidate vertex has passed this byte comparison. */
        for(unsigned i=0;i<n;i++){
            GpuVertex candidate=vertices[vertex_count];FhVertex reference;
            compiled_decode(&reference,compiled_mesh->packets+compiled_mesh->offsets[i]);
            if(packing.sphere)for(unsigned j=0;j<2;j++){
                reference.uv[6+j][0]=(.5f+15*reference.color[1][j*2])/16;
                reference.uv[6+j][1]=(.5f+15*reference.color[1][j*2+1])/16;
                reference.uv[6+j][2]=1;
            }
            emit_vertices(&reference,1,sink);
            if(memcmp(&candidate,vertices+vertex_count-1,sizeof(candidate)))fh_platform_panic("Fused native vertex differs from scalar renderer");
            fh_native_packing_checks++;
        }
    }else{
        vertex_count+=n;fh_vertex_unique+=n;gpu_transform_vertices+=n;
        if(sink->state->raw_position&2){
            gpu_lit_vertices+=n;gpu_light_evaluations+=n*prepared_light_count;
            if(prepared_lighting.mask&FH_LIGHT_MODEL_NORMAL)gpu_normal_vertices+=n;
        }
        if(sink->state->raw_position&4)gpu_texgen_vertices+=n;
    }
    return 1;
}
static int emit_compact_compiled(VertexSink *sink){
#ifdef FH_COMPACT_VERTEX_BUILD
    if(!compiled_compact || !compiled_mesh->decode_compact || sink->affine || !sink->plan->pipeline.count)return 0;
    FhNativePacking packing={.units=sink->plan->pipeline.textures,
        .generated_units=(sink->state->raw_position&4)?prepared_texgen_units:0};
    memcpy(packing.primary,sink->plan->pipeline.primary,sizeof(packing.primary));
    memcpy(packing.coord,sink->coord,sizeof(packing.coord));memcpy(packing.scale,sink->uv_scale,sizeof(packing.scale));
    unsigned first=vertex_count,n=compiled_mesh->unique;
    FhCompactVertex *output=(FhCompactVertex*)(vertices+first);
    unsigned result=compiled_mesh->decode_compact(output,compiled_mesh,&packing,&compact_fixed);
    if(!result)return 0;
    if(result==2){
        /* Last reserved ordinary vertex is beyond the compact payload. */
        if(n<3 || (n-1)*sizeof(GpuVertex)<n*sizeof(FhCompactVertex))fh_platform_panic("Compact validation scratch overlaps payload");
        for(unsigned i=0;i<n;i++){
            GpuVertex candidate;FhVertex reference;fh_compact_expand(&candidate,output+i,&compact_fixed);
            compiled_decode(&reference,compiled_mesh->packets+compiled_mesh->offsets[i]);
            vertex_count=first+n-1;emit_vertices(&reference,1,sink);
            if(memcmp(&candidate,vertices+first+n-1,sizeof(candidate)))fh_platform_panic("Compact vertex differs from scalar renderer");
            fh_compact_packing_checks++;
        }
    }else{
        fh_vertex_unique+=n;gpu_transform_vertices+=n;gpu_lit_vertices+=n;gpu_light_evaluations+=n*prepared_light_count;
        if(prepared_lighting.mask&FH_LIGHT_MODEL_NORMAL)gpu_normal_vertices+=n;
        if(sink->state->raw_position&4)gpu_texgen_vertices+=n;
    }
    vertex_count=first+n;return 1;
#else
    (void)sink;return 0;
#endif
}
void fh_renderer_service_audio(void){service_audio();}
static float clip_distance(const FhVertex *v,unsigned plane) {
    const float *p=v->position;
    return plane==0?p[3]+p[0]:plane==1?p[3]-p[0]:plane==2?p[3]+p[1]:plane==3?p[3]-p[1]:plane==4?p[3]+p[2]:-p[2];
}
static float edge(float ax,float ay,float bx,float by,float x,float y) { return (bx-ax)*(y-ay)-(by-ay)*(x-ax); }
unsigned fh_fragment_batch_enabled=1,fh_fragment_batch_submissions,fh_fragment_batch_flushes;
/* A batch owns only immutable, disjoint atlas tiles. Drain before reuse or any
 * intervening draw; keep original primitive order and all per-triangle shading.
 * 63 vertices add <10KiB bounded stack storage, no new linear/VRAM allocation. */
typedef struct {FhVertex vertices[63];unsigned count,mode,first,last;} FragmentBatch;
static void fragment_batch_flush(FragmentBatch *batch,const FhDrawState *state){
    if(!batch->count)return;
    if(R_FAILED(GSPGPU_FlushDataCache((u8*)fragment_atlas.gpu.data+batch->first,batch->last-batch->first)))
        fh_platform_panic("Fragment atlas batch cache flush failed");
    fh_fragment_batch_flushes++;
    FhDrawState draw=*state;draw.stages=1;draw.indirect_count=0;draw.textures[0]=fragment_atlas.source;
    draw.tev[0]=(FhTevStage){.color={15,8,12,15},.alpha={7,4,6,7},.color_clamp=1,.alpha_clamp=1,.channel=255};
    for(unsigned c=0;c<4;c++)draw.swaps[0][c]=c;
    if(batch->mode==2){
        FhMaterial plan;fh_fur_pipeline(&plan,state);draw.textures[1]=state->textures[0];draw.textures[2]=state->textures[1];
        render_direct(batch->vertices,batch->count,&draw,&plan);
    }else fh_renderer_draw(batch->vertices,batch->count,&draw);
    fh_fragment_batch_submissions++;batch->count=0;
}
#ifdef FH_EMULATOR_TEST
/* Captured-workload oracle: sample at the GPU's horizontal pixel centers,
 * rather than comparing its native image to the legacy 320-wide CPU atlas. */
unsigned fh_fragment_native_grid;
#endif
static void fragment_triangle(const FhVertex input[3],const FhDrawState *state,unsigned mode,const float matrix[2][2][3],FhFragmentProgram *program,unsigned *prepared,FragmentBatch *batch) {
    service_audio();
    unsigned raster_width=320;
#ifdef FH_EMULATOR_TEST
    if(fh_fragment_native_grid)raster_width=400;
#endif
    if(mode==2)for(unsigned i=1;i<3;i++)if(input[i].uv[0][2]!=input[0].uv[0][2] || input[i].uv[2][2]!=input[0].uv[2][2])mode=1;
    float screen[3][2];
    for(unsigned i=0;i<3;i++) {
        float w=input[i].position[3];if(w<=1e-10f) return;
        screen[i][0]=(state->viewport[0]+(input[i].position[0]/w+1)*state->viewport[2]*.5f)*(raster_width/640.f);
        screen[i][1]=(state->viewport[1]+(1-input[i].position[1]/w)*state->viewport[3]*.5f)*.5f;
    }
    float area=edge(screen[0][0],screen[0][1],screen[1][0],screen[1][1],screen[2][0],screen[2][1]);if(fabsf(area)<1e-6f)return;
    if(state->cull==3)return;
    float winding_error=.25f;
    for(unsigned i=0;i<3;i++)for(unsigned axis=0;axis<2;axis++)winding_error+=.25f*fabsf(screen[i][axis]-screen[(i+1)%3][axis]);
    if((state->cull==1 && area>winding_error) || (state->cull==2 && area<-winding_error))return;
    int left=raster_width,right=0,top=FH_EFB_NATIVE_HEIGHT,bottom=0;
    for(unsigned i=0;i<3;i++){left=fminf(left,floorf(screen[i][0]));right=fmaxf(right,ceilf(screen[i][0]));top=fminf(top,floorf(screen[i][1]));bottom=fmaxf(bottom,ceilf(screen[i][1]));}
    if(left<0)left=0;
    if(top<0)top=0;
    if(right>(int)raster_width)right=raster_width;
    if(bottom>FH_EFB_NATIVE_HEIGHT)bottom=FH_EFB_NATIVE_HEIGHT;
    if(left>=right || top>=bottom)return;
    unsigned box_pixels=(right-left)*(bottom-top);
    if(mode==2 && fur_mode() && box_pixels>=64) {
        unsigned capacity=box_pixels/8*3;if(capacity>192)capacity=192;
        u64 start=svcGetSystemTick();
        FhVertex mesh[192];unsigned n=fh_fur_mesh(mesh,capacity,input,state,matrix);
        mesh_ticks+=svcGetSystemTick()-start;
        if(n) {
            fragment_batch_flush(batch,state);
            FhMaterial plan;fh_fur_mesh_pipeline(&plan,state);FhDrawState draw=*state;
            draw.textures[0]=state->textures[3];draw.textures[1]=state->textures[0];draw.textures[2]=state->textures[1];
            draw.textures[3]=state->textures[2];
            render_direct(mesh,n,&draw,&plan);mesh_triangles+=n/3;return;
        }
        mesh_fallbacks++;
    }
    unsigned width=right-left,height=bottom-top,padded_width=(width+7)&~7u,padded_height=(height+7)&~7u;
    if(!fragment_atlas.bytes) {
        while(texture_bytes+512*512*4>TEXTURE_CACHE_LIMIT) {
            Texture *oldest=NULL;for(unsigned i=0;i<TEXTURES;i++)if(textures[i].bytes && (!oldest || textures[i].frame<oldest->frame))oldest=&textures[i];
            if(!oldest)fh_platform_panic("Fragment atlas texture budget");
            fh_renderer_sync();retire(oldest);
        }
        reserve_texture_linear(512*512*4);
        texture_init_reclaim(&fragment_atlas.gpu,512,512,GPU_RGBA8);
        fragment_atlas.bytes=512*512*4;texture_bytes+=fragment_atlas.bytes;
        fragment_atlas.source=(FhTexture){.pixels=fragment_atlas.gpu.data,.width=512,.height=512,.format=0x46};
    }
    if(atlas_x+padded_width>512){atlas_x=0;atlas_y+=atlas_row;atlas_row=0;}
    if(atlas_y+padded_height>512){fragment_batch_flush(batch,state);fh_renderer_sync();atlas_x=atlas_y=atlas_row=0;}
    if(batch->count && (batch->mode!=mode || batch->count==63))fragment_batch_flush(batch,state);
    if(!fragment_pixels_cpu)fragment_pixels_cpu=malloc(raster_width*FH_EFB_NATIVE_HEIGHT*4);
    if(!fragment_pixels_cpu)fh_platform_panic("CPU fragment staging allocation failed");
    /* One immutable material spans this draw's clipped triangles and row
     * batches. Prepare lazily after culling, then borrow until worker completion. */
    if(mode!=2 && !*prepared){fh_fragment_prepare(program,state);*prepared=1;}
    FhFragmentRaster job={.input=input,.state=state,.program=*prepared?program:NULL,.matrix=matrix,.screen=screen,.area=area,
        .left=left,.top=top,.width=width,.height=height,.mode=mode,.pixels=fragment_pixels_cpu};
    fh_fragment_worker_run(&job,service_audio);fragment_pixels+=job.evaluated;
    for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++) {
        unsigned tx=atlas_x+x,ty=atlas_y+y;
        unsigned morton=(tx&1)|((ty&1)<<1)|((tx&2)<<1)|((ty&2)<<2)|((tx&4)<<2)|((ty&4)<<3);
        ((u32*)fragment_atlas.gpu.data)[((ty/8)*64+tx/8)*64+morton]=fragment_pixels_cpu[y*width+x];
    }
    /* Tiles are cache-line disjoint; flushing the union can include older clean
     * tiles that the GPU only reads. No atlas content is overwritten here. */
    unsigned first=((atlas_y/8)*64+atlas_x/8)*256;
    unsigned last=(((atlas_y+padded_height-8)/8)*64+(atlas_x+padded_width)/8)*256;
    if(!batch->count){batch->mode=mode;batch->first=first;batch->last=last;}
    else {if(first<batch->first)batch->first=first;if(last>batch->last)batch->last=last;}
    FhVertex *v=batch->vertices+batch->count;memcpy(v,input,3*sizeof(*v));
    for(unsigned i=0;i<3;i++) {
        unsigned coord=mode==2?7:0;
        float w=v[i].position[3];v[i].uv[coord][0]=(screen[i][0]-left+atlas_x)*w/512;v[i].uv[coord][1]=(screen[i][1]-top+atlas_y)*w/512;v[i].uv[coord][2]=w;
        if(mode==1)memset(v[i].color,0,sizeof(v[i].color));
    }
    atlas_x+=padded_width;if(padded_height>atlas_row)atlas_row=padded_height;fragment_triangles++;
    batch->count+=3;
    if(!fh_fragment_batch_enabled)fragment_batch_flush(batch,state);
}
static int fur_vertex_draw(const FhVertex *input,unsigned count,const FhDrawState *state,const float matrix[2][2][3]) {
    if(fur_mode()==2 && count<=384) {
        float output[384][3];u64 start=svcGetSystemTick();
        const uint32_t *identities=NULL;
        uintptr_t offset=(uintptr_t)input-(uintptr_t)identified.vertices;
        if(identified.identities && offset<(size_t)identified.count*sizeof(*input) &&
           offset%sizeof(*input)==0 && count<=identified.count-offset/sizeof(*input))
            identities=identified.identities+offset/sizeof(*input);
#ifdef FH_EMULATOR_TEST
        if(fh_vertex_reference)identities=NULL;
#endif
        unsigned used=fh_fur_coordinates_identified(output,input,count,state,matrix,identities);
        mesh_ticks+=svcGetSystemTick()-start;
        if(used==count) {
            FhMaterial plan;fh_fur_mesh_pipeline(&plan,state);FhDrawState draw=*state;
            draw.textures[0]=state->textures[3];draw.textures[1]=state->textures[0];draw.textures[2]=state->textures[1];draw.textures[3]=state->textures[2];
            render_direct_coordinates(input,count,&draw,&plan,output);mesh_triangles+=count/3;return 1;
        }
    }
    return 0;
}
static unsigned clip_fragment_triangle(FhVertex buffers[2][12],const FhVertex input[3]) {
    memcpy(buffers[0],input,3*sizeof(*input));unsigned n=3,current=0;
    for(unsigned plane=0;plane<6 && n;plane++) {
        FhVertex *in=buffers[current],*out=buffers[1-current];unsigned next=0;
        for(unsigned i=0;i<n;i++) {
            const FhVertex *a=&in[i],*b=&in[(i+1)%n];float da=clip_distance(a,plane),db=clip_distance(b,plane);
            if(da>=0)out[next++]=*a;
            if((da>=0)!=(db>=0)) {
                float t=da/(da-db);for(unsigned c=0;c<36;c++)((float*)&out[next])[c]=((const float*)a)[c]+t*(((const float*)b)[c]-((const float*)a)[c]);
                out[next++].matrix_index=-1;
            }
        }
        n=next;current=1-current;
    }
    if(current)memcpy(buffers[0],buffers[current],n*sizeof(FhVertex));
    return n;
}
static unsigned chained_batch_append(FhVertex output[384],unsigned *pending,const FhVertex input[3],const FhDrawState *state,const FhMaterial *plan) {
    unsigned n=fh_chained_triangle_append(output,*pending,input,state);
    if(n>384 && *pending){
        /* An overflowing suffix never modifies the already accepted prefix.
         * Flush that prefix before retrying with the same bounded storage. */
        render_direct(output,*pending,state,plan);*pending=0;
        n=fh_chained_triangle_append(output,0,input,state);
    }
    if(n && n<=384)*pending=n;
    return n;
}
unsigned fh_renderer_texgen_inputs(const FhDrawState *state) {
    unsigned all=state->texgens>=8?255u:(1u<<state->texgens)-1u;
    const FhMaterial *plan=material(state,NULL,0,1);
    /* Generated sphere coordinates and CPU fragment programs have additional
     * consumers outside the ordinary material. Retain their complete inputs. */
    if(!plan || plan->cpu_fragment || plan->sphere_lighting || plan->movie_layer)return all;
    unsigned mask=0,units=plan->pipeline.count?plan->pipeline.textures:plan->map>=0?1:0;
    for(unsigned i=0;i<units;i++){
        int coord=plan->pipeline.count?plan->pipeline.coord[i]:plan->coord;
        if(coord<0 || coord>=(int)state->texgens)return all;
        mask|=1u<<coord;
    }
    return mask;
}
typedef struct {FhVertex vertices[63];unsigned count;const FhDrawState *state;const FhMaterial *plan;} IndirectLightingBatch;
static void indirect_lighting_sink(const FhVertex triangle[3],void *context){
    IndirectLightingBatch *b=context;memcpy(b->vertices+b->count,triangle,3*sizeof(*triangle));b->count+=3;
    if(b->count==63){render_direct(b->vertices,b->count,b->state,b->plan);b->count=0;}
}
static void fragment_draw(const FhVertex *input,unsigned count,const FhDrawState *state,unsigned mode);
static __attribute__((noinline)) int indirect_lighting_draw(const FhVertex *input,unsigned count,const FhDrawState *state){
    static unsigned active;
    if(!fh_indirect_lighting_enabled || active)return 0;
    FhMaterial plan;FhDrawState draw;FhIndirectLighting prepared;
    if(!fh_indirect_lighting_plan(&plan,&draw,state) || !fh_indirect_lighting_prepare(&prepared,state))return 0;
    IndirectLightingBatch batch={.state=&draw,.plan=&plan};active=1;
    for(unsigned i=0;i<count;i+=3){
        FhVertex clipped[2][12];unsigned n=clip_fragment_triangle(clipped,input+i);
        for(unsigned j=1;j+1<n;j++){
            FhVertex tri[3]={clipped[0][0],clipped[0][j],clipped[0][j+1]};
            if(!fh_indirect_lighting_mesh_prepared(tri,state,&prepared,indirect_lighting_sink,&batch)){
                if(batch.count){render_direct(batch.vertices,batch.count,&draw,&plan);batch.count=0;}
                fragment_draw(tri,3,state,1);
            }
        }
    }
    if(batch.count)render_direct(batch.vertices,batch.count,&draw,&plan);
    active=0;return 1;
}
static void fragment_draw(const FhVertex *input,unsigned count,const FhDrawState *state,unsigned mode) {
    static unsigned chained_active;
    static unsigned reflective_active;
    if(mode==1 && !reflective_active && fh_reflective_material(state)){
        FhMaterial plans[5];for(unsigned pass=0;pass<5;pass++)fh_reflective_pipeline(plans+pass,state,pass);
        FhVertex pending[63];unsigned pending_count=0;
        reflective_active=1;
        for(unsigned i=0;i<count;i+=3){
            FhVertex clipped[2][12];unsigned n=clip_fragment_triangle(clipped,input+i);
            for(unsigned j=1;j+1<n;j++){
                FhVertex tri[3]={clipped[0][0],clipped[0][j],clipped[0][j+1]};
                if(!fh_reflective_triangle(tri,state)){
                    fh_reflective_fallbacks++;
                    if(!fh_fragment_batch_enabled){fragment_draw(tri,3,state,mode);continue;}
                    memcpy(pending+pending_count,tri,sizeof(tri));pending_count+=3;
                    if(pending_count==63){fragment_draw(pending,pending_count,state,mode);pending_count=0;}
                    continue;
                }
                if(pending_count){fragment_draw(pending,pending_count,state,mode);pending_count=0;}
                fh_reflective_triangles++;
                FhDrawState draw=*state;draw.fog_type=0;
                render_direct(tri,3,&draw,plans);
                draw.depth_write=0;draw.depth_compare=2;draw.blend_mode=1;draw.blend_src=draw.blend_dst=1;draw.alpha_write=0;
                render_direct(tri,3,&draw,plans+1);render_direct(tri,3,&draw,plans+2);
                if(state->fog_type&7){
                    draw.fog_type=state->fog_type;draw.fog_color=0;
                    draw.blend_src=0;draw.blend_dst=2; /* destination *= F */
                    render_direct(tri,3,&draw,plans+3);
                    draw.fog_color=state->fog_color;draw.blend_src=draw.blend_dst=1;
                    render_direct(tri,3,&draw,plans+4);
                }
            }
        }
        if(pending_count)fragment_draw(pending,pending_count,state,mode);
        reflective_active=0;return;
    }
    if(mode==1 && !fh_chained_disabled && !chained_active && fh_chained_material(state)){
        FhVertex output[384];unsigned pending=0;FhMaterial plan;fh_chained_pipeline(&plan,state);chained_active=1;
        for(unsigned i=0;i<count;i+=3){
            unsigned n=chained_batch_append(output,&pending,input+i,state,&plan);
            if(n && n<=384)continue;
            if(n>384 && !fh_chained_reference_bounds){
                /* Retry an oversized valid triangle as four disjoint children.
                 * Keep the existing 384-vertex scratch/stream contract, with
                 * at most four chunks. Unresolved children retain scalar
                 * shading; never submit a partially generated triangle. */
                FhVertex mid[3],child[3];
                for(unsigned edge=0;edge<3;edge++)for(unsigned c=0;c<sizeof(FhVertex)/sizeof(float);c++)
                    ((float*)&mid[edge])[c]=.5f*(((const float*)&input[i+edge])[c]+((const float*)&input[i+(edge+1)%3])[c]);
                for(unsigned part=0;part<4;part++){
                    if(part<3){child[0]=input[i+part];child[1]=mid[part];child[2]=mid[(part+2)%3];}
                    else memcpy(child,mid,sizeof(child));
                    n=chained_batch_append(output,&pending,child,state,&plan);
                    if(!n || n>384){
                        if(pending){render_direct(output,pending,state,&plan);pending=0;}
                        fragment_draw(child,3,state,mode);
                    }
                }
            }else{
                if(pending){render_direct(output,pending,state,&plan);pending=0;}
                fragment_draw(input+i,3,state,mode);
            }
        }
        if(pending)render_direct(output,pending,state,&plan);
        chained_active=0;return;
    }
    if(mode==1 && fh_blur_material(state) && fh_blur_quad(input,count,state)){
        FhDrawState draw=*state;FhMaterial plan;
        for(unsigned tap=0;tap<5;tap++){
            fh_blur_pipeline(&plan,state,tap);render_direct(input,count,&draw,&plan);
            /* Preserve destination alpha from the first ordinary blend. */
            draw.blend_dst=1;draw.alpha_write=0;
        }
        return;
    }
    /* Preserve the established direct GPU translations above. The general
     * vertex approximation is useful only where those paths still require
     * CPU fragments; replacing their cheap accepted draws can be slower. */
    if(mode==1 && indirect_lighting_draw(input,count,state))return;
    float matrix[2][2][3];
    for(unsigned i=0;i<2;i++)for(unsigned axis=0;axis<2;axis++)for(unsigned c=0;c<3;c++)matrix[i][axis][c]=ldexpf(state->indirect_matrix[i][axis][c],state->indirect_exponent[i]);
    if(mode==2 && fur_vertex_draw(input,count,state,matrix))return;
    FhFragmentProgram program;unsigned prepared=0;FragmentBatch batch;batch.count=0;
    for(unsigned triangle=0;triangle<count;triangle+=3) {
        FhVertex buffers[2][12];unsigned n=clip_fragment_triangle(buffers,input+triangle);
        for(unsigned i=1;i+1<n;i++){FhVertex v[3]={buffers[0][0],buffers[0][i],buffers[0][i+1]};fragment_triangle(v,state,mode,matrix,&program,&prepared,&batch);}
    }
    fragment_batch_flush(&batch,state);
}
static __attribute__((noinline)) void transform_cpu_draw(const FhVertex *input,unsigned count,const FhDrawState *state) {
    FhVertex transformed[384];FhDrawState cpu=*state;cpu.raw_position=0;
    cpu_transform_fallback_vertices+=count;
    for(unsigned offset=0;offset<count;offset+=384){
        unsigned n=count-offset;if(n>384)n=384;
        memcpy(transformed,input+offset,n*sizeof(*input));
        for(unsigned i=0;i<n;i++)if(input[offset+i].matrix_index>=0){
            unsigned index=input[offset+i].matrix_index;if(index>27 || index%3)fh_platform_panic("CPU fallback palette index invalid");
            fh_gpu_texgen_resolve(&transformed[i],state,prepared_texgen_post);
            fh_gpu_lighting_resolve(&transformed[i],state,&prepared_lighting);
            fh_vertex_clip_position(transformed[i].position,input[offset+i].position,index,state);
            transformed[i].matrix_index=-1;
        }
        fh_renderer_draw(transformed,n,&cpu);
    }
}
int fh_renderer_cpu_cull_available(const FhDrawState *state) {
    /* The scalar and adaptive-mesh routes clip on ARM before rasterization.
     * Vertex fur goes directly to PICA and requires both visibility volumes. */
    return fur_mode()!=2 && state->raw_position==1 && fh_fur_material(state) && fh_fragment_supported(state);
}
int fh_renderer_gpu_cull_available(const FhDrawState *state) {
    /* Both GX and PICA exclusion cover individually CPU-transformed lighting
     * and fur fallbacks. Fur changes texture coordinates, never positions. */
    return state->raw_position&1;
}
int fh_renderer_points_available(const FhDrawState *state) {
    if(fh_gpu_points_disabled || state->texgens>1 || !(state->viewport[2]>0) || !(state->viewport[3]>0) ||
       state->viewport[4]!=0 || state->viewport[5]!=1)return 0;
#ifdef FH_DRAW_CAPTURE
    /* Draw-capture files currently describe triangles. Preserve that oracle. */
    if(fh_capture_draw_pending(frame_number,384,0))return 0;
#endif
    const FhMaterial *plan=material(state,NULL,0,1);
    if(!plan || plan->sphere_lighting || plan->cpu_fragment || plan->movie_layer)return 0;
    if(plan->pipeline.count)return plan->pipeline.textures<=1 &&
        (!plan->pipeline.textures || (!plan->pipeline.texture_mode[0] && plan->pipeline.coord[0]==0));
    return plan->map<0 || plan->coord==0;
}
void fh_renderer_draw_points(const FhVertex *input,unsigned count,const FhDrawState *state,unsigned width,unsigned offsets,float texture_offset) {
    if(!width || !count)return;
    if(width>255 || !fh_renderer_points_available(state) || point_context.active)
        fh_platform_panic("Invalid native point batch");
    /* At maximum textured size, full-width projection exposes different
     * float24 corner rounding between GS expansion and the scalar reference.
     * A nearest-sampled texel boundary can then select the adjacent texel.
     * Retain the original six-vertex expansion for that exceptional size. */
    unsigned fallback=state->texgens && width==255;
    if(state->texgens)for(unsigned i=0;i<count;i++)
        if(!isfinite(input[i].uv[0][2]) || fabsf(input[i].uv[0][2])<1e-20f)fallback=1;
    if(fallback){
        /* Packing substitutes q=1 for a degenerate projective coordinate,
         * while GX point offsets still multiply the original q. Preserve
         * that distinction with the scalar expansion for exceptional batches. */
        FhDrawState cpu=*state;cpu.raw_position=0;cpu.cull=0;
        const unsigned corners[]={0,1,2,2,1,3};
        for(unsigned i=0;i<count;i++){
            FhVertex v=input[i],quad[4],triangles[6];
            if(state->raw_position && v.matrix_index>=0){
                fh_gpu_texgen_resolve(&v,state,prepared_texgen_post);
                fh_gpu_lighting_resolve(&v,state,&prepared_lighting);
                fh_vertex_clip_position(v.position,v.position,(unsigned)v.matrix_index,state);v.matrix_index=-1;
            }
            if(fh_expand_point(&v,width,offsets,texture_offset,state->viewport[2],state->viewport[3],quad)){
                for(unsigned n=0;n<6;n++)triangles[n]=quad[corners[n]];
                fh_renderer_draw(triangles,6,&cpu);
            }
        }
        return;
    }
    point_context.active=1;point_context.width=width;
    point_context.offsets=offsets;point_context.texture_offset=texture_offset;
    fh_renderer_draw(input,count,state);fh_gpu_point_vertices+=count;
    point_context.active=0;
}
static __attribute__((noinline)) int sphere_draw(const FhVertex *input,unsigned count,const FhDrawState *state) {
    FhVertex output[384];FhDrawState draw;FhMaterial plan;
    if(count>384 || !fh_sphere_prepare(&plan,&draw,output,state,input,count))return 0;
    render_direct(output,count,&draw,&plan);return 1;
}
void fh_renderer_draw_identified(const FhVertex *input,unsigned count,const FhDrawState *state,const uint32_t *identities) {
    __typeof__(identified) previous=identified;
    identified.vertices=input;identified.identities=identities;identified.count=count;
    fh_renderer_draw(input,count,state);identified=previous;
}
void fh_renderer_draw(const FhVertex *input,unsigned count,const FhDrawState *state) {
    if(fh_render_floor)return;
    service_audio();
    if(!count) return;
    if(!point_context.active && count%3) fh_platform_panic("Renderer requires complete triangles");
    begin();reserve_commands();if(count>VERTICES) fh_platform_panic("Draw exceeds bounded vertex storage");
    unsigned outside=(state->raw_position || point_context.active)?0:63;
    for(unsigned i=0;i<count && outside;i++) {
        const float *p=input[i].position;
        unsigned mask=(p[0]<-p[3]?1u:0)|(p[0]>p[3]?2u:0)|(p[1]<-p[3]?4u:0)|(p[1]>p[3]?8u:0)|(p[2]<-p[3]?16u:0)|(p[2]>0?32u:0);
        outside&=mask;
    }
    if(outside)return;
    if(vertex_count+count>VERTICES) fh_renderer_sync();
    u64 phase=component_tick();
    /* Keep live raster colors out of reusable programs when the general form
     * fits PICA. Constant specialization remains available for harder forms. */
    const FhMaterial *plan=material(state,input,count,1);
    if(!plan){
        FhDrawState specialized=*state;specialized.constant_raster_mask=255;
        memcpy(specialized.constant_raster,input[0].color,sizeof(specialized.constant_raster));
        for(unsigned i=1;i<count && specialized.constant_raster_mask;i++) for(unsigned c=0;c<8;c++)
            if(((const float*)input[i].color)[c]!=((const float*)input[0].color)[c]) specialized.constant_raster_mask&=~(1u<<c);
        for(unsigned c=0;c<8;c++)if(!(specialized.constant_raster_mask&(1u<<c)))((float*)specialized.constant_raster)[c]=0;
        plan=material(&specialized,input,count,0);
    }
    if((state->raw_position&2) && !gpu_lighting_supported(plan) && !((state->raw_position&64) && plan->sphere_lighting) && !(plan->cpu_fragment==2 && fur_mode()==2))fh_platform_panic("Deferred lighting material changed");
    material_ticks+=component_tick()-phase;
    if(plan->cpu_fragment){
        /* GX bindings survive across draws. Only referenced TEV/indirect
         * samplers can require CPU bytes; stale unused depth/reflection slots
         * must not force a VRAM readback. Keep a conservative superset of
         * actual samples, including every declared indirect stage. */
        unsigned used=fh_texture_dependencies(state);
        for(unsigned i=0;i<8;i++)if(used&(1u<<i))fh_renderer_materialize_texture(state->textures[i].pixels);
    }
#ifdef FH_DRAW_CAPTURE
    unsigned capture_mode=plan->movie_layer?3:plan->cpu_fragment;
    if(fh_capture_draw_pending(frame_number,count,capture_mode)){
        unsigned used=fh_texture_dependencies(state);
        for(unsigned i=0;i<8;i++)if(used&(1u<<i))fh_renderer_materialize_texture(state->textures[i].pixels);
    }
    fh_capture_draw(frame_number,input,count,state,capture_mode);
#endif
    if(state->raw_position && plan->cpu_fragment==2 && fur_mode()==2){
        float matrix[2][2][3];
        for(unsigned i=0;i<2;i++)for(unsigned axis=0;axis<2;axis++)for(unsigned c=0;c<3;c++)matrix[i][axis][c]=ldexpf(state->indirect_matrix[i][axis][c],state->indirect_exponent[i]);
        phase=component_tick();int drawn=fur_vertex_draw(input,count,state,matrix);fragment_ticks+=component_tick()-phase;
        if(drawn)return;
    }
    if(plan->sphere_lighting
#ifdef FH_EMULATOR_TEST
       && !fh_sphere_reference
#endif
       && sphere_draw(input,count,state))return;
    unsigned mixed_sphere_matrices=0;
    if(state->raw_position && plan->sphere_lighting)for(unsigned i=0;i<count;i+=3)
        if(input[i].matrix_index!=input[i+1].matrix_index || input[i].matrix_index!=input[i+2].matrix_index){mixed_sphere_matrices=1;break;}
    /* Subdivision commutes with one homogeneous transform per triangle. Mixed
     * palettes must first enter the common clip space before interpolation. */
    if(state->raw_position && (plan->cpu_fragment || mixed_sphere_matrices)){transform_cpu_draw(input,count,state);return;}
    if(plan->cpu_fragment){
        if(!fh_fragment_supported(state))fh_platform_panic("Invalid fragment material sources");
        phase=component_tick();fragment_draw(input,count,state,plan->cpu_fragment);
        u64 elapsed=component_tick()-phase;fragment_ticks+=elapsed;
#ifdef FH_DRAW_CAPTURE
        /* Texture bytes and this triangle batch remain borrowed and valid.
         * Record only after timing, so capture I/O is not the trigger. */
        fh_capture_draw_timed(frame_number,input,count,state,plan->cpu_fragment,elapsed);
#endif
        return;
    }
    render_direct(input,count,state,plan);
}
static void render_direct(const FhVertex *input,unsigned count,const FhDrawState *state,const FhMaterial *plan) {
    render_direct_coordinates(input,count,state,plan,NULL);
}
int fh_renderer_compiled_fur_available(const FhDrawState *state){
    return !fur_surface_enabled && fur_mode()==2 && fh_material_scope_fur(state);
}
int fh_renderer_fur_texgen_available(const FhDrawState *state){
    return (fur_surface_enabled || fur_mode()==2) && fh_material_scope_fur(state);
}
/* Cumulative exclusive fur preparation buckets, ARM11 system ticks.
 * These bracket existing work; they are not GPU timings or a frame estimate. */
int fh_renderer_draw_compiled(const FhCompiledMesh *mesh,const FhDrawState *state,FhCompiledDecode decode,FhCompiledDecodeBatch batch){
#ifdef FH_EMULATOR_TEST
    u64 admission_started=component_tick();
#endif
    if(compiled_mesh || native_replay || point_context.active || !mesh || !decode || !(state->raw_position&1) || (state->raw_position&8) ||
       !mesh->count || mesh->count%3 || mesh->count>16384 || !mesh->unique || mesh->unique>8192)return 0;
#ifdef FH_DRAW_CAPTURE
    if(fh_capture_draw_pending(frame_number,384,0))return 0;
#endif
    const FhMaterial *plan=material(state,NULL,0,1);
    unsigned fur=plan && plan->cpu_fragment==2 && fur_mode()==2;
    compiled_fur_post=fur?mesh->fur_post:NULL;
#ifdef FH_DRAW_CAPTURE
    if(fur && fh_capture_draw_pending(frame_number,384,2))return 0;
#endif
    if(!plan || (plan->cpu_fragment && !fur) || plan->movie_layer || ((state->raw_position&2) && !gpu_lighting_supported(plan) && !((state->raw_position&64) && plan->sphere_lighting) && !fur))return 0;
    FhDrawState sphere_state;FhMaterial sphere_plan;
    uint64_t fur_id=0,staged_id=0;NativeGeometry *fur_replay=NULL;
    unsigned sphere=plan->sphere_lighting;
    if(sphere){
        FhVertex dummy[3]={0},output[3];
        if(!fh_sphere_prepare(&sphere_plan,&sphere_state,output,state,dummy,3))return 0;
        state=&sphere_state;plan=&sphere_plan;
    }
#ifdef FH_EMULATOR_TEST
    if(fur)fh_fur_path_ticks[5]+=component_tick()-admission_started;
    u64 queue_started=component_tick();
#endif
    service_audio();begin();reserve_commands();
#ifdef FH_EMULATOR_TEST
    if(fur)fh_fur_path_ticks[6]+=component_tick()-queue_started;
#endif
    /* Melee's owned indexed packets separate source identity from live raster
     * state. Reuse the bounded fur staging allocation for ordinary meshes;
     * no extra arena and no retention of game-owned vertex pointers. */
    if(!fur && batch && !fh_indexed_geometry_disabled && mesh->unique<=FH_COMPILED_RETAINED_UNIQUE){
        uint64_t id=batch(NULL,mesh);NativeGeometryKey key;
        NativeGeometry *entry=id && native_geometry_key(&key,state,plan)?native_geometry_find(id,mesh->count,&key):NULL;
        if(entry){
            native_replay=entry;render_direct(NULL,mesh->count,state,plan);native_replay=NULL;
            fh_indexed_geometry_draws++;fh_indexed_geometry_replays++;return 1;
        }
#ifdef FH_COMPACT_VERTEX_BUILD
        if(!id && !sphere && plan->pipeline.count && !affine_supported(plan) &&
           mesh->decode_compact && mesh->decode_compact(NULL,mesh,NULL,NULL)){compiled_compact=1;fh_indexed_geometry_draws++;}
#endif
        if(!compiled_compact && !id && (plan->pipeline.count || affine_supported(plan)) &&
           mesh->decode_native && mesh->decode_native(NULL,mesh,NULL)){
            compiled_native=1;fh_indexed_geometry_draws++;
        }
        if(!compiled_native && !compiled_compact && (!sphere || (state->raw_position&64))){
        if(!compiled_fur_storage){
            extern u32 __ctru_heap_size,__stacksize__;
            uint64_t used=(unsigned)mallinfo().uordblks;
            if((used+sizeof(CompiledFur)+__stacksize__+64)*100<=(uint64_t)__ctru_heap_size*85){
                compiled_fur_storage=memalign(32,sizeof(CompiledFur));
                if(compiled_fur_storage)fh_compiled_fur_bytes=sizeof(CompiledFur);
            }
        }
        if(compiled_fur_storage){
            staged_id=batch(compiled_fur_storage->vertices,mesh);
            for(unsigned i=0;i<mesh->unique;i++)if(compiled_fur_storage->vertices[i].matrix_index<0)return 0;
            compiled_staged=1;fh_indexed_geometry_draws++;
        }
        }
    }
    if(fur){
        if(mesh->unique>COMPILED_FUR_UNIQUE)return 0;
        /* Verify the immutable CPU identity first. A resident GPU packet does
         * not need to reconstruct/validate/repack every unique vertex again.
         * Pending texture copies are materialized before checking sample keys. */
        if(batch && !fh_gpu_fur_disabled && fh_fur_gpu_strength(state,&compiled_fur_strength)){
            u64 fur_phase=component_tick();
            uint64_t id=batch(NULL,mesh);
            fh_fur_path_ticks[0]+=component_tick()-fur_phase;
            if(id){
                fur_phase=component_tick();
                for(unsigned i=0;i<2;i++)fh_renderer_materialize_texture(state->textures[state->indirect_order[i][1]].pixels);
                sphere_state=*state;fh_fur_mesh_pipeline(&sphere_plan,state);
                sphere_state.textures[0]=state->textures[3];sphere_state.textures[1]=state->textures[0];
                sphere_state.textures[2]=state->textures[1];sphere_state.textures[3]=state->textures[2];
                compiled_fur_gpu=1;compiled_fur_source=state;
                NativeGeometryKey key;NativeGeometry *entry=NULL;
                if(native_geometry_key(&key,&sphere_state,&sphere_plan))entry=native_geometry_find(id,mesh->count,&key);
                fh_fur_path_ticks[1]+=component_tick()-fur_phase;
                if(entry){
                    native_replay=entry;u64 started=component_tick();
                    render_direct(NULL,mesh->count,&sphere_state,&sphere_plan);
                    u64 resident_ticks=component_tick()-started;
                    fh_fur_path_ticks[2]+=resident_ticks;
                    fragment_ticks+=resident_ticks;mesh_triangles+=mesh->count/3;
                    fh_compiled_fur_draws++;fh_compiled_fur_unique+=mesh->unique;fh_gpu_fur_draws++;fh_gpu_fur_replays++;
                    native_replay=NULL;compiled_fur_gpu=0;compiled_fur_source=NULL;return 1;
                }
                compiled_fur_gpu=0;compiled_fur_source=NULL;
            }
        }
        if(!compiled_fur_storage){
            extern u32 __ctru_heap_size,__stacksize__;
            uint64_t used=(unsigned)mallinfo().uordblks;
            if((used+sizeof(CompiledFur)+__stacksize__+64)*100>(uint64_t)__ctru_heap_size*85)return 0;
            compiled_fur_storage=memalign(32,sizeof(CompiledFur));if(!compiled_fur_storage)return 0;
            fh_compiled_fur_bytes=sizeof(CompiledFur);
        }
        /* Read current attributes once per distinct packet. Original index
         * order and all shells are preserved. Reject the entire candidate
         * before submission if any coordinate/layout needs scalar fallback. */
        FhVertex *v=compiled_fur_storage->vertices;
        u64 decode_start=component_tick();
        if(batch)fur_id=batch(v,mesh);
        for(unsigned i=0;i<mesh->unique;i++){
            if(!batch)decode(v+i,mesh->packets+mesh->offsets[i]);
            if(v[i].matrix_index<0)return 0;
        }
        fh_fur_path_ticks[3]+=component_tick()-decode_start;
        for(unsigned i=0;i<mesh->count;i+=3){
            unsigned a=mesh->indices[i],b=mesh->indices[i+1],c=mesh->indices[i+2];
            if(a>=mesh->unique || b>=mesh->unique || c>=mesh->unique)fh_platform_panic("Compiled fur index outside owned mesh");
            if(v[a].uv[0][2]!=v[b].uv[0][2] || v[a].uv[0][2]!=v[c].uv[0][2] ||
               v[a].uv[2][2]!=v[b].uv[2][2] || v[a].uv[2][2]!=v[c].uv[2][2])return 0;
        }
        unsigned padded=(mesh->unique+2)/3*3;
        for(unsigned i=mesh->unique;i<padded;i++)v[i]=v[mesh->unique-1];
        unsigned used=fh_texture_dependencies(state);
        for(unsigned i=0;i<8;i++)if(used&(1u<<i))fh_renderer_materialize_texture(state->textures[i].pixels);
        sphere_state=*state;fh_fur_mesh_pipeline(&sphere_plan,state);
        sphere_state.textures[0]=state->textures[3];sphere_state.textures[1]=state->textures[0];
        sphere_state.textures[2]=state->textures[1];sphere_state.textures[3]=state->textures[2];
        u64 start=component_tick();
        compiled_fur_source=state;
        compiled_fur_gpu=!fh_gpu_fur_disabled && (fur_id || state->raw_position==7) && fh_fur_gpu_strength(state,&compiled_fur_strength);
        if(compiled_fur_gpu){
            NativeGeometryKey key;
            if(fur_id && native_geometry_key(&key,&sphere_state,&sphere_plan))fur_replay=native_geometry_find(fur_id,mesh->count,&key);
            if(!fur_replay){
                int accepted=fh_fur_coefficients_mode?
                    fh_fur_gpu_coefficients_cached(compiled_fur_storage->uv,v,mesh->unique,state):
                    fh_fur_gpu_coefficients(compiled_fur_storage->uv,v,mesh->unique,state);
                if(!accepted)compiled_fur_gpu=0;
                if(accepted && fh_fur_coefficients_mode==2)for(unsigned i=0;i<mesh->unique;i++){
                    float reference[1][3];
                    if(!fh_fur_gpu_coefficients(reference,v+i,1,state) || memcmp(reference[0],compiled_fur_storage->uv[i],12))
                        fh_platform_panic("Cached fur samples differ from live scalar texture samples");
                    fh_fur_coefficients_checks++;
                }
            }
        }
        if(!compiled_fur_gpu){
            float matrix[2][2][3];
            for(unsigned i=0;i<2;i++)for(unsigned axis=0;axis<2;axis++)for(unsigned c=0;c<3;c++)matrix[i][axis][c]=ldexpf(state->indirect_matrix[i][axis][c],state->indirect_exponent[i]);
            unsigned n=fh_fur_coordinates_cached(compiled_fur_storage->uv,v,padded,state,matrix);
            if(n!=padded){compiled_fur_source=NULL;return 0;}
        }
        u64 elapsed=component_tick()-start;mesh_ticks+=elapsed;fragment_ticks+=elapsed;
        fh_fur_path_ticks[4]+=elapsed;
        state=&sphere_state;plan=&sphere_plan;
    }
    if(vertex_count+mesh->unique>VERTICES || index_count+(mesh->count>mesh->unique?mesh->count:mesh->unique)>VERTICES)fh_renderer_sync();
    compiled_mesh=mesh;compiled_decode=decode;compiled_sphere=sphere;compiled_fur=fur;compiled_failed=0;
    uint64_t old_id=native_record_id;const FhVertex *old_input=native_record_input;
    const FhDrawState *old_state=native_record_state;unsigned old_count=native_record_count;
    if(compiled_fur_gpu){
        native_record_id=fur_id;native_record_input=NULL;native_record_state=state;native_record_count=mesh->count;
        native_replay=fur_replay;fh_gpu_fur_draws++;if(fur_replay)fh_gpu_fur_replays++;
    }
    else if(compiled_staged){
        native_record_id=staged_id;native_record_input=NULL;native_record_state=state;native_record_count=mesh->count;
    }
    u64 started=component_tick();
    render_direct(NULL,mesh->count,state,plan);
    if(fur){u64 elapsed=component_tick()-started;fragment_ticks+=elapsed;fh_fur_path_ticks[7]+=elapsed;mesh_triangles+=mesh->count/3;fh_compiled_fur_draws++;fh_compiled_fur_unique+=mesh->unique;}
    native_replay=NULL;native_record_id=old_id;native_record_input=old_input;native_record_state=old_state;native_record_count=old_count;
    compiled_mesh=NULL;compiled_decode=NULL;compiled_sphere=compiled_fur=compiled_fur_gpu=compiled_staged=compiled_native=compiled_compact=0;compiled_fur_source=NULL;return !compiled_failed;
}
#include "platform_ordered_program.h"
#include "platform_native_model_draw.h"
#include "platform_native_sky.h"
static void render_direct_coordinates(const FhVertex *input,unsigned count,const FhDrawState *state,const FhMaterial *plan,const float uv7[][3]) {
#ifdef FH_EMULATOR_TEST
    unsigned detail_enabled=1;
#else
    unsigned detail_enabled=fh_render_attribution_active;
#endif
    u64 detail_started=detail_enabled?svcGetSystemTick():0;
    unsigned detail_fur=compiled_fur || compiled_fur_gpu;
    unsigned detail_backend=native_replay?2:compiled_mesh?1:0;
#define FUR_DETAIL(i) do { if(detail_enabled) { u64 now=svcGetSystemTick(); \
    if(detail_fur && (i)<15)fh_fur_path_ticks[i]+=now-detail_started; \
    fh_backend_ticks[detail_backend][(i)-10]+=now-detail_started; \
    if((i)==10){fh_backend_calls[detail_backend]++;}detail_started=now; } } while(0)
    reserve_commands();if(vertex_count+count>VERTICES)fh_renderer_sync();
    Texture *bound[3]={0};u64 phase;
    if(plan->movie_layer && count>3) {
        if(fh_movie_batch_enabled && !uv7 && input && count%3==0 && !point_context.active && state->raw_position<=1){
            /* Keep base/overlay adjacent for each triangle. Batching complete
             * passes would change blending for overlapping coplanar triangles.
             * Pinned Melee's reusable material programs are the reference;
             * SFA's fourth sampler still requires this ordered second pass. */
            FhMaterial base=*plan;base.movie_layer=0;
            FhDrawState overlay=*state;overlay.stages=1;
            overlay.tev[0]=(FhTevStage){.color={15,8,12,15},.alpha={7,7,7,6},.color_clamp=1,.alpha_clamp=1,.coord=3,.map=4,.channel=255,.kalpha=28};
            for(unsigned c=0;c<4;c++)overlay.swaps[0][c]=c;
            overlay.konst_colors[0][3]=plan->movie_mix;overlay.blend_mode=1;overlay.blend_src=4;overlay.blend_dst=5;
            if(state->depth_test && state->depth_write)overlay.depth_compare=2;
            overlay.depth_write=0;
            const FhMaterial *found=material(&overlay,input,count,1);
            if(found && !found->cpu_fragment && !found->sphere_lighting && !found->movie_layer){
                /* Texture materialization can use the material cache itself;
                 * keep both programs owned until this synchronous batch ends. */
                FhMaterial top=*found;
                if(fh_movie_batch_enabled==3 && ordered_movie_program(input,count,state,&base,&overlay,&top)){
                    fh_movie_batches++;fh_movie_batch_triangles+=count/3;return;
                }
                if(fh_movie_batch_enabled==2){
                    unsigned groups[128],n=fh_movie_groups(groups,input,count,state);
                    if(n && n<count/3){
                        unsigned first=0;
                        for(unsigned g=0;g<n;g++){
                            service_audio();
                            render_direct_coordinates(input+first,groups[g],state,&base,NULL);
                            render_direct_coordinates(input+first,groups[g],&overlay,&top,NULL);
                            first+=groups[g];
                        }
                        fh_movie_batches++;fh_movie_batch_triangles+=count/3;
                        fh_movie_disjoint_batches++;fh_movie_disjoint_triangles+=count/3;fh_movie_disjoint_groups+=n;return;
                    }
                    fh_movie_disjoint_rejections++;
                }
                for(unsigned i=0;i<count;i+=3){
                    service_audio();
                    render_direct_coordinates(input+i,3,state,&base,NULL);
                    render_direct_coordinates(input+i,3,&overlay,&top,NULL);
                }
                fh_movie_batches++;fh_movie_batch_triangles+=count/3;return;
            }
        }
        for(unsigned i=0;i<count;i+=3) fh_renderer_draw(input+i,3,state);
        return;
    }
    unsigned movie_layer=plan->movie_layer;float movie_mix=plan->movie_mix;
    FUR_DETAIL(10);
    phase=component_tick();
    unsigned units=plan->pipeline.count?plan->pipeline.textures:plan->map>=0?1:0;
    for(unsigned unit=0;unit<units;unit++) {
        unsigned map=plan->pipeline.count?plan->pipeline.map[unit]:plan->map;
        unsigned mode=plan->pipeline.count?plan->pipeline.texture_mode[unit]:0;
        Texture *texture=bound[unit]=load_texture(&state->textures[map],mode,(mode==1 || mode==3)?&state->textures[plan->pipeline.secondary_map[unit]]:NULL);
        texture->pinned=1;
    }
    /* Lazy framebuffer textures can themselves submit a conversion quad.
     * Recheck capacity before the caller emits its reserved vertex span. */
    if(vertex_count+count>VERTICES)fh_renderer_sync();
    unsigned sphere_shader_wanted=(state->raw_position&64)!=0;
    if(sphere_program_active!=sphere_shader_wanted){
        flush_draw();sphere_program_wanted=sphere_shader_wanted;bind_geometry();
    }
    if(sphere_shader_wanted)fh_gpu_sphere_draws++;
    for(unsigned unit=0;unit<units;unit++) {
        C3D_Tex next=bound[unit]->gpu;
        unsigned map=plan->pipeline.count?plan->pipeline.map[unit]:plan->map;
        static const GPU_TEXTURE_WRAP_PARAM wraps[]={GPU_CLAMP_TO_EDGE,GPU_REPEAT,GPU_MIRRORED_REPEAT};
        C3D_TexSetWrap(&next,wraps[state->textures[map].wrap_s%3],wraps[state->textures[map].wrap_t%3]);
        C3D_TexSetFilter(&next,state->textures[map].mag_filter?GPU_LINEAR:GPU_NEAREST,state->textures[map].min_filter?GPU_LINEAR:GPU_NEAREST);
        next.param=(next.param&~GPU_TEXTURE_MODE(7))|GPU_TEXTURE_MODE(unit?GPU_TEX_2D:GPU_TEX_PROJECTION);
        if(!bound_used[unit] || !combiner_valid || fh_memory_compare(&next,&bound_gpu[unit],sizeof(next))) {
            flush_draw();bound_gpu[unit]=next;bound_used[unit]=1;C3D_TexBind(unit,&bound_gpu[unit]);
        }
        bound[unit]->pinned=0;
    }
    for(unsigned unit=units;unit<3;unit++)if(bound_used[unit]){flush_draw();C3D_TexBind(unit,NULL);bound_used[unit]=0;}
    texture_ticks+=component_tick()-phase;phase=component_tick();
    FUR_DETAIL(11);
    unsigned intensity[3];for(unsigned i=0;i<3;i++)intensity[i]=bound[i] && bound[i]->mode!=3 && bound[i]->source.format<=1;
    configure_fog(state);
    if(!combiner_valid || fh_memory_compare(plan,&last_combiner,sizeof(*plan)) || fh_memory_compare(intensity,last_intensity,sizeof(intensity))) {
        flush_draw();
        if(plan->pipeline.count) pipeline_combiner(&plan->pipeline,bound);else combiner(plan,bound[0]);
        last_combiner=*plan;memcpy(last_intensity,intensity,sizeof(intensity));combiner_valid=1;
    }
    unsigned x=state->scissor[0],y=state->scissor[1],w=state->scissor[2],h=state->scissor[3];
    FUR_DETAIL(12);
    if(x>640)x=640;
    if(y>FH_EFB_HEIGHT)y=FH_EFB_HEIGHT;
    if(w>640-x)w=640-x;
    if(h>FH_EFB_HEIGHT-y)h=FH_EFB_HEIGHT-y;
    unsigned clip[4]={(FH_EFB_HEIGHT+1-y-h)/2,fh_screen_edge(640-x-w),(FH_EFB_HEIGHT+1-y)/2,fh_screen_edge(640-x)};
    if(clip[0]>=clip[2] || clip[1]>=clip[3])return;
    if(!raster_valid || fh_memory_compare(clip,last_raster.clip,sizeof(clip))){flush_draw();C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);}
    if(state->cull==3) return;
    if(!raster_valid || state->cull!=last_raster.cull){flush_draw();C3D_CullFace(state->cull==1?GPU_CULL_BACK_CCW:state->cull==2?GPU_CULL_FRONT_CCW:GPU_CULL_NONE);}
    static const GPU_TESTFUNC depths[]={GPU_NEVER,GPU_GREATER,GPU_EQUAL,GPU_GEQUAL,GPU_LESS,GPU_NOTEQUAL,GPU_LEQUAL,GPU_ALWAYS};
    GPU_WRITEMASK mask=(state->color_write?(GPU_WRITE_RED|GPU_WRITE_GREEN|GPU_WRITE_BLUE):0)|(state->alpha_write?GPU_WRITE_ALPHA:0)|(state->depth_write?GPU_WRITE_DEPTH:0);
    unsigned depth=state->depth_test?depths[state->depth_compare&7]:GPU_ALWAYS;
    if(!raster_valid || depth!=last_raster.depth || mask!=last_raster.mask){flush_draw();C3D_DepthTest(true,depth,mask);}
    unsigned packed=state->alpha_compare[0]|(state->alpha_compare[1]<<3)|(state->alpha_compare[2]<<11)|(state->alpha_compare[3]<<13)|(state->alpha_compare[4]<<16),function,reference;
    if(!raster_valid || packed!=last_raster.alpha){
        if(!fh_alpha_reduce_cached(packed,&function,&reference))fh_platform_panic("Compound alpha comparison needs multiple passes");
        flush_draw();C3D_AlphaTest(function!=7,alpha_comparison(function),reference);
    }
    unsigned blend_key=state->blend_mode|(state->logic_op<<8)|(state->pixel_format==1?0x10000:0);
    if(!raster_valid || blend_key!=last_raster.blend || state->blend_src!=last_raster.src || state->blend_dst!=last_raster.dst) {
        flush_draw();
        MPBlendState blend=mp_blend_state(blend_key,state->blend_src,state->blend_dst);
        if(blend.logical) C3D_ColorLogicOp((GPU_LOGICOP)blend.logic);
        else C3D_AlphaBlend((GPU_BLENDEQUATION)blend.equation,(GPU_BLENDEQUATION)blend.equation,(GPU_BLENDFACTOR)blend.src,(GPU_BLENDFACTOR)blend.dst,(GPU_BLENDFACTOR)blend.src_alpha,(GPU_BLENDFACTOR)blend.dst_alpha);
    }
    memcpy(last_raster.clip,clip,sizeof(clip));last_raster.cull=state->cull;last_raster.depth=depth;last_raster.mask=mask;
    last_raster.alpha=packed;last_raster.blend=blend_key;last_raster.src=state->blend_src;last_raster.dst=state->blend_dst;raster_valid=1;
    FUR_DETAIL(13);
    if(model_draw && state==&model_draw->state){native_model_emit(state,plan,bound);return;}
    if(native_sky.emit && state==&native_sky.state){native_sky_emit();return;}
    configure_transform(state);
    VertexSink sink={.state=state,.plan=plan,.textures={bound[0],bound[1],bound[2]},.batch_start=index_count,.affine=configure_affine(plan)};
    for(unsigned unit=0;unit<units;unit++){
        sink.coord[unit]=plan->pipeline.count?plan->pipeline.coord[unit]:plan->coord;
        sink.uv_scale[unit][0]=(float)bound[unit]->source.width/bound[unit]->gpu.width;
        sink.uv_scale[unit][1]=(float)bound[unit]->source.height/bound[unit]->gpu.height;
    }
    if(compiled_fur_gpu)sink.coord[0]=4;
    if(point_context.active){
        float texture_offset=(point_context.offsets&1)?point_context.texture_offset:0;
        flush_draw();
        /* Match fh_expand_point after the full-width 640-column logical EFB
         * projection. The former 800-column presentation denominator made
         * native particles 20 percent narrower than triangle expansion. */
        point_parameters[0]=point_context.width/(6.f*FH_EFB_HEIGHT);point_parameters[1]=point_context.width/(6.f*640.f);
        point_parameters[2]=texture_offset*sink.uv_scale[0][0];point_parameters[3]=texture_offset*sink.uv_scale[0][1];
        C3D_FVUnifSet(GPU_GEOMETRY_SHADER,0,point_parameters[0],point_parameters[1],point_parameters[2],point_parameters[3]);
    }
    FUR_DETAIL(14);
    if(native_replay){native_geometry_submit(state);submit_ticks+=component_tick()-phase;FUR_DETAIL(15);return;}
    sink.sx=state->viewport[2]/640.f;sink.sy=state->viewport[3]/FH_EFB_HEIGHT;
    sink.ox=(2*state->viewport[0]+state->viewport[2]-640)/640.f;sink.oy=(FH_EFB_HEIGHT-2*state->viewport[1]-state->viewport[3])/FH_EFB_HEIGHT;
    sink.sz=state->viewport[5]-state->viewport[4];sink.oz=state->viewport[5]-1;
    if(ordered_capture && ordered_capture->state==state && ordered_capture->plan==plan){
        ordered_program_capture(ordered_capture,&sink);return;
    }
    const uint32_t *identities=NULL;
    uintptr_t identity_offset=(uintptr_t)input-(uintptr_t)identified.vertices;
    if(identified.identities && identity_offset<(size_t)identified.count*sizeof(*input) &&
       identity_offset%sizeof(*input)==0 && count<=identified.count-identity_offset/sizeof(*input))
        identities=identified.identities+identity_offset/sizeof(*input);
    unsigned first_vertex=vertex_count,first_index=index_count,first_barrier=barriers;
#ifdef FH_EMULATOR_TEST
    u64 packing_start=compiled_fur?component_tick():0;
#endif
    unsigned compact_draw=0;
    if(compiled_mesh){
        /* Decode each distinct GX packet once against current source arrays,
         * and write GPU-ready vertices directly into the owned stream. */
        FhVertex value;
        if((compact_draw=emit_compact_compiled(&sink)) || emit_native_compiled(&sink)){
            /* The existing index loop below keeps the original draw order. */
        }else if(compiled_fur){
            sink.uv7=compiled_fur_gpu?NULL:compiled_fur_storage->uv;
            emit_vertices(compiled_fur_storage->vertices,compiled_mesh->unique,&sink);
        }else if(compiled_staged)emit_vertices(compiled_fur_storage->vertices,compiled_mesh->unique,&sink);
        else for(unsigned i=0;i<compiled_mesh->unique;i++){
            compiled_decode(&value,compiled_mesh->packets+compiled_mesh->offsets[i]);
            if(compiled_sphere && (!(state->raw_position&64) || value.matrix_index<0)){
                for(unsigned c=0;c<8;c++)if(!(((float*)value.color)[c]>=0 && ((float*)value.color)[c]<=1)){
                    vertex_count=first_vertex;index_count=first_index;
                    if(flushed_vertex_bytes>vertex_count*sizeof(*vertices))flushed_vertex_bytes=vertex_count*sizeof(*vertices);
                    if(flushed_index_bytes>index_count*sizeof(*indices))flushed_index_bytes=index_count*sizeof(*indices);
                    compiled_failed=1;return;
                }
                for(unsigned j=0;j<2;j++){
                    value.uv[6+j][0]=(.5f+15*value.color[1][j*2])/16;
                    value.uv[6+j][1]=(.5f+15*value.color[1][j*2+1])/16;
                    value.uv[6+j][2]=1;
                }
            }
            emit_vertices(&value,1,&sink);
        }
        index_count=first_index;
        if(flushed_index_bytes>index_count*sizeof(*indices))flushed_index_bytes=index_count*sizeof(*indices);
        for(unsigned i=0;i<compiled_mesh->count;i++)indices[index_count++]=(compact_draw?0:first_vertex)+compiled_mesh->indices[i];
    }else if(point_context.active){
        emit_vertices(input,count,&sink);
    }else for(unsigned i=0;i<count;i+=3) {
        if(plan->sphere_lighting) fh_material_subdivide(input+i,emit_triangle,&sink);
        else {sink.uv7=uv7?uv7+i:NULL;sink.identities=identities?identities+i:NULL;emit_triangle(input+i,&sink);}
    }
#ifdef FH_EMULATOR_TEST
    u64 retention_start=compiled_fur?component_tick():0;
    if(compiled_fur)fh_fur_path_ticks[8]+=retention_start-packing_start;
#endif
    if(!compact_draw)native_geometry_record(input,count,state,plan,first_vertex,first_index,first_barrier);
#ifdef FH_EMULATOR_TEST
    if(compiled_fur)fh_fur_path_ticks[9]+=component_tick()-retention_start;
#endif
#ifdef FH_COMPACT_VERTEX_BUILD
    if(compact_draw){
        /* Complete this draw under its own binding. Existing stream fences
         * and flush ranges retain the bytes until retirement. */
        flush_draw();compact_vertex_buffer(vertices+first_vertex,1);
        submit_vertices(&sink);flush_draw();fh_compact_draws++;
        compact_vertex_buffer(vertices,0);
    }else
#endif
    submit_vertices(&sink);
#ifdef FH_EMULATOR_TEST
    if(sink.affine)fh_affine_vertices+=count;
#endif
    submit_ticks+=component_tick()-phase;
    FUR_DETAIL(15);
    if(movie_layer) {
        FhDrawState overlay=*state;overlay.stages=1;
        overlay.tev[0]=(FhTevStage){.color={15,8,12,15},.alpha={7,7,7,6},.color_clamp=1,.alpha_clamp=1,.coord=3,.map=4,.channel=255,.kalpha=28};
        for(unsigned c=0;c<4;c++) overlay.swaps[0][c]=c;
        overlay.konst_colors[0][3]=movie_mix;overlay.blend_mode=1;overlay.blend_src=4;overlay.blend_dst=5;
        if(state->depth_test && state->depth_write) overlay.depth_compare=2;
        overlay.depth_write=0;fh_renderer_draw(input,3,&overlay);
    }
}
#undef FUR_DETAIL
void fh_renderer_present(void) {
    if(fh_minimal_enabled){fh_minimal_present();return;}
    if(fh_render_floor)return;
    fh_gx_dispatch_join_deferred();
    if(!initialized || !open_frame) return;
    boot_gpu_trace("present-submit");
    flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
    enum { CROP_BYTES=(FH_EFB_NATIVE_HEIGHT-240)*8*4,ROW_BYTES=240*8*4 };
    copy_owned_gpu((u32*)((u8*)target->frameBuf.colorBuf+CROP_BYTES),efb_storage_bytes-CROP_BYTES,
        GX_BUFFER_DIM(ROW_BYTES/16,(efb_storage_width-240)*8*4/16),display_target->frameBuf.colorBuf,400*240*4,0,400*240*4,FH_GPU_COPY_GAPPED);
    display_target->used=true;
    fh_early_queue_defer();C3D_FrameEnd(GX_CMDLIST_FLUSH);open_frame=0;frame_number++;
    depth_snapshot_valid=0;
    boot_gpu_trace("present-submitted");
    /* Whole presentation cadence includes game work, waits and log I/O between
     * renderer frames. The existing wall field measures renderer work only.
     * GPU accounting stays deferred until the submitted queue has completed. */
    u64 present_tick=svcGetSystemTick();
    float interval_ms=previous_present_tick?(present_tick-previous_present_tick)*1000.0/SYSCLOCK_ARM11:0;
    previous_present_tick=present_tick;
    pending_timing=(typeof(pending_timing)){1,frame_number,draws,total_vertices,barriers,command_peak,texture_bytes,fragment_pixels,fragment_triangles,(present_tick-frame_start)*1000.0/SYSCLOCK_ARM11,cpu_total+C3D_GetProcessingTime(),gpu_total,interval_ms};
    attribution_present(frame_number);
    if(profile && frame_profile_active){fprintf(profile,"%u,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%u,%u,%u,%.3f,%.3f,%.3f,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%.3f,%.3f,%u\n",frame_number,material_ticks*1000.0/SYSCLOCK_ARM11,texture_ticks*1000.0/SYSCLOCK_ARM11,submit_ticks*1000.0/SYSCLOCK_ARM11,fragment_ticks*1000.0/SYSCLOCK_ARM11,material_hits,material_misses,texture_uploads,hashed_bytes,mesh_triangles,mesh_fallbacks,audio_ticks*1000.0/SYSCLOCK_ARM11,mesh_ticks*1000.0/SYSCLOCK_ARM11,copy_ticks*1000.0/SYSCLOCK_ARM11,copy_pixels,gpu_transform_vertices,cpu_transform_fallback_vertices,fh_vertex_unique-frame_unique_base,fh_vertex_reused-frame_reused_base,gpu_lit_vertices,gpu_light_evaluations,gpu_lighting_fallback_vertices,lighting_uniform_vectors,gpu_normal_vertices,gpu_texgen_vertices,stream_flush_bytes,stream_flush_full_bytes,stream_flush_calls,stream_flush_ticks*1000.0/SYSCLOCK_ARM11,queue_join_ticks*1000.0/SYSCLOCK_ARM11,fh_early_queue_starts-early_frame_starts);}
#ifdef FH_EMULATOR_TEST
    if(frame_number==2 || frame_number==9 || frame_number==10 || frame_number%capture_interval==0) {
        C3D_FrameSync();
        C3D_SyncDisplayTransfer(display_target->frameBuf.colorBuf,GX_BUFFER_DIM(240,400),readback,GX_BUFFER_DIM(240,400),0);
        finish_timing();
        if(R_FAILED(GSPGPU_InvalidateDataCache(readback,400*240*4))) fh_platform_panic("Capture cache invalidation failed");
        unsigned char *copy=malloc(400*240*4);
        if(!copy) fh_platform_panic("Capture staging allocation failed");
        for(unsigned i=0;i<400*240*4;i++) copy[i]=((volatile unsigned char*)readback)[i];
        char path[100];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/frame-%06u.rgba",frame_number);
        FILE *file=fopen(path,"wb");
        if(!file || fwrite(copy,1,400*240*4,file)!=400*240*4) fh_platform_panic("Capture write failed");
        fclose(file);free(copy);
    }
#endif
}
void fh_renderer_black(int value) { black=value;GSPGPU_SetLcdForceBlack(value!=0); }
void fh_renderer_clear(uint32_t color,uint32_t depth) {
    clear_color=color;clear_depth=depth;
    if(fh_minimal_enabled)fh_minimal_clear(color,depth);
}
static void texture_cache_invalidate(const void *address,size_t bytes) {
    fh_native_scene_texture_written(address,bytes);
    fh_fur_cache_invalidate_range(address,bytes);
    /* Retire identity, never a queued GPU allocation, when an owned fur
     * coefficient's sampled source changes. Frame keys also force resampling
     * once per frame, matching the pre-existing CPU sample-cache contract. */
    for(unsigned i=0;i<NATIVE_GEOMETRY_SLOTS;i++){
        NativeGeometry *e=native_geometry+i;if(!e->id || !e->storage)continue;
        const NativeGeometryKey *key=e->key;if(!key->fur)continue;
        for(unsigned t=0;t<2;t++)if(fh_texture_source_overlap(key->fur_textures+t,address,bytes))e->id=0;
    }
    for(unsigned i=0;i<TEXTURES;i++) if(textures[i].bytes) {
        if(fh_texture_source_overlap(&textures[i].source,address,bytes) ||
           ((textures[i].mode==1 || textures[i].mode==3) &&
            fh_texture_source_overlap(&textures[i].secondary,address,bytes)))textures[i].valid=0;
    }
}
void fh_renderer_invalidate(const void *address,size_t bytes) {
    if(fh_minimal_enabled)fh_minimal_texture_written(address,bytes);
    if(fh_render_floor)return;
    efb_color_invalidate(address,bytes);
    efb_depth_invalidate(address,bytes);
    efb_shadow_invalidate(address,bytes);
    texture_cache_invalidate(address,bytes);
}
void fh_renderer_texture_hint(void) {
    if(fh_minimal_enabled)fh_minimal_texture_hint();
    if(fh_render_floor)return;
    efb_color_invalidate(NULL,0);
    efb_depth_invalidate(NULL,0);
    efb_shadow_invalidate(NULL,0);
    texture_cache_invalidate(NULL,0);
}
static void efb_clear_rect(const uint32_t source[4],uint32_t color,uint32_t depth,const FhDrawState *state) {
        combiner_valid=raster_valid=0;
        C3D_FogGasMode(GPU_NO_FOG,(GPU_GASMODE)0,false);fog_valid=0;
        if(vertex_count+6>VERTICES) fh_renderer_sync();
        configure_transform(NULL);configure_affine(NULL);
        float x0=source[0]/320.f-1,y0=1-source[1]/(float)FH_EFB_NATIVE_HEIGHT,x1=(source[0]+source[2])/320.f-1,y1=1-(source[1]+source[3])/(float)FH_EFB_NATIVE_HEIGHT;
        float xy[4][2]={{x0,y0},{x0,y1},{x1,y1},{x1,y0}};unsigned corner[]={0,1,2,0,2,3};
        for(unsigned i=0;i<6;i++) {
            GpuVertex *v=&vertices[vertex_count+i];memset(v,0,sizeof(*v));
            v->position[0]=xy[corner[i]][1];v->position[1]=-xy[corner[i]][0];v->position[2]=depth/16777215.f-1;v->position[3]=1;
            for(unsigned c=0;c<4;c++) v->color[c]=((color>>(24-8*c))&255)/255.f;
        }
        C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
        for(unsigned i=0;i<6;i++) C3D_TexEnvInit(C3D_GetTexEnv(i));
        C3D_TexEnv *env=C3D_GetTexEnv(0);C3D_TexEnvSrc(env,C3D_Both,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(env,C3D_Both,GPU_REPLACE);
        C3D_DepthTest(true,GPU_ALWAYS,(state->color_write?(GPU_WRITE_RED|GPU_WRITE_GREEN|GPU_WRITE_BLUE):0)|(state->alpha_write?GPU_WRITE_ALPHA:0)|(state->depth_write?GPU_WRITE_DEPTH:0));
        C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        flush_stream(vertices,(vertex_count+6)*sizeof(*vertices),&flushed_vertex_bytes,vertex_count*sizeof(*vertices));
        C3D_DrawArrays(GPU_TRIANGLES,vertex_count,6);vertex_count+=6;draws++;total_vertices+=6;
}
void fh_renderer_copy(void *destination,const uint32_t source[4],unsigned width,unsigned height,unsigned format,int half_scale,int clear,uint32_t color,uint32_t depth,const FhDrawState *state) {
    if(fh_render_floor){
        size_t bytes=fh_efb_bytes(width,height,format);
        if(!destination || !bytes)fh_platform_panic("Invalid render-disabled copy destination");
        /* Defined empty EFB data, not uninitialized memory. Depth consumers
         * see the far plane. Visual readback effects are absent in this mode. */
        memset(destination,(format==0x11 || format==0x13)?255:0,bytes);
        fh_render_floor_copies++;return;
    }
    begin();reserve_commands();size_t bytes=fh_efb_bytes(width,height,format);
    u64 copy_start=svcGetSystemTick();
    if(!bytes) { char message[128];snprintf(message,sizeof(message),"Unsupported EFB copy format=%x extent=%ux%u",format,width,height);fh_platform_panic(message); }
    flush_vertices();
    int depth_copy=format==0x11 || format==0x13;
    if(early_queue_enabled)reserve_commands();
#ifdef FH_EMULATOR_TEST
    u64 transfer_started=svcGetSystemTick();
#endif
    boot_gpu_trace(depth_copy?"EFB-depth-copy":"EFB-color-copy");
    if(depth_copy && depth_snapshot_valid && depth_snapshot_draws==draws) {
        fh_depth_snapshot_reuses++;
    } else {
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    /* Melee's color readback path, also used for depth as opaque 32-bit words.
     * The transfer unswizzles both buffers, so CPU depth lookup is now linear.
     * Do not restore raw TextureCopy here: it stalls on the tested console. */
    queue_readback(depth_copy?target->frameBuf.depthBuf:target->frameBuf.colorBuf,FH_EFB_NATIVE_HEIGHT,400);
    fh_renderer_sync();
    acquire_readback();
    if(depth_copy){depth_snapshot_valid=1;depth_snapshot_draws=draws;fh_depth_snapshot_reads++;}
    }
#ifdef FH_EMULATOR_TEST
    u64 transfer_finished=svcGetSystemTick();
#endif
    int converted=depth_copy?fh_efb_depth_convert_stride(destination,bytes,readback,readback_stride,source,width,height,format):fh_efb_convert_filtered_stride(destination,bytes,readback,readback_stride,source,width,height,format,state->pixel_format==1,half_scale);
#ifdef FH_EMULATOR_TEST
    u64 conversion_finished=svcGetSystemTick();
#endif
    if(!converted) {char message[160];snprintf(message,sizeof(message),"Invalid EFB copy source=%lu,%lu,%lu,%lu destination=%u,%u format=%x half=%d",(unsigned long)source[0],(unsigned long)source[1],(unsigned long)source[2],(unsigned long)source[3],width,height,format,half_scale);fh_platform_panic(message);}
    fh_renderer_invalidate(destination,bytes);
    if(clear)efb_clear_rect(source,color,depth,state);
    copy_ticks+=svcGetSystemTick()-copy_start;copy_pixels+=width*height;
#ifdef FH_EMULATOR_TEST
    profile_copy(source,width,height,format,(half_scale?1:0)|(clear?2:0),svcGetSystemTick()-copy_start,transfer_finished-transfer_started,conversion_finished-transfer_finished);
#endif
}
#include "platform_efb_color.h"
#include "platform_efb_depth.h"
#include "platform_efb_shadow.h"
static int copy_range_overlaps(const void *address,size_t bytes,const void *destination,size_t size) {
    uintptr_t p=(uintptr_t)address,q=(uintptr_t)destination;
    return address && destination && bytes && size && (p>=q?p-q<size:q-p<bytes);
}
unsigned fh_renderer_source_exclusions(const void **addresses,size_t *bytes,unsigned capacity){
    /* Called only with the render reader joined, before scope publication.
     * Exclude complete GPU-backed ranges regardless of current dirty flags;
     * materialization on the worker must not change producer admission. */
    if(capacity<11)fh_platform_panic("Source exclusion output too small");
    unsigned count=0;
    if(efb_color.destination){addresses[count]=efb_color.destination;bytes[count++]=320*240*2;}
    if(efb_depth.destination){addresses[count]=efb_depth.destination;bytes[count++]=320*240;}
    for(unsigned i=0;i<SHADOW_SLOTS;i++)if(shadow_copies[i].destination){
        addresses[count]=shadow_copies[i].destination;bytes[count++]=shadow_copies[i].width*shadow_copies[i].height/2;
    }
    if(fragment_atlas.gpu.data){addresses[count]=fragment_atlas.gpu.data;bytes[count++]=fragment_atlas.bytes;}
    return count;
}
void fh_renderer_cache_read(const void *address,size_t bytes) {
    if(fh_render_floor)return;
    /* Melee engine/os.c and renderer.c distinguish CPU visibility from CPU
     * writes. DCInvalidateRange must expose completed GPU data, not discard it. */
    if(copy_range_overlaps(address,bytes,efb_color.destination,320*240*2))fh_renderer_materialize_texture(efb_color.destination);
    if(copy_range_overlaps(address,bytes,efb_depth.destination,320*240))efb_depth_materialize(efb_depth.destination);
    for(unsigned i=0;i<SHADOW_SLOTS;i++) {
        ShadowCopy *s=shadow_copies+i;
        if(copy_range_overlaps(address,bytes,s->destination,s->width*s->height/2))shadow_materialize(s);
    }
    texture_cache_invalidate(address,bytes);
}
uint32_t fh_renderer_depth(unsigned x,unsigned y) {
    if(fh_render_floor)return 0xffffff;
    if(x>=640 || y>=FH_EFB_HEIGHT) fh_platform_panic("EFB depth peek outside framebuffer");
    begin();reserve_commands();flush_vertices();
    /* A GX breakpoint can request many positions from one completed EFB.
     * Retain the safe DisplayTransfer snapshot until any intervening draw,
     * framebuffer clear, other readback, or presentation invalidates it. */
    if(depth_snapshot_valid && depth_snapshot_draws==draws){
        fh_depth_snapshot_reuses++;return fh_efb_depth_pixel_stride(readback,readback_stride,x,y);
    }
    boot_gpu_trace("EFB-depth-peek");
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    queue_readback(target->frameBuf.depthBuf,FH_EFB_NATIVE_HEIGHT,400);fh_renderer_sync();
    acquire_readback();
    depth_snapshot_valid=1;depth_snapshot_draws=draws;fh_depth_snapshot_reads++;
    return fh_efb_depth_pixel_stride(readback,readback_stride,x,y);
}
void fh_renderer_shutdown(void) {
    if(fh_minimal_enabled){fh_minimal_shutdown();return;}
    if(!initialized) return;
    if(fur_surface_enabled){char line[96];snprintf(line,sizeof(line),"Reduced fur surface plans=%u\n",fur_surface_plans);fh_platform_log(line);}
    {char line[160];snprintf(line,sizeof(line),"EFB CPU views=%u stride=%u compaction_bytes_avoided=%llu\n",readback_views,readback_stride,(unsigned long long)readback_compact_bytes_avoided);fh_platform_log(line);}
    fh_fragment_worker_stop();free(fragment_pixels_cpu);fragment_pixels_cpu=NULL;
    free(compiled_fur_storage);compiled_fur_storage=NULL;fh_compiled_fur_bytes=0;
    fh_renderer_present();C3D_FrameSync();
    C3Di_RenderQueueWaitDone();fh_early_queue_retire();
    native_sky_shutdown();
    scene_storage_shutdown();
    {char line[192];snprintf(line,sizeof(line),"EFB capture row_DMA=%u contiguous_DMA=%u backing=%u readback_bytes=%u\n",capture_dma_rows,capture_dma_contiguous,efb_storage_width,efb_storage_bytes);fh_platform_log(line);}
    {char line[160];snprintf(line,sizeof(line),"EFB direct sampled_views=%u ordered=%u waits_avoided=%u\n",efb_sampled_views,efb_ordered_view,efb_view_waits_avoided);fh_platform_log(line);}
    if(early_queue_enabled){
        char line[224];snprintf(line,sizeof(line),"Early GPU queue starts=%u finished=%u runs=%u appends=%u capacity=%u wait_ms=%.3f overlap_span_ms=%.3f (span includes CPU work)\n",fh_early_queue_starts,fh_early_queue_finishes,fh_early_queue_runs,fh_early_queue_appends,fh_early_queue_capacity,fh_early_queue_wait_ticks*1000.0/SYSCLOCK_ARM11,fh_early_queue_span_ticks*1000.0/SYSCLOCK_ARM11);fh_platform_log(line);
    }
#ifdef FH_EMULATOR_TEST
    copy_profile_finish();
    texture_profile_finish();
#endif
    efb_shadow_finish();
    efb_color_finish();
    efb_depth_finish();
    for(unsigned i=0;i<NATIVE_GEOMETRY_SLOTS;i++)if(native_geometry[i].storage)native_geometry_retire(native_geometry+i);
    if(fh_native_geometry_key_bytes)fh_platform_panic("Geometry key ownership leaked at shutdown");
    C3D_SyncDisplayTransfer(display_target->frameBuf.colorBuf,GX_BUFFER_DIM(240,400),readback,GX_BUFFER_DIM(240,400),0);finish_timing();
    for(unsigned i=0;i<TEXTURES;i++) retire(&textures[i]);
    {char line[256];snprintf(line,sizeof(line),"Prepared textures entries=%u index_bytes=%u hits=%u misses=%u rejected=%u uploads=%u read_bytes=%llu avoided_RGBA_upload_bytes=%llu\n",prepared_textures.count,prepared_textures.directory_bytes,prepared_textures.hits,prepared_textures.misses,prepared_textures.rejected,prepared_textures.uploads,(unsigned long long)prepared_textures.read_bytes,(unsigned long long)prepared_textures.saved_upload_bytes);fh_platform_log(line);}
    fh_texture_pack_close(&prepared_textures);prepared_textures_checked=0;
    retire(&fragment_atlas);
    if(timings) fclose(timings);
    if(profile)fclose(profile);
    if(attribution){fclose(attribution);attribution=NULL;}
    attribution_period=fh_render_attribution_active=0;
    if(model_shader_binary){shaderProgramFree(&model_program);DVLB_Free(model_shader_binary);model_shader_binary=NULL;}
    shaderProgramFree(&sphere_program);DVLB_Free(sphere_shader_binary);shaderProgramFree(&point_program);shaderProgramFree(&program);DVLB_Free(shader);C3D_RenderTargetDelete(target);C3D_RenderTargetDelete(display_target);
    if(efb_target_texture.data){C3D_TexDelete(&efb_target_texture);memset(&efb_target_texture,0,sizeof(efb_target_texture));}
    linearFree(vertices);linearFree(indices);linearFree(readback);C3D_Fini();initialized=0;
}
