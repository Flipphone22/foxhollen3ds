/* Included by the native scene owner. Existing pipeline invalidation, uniforms,
 * material cache and command-capacity handling remain authoritative. */
void fh_pica_scene_actor_fog_init(void){
    if(actor_fog_binary)return;
    actor_fog_binary=DVLB_ParseFile((u32*)pica_actor_height_fog_shader,sizeof(pica_actor_height_fog_shader));
    if(!actor_fog_binary || actor_fog_binary->numDVLE!=1 || actor_fog_binary->DVLP.codeSize>512)
        fh_platform_panic("PICA actor fog shader");
    shaderProgramInit(&actor_fog_program);
    if(shaderProgramSetVsh(&actor_fog_program,actor_fog_binary->DVLE)<0 ||
       !fh_pica_actor_fog_layout_prepare(&actor_fog_layout,1))fh_platform_panic("PICA actor fog program");
    const char*names[]={"view","projection","tint","uvscale","bones","decode","ambient","light_position","light_color","fog_distance_scale"};
    const int slots[]={0,3,7,8,10,76,77,78,86,94};
    for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++)
        if(shaderInstanceGetUniformLocation(actor_fog_program.vertexShader,names[i])!=slots[i])
            fh_platform_panic("PICA actor fog float ABI");
    if(shaderInstanceGetUniformLocation(actor_fog_program.vertexShader,"actor_fur")!=0x76 ||
       shaderInstanceGetUniformLocation(actor_fog_program.vertexShader,"actor_specular")!=0x77)
        fh_platform_panic("PICA actor fog bool ABI");
}
unsigned fh_pica_scene_actor_fog_draw(const FhSceneAsset*asset,const FhPicaModelPlan*plan,
    const FhActorFogAsset*fog,const FhPicaActorFogLive*live,const float*poses,unsigned joints,
    const float world[3][4],const float view[3][4],const float projection[4][4],
    const unsigned clip[4],float near_z,float far_z,unsigned*out_indices){
    if(!out_indices)return 0;
    *out_indices=0;
    if(!actor_fog_binary || !asset || !plan || !fog || !live || !poses || !world || !view ||
       !projection || !clip || !live->poses || !live->world || !live->view ||
       !live->adjustment || !live->planes || !live->pose_count || live->pose_count>256 ||
       live->cached_normal>1 || !live->image || live->intensity>64 || !fog->groups || asset->block || !asset->compact ||
       fog->id!=asset->id || memcmp(asset->meta+16,fog->meta+20,8) ||
       shadow_active || fur_active || fur_gpu_active || !fh_pica_scene_actors ||
       !(fh_graphics_options&FH_GRAPHICS_HEIGHT_FOG) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS)) ||
       fh_pica_morph_view(asset,poses))return 0;
    /* No compilation/geometry rebuild here. All materials must have their
     * compiled variant ready, otherwise no part of this actor is submitted. */
    for(unsigned i=0;i<fog->groups;i++){
        FhActorFogDraw d;
        if(!fh_native_actor_fog_draw(fog,i,&d) ||
           !fh_pica_material_actor_height_ready(&plan->materials,d.material))return 0;
        for(unsigned b=0;b<d.bones;b++)if(d.bone[b]>joints || d.bone[b]>=asset->bones)return 0;
    }
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 start=svcGetSystemTick();unsigned total=0;
    /* Admit the shared atlas only AFTER every group is drawable. Unsupported
     * actors must not allocate/select an unused image before ordinary fallback. */
    fh_pica_height_fog_update(live->image,live->intensity);
    if(!fh_pica_height_fog_sampler())fh_platform_panic("PICA actor fog sampler publication");
    select_height(0);select_layered(0);select_ui(0);select_cloud(0);select_ambient(0);
    /* bound records the installed program, independently of this call's mode.
     * Retain only a real same-program owner. External targets/GS/queue changes
     * still invalidate it through the shared scene owner. */
    actor_fog_pipeline_reuses+=bound==9;
    actor_fog_active=1;
    unsigned lighting_on=!(fh_graphics_options&FH_GRAPHICS_NO_LIGHTING);
    actor_publish_lighting(lighting_on);
    for(unsigned i=0;i<fog->groups;i++){
        FhActorFogDraw d;fh_native_actor_fog_draw(fog,i,&d);
        fh_pica_scene_reserve();unsigned publish=!i || !bound;bind_scene_pipeline();
        if(publish){
            scene_depth(near_z,far_z);C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uniform,true);
            scene_camera(view,projection);
            C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
            C3D_SetAttrInfo(&actor_fog_layout);
            for(unsigned r=0;r<2;r++)C3D_FixedAttribSet(8+r,live->planes[r][0],live->planes[r][1],live->planes[r][2],live->planes[r][3]);
            actor_state_publications++;
        }
        float ps=d.format==7?1.f:d.format==5?1.f/8:1.f/256;
        C3D_FVUnifSet(GPU_VERTEX_SHADER,76,ps,1.f/15,1.f/128,lighting.normal_scale/64);
        for(unsigned b=0;b<d.bones+d.fog_matrices;b++){
            float rows[3][4];
            if(b<d.bones){
                unsigned bone=d.bone[b];const float*matrix=bone<joints?poses+bone*16:&world[0][0];
                const float*tail=(const float*)(asset->meta+asset->tails_at)+bone*3;
                fh_pica_model_pose(rows,matrix,tail);
            }else{
                const FhPicaActorFogPosePlan*p=fh_native_actor_fog_pose_plan(fog,d.pose_plan[b-d.bones]);
                unsigned world_pose=p->kind==FH_ACTOR_FOG_WORLD_POSITION || p->kind==FH_ACTOR_FOG_WORLD_NORMAL;
                int ok=world_pose?fh_pica_actor_fog_plan_world_pose(rows,p,live->world,live->view,live->adjustment):
                    fh_pica_actor_fog_plan_pose(rows,p,live->poses,live->pose_count,live->view,live->adjustment,live->cached_normal);
                if(!ok)fh_platform_panic("PICA actor fog live source pose");
            }
            for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,10+b*3+r,rows[r][0],rows[r][1],rows[r][2],rows[r][3]);
            actor_bones_written++;
        }
        C3D_BufInfo*buffers=C3D_GetBufInfo();BufInfo_Init(buffers);
        if(BufInfo_Add(buffers,fog->gpu+d.vertex_offset,40,6,0x543210)<0 ||
           BufInfo_Add(buffers,fog->gpu+d.fog_offset,16,2,0x76)<0)fh_platform_panic("PICA actor fog buffers");
        if(fh_pica_material_actor_height_bind(&plan->materials,d.material,d.format,1,live->color)!=1)
            fh_platform_panic("PICA actor fog material publication");
        C3D_CullFace(d.flags&8?GPU_CULL_FRONT_CCW:GPU_CULL_NONE);
        fh_pica_color_fade_bind(1);
        C3D_DrawElements(GPU_TRIANGLES,d.indices,C3D_UNSIGNED_SHORT,fog->gpu+d.index_offset);
        total+=d.indices;actor_bones+=d.bones+d.fog_matrices;
    }
    /* End the requested mode, not the installed pipeline's lifetime. Another
     * fog actor retains that owner; ordinary consumers select a different ID
     * and fully bind it. Every actor still publishes live lighting, palette,
     * material, scissor and attributes on its first draw. */
    actor_fog_active=0;
    actors++;actor_draws+=fog->groups;actor_indices+=total;actor_ticks+=svcGetSystemTick()-start;
    actor_fog_draws+=fog->groups;*out_indices=total;return fog->groups;
}
