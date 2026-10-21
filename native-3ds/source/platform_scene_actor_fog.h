/* Load-time material-generation owner. Existing scene join retires every
 * resident packet before its parent model is freed; VBlank is not a fence. */
#include "fh_actor_fog_bundle.h"
struct FhSceneActorFog {
    FhPicaActorFogVariants plan;
    unsigned generation,bytes,published;
    FhActorFogResident slots[];
};
static void release_actor_fog(SceneRecord*r){
    FhSceneActorFog*fog=r->actor_fog;if(!fog)return;
    fh_actor_fog_resident_release_many(fog->slots,fog->plan.count);
    fh_native_scene_cpu_bytes-=fog->bytes;
    free(fog);r->actor_fog=NULL;
}
static SceneRecord *actor_fog_record(const FhSceneAsset*a){
    SceneRecord*r=record(a);
    return fh_pica_scene_actors && r && !r->invalid && !r->actor_fog_dirty &&
        !r->active && !a->block && a->compact && a->gpu==r->gpu?r:NULL;
}
int fh_native_scene_actor_fog_prepare(const FhSceneAsset*a,
    const FhPicaActorFogVariants*plan,unsigned generation){
    SceneRecord*r=actor_fog_record(a);
    if(!r||!plan||!generation||plan->reserved||!plan->count||plan->count>FH_ACTOR_FOG_VARIANT_CELLS)return 0;
    for(unsigned i=0;i<FH_ACTOR_FOG_VARIANT_CELLS;i++)if(plan->slots[i]>plan->count)return 0;
    for(unsigned i=1;i<=plan->count;i++){
        FhPicaActorFogSelection selection;
        if(!fh_pica_actor_fog_variant_selection(&selection,plan,i))return 0;
    }
    if(r->actor_fog && r->actor_fog->generation==generation)
        return !memcmp(&r->actor_fog->plan,plan,sizeof(*plan));
    unsigned bytes=sizeof(FhSceneActorFog)+plan->count*sizeof(FhActorFogResident);
    if(!normal_reserve(bytes))return 0;
    FhSceneActorFog*fog=malloc(bytes);if(!fog)return 0;
    memset(fog,0,bytes);fog->plan=*plan;fog->generation=generation;fog->bytes=bytes;
    fh_native_scene_cpu_bytes+=bytes;
    /* A new generation invalidates old alternatives. Failed allocation above
     * retains them under their original generation, not the requested one. */
    release_actor_fog(r);r->actor_fog=fog;return 1;
}
int fh_native_scene_actor_fog_publish(const FhSceneAsset*a,unsigned generation,
    const void*metadata,unsigned cpu,const void*geometry,unsigned gpu,unsigned variant){
    SceneRecord*r=actor_fog_record(a);FhSceneActorFog*fog=r?r->actor_fog:NULL;
    if(!fog||fog->generation!=generation||!variant||variant>fog->plan.count)return 0;
    int ok=fh_actor_fog_resident_publish(fog->slots+variant-1,metadata,cpu,geometry,gpu,
        a->id,word(a->meta+16),word(a->meta+20),variant);
    if(ok)fog->published=1;
    return ok;
}
unsigned fh_native_scene_actor_fog_generation(const FhSceneAsset*a){
    SceneRecord*r=actor_fog_record(a);FhSceneActorFog*fog=r?r->actor_fog:NULL;
    return fog&&fog->published?fog->generation:0;
}
int fh_native_scene_actor_fog_load(const FhSceneAsset*a,unsigned generation,
    unsigned cpu,unsigned gpu,unsigned crc,FhActorFogRead read,void*context,unsigned variant){
    SceneRecord*r=actor_fog_record(a);FhSceneActorFog*fog=r?r->actor_fog:NULL;
    if(!fog||fog->generation!=generation||!variant||variant>fog->plan.count)return 0;
    int ok=fh_actor_fog_resident_load(fog->slots+variant-1,cpu,gpu,crc,read,context,
        a->id,word(a->meta+16),word(a->meta+20),variant);
    if(ok)fog->published=1;
    return ok;
}
int fh_native_scene_actor_fog_file(const FhSceneAsset*a,const char*path,unsigned bump){
    SceneRecord*r=actor_fog_record(a);
    if(!r||!path||bump>1||r->actor_fog)return -1;
    FILE*f=fopen(path,"rb");if(!f)return 0;
    FhActorFogBundle bundle;int loaded=-1;
    if(fh_actor_fog_bundle_open(&bundle,f,a->id,word(a->meta+16),word(a->meta+20),bump)){
        loaded=0;
        if(bundle.plan.count&&fh_native_scene_actor_fog_prepare(a,&bundle.plan,bump+1)){
            for(unsigned i=0;i<bundle.plan.count;i++){
                FhActorFogBundleRead read={f,bundle.entries[i]};
                loaded+=fh_native_scene_actor_fog_load(a,bump+1,read.entry.metadata,
                    read.entry.geometry,read.entry.crc,fh_actor_fog_bundle_read,&read,i+1)!=0;
            }
        }
    }
    fclose(f);return loaded;
}
int fh_native_scene_actor_fog_collection(const FhSceneAsset*a,
    const FhActorFogCollection*c,unsigned bump){
    SceneRecord*r=actor_fog_record(a);
    if(!r||!c||bump>1||r->actor_fog)return -1;
    const FhActorFogCollectionEntry*entry=fh_actor_fog_collection_find(c,
        a->id,word(a->meta+16),word(a->meta+20),bump);
    if(!entry)return 0;
    FhActorFogBundle bundle;
    if(!fh_actor_fog_collection_bundle(&bundle,c,entry))return -1;
    int loaded=0;
    if(bundle.plan.count&&fh_native_scene_actor_fog_prepare(a,&bundle.plan,bump+1)){
        for(unsigned i=0;i<bundle.plan.count;i++){
            FhActorFogCollectionRead read={c->file,bundle.entries[i],entry->start,entry->end};
            loaded+=fh_native_scene_actor_fog_load(a,bump+1,read.entry.metadata,
                read.entry.geometry,read.entry.crc,fh_actor_fog_collection_read,&read,i+1)!=0;
        }
    }
    return loaded;
}
const FhActorFogAsset *fh_native_scene_actor_fog_find(const FhSceneAsset*a,unsigned generation,unsigned variant){
    SceneRecord*r=actor_fog_record(a);FhSceneActorFog*fog=r?r->actor_fog:NULL;
    if(!fog||fog->generation!=generation||!variant||variant>fog->plan.count)return NULL;
    return fh_actor_fog_resident_find(fog->slots+variant-1,a->id,word(a->meta+16),word(a->meta+20),variant);
}
const FhActorFogAsset *fh_native_scene_actor_fog_select(const FhSceneAsset*a,
    unsigned generation,const FhPicaActorFogSelection*selection){
    SceneRecord*r=actor_fog_record(a);FhSceneActorFog*fog=r?r->actor_fog:NULL;
    if(!fog||fog->generation!=generation)return NULL;
    unsigned variant=fh_pica_actor_fog_variant(&fog->plan,selection);
    return variant?fh_actor_fog_resident_find(fog->slots+variant-1,a->id,
        word(a->meta+16),word(a->meta+20),variant):NULL;
}
