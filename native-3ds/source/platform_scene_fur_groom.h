/* Included by the scene owner; same publication and retirement as its mesh.
 * Per-frame code only indexes prepared descriptors and publishes parameters. */
static unsigned groom_gpu,groom_cpu,groom_peak,groom_builds,groom_declined;
static void release_fur_groom(FhPicaFurGroomPlan*p){
    if(!p)return;
    unsigned cpu=sizeof(*p)+p->count*sizeof(*p->draw);
    groom_gpu-=p->bytes;groom_cpu-=cpu;
    fh_native_scene_gpu_bytes-=p->bytes;fh_native_scene_cpu_bytes-=cpu;
    linearFree(p->gpu);free(p);
}
static void prepare_fur_groom(SceneRecord*r,unsigned material,const FhTexture*texture){
    if(!r || r->invalid || !r->fur_plan || !r->actor_plan)return;
    FhPicaFurGroomPlan*p=r->fur_plan->groom;
    unsigned count=r->actor_plan->count,bytes=0,admitted=0;
    for(unsigned i=0;i<count;i++){
        const FhPicaModelDraw*d=r->actor_plan->draw+i;
        if(!(d->flags&512))continue;
        if(d->material==material){
            if(!fh_pica_fur_groom_compile(&r->asset,d,texture,NULL,0)){groom_declined++;return;}
            if(p && p->draw[i].ready)return; /* source is immutable; no overwrite */
            admitted++;
        }
        unsigned n=d->vertices*sizeof(FhPicaFurGroomVertex);
        if(n>65536 || bytes>65536-((n+127)&~127u)){groom_declined++;return;}
        bytes+=(n+127)&~127u;
    }
    if(!admitted)return;
    if(!p){
        unsigned cpu=sizeof(*p)+count*sizeof(*p->draw);
        if(bytes>65536-groom_gpu || cpu>16384-groom_cpu || !normal_reserve(cpu) ||
           bytes>SCENE_GPU_LIMIT-fh_native_scene_gpu_bytes || !fh_minimal_linear_room(bytes) ||
           linearSpaceFree()<bytes+SCENE_LINEAR_RESERVE){groom_declined++;return;}
        p=calloc(1,cpu);void*gpu=linearMemAlign(bytes,128);
        if(!p || !gpu){free(p);linearFree(gpu);groom_declined++;return;}
        memset(gpu,0,bytes);fh_gpu_cache_flush(gpu,bytes);
        p->count=count;p->bytes=bytes;p->gpu=gpu;unsigned at=0;
        for(unsigned i=0;i<count;i++){
            const FhPicaModelDraw*d=r->actor_plan->draw+i;p->draw[i].offset=at;
            if(d->flags&512)at+=(d->vertices*sizeof(FhPicaFurGroomVertex)+127)&~127u;
        }
        r->fur_plan->groom=p;groom_cpu+=cpu;groom_gpu+=bytes;
        fh_native_scene_cpu_bytes+=cpu;fh_native_scene_gpu_bytes+=bytes;
        if(groom_gpu>groom_peak)groom_peak=groom_gpu;
    }
    for(unsigned i=0;i<count;i++){
        const FhPicaModelDraw*d=r->actor_plan->draw+i;
        if(!(d->flags&512) || d->material!=material)continue;
        void*dst=(unsigned char*)p->gpu+p->draw[i].offset;
        if(fh_pica_fur_groom_compile(&r->asset,d,texture,dst,d->vertices)!=d->vertices)
            fh_platform_panic("PICA fur groom publication");
        fh_gpu_cache_flush(dst,d->vertices*sizeof(FhPicaFurGroomVertex));
        p->draw[i].ready=1;groom_builds++;
    }
}
