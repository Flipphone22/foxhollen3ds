/* Load-time routing for the original mode-0 map. A co-registered mask shares
 * base UV1; independent layers use the existing three-UV/fragment-fog shader.
 * Its c7 is layer coordinates, so only constant-opacity layered materials
 * are admitted: vertex opacity needs a shader retaining the object tint. */
static int diffuse_material_compile(Material *m){
    /* Constant-opacity unlit materials use the same sampler layout, avoiding
     * a pipeline switch between adjacent lit/unlit parts of one actor. */
    if(!m->environment && !m->alpha_constant)return 1;
    if(m->layers>2 || (m->layers==2 && !m->masked && !m->layered) ||
       (m->layered && (!m->alpha_constant || m->masked))){diffuse_partial++;return 1;}
    if(m->masked){
        if(m->layers!=2 || !m->count[0] || m->count[0]!=m->count[1]){diffuse_partial++;return 1;}
        for(unsigned i=0;i<m->count[0];i++)for(unsigned xy=0;xy<2;xy++)
            if(m->frames[0][i].scale[xy]!=m->frames[1][i].scale[xy]){diffuse_partial++;return 1;}
    }
    C3D_TexEnv stages[4];memcpy(stages,m->environment?m->lit_env:m->env,sizeof(stages));
    for(unsigned i=0;i<4;i++){
        C3D_TexEnv *e=stages+i;
        unsigned sources[2]={e->srcRgb,e->srcAlpha};
        for(unsigned channel=0;channel<2;channel++)for(unsigned operand=0;operand<3;operand++){
            unsigned shift=4*operand,src=(sources[channel]>>shift)&15;
            if(m->environment && !i && !channel && src==GPU_FRAGMENT_PRIMARY_COLOR)src=GPU_TEXTURE0;
            else if((i || !m->environment) && src==GPU_TEXTURE0)src=GPU_TEXTURE1;
            else if((i || !m->environment) && src==GPU_TEXTURE1)src=GPU_TEXTURE2;
            sources[channel]=(sources[channel]&~(15u<<shift))|(src<<shift);
        }
        e->srcRgb=sources[0];e->srcAlpha=sources[1];
    }
    m->diffuse_program=acquire_program(stages,m->environment?1:0); /* C1 only when present. */
    if(!m->diffuse_program)return 0;
    diffuse_admitted++;return 1;
}
