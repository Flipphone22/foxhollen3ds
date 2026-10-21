/* Reduced-detail engine19 ripples and wakes. One native projective reflection
 * replaces two GX distorted samples; the original ABG mask channel, lifetime,
 * geometry, tint and fog remain. BUMPN's 5-bit quantization is approximated by
 * the full sampled channel (maximum 7/255 alpha difference before blending). */
FhMinimalWaterEffect fh_minimal_water_effect;
FhMinimalSplashMaterial fh_minimal_splash;
static unsigned splash_draws;
static unsigned water_effect_draws,water_effect_failures;
static u64 water_effect_ticks;
static void native_water_effect_env(void){
    const FhMinimalWaterEffect *m=&fh_minimal_water_effect;
    unsigned tint=m->tint[0]|m->tint[1]<<8|m->tint[2]<<16|0x80000000u;
    unsigned mix=m->mix[0]|m->mix[1]<<8|m->mix[2]<<16|(unsigned)m->mix[3]<<24;
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvBufUpdate(C3D_Both,0);
    C3D_TexEnv *e=C3D_GetTexEnv(0);C3D_TexEnvColor(e,tint);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_CONSTANT);
    if(m->heavy_fog){
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    }else C3D_TexEnvFunc(e,C3D_RGB,GPU_ADD);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);
    e=C3D_GetTexEnv(1);C3D_TexEnvColor(e,mix);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_PREVIOUS,GPU_CONSTANT);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE1,GPU_PREVIOUS,0);
    C3D_TexEnvOpAlpha(e,m->green_mask?GPU_TEVOP_A_SRC_G:GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
}
static void native_splash_env(void){
    const FhMinimalSplashMaterial *m=&fh_minimal_splash;
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<3;i++){
        const unsigned char *k=m->weight[i];C3D_TexEnv *e=C3D_GetTexEnv(i);
        C3D_TexEnvColor(e,k[0]|k[1]<<8|k[2]<<16|(unsigned)k[3]<<24);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,i?GPU_PREVIOUS:GPU_PRIMARY_COLOR);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MULTIPLY_ADD);
        C3D_TexEnvSrc(e,C3D_Alpha,i==1?GPU_TEXTURE1:i?GPU_PREVIOUS:GPU_PRIMARY_COLOR,GPU_PREVIOUS,0);
        C3D_TexEnvFunc(e,C3D_Alpha,i==1?GPU_MODULATE:GPU_REPLACE);
    }
    C3D_TexEnv *e=C3D_GetTexEnv(3);
    unsigned bright=(m->ambient[0]+192)|((m->ambient[1]+192)<<8)|((m->ambient[2]+192)<<16)|0xff000000u;
    C3D_TexEnvColor(e,bright);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,GPU_PREVIOUS,GPU_TEXTURE2);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
}
static void native_water_effect_draw(const FhMinimalEffectVertex *vertices,unsigned count,
    const float view[3][4],const unsigned scissor[4],unsigned splash){

    if(!initialized || !vertices || !view || !(splash?fh_minimal_splash.valid:fh_minimal_water_effect.valid) || !native_surface_copies || !count || count>UI_VERTICES || count%3)goto failed;
    unsigned clip[4];if(!mp_scissor_rect(scissor[0],scissor[1],scissor[2],scissor[3],clip))return;
    u64 started=svcGetSystemTick();begin();flush_ui();prepared_join();
    if(ui_used+count>UI_VERTICES)fh_minimal_sync();
    const FhTexture *mask_source=splash?&fh_minimal_splash.mask:&fh_minimal_water_effect.mask;
    C3D_Tex *mask=base_texture(mask_source);if(!mask)goto failed;
    base_pin=mask;C3D_Tex *foam=splash?base_texture(&fh_minimal_splash.foam):NULL;base_pin=NULL;
    if(splash && !foam)goto failed;
    u32 capacity,offset;GPUCMD_GetBuffer(NULL,&capacity,&offset);
    if(offset>capacity)fh_platform_panic("Native water effect command bounds");
    if((capacity-offset)*4<16*1024)fh_minimal_sync();
    /* Shared stream is fenced by fh_minimal_sync before reuse; no new pool. */
    MinimalUiVertex *first=ui_vertices+ui_used;
    _Static_assert(sizeof(*first)==sizeof(*vertices),"Water effect vertex layout");
    memcpy(first,vertices,count*sizeof(*first));ui_used+=count;
    fh_gpu_cache_flush(first,count*sizeof(*first));
    C3D_BindProgram(&water_program);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);AttrInfo_AddLoader(a,2,GPU_FLOAT,2);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,first,sizeof(*first),3,0x210)<0)fh_platform_panic("Native water effect binding");
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0],view[r][1],view[r][2],view[r][3]);
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,splash?fh_minimal_splash.ambient[0]/255.f:1,
        splash?fh_minimal_splash.ambient[1]/255.f:1,splash?fh_minimal_splash.ambient[2]/255.f:1,1);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,8,base_uv_scale(mask_source->width,mask->width),
        base_uv_scale(mask_source->height,mask->height),0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,9,foam?12*base_uv_scale(fh_minimal_splash.foam.width,foam->width):1,
        foam?base_uv_scale(fh_minimal_splash.foam.height,foam->height):1,0,0);
    static C3D_Tex reflection_sampler;reflection_sampler=native_surface_reflection;
    reflection_sampler.param=(reflection_sampler.param&~(7u<<28))|(GPU_TEX_PROJECTION<<28);
    C3D_TexBind(0,&reflection_sampler);C3D_TexBind(1,mask);C3D_TexBind(2,foam);
    if(splash)native_splash_env();else native_water_effect_env();fh_native_fog_apply(1);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
    C3D_CullFace(GPU_CULL_NONE);C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    unsigned draws=1;
    if(splash){
        /* Same front-cull then back-cull order as the original two GX lists. */
        C3D_CullFace(GPU_CULL_BACK_CCW);C3D_DrawArrays(GPU_TRIANGLES,0,count);
        C3D_CullFace(GPU_CULL_FRONT_CCW);draws=2;splash_draws++;
    }
    C3D_DrawArrays(GPU_TRIANGLES,0,count);
    water_effect_draws+=draws;fh_minimal_draws+=draws;frame_draws+=draws;
    frame_indices+=count*draws;fh_minimal_indices+=count*draws;
    u64 elapsed=svcGetSystemTick()-started;water_effect_ticks+=elapsed;submit_ticks+=elapsed;return;
failed:
    water_effect_failures++;fh_minimal_rejected++;
}

void fh_minimal_water_effect_draw(const FhMinimalEffectVertex vertices[6],
    const float view[3][4],const unsigned scissor[4]){
    if(fh_render_budget_world)return;
    native_water_effect_draw(vertices,6,view,scissor,0);
}
void fh_minimal_splash_draw(const FhMinimalEffectVertex vertices[630],const unsigned scissor[4]){
    static const float identity[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    if(fh_render_budget_world)return;
    native_water_effect_draw(vertices,630,identity,scissor,1);
}
