/* Included by the scene owner after collection admission. Single load-thread
 * ownership; index/FILE are never retained by GPU resident packet storage. */
static FhActorFogCollection actor_fog_archive;
static char actor_fog_archive_path[256];
static unsigned actor_fog_archive_attempted,actor_fog_archive_bytes;
static void close_actor_fog_archive(void){
    FILE*f=actor_fog_archive.file;
    fh_actor_fog_collection_close(&actor_fog_archive);
    if(f)fclose(f);
    fh_native_scene_cpu_bytes-=actor_fog_archive_bytes;
    actor_fog_archive_bytes=actor_fog_archive_attempted=0;
    actor_fog_archive_path[0]=0;
}
int fh_native_scene_actor_fog_archive(const FhSceneAsset*a,const char*path,unsigned bump){
    if(!actor_fog_record(a)||!path||bump>1||strlen(path)>=sizeof(actor_fog_archive_path))return -1;
    if(actor_fog_archive_attempted){
        if(strcmp(path,actor_fog_archive_path))return -1;
        if(!actor_fog_archive.file)return 0;
    }else{
        actor_fog_archive_attempted=1;strcpy(actor_fog_archive_path,path);
        FILE*f=fopen(path,"rb");if(!f)return 0;
        /* realloc can temporarily retain both old and new allocations. Keep
         * room for the maximum bounded index plus its predecessor, a stdio
         * buffer and nested load-stack use before allowing startup indexing. */
        unsigned reserve=FH_ACTOR_FOG_COLLECTION_RECORDS*sizeof(FhActorFogCollectionEntry)*3/2+16384;
        if(!normal_reserve(reserve)||!fh_actor_fog_collection_open(&actor_fog_archive,f)){
            fclose(f);return -1;
        }
        actor_fog_archive_bytes=actor_fog_archive.capacity*sizeof(FhActorFogCollectionEntry);
        fh_native_scene_cpu_bytes+=actor_fog_archive_bytes;
        char line[160];snprintf(line,sizeof(line),"Actor fog collection: entries=%u normal=%u file_bytes=%u reserve=%u\n",
            actor_fog_archive.count,actor_fog_archive_bytes,actor_fog_archive.bytes,reserve);fh_platform_log(line);
    }
    return fh_native_scene_actor_fog_collection(a,&actor_fog_archive,bump);
}
