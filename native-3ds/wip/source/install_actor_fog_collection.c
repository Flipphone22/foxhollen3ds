#include "fh_install_actor_fog_collection.h"
#include <mbedtls/sha256.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <lz4.h>
static void put(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static int fail(FhInstallTransaction*t,const char*why){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",why);return 0;}
/* A repeated model can resolve textures through another bank. Raw CRC/SHA
 * alone cannot prove identical source coordinate routing. Hash canonical
 * resolved fields as well; duplicate reuse never guesses texture dimensions. */
int fh_install_actor_fog_input_digest(unsigned char digest[32],const void*raw,unsigned bytes,
    unsigned bump,FhActorFogTextureWidth width,void*context){
    if(!digest)return 0;
    FhPicaActorFogMaterial facts[64];FhPicaActorFogVariants plan;unsigned count;
    if(!fh_install_actor_fog_prepare(&plan,facts,&count,raw,bytes,bump,width,context))return 0;
    mbedtls_sha256_context hash;mbedtls_sha256_init(&hash);
    int ok=!mbedtls_sha256_starts_ret(&hash,0)&&!mbedtls_sha256_update_ret(&hash,raw,bytes);
    for(unsigned i=0;ok&&i<count;i++){
        const FhPicaActorFogMaterial*m=facts+i;
        const unsigned fields[]={m->environment,m->auxiliary,m->bump_enabled,m->bump_parameters,
            m->layers,m->textured_layers,m->decal,m->base_width,m->auxiliary_width};
        unsigned char encoded[36];for(unsigned j=0;j<9;j++)put(encoded+j*4,fields[j]);
        ok=!mbedtls_sha256_update_ret(&hash,encoded,sizeof(encoded));
    }
    if(ok)ok=!mbedtls_sha256_finish_ret(&hash,digest);
    mbedtls_sha256_free(&hash);return ok;
}
void fh_install_actor_fog_collection_close(FhInstallActorFogCollection*s){
    if(s){free(s->lookup);fh_actor_fog_collection_close(&s->collection);memset(s,0,sizeof(*s));}
}
enum {LOOKUP_SLOTS=2*FH_ACTOR_FOG_COLLECTION_RECORDS};
_Static_assert((LOOKUP_SLOTS&(LOOKUP_SLOTS-1))==0,"Fog index must be power of two");
_Static_assert(FH_ACTOR_FOG_COLLECTION_RECORDS<65536,"Fog index must fit uint16");
/* Full key comparison is mandatory: a hash collision never admits a duplicate.
 * 50% maximum occupancy, bounded probes, and indices survive entry realloc. */
static unsigned lookup_slot(const FhInstallActorFogCollection*s,const FhActorFogCollectionEntry*key){
    unsigned hash=2166136261u;
    const unsigned fields[]={key->id,key->source_bytes,key->source_crc,key->bump};
    for(unsigned i=0;i<4;i++){hash^=fields[i];hash*=16777619u;hash^=hash>>16;}
    unsigned slot=hash&(LOOKUP_SLOTS-1);
    for(unsigned n=0;n<LOOKUP_SLOTS;n++,slot=(slot+1)&(LOOKUP_SLOTS-1)){
        unsigned index=s->lookup[slot];
        if(!index)return slot;
        if(index>s->collection.count)return LOOKUP_SLOTS;
        const FhActorFogCollectionEntry*e=s->collection.entries+index-1;
        if(e->id==key->id&&e->source_bytes==key->source_bytes&&
           e->source_crc==key->source_crc&&e->bump==key->bump)return slot;
    }
    return LOOKUP_SLOTS;
}
int fh_install_actor_fog_collection_begin(FhInstallActorFogCollection*s,FhInstallTransaction*t,unsigned artifact){
    if(!s||s->transaction||s->collection.entries||s->lookup||!t||t->failed)return 0;
    if(!fh_install_artifact_begin(t,artifact))return 0;
    if(!fh_actor_fog_collection_open(&s->collection,t->output)||s->collection.bytes!=t->written||
       fseek(t->output,t->written,SEEK_SET)){
        fh_actor_fog_collection_close(&s->collection);return fail(t,"Invalid actor fog collection checkpoint.");
    }
    s->lookup=calloc(LOOKUP_SLOTS,sizeof(*s->lookup));
    if(!s->lookup){fh_install_actor_fog_collection_close(s);return fail(t,"Cannot allocate actor fog preparation index.");}
    for(unsigned i=0;i<s->collection.count;i++){
        unsigned slot=lookup_slot(s,s->collection.entries+i);
        if(slot==LOOKUP_SLOTS||s->lookup[slot]){
            fh_install_actor_fog_collection_close(s);return fail(t,"Invalid resumed actor fog preparation index.");
        }
        s->lookup[slot]=(uint16_t)(i+1);
    }
    s->transaction=t;s->cursor.batch=32;s->cursor.payload_bytes=1024*1024;return 1;
}
typedef struct {
    FhInstallTransaction*t;FhActorFogCollectionEntry entry;unsigned initial,finished;
    unsigned raw,used,blocks;
    unsigned char *memory,*input,*encoded,*directory;
} Sink;
enum {BLOCK=FH_ACTOR_FOG_COLLECTION_BLOCK,
      MAX_BLOCKS=(FH_ACTOR_FOG_BUNDLE_LIMIT+BLOCK-1)/BLOCK};
/* Startup-only workspace. A single buffered block spans compiler append calls;
 * no uncompressed collection or temporary second copy is written to SD. The
 * LZ4 state is explicitly heap-owned instead of a large implicit ARM stack. */
static int block(Sink*s){
    if(!s->used)return 1;
    if(s->blocks==MAX_BLOCKS)return 0;
    int n=LZ4_compress_fast_extState(s->memory,(const char*)s->input,
        (char*)s->encoded,s->used,BLOCK,1);
    unsigned stored=n>0&&(unsigned)n<s->used?(unsigned)n:s->used;
    if(s->t->written<s->entry.start||s->t->written>FH_ACTOR_FOG_COLLECTION_LIMIT-stored)return 0;
    unsigned char*e=s->directory+s->blocks*8;
    put(e,s->t->written-s->entry.start);put(e+4,stored|(stored<s->used?0x80000000u:0));
    if(!fh_install_artifact_write(s->t,stored<s->used?s->encoded:s->input,stored))return 0;
    s->blocks++;s->used=0;return 1;
}
static int append(void*context,const void*data,unsigned bytes){
    Sink*s=context;
    if(s->finished)return 0;
    if(!s->initial){
        if(bytes!=FH_ACTOR_FOG_BUNDLE_HEADER)return 0;
        const unsigned char*p=data;for(unsigned i=0;i<bytes;i++)if(p[i])return 0;
        s->initial=1;return 1; /* Placeholder is replaced by the final footer. */
    }
    if(s->raw>FH_ACTOR_FOG_BUNDLE_LIMIT-FH_ACTOR_FOG_BUNDLE_HEADER||
       bytes>FH_ACTOR_FOG_BUNDLE_LIMIT-FH_ACTOR_FOG_BUNDLE_HEADER-s->raw)return 0;
    if(!s->memory){
        unsigned state=(unsigned)LZ4_sizeofState();
        if(!state||state>65536)return 0;
        state=(state+7u)&~7u;
        s->memory=malloc(state+2*BLOCK+MAX_BLOCKS*8);if(!s->memory)return 0;
        s->input=s->memory+state;s->encoded=s->input+BLOCK;s->directory=s->encoded+BLOCK;
    }
    const unsigned char*p=data;s->raw+=bytes;
    while(bytes){
        unsigned n=BLOCK-s->used;if(n>bytes)n=bytes;
        memcpy(s->input+s->used,p,n);s->used+=n;p+=n;bytes-=n;
        if(s->used==BLOCK&&!block(s))return 0;
    }
    return 1;
}
static int header(void*context,const void*data,unsigned bytes){
    Sink*s=context;FhActorFogBundle b;
    if(!s->initial||s->finished||bytes!=FH_ACTOR_FOG_BUNDLE_HEADER||s->t->written<s->entry.start)return 0;
    if(!fh_actor_fog_bundle_decode(&b,data,s->raw+bytes,s->entry.id,
        s->entry.source_bytes,s->entry.source_crc,s->entry.bump))return 0;
    if(!block(s)||s->blocks!=(s->raw+BLOCK-1)/BLOCK||
       s->t->written>FH_ACTOR_FOG_COLLECTION_LIMIT-FH_ACTOR_FOG_COLLECTION_FOOTER-s->blocks*8)
        return fail(s->t,"Actor fog collection exceeds bounded storage.");
    if(s->blocks&&!fh_install_artifact_write(s->t,s->directory,s->blocks*8))return 0;
    unsigned payload=s->t->written-s->entry.start;
    unsigned char tail[64]={0};memcpy(tail,"FHFCL002",8);
    put(tail+8,s->entry.start);put(tail+12,bytes);put(tail+16,payload);
    put(tail+20,s->raw);
    memcpy(tail+24,s->entry.input_sha256,32);
    put(tail+56,(unsigned)crc32(crc32(0,data,bytes),tail,56));
    if(!fh_install_artifact_write(s->t,data,bytes)||!fh_install_artifact_write(s->t,tail,sizeof(tail)))return 0;
    s->entry.end=s->t->written;s->finished=1;return 1;
}
int fh_install_actor_fog_collection_add(FhInstallActorFogCollection*s,
    const void*raw,unsigned bytes,unsigned id,unsigned bump,FhActorFogTextureWidth width,
    void*context,unsigned next_file,unsigned next_index){
    if(!s||!s->transaction||!s->lookup||!raw)return 0;
    FhInstallTransaction*t=s->transaction;FhActorFogCollection*c=&s->collection;
    if(t->failed||!t->output)return 0;
    if(id>>31||bump>1||bytes<252||bytes>4*1024*1024)return fail(t,"Invalid actor fog source identity.");
    Sink sink={.t=t,.entry={.id=id,.source_bytes=bytes,.source_crc=(unsigned)crc32(0,raw,bytes),.bump=bump,.start=t->written}};
    if(!fh_install_actor_fog_input_digest(sink.entry.input_sha256,raw,bytes,bump,width,context))return fail(t,"Cannot resolve actor fog input identity.");
    if(!fh_install_cursor_boundary(t,&s->cursor,next_file))return 0;
    unsigned slot=lookup_slot(s,&sink.entry);
    if(slot==LOOKUP_SLOTS)return fail(t,"Invalid actor fog preparation lookup.");
    const FhActorFogCollectionEntry*prior=s->lookup[slot]?c->entries+s->lookup[slot]-1:NULL;
    if(prior){
        if(memcmp(prior->input_sha256,sink.entry.input_sha256,32))return fail(t,"Ambiguous actor fog source or texture identity.");
        s->duplicates++;return fh_install_cursor_duplicate(t,&s->cursor,next_file,next_index);
    }
    if(c->count==FH_ACTOR_FOG_COLLECTION_RECORDS)return fail(t,"Too many actor fog source records.");
    if(c->count==c->capacity){
        unsigned capacity=c->capacity?c->capacity*2:16;
        void*p=realloc(c->entries,capacity*sizeof(*c->entries));
        if(!p)return fail(t,"Cannot allocate actor fog source index.");
        c->entries=p;c->capacity=capacity;
    }
    const FhInstallActorFogSink writer={append,header,&sink};
    int compiled=fh_install_actor_fog_bundle_write(&writer,raw,bytes,id,bump,width,context);
    free(sink.memory);
    if(!compiled||!sink.finished)
        return fail(t,"Cannot compile actor fog collection record.");
    c->entries[c->count++]=sink.entry;s->lookup[slot]=(uint16_t)c->count;
    c->bytes=t->written;
    return fh_install_cursor_record(t,&s->cursor,next_file,next_index);
}
static int validate(const char*path,void*context){
    (void)context;FILE*f=fopen(path,"rb");if(!f)return 0;
    FhActorFogCollection c={0};int ok=fh_actor_fog_collection_open(&c,f)&&fh_actor_fog_collection_verify(&c);
    fh_actor_fog_collection_close(&c);if(fclose(f))ok=0;return ok;
}
int fh_install_actor_fog_collection_finish(FhInstallActorFogCollection*s){
    if(!s||!s->transaction)return 0;
    int ok=fh_install_cursor_flush(s->transaction,&s->cursor)&&
        fh_install_artifact_finish(s->transaction,validate,NULL);
    fh_install_actor_fog_collection_close(s);return ok;
}
