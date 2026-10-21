#include "fh_pica_sampler.h"
#include "fh_pica_texture_coordinates.h"
#include "fh_graphics_options.h"
#include "fh_replay.h"
#include "fh_pica_model.h"
#include "fh_pica_ui_model.h"
#include "fh_pica_viewport.h"
#include "fh_asset_paths.h"
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "fh_minimal_graphics.h"
#include "fh_pica_scene.h"
#include "fh_render_floor.h"
#include "fh_native_normal_palette.h"
#include "fh_native_visibility.h"
#include "fh_platform.h"
#include "fh_pica_world_packet_owner.h"
#include "fh_pica_shadow_image.h"
#include "minimal_shared_shader.h"
#include "fh_native_ui.h"
#include "fh_pica_texture.h"
#include "fh_texture_pack.h"
#include "fh_texture_dependencies.h"
#include "fh_memory_bytes.h"
#include "fh_gpu_cache.h"
#include "fh_gpu_work.h"
#include "fh_replay_timing.h"
#include "fh_early_queue.h"
#include "fh_native_lighting.h"
#include "fh_native_constant_light.h"
#include "fh_uniform_reuse.h"
extern unsigned fh_raster_reuse_enabled;
extern unsigned long long fh_raster_reuse_counts[6][2];
#include "fh_memory_budget.h"
#include "fh_gpu_input_snapshot.h"
#include "fh_native_fog.h"
#include "fh_world_index_stream.h"
#include "fh_world_material_key.h"
#include "platform_prepared_draws.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include "../vendor/melee/scissor.h"
#include "../vendor/melee/texture_index.h"
#pragma GCC diagnostic pop

static unsigned flat_world_scope,flat_world_valid,flat_world_format,flat_world_packed;
static unsigned flat_world_last;
static FhWorldMaterialKey world_material_last;
static unsigned world_material_reuses,world_pipeline_reuses,world_pipeline_flags,world_pipeline_cull;
static unsigned world_pipeline_clip[4];
static unsigned flat_world_setups,flat_world_reuses;
FhMinimalCallbackMaterial fh_minimal_callback;
static unsigned callback_draws,callback_constant_draws,callback_mask_draws,callback_hidden_draws;
static unsigned model_mask_draws,model_constant_alpha_draws,model_constant_rgb_draws;
unsigned fh_minimal_cloud_active,fh_minimal_cloud_depth;
unsigned fh_minimal_frames,fh_minimal_draws,fh_minimal_indices;
unsigned fh_minimal_missing,fh_minimal_rejected;
/* Bounded failure-only evidence; never write SD or format text in a draw. */
static struct {unsigned site,asset,detail,count;} coverage_failures[128];
static unsigned coverage_overflow;
void fh_minimal_coverage_failure(unsigned site,unsigned asset,unsigned detail){
    unsigned bucket=(site*17u+asset*31u+detail)&127;
    for(unsigned n=0;n<128;n++){
        unsigned i=(bucket+n)&127;
        if(!coverage_failures[i].count){
            coverage_failures[i].site=site;coverage_failures[i].asset=asset;coverage_failures[i].detail=detail;
            coverage_failures[i].count=1;return;
        }
        if(coverage_failures[i].site==site && coverage_failures[i].asset==asset && coverage_failures[i].detail==detail){
            coverage_failures[i].count++;return;
        }
    }
    coverage_overflow++;
}
static unsigned initialized,open_frame;
static uint32_t clear_color=0x000000ff,clear_depth=0xffffff;
void fh_minimal_clear(uint32_t color,uint32_t depth){
    clear_color=color;clear_depth=depth&0xffffff;
}
static C3D_RenderTarget *target;
static DVLB_s *binary;
static shaderProgram_s program;
static shaderProgram_s ui_program;
static shaderProgram_s world_program,water_program;
static shaderProgram_s hologram_program;
static unsigned hologram_draws,normal_disk_draws;
unsigned fh_minimal_hologram_calls;
static unsigned world_draws,world_layered_draws,world_failures,world_deferred_draws,world_movie_draws;
static unsigned world_sampler_resets,world_falloff_draws;
static unsigned world_cull_checks,world_culled_indices,world_culled_groups,world_split_draws;
enum { WORLD_STREAM_BYTES=64*1024 };
static FhWorldIndexStream world_stream;
static unsigned world_index_publications;
static u64 world_index_published_bytes;
void fh_minimal_publish_world_indices(void){
    unsigned bytes=fh_world_index_publish(&world_stream,fh_gpu_cache_flush);
    if(bytes){world_index_publications++;world_index_published_bytes+=bytes;}
}
static unsigned world_gather_draws,world_gather_fallbacks,world_gather_saved_draws;
static u64 world_gather_bytes;
static u64 world_ticks;
static float projection[4][4];
static FILE *metrics;
#include "fh_frame_metrics.h"
static FhFrameMetrics frame_metrics;
static char metrics_buffer[32768];
static u64 submit_ticks;
static u64 first_present_ticks,last_present_ticks;
static unsigned retire_syncs;static u64 retire_sync_ticks;
static void gpu_work_report(void){
    FhGpuWork w;fh_gpu_work_snapshot(&w);char line[360];
    snprintf(line,sizeof(line),"Texture retire syncs: frame=%u count=%u us=%llu\n",fh_minimal_frames,retire_syncs,
        (unsigned long long)(retire_sync_ticks*1000000/SYSCLOCK_ARM11));fh_platform_log(line);
    snprintf(line,sizeof(line),"GPU work: frame=%u submitted=%u completed=%u pending=%u failed=%u unmatched=%u unsupported=%u result=%08lx command=%u event=%u busy_ticks=%llu sample_ticks=%llu\n",
        fh_minimal_frames,(unsigned)w.submitted,(unsigned)w.completed,(unsigned)w.pending,(unsigned)w.failed,(unsigned)w.unmatched,(unsigned)w.unsupported,(unsigned long)(uint32_t)w.last_result,
        (unsigned)w.last_command,(unsigned)w.last_event,(unsigned long long)w.busy_ticks,(unsigned long long)w.sampled_ticks);
    fh_platform_log(line);
}
static unsigned frame_draws,frame_indices;
static unsigned queue_draws,queue_submissions;
static unsigned upload_writes,upload_drains;
static unsigned lit_draws,lighting_rejected,lighting_max,lighting_complex_max;
static u64 skin_lit_indices,skin_single_bone_indices;
static unsigned skin_single_bone_draws;
static unsigned normal_palette_draws,normal_palette_fallbacks,normal_palette_bones;
static unsigned collapsed_culled_draws;
static unsigned animated_bounds_checks,animated_bounds_culled;
static u64 animated_bounds_indices;
static unsigned collapsed_subpixel_lod_draws;
static unsigned collapsed_offscreen_draws;
static u64 collapsed_culled_indices;
static u64 normal_palette_indices;
static unsigned sphere_draws,sphere_failures,model_layered_draws;
static unsigned constant_light_mode=1,constant_light_draws;
static u64 constant_light_indices;
#ifdef FH_EMULATOR_TEST
static u64 model_profile[6];
#endif
enum { UI_VERTICES=1536 };
typedef struct {float position[4],color[4],uv[2];} MinimalUiVertex;
_Static_assert(sizeof(MinimalUiVertex)==40,"UI attribute stride");
static MinimalUiVertex *ui_vertices;
static unsigned ui_used,ui_count,ui_first,ui_alpha_only,ui_draws,ui_quads;
static unsigned ui_attempts,ui_empty;
static FhTexture ui_source;
static C3D_Tex *ui_texture;
static FhTexture ui_mask_source;
static C3D_Tex *ui_mask,*base_pin,*second_pin;
static unsigned ui_additive,ui_scissored,ui_clip[4],ui_projected_quads;
static unsigned ui_effect,ui_effect_blend,effect_quads,effect_draws;
static float effect_view[3][4],effect_projection[4][4],effect_near,effect_far;
static unsigned effect_clip[4];
unsigned fh_minimal_text_color=0xffffffff;
unsigned fh_minimal_text_alpha_only;
FhTexture fh_minimal_text_texture;
FhTexture fh_minimal_movie_texture;
unsigned fh_minimal_movie_active;
static unsigned movie_draws;
static void flush_ui(void);
#include "platform_prepared_worker.h"
/* Startup-only crash localization. Full original draws still execute; the
 * first three rendered frames are serialized, so this is NOT an FPS build. */
static void startup_draw_probe(const char *site);
static void native_draw_submitted(const char *site);
#define C3D_DrawArrays(...) do { C3D_DrawArrays(__VA_ARGS__); native_draw_submitted(__func__); } while(0)
#define C3D_DrawElements(...) do { C3D_DrawElements(__VA_ARGS__); native_draw_submitted(__func__); } while(0)
#include "platform_native_surface.h"
#include "platform_pica_motion_blur.h"
#include "platform_pica_blur_filter.h"
#include "platform_pica_heat.h"
#include "platform_pica_distortion.h"
#include "platform_pica_water_setup.h"

enum { SPHERE_SLOTS=6,SPHERE_VERTICES=17*17,SPHERE_INDICES=16*16*6 };
typedef struct {const void *source;C3D_Tex texture;C3D_RenderTarget *target;unsigned valid;} SphereMap;
static SphereMap sphere_maps[SPHERE_SLOTS];
static FhModelVertex *sphere_vertices;
static u16 *sphere_indices;
static unsigned sphere_updates,sphere_update_failures,sphere_vram_bytes;
static u64 sphere_update_ticks;
enum { BASE_TEXTURES=512 };
#define BASE_GPU_LIMIT ((fh_hardware_large_memory?4u:2u)*1024u*1024u)
#define BASE_CPU_LIMIT ((fh_hardware_large_memory?4u:1u)*1024u*1024u)
typedef struct {
    FhTexture source;C3D_Tex gpu;
    void *linear_shadow; /* Immutable pre-promotion image, bounded by base GPU cap. */
    unsigned char *snapshot;
    unsigned snapshot_linear;
    unsigned image_bytes,palette_bytes,gpu_bytes,snapshot_bytes,valid,surface,scope;
    unsigned asset_slot,asset_generation,asset_miss_epoch;
    unsigned used_frame,stable_frames;
    u64 last_used;
} BaseTexture;
/* Old hardware has spare linear space but a tight normal-memory envelope.
 * These are CPU comparison copies, never GPU inputs or stdio destinations.
 * Both images and snapshots count against the same linear headroom gate. */
static void base_snapshot_free(BaseTexture *t){
    if(t->snapshot_linear)linearFree(t->snapshot);else free(t->snapshot);
    t->snapshot=NULL;t->snapshot_linear=0;
}
static BaseTexture base_textures[BASE_TEXTURES];
static MPTextureIndex base_index;
#define FH_WRITE_SLOTS BASE_TEXTURES
#include "fh_texture_write_index.h"
static FhTextureWriteIndex base_write_index;
static unsigned base_bucket(const FhTexture *s){
    return mp_texture_bucket((uint32_t)(uintptr_t)s->pixels,(uint32_t)(uintptr_t)s->palette,
        s->format,s->width,s->height);
}
static FhTexturePack texture_pack;
static void *texture_resident_storage;
static unsigned texture_resident_allocation,texture_resident_releases;
static void *texture_window_storage;
static unsigned texture_window_allocation,texture_window_releases;
static u64 texture_resident_startup_ticks,texture_resident_startup_read_bytes;
unsigned fh_minimal_texture_cache_release(void){
    unsigned bytes=texture_resident_allocation+texture_window_allocation;if(!bytes)return 0;
    /* CPU-only staging cache. GPU textures own independent published copies;
     * no GPU fence is needed to release these unpublished source bytes. */
    if(texture_resident_storage){
        fh_texture_pack_resident_drop(&texture_pack);linearFree(texture_resident_storage);
        texture_resident_storage=NULL;texture_resident_allocation=0;texture_resident_releases++;
    }
    if(texture_window_storage){
        fh_texture_pack_window_drop(&texture_pack);linearFree(texture_window_storage);
        texture_window_storage=NULL;texture_window_allocation=0;texture_window_releases++;
    }
    return bytes;
}
static void texture_window_start(unsigned required_free){
    const unsigned bytes=(fh_hardware_large_memory?8u:4u)*FH_TEXTURE_WINDOW_PAGE;
    if(texture_resident_storage || texture_window_storage || !texture_pack.file ||
       required_free>UINT32_MAX-bytes || linearSpaceFree()<required_free+bytes)return;
    void *storage=linearMemAlign(bytes,128);if(!storage)return;
    if(!fh_texture_pack_window(&texture_pack,storage,bytes)){linearFree(storage);return;}
    texture_window_storage=storage;texture_window_allocation=bytes;
}
void fh_minimal_texture_cache_trim(unsigned required_free){
    if(linearSpaceFree()>=required_free)return;
    fh_minimal_texture_cache_release();
    /* Reuse only a small part of the released budget. The caller's allocation
     * and existing headroom take priority; published GPU images are untouched. */
    texture_window_start(required_free);
}
static unsigned base_gpu_bytes,base_cpu_bytes,base_uploads,base_hits,base_failures,base_retired;
static u64 base_ticks,base_compare_bytes;
static unsigned base_cmpr16_uploads;
static u64 base_cmpr16_saved_bytes;
static u64 base_use_serial;
static unsigned base_live,base_peak,base_changes,base_slot_evictions,base_budget_evictions;
static unsigned texture_scope_serial,texture_scope,texture_scope_mode=1;
static u64 texture_scope_hits,texture_scope_checks;
#include "platform_texture_assets.h"
/* Audited synchronous map and callback-free model instruction loops open
 * this scope after their animation/setup. The map movie callback closes it
 * BEFORE external code. No reuse crosses callbacks, objects or blocks;
 * explicit source writes invalidate affected entries. Other producers still
 * compare every byte, including unannounced writes. */
void fh_minimal_texture_scope_end(void){texture_scope=0;}
void fh_minimal_texture_scope_begin(void){
    if(!++texture_scope_serial){
        for(unsigned i=0;i<BASE_TEXTURES;i++)base_textures[i].scope=0;
        texture_scope_serial=1;
    }
    texture_scope=texture_scope_mode?texture_scope_serial:0;
}

static void texture_visibility(const void *address,size_t bytes,int hint){
    if(ui_count && (fh_texture_source_overlap(&ui_source,address,bytes) ||
        (ui_mask && fh_texture_source_overlap(&ui_mask_source,address,bytes))))flush_ui();
#ifdef FH_TEXTURE_INDEX_VERIFY
    unsigned expected[BASE_TEXTURES];
    for(unsigned i=0;i<BASE_TEXTURES;i++)expected[i]=base_textures[i].valid &&
        (texture_asset_preserve_hint(base_textures+i,hint) ||
         !(base_textures[i].gpu_bytes && fh_texture_source_overlap(&base_textures[i].source,address,bytes)));
#endif
    uint32_t candidates[FH_WRITE_WORDS];
    fh_write_index_query(&base_write_index,address,bytes,candidates);
    for(unsigned w=0;w<FH_WRITE_WORDS;w++)for(uint32_t bits=candidates[w];bits;bits&=bits-1){
        unsigned i=w*32+(unsigned)__builtin_ctz(bits);
        if(i>=BASE_TEXTURES)continue;
        BaseTexture *t=base_textures+i;
        /* Modes4/5 separate GX cache hints from CPU stores only
         * for the file-loader ownership contract. Explicit range publication,
         * lifetime reuse and every unregistered/paletted producer stay dirty.
         * Mode5 independently compares every claimed reusable source. */
        if(texture_asset_preserve_hint(t,hint)){texture_asset_hint_retained++;continue;}
        if(t->gpu_bytes && (fh_write_range_overlap(t->source.pixels,t->image_bytes,address,bytes) ||
            fh_write_range_overlap(t->source.palette,t->palette_bytes,address,bytes)))t->valid=0;
    }
#ifdef FH_TEXTURE_INDEX_VERIFY
    for(unsigned i=0;i<BASE_TEXTURES;i++)if(base_textures[i].valid!=expected[i])
        fh_platform_panic("Native texture write index mismatch");
#endif
}

void fh_minimal_texture_written(const void *address,size_t bytes){texture_visibility(address,bytes,0);}
void fh_minimal_texture_hint(void){texture_visibility(NULL,0,1);}

/* As in pinned Melee, a GPU completion fence precedes storage reuse. Vblank
 * alone does not retire GPU reads. A slow fence is diagnostic, never permission
 * to exit or release allocations still referenced by the GPU. */
static void gpu_timeout_snapshot(void){
            gpu_work_report();
            /* Failure-only evidence: no disk IO or extra fences on good frames.
             * Queue counters can still advance; this is an observation, not a
             * synchronized declaration of which hardware engine stalled. */
            C3D_Context *ctx=C3Di_GetContext();gxCmdQueue_s *q=&ctx->gxQueue;
            unsigned count=q->numEntries,last=q->lastEntry,current=q->curEntry;
            char line[240];snprintf(line,sizeof(line),
                "GPU timeout queue: count=%u last=%u current=%u capacity=%u world_sampler_resets=%u\n",
                count,last,current,q->maxEntries,world_sampler_resets);fh_platform_log(line);
            if(q->entries && count<=q->maxEntries && count<=32)for(unsigned i=0;i<count;i++){
                const u32 *d=q->entries[i].data;
                snprintf(line,sizeof(line),"GPU queue %u: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
                    i,(unsigned long)d[0],(unsigned long)d[1],(unsigned long)d[2],(unsigned long)d[3],
                    (unsigned long)d[4],(unsigned long)d[5],(unsigned long)d[6],(unsigned long)d[7]);fh_platform_log(line);
            }
            /* Preserve the small queue record before slow failure-only SD
             * snapshots. A power-cycle during the input dump must not erase
             * the identity and bounds of the submitted command lists. */
            fh_platform_gpu_wait_log_flush();
            if(ctx->cmdBuf && ctx->cmdBufSize && ctx->cmdBufSize<=65536){
                FILE *f=fopen("sdmc:/3ds/foxhollow/gpu-timeout.bin","wb");
                if(f){size_t words=fwrite(ctx->cmdBuf,4,ctx->cmdBufSize,f);int closed=fclose(f);
                    snprintf(line,sizeof(line),"GPU timeout command snapshot: words=%u expected=%u close=%d\n",
                        (unsigned)words,(unsigned)ctx->cmdBufSize,closed);fh_platform_log(line);}
            }
            fh_platform_gpu_wait_log_flush();
            /* One attempt per process: no recurring large SD write after a
             * recoverable slow fence. CPU-owned inputs remain retained by the
             * existing fence. GPU outputs may be stale; no GPU readback here. */
            static unsigned inputs_attempted;
            if(!inputs_attempted){
                inputs_attempted=1;
                extern u32 __ctru_linear_heap,__ctru_linear_heap_size;
                unsigned char *scratch=malloc(FH_GPU_SNAPSHOT_CHUNK);
                FILE *f=scratch?fopen("sdmc:/3ds/foxhollow/gpu-inputs.bin","wb"):NULL;
                int ok=0,closed=-1;
                if(f){
                    ok=fh_gpu_input_snapshot(f,(const unsigned char *)(uintptr_t)__ctru_linear_heap,
                        __ctru_linear_heap,osConvertVirtToPhys((void *)(uintptr_t)__ctru_linear_heap),
                        __ctru_linear_heap_size,(u32)(uintptr_t)ctx->cmdBuf,
                        osConvertVirtToPhys(ctx->cmdBuf),ctx->cmdBufSize*4,scratch);
                    closed=fclose(f);
                }
                free(scratch);
                snprintf(line,sizeof(line),"GPU timeout CPU inputs: bytes=%lu complete=%d close=%d; CPU view only\n",
                    (unsigned long)__ctru_linear_heap_size,ok,closed);fh_platform_log(line);
            }
}
/* Blocking private-SDK waits, including capacity drains and target teardown.
 * The completion lock is not held here. FrameBegin's nonblocking caller keeps
 * its existing observation below. Diagnostics never return GPU ownership. */
void fh_native_gpu_wait_diagnostic(u64 start,unsigned *warned){
    if(prepared_on_worker()){fh_audio_worker_service_current();return;}
    if(!*warned && svcGetSystemTick()-start>3ULL*SYSCLOCK_ARM11){
        *warned=1;
        fh_platform_log("GPU queue wait exceeded three seconds; retaining resources and waiting for completion\n");
        if(initialized)gpu_timeout_snapshot();
        fh_platform_gpu_wait_log_flush();
    }
    if(*warned)fh_platform_gpu_wait_poll();
}
static void acquire(void){
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    unsigned replay_phase=prepared_on_worker()?FH_PHASE_COUNT:fh_replay_phase(FH_PHASE_GPU_WAIT);
#endif
    u64 start=svcGetSystemTick();unsigned warned=0;
    while(!C3D_FrameBegin(C3D_FRAME_NONBLOCK)){
        if(!prepared_on_worker() && !warned && svcGetSystemTick()-start>3ULL*SYSCLOCK_ARM11){
            warned=1;
            fh_platform_log("GPU wait exceeded three seconds; retaining resources and waiting for completion\n");
            gpu_timeout_snapshot();
            fh_graphics_gpu_quarantine();
            fh_platform_gpu_wait_log_flush();
        }
        if(warned)fh_platform_gpu_wait_poll();
        if(prepared_on_worker()){fh_audio_worker_service_current();svcSleepThread(1000000);}
        else (void)gxCmdQueueWait(&C3Di_GetContext()->gxQueue,SYSCLOCK_ARM11/1000);
    }
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    if(replay_phase!=FH_PHASE_COUNT)fh_replay_phase(replay_phase);
#endif
    fh_early_queue_retire();queue_draws=0;
    if(!fh_pica_packet_frame_retired())fh_platform_panic("PICA packet retirement ownership");
    fh_pica_scene_invalidate();
    world_stream.used=world_stream.published=0; /* Successful FrameBegin retired every old reader. */
    if(warned){
        char line[160];snprintf(line,sizeof(line),"GPU wait recovered after %llu ms\n",
            (unsigned long long)((svcGetSystemTick()-start)*1000/SYSCLOCK_ARM11));
        fh_platform_log(line);fh_platform_gpu_wait_log_flush();
    }
}
#include "platform_texture_vram.h"
static void begin(void){
    if(open_frame)return;
    acquire();base_vram_begin();open_frame=1;ui_used=0;
    native_surface_draw_on(target);
    C3D_RenderTargetClear(target,C3D_CLEAR_ALL,clear_color,0xffffff-clear_depth);
}
static void retire_base(BaseTexture *t){
    if(!t->gpu_bytes)return;
    u64 retire_start=svcGetSystemTick();
    fh_minimal_sync(); /* Includes queued reads, not merely vblank. */
    retire_syncs++;retire_sync_ticks+=svcGetSystemTick()-retire_start;
    mp_texture_index_remove(&base_index,base_bucket(&t->source),(unsigned)(t-base_textures));
    fh_write_index_remove(&base_write_index,(unsigned)(t-base_textures));
    base_vram_forget(t);C3D_TexDelete(&t->gpu);base_snapshot_free(t);
    base_gpu_bytes-=t->gpu_bytes;base_cpu_bytes-=t->snapshot_bytes;
    memset(t,0,sizeof(*t));base_retired++;base_live--;
}
static int base_key(const FhTexture *a,const FhTexture *b){
    return a->pixels==b->pixels && a->palette==b->palette && a->width==b->width &&
        a->height==b->height && a->format==b->format && a->palette_format==b->palette_format &&
        a->palette_count==b->palette_count;
}
static void startup_draw_probe(const char *site){
    if(fh_minimal_frames>2)return;
    static unsigned serial;
    unsigned draw=serial++;char line[180];
    snprintf(line,sizeof(line),"Startup GPU draw begin: frame=%u draw=%u site=%s\n",fh_minimal_frames,draw,site);fh_platform_log(line);
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    gxCmdQueue_s *q=&C3Di_GetContext()->gxQueue;
    u64 start=svcGetSystemTick();unsigned warned=0;gxCmdQueueRun(q);
    while(!gxCmdQueueWait(q,0)){
        if(!prepared_on_worker() && !warned && svcGetSystemTick()-start>3ULL*SYSCLOCK_ARM11){
            warned=1;
            snprintf(line,sizeof(line),"Startup GPU draw timeout: frame=%u draw=%u site=%s\n",fh_minimal_frames,draw,site);fh_platform_log(line);
            gpu_timeout_snapshot();fh_platform_gpu_wait_log_flush();
        }
        if(warned)fh_platform_gpu_wait_poll();
        svcSleepThread(100000);
    }
    snprintf(line,sizeof(line),"Startup GPU draw complete: frame=%u draw=%u site=%s\n",fh_minimal_frames,draw,site);fh_platform_log(line);
}
static void native_draw_submitted(const char *site){
    if(fh_minimal_frames<=2){startup_draw_probe(site);return;}
    /* Producers flush vertex/texture ranges; appended world indices are
     * cleaned by the command-publication wrapper before GPU admission.
     * FrameSplit publishes an immutable command prefix; the existing Melee
     * queue adapter reserves presentation space and never recycles storage.
     * CPU preparation can continue while this prefix executes. */
    if(++queue_draws==8){
        queue_draws=0;queue_submissions+=fh_early_queue_append()!=0;
    }
}
int fh_minimal_linear_room(unsigned bytes){
    if(bytes>UINT32_MAX-FH_NATIVE_LINEAR_RESERVE)return 0;
    if(linearSpaceFree()<bytes+FH_NATIVE_LINEAR_RESERVE)fh_minimal_texture_cache_trim(bytes+FH_NATIVE_LINEAR_RESERVE);
    while(linearSpaceFree()<bytes+FH_NATIVE_LINEAR_RESERVE){
        BaseTexture *victim=NULL;
        for(unsigned i=0;i<BASE_TEXTURES;i++)if(base_textures[i].gpu_bytes &&
            &base_textures[i].gpu!=base_pin && &base_textures[i].gpu!=second_pin &&
            (!victim || base_textures[i].last_used<victim->last_used))victim=base_textures+i;
        if(!victim)return 0;
        retire_base(victim);
    }
    return 1;
}
static float base_uv_scale(unsigned logical,unsigned physical){
    return (float)logical/(physical<logical?fh_texture_extent(logical):physical);
}
static C3D_Tex *base_texture_mode(const FhTexture *s,unsigned surface){
    FH_REPLAY_SCOPE(TEXTURE_RESOLVE);
    u64 start=svcGetSystemTick();
    if(!s || !s->pixels)return NULL;
    for(unsigned i=0;i<SPHERE_SLOTS;i++)if(sphere_maps[i].valid && sphere_maps[i].source==s->pixels &&
        s->width==32 && s->height==32 && s->format==6)return &sphere_maps[i].texture;
    unsigned image=fh_texture_source_bytes(s->format,s->width,s->height);
    if(!image || s->palette_count>16384){base_failures++;return NULL;}
    unsigned palette=s->palette?s->palette_count*2:0;
    unsigned image_at=(uintptr_t)s->pixels&3,palette_at=(image_at+image+3)&~3u;
    palette_at+=(uintptr_t)s->palette&3;
    unsigned snapshot=palette_at+palette;
    BaseTexture *t=NULL,*oldest=NULL;
    for(unsigned link=base_index.head[base_bucket(s)];link;link=base_index.next[link-1]){
        BaseTexture *q=base_textures+link-1;
        if(q->surface==surface && base_key(&q->source,s)){t=q;break;}
    }
#ifdef FH_TEXTURE_INDEX_VERIFY
    BaseTexture *linear=NULL;
    for(unsigned i=0;i<BASE_TEXTURES;i++)if(base_textures[i].gpu_bytes &&
        base_textures[i].surface==surface && base_key(&base_textures[i].source,s)){linear=base_textures+i;break;}
    if(t!=linear)fh_platform_panic("Native texture index mismatch");
#endif
    if(!t)for(unsigned i=0;i<BASE_TEXTURES;i++){
        BaseTexture *q=base_textures+i;
        if(&q->gpu==base_pin || &q->gpu==second_pin)continue;
        if(!oldest || !q->gpu_bytes || (oldest->gpu_bytes && q->last_used<oldest->last_used))oldest=q;
    }
    if(t){
        if(texture_asset_mode && t->valid && texture_asset_current(t)){
            if(texture_asset_mode==2 || texture_asset_mode==5){
                texture_asset_checks++;
                if(fh_memory_compare(t->snapshot+image_at,s->pixels,image))
                    fh_platform_panic("File texture changed without publication");
            }
            texture_asset_hits++;base_hits++;goto ready;
        }
        if(texture_scope && t->valid && t->scope==texture_scope){
            if(texture_scope_mode==2){
                texture_scope_checks++;
                if(fh_memory_compare(t->snapshot+image_at,s->pixels,image) ||
                    (palette && fh_memory_compare(t->snapshot+palette_at,s->palette,palette)))
                    fh_platform_panic("Texture changed inside audited render scope");
            }
            texture_scope_hits++;base_hits++;goto ready;
        }
        /* Content equality is checked at every scope entrance, even for
         * uninstrumented writers or recycled game addresses. Snapshots share
         * source alignment on ARM. */
        base_compare_bytes+=image+palette;
        if(!fh_memory_compare(t->snapshot+image_at,s->pixels,image) &&
            (!palette || !fh_memory_compare(t->snapshot+palette_at,s->palette,palette))){
            /* GXInvalidateTexAll also occurs with unchanged source bytes.
             * Our private GPU copy remains valid after that broad hint. */
            t->valid=1;texture_asset_attach(t);base_hits++;goto ready;
        }
        base_changes++;retire_base(t);
    }else t=oldest;
    if(!t)goto failed;
    if(t->gpu_bytes){base_slot_evictions++;retire_base(t);}
    unsigned width=fh_texture_extent(s->width),height=fh_texture_extent(s->height);
    unsigned format=fh_texture_native_format(s),bytes=fh_texture_native_bytes(s,width,height);
    FhPackedTexture packed={0};
    int prepared=s->format==14 && (surface?fh_texture_pack_find_surface:fh_texture_pack_find)(&texture_pack,s->pixels,s->width,s->height,&packed);
    if(prepared){format=packed.format;bytes=packed.bytes;width=packed.width;height=packed.height;}
    /* Unprepared CMPR otherwise expands eightfold to RGBA8, repeatedly
     * evicting the scene's working set. Native RGB5A1 preserves binary alpha
     * and reduces each RGB channel by at most four out of 255. Same texels,
     * dimensions, one texture/pass; no extra resident allocation or SD IO. */
    unsigned cmpr16=!prepared && s->format==14;
    if(cmpr16){format=2;bytes=width*height*2;} /* GPU_RGBA5551 */
    extern u32 __ctru_heap_size,__stacksize__;
    if(!bytes || bytes>BASE_GPU_LIMIT || snapshot>BASE_CPU_LIMIT)goto failed;
    unsigned snapshot_linear=!fh_hardware_large_memory;
    unsigned linear_required=bytes+FH_NATIVE_LINEAR_RESERVE+(snapshot_linear?snapshot+128:0);
    unsigned normal_required=snapshot_linear?0:snapshot+64;
    if(linearSpaceFree()<linear_required)fh_minimal_texture_cache_trim(linear_required);
    if(((u64)(unsigned)mallinfo().uordblks+__stacksize__+normal_required)*100>(u64)__ctru_heap_size*85)
        fh_native_scene_cache_release();
    while(base_gpu_bytes+bytes>BASE_GPU_LIMIT || base_cpu_bytes+snapshot>BASE_CPU_LIMIT ||
          linearSpaceFree()<linear_required ||
          ((u64)(unsigned)mallinfo().uordblks+__stacksize__+normal_required)*100>(u64)__ctru_heap_size*85){
        BaseTexture *victim=NULL;
        for(unsigned i=0;i<BASE_TEXTURES;i++)if(base_textures[i].gpu_bytes && &base_textures[i].gpu!=base_pin && &base_textures[i].gpu!=second_pin &&
            (!victim || base_textures[i].last_used<victim->last_used))victim=base_textures+i;
        if(!victim)goto failed;
        base_budget_evictions++;retire_base(victim);
    }
    t->snapshot_linear=snapshot_linear;
    t->snapshot=snapshot_linear?linearMemAlign(snapshot,128):malloc(snapshot);
    if(!t->snapshot)goto failed;
    if(!C3D_TexInit(&t->gpu,width,height,(GPU_TEXCOLOR)format)){base_snapshot_free(t);goto failed;}
    if(!(prepared?fh_texture_pack_read(&texture_pack,&packed,t->gpu.data,bytes):
        cmpr16?fh_texture_convert_cmpr16(t->gpu.data,bytes,width,height,s,image):
        fh_texture_convert(t->gpu.data,bytes,width,height,s,image))){
        C3D_TexDelete(&t->gpu);base_snapshot_free(t);memset(t,0,sizeof(*t));goto failed;
    }
    memcpy(t->snapshot+image_at,s->pixels,image);
    if(palette)memcpy(t->snapshot+palette_at,s->palette,palette);
    fh_gpu_cache_flush(t->gpu.data,bytes);
    t->source=*s;t->surface=surface;t->snapshot_bytes=snapshot;t->image_bytes=image;t->palette_bytes=palette;
    t->gpu_bytes=bytes;t->valid=1;base_gpu_bytes+=bytes;base_cpu_bytes+=snapshot;base_uploads++;
    texture_asset_attach(t);
    mp_texture_index_add(&base_index,base_bucket(s),(unsigned)(t-base_textures));
    fh_write_index_add(&base_write_index,(unsigned)(t-base_textures),s->pixels,image);
    fh_write_index_add(&base_write_index,(unsigned)(t-base_textures),s->palette,palette);
    base_live++;if(base_live>base_peak)base_peak=base_live;
    if(cmpr16){base_cmpr16_uploads++;base_cmpr16_saved_bytes+=bytes;}
ready:
    t->scope=texture_scope;
    if(t->used_frame!=fh_minimal_frames+1){
        t->stable_frames=t->used_frame==fh_minimal_frames && t->stable_frames<UINT32_MAX?
            t->stable_frames+1:1;
        t->used_frame=fh_minimal_frames+1;
    }
    if(base_on_vram(t))base_vram_hits++;else base_linear_hits++;
    /* Many materials are visited in one frame. Frame-only ties otherwise
     * evict a newly uploaded texture repeatedly while older entries survive.
     * More small entries share the SAME byte caps (pinned Melee's policy). */
    t->last_used=++base_use_serial;
    static const GPU_TEXTURE_WRAP_PARAM wraps[]={GPU_CLAMP_TO_EDGE,GPU_REPEAT,GPU_MIRRORED_REPEAT};
    C3D_TexSetWrap(&t->gpu,wraps[s->wrap_s%3],wraps[s->wrap_t%3]);
    fh_pica_sampler_filter(&t->gpu,s->min_filter,s->mag_filter);
    base_ticks+=svcGetSystemTick()-start;return &t->gpu;
failed:
    base_failures++;base_ticks+=svcGetSystemTick()-start;return NULL;
}
static C3D_Tex *base_texture(const FhTexture *s){return base_texture_mode(s,0);}
static C3D_Tex *surface_texture(const FhTexture *s){return base_texture_mode(s,1);}
static void bind_vertices_program(const FhModelVertex *vertices,unsigned compact,unsigned format,shaderProgram_s *selected){
    C3D_BindProgram(selected);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    if(compact){
        AttrInfo_AddLoader(a,0,GPU_SHORT,4);AttrInfo_AddLoader(a,1,GPU_UNSIGNED_BYTE,4);
        AttrInfo_AddLoader(a,2,GPU_SHORT,4);AttrInfo_AddLoader(a,3,GPU_BYTE,4);
        AttrInfo_AddLoader(a,4,GPU_UNSIGNED_BYTE,4);AttrInfo_AddLoader(a,5,GPU_FLOAT,3);
    }else{
        AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);
        AttrInfo_AddLoader(a,2,GPU_FLOAT,2);AttrInfo_AddLoader(a,3,GPU_FLOAT,3);AttrInfo_AddLoader(a,4,GPU_FLOAT,3);
        AttrInfo_AddFixed(a,5);C3D_FixedAttribSet(5,0,0,0,0);
    }
    for(unsigned i=6;i<12;i++){
        if(AttrInfo_AddFixed(a,i)<0)fh_platform_panic("Native lighting attribute limit");
        /* Never inherit GPU register contents from a previous application or
         * draw. Lit groups overwrite these after binding; unlit groups don't. */
        C3D_FixedAttribSet(i,0,0,0,0);
    }
    /* Pinned Melee renderer.c hardware fix: this SDK shifts a 32-bit int
     * in AddFixed. Slots8..11 need an explicit 64-bit identity permutation. */
    a->permutation=UINT64_C(0xba9876543210);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,73,compact?(format==7?1.f:format==5?1.f/8:1.f/256):1,
        compact?(format==5?1.f/256:1.f/1024):1,compact?1.f/15:1,compact?.25f:1);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,vertices,compact?sizeof(FhModelCompactVertex):sizeof(FhModelVertex),
        compact?6:5,compact?0x543210:0x43210)<0)fh_platform_panic("Minimal geometry binding");
}
static void bind_vertices(const FhModelVertex *vertices,unsigned compact,unsigned format){
    bind_vertices_program(vertices,compact,format,&program);
}
static void submit_prepared_ui(const FhPreparedUi *p){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Prepared UI command bounds");
    if((capacity-offset)*4<16*1024)owner_gpu_sync();
    unsigned count=p->count,ui_effect=p->effect,ui_effect_blend=p->effect_blend;
    unsigned ui_alpha_only=p->alpha_only,ui_additive=p->additive,ui_scissored=p->scissored;
    const unsigned *ui_clip=p->clip,*effect_clip=p->effect_clip;
    const FhTexture ui_source=p->source,ui_mask_source=p->mask_source;
    static C3D_Tex samplers[2];memcpy(samplers,p->samplers,sizeof(samplers));
    C3D_Tex *ui_texture=p->texture?samplers:NULL,*ui_mask=p->mask?samplers+1:NULL;
    const float (*effect_view)[4]=p->view,(*effect_projection)[4]=p->projection;
    float effect_near=p->near_z,effect_far=p->far_z;
    fh_native_fog_apply(0); /* HUD/effect batches never inherit scene fog. */
    /* UI vertices have already been clipped in logical screen coordinates. */
    if(ui_effect)C3D_SetScissor(GPU_SCISSOR_NORMAL,effect_clip[0],effect_clip[1],effect_clip[2],effect_clip[3]);
    else if(ui_scissored)C3D_SetScissor(GPU_SCISSOR_NORMAL,ui_clip[0],ui_clip[1],ui_clip[2],ui_clip[3]);
    else C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
    C3D_BindProgram(ui_effect?&world_program:&ui_program);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);
    AttrInfo_AddLoader(a,2,GPU_FLOAT,2);
    if(ui_effect){AttrInfo_AddFixed(a,3);C3D_FixedAttribSet(3,0,0,0,1);}
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,p->vertices,sizeof(*ui_vertices),3,0x210)<0)
        fh_platform_panic("Minimal UI binding");
    C3D_FVUnifSet(GPU_VERTEX_SHADER,0,ui_texture?(float)ui_source.width/ui_texture->width:1,
        ui_texture?(float)ui_source.height/ui_texture->height:1,0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,1,ui_mask?(float)ui_mask_source.width/ui_mask->width:1,
        ui_mask?(float)ui_mask_source.height/ui_mask->height:1,0,0);
    if(ui_effect){
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,false);
        for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,effect_view[r][0],effect_view[r][1],effect_view[r][2],effect_view[r][3]);
        for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,effect_projection[r][0],effect_projection[r][1],effect_projection[r][2],effect_projection[r][3]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,7,1,1,1,1);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,8,ui_texture?(float)ui_source.width/ui_texture->width:1,
            ui_texture?(float)ui_source.height/ui_texture->height:1,0,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,9,1,1,0,0);
    }
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnv *env=C3D_GetTexEnv(0);
    C3D_TexEnvSrc(env,C3D_Both,GPU_PRIMARY_COLOR,ui_texture?GPU_TEXTURE0:0,0);
    C3D_TexEnvFunc(env,C3D_RGB,ui_texture && !ui_alpha_only?GPU_MODULATE:GPU_REPLACE);
    C3D_TexEnvFunc(env,C3D_Alpha,ui_texture?GPU_MODULATE:GPU_REPLACE);
    if(ui_texture && (ui_source.format==0 || ui_source.format==1))
        C3D_TexEnvOpAlpha(env,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
    if(ui_additive==FH_MINIMAL_SKY_TINT){
        /* Original sky TEV: texture*(1-C1.a) + C1.rgb*C1.a, texture alpha.
         * Zero tint alpha means a fully visible untinted backdrop. */
        C3D_TexEnvSrc(env,C3D_RGB,GPU_PRIMARY_COLOR,GPU_TEXTURE0,GPU_PRIMARY_COLOR);
        C3D_TexEnvOpRgb(env,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(env,C3D_RGB,GPU_INTERPOLATE);
        C3D_TexEnvSrc(env,C3D_Alpha,GPU_TEXTURE0,0,0);
        C3D_TexEnvFunc(env,C3D_Alpha,GPU_REPLACE);
    }
    C3D_TexBind(0,ui_texture);
    C3D_TexBind(1,ui_mask);
    fh_pica_texture2_coord1(0);C3D_TexBind(2,NULL);
    if(ui_mask){
        /* Original secondary image replaces the base alpha with mask*fade. */
        env=C3D_GetTexEnv(1);
        C3D_TexEnvSrc(env,C3D_RGB,GPU_PREVIOUS,0,0);C3D_TexEnvFunc(env,C3D_RGB,GPU_REPLACE);
        C3D_TexEnvSrc(env,C3D_Alpha,GPU_PRIMARY_COLOR,GPU_TEXTURE1,0);
        C3D_TexEnvFunc(env,C3D_Alpha,GPU_MODULATE);
        if(ui_mask_source.format==0 || ui_mask_source.format==1)
            C3D_TexEnvOpAlpha(env,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
    }
    C3D_CullFace(GPU_CULL_NONE);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    if(ui_additive==FH_MINIMAL_SKY_TINT)
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    else C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,ui_additive?GPU_ONE:GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
    if(ui_effect){
        C3D_DepthMap(true,-(effect_far-effect_near),1-effect_far);
        C3D_DepthTest(true,GPU_GEQUAL,ui_effect_blend?GPU_WRITE_COLOR:GPU_WRITE_ALL);
        C3D_AlphaTest(!ui_effect_blend,GPU_GREATER,0xfe);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,ui_effect_blend?GPU_SRC_ALPHA:GPU_ONE,
            ui_effect_blend==1?GPU_ONE:ui_effect_blend==2?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
        effect_draws++;
    }
    C3D_DrawArrays(GPU_TRIANGLES,0,count);
    if(ui_effect)C3D_DepthMap(true,-1,0);
    frame_draws++;frame_indices+=count;__atomic_add_fetch(&fh_minimal_draws,1,__ATOMIC_RELAXED);__atomic_add_fetch(&fh_minimal_indices,count,__ATOMIC_RELAXED);ui_draws++;
}
static void flush_ui(void){
    if(!ui_count)return;
    FhPreparedUi local;FhPreparedPayload *slot=prepared_reserve(PREPARED_UI);
    FhPreparedUi *ready=slot?&slot->ui:&local;
    *ready=(FhPreparedUi){.vertices=ui_vertices+ui_first,.count=ui_count,
        .texture=ui_texture!=NULL,.mask=ui_mask!=NULL,.alpha_only=ui_alpha_only,
        .additive=ui_additive,.scissored=ui_scissored,.effect=ui_effect,.effect_blend=ui_effect_blend,
        .source=ui_source,.mask_source=ui_mask_source,.near_z=effect_near,.far_z=effect_far};
    if(ui_texture)ready->samplers[0]=*ui_texture;
    if(ui_mask)ready->samplers[1]=*ui_mask;
    memcpy(ready->clip,ui_clip,sizeof(ready->clip));memcpy(ready->effect_clip,effect_clip,sizeof(ready->effect_clip));
    memcpy(ready->view,effect_view,sizeof(ready->view));memcpy(ready->projection,effect_projection,sizeof(ready->projection));
    ui_count=0;fh_gpu_cache_flush(ready->vertices,ready->count*sizeof(*ui_vertices));
    if(slot)prepared_commit(PREPARED_UI);else submit_prepared_ui(ready);
}

void fh_minimal_image(const FhTexture *texture,const FhTexture *mask,const float matrix[4][4],
    const float rect[4],const float uv[4],unsigned rgba,unsigned alpha_only,unsigned additive,const unsigned scissor[4]){
    if(fh_pica_scene_enabled)return;
    ui_attempts++;
    if(!initialized || (!(rgba&255) && additive!=FH_MINIMAL_SKY_TINT))return;
    /* HUD positions are orthographic. Clip in logical 640x480 screen space
     * and interpolate UVs before the LCD rotation, preserving text scissors. */
    if(matrix[3][0] || matrix[3][1] || matrix[3][2] || matrix[3][3]!=1){fh_minimal_rejected++;return;}
    float x[2],y[2];
    for(unsigned i=0;i<2;i++){
        /* GX_VTXFMT1 positions have two fractional bits. The game supplies
         * quarter-pixel s16 coordinates, including its -8 depth value. */
        x[i]=matrix[0][0]*(rect[i*2]*.25f)+matrix[0][1]*(rect[i*2+1]*.25f)-2*matrix[0][2]+matrix[0][3];
        y[i]=matrix[1][0]*(rect[i*2]*.25f)+matrix[1][1]*(rect[i*2+1]*.25f)-2*matrix[1][2]+matrix[1][3];
    }
    if(ui_attempts==1){char report[256];snprintf(report,sizeof(report),
        "Minimal first UI: rect=%.2f,%.2f,%.2f,%.2f clip=%.3f,%.3f,%.3f,%.3f scissor=%u,%u,%u,%u\n",
        rect[0],rect[1],rect[2],rect[3],x[0],y[0],x[1],y[1],scissor[0],scissor[1],scissor[2],scissor[3]);fh_platform_log(report);}
    if(x[0]>=x[1] || y[0]<=y[1]){ui_empty++;return;}
    float left=scissor[0]/320.f-1,right=(scissor[0]+scissor[2])/320.f-1;
    float top=1-scissor[1]/240.f,bottom=1-(scissor[1]+scissor[3])/240.f;
    if(left<-1)left=-1;
    if(right>1)right=1;
    if(top>1)top=1;
    if(bottom<-1)bottom=-1;
    float nx[2]={x[0]>left?x[0]:left,x[1]<right?x[1]:right};
    float ny[2]={y[0]<top?y[0]:top,y[1]>bottom?y[1]:bottom};
    if(nx[0]>=nx[1] || ny[0]<=ny[1]){ui_empty++;return;}
    float u[2],v[2];
    for(unsigned i=0;i<2;i++){
        u[i]=uv[0]+(uv[2]-uv[0])*(nx[i]-x[0])/(x[1]-x[0]);
        v[i]=uv[1]+(uv[3]-uv[1])*(ny[i]-y[0])/(y[1]-y[0]);
    }
    begin();
    if(ui_count && (ui_effect || ui_scissored || alpha_only!=ui_alpha_only || additive!=ui_additive || !!texture!=!!ui_texture ||
        !!mask!=!!ui_mask || (mask && memcmp(&ui_mask_source,mask,sizeof(*mask))) ||
        (texture && (memcmp(&ui_source,texture,sizeof(*texture))))))flush_ui();
    if(ui_used+6>UI_VERTICES){flush_ui();fh_minimal_sync();}
    if(!ui_count){
        ui_texture=base_texture(texture);ui_source=texture?*texture:(FhTexture){0};
        if(texture && !ui_texture)return;
        /* Acquiring the second image may evict cache entries; keep the first
         * binding alive even before this batch has emitted a GPU command. */
        base_pin=ui_texture;ui_mask=base_texture(mask);base_pin=NULL;
        if(mask && !ui_mask)return;
        ui_mask_source=mask?*mask:(FhTexture){0};ui_additive=additive;
        ui_alpha_only=alpha_only;ui_first=ui_used;ui_effect=0;ui_scissored=0;
    }
    static const unsigned corners[6]={0,1,2,0,2,3};
    static const unsigned cx[4]={0,1,1,0},cy[4]={0,0,1,1};
    for(unsigned i=0;i<6;i++){
        unsigned c=corners[i];MinimalUiVertex *out=ui_vertices+ui_used++;
        *out=(MinimalUiVertex){.position={ny[cy[c]],-nx[cx[c]],-.5f,1},
            .color={(rgba>>24)/255.f,((rgba>>16)&255)/255.f,((rgba>>8)&255)/255.f,(rgba&255)/255.f},
            .uv={u[cx[c]],v[cy[c]]}};
    }
    ui_count+=6;ui_quads++;
}
/* Custom menu glyphs/icons use the original 3D UI view. Keep homogeneous W
 * through the existing UI shader so the PICA clips and interpolates the image. */
void fh_minimal_projected_image(const FhTexture *texture,const FhTexture *mask,
    const float view[3][4],const float rect[4],float depth,const float uv[4],
    unsigned rgba,unsigned alpha_only,unsigned additive,const unsigned scissor[4]){
    if(fh_pica_scene_enabled)return;
    if(!initialized || !(rgba&255))return;
    unsigned clip[4];
    if(additive>1 || !view || !rect || !uv || !scissor){fh_minimal_rejected++;return;}
    if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return;
    float position[4][4];
    if(!fh_native_ui_project(position,projection,view,rect,depth)){fh_minimal_rejected++;return;}
    begin();
    if(ui_count && (ui_effect || !ui_scissored || memcmp(ui_clip,clip,sizeof(clip)) ||
        alpha_only!=ui_alpha_only || additive!=ui_additive || !!texture!=!!ui_texture ||
        !!mask!=!!ui_mask || (texture && memcmp(&ui_source,texture,sizeof(*texture))) ||
        (mask && memcmp(&ui_mask_source,mask,sizeof(*mask)))))flush_ui();
    if(ui_used+6>UI_VERTICES){flush_ui();fh_minimal_sync();}
    if(!ui_count){
        ui_texture=base_texture(texture);ui_source=texture?*texture:(FhTexture){0};
        if(texture && !ui_texture){fh_minimal_rejected++;return;}
        base_pin=ui_texture;ui_mask=base_texture(mask);base_pin=NULL;
        if(mask && !ui_mask){fh_minimal_rejected++;return;}
        ui_mask_source=mask?*mask:(FhTexture){0};ui_additive=additive;
        ui_alpha_only=alpha_only;ui_first=ui_used;ui_effect=0;ui_scissored=1;
        memcpy(ui_clip,clip,sizeof(clip));
    }
    static const unsigned corners[6]={0,1,2,0,2,3};
    static const unsigned cx[4]={0,1,1,0},cy[4]={0,0,1,1};
    for(unsigned i=0;i<6;i++){
        unsigned c=corners[i];MinimalUiVertex *out=ui_vertices+ui_used++;
        memcpy(out->position,position[c],sizeof(out->position));
        for(unsigned j=0;j<4;j++)out->color[j]=((rgba>>(24-8*j))&255)/255.f;
        out->uv[0]=uv[cx[c]*2];out->uv[1]=uv[cy[c]*2+1];
    }
    ui_count+=6;ui_quads++;ui_projected_quads++;
}
void fh_minimal_quad(const FhTexture *texture,const float matrix[4][4],
    const float rect[4],const float uv[4],unsigned rgba,unsigned alpha_only,const unsigned scissor[4]){
    fh_minimal_image(texture,NULL,matrix,rect,uv,rgba,alpha_only,0,scissor);
}
static unsigned effect_triangles;
static void effect_vertices(const FhTexture *texture,const FhMinimalEffectVertex *vertices,unsigned count,
    const float view[3][4],unsigned blend,float near_z,float far_z,const unsigned scissor[4]){
    if(!initialized || fh_render_budget_world)return;
    unsigned clip[4];
    if(!vertices || !view || blend>2 || !isfinite(near_z) || !isfinite(far_z) || far_z<=near_z){fh_minimal_rejected++;return;}
    if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return;
    float projected[4][4];memcpy(projected,projection,sizeof(projected));
    /* Preserve camera clip planes. DepthMap applies the viewport range after
     * clipping; changing projection z here would incorrectly clip particles. */
    begin();
    if(ui_count && (!ui_effect || ui_effect_blend!=blend || effect_near!=near_z || effect_far!=far_z ||
        memcmp(effect_view,view,sizeof(effect_view)) || memcmp(effect_projection,projected,sizeof(projected)) ||
        memcmp(effect_clip,clip,sizeof(clip)) || !!texture!=!!ui_texture ||
        (texture && memcmp(&ui_source,texture,sizeof(*texture)))))flush_ui();
    if(ui_used+count>UI_VERTICES){flush_ui();fh_minimal_sync();}
    if(!ui_count){
        ui_texture=base_texture(texture);ui_source=texture?*texture:(FhTexture){0};
        if(texture && !ui_texture){fh_minimal_rejected++;return;}
        ui_mask=NULL;ui_mask_source=(FhTexture){0};ui_effect=1;ui_effect_blend=blend;
        ui_alpha_only=0;ui_additive=0;ui_first=ui_used;
        memcpy(effect_view,view,sizeof(effect_view));memcpy(effect_projection,projected,sizeof(projected));
        memcpy(effect_clip,clip,sizeof(clip));effect_near=near_z;effect_far=far_z;
    }
    static const unsigned corners[6]={0,1,2,0,2,3};
    for(unsigned i=0;i<count;i++)memcpy(ui_vertices+ui_used++,vertices+corners[i],sizeof(*ui_vertices));
    ui_count+=count;effect_triangles+=count/3;
    if(count==6)effect_quads++;
}
void fh_minimal_effect(const FhTexture *texture,const FhMinimalEffectVertex vertices[4],
    const float view[3][4],unsigned blend,float near_z,float far_z,const unsigned scissor[4]){
    effect_vertices(texture,vertices,6,view,blend,near_z,far_z,scissor);
}
void fh_minimal_effect_triangle(const FhTexture *texture,const FhMinimalEffectVertex vertices[3],
    const float view[3][4],unsigned blend,float near_z,float far_z,const unsigned scissor[4]){
    effect_vertices(texture,vertices,3,view,blend,near_z,far_z,scissor);
}
void fh_minimal_init(void){
    if(initialized)return;
    if(fh_pica_scene_enabled && (!fh_render_budget_world || fh_render_budget_models || fh_render_budget_textures))
        fh_platform_panic("PICA scene must not enable legacy tiers");
    FILE *vram_config=fopen("sdmc:/3ds/foxhollow/texture_vram.txt","r");
    if(vram_config){
        unsigned mode,start;
        if(fscanf(vram_config,"%u",&mode)==1)base_vram_enabled=mode==1;
        if(fscanf(vram_config,"%u",&start)==1 && start>=3 && start<=10000)base_vram_start_frame=start;
        fclose(vram_config);
    }
    FILE *worker_config=fopen("sdmc:/3ds/foxhollow/native_worker.txt","r");
    if(worker_config){prepared_enabled=fgetc(worker_config)=='1';fclose(worker_config);}
#ifdef FH_REPLAY_BUILD
    /* Menu choice applies to this launch only; never rewrite ordinary settings. */
    extern unsigned fh_replay_native_worker;
    if(fh_replay_active())prepared_enabled=fh_replay_native_worker;
    extern unsigned fh_replay_texture_vram;
    if(fh_replay_active())base_vram_enabled=fh_replay_texture_vram;
#endif
    FILE *raster_config=fopen("sdmc:/3ds/foxhollow/raster_reuse.txt","r");
    if(raster_config){fh_raster_reuse_enabled=fgetc(raster_config)=='1';fclose(raster_config);}
    FILE *uniform_config=fopen("sdmc:/3ds/foxhollow/uniform_reuse.txt","r");
    if(uniform_config){fh_uniform_reuse_enabled=fgetc(uniform_config)=='1';fclose(uniform_config);}
    FILE *scope_config=fopen("sdmc:/3ds/foxhollow/texture_scope.txt","r");
    if(scope_config){unsigned mode;if(fscanf(scope_config,"%u",&mode)==1 && mode<=2)texture_scope_mode=mode;fclose(scope_config);}
    FILE *constant_config=fopen("sdmc:/3ds/foxhollow/constant_lighting.txt","r");
    if(constant_config){unsigned mode;if(fscanf(constant_config,"%u",&mode)==1 && mode<=1)constant_light_mode=mode;fclose(constant_config);}
    FILE *asset_config=fopen("sdmc:/3ds/foxhollow/texture_assets.txt","r");
    if(asset_config){unsigned mode;if(fscanf(asset_config,"%u",&mode)==1 && mode<=5)texture_asset_mode=mode;fclose(asset_config);}
    if(!C3D_Init(256*1024))fh_platform_panic("Minimal renderer command allocation");
    fh_native_fog_reset();
    target=native_surface_create();
    if(!target)fh_platform_panic("Minimal renderer target allocation");

    /* Same operations/uniform/output tables, one instruction store. Pinned
     * Melee shares DVLPs to avoid code reuploads between native draw paths. */
    binary=DVLB_ParseFile((u32*)minimal_shared_shader,sizeof(minimal_shared_shader));
    if(!binary || binary->numDVLE!=5 || binary->DVLP.codeSize>=512)fh_platform_panic("Minimal shared shader");
    shaderProgramInit(&program);shaderProgramSetVsh(&program,&binary->DVLE[0]);
    shaderProgramInit(&ui_program);shaderProgramSetVsh(&ui_program,&binary->DVLE[1]);
    shaderProgramInit(&world_program);shaderProgramSetVsh(&world_program,&binary->DVLE[2]);
    shaderProgramInit(&water_program);shaderProgramSetVsh(&water_program,&binary->DVLE[3]);
    shaderProgramInit(&hologram_program);shaderProgramSetVsh(&hologram_program,&binary->DVLE[4]);
    ui_vertices=linearMemAlign(UI_VERTICES*sizeof(*ui_vertices),128);
    if(!ui_vertices)fh_platform_panic("Minimal UI allocation");
    if(linearSpaceFree()>=WORLD_STREAM_BYTES+FH_NATIVE_LINEAR_RESERVE){
        world_stream.data=linearMemAlign(WORLD_STREAM_BYTES,128);
        if(world_stream.data)world_stream.capacity=WORLD_STREAM_BYTES/sizeof(uint16_t);
    }
    if(!fh_pica_scene_enabled)fh_texture_pack_open(&texture_pack,fh_asset_path(FH_ASSET_TEXTURE),128*1024);
    if(fh_hardware_large_memory){
    /* Exact sources plus native images, deduplicated only after byte comparison.
     * Spare linear RAM is CPU-readable; SD still reads into normal staging.
     * This is not a GPU texture allocation or a new render pass. */
    unsigned resident_bytes=fh_texture_pack_resident_size(&texture_pack);
    if(resident_bytes && resident_bytes<=11*1024*1024 &&
       linearSpaceFree()>=resident_bytes+FH_NATIVE_LINEAR_RESERVE){
        u64 start=svcGetSystemTick();void *storage=linearMemAlign(resident_bytes,128);
        if(storage && fh_texture_pack_resident_load(&texture_pack,storage,resident_bytes)){
            texture_resident_storage=storage;texture_resident_allocation=resident_bytes;
        }else linearFree(storage);
        texture_resident_startup_ticks=svcGetSystemTick()-start;
    }
    }
    texture_window_start(FH_NATIVE_LINEAR_RESERVE);
    texture_resident_startup_read_bytes=texture_pack.read_bytes;
    char cache_report[160];snprintf(cache_report,sizeof(cache_report),
        "Native texture startup cache: allocation=%u packed=%u ticks=%llu\n",
        texture_resident_allocation,texture_pack.resident_bytes,(unsigned long long)texture_resident_startup_ticks);
    fh_platform_log(cache_report);
    metrics=fopen("sdmc:/3ds/foxhollow/minimal-render.csv","w");
    if(metrics)setvbuf(metrics,metrics_buffer,_IOFBF,sizeof(metrics_buffer));
#if defined(FH_REPLAY_BUILD) || defined(FH_EMULATOR_TEST)
    const unsigned frame_detail=
#ifdef FH_UNIFIED_APP
        fh_replay_active();
#else
        1;
#endif
#else
    const unsigned frame_detail=0;
#endif
    fh_frame_metrics_init(&frame_metrics,metrics,SYSCLOCK_ARM11,frame_detail);
    if(fh_pica_scene_enabled)fh_pica_scene_init();
    initialized=1;
    fh_platform_log("PICA reconstruction stage 2: base textures/masks and batched UI, 400x240; incomplete visual diagnostic\n");
}
int fh_minimal_ready(void){return initialized;}
void fh_minimal_projection(const float matrix[4][4]){
    /* Game camera is already 5:3. Rotate for the LCD; GX depth [-w,0]
     * maps directly to PICA. Rendering is full 400x240, without an EFB crop. */
    for(unsigned c=0;c<4;c++){
        projection[0][c]=matrix[1][c];projection[1][c]=-matrix[0][c];
        projection[2][c]=matrix[2][c];projection[3][c]=matrix[3][c];
    }
}
static void owner_gpu_sync(void){
    flat_world_valid=0;
    if(open_frame){bool used=target->used;target->used=false;
        fh_early_queue_defer();C3D_FrameEnd(GX_CMDLIST_FLUSH);acquire();native_surface_draw_on(target);target->used=used;
    }else{acquire();open_frame=1;native_surface_draw_on(target);
        C3D_RenderTargetClear(target,C3D_CLEAR_ALL,0x142030ff,0);}
 }
#include "fh_pica_hud.h"
void fh_minimal_sync(void){
    if(!initialized)return;
    fh_pica_hud_flush();
    flush_ui();prepared_join();owner_gpu_sync();ui_used=0;
}

static u64 auxiliary_started;
void fh_minimal_auxiliary_begin(void){
    auxiliary_started=svcGetSystemTick();
    begin();fh_pica_hud_flush();flush_ui();prepared_join();
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Native auxiliary command bounds");
    if((capacity-offset)*4<32*1024)fh_minimal_sync();
}
void fh_minimal_auxiliary_end(unsigned draws,unsigned indices){
    native_surface_draw_on(target);
    fh_pica_scene_invalidate();fh_pica_materials_invalidate();
    frame_draws+=draws;frame_indices+=indices;
    __atomic_add_fetch(&fh_minimal_draws,draws,__ATOMIC_RELAXED);
    __atomic_add_fetch(&fh_minimal_indices,indices,__ATOMIC_RELAXED);
    submit_ticks+=svcGetSystemTick()-auxiliary_started;
}

unsigned fh_minimal_prepare_write(unsigned last_used){
    /* Begin retires the previous frame before any CPU overwrite. Current
     * commands only need a drain if they actually reference this asset. */
    begin();upload_writes++;
    if(last_used==fh_minimal_frames+1){fh_minimal_sync();upload_drains++;}
    return fh_minimal_frames+1;
}
int fh_minimal_sphere_update(unsigned slot,const FhTexture *destination){
    FhNativeLighting lighting;
    if(fh_pica_scene_enabled)return 0;
    if(!initialized || slot>=SPHERE_SLOTS || !destination || !destination->pixels ||
        destination->width!=32 || destination->height!=32 || destination->format!=6 ||
        !fh_gx_native_lighting(&lighting)){sphere_update_failures++;return 0;}
    u64 started=svcGetSystemTick();begin();flush_ui();prepared_join();
    if(!sphere_vertices){
        sphere_vertices=linearMemAlign(SPHERE_VERTICES*sizeof(*sphere_vertices),128);
        sphere_indices=linearMemAlign(SPHERE_INDICES*sizeof(*sphere_indices),128);
        if(!sphere_vertices || !sphere_indices)fh_platform_panic("Native sphere geometry allocation");
        for(unsigned x=0;x<17;x++)for(unsigned y=0;y<17;y++){
            float nx=2.f*x/15-1,ny=2.f*y/15-1,q=1-nx*nx-ny*ny;
            sphere_vertices[x*17+y]=(FhModelVertex){.position={nx,ny,-2,1},
                .normal={nx,ny,q>0?sqrtf(q):0},.color={1,1,1,1},.skin={0,0,1}};
        }
        unsigned at=0;
        for(unsigned x=0;x<16;x++)for(unsigned y=0;y<16;y++){
            unsigned a=x*17+y,b=a+17,c=a+1,d=b+1;
            sphere_indices[at++]=a;sphere_indices[at++]=b;sphere_indices[at++]=c;
            sphere_indices[at++]=b;sphere_indices[at++]=d;sphere_indices[at++]=c;
        }
        fh_gpu_cache_flush(sphere_vertices,SPHERE_VERTICES*sizeof(*sphere_vertices));
        fh_gpu_cache_flush(sphere_indices,SPHERE_INDICES*sizeof(*sphere_indices));
    }
    SphereMap *map=sphere_maps+slot;
    if(!map->target){
        if(!C3D_TexInitVRAM(&map->texture,32,32,GPU_RGBA8))fh_platform_panic("Native sphere texture allocation");
        map->target=C3D_RenderTargetCreateFromTex(&map->texture,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
        if(!map->target)fh_platform_panic("Native sphere target allocation");
        C3D_TexSetFilter(&map->texture,GPU_LINEAR,GPU_LINEAR);
        C3D_TexSetWrap(&map->texture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
        sphere_vram_bytes+=4096;
    }
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Native sphere command bounds");
    if((capacity-offset)*4<16*1024)fh_minimal_sync();
    C3D_FrameDrawOn(map->target);
    fh_native_fog_apply(0); /* Lighting lookup is not a camera-space surface. */
    C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
    bind_vertices(sphere_vertices,0,0);
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,r==0,r==1,r==2,0);
    /* Original ortho(1,-1,1,-1,1,15), with no LCD rotation for a texture. */
    C3D_FVUnifSet(GPU_VERTEX_SHADER,66,-1,0,0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,67,0,1,0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,68,0,0,0,-.5f);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,69,0,0,0,1);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,70,1,1,1,1);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,71,1,1,0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,72,1,1,0,0);
    for(unsigned i=0;i<13;i++)C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68+i,(lighting.booleans>>i)&1);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x75,false);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x76,true);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x77,false);
    for(unsigned i=0;i<21;i++){const float *v=lighting.uniform[i];C3D_FVUnifSet(GPU_VERTEX_SHADER,74+i,v[0],v[1],v[2],v[3]);}
    for(unsigned i=0;i<6;i++){const float *v=lighting.fixed[i];C3D_FixedAttribSet(6+i,v[0],v[1],v[2],v[3]);}
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_RGB,GPU_PRIMARY_COLOR,0,0);
    C3D_TexEnvFunc(C3D_GetTexEnv(0),C3D_RGB,GPU_REPLACE);
    C3D_TexEnvColor(C3D_GetTexEnv(0),0xffffffff);
    C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Alpha,GPU_CONSTANT,0,0);
    C3D_TexEnvFunc(C3D_GetTexEnv(0),C3D_Alpha,GPU_REPLACE);
    C3D_TexBind(0,NULL);C3D_TexBind(1,NULL);fh_pica_texture2_coord1(0);C3D_TexBind(2,NULL);
    C3D_CullFace(GPU_CULL_NONE);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    C3D_DrawElements(GPU_TRIANGLES,SPHERE_INDICES,C3D_UNSIGNED_SHORT,sphere_indices);
    /* Citro3D flushes framebuffer writes before switching targets, and a later
     * TexBind invalidates sampler state. GPU writes and sampling share the same
     * ordered command stream. No CPU copies or borrowed decoder buffers. */
    native_surface_draw_on(target);map->source=destination->pixels;map->valid=1;
    frame_draws++;frame_indices+=SPHERE_INDICES;__atomic_add_fetch(&fh_minimal_draws,1,__ATOMIC_RELAXED);__atomic_add_fetch(&fh_minimal_indices,SPHERE_INDICES,__ATOMIC_RELAXED);
    u64 elapsed=svcGetSystemTick()-started;submit_ticks+=elapsed;
    sphere_updates++;sphere_update_ticks+=elapsed;return 1;
}
#include "platform_native_world_light.h"
/* Original foreground alpha is 1 - radial.green * inverseRamp.green.
 * RGB stays in its existing stages; all six alpha stages remain independent. */
static void world_falloff_env(const FhMinimalWorldFalloff *f){
    if(!f->active)return;
    C3D_TexEnv *e=C3D_GetTexEnv(3);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE1,GPU_TEXTURE2,0);
    C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_G,GPU_TEVOP_A_SRC_G,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
    e=C3D_GetTexEnv(4);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,0,0);
    C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA,0,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);
}
static void world_material_env(const FhMinimalWorldMaterial *m){
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnv *e=C3D_GetTexEnv(0);
    unsigned base=m->texture[0].pixels!=NULL;
    unsigned intensity0=m->texture[0].format<=1,intensity1=m->texture[1].format<=1;
    if(!m->layered){
        C3D_TexEnvSrc(e,C3D_Both,GPU_PRIMARY_COLOR,base?GPU_TEXTURE0:0,0);
        C3D_TexEnvFunc(e,C3D_Both,base?GPU_MODULATE:GPU_REPLACE);
        if(base && intensity0)C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
        world_light_env_stages(&m->lighting);world_falloff_env(&m->falloff);return;
    }
    unsigned k=m->konst[0]|m->konst[1]<<8|m->konst[2]<<16|(unsigned)m->konst[3]<<24;
    /* Original map sequence: KColor base, mode9 overlay, vertex/KAlpha.
     * Preserve the first image's alpha in the interpolation weight. */
    C3D_TexEnvColor(e,k);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,GPU_PRIMARY_COLOR,0);
    C3D_TexEnvFunc(e,C3D_Both,GPU_MODULATE);
    if(intensity0)C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_R,GPU_TEVOP_A_SRC_ALPHA,0);
    e=C3D_GetTexEnv(1);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE1,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE1,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
    if(intensity1)C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
    e=C3D_GetTexEnv(2);C3D_TexEnvColor(e,k);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_PRIMARY_COLOR,0);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e,C3D_Both,GPU_MODULATE);
    world_light_env_stages(&m->lighting);world_falloff_env(&m->falloff);
}
/* Scope opened only around the callback-free flat-world executor. UI is
 * flushed before publication. Worker mode uses the complete ordinary setup;
 * no mutable pass state crosses the producer/consumer boundary. */
void fh_minimal_world_batch_begin(void){
    if(!initialized)return;
    begin();flush_ui();flat_world_valid=0;flat_world_scope=!prepared_enabled;
}
void fh_minimal_world_batch_end(void){flat_world_scope=flat_world_valid=0;}
static int submit_prepared_world(const FhPreparedWorld *p){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    const FhSceneAsset *asset=&p->asset;const FhSceneGroup *group=&p->group;
    const FhMinimalWorldMaterial *m=&p->material;const FhTexture *image=&p->image;
    const float (*view)[4]=p->view,(*projection)[4]=p->projection;
    static C3D_Tex samplers[3];memcpy(samplers,p->samplers,sizeof(samplers));
    C3D_Tex *base=p->base?samplers:NULL,*second=p->second?samplers+1:NULL;
    unsigned movie=p->movie,falloff=p->falloff;const unsigned *clip=p->clip;
    unsigned visible_count=p->visible_count;const FhWorldRange *visible=p->visible;
    u64 start=svcGetSystemTick();
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Native world command bounds");
    if((capacity-offset)*4<16*1024)owner_gpu_sync();
    unsigned scoped=flat_world_scope && fh_render_budget_world && !movie && !falloff &&
        !m->lighting.count && !m->lighting.mode && m->near_z==0;
    unsigned flat=scoped && !fh_render_budget_textures && !base && !second && !m->layered;
    unsigned packed=asset->compact;
    unsigned reuse=scoped && flat_world_valid && flat_world_format==group->format && flat_world_packed==packed;
    if(!reuse){
    C3D_BindProgram(&world_program);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    AttrInfo_AddLoader(a,0,packed?GPU_SHORT:GPU_FLOAT,4);
    AttrInfo_AddLoader(a,1,packed?GPU_UNSIGNED_BYTE:GPU_FLOAT,4);
    AttrInfo_AddLoader(a,2,packed?GPU_SHORT:GPU_FLOAT,2);
    AttrInfo_AddLoader(a,3,packed?GPU_SHORT:GPU_FLOAT,packed?2:3);
    float ps=packed?(group->format==7?1.f:group->format==5?1.f/8:1.f/256):1.f;
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0]*ps,view[r][1]*ps,view[r][2]*ps,view[r][3]);
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    }
    /* A block's transforms/layout can survive material changes. Texture,
     * scroll, raster colour and TEV state must still follow every live material.
     * Only the original constant flat pass can reuse that whole state. */
    if(!(reuse && flat && flat_world_last)){
    float cs=packed?1.f/15:1.f,us=packed?(group->format==5?1.f/256:1.f/1024):1.f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,m->raster[0]*cs,m->raster[1]*cs,m->raster[2]*cs,m->raster[3]*cs);
    for(unsigned i=0;i<2;i++){
        unsigned present=i?second!=NULL:base!=NULL;
        const FhTexture *source=i?m->texture+1:image;
        float sx=present?base_uv_scale(source->width,samplers[i].width):1;
        float sy=present?base_uv_scale(source->height,samplers[i].height):1;
        C3D_FVUnifSet(GPU_VERTEX_SHADER,8+i,sx*us,sy*us,movie?0:m->scroll[i][0]*sx,movie?0:m->scroll[i][1]*sy);
    }
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,falloff);
    if(falloff){
        const FhTexture *f=m->falloff.texture;
        float sx=base_uv_scale(f[0].width,samplers[1].width);
        float sy=base_uv_scale(f[0].height,samplers[1].height);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,10,.02f*sx,-.02f*sy,.5f*sx,1-.5f*sy);
        sx=base_uv_scale(f[1].width,samplers[2].width);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,11,-.1f*sx,(1-.1f*m->falloff.distance)*sx,0,1);
    }
    if(movie){
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        C3D_TexEnvBufUpdate(C3D_Both,0);
        C3D_TexEnv *env=C3D_GetTexEnv(0);
        C3D_TexEnvSrc(env,C3D_Both,GPU_TEXTURE0,0,0);
        C3D_TexEnvFunc(env,C3D_Both,GPU_REPLACE);
        if(image->format<=1)C3D_TexEnvOpAlpha(env,GPU_TEVOP_A_SRC_R,0,0);
    }else{
        FhWorldMaterialKey key=fh_world_material_key(m);
        if(reuse && fh_world_material_equal(key,world_material_last))world_material_reuses++;
        else world_material_env(m);
        world_material_last=key;
    }
    world_light_bind(movie?NULL:&m->lighting);
    prepared_fog_apply(&p->fog,!movie);
    C3D_TexBind(0,base?samplers:NULL);C3D_TexBind(1,second?samplers+1:NULL);
    /* Layered model draws use unit2/texcoord2. Only foreground falloff uses the third sampler; ordinary world draws
     * must disable the preceding model/falloff binding.
     * Match pinned Melee's explicit unbind when leaving its dual-map path. */
    if(!falloff && C3Di_GetContext()->tex[2])world_sampler_resets++;
    fh_pica_texture2_coord1(0);C3D_TexBind(2,falloff?samplers+2:NULL);
    }
    flat_world_valid=scoped;flat_world_last=flat;
    if(scoped){flat_world_format=group->format;flat_world_packed=packed;
        if(reuse)flat_world_reuses++;else flat_world_setups++;}
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    /* Float world storage retains three unused skin floats after normal/TEX1.
     * Describe those twelve bytes as padding rather than an oversized stride. */
    if(BufInfo_Add(b,group->vertex,packed?sizeof(FhWorldCompactVertex):sizeof(FhModelVertex),
        packed?4:5,packed?0x3210:0xe3210)<0)fh_platform_panic("Native world attributes");
    unsigned blend=(m->flags&(0x40000000u|0x20000000u))!=0;
    unsigned pipeline_flags=m->flags&(0x60000000u|0x480u);
    unsigned pipeline_reuse=reuse && pipeline_flags==world_pipeline_flags && group->cull==world_pipeline_cull &&
        !memcmp(clip,world_pipeline_clip,sizeof(world_pipeline_clip));
    if(pipeline_reuse)world_pipeline_reuses++;
    else{
    C3D_CullFace(group->cull?GPU_CULL_FRONT_CCW:GPU_CULL_NONE);
    C3D_DepthMap(true,-(1-m->near_z),0);
    C3D_DepthTest(true,GPU_GEQUAL,blend?GPU_WRITE_COLOR:GPU_WRITE_ALL);
    C3D_AlphaTest(!blend && (m->flags&0x400) && !(m->flags&0x80),GPU_GREATER,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,blend?GPU_SRC_ALPHA:GPU_ONE,
        blend?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
    if(movie){
        /* Same opaque THP callback state as the original model route. */
        C3D_CullFace(GPU_CULL_FRONT_CCW);
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
        C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    }
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    }
    world_pipeline_flags=pipeline_flags;world_pipeline_cull=group->cull;
    memcpy(world_pipeline_clip,clip,sizeof(world_pipeline_clip));
    unsigned gathered_count=0;
    if(visible_count>1){
        /* Reserve command space BEFORE publishing the index slice: a join
         * resets stream ownership. Never reset/reuse it after publication. */
        GPUCMD_GetBuffer(NULL,&capacity,&offset);
        if(offset>capacity)fh_platform_panic("Native gathered world command bounds");
        if((capacity-offset)*4<16*1024)owner_gpu_sync();
        const uint16_t *gathered=fh_world_index_append(&world_stream,group->index,
            group->indices,visible,visible_count,&gathered_count);
        if(gathered){
            C3D_DrawElements(GPU_TRIANGLES,gathered_count,C3D_UNSIGNED_SHORT,gathered);
            world_gather_draws++;world_gather_saved_draws+=visible_count-1;
            world_gather_bytes+=gathered_count*sizeof(uint16_t);
        }else world_gather_fallbacks++; /* Full stream retains original spans. */
    }
    unsigned submitted_indices=gathered_count;
    for(unsigned i=0;!gathered_count && i<visible_count;i++){
        /* Culling can split one formerly single draw into many spans. The
         * earlier whole-group reservation alone does not bound that packet
         * count. Retire a full command buffer before recording another span. */
        GPUCMD_GetBuffer(NULL,&capacity,&offset);
        if(offset>capacity)fh_platform_panic("Native world span command bounds");
        if((capacity-offset)*4<16*1024)owner_gpu_sync();
        C3D_DrawElements(GPU_TRIANGLES,visible[i].count,C3D_UNSIGNED_SHORT,group->index+visible[i].first);
        submitted_indices+=visible[i].count;
    }
    unsigned actual_draws=gathered_count?1:visible_count;
    world_split_draws+=actual_draws-1;
    C3D_LightEnvBind(NULL); /* Other native shaders do not emit lighting outputs. */
    C3D_DepthMap(true,-1,0);
    world_falloff_draws+=falloff;
    world_draws++;world_layered_draws+=!movie && m->layered;world_deferred_draws+=m->near_z!=0;
    movie_draws+=movie;world_movie_draws+=movie;
    __atomic_add_fetch(&fh_minimal_draws,actual_draws,__ATOMIC_RELAXED);__atomic_add_fetch(&fh_minimal_indices,submitted_indices,__ATOMIC_RELAXED);frame_draws+=actual_draws;frame_indices+=submitted_indices;
    u64 elapsed=svcGetSystemTick()-start;world_ticks+=elapsed;submit_ticks+=elapsed;return 1;
}
int fh_minimal_world_group(const FhSceneAsset *asset,const FhSceneGroup *group,
    const float view[3][4],const FhMinimalWorldMaterial *m,const unsigned scissor[4]){
    FH_REPLAY_SCOPE(NATIVE_WORLD);
    if(fh_pica_scene_enabled)return 1; /* Only replacement block traversal may draw. */
    unsigned movie=fh_minimal_movie_active;
    unsigned falloff=!movie && m && m->falloff.active;
    if(!initialized || !asset || !group || !m || !asset->block || !asset->direct ||
        !group->indices || (!movie && group->two_uv!=m->layered) ||
        m->lighting.count>2 || m->lighting.mode>2 ||
        (falloff && (m->layered || !isfinite(m->falloff.distance))))goto failed;
    for(unsigned i=0;i<m->lighting.count;i++){
        const FhMinimalWorldLight *l=m->lighting.light+i;
        if(!isfinite(l->radius) || !isfinite(l->position[0]) ||
           !isfinite(l->position[1]) || !isfinite(l->position[2]))goto failed;
    }
    unsigned clip[4];
    if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return 1;
    u64 start=svcGetSystemTick(),wait_start=prepared_wait_ticks;
    FhWorldRange visible[FH_WORLD_MAX_RUNS];visible[0]=(FhWorldRange){0,group->indices};unsigned visible_count=1;
    const FhWorldBounds *parents;
    unsigned first,total;const FhWorldBounds *bounds=fh_native_scene_world_bounds(asset,group,&first,&total,&parents);
    if(bounds){
        float planes[4][4];fh_world_clip_planes(planes,view,projection);
        { FH_REPLAY_SCOPE(WORLD_VISIBILITY);
        visible_count=fh_world_visible_hierarchy(visible,FH_WORLD_MAX_RUNS,bounds,parents,total,first,group->indices,planes);
        }
        unsigned retained=0;for(unsigned i=0;i<visible_count;i++)retained+=visible[i].count;
        world_cull_checks++;world_culled_indices+=group->indices-retained;
        if(!visible_count){
            world_culled_groups++;begin();
            u64 elapsed=svcGetSystemTick()-start;prepared_world_prep_ticks+=elapsed;prepared_prep_ticks+=elapsed;return 1;
        }
    }
    begin();flush_ui();
    static C3D_Tex samplers[3];
    const FhTexture *image=movie?&fh_minimal_movie_texture:m->texture;
    C3D_Tex *base=movie?base_texture(image):surface_texture(image->pixels?image:NULL);
    if((movie || image->pixels) && !base)goto failed;
    /* Capture sampler0 before sampler1 can change wrapping on an aliased
     * texture object. Both GPU allocations stay pinned through submission. */
    if(base)samplers[0]=*base;
    base_pin=base;
    C3D_Tex *second=falloff?surface_texture(m->falloff.texture):
        !movie && m->layered?surface_texture(m->texture+1):NULL;
    if(second)samplers[1]=*second;
    second_pin=second;
    C3D_Tex *third=falloff?surface_texture(m->falloff.texture+1):NULL;
    if(third)samplers[2]=*third;
    second_pin=NULL;base_pin=NULL;
    if(falloff && (!second || !third))goto failed;
    if(!movie && m->layered && (!base || !second))goto failed;
    if(second)samplers[1]=*second;
    FhPreparedWorld local;FhPreparedPayload *slot=prepared_reserve(PREPARED_WORLD);
    FhPreparedWorld *ready=slot?&slot->world:&local;
    ready->asset=*asset;ready->group=*group;ready->material=*m;ready->image=*image;
    memcpy(ready->view,view,sizeof(ready->view));memcpy(ready->projection,projection,sizeof(ready->projection));
    memcpy(ready->samplers,samplers,sizeof(ready->samplers));
    ready->base=base!=NULL;ready->second=second!=NULL;ready->third=third!=NULL;
    ready->movie=movie;ready->falloff=falloff;memcpy(ready->clip,clip,sizeof(ready->clip));
    ready->visible_count=visible_count;memcpy(ready->visible,visible,visible_count*sizeof(*visible));
    ready->fog=prepared_fog();ready->start=start;
    u64 preparation=svcGetSystemTick()-start-(prepared_wait_ticks-wait_start);prepared_prep_ticks+=preparation;prepared_world_prep_ticks+=preparation;
    fh_native_scene_used(asset); /* Publish lifetime before a later game write. */
    if(slot){prepared_commit(PREPARED_WORLD);return 1;}
    return submit_prepared_world(ready);
failed:
    world_failures++;fh_minimal_rejected++;return 0;
}
#include "platform_native_water.h"
#include "platform_native_water_effect.h"
/* Two original source UVs: layer0 mode0/8 then mode1 alpha interpolation.
 * Texture1 remains the diffuse sphere; Texture2 uses independent TEX1 UVs.
 * Both-before materials multiply the combined image by light after blending.
 */
static unsigned model_fade_draws;
#if defined(FH_EMULATOR_TEST) || defined(FH_REPLAY_BUILD)
/* Bounded geometry-only capture of the actual native draw, before culling.
 * Normal-memory staging keeps stdio away from linear GPU-owned storage. */
static void capture_model_geometry(const FhSceneAsset *asset,const FhSceneGroup *group,
    const float palette[FH_MODEL_BONES][3][4],unsigned material,unsigned model){
    static unsigned checked,first,used,seen[64][2];
    if(!checked){FILE *f=fopen("sdmc:/3ds/foxhollow/capture-native-models.txt","r");
        if(f){if(fscanf(f,"%u",&first)!=1)first=0;fclose(f);}checked=1;}
    if(!first || fh_minimal_frames<first || used==64 || !asset->compact || asset->block ||
       group->vertices>8192 || group->indices>24576)return;
    for(unsigned i=0;i<used;i++)if(seen[i][0]==asset->id && seen[i][1]==group->first)return;
    seen[used][0]=asset->id;seen[used++][1]=group->first;
    unsigned vb=group->vertices*sizeof(FhModelCompactVertex),ib=group->indices*2;
    unsigned char *copy=malloc(vb+ib);if(!copy)return;
    memcpy(copy,group->vertex,vb);memcpy(copy+vb,group->index,ib);
    uint32_t header[12]={0x31474d4e,asset->id,group->first,fh_minimal_frames,
        group->format,group->vertices,group->indices,group->bones,group->cull,material,model,40};
    char name[128];snprintf(name,sizeof(name),"sdmc:/3ds/foxhollow/native-model-%u-%u.bin",asset->id,group->first);
    FILE *f=fopen(name,"wb");unsigned good=f!=NULL;
    if(f){good=fwrite(header,1,sizeof(header),f)==sizeof(header);
        good&=fwrite(projection,1,sizeof(projection),f)==sizeof(projection);
        good&=fwrite(palette,48,group->bones,f)==group->bones;
        good&=fwrite(copy,1,vb+ib,f)==vb+ib;good&=fclose(f)==0;}
    free(copy);if(!good)fh_platform_panic("Native model snapshot failed");
}
#endif
#ifdef FH_EMULATOR_TEST
/* Opt-in private diagnostic. Copy CPU-owned vertices out of linear memory
 * before stdio, as in the pinned Melee readback staging discipline. No GPU
 * readback and no physical-console path; never use this run for timing. */
static void capture_cloud(const FhSceneAsset *asset,const FhSceneGroup *group,
    const float palette[FH_MODEL_BONES][3][4],const FhNativeLighting *light,
    const FhTexture *texture,unsigned material_flags,unsigned model_flags,
    const float tint[4],const float uv[4],const unsigned scissor[4]){
    static unsigned checked,enabled,used,seen[4];
    if(!fh_minimal_cloud_active)return;
    if(!checked){FILE *f=fopen("sdmc:/3ds/foxhollow/capture-native-clouds.txt","r");
        enabled=f!=NULL;if(f)fclose(f);checked=1;}
    if(!enabled || used==4 || !asset->compact || !texture || texture->palette || group->vertices>4096)return;
    for(unsigned i=0;i<used;i++)if(seen[i]==asset->id)return;
    seen[used++]=asset->id;
    unsigned bytes=fh_texture_source_bytes(texture->format,texture->width,texture->height);
    unsigned vertex_bytes=group->vertices*sizeof(FhModelCompactVertex),index_bytes=group->indices*2;
    if(!bytes || bytes>1024*1024 || group->indices>24576)return;
    unsigned char *copy=malloc(vertex_bytes+index_bytes+bytes);if(!copy)return;
    memcpy(copy,group->vertex,vertex_bytes);memcpy(copy+vertex_bytes,group->index,index_bytes);
    memcpy(copy+vertex_bytes+index_bytes,texture->pixels,bytes);
    uint32_t header[24]={0x31434c43,asset->id,fh_minimal_frames,group->format,
        group->vertices,group->indices,group->bones,group->cull,material_flags,model_flags,
        texture->width,texture->height,texture->format,texture->wrap_s,texture->wrap_t,bytes,
        sizeof(FhModelCompactVertex),light->booleans,0,0,scissor[0],scissor[1],scissor[2],scissor[3]};
    char name[128],temporary[132];snprintf(name,sizeof(name),"sdmc:/3ds/foxhollow/native-cloud-%u.bin",asset->id);
    snprintf(temporary,sizeof(temporary),"%s.tmp",name);
    FILE *f=fopen(temporary,"wb");unsigned good=f!=NULL;
    if(f){
        good=fwrite(header,1,sizeof(header),f)==sizeof(header);
        good&=fwrite(projection,1,sizeof(projection),f)==sizeof(projection);
        good&=fwrite(palette,48,group->bones,f)==group->bones;
        good&=fwrite(light->uniform,1,sizeof(light->uniform),f)==sizeof(light->uniform);
        good&=fwrite(tint,4,4,f)==4;good&=fwrite(uv,4,4,f)==4;
        good&=fwrite(copy,1,vertex_bytes+index_bytes+bytes,f)==vertex_bytes+index_bytes+bytes;
        good&=fclose(f)==0;
    }
    if(good)good=rename(temporary,name)==0;
    free(copy);char line[120];snprintf(line,sizeof(line),"Native cloud snapshot id=%u complete=%u\n",asset->id,good);fh_platform_log(line);
}
#endif
/* Original addColorFadeStage: interpolate RGB toward the live object color,
 * retaining previous alpha. Stage5 follows all current native model layers. */
static void model_color_fade_env(const unsigned char rgba[4]){
    C3D_TexEnv *e=C3D_GetTexEnv(5);
    C3D_TexEnvInit(e);
    C3D_TexEnvColor(e,(unsigned)rgba[0]|(unsigned)rgba[1]<<8|
        (unsigned)rgba[2]<<16|(unsigned)rgba[3]<<24);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,GPU_PREVIOUS,GPU_CONSTANT);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
}
static void model_layered_env(unsigned mapped,unsigned both_before,unsigned intensity,
    unsigned alpha,unsigned intensity0,unsigned intensity2){
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvBufUpdate(C3D_Both,0);
    unsigned first=mapped && !both_before;
    C3D_TexEnv *e=C3D_GetTexEnv(0);
    if(first){
        C3D_TexEnvColor(e,(intensity&255)*0x010101u|0xff000000u);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE1,GPU_CONSTANT,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MULTIPLY_ADD);
        e=C3D_GetTexEnv(1);
    }
    C3D_TexEnvColor(e,0x00ffffffu|((alpha&255)<<24));
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,first?GPU_PREVIOUS:GPU_PRIMARY_COLOR,0);
    C3D_TexEnvFunc(e,C3D_RGB,mapped && both_before?GPU_REPLACE:GPU_MODULATE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvOpAlpha(e,intensity0?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
    e=C3D_GetTexEnv(first+1);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE2,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE2,0);
    C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,intensity2?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
    if(mapped && both_before){
        /* Stage1 image becomes visible through PREVIOUS_BUFFER at stage3. */
        C3D_TexEnvBufUpdate(C3D_RGB,1u<<1);
        e=C3D_GetTexEnv(2);
        C3D_TexEnvColor(e,(intensity&255)*0x010101u|0xff000000u);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE1,GPU_CONSTANT,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MULTIPLY_ADD);
        e=C3D_GetTexEnv(3);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,0);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
    }
}
#ifdef FH_EMULATOR_TEST
#define FH_MODEL_PROFILE(n) do{u64 now=svcGetSystemTick();model_profile[n]+=now-profile_stamp;profile_stamp=now;}while(0)
#else
#define FH_MODEL_PROFILE(n) ((void)0)
#endif
static int submit_prepared_model(const FhPreparedModel *p){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    const FhSceneAsset *asset=&p->asset;const FhSceneGroup *group=&p->group;
    float palette[FH_MODEL_BONES][3][4];
    memcpy(palette,p->palette,group->bones*sizeof(palette[0]));
    const float (*projection)[4]=p->projection;
    FhNativeLighting lighting;if(p->lit)lighting=p->lighting;
    unsigned lit=p->lit,movie=p->movie;
    const FhMinimalCallbackMaterial *callback=p->has_callback?&p->callback:NULL;
    const FhMinimalModelLayers *layers=p->has_layers?&p->layers:NULL;
    const FhTexture *texture=p->has_texture?&p->texture:NULL,*sphere=p->has_sphere?&p->sphere:NULL;
    /* Citro3D retains these descriptor addresses after DrawElements. Keep
     * consumer-owned sampler storage alive beyond the temporary ready record. */
    static C3D_Tex samplers[3];memcpy(samplers,p->samplers,sizeof(samplers));
    C3D_Tex *base=p->base?samplers:NULL,*illumination=p->illumination?samplers+1:NULL;
    C3D_Tex *second=p->second?samplers+2:NULL;
    const unsigned char *unlit_rgba=p->has_unlit?p->unlit:NULL,*fade_rgba=p->has_fade?p->fade:NULL;
    unsigned fh_minimal_cloud_depth=p->cloud_depth;
    float alpha=p->alpha;unsigned sphere_intensity=p->sphere_intensity;
    unsigned material_flags=p->material_flags,model_flags=p->model_flags;
    const unsigned *clip=p->clip;u64 start=svcGetSystemTick();
#ifdef FH_EMULATOR_TEST
    unsigned fh_minimal_cloud_active=p->cloud_active;u64 profile_stamp=svcGetSystemTick();
#endif
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Minimal renderer command bounds");
    if((capacity-offset)*4<16*1024)owner_gpu_sync();
    bind_vertices_program(group->vertex,asset->compact,group->format,
        callback && callback->hologram?&hologram_program:&program);
    for(unsigned bone=0;bone<group->bones;bone++)for(unsigned r=0;r<3;r++){
        const float *v=palette[bone][r];C3D_FVUnifSet(GPU_VERTEX_SHADER,bone*3+r,v[0],v[1],v[2],v[3]);
    }
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,66+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,70,unlit_rgba?unlit_rgba[0]/255.f:1,
        unlit_rgba?unlit_rgba[1]/255.f:1,unlit_rgba?unlit_rgba[2]/255.f:1,
        callback && callback->normal_disk?1:layers && !layers->layered && !movie?layers->alpha/255.f:alpha);
    FH_MODEL_PROFILE(2);
    float constant_fill[4];
    /* Sphere UVs still consume skinned normals even when diffuse is constant.
     * Keep those draws on the full path. No sphere-generation call uses this
     * marker. All original alpha, material, vertex ambient and layers remain. */
    unsigned constant=lit && !illumination && !(callback && callback->normal_disk) && constant_light_mode &&
        fh_native_constant_light(&lighting,constant_fill);
    if(constant){
        lighting.uniform[18][0]=-1;memcpy(lighting.uniform[19],constant_fill,16);
        constant_light_draws++;constant_light_indices+=group->indices;
    }
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,lit);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x76,false);
    unsigned normal_palette=lit && !constant && fh_native_normal_palette(palette,group->bones);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x77,normal_palette);
    if(normal_palette){
        for(unsigned bone=0;bone<group->bones;bone++)for(unsigned r=0;r<3;r++){
            const float *v=palette[11+bone][r];
            C3D_FVUnifSet(GPU_VERTEX_SHADER,33+bone*3+r,v[0],v[1],v[2],0);
        }
        normal_palette_draws++;normal_palette_bones+=group->bones;normal_palette_indices+=group->indices;
    }else if(lit && !constant)normal_palette_fallbacks++;
    if(lit){
        for(unsigned i=constant?9:1;i<13;i++)C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68+i,(lighting.booleans>>i)&1);
        for(unsigned i=constant?16:0;i<(constant?20:21);i++){const float *v=lighting.uniform[i];C3D_FVUnifSet(GPU_VERTEX_SHADER,74+i,v[0],v[1],v[2],v[3]);}
        if(!constant)for(unsigned i=0;i<6;i++){const float *v=lighting.fixed[i];C3D_FixedAttribSet(6+i,v[0],v[1],v[2],v[3]);}
        lit_draws++;
        if(lighting.lights>lighting_max)lighting_max=lighting.lights;
        if(lighting.complex>lighting_complex_max)lighting_complex_max=lighting.complex;
    }
    FH_MODEL_PROFILE(3);
    unsigned disk=callback && callback->normal_disk;
    unsigned mapped=(illumination || disk) && lit;
#ifdef FH_EMULATOR_TEST
    if(lit && fh_minimal_cloud_active){
        static unsigned seen[16],used;
        unsigned i;for(i=0;i<used;i++)if(seen[i]==asset->id)break;
        if(i==used && used<16){seen[used++]=asset->id;char line[360];
            snprintf(line,sizeof(line),"Native cloud lighting id=%u bits=%x ambient=%.3f,%.3f,%.3f,%.3f material=%.3f,%.3f,%.3f,%.3f mapped=%u fade=%u base=%u\n",
                asset->id,lighting.booleans,lighting.uniform[16][0],lighting.uniform[16][1],lighting.uniform[16][2],lighting.uniform[16][3],
                lighting.uniform[17][0],lighting.uniform[17][1],lighting.uniform[17][2],lighting.uniform[17][3],mapped,fade_rgba!=NULL,base!=NULL);
            fh_platform_log(line);
        }
    }
#endif
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x75,mapped);
    if(disk && mapped){
        float sx=base_uv_scale(callback->mask.width,second->width),sy=base_uv_scale(callback->mask.height,second->height);
        /* Original normalized normal texgen, post-scale .7 and offset .5.
         * Reuse the existing GPU normal transform; no new shader or CPU loop. */
        C3D_FixedAttribSet(11,.7f*sx,-.7f*sy,.5f*sx,1-.5f*sy);
    }else if(mapped){
        float sx=(float)sphere->width/illumination->width,sy=(float)sphere->height/illumination->height;
        C3D_FixedAttribSet(11,-.5f*sx,.5f*sy,.5f*sx,1-.5f*sy);
    }
    C3D_FVUnifSet(GPU_VERTEX_SHADER,71,base?base_uv_scale(texture->width,base->width):1,
        base?base_uv_scale(texture->height,base->height):1,
        layers && base && !movie?layers->scroll[0][0]*base_uv_scale(texture->width,base->width):0,
        layers && base && !movie?layers->scroll[0][1]*base_uv_scale(texture->height,base->height):0);
    const FhTexture *second_source=callback && callback->has_mask?&callback->mask:layers?&layers->second:NULL;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,72,second?base_uv_scale(second_source->width,second->width):1,
        second?base_uv_scale(second_source->height,second->height):1,
        second && layers?layers->scroll[1][0]*base_uv_scale(second_source->width,second->width):0,
        second && layers?layers->scroll[1][1]*base_uv_scale(second_source->height,second->height):0);
    if(callback && callback->hologram){
        for(unsigned r=0;r<3;r++){
            const float *v=callback->projected[r];C3D_FVUnifSet(GPU_VERTEX_SHADER,74+r,v[0],v[1],v[2],v[3]);
        }
        float ns=asset->compact?1.f/64:1;
        for(unsigned r=0;r<2;r++){
            const float *v=callback->normal_uv[r];C3D_FVUnifSet(GPU_VERTEX_SHADER,77+r,v[0]*ns,v[1]*ns,v[2]*ns,v[3]);
        }
        /* Per-draw descriptor copy: never leave a shared cache texture in
         * projection mode for subsequent ordinary model/UI draws. */
        if(base)samplers[0].param=(samplers[0].param&~(7u<<28))|(GPU_TEX_PROJECTION<<28);
    }
#ifdef FH_EMULATOR_TEST
    if(lit && !movie && base){
        float tint[4]={1,1,1,layers && !layers->layered?layers->alpha/255.f:alpha};
        float uv[4]={base_uv_scale(texture->width,base->width),base_uv_scale(texture->height,base->height),
            layers?layers->scroll[0][0]*base_uv_scale(texture->width,base->width):0,
            layers?layers->scroll[0][1]*base_uv_scale(texture->height,base->height):0};
        capture_cloud(asset,group,palette,&lighting,texture,material_flags,model_flags,tint,uv,p->capture_scissor);
    }
#endif
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,movie && base?GPU_TEXTURE0:GPU_PRIMARY_COLOR,base?GPU_TEXTURE0:0,0);
    C3D_TexEnvFunc(C3D_GetTexEnv(0),C3D_Both,base && !movie?GPU_MODULATE:GPU_REPLACE);
    C3D_TexBind(0,base?samplers:NULL);
    /* GX intensity supplies alpha as well as RGB; PICA L4/L8 alpha is one. */
    if(base && (texture->format==0 || texture->format==1))
        C3D_TexEnvOpAlpha(C3D_GetTexEnv(0),GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
    C3D_TexBind(1,callback && second?samplers+2:mapped?samplers+1:NULL);
    fh_pica_texture2_coord1(0);C3D_TexBind(2,!callback && second?samplers+2:NULL);
    if(mapped && !disk){
        /* Original addSphereMapTexStage: C1=clamp(raster+map*intensity).
         * The following base layer multiplies C1; map alpha is not opacity. */
        C3D_TexEnv *env=C3D_GetTexEnv(0);
        unsigned value=sphere_intensity&255;
        C3D_TexEnvColor(env,value|(value<<8)|(value<<16)|0xff000000u);
        C3D_TexEnvSrc(env,C3D_RGB,GPU_TEXTURE1,GPU_CONSTANT,GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(env,C3D_RGB,GPU_MULTIPLY_ADD);
        C3D_TexEnvSrc(env,C3D_Alpha,GPU_PRIMARY_COLOR,0,0);
        C3D_TexEnvOpAlpha(env,GPU_TEVOP_A_SRC_ALPHA,0,0);
        C3D_TexEnvFunc(env,C3D_Alpha,GPU_REPLACE);
        env=C3D_GetTexEnv(1);
        C3D_TexEnvSrc(env,C3D_Both,GPU_PREVIOUS,base?GPU_TEXTURE0:0,0);
        C3D_TexEnvFunc(env,C3D_Both,base?GPU_MODULATE:GPU_REPLACE);
        if(base && (texture->format==0 || texture->format==1))
            C3D_TexEnvOpAlpha(env,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
        sphere_draws++;
    }
    if(layers && layers->layered){
        model_layered_env(mapped,layers->both_before_lighting,sphere_intensity,layers->alpha,
            texture->format<=1,layers->second.format<=1);
        model_layered_draws++;
    }
    if(layers && layers->constant_alpha && !layers->masked){
        /* KAlpha/Swizzled use material opacity, not lit raster alpha. This
         * overrides only alpha in the existing base stage, preserving RGB. */
        C3D_TexEnv *e=C3D_GetTexEnv(mapped?1:0);
        C3D_TexEnvColor(e,0x00ffffffu|((layers->alpha&255)<<24));
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,base?GPU_TEXTURE0:0,0);
        C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,
            base && texture->format<=1?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
        C3D_TexEnvFunc(e,C3D_Alpha,base?GPU_MODULATE:GPU_REPLACE);
        model_constant_alpha_draws++;
    }
    if(layers && layers->masked){
        /* Swizzled secondary stage preserves colour and REPLACES opacity:
         * mask * material alpha, independent of base/raster alpha. Same UV
         * is already duplicated by compact_attributes for single-UV models. */
        C3D_TexEnv *e=C3D_GetTexEnv(mapped?1:0);
        C3D_TexEnvColor(e,0x00ffffffu|((layers->alpha&255)<<24));
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE2,GPU_CONSTANT,0);
        C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_R,GPU_TEVOP_A_SRC_ALPHA,0);
        C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
        model_mask_draws++;
    }
    if(layers && layers->constant_rgb){
        /* Mode-0 Swizzled/KColor: TEXC*KONST. Preserve alpha sources,
         * including an independent opacity mask, in the same TEV stage. */
        C3D_TexEnv *e=C3D_GetTexEnv(0);
        C3D_TexEnvColor(e,(layers->rgb&0x00ffffffu)|((layers->alpha&255)<<24));
        C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,GPU_TEXTURE0,0);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
        model_constant_rgb_draws++;
    }
    if(callback){
        callback_draws++;callback_constant_draws+=callback->constant_rgb;
        callback_mask_draws+=!!second;callback_hidden_draws+=callback->color[3]==0;
        C3D_TexEnv *env=C3D_GetTexEnv(0);
        unsigned rgba=callback->color[0]|(callback->color[1]<<8)|(callback->color[2]<<16)|((unsigned)callback->color[3]<<24);
        C3D_TexEnvColor(env,rgba);
        C3D_TexEnvSrc(env,C3D_RGB,callback->constant_rgb?GPU_CONSTANT:GPU_PRIMARY_COLOR,base?GPU_TEXTURE0:0,0);
        C3D_TexEnvFunc(env,C3D_RGB,base?GPU_MODULATE:GPU_REPLACE);
        C3D_TexEnvSrc(env,C3D_Alpha,GPU_CONSTANT,second?GPU_TEXTURE1:base?GPU_TEXTURE0:0,0);
        C3D_TexEnvFunc(env,C3D_Alpha,second || base?GPU_MODULATE:GPU_REPLACE);
        C3D_TexEnvOpAlpha(env,GPU_TEVOP_A_SRC_ALPHA,
            (second?second_source->format:base?texture->format:99)<=1?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
        if(callback->hologram){
            /* GX: colour=projected texture; alpha=clamp(channelA-maskA*K0A).
             * The original indirect matrix is zero. No distortion pass is
             * needed. PICA expresses the product/subtract in two TEV stages. */
            C3D_TexEnvColor(env,(callback->mask_alpha&255)<<24);
            C3D_TexEnvSrc(env,C3D_RGB,GPU_TEXTURE0,0,0);
            C3D_TexEnvFunc(env,C3D_RGB,GPU_REPLACE);
            C3D_TexEnvSrc(env,C3D_Alpha,GPU_TEXTURE1,GPU_CONSTANT,0);
            C3D_TexEnvOpAlpha(env,second_source->format<=1?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,0);
            C3D_TexEnvFunc(env,C3D_Alpha,GPU_MODULATE);
            env=C3D_GetTexEnv(1);
            C3D_TexEnvColor(env,(unsigned)callback->color[3]<<24);
            C3D_TexEnvSrc(env,C3D_Alpha,GPU_CONSTANT,GPU_PREVIOUS,0);
            C3D_TexEnvFunc(env,C3D_Alpha,GPU_SUBTRACT);
            hologram_draws++;
        }
    }
    if(disk){
        C3D_TexEnv *e=C3D_GetTexEnv(0);
        unsigned rgba=callback->color[0]|(callback->color[1]<<8)|(callback->color[2]<<16)|((unsigned)callback->color[3]<<24);
        C3D_TexEnvColor(e,rgba);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_ADD);
        unsigned inverse=second_source->format<=1?GPU_TEVOP_A_ONE_MINUS_SRC_R:GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA;
        unsigned direct=second_source->format<=1?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA;
        C3D_TexEnvSrc(e,C3D_Alpha,callback->blend==2?GPU_PRIMARY_COLOR:GPU_CONSTANT,GPU_TEXTURE1,0);
        C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,callback->blend==2?inverse:direct,0);
        C3D_TexEnvFunc(e,C3D_Alpha,callback->blend==2?GPU_MODULATE:GPU_SUBTRACT);
        e=C3D_GetTexEnv(1);
        C3D_TexEnvColor(e,(callback->mask_alpha&255)<<24);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_CONSTANT,0);
        C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
        normal_disk_draws++;
    }
    if(fade_rgba && !movie){model_color_fade_env(fade_rgba);model_fade_draws++;}
    C3D_CullFace(group->cull?GPU_CULL_FRONT_CCW:GPU_CULL_NONE);
    unsigned blend=alpha<1 || (material_flags&0x40000000u);
    unsigned cutout=(material_flags&0x400)!=0;
    unsigned test=!(model_flags&0x400),write=test && (!blend || (model_flags&0x2000));
    unsigned threshold=blend?((model_flags&0x2000)?0xdf:0):(cutout?0x40:0);
    C3D_DepthTest(test,GPU_GEQUAL,write?GPU_WRITE_ALL:GPU_WRITE_COLOR);
    C3D_AlphaTest(threshold!=0,GPU_GREATER,threshold);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,blend?GPU_SRC_ALPHA:GPU_ONE,
        blend?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
    if(movie){
        /* THPPlayerDrawCurrentFrame's opaque, unlit screen material. */
        C3D_CullFace(GPU_CULL_FRONT_CCW);
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
        C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        movie_draws++;
    }
    if(callback){
        C3D_CullFace(callback->constant_rgb && !callback->normal_disk?(group->cull?GPU_CULL_FRONT_CCW:GPU_CULL_NONE):GPU_CULL_FRONT_CCW);
        if(callback->hologram)C3D_CullFace(callback->cull_front?GPU_CULL_BACK_CCW:GPU_CULL_FRONT_CCW);
        C3D_DepthTest(callback->depth_test,GPU_GEQUAL,callback->depth_write?GPU_WRITE_ALL:GPU_WRITE_COLOR);
        C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,callback->blend?GPU_SRC_ALPHA:GPU_ONE,
            callback->blend==2?GPU_ONE:callback->blend?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
    }
    if(fh_minimal_cloud_depth){
        /* Original sun-occlusion cloud pass: scissored depth only, not a
         * second visible cloud layer. Original threshold override is 0x80. */
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_DEPTH);
        C3D_AlphaTest(true,GPU_GREATER,0x80);
    }
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    FH_MODEL_PROFILE(4);
    prepared_fog_apply(&p->fog,!movie && !callback);
    if(fh_minimal_frames<=2){
        char line[200];
        snprintf(line,sizeof(line),"Startup model: id=%u first=%u bones=%u vertices=%u indices=%u normal_palette=%u\n",
            asset->id,group->first,group->bones,group->vertices,group->indices,normal_palette);
        fh_platform_log(line);
    }
    C3D_DrawElements(GPU_TRIANGLES,group->indices,C3D_UNSIGNED_SHORT,group->index);
    if(lit){
        skin_lit_indices+=group->indices;
        if(group->bones==1){skin_single_bone_indices+=group->indices;skin_single_bone_draws++;}
    }
    __atomic_add_fetch(&fh_minimal_draws,1,__ATOMIC_RELAXED);__atomic_add_fetch(&fh_minimal_indices,group->indices,__ATOMIC_RELAXED);
    FH_MODEL_PROFILE(5);
    frame_draws++;frame_indices+=group->indices;submit_ticks+=svcGetSystemTick()-start;return 1;
}
int fh_minimal_group(const FhSceneAsset *asset,const FhSceneGroup *group,
    const float *poses,unsigned pose_count,const float view[3][4],float alpha,
    const FhTexture *texture,const FhTexture *sphere,unsigned sphere_intensity,
    unsigned material_flags,unsigned model_flags,const unsigned char *unlit_rgba,const unsigned scissor[4],
    const FhMinimalModelLayers *layers,const unsigned char *fade_rgba){
    FH_REPLAY_SCOPE(NATIVE_MODEL);
    /* World movie callbacks used this entry before compact world packets.
     * Never feed their20byte stride to the40byte skinned model fetch. */
    if(asset && asset->block && fh_minimal_movie_active){
        FhMinimalWorldMaterial movie={.flags=material_flags,.raster={1,1,1,1}};
        return fh_minimal_world_group(asset,group,view,&movie,scissor);
    }
#ifdef FH_EMULATOR_TEST
    u64 profile_stamp=svcGetSystemTick();

#endif
    if(fh_pica_scene_enabled || (fh_render_budget_world && !fh_render_budget_models))return 1;
    const FhMinimalCallbackMaterial *callback=fh_minimal_callback.active?&fh_minimal_callback:NULL;
    if(callback && (callback->hologram || callback->normal_disk) && (!callback->has_texture || !callback->has_mask)){
        fh_minimal_rejected++;return 0;
    }
    float palette[FH_MODEL_BONES][3][4];
    if(!initialized || !asset || !group || !asset->direct || !group->indices ||
       (layers && (layers->layered!=group->two_uv || !asset->compact || asset->block)) ||
       !fh_native_model_palette(palette,group->bone_ids,group->bones,poses,pose_count,
           (const float(*)[3])(asset->meta+asset->tails_at),view)){
        fh_minimal_rejected++;return 0;
    }
    FH_MODEL_PROFILE(0);
#if defined(FH_EMULATOR_TEST) || defined(FH_REPLAY_BUILD)
#ifdef FH_UNIFIED_APP
    if(fh_replay_active())
#endif
    capture_model_geometry(asset,group,palette,material_flags,model_flags);
#endif
    if(asset->compact && !asset->block){
        const FhAnimatedBounds *bounds=(!callback && !fh_minimal_movie_active && !fh_minimal_cloud_active)?fh_native_scene_model_bounds(asset,group):NULL;
        animated_bounds_checks++;
        if(fh_animated_bounds_outside(bounds,palette,group->bones,projection)){
            animated_bounds_culled++;animated_bounds_indices+=group->indices;return 1;
        }
        unsigned coverage=fh_native_collapsed_invisible(palette,group->bones,
            group->bone_ids,(const float(*)[3])(asset->meta+asset->tails_at),group->format,projection)?2:
            fh_native_collapsed_mesh_coverage(palette,group->bones,
                (const FhModelCompactVertex*)group->vertex,group->vertices,group->format,projection);
        /* Authorized 3DS detail reduction: a collapsed model whose inflated
         * bounds are <1/4 pixel in BOTH axes may omit a pixel-center speck.
         * This is LOD, not a claim of mathematically zero coverage. Ordinary
         * models, camera-plane crossings and nonfinite bounds remain intact.
         * Classification 3 separately rejects fully off-screen bounds. */
        if(coverage){
            collapsed_subpixel_lod_draws+=coverage==1;
            collapsed_offscreen_draws+=coverage==3;
            collapsed_culled_draws++;collapsed_culled_indices+=group->indices;return 1;
        }
    }
#ifdef FH_EMULATOR_TEST
    if(asset->block){
        static unsigned seen[32],used;
        unsigned i;for(i=0;i<used;i++)if(seen[i]==asset->id)break;
        if(i==used && used<32){seen[used++]=asset->id;char line[420];
            snprintf(line,sizeof(line),"Native world id=%u indices=%u texture=%ux%u fmt=%u flags=%08x position=%.2f,%.2f,%.2f projection=%.3f,%.3f,%.3f,%.3f vertex=%.3f,%.3f,%.3f\n",
                asset->id,group->indices,(unsigned)(texture?texture->width:0),(unsigned)(texture?texture->height:0),
                (unsigned)(texture?texture->format:0),material_flags,palette[0][0][3],palette[0][1][3],palette[0][2][3],
                projection[0][1],projection[1][0],projection[2][2],projection[3][2],
                group->vertex[0].position[0],group->vertex[0].position[1],group->vertex[0].position[2]);fh_platform_log(line);}
    }
    if(fh_minimal_cloud_active){
        static unsigned seen[16],used;
        unsigned i;for(i=0;i<used;i++)if(seen[i]==asset->id)break;
        if(i==used && used<16){seen[used++]=asset->id;char line[400];
            snprintf(line,sizeof(line),"Native cloud id=%u groups=%u shader=%08x model=%08x cull=%u indices=%u tex=%ux%u fmt=%u alpha=%.3f pos=%.3f,%.3f,%.3f\n",
                asset->id,asset->groups,material_flags,model_flags,group->cull,group->indices,
                (unsigned)(texture?texture->width:0),(unsigned)(texture?texture->height:0),(unsigned)(texture?texture->format:0),alpha,
                palette[0][0][3],palette[0][1][3],palette[0][2][3]);fh_platform_log(line);}
    }
#endif
    u64 start=svcGetSystemTick(),wait_start=prepared_wait_ticks;begin();
    flush_ui();
    unsigned clip[4];
    if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return 1;
    unsigned movie=fh_minimal_movie_active;
    if(movie)texture=&fh_minimal_movie_texture;
    if(callback){texture=callback->has_texture?&callback->texture:NULL;sphere=NULL;layers=NULL;fade_rgba=NULL;}
    C3D_Tex *base=callback || movie || unlit_rgba?base_texture(texture):surface_texture(texture);
    static C3D_Tex samplers[3];
    if(base)samplers[0]=*base;
    /* Keep the base pinned while the second texture may evict cache entries. */
    base_pin=base;
    C3D_Tex *illumination=!movie && !asset->block?base_texture(sphere):NULL;
    if(illumination)samplers[1]=*illumination;
    second_pin=illumination;
    C3D_Tex *second=callback && callback->has_mask?base_texture(&callback->mask):
        layers && layers->masked?base_texture(&layers->second):
        layers && layers->layered?surface_texture(&layers->second):NULL;
    if(second)samplers[2]=*second;
    second_pin=NULL;
    base_pin=NULL;
    if(layers && (layers->layered || layers->masked) && (!base || !second)){fh_minimal_rejected++;return 0;}
    if(callback && ((callback->has_texture && !base) || (callback->has_mask && !second))){fh_minimal_rejected++;return 0;}
    if(sphere && !movie && !illumination)sphere_failures++;
    FH_MODEL_PROFILE(1);
    FhNativeLighting lighting;
    unsigned lit=!asset->block && !movie && !unlit_rgba && !(callback && callback->constant_rgb && !callback->normal_disk);
    if(lit && !fh_gx_native_lighting(&lighting)){
        lighting_rejected++;lit=0;
    }
    FhPreparedModel local;FhPreparedPayload *slot=prepared_reserve(PREPARED_MODEL);
    FhPreparedModel *ready=slot?&slot->model:&local;
    ready->asset=*asset;ready->group=*group;
    memcpy(ready->palette,palette,group->bones*sizeof(palette[0]));
    memcpy(ready->projection,projection,sizeof(ready->projection));
    ready->lit=lit;if(lit)ready->lighting=lighting;
    ready->has_callback=callback!=NULL;if(callback)ready->callback=*callback;
    ready->has_layers=layers!=NULL;if(layers)ready->layers=*layers;
    ready->has_texture=texture!=NULL;if(texture)ready->texture=*texture;
    ready->has_sphere=sphere!=NULL;if(sphere)ready->sphere=*sphere;
    ready->base=base!=NULL;ready->illumination=illumination!=NULL;ready->second=second!=NULL;
    memcpy(ready->samplers,samplers,sizeof(ready->samplers));
    ready->has_unlit=unlit_rgba!=NULL;if(unlit_rgba)memcpy(ready->unlit,unlit_rgba,4);
    ready->has_fade=fade_rgba!=NULL;if(fade_rgba)memcpy(ready->fade,fade_rgba,4);
    ready->cloud_depth=fh_minimal_cloud_depth;ready->cloud_active=fh_minimal_cloud_active;
    ready->alpha=alpha;ready->sphere_intensity=sphere_intensity;
    ready->material_flags=material_flags;ready->model_flags=model_flags;
    memcpy(ready->clip,clip,sizeof(ready->clip));ready->movie=movie;
#ifdef FH_EMULATOR_TEST
    memcpy(ready->capture_scissor,scissor,sizeof(ready->capture_scissor));
#endif
    ready->fog=prepared_fog();ready->start=start;
#ifdef FH_EMULATOR_TEST
    ready->profile_stamp=profile_stamp;
#endif
    u64 preparation=svcGetSystemTick()-start-(prepared_wait_ticks-wait_start);prepared_prep_ticks+=preparation;
    fh_native_scene_used(asset); /* Publish lifetime before a later game write. */
    if(slot){prepared_commit(PREPARED_MODEL);return 1;}
    return submit_prepared_model(ready);
}
#undef FH_MODEL_PROFILE

void fh_minimal_present(void){
    FH_REPLAY_SCOPE(PRESENT_PREP);
    /* GXCopyDisp and the retrace service can both call present. Only submit
     * actual pending geometry; a second empty present must not erase it. */
    if(!initialized)return;
    static unsigned presented_options=FH_GRAPHICS_ALL;
    if(presented_options!=fh_graphics_options){
        /* Clear the previous image even in a HUD-only scene with all draws
         * disabled. Other empty duplicate presents retain normal semantics. */
        if(fh_graphics_options&FH_GRAPHICS_NO_RENDER)begin();
        presented_options=fh_graphics_options;
    }
    fh_pica_hud_flush();
    if(!open_frame)return;
    flush_ui();prepared_join();
    {FH_REPLAY_SCOPE(PRESENT_COPY);native_surface_present();}
    {FH_REPLAY_SCOPE(PRESENT_PUBLISH);fh_early_queue_defer();C3D_FrameEnd(GX_CMDLIST_FLUSH);}open_frame=0;
    /* End-to-end submission cadence, including waits between these points.
     * This is not display scanout or a claim about emulator/console scaling. */
    u64 now=svcGetSystemTick();if(!first_present_ticks)first_present_ticks=now;
    if(fh_graphics_fps_visible)fh_graphics_options_frame(now,SYSCLOCK_ARM11);
    if(metrics){
        const FhFrameSample sample={.frame=fh_minimal_frames,.draws=frame_draws,.indices=frame_indices,
            .now=now,.submit_ticks=submit_ticks+prepared_prep_ticks,.gpu_ms=C3D_GetDrawingTime(),
            .missing=fh_minimal_missing,.rejected=fh_minimal_rejected,.uploads=base_uploads,.changes=base_changes,
            .slot_evictions=base_slot_evictions,.budget_evictions=base_budget_evictions,
            .live=base_live,.gpu_bytes=base_gpu_bytes,.cpu_bytes=base_cpu_bytes};
        fh_frame_metrics_observe(&frame_metrics,&sample);
    }
    last_present_ticks=now;
    /* Reuse the buffered native log; no extra per-frame file writes. */
    if(fh_minimal_frames%120==0){gpu_work_report();base_vram_report();}
    fh_minimal_frames++;frame_draws=frame_indices=0;submit_ticks=prepared_prep_ticks=0;
}
/* Native semantic consumer. No legacy GX copy callback or CPU readback. */
#include "platform_pica_glow_reflection_capture.h"
#include "platform_pica_screen_filter.h"
#include "platform_pica_spirit_filter.h"
static C3D_Tex *native_water_capture(unsigned requested,unsigned effects){
    static unsigned captured_frame=~0u;
    requested &= fh_graphics_options&~fh_graphics_unavailable_mask;
    effects &= fh_graphics_effects&~fh_effects_unavailable_mask;
    if(!initialized || (!requested && !effects) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_PARTICLE_TEX)))return NULL;
    if(captured_frame==fh_minimal_frames && native_surface_reflection_target)
        return &native_surface_reflection;
    fh_minimal_auxiliary_begin();
    if(!native_surface_reflection_target){
        extern u32 __ctru_linear_heap_size;
        if(vramSpaceFree()<32768+1024*1024 ||
           linearSpaceFree()<6*sizeof(NativeSurfaceVertex)+__ctru_linear_heap_size*15/100)
        {
            fh_minimal_auxiliary_end(0,0);
            fh_graphics_options_reject(requested);fh_graphics_effects_reject(effects);return NULL;
        }
    }
    C3D_Tex *result=native_surface_capture(target,&ui_program);
    if(!result){
        fh_minimal_auxiliary_end(0,0);fh_graphics_options_reject(requested);fh_graphics_effects_reject(effects);return NULL;
    }
    captured_frame=fh_minimal_frames;
    fh_minimal_auxiliary_end(1,6);
    return result;
}
C3D_Tex *fh_pica_splash_capture(void){return native_water_capture(FH_GRAPHICS_SPLASH_REFLECTION,0);}
const void *fh_pica_water_sheet_capture(void){return native_water_capture(FH_GRAPHICS_WAKES,FH_EFFECT_RIPPLES);}
int fh_minimal_reflection_update(void *destination,const uint32_t source[4],unsigned w,unsigned h,unsigned format,int half,int clear){
    if(!initialized || !destination || format!=4 || w!=320 || h!=240 || !half || clear ||
        source[0] || source[1] || source[2]!=640 || source[3]!=480)return 0;
    begin();flush_ui();prepared_join();u64 started=svcGetSystemTick();
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Native reflection command bounds");
    if((capacity-offset)*4<16*1024)fh_minimal_sync();
    if(!native_surface_capture(target,&ui_program)){
        submit_ticks+=svcGetSystemTick()-started;return 0;
    }
    frame_draws++;frame_indices+=6;submit_ticks+=svcGetSystemTick()-started;
    return 1;
}
static void frame_capture_snapshot(void){
    FILE *flag=fopen("sdmc:/3ds/foxhollow/frame_capture.txt","rb");if(!flag)return;fclose(flag);
    C3D_Context *ctx=C3Di_GetContext();char line[240];
    gxCmdQueue_s *q=&ctx->gxQueue;
    unsigned count=q->numEntries,last=q->lastEntry,current=q->curEntry;
    snprintf(line,sizeof(line),"GPU timeout queue: count=%u last=%u current=%u capacity=%u world_sampler_resets=%u\n",
        count,last,current,q->maxEntries,world_sampler_resets);fh_platform_log(line);
    if(q->entries && count<=q->maxEntries && count<=32)for(unsigned i=0;i<count;i++){
        const u32 *d=q->entries[i].data;
        snprintf(line,sizeof(line),"GPU queue %u: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
            i,(unsigned long)d[0],(unsigned long)d[1],(unsigned long)d[2],(unsigned long)d[3],
            (unsigned long)d[4],(unsigned long)d[5],(unsigned long)d[6],(unsigned long)d[7]);fh_platform_log(line);
    }
    if(ctx->cmdBuf && ctx->cmdBufSize && ctx->cmdBufSize<=65536){
        FILE *f=fopen("sdmc:/3ds/foxhollow/frame-commands.bin","wb");
        if(f){size_t words=fwrite(ctx->cmdBuf,4,ctx->cmdBufSize,f);int closed=fclose(f);
            snprintf(line,sizeof(line),"Frame capture commands: words=%u expected=%u close=%d\n",(unsigned)words,(unsigned)ctx->cmdBufSize,closed);fh_platform_log(line);}
    }
    extern u32 __ctru_linear_heap,__ctru_linear_heap_size;
    unsigned char *scratch=malloc(FH_GPU_SNAPSHOT_CHUNK);
    FILE *f=scratch?fopen("sdmc:/3ds/foxhollow/frame-inputs.bin","wb"):NULL;
    int ok=0,closed=-1;
    if(f){
        ok=fh_gpu_input_snapshot(f,(const unsigned char *)(uintptr_t)__ctru_linear_heap,
            __ctru_linear_heap,osConvertVirtToPhys((void *)(uintptr_t)__ctru_linear_heap),
            __ctru_linear_heap_size,(u32)(uintptr_t)ctx->cmdBuf,
            osConvertVirtToPhys(ctx->cmdBuf),ctx->cmdBufSize*4,scratch);
        closed=fclose(f);
    }
    free(scratch);
    snprintf(line,sizeof(line),"GPU timeout CPU inputs: bytes=%lu complete=%d close=%d; CPU view only\n",(unsigned long)__ctru_linear_heap_size,ok,closed);fh_platform_log(line);
}
void fh_minimal_shutdown(void){
    if(!initialized)return;
    if(fh_platform_capture_requested())frame_capture_snapshot();
    fh_minimal_worker_stop();fh_minimal_sync();{extern void fh_program_plan_report(void);fh_program_plan_report();}
    fh_native_scene_shutdown();
    for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
    base_vram_report();
    gpu_work_report();
    char report[320];snprintf(report,sizeof(report),
        "Minimal lighting: draws=%u rejected=%u max_lights=%u max_complex=%u\n",lit_draws,lighting_rejected,lighting_max,lighting_complex_max);fh_platform_log(report);
    extern unsigned fh_geometry_boundary_enabled,fh_geometry_boundary_edges;
    extern unsigned fh_geometry_boundary_splits,fh_geometry_boundary_separated;
    extern unsigned fh_queue_capacity_drains;
    snprintf(report,sizeof(report),"Native geometry boundary: enabled=%u edges=%u splits=%u already_separated=%u\n",
        fh_geometry_boundary_enabled,fh_geometry_boundary_edges,fh_geometry_boundary_splits,fh_geometry_boundary_separated);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native queue capacity: drains=%u\n",fh_queue_capacity_drains);fh_platform_log(report);
    extern unsigned fh_sampler_reuse_enabled;
    extern unsigned long long fh_sampler_reuse_counts[2];
    snprintf(report,sizeof(report),"Native sampler reuse: enabled=%u requested=%llu retained=%llu\n",
        fh_sampler_reuse_enabled,fh_sampler_reuse_counts[0],fh_sampler_reuse_counts[1]);fh_platform_log(report);
    extern unsigned long long fh_prepared_buffer_counts[5];
    snprintf(report,sizeof(report),"Native prepared buffers: binds=%llu layout_changes=%llu full=%llu offset_writes=%llu ordinary=%llu\n",
        fh_prepared_buffer_counts[0],fh_prepared_buffer_counts[1],fh_prepared_buffer_counts[2],
        fh_prepared_buffer_counts[3],fh_prepared_buffer_counts[4]);fh_platform_log(report);
    for(unsigned k=0;k<6;k++){
        snprintf(report,sizeof(report),"Native raster reuse: enabled=%u kind=%u requested=%llu retained=%llu\n",
            fh_raster_reuse_enabled,k,fh_raster_reuse_counts[k][0],fh_raster_reuse_counts[k][1]);fh_platform_log(report);
    }
    snprintf(report,sizeof(report),"Native uniform batches: enabled=%u requested=%llu uploaded=%llu retained=%llu\n",
        fh_uniform_reuse_enabled,fh_uniform_reuse_counts[0],fh_uniform_reuse_counts[1],fh_uniform_reuse_counts[2]);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native queue overlap: submissions=%u starts=%u finishes=%u deferrals=%u retirements=%u\n",
        queue_submissions,fh_early_queue_starts,fh_early_queue_finishes,fh_early_queue_deferrals,fh_early_queue_retirements);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native world culling: checked=%u removed_indices=%u removed_groups=%u additional_draws=%u\n",
        world_cull_checks,world_culled_indices,world_culled_groups,world_split_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native world index stream: draws=%u fallback=%u saved_draws=%u copied_bytes=%llu peak_bytes=%u linear_bytes=%u\n",
        world_gather_draws,world_gather_fallbacks,world_gather_saved_draws,
        (unsigned long long)world_gather_bytes,world_stream.peak*2,world_stream.capacity*2);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native skin work: lit_index_references=%llu single_bone_index_references=%llu single_bone_draws=%u sphere_updates=%u\n",
        (unsigned long long)skin_lit_indices,(unsigned long long)skin_single_bone_indices,
        skin_single_bone_draws,sphere_updates);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native normal palettes: draws=%u fallback=%u bones=%u index_references=%llu extra_linear=0 extra_vram=0\n",
        normal_palette_draws,normal_palette_fallbacks,normal_palette_bones,
        (unsigned long long)normal_palette_indices);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native animated culling: checks=%u draws=%u indices=%llu\n",animated_bounds_checks,animated_bounds_culled,(unsigned long long)animated_bounds_indices);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native collapsed culling: draws=%u index_references=%llu subpixel_lod=%u offscreen=%u\n",
        collapsed_culled_draws,(unsigned long long)collapsed_culled_indices,collapsed_subpixel_lod_draws,collapsed_offscreen_draws);fh_platform_log(report);
#ifdef FH_EMULATOR_TEST
    snprintf(report,sizeof(report),"Native model profile ticks: palette=%llu textures=%llu binding=%llu lighting=%llu material=%llu draw=%llu hz=%llu\n",
        (unsigned long long)model_profile[0],(unsigned long long)model_profile[1],
        (unsigned long long)model_profile[2],(unsigned long long)model_profile[3],
        (unsigned long long)model_profile[4],(unsigned long long)model_profile[5],
        (unsigned long long)SYSCLOCK_ARM11);fh_platform_log(report);
#endif
    snprintf(report,sizeof(report),"Native constant lighting: mode=%u draws=%u indices=%llu\n",
        constant_light_mode,constant_light_draws,(unsigned long long)constant_light_indices);fh_platform_log(report);
    snprintf(report,sizeof(report),"Minimal movie: draws=%u\n",movie_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native flat world: setups=%u reused=%u\n",flat_world_setups,flat_world_reuses);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native world state: material_reuses=%u pipeline_reuses=%u\n",world_material_reuses,world_pipeline_reuses);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native world index publication: calls=%u bytes=%llu\n",
        world_index_publications,(unsigned long long)world_index_published_bytes);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native world: draws=%u layered=%u failed=%u ticks=%llu deferred=%u movie=%u\n",
        world_draws,world_layered_draws,world_failures,(unsigned long long)(world_ticks+prepared_world_prep_ticks),world_deferred_draws,world_movie_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native world lights: draws=%u selected=%u normal_bytes=%u\n",
        world_light_draws,world_light_selected,(unsigned)(sizeof(world_light_env)+sizeof(world_lights)+sizeof(world_light_lut)));fh_platform_log(report);
    snprintf(report,sizeof(report),"Minimal sphere: draws=%u failed=%u\n",sphere_draws,sphere_failures);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native world falloff: draws=%u\n",world_falloff_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native world sampler resets: %u\n",world_sampler_resets);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native model layers: draws=%u\n",model_layered_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native model opacity masks: draws=%u\n",model_mask_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native callback materials: draws=%u constant=%u masks=%u transparent=%u\n",callback_draws,callback_constant_draws,callback_mask_draws,callback_hidden_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native hologram: calls=%u draws=%u\n",fh_minimal_hologram_calls,hologram_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native normal disk: draws=%u\n",normal_disk_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native material opacity: constant_draws=%u\n",model_constant_alpha_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native material RGB: constant_draws=%u\n",model_constant_rgb_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native model color fades: draws=%u\n",model_fade_draws);fh_platform_log(report);
    fh_native_fog_report();
    snprintf(report,sizeof(report),"Native water: draws=%u failures=%u ticks=%llu\n",water_draws,water_failures,(unsigned long long)water_ticks);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native water effects: draws=%u failures=%u ticks=%llu extra_linear=0\n",water_effect_draws,water_effect_failures,(unsigned long long)water_effect_ticks);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native splash rings: bursts=%u triangles_per_pass=210 extra_linear=0\n",splash_draws);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native reflection: copies=%u vram_extra=%u linear=%u\n",native_surface_copies,2*256*512*4+240*400*4-2*240*400*4+(native_surface_reflection.data?128*128*2:0),native_surface_quad?(unsigned)(6*sizeof(*native_surface_quad)):0);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native sphere updates: draws=%u failed=%u vram=%u linear=%u ticks=%llu\n",
        sphere_updates,sphere_update_failures,sphere_vram_bytes,sphere_vertices?
        (unsigned)(SPHERE_VERTICES*sizeof(*sphere_vertices)+SPHERE_INDICES*sizeof(*sphere_indices)):0,
        (unsigned long long)sphere_update_ticks);fh_platform_log(report);
    snprintf(report,sizeof(report),
        "Minimal textures: uploads=%u hits=%u failed=%u retired=%u compare_bytes=%llu ticks=%llu pack_hits=%u\n",
        base_uploads,base_hits,base_failures,base_retired,(unsigned long long)base_compare_bytes,
        (unsigned long long)base_ticks,texture_pack.hits);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native texture residency: peak=%u slots=%u changes=%u slot_evictions=%u budget_evictions=%u metadata=%u\n",
        base_peak,BASE_TEXTURES,base_changes,base_slot_evictions,base_budget_evictions,(unsigned)(sizeof(base_textures)+sizeof(base_index)));fh_platform_log(report);
    for(unsigned i=0;i<128;i++)if(coverage_failures[i].count){
        snprintf(report,sizeof(report),"Native coverage failure: site=%u asset=%u detail=%u count=%u\n",
            coverage_failures[i].site,coverage_failures[i].asset,coverage_failures[i].detail,coverage_failures[i].count);fh_platform_log(report);
    }
    if(coverage_overflow){snprintf(report,sizeof(report),"Native coverage overflow: %u\n",coverage_overflow);fh_platform_log(report);}
    snprintf(report,sizeof(report),"Native CMPR16: uploads=%u saved_upload_bytes=%llu\n",
        base_cmpr16_uploads,(unsigned long long)base_cmpr16_saved_bytes);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native texture scope: mode=%u hits=%llu verified=%llu\n",
        texture_scope_mode,(unsigned long long)texture_scope_hits,(unsigned long long)texture_scope_checks);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native texture assets: mode=%u registered=%u forgotten=%u hits=%u checked=%u bytes=%u\n",
        texture_asset_mode,texture_asset_registered,texture_asset_forgotten,texture_asset_hits,texture_asset_checks,
        (unsigned)sizeof(texture_assets));fh_platform_log(report);
    snprintf(report,sizeof(report),"Native texture registry: searches=%u negative_hits=%u miss_storage=%u\n",
        texture_asset_searches,texture_asset_negative_hits,(unsigned)(BASE_TEXTURES*sizeof(unsigned)));fh_platform_log(report);
    snprintf(report,sizeof(report),"Native file texture hints: retained=%u\n",texture_asset_hint_retained);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native texture cache IO: allocation=%u reads=%llu bytes=%llu startup_sd_bytes=%llu runtime_sd_bytes=%llu releases=%u\n",
        texture_resident_allocation,(unsigned long long)texture_pack.resident_reads,
        (unsigned long long)texture_pack.resident_read_bytes,(unsigned long long)texture_resident_startup_read_bytes,
        (unsigned long long)(texture_pack.read_bytes-texture_resident_startup_read_bytes),texture_resident_releases);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native texture window IO: allocation=%u hits=%llu misses=%llu served_bytes=%llu releases=%u\n",
        texture_window_allocation,(unsigned long long)texture_pack.window_hits,
        (unsigned long long)texture_pack.window_misses,(unsigned long long)texture_pack.window_bytes,
        texture_window_releases);fh_platform_log(report);
    fh_minimal_texture_cache_release();fh_texture_pack_close(&texture_pack);
    snprintf(report,sizeof(report),"Minimal uploads: writes=%u same_frame_drains=%u\n",upload_writes,upload_drains);fh_platform_log(report);
    snprintf(report,sizeof(report),"Minimal UI: quads=%u draws=%u linear_bytes=%u attempts=%u empty=%u projected=%u\n",ui_quads,ui_draws,
        (unsigned)(UI_VERTICES*sizeof(*ui_vertices)),ui_attempts,ui_empty,ui_projected_quads);fh_platform_log(report);
    snprintf(report,sizeof(report),"Native effects: quads=%u batches=%u shared_linear_bytes=%u triangles=%u\n",effect_quads,effect_draws,
        (unsigned)(UI_VERTICES*sizeof(*ui_vertices)),effect_triangles);fh_platform_log(report);
    target->used=false;C3D_FrameEnd(GX_CMDLIST_FLUSH);open_frame=0;
    acquire();target->used=false;C3D_FrameEnd(GX_CMDLIST_FLUSH);
    for(unsigned i=0;i<SPHERE_SLOTS;i++)if(sphere_maps[i].target){
        C3D_RenderTargetDelete(sphere_maps[i].target);C3D_TexDelete(&sphere_maps[i].texture);
    }
    linearFree(sphere_vertices);linearFree(sphere_indices);sphere_vertices=NULL;sphere_indices=NULL;
    fh_gpu_cache_report();
    linearFree(ui_vertices);ui_vertices=NULL;
    linearFree(world_stream.data);memset(&world_stream,0,sizeof(world_stream));
    water_sheet_shutdown();
    fh_pica_scene_shutdown();
    shaderProgramFree(&program);shaderProgramFree(&ui_program);
    shaderProgramFree(&world_program);shaderProgramFree(&water_program);shaderProgramFree(&hologram_program);
    distortion_shutdown();heat_shutdown();blur_shutdown();motion_shutdown();
    DVLB_Free(binary);C3D_RenderTargetDelete(target);spirit_filter_report();screen_filter_destroy();glow_reflection_capture_destroy();native_surface_destroy();C3D_Fini();
    if(metrics){fh_frame_metrics_flush(&frame_metrics);fclose(metrics);}
    metrics=NULL;initialized=0;
}

/* The replacement shares only frame ownership, target and resident allocation
 * lifetimes. Its own module emits all geometry commands and shader state. */
unsigned fh_pica_scene_reserve_changed(void){
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("PICA scene command bounds");
    if((capacity-offset)*4<16*1024){owner_gpu_sync();return 1;}
    return 0;
}
void fh_pica_scene_reserve(void){(void)fh_pica_scene_reserve_changed();}
/* Consume live game camera state directly. The new backend must not depend
 * on a GX dispatcher build option to observe the current projection. */
static void pica_live_projection(float out[4][4],const float matrix[4][4]){
    for(unsigned c=0;c<4;c++){
        out[0][c]=matrix[1][c];out[1][c]=-matrix[0][c];
        out[2][c]=matrix[2][c];out[3][c]=matrix[3][c];
    }
}

#include "fh_gpu_queue_mode.h"
static void native_scene_submitted(unsigned draws){
    static unsigned frame=~0u,prefixes,logged;
    if(!logged){logged=1;fh_platform_log(fh_gpu_queue_early()?"GPU queue mode: early prefixes\n":"GPU queue mode: frame end\n");}
    if(!fh_gpu_queue_early() || !draws || fh_minimal_frames<=2)return;
    if(frame!=fh_minimal_frames){frame=fh_minimal_frames;prefixes=0;}
    if(prefixes>=2)return;
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Native prefix command bounds");
    if(offset<32u*1024u/4u)return;
    if(fh_early_queue_append()){prefixes++;queue_submissions++;}
    else prefixes=2;
}

void fh_pica_scene_world(const FhSceneAsset *asset,const float view[3][4],const float camera_projection[4][4],const unsigned scissor[4]){
    fh_pica_hud_flush();
    unsigned clip[4];if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return;
    const FhPicaWorldPlan *plan=fh_native_scene_pica_plan(asset);
    if(!plan){fh_minimal_coverage_failure(28,asset->id,0);return;}
    begin();unsigned indices=0;
    float native_projection[4][4];pica_live_projection(native_projection,camera_projection);
    unsigned draws=fh_pica_scene_draw(asset,plan,view,native_projection,clip,&indices);
    fh_native_scene_used(asset);
    fh_minimal_draws+=draws;fh_minimal_indices+=indices;frame_draws+=draws;frame_indices+=indices;
    native_scene_submitted(draws);
}


extern unsigned fh_pica_world_slice_draw(const FhSceneAsset*,const FhPicaWorldDraw*,
    const float[3][4],const float[4][4],const unsigned[4]);
void fh_pica_scene_world_slice(const FhSceneAsset *asset,const FhPicaWorldDraw *g,
    const float view[3][4],const float projection[4][4],const unsigned scissor[4]){
    fh_pica_hud_flush();unsigned clip[4];
    if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return;
    begin();float native_projection[4][4];pica_live_projection(native_projection,projection);
    unsigned count=fh_pica_world_slice_draw(asset,g,view,native_projection,clip);
    fh_native_scene_used(asset);
    fh_minimal_draws+=count;fh_minimal_indices+=count*g->indices;
    frame_draws+=count;frame_indices+=count*g->indices;
}

#include "fh_pica_hologram.h"
void fh_pica_scene_hologram(const FhSceneAsset *asset,const float *poses,unsigned joints,const float world[3][4],const float view[3][4],const float camera_projection[4][4],const unsigned scissor[4],const FhPicaHologram *state){
    fh_pica_hud_flush();unsigned clip[4];if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return;
    const FhPicaModelPlan *plan=fh_native_scene_pica_model(asset);
    if(!plan)return;
    begin();unsigned indices=0;float projection[4][4];pica_live_projection(projection,camera_projection);
    unsigned draws=fh_pica_hologram_draw(asset,plan,poses,joints,world,view,projection,clip,state,&indices);
    fh_native_scene_used(asset);fh_minimal_draws+=draws;fh_minimal_indices+=indices;frame_draws+=draws;frame_indices+=indices;
}
#include "fh_pica_actor_fog_live.h"
#include "fh_pica_height_fog.h"
static void pica_scene_actor(const FhSceneAsset *asset,const float *poses,unsigned joints,const float world[3][4],const float view[3][4],const float camera_projection[4][4],const unsigned scissor[4],float near_z,float far_z,const FhActorFogAsset*fog,const FhPicaActorFogSnapshot*snapshot){
    fh_pica_hud_flush();
    unsigned clip[4];if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return;
    const FhPicaModelPlan *plan=fh_native_scene_pica_model(asset);
    if(!plan){fh_minimal_coverage_failure(35,asset->id,0);return;}
    begin();unsigned indices=0;
    float native_projection[4][4];pica_live_projection(native_projection,camera_projection);
    unsigned draws=0;
    if(fog && snapshot && (fh_graphics_options&FH_GRAPHICS_HEIGHT_FOG) &&
       !(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS))){
        /* begin() owns command publication. The snapshot and source skeleton
         * are consumed synchronously; no worker sees these borrowed pointers. */
        draws=fh_pica_scene_actor_fog_draw(asset,plan,fog,&snapshot->live,poses,joints,
            world,view,native_projection,clip,near_z,far_z,&indices);
    }
    if(!draws)draws=fh_pica_scene_actor_draw(asset,plan,poses,joints,world,view,native_projection,clip,near_z,far_z,&indices);
    fh_native_scene_used(asset);fh_minimal_draws+=draws;fh_minimal_indices+=indices;frame_draws+=draws;frame_indices+=indices;
    native_scene_submitted(draws);
}
void fh_pica_scene_actor_with_fog(const FhSceneAsset*asset,const FhActorFogAsset*fog,
    const FhPicaActorFogSnapshot*snapshot,const float*poses,unsigned joints,
    const float world[3][4],const float view[3][4],const float projection[4][4],const unsigned scissor[4]){
    pica_scene_actor(asset,poses,joints,world,view,projection,scissor,0,1,fog,snapshot);
}
void fh_pica_scene_actor(const FhSceneAsset *asset,const float *poses,unsigned joints,const float world[3][4],const float view[3][4],const float camera_projection[4][4],const unsigned scissor[4]){
    pica_scene_actor(asset,poses,joints,world,view,camera_projection,scissor,0,1,NULL,NULL);
}
void fh_pica_scene_ui_actor(const FhSceneAsset *asset,const float *poses,unsigned joints,const float world[3][4],const float view[3][4],const float camera_projection[4][4],const unsigned scissor[4]){
    float viewport[6],projection[4][4];fh_pica_live_viewport(viewport);
    fh_pica_viewport_xy(projection,camera_projection,viewport);
    pica_scene_actor(asset,poses,joints,world,view,projection,scissor,viewport[4],viewport[5],NULL,NULL);
}
int fh_pica_hud_target(const unsigned scissor[4],unsigned clip[4]){
    if(!initialized || !mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return 0;
    begin();return 1;
}
void fh_pica_hud_account(unsigned vertices){
    fh_minimal_draws++;fh_minimal_indices+=vertices;frame_draws++;frame_indices+=vertices;
}
