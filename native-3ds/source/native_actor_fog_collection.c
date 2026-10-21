#include "fh_actor_fog_collection.h"
#include "fh_native_actor_fog.h"
#include "fh_lz4_block.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static unsigned word(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static int order(const void*x,const void*y){
    const FhActorFogCollectionEntry*a=x,*b=y;
    const unsigned av[]={a->id,a->source_bytes,a->source_crc,a->bump};
    const unsigned bv[]={b->id,b->source_bytes,b->source_crc,b->bump};
    for(unsigned i=0;i<4;i++)if(av[i]!=bv[i])return av[i]<bv[i]?-1:1;
    return 0;
}
static int footer(FILE*f,unsigned end,FhActorFogCollectionEntry*out,FhActorFogBundle*b,unsigned*compressed){
    unsigned char data[FH_ACTOR_FOG_COLLECTION_FOOTER];
    if(!f||end<FH_ACTOR_FOG_COLLECTION_FOOTER||end>FH_ACTOR_FOG_COLLECTION_LIMIT||
       fseek(f,end-sizeof(data),SEEK_SET)||fread(data,1,sizeof(data),f)!=sizeof(data))return 0;
    const unsigned char*t=data+FH_ACTOR_FOG_BUNDLE_HEADER;
    unsigned start=word(t+8),payload=word(t+16);
    unsigned packed=!memcmp(t,"FHFCL002",8),raw=packed?word(t+20):payload;
    if((!packed&&memcmp(t,"FHFCL001",8))||word(t+12)!=FH_ACTOR_FOG_BUNDLE_HEADER||
       (!packed&&word(t+20))||word(t+60)||raw>FH_ACTOR_FOG_BUNDLE_LIMIT-FH_ACTOR_FOG_BUNDLE_HEADER||
       start>end-sizeof(data)||payload!=end-sizeof(data)-start||
       (unsigned)crc32(0,data,sizeof(data)-8)!=word(t+56))return 0;
    unsigned blocks=(raw+FH_ACTOR_FOG_COLLECTION_BLOCK-1)/FH_ACTOR_FOG_COLLECTION_BLOCK;
    if(packed&&(blocks*8>payload||(!raw&&payload)))return 0;
    FhActorFogCollectionEntry e={.id=word(data+8),.source_bytes=word(data+12),
        .source_crc=word(data+16),.bump=word(data+20),.start=start,.end=end};
    memcpy(e.input_sha256,t+24,32);
    if(!fh_actor_fog_bundle_decode(b,data,raw+FH_ACTOR_FOG_BUNDLE_HEADER,
        e.id,e.source_bytes,e.source_crc,e.bump))return 0;
    *out=e;if(compressed)*compressed=packed;return 1;
}
void fh_actor_fog_collection_close(FhActorFogCollection*c){if(c){free(c->entries);memset(c,0,sizeof(*c));}}
int fh_actor_fog_collection_open(FhActorFogCollection*out,FILE*f){
    if(!out||!f||fseek(f,0,SEEK_END))return 0;
    long length=ftell(f);if(length<0||length>FH_ACTOR_FOG_COLLECTION_LIMIT)return 0;
    FhActorFogCollection c={.file=f,.bytes=(unsigned)length};unsigned end=c.bytes;
    while(end){
        if(c.count==FH_ACTOR_FOG_COLLECTION_RECORDS)goto fail;
        FhActorFogCollectionEntry e;FhActorFogBundle b;
        if(!footer(f,end,&e,&b,NULL))goto fail;
        if(c.count==c.capacity){
            unsigned capacity=c.capacity?c.capacity*2:16;
            void*p=realloc(c.entries,capacity*sizeof(*c.entries));if(!p)goto fail;
            c.entries=p;c.capacity=capacity;
        }
        c.entries[c.count++]=e;end=e.start;
    }
    if(c.count)qsort(c.entries,c.count,sizeof(*c.entries),order);
    for(unsigned i=1;i<c.count;i++)if(!order(c.entries+i-1,c.entries+i))goto fail;
    fh_actor_fog_collection_close(out);*out=c;return 1;
fail:free(c.entries);return 0;
}
const FhActorFogCollectionEntry*fh_actor_fog_collection_find(const FhActorFogCollection*c,
    unsigned id,unsigned bytes,unsigned crc,unsigned bump){
    if(!c||!c->file||!c->count||!c->entries)return NULL;
    FhActorFogCollectionEntry key={.id=id,.source_bytes=bytes,.source_crc=crc,.bump=bump};
    return bsearch(&key,c->entries,c->count,sizeof(*c->entries),order);
}
int fh_actor_fog_collection_bundle(FhActorFogBundle*out,const FhActorFogCollection*c,
    const FhActorFogCollectionEntry*e){
    if(!out||!c||!e||e->end>c->bytes)return 0;
    FhActorFogCollectionEntry actual;FhActorFogBundle b;
    if(!footer(c->file,e->end,&actual,&b,NULL)||memcmp(&actual,e,sizeof(actual)))return 0;
    *out=b;return 1;
}
int fh_actor_fog_collection_read(void*context,unsigned at,void*out,unsigned bytes){
    FhActorFogCollectionRead*r=context;
    if(!r||!r->file||!out||r->start>r->end||r->end>FH_ACTOR_FOG_COLLECTION_LIMIT||
       r->end-r->start<FH_ACTOR_FOG_COLLECTION_FOOTER||
       r->entry.offset<FH_ACTOR_FOG_BUNDLE_HEADER||r->entry.metadata>FH_ACTOR_FOG_META_CAP||
       r->entry.geometry>FH_ACTOR_FOG_GPU_CAP)return 0;
    unsigned size=r->entry.metadata+r->entry.geometry;
    unsigned relative=r->entry.offset-FH_ACTOR_FOG_BUNDLE_HEADER;
    if(at>size||bytes>size-at)return 0;
    FhActorFogCollectionEntry e;FhActorFogBundle b;unsigned compressed;
    if(!footer(r->file,r->end,&e,&b,&compressed)||e.start!=r->start)return 0;
    unsigned raw=b.bytes-FH_ACTOR_FOG_BUNDLE_HEADER;
    if(relative>raw||size>raw-relative)return 0;
    unsigned found=0;
    for(unsigned i=0;i<b.plan.count;i++)if(!memcmp(b.entries+i,&r->entry,sizeof(r->entry)))found=1;
    if(!found)return 0;
    if(!compressed)return !fseek(r->file,r->start+relative+at,SEEK_SET)&&fread(out,1,bytes,r->file)==bytes;
    if(!bytes)return 1;
    /* Only selected prepared packets are decoded, during resident loading.
     * No global cache, retained raw collection or draw-time decompression.
     * The resident publisher checks the complete original packet CRC before
     * flushing GPU-visible storage. This workspace is ordinary CPU memory. */
    const unsigned chunk=FH_ACTOR_FOG_COLLECTION_BLOCK;
    unsigned blocks=(raw+chunk-1)/chunk;
    unsigned directory=r->end-FH_ACTOR_FOG_COLLECTION_FOOTER-blocks*8;
    unsigned stored=directory-r->start;
    unsigned cursor=relative+at,first=cursor/chunk,last=(cursor+bytes-1)/chunk;
    unsigned entries=last-first+1+(last+1<blocks);
    unsigned char*memory=malloc(2*chunk+entries*8);if(!memory)return 0;
    unsigned char*index_data=memory+2*chunk;
    int ok=!fseek(r->file,directory+first*8,SEEK_SET)&&fread(index_data,8,entries,r->file)==entries;
    /* One directory slice and one forward payload stream, rather than two
     * filesystem seeks per block. The last lookahead entry proves adjacency. */
    if(ok)ok=word(index_data)<=stored&&!fseek(r->file,r->start+word(index_data),SEEK_SET);
    unsigned char*target=out;unsigned remaining=bytes;
    while(ok&&remaining){
        unsigned index=cursor/chunk,offset=cursor%chunk;
        const unsigned char*pair=index_data+(index-first)*8;
        unsigned position=word(pair),packed=word(pair+4),length=packed&0x7fffffffu;
        unsigned decoded=raw-index*chunk;if(decoded>chunk)decoded=chunk;
        unsigned next=index+1<blocks?word(pair+8):stored;
        if(position>stored||!length||length>stored-position||length>decoded||
           (!(packed>>31)&&length!=decoded)||((packed>>31)&&length>=decoded)||
           (!index&&position)||next!=position+length){ok=0;break;}
        ok=fread(memory,1,length,r->file)==length;
        if(!ok)break;
        const unsigned char*source=memory;
        if(packed>>31){ok=fh_lz4_block(memory+chunk,decoded,memory,length);source=memory+chunk;}
        if(!ok)break;
        unsigned n=decoded-offset;if(n>remaining)n=remaining;
        memcpy(target,source+offset,n);target+=n;cursor+=n;remaining-=n;
    }
    free(memory);return ok;
}
int fh_actor_fog_collection_verify(const FhActorFogCollection*c){
    if(!c||!c->file)return 0;
    unsigned char*chunk=malloc(16384);if(!chunk)return 0;int ok=1;
    for(unsigned i=0;ok&&i<c->count;i++){
        const FhActorFogCollectionEntry*e=c->entries+i;FhActorFogBundle b;
        ok=fh_actor_fog_collection_bundle(&b,c,e);
        for(unsigned j=0;ok&&j<b.plan.count;j++){
            FhActorFogCollectionRead read={c->file,b.entries[j],e->start,e->end};
            unsigned total=read.entry.metadata+read.entry.geometry,crc=0;
            for(unsigned at=0;ok&&at<total;){
                unsigned n=total-at;if(n>16384)n=16384;
                ok=fh_actor_fog_collection_read(&read,at,chunk,n);
                if(ok)crc=(unsigned)crc32(crc,chunk,n);
                at+=n;
            }
            if(crc!=read.entry.crc)ok=0;
        }
    }
    free(chunk);return ok;
}
