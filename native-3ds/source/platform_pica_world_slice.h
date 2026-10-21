/* Prepared transparent surface slice. No source bytecode or geometry rebuild.
 * This bypasses main-only command-plan indices, which cannot name this slice. */
extern unsigned fh_pica_world_water_active;
static unsigned world_slice_count;
static u64 world_slice_ticks,world_slice_max_ticks,world_slice_calls;
static u64 world_slice_frame_ticks,world_slice_max_frame_ticks,world_slice_active_frames;
static unsigned world_slice_frame,world_slice_first_frame,world_slice_max_frame;
extern unsigned fh_minimal_frames;
unsigned fh_pica_world_slice_draw_count(void){return world_slice_count;}
void fh_pica_world_slice_metrics(u64 out[8]){
    out[0]=world_slice_ticks;out[1]=world_slice_max_ticks;out[2]=world_slice_calls;
    out[3]=world_slice_active_frames;out[4]=world_slice_max_frame_ticks;
    out[5]=world_slice_max_frame;out[6]=world_slice_first_frame;out[7]=world_slice_frame;
}
static unsigned world_slice_draw(const FhSceneAsset *asset,const FhPicaWorldDraw *g,
    const float view[3][4],const float projection[4][4],const unsigned clip[4]){
    if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_WORLD))return 0;
    const FhPicaWorldPlan *plan=fh_native_scene_pica_plan(asset);
    if(!plan || (fh_pica_scene_textured && !plan->materials.items))return 0;
    unsigned height_requested=fh_pica_scene_actors && fh_pica_height_fog_world_active();
    unsigned height=height_requested?fh_pica_material_height_fog_eligible(&plan->materials,g->material):0;
    if(height)fh_pica_height_fog_world_prepare();
    fh_pica_scene_reserve();
    select_layered(0);select_ui(0);select_cloud(0);select_ambient(0);select_height(height_requested);bind_scene_pipeline();
    if(fh_pica_scene_actors){
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uniform,false);
        fh_pica_environment_bind(0,0);
    }
    /* Source Camera_ApplyTransparentViewport uses near=-.01, far=1. */
    scene_depth(fh_pica_world_water_active?0.f:-.01f,1.f);camera_valid=0;
    fh_pica_world_layout_bind(asset->compact,fh_pica_scene_textured);
    float scale=asset->compact?(g->format==7?1.f:g->format==5?1.f/8:1.f/256):1.f;
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0]*scale,view[r][1]*scale,view[r][2]*scale,view[r][3]);
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    float color=asset->compact?1.f/15:1.f;C3D_FVUnifSet(GPU_VERTEX_SHADER,7,color,color,color,color);
    if(fh_pica_scene_textured){
        int material=fh_pica_material_bind(&plan->materials,g->material,g->format,asset->compact);
        if(material==2)return 0;
        if(!material)fh_platform_panic("PICA deferred material");
    }
    if(fh_pica_world_water_active){
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_CONSTANT_ALPHA,GPU_ONE_MINUS_CONSTANT_ALPHA,GPU_ZERO,GPU_ONE);
        C3D_BlendingColor(0x60ffffffu);
    }
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    C3D_CullFace(g->cull?GPU_CULL_FRONT_CCW:GPU_CULL_NONE);
    fh_pica_world_buffers(asset->compact,fh_pica_scene_textured,g->vertex);
    fh_pica_color_fade_bind(0);if(height_requested)fh_pica_height_fog_world_bind(height);
    C3D_DrawElements(GPU_TRIANGLES,g->indices,C3D_UNSIGNED_SHORT,g->index);
    if(fh_pica_world_water_active){
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        C3D_BlendingColor(0);
    }
    return 1;
}
unsigned fh_pica_world_slice_draw(const FhSceneAsset *asset,const FhPicaWorldDraw *g,
    const float view[3][4],const float projection[4][4],const unsigned clip[4]){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 start=svcGetSystemTick();
    unsigned drawn=world_slice_draw(asset,g,view,projection,clip);
    world_slice_count+=drawn;draws+=drawn;if(drawn)indices+=g->indices;
    /* Retire cached camera/raster assumptions on departure, including hidden
     * materials. Existing shared state owner handles SDK publication. */
    fh_pica_scene_invalidate();
    if(fh_pica_world_water_active)fh_pica_materials_invalidate();
    u64 elapsed=svcGetSystemTick()-start;
    if(!world_slice_calls || world_slice_frame!=fh_minimal_frames){
        if(!world_slice_calls)world_slice_first_frame=fh_minimal_frames;
        world_slice_frame=fh_minimal_frames;world_slice_frame_ticks=0;world_slice_active_frames++;
    }
    world_slice_frame_ticks+=elapsed;
    if(world_slice_frame_ticks>world_slice_max_frame_ticks){
        world_slice_max_frame_ticks=world_slice_frame_ticks;world_slice_max_frame=world_slice_frame;
    }
    ticks+=elapsed;world_slice_ticks+=elapsed;world_slice_calls++;
    if(elapsed>world_slice_max_ticks)world_slice_max_ticks=elapsed;
    return drawn;
}
