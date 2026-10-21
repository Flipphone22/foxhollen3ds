/* Included by the scene owner. CPU-only records borrow their owning scene's
 * immutable index ranges; they cannot outlive that scene or publish GPU work. */
enum { FH_DEFERRED_PLAN_LIMIT=64*1024 };
static unsigned deferred_bytes,deferred_peak,deferred_builds,deferred_rejected;
static unsigned deferred_be32(const unsigned char *p){
    return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];
}
static void release_world_deferred(SceneRecord *r){
    if(!r->deferred)return;
    unsigned bytes=r->deferred->bytes;
    deferred_bytes-=bytes;fh_native_scene_cpu_bytes-=bytes;
    free(r->deferred);r->deferred=NULL;
}
static void prepare_world_deferred(SceneRecord *r,const void *raw,unsigned bytes){
    if(!r->asset.block || r->deferred)return;
    unsigned counts[2]={fh_pica_world_deferred_compile(&r->asset,1,NULL,0),
        fh_pica_world_deferred_compile(&r->asset,2,NULL,0)};
    unsigned count=counts[0]+counts[1];
    if(!count)return;
    if(!raw || bytes<164 || count>256){deferred_rejected++;return;}
    const unsigned char *b=raw;unsigned at=deferred_be32(b+0x68),lists=b[0xa1];
    if(at>bytes || lists>(bytes-at)/28){deferred_rejected++;return;}
    unsigned needed=sizeof(FhPicaWorldDeferredPlan)+count*sizeof(FhPicaWorldDeferredEntry);
    if(needed>FH_DEFERRED_PLAN_LIMIT-deferred_bytes || !normal_reserve(needed)){
        deferred_rejected++;return;
    }
    FhPicaWorldDeferredPlan *p=malloc(needed);
    if(!p){deferred_rejected++;return;}
    p->bytes=needed;p->count[0]=counts[0];p->count[1]=counts[1];
    FhPicaWorldDeferred scratch[256];unsigned cursor=0;
    for(unsigned stream=1;stream<=2;stream++){
        unsigned n=counts[stream-1];
        if(fh_pica_world_deferred_compile(&r->asset,stream,scratch,n)!=n)goto reject;
        for(unsigned i=0;i<n;i++){
            FhPicaWorldDeferredEntry *e=p->entries+cursor++;
            e->draw=scratch[i];
            if(e->draw.material>=b[0xa2] || !fh_pica_world_sort_bounds(&e->bounds,b+at,lists*28,e->draw.list))goto reject;
            FhSceneGroup g;FhSceneSection section;
            if(!fh_native_scene_group(&r->asset,e->draw.group,&g) ||
               !fh_native_scene_section(&r->asset,e->draw.section,&section))goto reject;
            e->geometry=(FhPicaWorldDraw){g.vertex,g.index+e->draw.first,e->draw.count,
                g.vertices,g.format,g.cull,e->draw.material,section.shader_flags};
        }
    }
    r->deferred=p;deferred_bytes+=needed;fh_native_scene_cpu_bytes+=needed;
    if(deferred_bytes>deferred_peak)deferred_peak=deferred_bytes;
    deferred_builds++;return;
reject:
    free(p);deferred_rejected++;
}
