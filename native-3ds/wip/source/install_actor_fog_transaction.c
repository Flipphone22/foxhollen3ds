#include "fh_install_actor_fog_transaction.h"
#include <stdlib.h>
#include <zlib.h>
typedef struct {unsigned id,bytes,crc,bump;} Identity;
static int append(void*c,const void*p,unsigned bytes){return fh_install_artifact_write(c,p,bytes);}
static int header(void*c,const void*p,unsigned bytes){return fh_install_artifact_patch(c,0,p,bytes);}
static int validate(const char*path,void*context){
    const Identity*i=context;FILE*f=fopen(path,"rb");if(!f)return 0;
    FhActorFogBundle b;int ok=fh_actor_fog_bundle_open(&b,f,i->id,i->bytes,i->crc,i->bump);
    /* Bounded streaming verification, never a second resident packet copy.
     * The compiler already checked semantics; validate every payload's bytes
     * again before the transaction marks the artifact complete. */
    unsigned char*chunk=ok?malloc(16384):NULL;
    if(ok&&!chunk)ok=0;
    for(unsigned n=0;ok&&n<b.plan.count;n++){
        FhActorFogBundleRead reader={f,b.entries[n]};unsigned crc=0;
        unsigned total=reader.entry.metadata+reader.entry.geometry;
        for(unsigned at=0;ok&&at<total;){
            unsigned count=total-at;if(count>16384)count=16384;
            ok=fh_actor_fog_bundle_read(&reader,at,chunk,count);
            if(ok)crc=(unsigned)crc32(crc,chunk,count);
            at+=count;
        }
        if(crc!=reader.entry.crc)ok=0;
    }
    free(chunk);if(fclose(f))ok=0;return ok;
}
int fh_install_actor_fog_artifact(FhInstallTransaction*t,unsigned artifact,
    const void*raw,unsigned bytes,unsigned id,unsigned bump,
    FhActorFogTextureWidth width,void*context){
    if(!t||t->failed||artifact>=FH_INSTALL_ARTIFACTS||!raw||!bytes||
       bytes>4*1024*1024||id>>31||bump>1)return 0;
    Identity identity={id,bytes,(unsigned)crc32(0,raw,bytes),bump};
    if(t->state.complete&(1u<<artifact)){
        char path[FH_INSTALL_TX_PATH];
        return fh_install_artifact_path(t->root,&t->state,artifact,path)&&validate(path,&identity);
    }
    if(!fh_install_artifact_begin(t,artifact))return 0;
    /* finish() checkpoints the full file before validation/rename. Recover
     * that interrupted finish without rewriting checkpointed bytes. An
     * incomplete nonzero checkpoint still fails exact bundle validation. */
    if(t->written)return fh_install_artifact_finish(t,validate,&identity);
    const FhInstallActorFogSink sink={append,header,t};
    if(!fh_install_actor_fog_bundle_write(&sink,raw,bytes,id,bump,width,context)){
        if(!t->failed)snprintf(t->error,sizeof(t->error),"Actor fog artifact conversion failed.");
        t->failed=1;return 0;
    }
    return fh_install_artifact_finish(t,validate,&identity);
}
