/* Persistent native water quad and ordered two-projective-sample owner.
 * Native wake consumer: indirect distortion/filtered bump quantization are
 * explicit omissions, not an alternate game renderer or a parity claim. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include "fh_pica_water_grid.h"
#include "fh_pica_water_bounds.h"
#include "fh_pica_ripple_reduce.h"
#include <string.h>
#include "fh_pica_water_sheet.h"
#include "fh_graphics_options.h"
#include "fh_platform.h"
#include "fh_pica_water_material.h"
#include "fh_pica_image.h"
#include "fh_pica_hud.h"
#include "fh_pica_scene.h"
#include "fh_pica_fog.h"
#include "fh_pica_layered.h"
#include "fh_pica_viewport.h"
#include "fh_minimal_graphics.h"
#include "fh_gpu_cache.h"
#include "fh_replay_timing.h"
typedef struct {int16_t position[4],uv[4];} SheetVertex;
_Static_assert(sizeof(SheetVertex)==16,"Water sheet full fetch stride");

static shaderProgram_s program;
typedef struct {
    void *geometry;
    unsigned image,grid_index_offset,grid_index_count,reserved_bytes;
    unsigned radial_vertex_offset,radial_index_offset,radial_index_count;
    float grid_origin[3],grid_extent[3],mask_scale[2];
    FhWaterBounds bounds;
    C3D_Tex mask_sampler;
    C3D_TexEnv material[2][2][6];
    unsigned pass_delta[2];
} WaterSheet;
static WaterSheet sheets[2];
static unsigned ripple_loads,ripple_objects,ripple_draws;
static unsigned detail_draws[2],detail_indices[2],radial_loaded,radial_declined;
static size_t ripple_scratch_peak;
static unsigned bounds_checks[2],bounds_rejected[2];
static u64 bounds_ticks;
static unsigned batch_calls,full_publications,retained_publications;
static unsigned ready,loads,objects,draws,peak;
static C3D_Tex projected_sampler;


static u64 submit_ticks;

void fh_pica_water_sheet_init(void *entry){
    if(ready)fh_platform_panic("Water sheet duplicate init");
    shaderProgramInit(&program);shaderProgramSetVsh(&program,(DVLE_s*)entry);
    const char *names[]={"model_view","projection","reflected","tint","mask_scale","quad_origin","quad_extent","indirect_scale","fog_distance_scale"};
    const int slots[]={0,3,7,10,11,12,13,14,94};
    for(unsigned i=0;i<9;i++)if(shaderInstanceGetUniformLocation(program.vertexShader,names[i])!=slots[i])
        fh_platform_panic("Water sheet shader ABI");
    if(shaderInstanceGetUniformLocation(program.vertexShader,"reflected_second")!=15 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"second_pass")!=0x68)
        fh_platform_panic("Water batch uniform layout");
    ready=1;
}
static void release_retired(WaterSheet *s){
    fh_pica_image_release(s->image);s->image=0;
    linearFree(s->geometry);s->geometry=NULL;s->reserved_bytes=0;
    s->radial_vertex_offset=s->radial_index_offset=s->radial_index_count=0;
}
void fh_pica_water_sheet_release(void){
    if(!sheets[0].geometry && !sheets[0].image && !sheets[1].geometry && !sheets[1].image)return;
    /* Same pinned Melee ownership discipline as native splash: queued GPU
     * readers retire before shared image/geometry memory can be recycled. */
    fh_minimal_sync();fh_pica_scene_invalidate();release_retired(sheets);release_retired(sheets+1);
}
static int load_sheet(WaterSheet *s,unsigned ripple,const int16_t pos[4][3],const int16_t uv[4][2],
                            const uint16_t index[6],const FhTexture *mask){
    if(!ready || !pos || !uv || !index || !mask)fh_platform_panic("Water sheet load contract");
    for(unsigned i=0;i<6;i++)if(index[i]>=4)fh_platform_panic("Water sheet index");
    if(s->geometry || s->image){fh_minimal_sync();fh_pica_scene_invalidate();release_retired(s);}
    /* Only the source wake/ripple layouts are supported. Never reinterpret arbitrary
     * geometry or a converted/native texture as a tiled source distortion map. */
    if(mask->format!=5 || mask->width!=(ripple?64:32) || mask->height!=64 ||
       mask->wrap_s || mask->wrap_t || !mask->pixels)return 0;
    int16_t endpoint=ripple?127:128;
    const int16_t expected_uv[4][2]={{0,0},{0,endpoint},{endpoint,endpoint},{endpoint,0}};
    if(memcmp(uv,expected_uv,sizeof expected_uv))return 0;
    if(pos[1][0]!=pos[0][0] || pos[3][1]!=pos[0][1] || pos[3][2]!=pos[0][2])return 0;
    for(unsigned c=0;c<3;c++)if((int)pos[2][c]!=(int)pos[1][c]+pos[3][c]-pos[0][c])return 0;
    FhWaterGridSize capacity,used;
    if(!fh_water_grid_size(ripple?64:32,64,1,&capacity))return 0;
    unsigned char *prepared=malloc(capacity.vertex_bytes+capacity.index_bytes);
    if(!prepared)return 0;
    int compiled=ripple?
        fh_water_grid_compile_ripple_visible(mask->pixels,8192,1,
            (FhWaterGridVertex*)prepared,capacity.vertex_bytes,
            (uint16_t*)(prepared+capacity.vertex_bytes),capacity.index_bytes,&used):
        fh_water_grid_compile_visible(mask->pixels,4096,32,64,1,
            (FhWaterGridVertex*)prepared,capacity.vertex_bytes,
            (uint16_t*)(prepared+capacity.vertex_bytes),capacity.index_bytes,&used);
    if(!compiled){free(prepared);return 0;}
    if(ripple){
        size_t temporary_peak=0;
        if(!fh_pica_ripple_reduce((FhWaterGridVertex*)prepared,
             (uint16_t*)(prepared+capacity.vertex_bytes),&used,&temporary_peak)){
            free(prepared);return 0;
        }
        temporary_peak+=capacity.vertex_bytes+capacity.index_bytes;
        if(temporary_peak>ripple_scratch_peak)ripple_scratch_peak=temporary_peak;
    }
    if(!used.indices){free(prepared);return 0;}
    unsigned original_allocation=(used.vertex_bytes+used.index_bytes+127u)&~127u;
    unsigned allocation=original_allocation,radial_allocation=0;
    FhWaterGridSize radial_capacity={0},radial_used={0};unsigned char *radial=NULL;
    extern u32 __ctru_linear_heap_size;
    if(linearSpaceFree()<allocation+__ctru_linear_heap_size*15/100){free(prepared);return 0;}
    if(ripple && fh_water_grid_ripple_radial_size(32,16,&radial_capacity)){
        radial=malloc(radial_capacity.vertex_bytes+radial_capacity.index_bytes);
        if(radial){
            size_t temporary=capacity.vertex_bytes+capacity.index_bytes+radial_capacity.vertex_bytes+radial_capacity.index_bytes;
            if(temporary>ripple_scratch_peak)ripple_scratch_peak=temporary;
            if(!fh_water_grid_compile_ripple_radial(mask->pixels,8192,32,16,
                (FhWaterGridVertex*)radial,radial_capacity.vertex_bytes,
                (uint16_t*)(radial+radial_capacity.vertex_bytes),radial_capacity.index_bytes,&radial_used)){
                free(radial);radial=NULL;
            }
        }
        if(radial){
            radial_allocation=(radial_used.vertex_bytes+radial_used.index_bytes+127u)&~127u;
            if(linearSpaceFree()<allocation+radial_allocation+__ctru_linear_heap_size*15/100){
                free(radial);radial=NULL;radial_allocation=0;
            }
        }
    }
    allocation+=radial_allocation;
    s->geometry=linearMemAlign(allocation,128);
    /* Losing optional lower-detail residency must not lose the normal plan.
     * Neither attempt has a GPU reader yet; no fence or per-frame retry. */
    if(!s->geometry && radial){
        free(radial);radial=NULL;radial_allocation=0;allocation=original_allocation;
        s->geometry=linearMemAlign(allocation,128);
    }
    if(!s->geometry){free(prepared);return 0;}
    memset(s->geometry,0,allocation);
    memcpy(s->geometry,prepared,used.vertex_bytes);
    memcpy((char*)s->geometry+used.vertex_bytes,prepared+capacity.vertex_bytes,used.index_bytes);
    free(prepared);
    s->radial_vertex_offset=s->radial_index_offset=s->radial_index_count=0;
    if(radial){
        s->radial_vertex_offset=original_allocation;
        s->radial_index_offset=original_allocation+radial_used.vertex_bytes;
        s->radial_index_count=radial_used.indices;
        memcpy((char*)s->geometry+s->radial_vertex_offset,radial,radial_used.vertex_bytes);
        memcpy((char*)s->geometry+s->radial_index_offset,radial+radial_capacity.vertex_bytes,radial_used.index_bytes);
        free(radial);radial_loaded++;
    }else if(ripple)radial_declined++;
    s->grid_index_offset=used.vertex_bytes;s->grid_index_count=used.indices;
    for(unsigned c=0;c<3;c++){
        s->grid_origin[c]=pos[0][c];
        s->grid_extent[c]=(c?pos[1][c]:pos[3][c])-pos[0][c];
    }
    fh_water_bounds_prepare(&s->bounds,s->grid_origin,s->grid_extent);
    fh_gpu_cache_flush(s->geometry,allocation);
    s->image=fh_pica_image_acquire(mask);
    /* New s->geometry has never been submitted; it needs no additional GPU wait
     * if the optional s->image cannot be admitted. Older readers retired above. */
    if(!s->image){linearFree(s->geometry);s->geometry=NULL;return 0;}
    fh_pica_image_sampler(s->image,mask,&s->mask_sampler,s->mask_scale);
    if(ripple){s->mask_scale[0]*=127.f/128;s->mask_scale[1]*=127.f/128;}
    for(unsigned heavy=0;heavy<2;heavy++){
        C3D_TexEnv stages[2][2];fh_pica_water_split_compile(stages,heavy,0);
        for(unsigned pass=0;pass<2;pass++){
            for(unsigned stage=0;stage<6;stage++)C3D_TexEnvInit(&s->material[heavy][pass][stage]);
            s->material[heavy][pass][0]=stages[pass][0];s->material[heavy][pass][1]=stages[pass][1];
            C3D_TexEnv *e=&s->material[heavy][pass][0];
            C3D_TexEnvSrc(e,C3D_Alpha,GPU_PRIMARY_COLOR,GPU_TEXTURE1,0);
            C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
            C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,
                ripple?GPU_TEVOP_A_SRC_G:(mask->format<=1?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA),0);
        }
    }
    for(unsigned heavy=0;heavy<2;heavy++){
        s->pass_delta[heavy]=0;
        for(unsigned stage=0;stage<5;stage++)
            if(memcmp(&s->material[heavy][0][stage],&s->material[heavy][1][stage],sizeof(C3D_TexEnv)))
                s->pass_delta[heavy]|=1u<<stage;
        /* Dynamic mix constants differ between stages0/1 on every pass. */
        s->pass_delta[heavy]|=3;
    }
    s->reserved_bytes=allocation;loads++;ripple_loads+=!!ripple;
    unsigned resident=sheets[0].reserved_bytes+sheets[1].reserved_bytes;
    if(resident>peak)peak=resident;
    return 1;
}
static unsigned draw_batch(WaterSheet *s,unsigned ripple,const void *capture,
    const FhWaterSheetInstance *instances,unsigned count,const float projection[4][4],
    const float reflected[2][3][4],const unsigned scissor[4],const uint8_t tint[4],
    uint32_t mix,unsigned heavy){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    if(!count)return 0;
    if(count>30 || !ready || !s->geometry || !s->image || !capture)
        fh_platform_panic("Water batch lifetime/count");
    u64 started=svcGetSystemTick();
    fh_pica_hud_flush();unsigned clip[4];if(!fh_pica_hud_target(scissor,clip))return 0;
    projected_sampler=*(const C3D_Tex*)capture;
    projected_sampler.param=(projected_sampler.param&~(7u<<28))|GPU_TEXTURE_MODE(GPU_TEX_PROJECTION);
    float viewport[6],native_projection[4][4],depth[2];fh_pica_live_viewport(viewport);
    fh_pica_viewport_xy(native_projection,projection,viewport);
    fh_pica_viewport_depth(depth,viewport[4],viewport[5]);
    unsigned state_ready=0,rendered=0;uint32_t fog_color=0;
    batch_calls++;
    unsigned radial=ripple && (fh_graphics_effects&FH_EFFECT_RIPPLE_FAST);
    if(radial && !s->radial_index_count){fh_graphics_effects_reject(FH_EFFECT_RIPPLE_FAST);radial=0;}
    void *vertices=(char*)s->geometry+(radial?s->radial_vertex_offset:0);
    const void *indices=(char*)s->geometry+(radial?s->radial_index_offset:s->grid_index_offset);
    unsigned index_count=radial?s->radial_index_count:s->grid_index_count;
    /* All source camera/pose work has finished before entering this scope.
     * No callbacks or other GPU owners may run between these ordered draws.
     * reserve_changed is the only possible state handoff inside the batch. */
    for(unsigned item=0;item<count;item++){
        const float (*view)[4]=instances[item].view;
        u64 bounds_started=svcGetSystemTick();bounds_checks[ripple]++;
        unsigned outside=fh_water_bounds_outside(&s->bounds,view,native_projection);
        bounds_ticks+=svcGetSystemTick()-bounds_started;
        if(outside){bounds_rejected[ripple]++;continue;}
        for(unsigned pass=0;pass<2;pass++){
            unsigned reset=fh_pica_scene_reserve_changed();
            unsigned full=!state_ready || reset;
            if(full){
                full_publications++;fh_pica_scene_invalidate();
                C3D_BindProgram(&program);C3D_LightEnvBind(NULL);
                C3D_TexBind(0,&projected_sampler);C3D_TexBind(1,&s->mask_sampler);C3D_TexBind(2,NULL);
                C3D_TexEnvBufUpdate(C3D_Both,0);
                for(unsigned stage=0;stage<6;stage++)C3D_SetTexEnv(stage,&s->material[!!heavy][pass][stage]);
                fh_pica_fog_invalidate();C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
                float fog_parameters[2];unsigned fog=fh_pica_fog_layered_bind(1,fog_parameters);
                if(!fh_pica_environment_layered_bind(0,0,fog,fog_parameters))fh_platform_panic("Water sheet fog admission");
                fog_color=C3D_GetTexEnv(5)->color;
                C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);C3D_CullFace(GPU_CULL_NONE);
                C3D_DepthMap(true,depth[0],depth[1]);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
                C3D_StencilTest(false,GPU_ALWAYS,0,255,255);C3D_AlphaTest(false,GPU_ALWAYS,0);
                for(unsigned i=0;i<4;i++){
                    unsigned row=i==0?1:i==1?0:i;float sign=i==1?-1.f:1.f;
                    C3D_FVUnifSet(GPU_VERTEX_SHADER,3+i,sign*native_projection[row][0],sign*native_projection[row][1],sign*native_projection[row][2],sign*native_projection[row][3]);
                }
                for(unsigned channel=0;channel<2;channel++)for(unsigned row=0;row<3;row++){
                    unsigned slot=(channel?15:7)+row;
                    C3D_FVUnifSet(GPU_VERTEX_SHADER,slot,reflected[channel][row][0],reflected[channel][row][1],reflected[channel][row][2],reflected[channel][row][3]);
                }
                float first=.25f*(ripple?.5f:1.f),second=.125f*(ripple?.5f:1.f);
                C3D_FVUnifSet(GPU_VERTEX_SHADER,14,first/320.f,first/240.f,second/320.f,second/240.f);
                C3D_FVUnifSet(GPU_VERTEX_SHADER,11,s->mask_scale[0],s->mask_scale[1],0,0);
                C3D_FVUnifSet(GPU_VERTEX_SHADER,12,s->grid_origin[0],s->grid_origin[1],s->grid_origin[2],0);
                C3D_FVUnifSet(GPU_VERTEX_SHADER,13,s->grid_extent[0],s->grid_extent[1],s->grid_extent[2],0);
                C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddLoader(a,0,GPU_SHORT,4);
                C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
                if(BufInfo_Add(b,vertices,sizeof(FhWaterGridVertex),1,0)<0)fh_platform_panic("Water sheet fetch layout");
                state_ready=1;
            }else{
                retained_publications++;
                /* This load-compiled mask excludes stage5, owned by fog. */
                for(unsigned stage=0;stage<5;stage++)if(s->pass_delta[!!heavy]&(1u<<stage))
                    C3D_SetTexEnv(stage,&s->material[!!heavy][pass][stage]);
            }
            C3D_TexEnvColor(C3D_GetTexEnv(pass?0:1),pass?mix:mix^0x00ffffffu);
            C3D_TexEnvColor(C3D_GetTexEnv(5),pass?0:fog_color);
            C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,pass?GPU_ONE:GPU_ONE_MINUS_SRC_ALPHA,GPU_ZERO,GPU_ONE);
            if(!pass || full){
                for(unsigned row=0;row<3;row++)
                    C3D_FVUnifSet(GPU_VERTEX_SHADER,row,view[row][0],view[row][1],view[row][2],view[row][3]);
                C3D_FVUnifSet(GPU_VERTEX_SHADER,10,tint[0]/255.f,tint[1]/255.f,tint[2]/255.f,instances[item].alpha/255.f);
            }
            /* Publish every pass, including after a split/program handoff. */
            C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,pass);
            C3D_DrawElements(GPU_TRIANGLES,index_count,C3D_UNSIGNED_SHORT,indices);
            fh_pica_hud_account(index_count);
            if(ripple){detail_draws[radial]++;detail_indices[radial]+=index_count;}
        }
        rendered++;
    }
    objects+=rendered;draws+=rendered*2;
    if(ripple){ripple_objects+=rendered;ripple_draws+=rendered*2;}
    submit_ticks+=svcGetSystemTick()-started;
    fh_pica_scene_invalidate();fh_pica_fog_invalidate();return rendered*2;
}
static unsigned draw_sheet(WaterSheet *s,unsigned ripple,const void *capture,const float view[3][4],
    const float projection[4][4],const float reflected[2][3][4],
    const unsigned scissor[4],const uint8_t tint[4],uint32_t mix,unsigned heavy){
    FhWaterSheetInstance instance;memcpy(instance.view,view,sizeof instance.view);instance.alpha=tint[3];
    return draw_batch(s,ripple,capture,&instance,1,projection,reflected,scissor,tint,mix,heavy);
}
unsigned fh_pica_water_sheet_batch(unsigned ripple,const void *capture,const FhWaterSheetInstance *instances,unsigned count,
    const float projection[4][4],const float reflected[2][3][4],const unsigned scissor[4],const uint8_t tint[4],uint32_t mix,unsigned heavy){
    if(ripple>1)fh_platform_panic("Water batch plan");
    return draw_batch(sheets+ripple,ripple,capture,instances,count,projection,reflected,scissor,tint,mix,heavy);
}

int fh_pica_water_sheet_load(const int16_t pos[4][3],const int16_t uv[4][2],
    const uint16_t index[6],const FhTexture *mask){return load_sheet(sheets+0,0,pos,uv,index,mask);}
unsigned fh_pica_water_sheet_draw(const void *capture,const float view[3][4],const float projection[4][4],
    const float reflected[2][3][4],const unsigned scissor[4],const uint8_t tint[4],uint32_t mix,unsigned heavy){
    return draw_sheet(sheets+0,0,capture,view,projection,reflected,scissor,tint,mix,heavy);
}
int fh_pica_ripple_load(const int16_t pos[4][3],const int16_t uv[4][2],
    const uint16_t index[6],const FhTexture *mask){return load_sheet(sheets+1,1,pos,uv,index,mask);}
unsigned fh_pica_ripple_draw(const void *capture,const float view[3][4],const float projection[4][4],
    const float reflected[2][3][4],const unsigned scissor[4],const uint8_t tint[4],uint32_t mix,unsigned heavy){
    return draw_sheet(sheets+1,1,capture,view,projection,reflected,scissor,tint,mix,heavy);
}
void fh_pica_water_sheet_shutdown(void){
    release_retired(sheets);release_retired(sheets+1);
    char line[240];snprintf(line,sizeof(line),"PICA water sheet: loads=%u objects=%u draws=%u linear_peak=%u submit_ticks=%llu CPU_vertices=0 indirect=1 quantized_bump_alpha=0\n",loads,objects,draws,peak,(unsigned long long)submit_ticks);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA ripples: loads=%u objects=%u draws=%u temporary_reserved_peak=%u CPU_vertices=0\n",
        ripple_loads,ripple_objects,ripple_draws,(unsigned)ripple_scratch_peak);fh_platform_log(line);
    snprintf(line,sizeof line,"PICA water bounds: wake_checks=%u wake_rejected=%u ripple_checks=%u ripple_rejected=%u ticks=%llu nested_visible_owner=1\n",
        bounds_checks[0],bounds_rejected[0],bounds_checks[1],bounds_rejected[1],(unsigned long long)bounds_ticks);fh_platform_log(line);
    snprintf(line,sizeof line,"PICA water batch: calls=%u full=%u retained=%u CPU_vertices=0\n",
        batch_calls,full_publications,retained_publications);fh_platform_log(line);
    snprintf(line,sizeof line,"PICA ripple detail: normal_draws=%u normal_indices=%u fast_draws=%u fast_indices=%u fast_loaded=%u fast_declined=%u CPU_vertices=0\n",
        detail_draws[0],detail_indices[0],detail_draws[1],detail_indices[1],radial_loaded,radial_declined);fh_platform_log(line);
    if(ready)shaderProgramFree(&program);
    ready=0;
}
