/* Integer gather only; PICA performs exact power-of-two position/normal
 * conversion. UV arithmetic and color semantics retain the ordinary packer.
 * This deliberately excludes floats, NBT, CPU lighting and nontrivial texgen. */
unsigned fh_compiled_compact_mode,fh_compiled_compact_vertices;
static int prepare_compact_compiled(FastCompiled *fast){
    if(!prepare_fast_compiled(fast))return 0;
    /* This layout supplies a fixed color per draw, not a vertex color array. */
    if(fast->color_mask || fast->color[0] || fast->color[1])return 0;
    for(unsigned i=!!(fh_gx.draw.raw_position&4);i<fast->texgens;i++)
        if(prepared_texgen[i].post)return 0;
    const PreparedField *p=fast->position,*n=fast->normal;
    if(!n)return 0;
    if(p->format.type!=GX_S16 || p->components!=3 || p->bytes!=6 ||
       n->format.type!=GX_S8 || n->components!=3 || n->bytes!=3)return 0;
    /* Integer inputs have <=16 significant bits. Scaling by these exact
     * powers of two cannot introduce a new float24 rounding step. Bounds
     * also imply every scalar GPU-eligibility test, including palette3. */
    if(!(p->scale>=0x1p-30f && p->scale<=1.f) ||
       !(n->scale>=0x1p-16f && n->scale<=0x1p-5f))return 0;
    int e; if(frexpf(p->scale,&e)!=.5f || frexpf(n->scale,&e)!=.5f)return 0;
    return 1;
}
static const u8 *compact_field(const PreparedField *f,const u8 *packet,int *le){
    const u8 *data=packet+f->offset;*le=0;
    if(f->mode!=GX_DIRECT)data=indexed(f->attr,f->mode==GX_INDEX8?*data:be16(data),0,f->bytes,le);
    return data;
}
static int decode_compact_vertex(FhCompactVertex *out,const u8 *packet,const FastCompiled *fast,
        const FhNativePacking *pack,const FhCompactVertex *base){
    unsigned matrix=fast->matrix_offset==~0u?fh_gx.current_matrix:packet[fast->matrix_offset];
    if(matrix%3 || matrix>=192 || (fast->matrix_offset!=~0u && matrix>=30) || (gpu_lighting_palette && matrix>=30))return 0;
    if(fh_gx.draw.raw_position&4){
        unsigned texture=fast->texture_offset==~0u?fh_gx.texgens[0].matrix:packet[fast->texture_offset];
        if(texture!=matrix+GX_TEXMTX0)return 0;
    }
    *out=*base;out->matrix[0]=fast->matrix_offset==~0u?0:matrix;
    int le;const u8 *data=compact_field(fast->position,packet,&le);
    for(unsigned c=0;c<3;c++)out->position[c]=(int16_t)(le?data[c*2]|(unsigned)data[c*2+1]<<8:be16(data+c*2));
    data=compact_field(fast->normal,packet,&le);
    for(unsigned c=0;c<3;c++)out->normal[c]=(int8_t)data[c];
    for(unsigned u=0;u<pack->units;u++){
        if((fh_gx.draw.raw_position&4) && (pack->generated_units&(1u<<u))){
            out->uv[u][0]=pack->scale[u][0];out->uv[u][1]=pack->scale[u][1];continue;
        }
        unsigned coord=pack->coord[u];float uv[3]={0,0,1};
        if(!(coord==0 && (fh_gx.draw.raw_position&4))){
            unsigned source=fh_gx.texgens[coord].source-GX_TG_TEX0;
            if(fast->uv[source])fast_compiled_field(uv,fast->uv[source],packet);
        }
        out->uv[u][0]=uv[0]*pack->scale[u][0];out->uv[u][1]=1-uv[1]*pack->scale[u][1];
    }
    return 1;
}
static int decode_compiled_compact(FhCompactVertex *output,const FhCompiledMesh *mesh,
        const FhNativePacking *packing,FhCompactPacking *fixed){
    if(!fh_compiled_compact_mode || fh_retained_disabled || mesh->unique<3 || mesh->unique>FH_COMPILED_RETAINED_UNIQUE)return 0;
    CompiledAdmission *admission=compiled_admission_for(mesh);
    if(admission->misses<2 || !(admission->misses&31u))return 0;
    FastCompiled fast;if(!prepare_compact_compiled(&fast))return 0;
    if(!output)return 1;
    FhNativeVertex native;if(!fixed || !prepare_native_compiled(&native,&fast,packing))return 0;
    memcpy(fixed->color,native.color,sizeof(fixed->color));
    fixed->position_scale=fast.position->scale;fixed->normal_scale=fast.normal->scale;
    FhCompactVertex base={.position={0,0,0,1},.matrix={0,native.generated_normal_uv}};
    memcpy(base.uv,native.uv,sizeof(base.uv));
    for(unsigned i=0;i<mesh->unique;i++)if(!decode_compact_vertex(output+i,mesh->packets+mesh->offsets[i],&fast,packing,&base))return 0;
    admission->misses++;fh_retained_end();fh_compiled_admission_skips++;
    fh_compiled_compact_vertices+=mesh->unique;fh_compiled_decode_misses++;
    if(fh_compiled_compact_mode!=2)for(unsigned i=0;i<mesh->unique;i++){
        decoded_vertices++;received++;if(!++vertex_identity)++vertex_identity;
    }
    return fh_compiled_compact_mode==2?2:1;
}
