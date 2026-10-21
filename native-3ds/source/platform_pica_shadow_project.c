/* Project native GPU-produced silhouettes onto source collision receivers.
 * No source interpreter, collision-plane flattening, or per-frame vertex work. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_shadow_project.h"
#include "fh_pica_shadow_projection.h"
#include "fh_pica_shadow_image.h"
#include "fh_pica_shadow_depth.h"
#include "fh_pica_receiver_plane.h"
#include "fh_pica_fog.h"
#include "fh_graphics_options.h"
#include "fh_scene_storage.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"
#include "fh_pica_image.h"
#include "fh_pica_shadow_points.h"
#include "fh_pica_shadow_ranges.h"
#include "fh_pica_buffers.h"
#include "pica_shadow_points_shader.h"
extern void fh_minimal_auxiliary_begin(void);
extern void fh_minimal_auxiliary_end(unsigned,unsigned);
enum {RECEIVER_INSTANCES=128};
typedef struct {const FhSceneAsset *asset;const FhShadowPointPlan *plan;
    float view[3][4],world[3][4],projection[4][4],origin[2];unsigned clip[4];} Instance;
static Instance instances[RECEIVER_INSTANCES];
static unsigned frame,count,peak,casters,draws,indices,rejected,empty;
static u64 ticks,range_queries,range_untrimmed,range_submitted,range_visits,range_hits;
static C3D_Tex ramp;
static C3D_Tex mask_sampler;
static C3D_Tex *shadow_sampler;
static unsigned source_mask,mask_intensity,mask_objects;
static float mask_scale[2],receiver_light[3];
static unsigned ramp_ready,project_bound,surface_alternate,missing_plans,culled_blocks;
static DVLB_s *track_binary;
static shaderProgram_s track_program;
static C3D_AttrInfo point_attributes;
static const FhPicaBufferLayout point_buffers={1,{{0x43210,(32u<<16)|(5u<<28)}}};
static float point_projection[4][4];static unsigned point_projection_valid;
static float world_origin[2],world_transform[3][4];
static unsigned world_ready,world_publications,world_frame;
static const FhSceneAsset *world_asset;
static int32_t query[6];
static unsigned query_ready,query_culled;
void fh_pica_shadow_project_transform(const FhSceneAsset *asset,const float m[3][4]){
    world_ready=0;
    if(!(fh_graphics_options&FH_GRAPHICS_SILHOUETTE_SHADOWS) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_WORLD|FH_GRAPHICS_NO_GROUND_SHADOWS)))return;
    memcpy(world_transform,m,sizeof(world_transform));
    world_origin[0]=m[0][3];world_origin[1]=m[2][3];
    world_asset=asset;world_frame=fh_renderer_scene_frame();world_ready=1;world_publications++;
}
void fh_pica_shadow_project_query(const int32_t bounds[6]){memcpy(query,bounds,sizeof(query));query_ready=1;}
void fh_pica_shadow_project_surface(unsigned alternate){surface_alternate=!!alternate;}
void fh_pica_shadow_project_invalidate(void){project_bound=0;}
static unsigned tile(unsigned x,unsigned y){
    unsigned n=0;for(unsigned b=0;b<3;b++){n|=((x>>b)&1)<<(2*b);n|=((y>>b)&1)<<(2*b+1);}return n;
}
void fh_pica_shadow_project_init(void){
    extern u32 __ctru_linear_heap_size;
    if(ramp_ready)fh_platform_panic("PICA projected shadow duplicate init");
    if(linearSpaceFree()<2048+__ctru_linear_heap_size*15/100 || !C3D_TexInit(&ramp,256,8,GPU_L8))
        fh_platform_panic("PICA shadow ramp allocation");
    /* Original fillRampTexture has byte x in every row. PICA minimum height8
     * duplicates the original four identical rows; no per-frame pixel work. */
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<256;x++)((u8*)ramp.data)[(x/8)*64+tile(x&7,y)]=x;
    fh_gpu_cache_flush(ramp.data,2048);
    C3D_TexSetFilter(&ramp,GPU_LINEAR,GPU_LINEAR);C3D_TexSetWrap(&ramp,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    track_binary=DVLB_ParseFile((u32*)pica_shadow_points_shader,sizeof(pica_shadow_points_shader));
    if(!track_binary || track_binary->numDVLE!=2 || track_binary->DVLP.codeSize!=133)fh_platform_panic("PICA collision receiver shader");
    shaderProgramInit(&track_program);shaderProgramSetVsh(&track_program,track_binary->DVLE);shaderProgramSetGsh(&track_program,track_binary->DVLE+1,7);
    const char*names[]={"view","projection","shadow_uv","shadow_ramp","fog_distance_scale","receiver_light","query_min","query_max","collision_offset"};
    const int slots[]={0,3,7,9,10,11,12,13,14};
    for(unsigned i=0;i<9;i++)if(shaderInstanceGetUniformLocation(track_program.vertexShader,names[i])!=slots[i])fh_platform_panic("PICA collision receiver uniform ABI");
    AttrInfo_Init(&point_attributes);
    /* The final short is existing record padding. PICA requires the fetch
     * extent to match its 32-byte stride, even though the shader ignores v4.w. */
    for(unsigned i=0;i<5;i++)AttrInfo_AddLoader(&point_attributes,i,GPU_SHORT,i==4?4:3);
    fh_pica_shadow_track_enabled=1;
    fh_pica_shadow_image_init();ramp_ready=1;count=0;frame=~0u;
}
void fh_pica_shadow_project_world(const FhSceneAsset*a,const FhPicaWorldPlan*p,const float v[3][4],const float projection[4][4],const unsigned clip[4]){
    (void)p;
    if(!(fh_graphics_options&FH_GRAPHICS_SILHOUETTE_SHADOWS) || (fh_graphics_options&FH_GRAPHICS_NO_GROUND_SHADOWS))return;
    if(!world_ready || world_asset!=a || world_frame!=fh_renderer_scene_frame())fh_platform_panic("PICA shadow missing source block transform");
    world_ready=0;
    unsigned now=fh_renderer_scene_frame();if(now!=frame){count=0;frame=now;}
    const FhShadowPointPlan*plan=fh_native_scene_shadow_points(a);
    if(!plan){missing_plans++;return;} /* Count explicitly; no visible-mesh substitute. */
    float corrected[3][4],world[3][4];memcpy(corrected,v,sizeof(corrected));
    fh_pica_shadow_receiver_world(world,world_transform,plan->y_offset);
    /* Original collision points already contain this offset; the render block
     * matrix also contains it. Remove exactly that duplicate translation. */
    for(unsigned row=0;row<3;row++)corrected[row][3]-=corrected[row][1]*plan->y_offset;
    for(unsigned i=0;i<count;i++)if(instances[i].asset==a && !memcmp(instances[i].origin,world_origin,sizeof(world_origin)) && !memcmp(instances[i].view,corrected,sizeof(corrected)) && !memcmp(instances[i].world,world,sizeof(world)) &&
        !memcmp(instances[i].projection,projection,sizeof(instances[i].projection)) && !memcmp(instances[i].clip,clip,sizeof(instances[i].clip)))return;
    if(count==RECEIVER_INSTANCES)fh_platform_panic("PICA shadow receiver frame capacity");
    Instance*item=instances+count++;item->asset=a;item->plan=plan;
    memcpy(item->origin,world_origin,sizeof(world_origin));memcpy(item->world,world,sizeof(world));
    memcpy(item->view,corrected,sizeof(corrected));memcpy(item->projection,projection,sizeof(item->projection));memcpy(item->clip,clip,sizeof(item->clip));
    if(count>peak)peak=count;

}
void fh_pica_shadow_project_forget(const FhSceneAsset*a){
    for(unsigned i=0;i<count;)if(instances[i].asset==a)instances[i]=instances[--count];else i++;
}
static void uniform(unsigned n,const float row[4],float scale){C3D_FVUnifSet(GPU_VERTEX_SHADER,n,row[0]*scale,row[1]*scale,row[2]*scale,row[3]);}
static void project_bind(unsigned alpha){
    C3D_BindProgram(&track_program);
    C3D_SetAttrInfo(&point_attributes);C3D_CullFace(GPU_CULL_NONE);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,11,receiver_light[0],receiver_light[1],receiver_light[2],0);
    point_projection_valid=0;
    C3D_TexBind(0,shadow_sampler);C3D_TexBind(1,source_mask?NULL:&ramp);C3D_TexBind(2,NULL);
    C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvBufUpdate(C3D_Both,0);
    C3D_TexEnv *e=C3D_GetTexEnv(0);C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
    unsigned intensity=source_mask?alpha:fh_pica_shadow_source_intensity(alpha);C3D_TexEnvColor(e,intensity*0x010101u);
    if(source_mask)C3D_TexEnvOpRgb(e,mask_intensity?GPU_TEVOP_RGB_SRC_R:GPU_TEVOP_RGB_SRC_ALPHA,GPU_TEVOP_RGB_SRC_COLOR,0);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,0,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);
    if(!source_mask){
        e=C3D_GetTexEnv(1);C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE1,0);
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR,0);C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
    }
    fh_pica_shadow_depth_bind();
    C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ZERO,GPU_ONE_MINUS_SRC_COLOR,GPU_ZERO,GPU_ONE);
    fh_pica_fog_invalidate();fh_pica_fog_bind(1);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,10,fh_pica_fog_distance_scale(),0,0,0);
    /* Source projected shadow colour is attenuated to zero in distant fog.
     * Republish ordinary fog colour on the next material handoff. */
    C3D_TexEnvColor(C3D_GetTexEnv(5),0);fh_pica_fog_invalidate();
    project_bound=1;
}
static void project_receivers(const float texture[3][4],unsigned alpha){
    if(!query_ready)fh_platform_panic("PICA shadow missing finite query");
    query_ready=0;
    fh_minimal_auxiliary_begin();unsigned emitted=0,total=0;project_bound=0;
    for(unsigned i=0;i<count;i++){
        const Instance*item=instances+i;const FhShadowPointPlan*plan=item->plan;
        const FhShadowPointRoute*route=&plan->surfaces.route[surface_alternate];
        if(!route->count)continue;
        float lo[3]={(float)query[0]-item->origin[0],(float)query[1],(float)query[2]-item->origin[1]};
        float hi[3]={(float)query[3]-item->origin[0],(float)query[4],(float)query[5]-item->origin[1]};
        /* Original floor(queryMin/640) excludes the preceding block when a
         * query starts exactly at its far edge. Clamping alone would retain
         * that block's boundary triangle and cast an extra shadow there. */
        if(lo[0]>=640 || lo[2]>=640){query_culled++;continue;}
        lo[0]=fmaxf(lo[0],0);lo[2]=fmaxf(lo[2],0);hi[0]=fminf(hi[0],640);hi[2]=fminf(hi[2],640);
        unsigned outside=0;
        for(unsigned k=0;k<3;k++)outside|=lo[k]>hi[k]||lo[k]>route->query_hi[k]||hi[k]<route->query_lo[k];
        if(outside){query_culled++;continue;}
        const FhShadowRangeNode*tree=(const void*)((const unsigned char*)plan+plan->range_offset[surface_alternate]);
        FhShadowRangeCache*cache=(void*)((unsigned char*)plan+plan->range_cache_offset);
        FhShadowRange span=fh_pica_shadow_range_cached(cache,tree,lo,hi,plan->y_offset);
        range_hits+=!span.visited;
        range_queries++;range_untrimmed+=route->count;range_submitted+=span.count;range_visits+=span.visited;
        if(!span.count){query_culled++;continue;}
        float local[3][4];FhWorldPlane planes[4];FhWorldBounds bounds;
        fh_pica_shadow_concat(local,texture,item->world);fh_pica_shadow_receiver_planes(planes,(const float(*)[4])local);
        for(unsigned k=0;k<3;k++){bounds.lo[k]=route->visible_lo[k];bounds.hi[k]=route->visible_hi[k];}
        if(fh_world_prepared_outside(&bounds,planes)){culled_blocks++;continue;}
        if(source_mask){
            for(unsigned k=0;k<4;k++){local[0][k]*=mask_scale[0];local[1][k]*=-mask_scale[1];}
            local[1][3]+=1;
        }
        fh_pica_scene_reserve();if(!project_bound)project_bind(alpha);
        for(unsigned row=0;row<3;row++)uniform(row,item->view[row],1);
        if(!point_projection_valid || memcmp(point_projection,item->projection,sizeof(point_projection))){
            for(unsigned row=0;row<4;row++)uniform(3+row,item->projection[row],1);
            memcpy(point_projection,item->projection,sizeof(point_projection));point_projection_valid=1;
        }
        for(unsigned row=0;row<3;row++)uniform(7+row,local[row],1);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,12,lo[0],lo[1],lo[2],0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,13,hi[0],hi[1],hi[2],0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,14,plan->y_offset,0,0,0);
        C3D_SetScissor(GPU_SCISSOR_NORMAL,item->clip[0],item->clip[1],item->clip[2],item->clip[3]);
        const void*streams[3]={plan->gpu,NULL,NULL};
        if(!fh_pica_buffers_bind(&point_buffers,streams))fh_platform_panic("PICA prepared point storage");
        const uint16_t*index=(const void*)((const unsigned char*)plan->gpu+plan->indices_offset);
        if(route->indexed)C3D_DrawElements(GPU_GEOMETRY_PRIM,span.count,C3D_UNSIGNED_SHORT,index+span.first);
        else C3D_DrawArrays(GPU_GEOMETRY_PRIM,span.first,span.count);
        emitted++;total+=span.count;fh_native_scene_used(item->asset);
    }
    fh_minimal_auxiliary_end(emitted,total*3);draws+=emitted;indices+=total;

}
static int visible(unsigned alpha){
    if(!alpha || !(fh_graphics_options&FH_GRAPHICS_SILHOUETTE_SHADOWS) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_WORLD|FH_GRAPHICS_NO_ACTORS|FH_GRAPHICS_NO_GROUND_SHADOWS)))return 0;
    if(frame!=fh_renderer_scene_frame() || !count){empty++;return 0;}
    if(!ramp_ready)fh_platform_panic("PICA projected shadow lifetime");
    return 1;
}
void fh_pica_shadow_project(const FhSceneAsset*a,const float*poses,unsigned joints,const float world[3][4],const float light[3],float extent,float fade,unsigned alpha){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    if(!visible(alpha))return;
    if(!poses)fh_platform_panic("PICA projected shadow pose");
    u64 start=svcGetSystemTick();float center[3]={joints?poses[3]:world[0][3],joints?poses[7]:world[1][3],joints?poses[11]:world[2][3]};FhPicaShadowProjection p;
    if(!fh_pica_shadow_projection(&p,center,light,extent,fade)){rejected++;return;}
    fh_pica_shadow_image_begin();fh_pica_receiver_clear_image();unsigned caster_indices=0;
    unsigned caster_draws=fh_pica_scene_shadow_capture(a,poses,joints,world,p.capture,p.projection,&caster_indices);
    fh_native_scene_used(a);fh_pica_shadow_image_end(caster_draws+1,caster_indices+6);
    source_mask=0;shadow_sampler=(C3D_Tex*)fh_pica_shadow_image_sampler();
    memcpy(receiver_light,light,sizeof(receiver_light));
    unsigned before_draws=draws;project_receivers(p.texture,alpha);
    fh_pica_shadow_image_describe(a->id,frame,alpha,draws-before_draws);
    casters++;ticks+=svcGetSystemTick()-start;
}
void fh_pica_shadow_project_mask(unsigned page,const float center[3],float extent,unsigned alpha){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    if(!visible(alpha))return;
    if(!isfinite(extent) || extent<=0){rejected++;return;}
    u64 start=svcGetSystemTick();float scale=.5f/extent;
    float texture[3][4]={{scale,0,0,.5f-center[0]*scale},{0,0,scale,.5f-center[2]*scale},{0,0,0,0}};
    mask_sampler=*fh_pica_hud_sampler(page,mask_scale,&mask_intensity);
    C3D_TexSetWrap(&mask_sampler,GPU_CLAMP_TO_BORDER,GPU_CLAMP_TO_BORDER);mask_sampler.border=0;
    source_mask=1;shadow_sampler=&mask_sampler;
    receiver_light[0]=0;receiver_light[1]=1;receiver_light[2]=0;
    project_receivers(texture,alpha);mask_objects++;ticks+=svcGetSystemTick()-start;
}
void fh_pica_shadow_project_shutdown(void){
    if(!ramp_ready)return;
    fh_pica_shadow_image_shutdown();C3D_TexDelete(&ramp);ramp_ready=0;count=0;
    shaderProgramFree(&track_program);DVLB_Free(track_binary);track_binary=NULL;
    char line[256];snprintf(line,sizeof(line),"PICA shadow transforms: source_publications=%u camera_inversions=0 extra_instance_bytes=6144\n",world_publications);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA projected shadows: casters=%u draws=%u indices=%u rejected=%u empty=%u peak_instances=%u frame_bytes=%u submit_ticks=%llu\n",
        casters,draws,indices,rejected,empty,peak,(unsigned)sizeof(instances),(unsigned long long)ticks);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA collision projection: missing_plans=%u culled_blocks=%u query_culled=%u shader_instructions=133 point_records=1 CPU_normals=0\n",missing_plans,culled_blocks,query_culled);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA shadow point ranges: queries=%llu untrimmed=%llu submitted=%llu node_visits=%llu cache_hits=%llu CPU_vertices=0\n",
        (unsigned long long)range_queries,(unsigned long long)range_untrimmed,(unsigned long long)range_submitted,(unsigned long long)range_visits,(unsigned long long)range_hits);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA authored projected masks: objects=%u CPU_pixels=0\n",mask_objects);fh_platform_log(line);
}
