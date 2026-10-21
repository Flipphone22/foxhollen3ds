/* Pinned Melee explicit cache publication/resource retirement, adapted to the
 * existing native frame owner. Immutable atlas/views; no hot VRAM copies,
 * readback, per-update writes/fences, or allocation. First consumer is
 * single-texture world materials; actors and indirect distortion are absent. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_height_fog.h"
#include "fh_pica_color_fade.h"
#include "fh_pica_post_stages.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_graphics_options.h"
static C3D_Tex atlas,images[FH_HEIGHT_FOG_STATES];
static unsigned ready,valid,intensity;
static unsigned updates,reuses,selections;
static unsigned prepares,resident_peak;
static u64 prepare_ticks;
static const uint8_t *world_source;
static unsigned world_intensity,world_color,world_indirect,world_draws,world_skipped,world_partial;
static float world_planes[2][4];
static C3D_TexEnv world_env,world_layered_env;
static unsigned world_layered_draws;
static unsigned planes_published,stage_valid,stage_enabled,stage_color;
static unsigned plane_uploads,stage_uploads;
void fh_pica_height_fog_stage_invalidate(void){stage_valid=0;}
void fh_pica_height_fog_invalidate(void){planes_published=0;stage_valid=0;}
static unsigned snapshots, intensity_min=64, intensity_max;
unsigned fh_pica_height_fog_source_updates;
static float camera_min,camera_max,range_top,range_bottom;
void fh_pica_height_fog_world_disable(void){world_source=NULL;}
void fh_pica_height_fog_world_set(const uint8_t *source,unsigned value,
    const float inverse_view[3][4],float depth_scale,float depth_offset,
    float top,float bottom,unsigned color,unsigned indirect){
    world_source=NULL;
    if(!source || !(fh_graphics_options&FH_GRAPHICS_HEIGHT_FOG))return;
    float planes[2][4];
    if(value>64 || !fh_pica_height_fog_planes(planes,inverse_view,depth_scale,depth_offset,top,bottom))
        fh_platform_panic("Native height fog parameters");
    if(memcmp(planes,world_planes,sizeof(planes))){
        memcpy(world_planes,planes,sizeof(planes));planes_published=0;
    }
    world_source=source;world_intensity=value;world_color=color;world_indirect=indirect;
    /* Bounded coverage telemetry, written only at joined shutdown. */
    float camera_y=inverse_view[1][3];
    if(!snapshots || camera_y<camera_min)camera_min=camera_y;
    if(!snapshots || camera_y>camera_max)camera_max=camera_y;
    if(value<intensity_min)intensity_min=value;
    if(value>intensity_max)intensity_max=value;
    range_top=top;range_bottom=bottom;snapshots++;
}
unsigned fh_pica_height_fog_world_active(void){return world_source!=NULL && (fh_graphics_options&FH_GRAPHICS_HEIGHT_FOG)!=0;}
void fh_pica_height_fog_world_skipped(void){if(fh_pica_height_fog_world_active())world_skipped++;}
void fh_pica_height_fog_world_prepare(void){
    if(!fh_pica_height_fog_world_active())fh_platform_panic("Native height fog preparation without snapshot");
    fh_pica_height_fog_update(world_source,world_intensity);
}
void fh_pica_height_fog_update(const uint8_t *source,unsigned value){
    if(!source || value>64)fh_platform_panic("Native height fog source");
    if(valid && intensity==value){reuses++;return;}
    if(!ready){
        extern u32 __ctru_linear_heap_size;
        unsigned free_bytes=linearSpaceFree(),bytes=FH_HEIGHT_FOG_ATLAS_BYTES;
        if(free_bytes<bytes || (u64)(free_bytes-bytes)*100<(u64)__ctru_linear_heap_size*15)
            fh_platform_panic("Native height fog resident budget");
        u64 start=svcGetSystemTick();
        if(!C3D_TexInit(&atlas,64,1024,GPU_A8))fh_platform_panic("Native height fog allocation");
        C3D_TexSetFilter(&atlas,GPU_LINEAR,GPU_LINEAR);
        C3D_TexSetWrap(&atlas,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
        fh_pica_height_fog_atlas_prepare(atlas.data);
        fh_gpu_cache_flush(atlas.data,bytes);
        for(unsigned i=0;i<FH_HEIGHT_FOG_STATES;i++){
            unsigned offset=fh_pica_height_fog_atlas_offset(i);
            if(offset>bytes-FH_HEIGHT_FOG_BYTES || (offset&511))fh_platform_panic("Native height fog view bounds");
            images[i]=atlas;
            images[i].data=(uint8_t*)atlas.data+offset;
            images[i].height=64;images[i].size=FH_HEIGHT_FOG_BYTES;
        }
        C3D_TexEnv *e=&world_env;C3D_TexEnvInit(e);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,GPU_PREVIOUS,GPU_TEXTURE1);
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,0,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);
        world_layered_env=world_env;
        C3D_TexEnvSrc(&world_layered_env,C3D_RGB,GPU_CONSTANT,GPU_PREVIOUS,GPU_TEXTURE2);
        ready=1;prepares++;resident_peak=bytes;prepare_ticks+=svcGetSystemTick()-start;
    }
    /* Every descriptor and texel is immutable for the entire owner lifetime.
     * Existing GPU readers survive arbitrarily many same-frame selections. */
    intensity=value;valid=1;updates++;
}
const C3D_Tex *fh_pica_height_fog_sampler(void){
    if(!valid)return NULL;
    selections++;return images+intensity;
}
void fh_pica_height_fog_world_bind(unsigned enabled){
    if(!fh_pica_height_fog_world_active())fh_platform_panic("Native height fog draw without snapshot");
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,enabled==1);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x69,enabled==2);
    /* The scene selects colour-fade passthrough for draws without
     * height fog. This owner never touches material stages 0..3. */
    if(!enabled)return;
    const C3D_Tex *sampler=fh_pica_height_fog_sampler();
    if(!sampler || intensity!=world_intensity)fh_platform_panic("Native height fog image not prepared");
    C3D_TexBind(enabled==2?2:1,(C3D_Tex*)sampler);
    if(!planes_published){
        for(unsigned i=0;i<2;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,10+i,
            world_planes[i][0],world_planes[i][1],world_planes[i][2],world_planes[i][3]);
        planes_published=1;plane_uploads++;
    }
    if(!stage_valid || stage_enabled!=enabled || stage_color!=world_color){
        /* World materials retain all four compiled stages. This slot
         * replaces the actor-only colour fade; restore that owner on return. */
        C3D_SetTexEnv(FH_PICA_WORLD_HEIGHT_FOG_STAGE,enabled==2?&world_layered_env:&world_env);
        C3D_TexEnvColor(C3D_GetTexEnv(FH_PICA_WORLD_HEIGHT_FOG_STAGE),world_color);
        fh_pica_color_fade_invalidate();
        stage_valid=1;stage_enabled=enabled;stage_color=world_color;stage_uploads++;
    }
    world_draws++;world_layered_draws+=enabled==2;world_partial+=world_indirect!=0;
}
void fh_pica_height_fog_shutdown(void){
    /* Host shutdown has already joined; never reopen a frame after teardown. */
    /* Views borrow interior addresses; only the complete atlas is freed. */
    if(ready)C3D_TexDelete(&atlas);
    ready=valid=intensity=0;memset(images,0,sizeof(images));
    char family[128];snprintf(family,sizeof(family),"PICA height fog layered: draws=%u extra_cpu_bytes=%u extra_gpu_bytes=0\n",world_layered_draws,(unsigned)(sizeof(world_layered_env)+sizeof(world_layered_draws)));fh_platform_log(family);
    world_source=NULL;
    fh_pica_height_fog_invalidate();
    char source_line[144];snprintf(source_line,sizeof(source_line),"PICA height fog source: selector_updates=%u source_pixel_writes=0 source_copy_bytes=0\n",fh_pica_height_fog_source_updates);
    fh_platform_log(source_line);
    char line[240];snprintf(line,sizeof(line),"PICA height fog: updates=%u reuse=%u selections=%u prepares=%u prepare_ticks=%llu resident_peak=%u capacity_bytes=%u update_pixel_bytes=0 update_fences=0\n",
        updates,reuses,selections,prepares,(unsigned long long)prepare_ticks,resident_peak,FH_HEIGHT_FOG_ATLAS_BYTES);
    fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA height fog publication: plane_pairs=%u stage_updates=%u\n",plane_uploads,stage_uploads);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA height fog coverage: snapshots=%u intensity_min=%u intensity_max=%u camera_y_min=%.3f camera_y_max=%.3f final_top=%.3f final_bottom=%.3f\n",
        snapshots,intensity_min,intensity_max,camera_min,camera_max,range_top,range_bottom);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA height fog world: draws=%u unsupported_draws=%u indirect_missing_draws=%u actors=0\n",world_draws,world_skipped,world_partial);fh_platform_log(line);
}
