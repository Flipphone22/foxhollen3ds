/* Included in gx_vertices.c. Lower the current model's attribute/texgen
 * program to the PICA inputs actually consumed by its material. The program
 * borrows GX state only for this synchronous draw; it owns no game pointers
 * across a callback, animation update, or dispatch join.
 *
 * Unlike the FIFO interpreter, execution has no primitive assembly, decoded
 * vertex cache, FhVertex staging, or renderer repacking. Live CPU lighting is
 * deliberately retained until an equivalent GPU program can replace it.
 */
typedef struct {
    const PreparedField *position, *normal, *color[2], *uv[8];
    unsigned matrix_offset, texture_offset[8];
    unsigned generators, attributes, need_normal, need_eye;
} FhVertexProgram;

static int vertex_program_prepare(FhVertexProgram *p,const FhNativePacking *pack) {
    if(!pack || pack->units>3 || fh_gx.draw.raw_position!=1 || gpu_lighting ||
       fh_gx_active_texgens>8)return 0;
    *p=(FhVertexProgram){0};p->matrix_offset=~0u;
    for(unsigned i=0;i<8;i++)p->texture_offset[i]=~0u;
    p->need_normal=lighting_needs_normal;p->need_eye=lighting_needs_normal;
    if(pack->affine && pack->units>1)return 0;
    for(unsigned c=0;c<4;c++)if(pack->primary[c]>7)return 0;
    for(unsigned u=0;u<pack->units;u++) {
        if(pack->sphere && (pack->coord[u]==6 || pack->coord[u]==7))continue;
        if(pack->coord[u]>=fh_gx_active_texgens)return 0;
        p->generators|=1u<<pack->coord[u];
    }
    /* Close dependencies backwards. Forward coordinate references read the
     * scalar decoder's initial zero vector; retain that behavior explicitly. */
    for(int i=(int)fh_gx_active_texgens-1;i>=0;i--)if(p->generators&(1u<<i)) {
        const FhTexGen *g=&fh_gx.texgens[i];
        if(g->source>=GX_TG_TEXCOORD0 && g->source<=GX_TG_TEXCOORD6) {
            unsigned source=g->source-GX_TG_TEXCOORD0;
            if(source<(unsigned)i)p->generators|=1u<<source;
        } else if(g->source>=GX_TG_TEX0 && g->source<=GX_TG_TEX7)
            p->attributes|=1u<<(g->source-GX_TG_TEX0);
        if(g->source>=GX_TG_NRM && g->source<=GX_TG_TANGENT)p->need_normal=1;
        if(g->type>=GX_TG_BUMP0 && g->type<=GX_TG_BUMP7){
            if(g->source<GX_TG_TEXCOORD0 || g->source>GX_TG_TEXCOORD6 ||
               g->source-GX_TG_TEXCOORD0>=(unsigned)i)return 0;
            p->need_eye=p->need_normal=1;
        }
    }
    for(unsigned i=0;i<field_count;i++) {
        const PreparedField *f=fields+i;
        if(f->attr==GX_VA_PNMTXIDX)p->matrix_offset=f->offset;
        else if(f->attr<GX_VA_POS)p->texture_offset[f->attr-1]=f->offset;
        else if(f->attr==GX_VA_POS)p->position=f;
        else if(f->attr==GX_VA_NRM)p->normal=f;
        else if(f->attr==GX_VA_CLR0 || f->attr==GX_VA_CLR1)p->color[f->attr-GX_VA_CLR0]=f;
        else if(f->attr>=GX_VA_TEX0 && f->attr<=GX_VA_TEX7)p->uv[f->attr-GX_VA_TEX0]=f;
        else return 0;
    }
    return p->position!=NULL;
}

static void vertex_program_normal(float out[3][3],const PreparedField *f,const u8 *packet) {
    if(!f)return;
    if(f->mode!=GX_DIRECT && f->format.count==GX_NRM_NBT3) {
        const u8 *cursor=packet+f->offset;
        for(unsigned slice=0;slice<3;slice++) {
            unsigned index=f->mode==GX_INDEX8?*cursor++:be16(cursor);
            if(f->mode==GX_INDEX16)cursor+=2;
            int le=0;const u8 *data=indexed(GX_VA_NRM,index,slice*3*f->component_bytes,3*f->component_bytes,&le);
            components(out[slice],data,3,f->format.type,le,f->scale);
        }
    } else fast_compiled_field(&out[0][0],f,packet);
}

static int vertex_program_execute(FhNativeVertex *out,const u8 *packet,
        const FhVertexProgram *p,const FhNativePacking *pack) {
    unsigned matrix=p->matrix_offset==~0u?fh_gx.current_matrix:packet[p->matrix_offset];
    if(matrix%3 || matrix>=192 || (p->matrix_offset!=~0u && matrix>=30))return 0;
    float position[3]={0},normals[3][3]={{0,0,1},{0,1,0},{1,0,0}};
    fast_compiled_field(position,p->position,packet);
    if(p->need_normal)vertex_program_normal(normals,p->normal,packet);
    float colors[2][4]={{1,1,1,1},{1,1,1,1}},lit[2][4]={{0}},eye[4]={0,0,0,1},normal[3]={0};
    for(unsigned ch=0;ch<2;ch++)if(p->color[ch]) {
        const PreparedField *f=p->color[ch];const u8 *data=packet+f->offset;int le=0;
        if(f->mode!=GX_DIRECT)data=indexed(f->attr,f->mode==GX_INDEX8?*data:be16(data),0,f->bytes,&le);
        decode_color(colors[ch],data,f->format.type,le);
    }
    if(p->need_eye)transform(eye,fh_gx.position[matrix/3],position,1);
    if(lighting_needs_normal){transform(normal,fh_gx.normal[matrix/3],normals[0],0);normalize(normal);}
    illuminate(lit,colors,eye,normal,0);
    if(pack->sphere)for(unsigned c=0;c<8;c++)
        if(!(((const float*)lit)[c]>=0 && ((const float*)lit)[c]<=1))return 0;
    float attributes[8][3]={{0}},generated[8][3]={{0}};
    for(unsigned i=0;i<8;i++)if(p->attributes&(1u<<i)) {
        attributes[i][2]=1;
        if(p->uv[i])fast_compiled_field(attributes[i],p->uv[i],packet);
    }
    for(unsigned i=0;i<fh_gx_active_texgens;i++)if(p->generators&(1u<<i)) {
        const FhTexGen *g=&fh_gx.texgens[i];const PreparedTexGen *prepared=&prepared_texgen[i];
        float *result=generated[i],source[3]={0,0,1};
        if(g->type>=GX_TG_BUMP0 && g->type<=GX_TG_BUMP7) {
            float direction[3],tangent[3],binormal[3];
            const FhLight *light=&fh_gx.lights[g->type-GX_TG_BUMP0];
            for(unsigned c=0;c<3;c++)direction[c]=light->position[c]-eye[c];
            normalize(direction);
            transform(tangent,fh_gx.normal[matrix/3],normals[2],0);
            transform(binormal,fh_gx.normal[matrix/3],normals[1],0);
            fh_copy_bytes(result,generated[g->source-GX_TG_TEXCOORD0],12);
            result[0]+=dot(direction,tangent);result[1]+=dot(direction,binormal);continue;
        }
        if(g->source<=GX_TG_TANGENT)fh_copy_bytes(source,g->source==GX_TG_POS?position:normals[g->source-1],12);
        else if(g->source>=GX_TG_TEX0 && g->source<=GX_TG_TEX7)fh_copy_bytes(source,attributes[g->source-GX_TG_TEX0],12);
        else if(g->source>=GX_TG_TEXCOORD0 && g->source<=GX_TG_TEXCOORD6)fh_copy_bytes(source,generated[g->source-GX_TG_TEXCOORD0],12);
        else if(g->source==GX_TG_COLOR0 || g->source==GX_TG_COLOR1){source[0]=lit[g->source-GX_TG_COLOR0][0];source[1]=lit[g->source-GX_TG_COLOR0][1];}
        unsigned index=p->texture_offset[i]==~0u?g->matrix:packet[p->texture_offset[i]];
        if(index==GX_IDENTITY || g->type==GX_TG_SRTG)fh_copy_bytes(result,source,12);
        else {
            const float (*mtx)[4]=prepared->matrix;
            if(prepared->dynamic){
                if(index%3 || index/3>=64)return 0;
                mtx=index<GX_TEXMTX0?fh_gx.position[index/3]:fh_gx.texture[index/3];
            }
            transform(result,mtx,source,1);
        }
        if(g->type==GX_TG_MTX2x4 || g->type==GX_TG_SRTG)result[2]=1;
        if(g->normalize)normalize(result);
        if(prepared->post){float v[3];transform(v,prepared->post,result,1);fh_copy_bytes(result,v,12);}
    }
    *out=(FhNativeVertex){0};fh_copy_bytes(out->position,position,12);out->position[3]=1;
    out->matrix_index=p->matrix_offset==~0u?0:matrix;
    for(unsigned c=0;c<4;c++)out->color[c]=pack->affine?lit[0][c]:pack->primary[c]<0?0:((const float*)lit)[pack->primary[c]];
    if(pack->sphere)for(unsigned j=0;j<2;j++) {
        generated[6+j][0]=(.5f+15*lit[1][j*2])/16;
        generated[6+j][1]=(.5f+15*lit[1][j*2+1])/16;
        generated[6+j][2]=1;
    }
    for(unsigned u=0;u<pack->units;u++) {
        const float *uv=generated[pack->coord[u]];float q=uv[2];if(fabsf(q)<1e-20f)q=1;
        float s=uv[0],t=uv[1];if(u){s/=q;t/=q;}
        out->uv[u][0]=s*pack->scale[u][0];out->uv[u][1]=(u?1:q)-t*pack->scale[u][1];out->uv[u][2]=u?1:q;
    }
    if(pack->affine){fh_copy_bytes(out->uv[1],lit[1],12);out->uv[2][0]=lit[1][3];}
    return 1;
}
