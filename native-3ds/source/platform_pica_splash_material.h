/* Load-compiled SDK combiner descriptions. CPU storage only; the SDK copies
 * these values into its owned command state at publication. No GPU lifetime. */
static C3D_TexEnv splash_materials[3][6];
static void splash_material_prepare(void){
    for(unsigned mode=0;mode<3;mode++){
        C3D_TexEnv *stages=splash_materials[mode];
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(stages+i);
        C3D_TexEnv *e=stages;
        C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,0,0);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_REPLACE);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_PRIMARY_COLOR,
            mode==2?GPU_TEXTURE1:mode?GPU_TEXTURE0:GPU_PRIMARY_COLOR,0);
        C3D_TexEnvFunc(e,C3D_Alpha,mode?GPU_MODULATE:GPU_REPLACE);
        C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,
            intensity[0]?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
        if(mode==2)for(unsigned i=1;i<4;i++){
            e=stages+i;
            C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
            C3D_TexEnvFunc(e,C3D_RGB,GPU_MULTIPLY_ADD);
        }
        if(mode){
            e=stages+(mode==2?4:1);
            C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,GPU_PREVIOUS,
                mode==2?GPU_TEXTURE2:GPU_TEXTURE1);
            C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,
                intensity[1]?GPU_TEVOP_RGB_SRC_R:GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
        }
    }
}
static void splash_material_publish(unsigned mode,uint32_t low,const uint32_t *weights){
    /* Republish all six stages after any other draw owner. Do not cache only
     * by mode: sky, fog, particles and ordinary actors share this SDK state. */
    for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,splash_materials[mode]+i);
    C3D_TexEnvColor(C3D_GetTexEnv(0),low);
    if(mode)C3D_TexEnvColor(C3D_GetTexEnv(mode==2?4:1),low+0x00c0c0c0u);
    if(mode==2)for(unsigned i=0;i<3;i++)C3D_TexEnvColor(C3D_GetTexEnv(i+1),weights[i]);
}
