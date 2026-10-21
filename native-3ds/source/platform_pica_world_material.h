/* Ordinary world materials consume their already compiled TEV program directly.
 * Special owners retain their native binders. No material/image allocation,
 * geometry work, source interpretation, target switch or cache-coherence change. */
static unsigned world_material_binds;
static unsigned world_material_plain(const Material *m){
    return !m->actor && !m->environment && !m->swizzle && !m->ui.kind &&
        !m->moon_alpha && !m->movie_active && !m->specular_program &&
        !m->diffuse_program && !m->cloud_state && !m->post_state;
}
static int world_material_bind(Material *m,unsigned format,unsigned compact){
    if(m->flags&((fh_pica_world_water_active?0u:0x80000000u)|2u))return 2;
    if(!m->program || !programs[m->program-1].refs)
        fh_platform_panic("PICA world material program lifetime");
    Program *program=programs+m->program-1;
    fh_pica_texture2_coord1(0);C3D_TexBind(2,NULL);
    unsigned changed=bound_program!=m->program;
    if(changed){
        C3D_TexEnvBufUpdate(C3D_RGB,program->buffer_mask&15);
        C3D_TexEnvBufUpdate(C3D_Alpha,program->buffer_mask>>4);program_binds++;
    }else program_reuses++;
    for(unsigned i=0;i<4;i++){
        if(changed){C3D_SetTexEnv(i,program->stages+i);C3D_TexEnvColor(C3D_GetTexEnv(i),m->konst);}
        else if(bound_colors[i]!=m->konst)C3D_TexEnvColor(C3D_GetTexEnv(i),m->konst);
        bound_colors[i]=m->konst;
    }
    bound_program=m->program;
    float cs=compact?1.f/15:1.f,us=compact?(format==5?1.f/256:1.f/1024):1.f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,m->tint[0]*cs,m->tint[1]*cs,m->tint[2]*cs,m->tint[3]*cs);
    for(unsigned i=0;i<2;i++){
        Frame *f=i<m->layers?m->frames[i]+m->current[i]:NULL;
        if(f && (!f->handle || !images[f->handle-1].refs))fh_platform_panic("PICA world image lifetime");
        float sx=f?f->scale[0]:1,sy=f?f->scale[1]:1;
        C3D_TexBind(i,fh_pica_options_texture(f?&f->sampler:NULL,FH_GRAPHICS_NO_WORLD_TEX));
        C3D_FVUnifSet(GPU_VERTEX_SHADER,8+i,sx*us,sy*us,m->scroll[i][0]*sx,m->scroll[i][1]*sy);
    }
    unsigned blend=!!(m->flags&0x60000000u);
    C3D_DepthTest(true,GPU_GEQUAL,blend?GPU_WRITE_COLOR:GPU_WRITE_ALL);
    C3D_AlphaTest(!blend && (m->masked || ((m->flags&0x400) && !(m->flags&0x80))),GPU_GREATER,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,blend?GPU_SRC_ALPHA:GPU_ONE,blend?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
    world_material_binds++;bindings++;return 1;
}
