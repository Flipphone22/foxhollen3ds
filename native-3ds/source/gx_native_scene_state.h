/* Atlas geometry has already resolved vertex formats, texture matrices and
 * joint bindings. Re-running the display-list decoder's shading preparation
 * here both duplicates work and consults irrelevant legacy descriptors. Build
 * the native diffuse contract directly from the live game channels/lights. */
static int scene_model_state(FhNativeModelDraw *draw){
    draw->state=fh_gx.draw;draw->state.raw_position=1;
    memcpy(draw->state.position_matrix,fh_gx.projection,sizeof(fh_gx.projection));
    if(!draw->direct_uv)draw->coordinates[0]=1;
    else for(unsigned i=0;i<fh_gx.draw.texgens && i<8;i++){
        const FhTexGen *g=fh_gx.texgens+i;
        if(g->type!=GX_TG_MTX2x4 || g->source!=GX_TG_TEX0 || g->normalize ||
           fh_gx.descriptors[GX_VA_TEX0MTXIDX+i])continue;
        float *t=draw->uv_transform[i];t[0]=t[1]=1;
        const float (*m)[4]=NULL;
        if(g->matrix!=GX_IDENTITY){
            if(g->matrix%3 || g->matrix>GX_TEXMTX9)continue;
            m=g->matrix<GX_TEXMTX0?fh_gx.position[g->matrix/3]:fh_gx.texture[g->matrix/3];
            if(m[0][1] || m[0][2] || m[1][0] || m[1][2])continue;
            t[0]=m[0][0];t[1]=m[1][1];t[2]=m[0][3];t[3]=m[1][3];
        }
        if(g->postmatrix!=GX_PTIDENTITY){
            if(g->postmatrix<GX_PTTEXMTX0 || g->postmatrix>GX_PTTEXMTX19 || (g->postmatrix-GX_PTTEXMTX0)%3)continue;
            m=fh_gx.post[(g->postmatrix-GX_PTTEXMTX0)/3];
            if(m[0][1] || m[1][0] || m[2][0] || m[2][1] || m[2][2]+m[2][3]!=1)continue;
            t[2]=m[0][0]*t[2]+m[0][2]+m[0][3];t[3]=m[1][1]*t[3]+m[1][2]+m[1][3];
            t[0]*=m[0][0];t[1]*=m[1][1];
        }
        if(!isfinite(t[0]) || !isfinite(t[1]) || !isfinite(t[2]) || !isfinite(t[3]))continue;
        draw->coordinates[i]=1;
    }
    draw->post[0][0]=draw->post[1][1]=draw->post[2][3]=1;
    const FhChannel *rgb=&fh_gx.channel[0],*alpha=&fh_gx.channel[2];
    if(alpha->enabled && (alpha->ambient_source!=GX_SRC_REG ||
       alpha->diffuse!=GX_DF_NONE || alpha->attenuation!=GX_AF_NONE))return 0;
    for(unsigned c=0;c<4;c++){
        const FhChannel *ch=c==3?alpha:rgb;
        draw->color_from_vertex[c]=ch->material_source==GX_SRC_VTX;
        draw->color[c]=draw->color_from_vertex[c]?1.f:fh_gx.material[0][c];
    }
    if(alpha->enabled){
        float sum=fh_gx.ambient[0][3];
        for(unsigned i=0;i<8;i++)if(alpha->mask&(1u<<i))sum+=fh_gx.lights[i].color[3];
        draw->color[3]*=clamp(sum);
    }
    if(!rgb->enabled)return 1;
    if(rgb->ambient_source!=GX_SRC_REG)return 0;
    if(!rgb->mask || (rgb->diffuse==GX_DF_NONE && rgb->attenuation==GX_AF_NONE)){
        for(unsigned c=0;c<3;c++){
            float sum=fh_gx.ambient[0][c];
            for(unsigned i=0;i<8;i++)if(rgb->mask&(1u<<i))sum+=fh_gx.lights[i].color[c];
            draw->color[c]*=clamp(sum);
        }
        return 1;
    }
    if(!draw->source.normal_count || rgb->diffuse!=GX_DF_CLAMP ||
       (rgb->attenuation!=GX_AF_SPOT && rgb->attenuation!=GX_AF_NONE))return 0;
    FhGpuLighting *light=&draw->lighting;light->mask=rgb->mask;
    memcpy(light->ambient,fh_gx.ambient[0],sizeof(light->ambient));
    for(unsigned i=0;i<8;i++)if(rgb->mask&(1u<<i)){
        const FhLight *src=&fh_gx.lights[i];float (*dst)[4]=light->lights[i];
        memcpy(dst[0],src->position,12);memcpy(dst[1],src->direction,12);memcpy(dst[2],src->color,16);
        if(rgb->attenuation==GX_AF_SPOT){memcpy(dst[3],src->a,12);memcpy(dst[4],src->k,12);}
        else {dst[3][0]=dst[4][0]=1;}
        for(unsigned r=0;r<5;r++)for(unsigned c=0;c<4;c++)
            if(!isfinite(dst[r][c]) || fabsf(dst[r][c])>(r==0?1e7f:100.f))return 0;
    }
    if(__builtin_popcount(rgb->mask&255)>2){
        /* The scene visual tier retains two spatial lights and includes the
         * rest as live diffuse fill, rather than discarding light work/cues. */
        float center[3]={0};
        for(unsigned b=0;b<draw->group.bones;b++)for(unsigned c=0;c<3;c++)center[c]+=draw->palette[b][c][3]/draw->group.bones;
        if(!fh_light_lod_reduce(light,center))return 0;
    }else{
        unsigned n=0;
        for(unsigned i=0;i<8;i++)if(rgb->mask&(1u<<i)){
            if(n!=i)memcpy(light->lights[n],light->lights[i],sizeof(light->lights[0]));n++;
        }
        light->mask=(1u<<n)-1;
    }
    draw->lit=1;draw->state.raw_position|=2;return 1;
}
