#include "fh_native_model_asset.h"
#include "fh_platform.h"
#include "fh_model_resources.h"
#include "fh_memory_bytes.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <zlib.h>
enum { MODEL_OWNERS=64,MODEL_CPU_BUDGET=256*1024,MODEL_PACK_LIMIT=8*1024*1024 };
typedef struct {const void *owner;void *storage;FhModelAsset asset;const void *geometry;const void **sources;
    unsigned geometry_bytes,version,allocation_bytes,invalid,sealed;} Record;
static Record records[MODEL_OWNERS];
static FILE *pack;
static unsigned char *directory;
static unsigned initialized,entries,pack_bytes;
static unsigned lookup_count,lookup_hits;
unsigned fh_native_models_enabled,fh_native_models_loaded,fh_native_models_rejected,fh_native_models_bytes;
static unsigned word(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static int reserve(unsigned n){
    extern uint32_t __ctru_heap_size,__stacksize__;
    return n<=MODEL_CPU_BUDGET-fh_native_models_bytes &&
        ((uint64_t)(unsigned)mallinfo().uordblks+__stacksize__+n+64)*100<=(uint64_t)__ctru_heap_size*85;
}
void fh_native_model_shutdown(void){
    if(fh_native_models_enabled){
        extern unsigned fh_native_model_draws,fh_native_model_lists,fh_native_model_vertices_drawn,fh_native_model_failures;
        char message[256];snprintf(message,sizeof(message),"Native models: loaded=%u rejected=%u bytes=%u draws=%u lists=%u vertices=%u failures=%u\n",
            fh_native_models_loaded,fh_native_models_rejected,fh_native_models_bytes,fh_native_model_draws,
            fh_native_model_lists,fh_native_model_vertices_drawn,fh_native_model_failures);
        fh_platform_log(message);
        extern unsigned fh_native_model_admission[16];
        unsigned at=snprintf(message,sizeof(message),"Native model lookup: calls=%u hits=%u; admission:",lookup_count,lookup_hits);
        for(unsigned i=0;i<16 && at<sizeof(message)-1;i++)at+=snprintf(message+at,sizeof(message)-at," %u",fh_native_model_admission[i]);
        message[sizeof(message)-2]='\0';strncat(message,"\n",sizeof(message)-strlen(message)-1);fh_platform_log(message);
    }
    for(unsigned i=0;i<MODEL_OWNERS;i++){free(records[i].storage);records[i]=(Record){0};}
    free(directory);directory=NULL;if(pack)fclose(pack);pack=NULL;
    fh_native_models_bytes=fh_native_models_enabled=entries=pack_bytes=initialized=0;
}
static void initialize(void){
    initialized=1;pack=fopen("sdmc:/3ds/foxhollow/native-models.bin","rb");if(!pack)return;
    unsigned char header[16];long bytes;
    if(fseek(pack,0,SEEK_END) || (bytes=ftell(pack))<16 || bytes>MODEL_PACK_LIMIT ||
       fseek(pack,0,SEEK_SET) || fread(header,1,16,pack)!=16 || memcmp(header,"FHNMSET1",8) ||
       !(entries=word(header+8)) || entries>1024 || entries*32> (unsigned)bytes-16 || !reserve(entries*32))goto bad;
    pack_bytes=(unsigned)bytes;directory=malloc(entries*32);
    if(!directory || fread(directory,32,entries,pack)!=entries || (unsigned)crc32(0,directory,entries*32)!=word(header+12))goto bad;
    unsigned previous_end=16+entries*32;
    for(unsigned i=0;i<entries;i++){
        const unsigned char *e=directory+i*32;unsigned at=word(e+12),n=word(e+16);
        if(word(e+28) || at<previous_end || at&3 || at>pack_bytes || n<64 || n>MODEL_CPU_BUDGET || n>pack_bytes-at)goto bad;
        previous_end=at+n;
    }
    fh_native_models_bytes=entries*32;fh_native_models_enabled=1;
    fh_platform_log("Native model companion opened; runtime draw admission still required\n");return;
bad:
    free(directory);directory=NULL;fclose(pack);pack=NULL;entries=pack_bytes=0;fh_native_models_rejected++;
    fh_platform_log("Native model companion rejected; original rendering retained\n");
}
void fh_native_model_forget(const void *allocation,unsigned bytes){
    uintptr_t at=(uintptr_t)allocation;
    for(unsigned i=0;i<MODEL_OWNERS;i++){
        uintptr_t owner=(uintptr_t)records[i].owner;
        if(owner && owner>=at && owner-at<bytes){
            fh_native_models_bytes-=records[i].allocation_bytes;free(records[i].storage);records[i]=(Record){0};
        }
    }
}
const FhModelAsset *fh_native_model_find(const void *owner){
    if(!owner || !fh_native_models_enabled)return NULL;
    lookup_count++;
    for(unsigned i=0;i<MODEL_OWNERS;i++)if(records[i].owner==owner){
        Record *r=records+i;
        if(!r->sealed || r->invalid)return NULL;
        unsigned version=fh_model_resource_version(r->geometry,r->geometry_bytes);
        if(!version || version!=r->version){
            /* Poses and attribute writers can advance the coarse file-owner
             * generation without changing render topology. Revalidate the
             * exact original instruction/list bytes before refreshing it.
             * Escaped sources get a fresh byte comparison on EVERY lookup,
             * never generation-based reuse. The game allocation remains live
             * until the registered release hook; vertices are copied now,
             * rather than retained through an escaped game pointer. */
            for(unsigned j=0;j<r->asset.topology_count;j++){
                unsigned n;const void *snapshot=fh_native_model_asset_topology(&r->asset,j,&n);
                if(fh_memory_compare(snapshot,r->sources[j],n)){r->invalid=1;return NULL;}
            }
            r->version=version;
        }
        lookup_hits++;return &r->asset;
    }
    return NULL;
}
int fh_native_model_source_matches(const FhModelAsset *asset,unsigned index,const void *source,unsigned bytes){
    /* Exact old bytes are not sufficient if a live header rebound its list or
     * instruction pointer. Require the same allocation slice as the seal. */
    if(!asset || index>=asset->topology_count)return 0;
    for(unsigned i=0;i<MODEL_OWNERS;i++)if(&records[i].asset==asset){
        Record *r=records+i;unsigned expected;
        if(!r->sealed || r->invalid)return 0;
        if(!fh_native_model_asset_topology(asset,index,&expected) || expected!=bytes || r->sources[index]!=source){r->invalid=1;return 0;}
        return 1;
    }
    return 0;
}
void fh_native_model_seal(const void *owner,const FhModelResourceRange *ranges,unsigned lists,const void *instructions,unsigned instruction_bytes){
    for(unsigned i=0;i<MODEL_OWNERS;i++)if(records[i].owner==owner){
        Record *r=records+i;
        if(!ranges || !instructions || r->asset.topology_count!=lists+1){r->invalid=1;fh_platform_log("Native model topology count differs at seal\n");return;}
        for(unsigned j=0;j<r->asset.topology_count;j++){
            unsigned n;const void *snapshot=fh_native_model_asset_topology(&r->asset,j,&n);
            const void *source=j?ranges[j+3].data:instructions;
            unsigned bytes=j?ranges[j+3].bytes:instruction_bytes;
            if(n!=bytes || !source || memcmp(snapshot,source,n)){
                char message[160];snprintf(message,sizeof(message),"Native model %u topology seal rejected at %u: expected=%u actual=%u\n",r->asset.id,j,n,bytes);
                fh_platform_log(message);r->invalid=1;return;
            }
            r->sources[j]=source;
        }
        r->geometry=ranges[0].data;r->geometry_bytes=ranges[0].bytes;
        r->version=fh_model_resource_version(r->geometry,r->geometry_bytes);r->sealed=1;return;
    }
}
void fh_native_model_register(const void *owner,const void *raw,unsigned bytes,unsigned id){
    if(!owner || !raw || bytes<252 || bytes>4*1024*1024)return;
    if(!initialized)initialize();
    if(!pack)return;
    for(unsigned i=0;i<MODEL_OWNERS;i++)if(records[i].owner==owner)return;
    unsigned slot=MODEL_OWNERS;
    for(unsigned i=0;i<MODEL_OWNERS;i++)if(!records[i].owner){slot=i;break;}
    if(slot==MODEL_OWNERS){fh_native_models_rejected++;return;}
    unsigned hash=0,hashed=0;
    for(unsigned i=0;i<entries;i++){
        const unsigned char *e=directory+i*32;
        if(word(e)!=id || word(e+4)!=bytes)continue;
        if(!hashed){hash=crc32(0,raw,bytes);hashed=1;}
        if(word(e+8)!=hash)continue;
        unsigned size=word(e+16),allocation=((size+3)&~3u)+257*sizeof(void*);
        if(!reserve(allocation)){fh_native_models_rejected++;return;}
        void *storage=malloc(allocation);FhModelAsset asset;
        if(!storage){fh_native_models_rejected++;return;}
        int accepted=!fseek(pack,(long)word(e+12),SEEK_SET) && fread(storage,1,size,pack)==size &&
            (unsigned)crc32(0,storage,size)==word(e+20) && fh_native_model_asset_open(&asset,storage,size,id,bytes,hash);
        if(!accepted){free(storage);fh_native_models_rejected++;return;}
        records[slot]=(Record){.owner=owner,.storage=storage,.asset=asset,
            .sources=(const void**)((unsigned char*)storage+((size+3)&~3u)),.allocation_bytes=allocation};
        fh_native_models_bytes+=allocation;fh_native_models_loaded++;
        return;
    }
}
