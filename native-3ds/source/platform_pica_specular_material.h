/* Load-time secondary-light material compiler, included by native image owner.
 * One base image with optional co-registered opacity, no auxiliary bump or
 * secondary light mask. The caller counts rejected source materials. */
/* SPECULAR_PREFETCH_V1: compiled once, no per-frame material scan. */
static void specular_demand_add(const FhSceneAsset *asset,const Material *m){
    Material *first=material_for(asset,0);
    if(first)first->specular_demand|=1u<<(m->specular_slot+(m->layered?3:0));
}
int fh_pica_material_specular_compile(const FhSceneAsset *asset,unsigned index,
    unsigned selector,unsigned alpha,unsigned base_modulate){
    Material *m=material_for(asset,index);
    if(!m || !m->ready || !m->actor || m->specular_program)return 0;
    if(m->layered){
        /* Common pre-light interpolated pair, constant opacity, C2*base.
         * Other modes/masks retain counted source-family omissions. */
        if(m->layers!=2 || m->masked || (!(m->layered&2) && !m->environment) || (m->layered>>2)!=1 ||
           !m->alpha_constant || alpha || !base_modulate || m->intensity[0] || m->intensity[1])return 0;
        C3D_TexEnv e[4];
        if(m->layered&2)fh_pica_layered_specular_plan(e,m->environment,selector);
        else fh_pica_postlight_specular_plan(e,m->environment,selector);
        m->specular_program=acquire_program(e,(m->layered&2)?0x11:0);
        if(!m->specular_program)return 0;
        m->specular_slot=selector>=6?0:selector>>1;m->specular_alpha=0;specular_demand_add(asset,m);return 1;
    }
    if(m->layers!=1+!!m->masked)return 0;
    if(base_modulate && alpha)return 0; /* Mutually exclusive original modes. */
    if(m->masked){
        if(!m->count[0] || m->count[0]!=m->count[1])return 0;
        for(unsigned i=0;i<m->count[0];i++)for(unsigned xy=0;xy<2;xy++)
            if(m->frames[0][i].scale[xy]!=m->frames[1][i].scale[xy])return 0;
    }
    C3D_TexEnv e[4];for(unsigned i=0;i<4;i++)C3D_TexEnvInit(e+i);
    if(m->environment)e[0]=m->lit_env[0];
    else {
        C3D_TexEnvSrc(e,C3D_Both,GPU_PRIMARY_COLOR,0,0);
        C3D_TexEnvFunc(e,C3D_Both,GPU_REPLACE);
    }
    /* C1 is saved by stage0. Stage1 can read its preceding result; following
     * stages consume the retained buffer, preserving both clamps in C2. */
    C3D_TexEnvSrc(e+1,C3D_RGB,GPU_TEXTURE0,GPU_PREVIOUS,0);
    C3D_TexEnvOpRgb(e+1,(selector&1)?GPU_TEVOP_RGB_SRC_B:GPU_TEVOP_RGB_SRC_R,GPU_TEVOP_RGB_SRC_COLOR,0);
    C3D_TexEnvFunc(e+1,C3D_RGB,GPU_MODULATE);
    C3D_TexEnvSrc(e+1,C3D_Alpha,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvOpAlpha(e+1,GPU_TEVOP_A_SRC_G,GPU_TEVOP_A_SRC_ALPHA,0);
    C3D_TexEnvFunc(e+1,C3D_Alpha,GPU_MODULATE);
    C3D_TexEnvSrc(e+2,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e+2,GPU_TEVOP_RGB_SRC_ALPHA,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(e+2,C3D_RGB,GPU_MULTIPLY_ADD);
    if(base_modulate){
        /* Original addTexModulateReg2Stage uses the following base layer's
         * texture unit and coordinates; it does not load a separate image. */
        C3D_TexEnvSrc(e+2,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE1,0);
        C3D_TexEnvOpRgb(e+2,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,0);
        C3D_TexEnvFunc(e+2,C3D_RGB,GPU_MODULATE);
    }
    /* Stage2 RGB consumes stage1 alpha while its independent alpha channel
     * prepares base opacity. Stage3 then applies the optional mask. */
    C3D_TexEnvSrc(e+2,C3D_Alpha,m->alpha_constant?GPU_CONSTANT:GPU_PRIMARY_COLOR,GPU_TEXTURE1,0);
    C3D_TexEnvOpAlpha(e+2,GPU_TEVOP_A_SRC_ALPHA,m->intensity[0]?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
    C3D_TexEnvFunc(e+2,C3D_Alpha,m->masked?GPU_REPLACE:GPU_MODULATE);
    C3D_TexEnvSrc(e+3,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE2,0);
    C3D_TexEnvOpAlpha(e+3,GPU_TEVOP_A_SRC_ALPHA,m->intensity[1]?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
    C3D_TexEnvFunc(e+3,C3D_Alpha,m->masked?GPU_MODULATE:GPU_REPLACE);
    C3D_TexEnvSrc(e+3,C3D_RGB,GPU_TEXTURE1,GPU_PREVIOUS_BUFFER,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e+3,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(e+3,C3D_RGB,GPU_MULTIPLY_ADD);
    m->specular_program=acquire_program(e,1);
    if(!m->specular_program)return 0;
    m->specular_slot=selector>=6?0:selector>>1;m->specular_alpha=alpha&255;
    specular_demand_add(asset,m);return 1;
}
