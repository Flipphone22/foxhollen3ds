/* Included by gx_vertices.c after the scalar decoder. Animated arrays remain
 * live: this specializes decoding, never caches mutable vertex values.
 * Pinned Melee gx.c's prepared fields/raw attributes are the reference;
 * SFA retains little-endian array handling and per-vertex scalar fallbacks. */
typedef struct {
    FhVertex base;
    const PreparedField *position,*normal,*uv[8],*color[2];
    unsigned matrix_offset,texture_offset,texgens,color_mask;
} FastCompiled;
unsigned fh_compiled_fast_mode=1,fh_compiled_fast_vertices,fh_compiled_fast_checks;
unsigned fh_compiled_color_reads;
static int prepare_fast_compiled(FastCompiled *p){
    unsigned raw=fh_gx.draw.raw_position;
    if(!fh_compiled_fast_mode || (raw&3)!=3 || (raw&~39u) || !gpu_lighting ||
       gpu_lighting_palette==1 || lighting_needs_normal || texgen_needs_eye ||
       fh_gx_active_texgens>6)return 0;
    memset(p,0,sizeof(*p));p->matrix_offset=p->texture_offset=~0u;
    p->texgens=fh_gx_active_texgens;
    for(unsigned i=0;i<4;i++)if(raster_channels&(1u<<(i&1))){
        const FhChannel *c=&fh_gx.channel[i];
        unsigned deferred=i==0 || (i==2 && (raw&32));
        if(!deferred && c->enabled && !(constant_lighting&(1u<<i)))return 0;
        /* Deferred and unlit material channels consume current vertex color.
         * CPU-lit variable channels keep the full scalar lighting path. */
        if(c->material_source==GX_SRC_VTX &&
           (deferred || !(constant_lighting&(1u<<i))))
            p->color_mask|=(i>=2?8u:7u)<<((i&1)*4);
    }
    for(unsigned i=0;i<p->texgens;i++){
        if(!(fh_gx_texgen_inputs&(1u<<i)))continue;
        const FhTexGen *g=&fh_gx.texgens[i];
        if(!i && (raw&4)){if(!gpu_texgen)return 0;continue;}
        if(g->type!=GX_TG_MTX2x4 && g->type!=GX_TG_MTX3x4)return 0;
        if(g->matrix!=GX_IDENTITY || g->normalize ||
           g->source<GX_TG_TEX0 || g->source>GX_TG_TEX7 || fh_gx.descriptors[GX_VA_TEX0MTXIDX+i])return 0;
        if(g->postmatrix!=GX_PTIDENTITY){
            const float (*post)[4]=prepared_texgen[i].post;
            if(!post || post[2][0]!=0 || post[2][1]!=0 ||
               !((post[2][2]==0 && post[2][3]==1) || (post[2][2]==1 && post[2][3]==0)))return 0;
            for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)if(!finite_vertex_float(post[r][c]))return 0;
        }
    }
    for(unsigned i=0;i<field_count;i++){
        const PreparedField *f=fields+i;
        if(f->attr==GX_VA_PNMTXIDX)p->matrix_offset=f->offset;
        else if(f->attr==GX_VA_TEX0MTXIDX && (raw&4))p->texture_offset=f->offset;
        else if(f->attr==GX_VA_POS)p->position=f;
        else if(f->attr==GX_VA_NRM){if(f->format.count!=GX_NRM_XYZ)return 0;p->normal=f;}
        else if(f->attr==GX_VA_CLR0 || f->attr==GX_VA_CLR1)p->color[f->attr-GX_VA_CLR0]=f;
        else if(f->attr>=GX_VA_TEX0 && f->attr<=GX_VA_TEX7)p->uv[f->attr-GX_VA_TEX0]=f;
        else return 0;
    }
    if(!p->position || (!p->normal && !gpu_lighting_flat))return 0;
    float colors[2][4]={{1,1,1,1},{1,1,1,1}},zero[4]={0};
    illuminate(p->base.color,colors,zero,zero,1);p->base.position[3]=1;
    for(unsigned i=0;i<p->texgens;i++)if(fh_gx_texgen_inputs&(1u<<i))p->base.uv[i][2]=1;
    if(raw&4)p->base.uv[6][0]=1;
    return 1;
}
static void fast_compiled_field(float *out,const PreparedField *f,const u8 *packet){
    const u8 *data=packet+f->offset;int le=0;
    if(f->mode!=GX_DIRECT)data=indexed(f->attr,f->mode==GX_INDEX8?*data:be16(data),0,f->bytes,&le);
    components(out,data,f->components,f->format.type,le,f->scale);
}
static void fast_compiled_colors(float out[2][4],const FastCompiled *p,const u8 *packet){
    fh_compiled_color_reads++;
    float colors[2][4]={{1,1,1,1},{1,1,1,1}};
    for(unsigned channel=0;channel<2;channel++)if(p->color[channel]){
        const PreparedField *f=p->color[channel];const u8 *data=packet+f->offset;int le=0;
        if(f->mode!=GX_DIRECT)data=indexed(f->attr,f->mode==GX_INDEX8?*data:be16(data),0,f->bytes,&le);
        decode_color(colors[channel],data,f->format.type,le);
    }
    for(unsigned channel=0;channel<2;channel++)for(unsigned c=0;c<4;c++)
        if(p->color_mask&(1u<<(channel*4+c)))out[channel][c]=colors[channel][c];
}
static int fast_compiled_post(float uv[3],unsigned coord){
    const float (*post)[4]=prepared_texgen[coord].post;
    if(!post || (compiled_post_template && coord==1))return 1;
    /* Keep the renderer's affine q=1 contract; nonfinite source coordinates
     * can make zero coefficients produce NaN and require scalar fallback. */
    for(unsigned c=0;c<3;c++)if(!finite_vertex_float(uv[c]))return 0;
    float result[3];transform(result,post,uv,1);fh_copy_bytes(uv,result,12);return 1;
}
static int fast_compiled_position_normal(float position[3],float normal[3],unsigned *matrix_out,const u8 *packet,const FastCompiled *p){
    unsigned matrix=p->matrix_offset==~0u?fh_gx.current_matrix:packet[p->matrix_offset];
    if(matrix%3 || matrix>=192 || (p->matrix_offset!=~0u && matrix>=30) ||
       (gpu_lighting_palette && matrix>=30))return 0;
    if(fh_gx.draw.raw_position&4){
        unsigned texture=p->texture_offset==~0u?fh_gx.texgens[0].matrix:packet[p->texture_offset];
        if(texture!=matrix+GX_TEXMTX0)return 0;
    }
    float largest=0;position[0]=position[1]=position[2]=0;normal[0]=normal[1]=0;normal[2]=1;
    fast_compiled_field(position,p->position,packet);
    if(p->normal)fast_compiled_field(normal,p->normal,packet);
    for(unsigned c=0;c<3;c++){
        float n=fabsf(normal[c]);if(n>largest)largest=n;
        if(!finite_vertex_float(position[c]) || fabsf(position[c])>1e5f ||
           (!gpu_lighting_flat && (!finite_vertex_float(n) || n>64)))return 0;
    }
    if(!gpu_lighting_flat && largest>0 && largest<1e-6f)return 0;
    if(gpu_lighting_palette>=2){
        float maximum=gpu_lighting_palette==3?8.f:16.f,minimum=gpu_lighting_palette==3?8e-6f:4e-6f;
        if(largest>maximum || (largest>0 && largest<minimum))return 0;
    }
    *matrix_out=p->matrix_offset==~0u?0:matrix;return 1;
}
static int decode_fast_compiled(FhVertex *out,const u8 *packet,const FastCompiled *p){
    float position[3],normal[3];unsigned matrix;
    if(!fast_compiled_position_normal(position,normal,&matrix,packet,p))return 0;
    *out=p->base;fh_copy_bytes(out->position,position,12);fh_copy_bytes(out->uv[7],normal,12);
    if(p->color[0] || p->color[1] || p->color_mask)fast_compiled_colors(out->color,p,packet);
    out->matrix_index=matrix;
    float uv[8][3];
    for(unsigned i=0;i<8;i++){
        uv[i][0]=uv[i][1]=0;uv[i][2]=1;
        if(p->uv[i])fast_compiled_field(uv[i],p->uv[i],packet);
    }
    for(unsigned i=!!(fh_gx.draw.raw_position&4);i<p->texgens;i++){
        if(!(fh_gx_texgen_inputs&(1u<<i)))continue;
        fh_copy_bytes(out->uv[i],uv[fh_gx.texgens[i].source-GX_TG_TEX0],12);
        if(!fast_compiled_post(out->uv[i],i))return 0;
    }
    return 1;
}

/* Pack the renderer's existing 88-byte layout directly from current indexed
 * attributes. There is no intermediate 148-byte vertex or animated cache.
 * Lighting and sphere-coordinate transforms remain live GPU uniforms. */
static int prepare_native_compiled(FhNativeVertex *base,const FastCompiled *fast,const FhNativePacking *pack){
    if(!pack || pack->units>3 || pack->affine || pack->sphere)return 0;
    memset(base,0,sizeof(*base));base->position[3]=1;
    for(unsigned c=0;c<4;c++){
        if(pack->primary[c]>7)return 0;
        base->color[c]=pack->primary[c]<0?0:((const float*)fast->base.color)[pack->primary[c]];
    }
    for(unsigned u=0;u<pack->units;u++){
        if(pack->coord[u]>=fast->texgens)return 0;
        base->uv[u][2]=1;
    }
    if(fh_gx.draw.raw_position&4)base->generated_normal_uv=1;
    return 1;
}
static int decode_native_compiled(FhNativeVertex *out,const u8 *packet,const FastCompiled *fast,
        const FhNativePacking *pack,const FhNativeVertex *base){
    float position[3],normal[3];unsigned matrix;
    if(!fast_compiled_position_normal(position,normal,&matrix,packet,fast))return 0;
    /* The existing renderer only uploads ten matrices, even when GX accepts
     * a larger constant matrix index and remaps it to slot zero. */
    if(matrix>27 || matrix%3)return 0;
    *out=*base;fh_copy_bytes(out->position,position,12);fh_copy_bytes(out->normal,normal,12);out->matrix_index=matrix;
    if(fast->color[0] || fast->color[1] || fast->color_mask){
        float colors[2][4];fh_copy_bytes(colors,fast->base.color,sizeof(colors));
        fast_compiled_colors(colors,fast,packet);
        for(unsigned c=0;c<4;c++)out->color[c]=pack->primary[c]<0?0:((const float*)colors)[pack->primary[c]];
    }
    for(unsigned u=0;u<pack->units;u++){
        if((fh_gx.draw.raw_position&4) && (pack->generated_units&(1u<<u))){
            out->uv[u][0]=pack->scale[u][0];out->uv[u][1]=pack->scale[u][1];continue;
        }
        unsigned coord=pack->coord[u];float uv[3]={0,0,1};
        if(!(coord==0 && (fh_gx.draw.raw_position&4))){
            unsigned source=fh_gx.texgens[coord].source-GX_TG_TEX0;
            if(fast->uv[source])fast_compiled_field(uv,fast->uv[source],packet);
            if(!fast_compiled_post(uv,coord))return 0;
        }
        out->uv[u][0]=uv[0]*pack->scale[u][0];out->uv[u][1]=1-uv[1]*pack->scale[u][1];
    }
    return 1;
}
