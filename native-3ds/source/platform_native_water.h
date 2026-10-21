/* Native low-detail deferred map water. Original visibility, geometry, vertex
 * alpha, tint, caustic image and animation clock are retained. A single small
 * screen reflection plus moving caustic brightness replaces GX's two indirect
 * distortion samples. This is not the separate engine19 dynamic water mesh. */
FhMinimalWaterMaterial fh_minimal_water;
static unsigned water_draws,water_failures;
static u64 water_ticks;
static void native_water_env(void){
    const unsigned char *k=fh_minimal_water.tint;
    unsigned rgba=k[0]|k[1]<<8|k[2]<<16|(unsigned)k[3]<<24;
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvBufUpdate(C3D_Both,0);
    C3D_TexEnv *e=C3D_GetTexEnv(0);C3D_TexEnvColor(e,rgba);
    if(fh_minimal_water.heavy_fog){
        /* GX divides the sum before clamping: average here, never clamp an
         * ADD and halve afterwards (that darkens bright fog reflections). */
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_CONSTANT);
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    }else{
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);C3D_TexEnvFunc(e,C3D_RGB,GPU_ADD);
    }
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);

    e=C3D_GetTexEnv(2);C3D_TexEnvColor(e,rgba);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE0,GPU_CONSTANT);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    C3D_TexEnvBufUpdate(C3D_RGB,1<<2);
    e=C3D_GetTexEnv(3);C3D_TexEnvColor(e,0x20ffffff);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE1,GPU_CONSTANT,GPU_CONSTANT);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    e=C3D_GetTexEnv(4);C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,0);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
}
int fh_minimal_water_group(const FhSceneAsset *asset,const FhSceneGroup *group,
    const float view[3][4],const unsigned scissor[4]){
    if(fh_render_budget_world)return 1;
    if(!initialized || !asset || !group || !asset->block || !asset->direct ||
        !group->indices || !fh_minimal_water.valid || !native_surface_copies)goto failed;
    unsigned clip[4];if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return 1;
    u64 started=svcGetSystemTick();begin();flush_ui();prepared_join();
    C3D_Tex *caustic=base_texture(&fh_minimal_water.caustic);if(!caustic)goto failed;
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Native water command bounds");
    if((capacity-offset)*4<16*1024)fh_minimal_sync();
    C3D_BindProgram(&water_program);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    unsigned packed=asset->compact;
    AttrInfo_AddLoader(a,0,packed?GPU_SHORT:GPU_FLOAT,4);
    AttrInfo_AddLoader(a,1,packed?GPU_UNSIGNED_BYTE:GPU_FLOAT,4);
    AttrInfo_AddLoader(a,2,packed?GPU_SHORT:GPU_FLOAT,2);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    /* PICA requires fetch extent == stride. Skip unused second UV (packed)
     * or normal/skin tail (float) explicitly; padding consumes no shader input.
     * Padding C=4 bytes, F=16 bytes, D=8 bytes, each aligned to four. */
    if(BufInfo_Add(b,group->vertex,packed?sizeof(FhWorldCompactVertex):sizeof(FhModelVertex),
        packed?4:5,packed?0xc210:0xdf210)<0)
        fh_platform_panic("Native water vertex binding");
    float ps=packed?(group->format==7?1.f:group->format==5?1.f/8:1.f/256):1.f;
    float cs=packed?1.f/15:1.f,us=packed?(group->format==5?1.f/256:1.f/1024):1.f;
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0]*ps,view[r][1]*ps,view[r][2]*ps,view[r][3]);
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,cs,cs,cs,cs);
    float sx=base_uv_scale(fh_minimal_water.caustic.width,caustic->width);
    float sy=base_uv_scale(fh_minimal_water.caustic.height,caustic->height);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,8,sx*us,sy*us,fh_minimal_water.scroll[0]*sx,fh_minimal_water.scroll[1]*sy);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,9,1,1,0,0);
    static C3D_Tex reflection_sampler;
    reflection_sampler=native_surface_reflection;
    reflection_sampler.param=(reflection_sampler.param&~(7u<<28))|(GPU_TEX_PROJECTION<<28);
    C3D_TexBind(0,&reflection_sampler);C3D_TexBind(1,caustic);C3D_TexBind(2,NULL);
    native_water_env();fh_native_fog_apply(1);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    C3D_DrawElements(GPU_TRIANGLES,group->indices,C3D_UNSIGNED_SHORT,group->index);
    fh_native_scene_used(asset);water_draws++;fh_minimal_draws++;frame_draws++;
    frame_indices+=group->indices;fh_minimal_indices+=group->indices;
    u64 elapsed=svcGetSystemTick()-started;water_ticks+=elapsed;submit_ticks+=elapsed;return 1;
failed:
    water_failures++;fh_minimal_rejected++;return 0;
}
