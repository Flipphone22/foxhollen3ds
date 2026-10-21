/* Included after the dense retained-key oracle. The common header describes
 * every conditional payload, so equal byte strings have equal dependencies.
 * Only ordinary CPU arena storage changes; GPU ownership is unchanged. */
static void key_append(Key *key,unsigned *used,const void *data,unsigned bytes){
    fh_gx_require(*used<=sizeof(*key) && bytes<=sizeof(*key)-*used,"Compact geometry key overflow");
    memcpy((unsigned char*)key+*used,data,bytes);*used+=bytes;
}
static unsigned encode_dense_key(Key *out,const Key *in){
    unsigned used=offsetof(Key,descriptors);memcpy(out,in,used);
    key_append(out,&used,in->descriptors,sizeof(in->descriptors));
    for(unsigned a=0;a<26;a++)if(in->descriptors[a]){
        key_append(out,&used,in->formats+a,sizeof(FhVtxFormat));
        if(a>=GX_VA_POS && in->descriptors[a]>=GX_INDEX8)
            key_append(out,&used,&in->arrays[a].bytes,12);
    }
    for(unsigned i=0;i<4;i++)if(in->raster&(1u<<(i&1))){
        const FhChannel *c=in->channel+i;unsigned channel=i&1,first=i>=2?3:0,end=i>=2?4:3;
        key_append(out,&used,c,sizeof(*c));
        if((in->raw&64) || (i==0 && (in->raw&2)) || (i==2 && (in->raw&32)))continue;
        if(c->material_source==GX_SRC_REG)key_append(out,&used,in->material[channel]+first,(end-first)*4);
        if(!c->enabled)continue;
        if(c->ambient_source==GX_SRC_REG)key_append(out,&used,in->ambient[channel]+first,(end-first)*4);
        for(unsigned n=0;n<8;n++)if(c->mask&(1u<<n))key_append(out,&used,in->light_colors[n]+first,(end-first)*4);
    }
    for(unsigned i=0;i<in->texgens;i++){
        if(!(in->texgen_inputs&(1u<<i)))continue;
        const FhTexGen *g=in->gen+i;key_append(out,&used,g,sizeof(*g));
        if(!i && (in->raw&4))continue;
        if(g->matrix!=GX_IDENTITY && g->type!=GX_TG_SRTG)key_append(out,&used,in->matrix[i],48);
        if(g->postmatrix!=GX_PTIDENTITY && !(in->fur_post && i==1))key_append(out,&used,in->post[i],48);
    }
    return used;
}
static unsigned make_compact_key(Key *k,unsigned format,unsigned raster){
    if(fh_gx.draw.texgens>8)return 0;
    unsigned used=offsetof(Key,descriptors);memset(k,0,used);
    k->raw=fh_gx.draw.raw_position;k->raster=raster;k->texgens=fh_gx_active_texgens;k->compiled=compiled_record;
    k->texgen_inputs=fh_gx_texgen_inputs;
    k->fur_post=compiled_record && fh_compiled_fur_post()!=NULL;
#ifdef __arm__
    __asm__ volatile("vmrs %0, fpscr":"=r"(k->fpscr));k->fpscr&=0x03c00000u;
#endif
    for(unsigned a=GX_VA_POS;a<=GX_VA_TEX7;a++)if(fh_gx.descriptors[a]>=GX_INDEX8)k->source_mask|=1u<<(a-GX_VA_POS);
    k->flat=(k->raw&2) && (fh_renderer_prepared_lighting()->mask&FH_LIGHT_FLAT);
    if(k->flat){
        unsigned consumed=1u;const FhChannel *rgb=&fh_gx.channel[0],*alpha=&fh_gx.channel[2];
        if(rgb->material_source==GX_SRC_VTX || alpha->material_source==GX_SRC_VTX ||
           (alpha->enabled && alpha->ambient_source==GX_SRC_VTX))consumed|=1u<<(GX_VA_CLR0-GX_VA_POS);
        for(unsigned i=0;i<k->texgens;i++){
            if(!(k->texgen_inputs&(1u<<i)))continue;
            unsigned source=fh_gx.texgens[i].source;
            if(source==GX_TG_POS)continue;
            if(source<GX_TG_TEX0 || source>GX_TG_TEX7)return 0;
            consumed|=1u<<(GX_VA_TEX0+source-GX_TG_TEX0-GX_VA_POS);
        }
        k->source_mask&=consumed;
    }
    key_append(k,&used,fh_gx.descriptors,sizeof(fh_gx.descriptors));
    for(unsigned a=0;a<26;a++)if(fh_gx.descriptors[a]){
        key_append(k,&used,fh_gx.formats[format]+a,sizeof(FhVtxFormat));
        if(a>=GX_VA_POS && fh_gx.descriptors[a]>=GX_INDEX8)key_append(k,&used,&fh_gx.arrays[a].bytes,12);
    }
    for(unsigned i=0;i<4;i++)if(raster&(1u<<(i&1))){
        const FhChannel *src=fh_gx.channel+i;FhChannel dst={0};dst.material_source=src->material_source;
        unsigned channel=i&1,first=i>=2?3:0,end=i>=2?4:3;
        unsigned gpu=(k->raw&64) || (i==0 && (k->raw&2)) || (i==2 && (k->raw&32));
        if(!gpu && src->enabled){
            dst.enabled=src->enabled;dst.ambient_source=src->ambient_source;dst.mask=src->mask;
            if(src->mask){dst.diffuse=src->diffuse;dst.attenuation=src->attenuation;}
        }
        key_append(k,&used,&dst,sizeof(dst));
        if(gpu)continue;
        if(src->material_source==GX_SRC_REG)key_append(k,&used,fh_gx.material[channel]+first,(end-first)*4);
        if(!src->enabled)continue;
        if(src->ambient_source==GX_SRC_REG)key_append(k,&used,fh_gx.ambient[channel]+first,(end-first)*4);
        for(unsigned n=0;n<8;n++)if(src->mask&(1u<<n))key_append(k,&used,fh_gx.lights[n].color+first,(end-first)*4);
    }
    for(unsigned i=0;i<k->texgens;i++){
        if(!(k->texgen_inputs&(1u<<i)))continue;
        const FhTexGen *g=fh_gx.texgens+i;key_append(k,&used,g,sizeof(*g));
        if(!i && (k->raw&4))continue;
        if(fh_gx.descriptors[GX_VA_TEX0MTXIDX+i] || (g->type>=GX_TG_BUMP0 && g->type<=GX_TG_BUMP7))return 0;
        if(g->matrix!=GX_IDENTITY && g->type!=GX_TG_SRTG)
            key_append(k,&used,g->matrix<GX_TEXMTX0?fh_gx.position[g->matrix/3]:fh_gx.texture[g->matrix/3],48);
        if(g->postmatrix!=GX_PTIDENTITY && !(k->fur_post && i==1))key_append(k,&used,fh_gx.post[(g->postmatrix-GX_PTTEXMTX0)/3],48);
    }
    return used;
}
static __attribute__((noinline)) void check_compact_key(const Key *key,unsigned bytes,unsigned format,unsigned raster){
    Key dense,encoded;unsigned valid=make_key(&dense,format,raster);
    fh_gx_require(!!valid==!!bytes,"Compact geometry key eligibility mismatch");
    if(!valid)return;
    unsigned expected=encode_dense_key(&encoded,&dense);
    fh_gx_require(expected==bytes && !fh_memory_compare(key,&encoded,bytes),"Compact geometry key dependency mismatch");
    ++fh_retained_key_checks;
}
static unsigned make_stored_key(Key *key,unsigned format,unsigned raster){
    if(!fh_retained_key_mode)return make_key(key,format,raster)?sizeof(*key):0;
    unsigned bytes=make_compact_key(key,format,raster);
    if(fh_retained_key_mode==2)check_compact_key(key,bytes,format,raster);
    if(bytes){++fh_retained_key_calls;fh_retained_key_bytes+=bytes;fh_retained_key_dense_bytes+=sizeof(*key);}
    return bytes;
}
