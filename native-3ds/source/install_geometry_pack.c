/* Bounded on-device geometry conversion; never called from game rendering. */
#include "fh_install_geometry_pack.h"
#include "fh_install_assembly.h"
#include "fh_install_scene.h"
#include <mbedtls/sha256.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

enum {HEADER=68,STAGE=16384,SPOOL_LIMIT=128*1024*1024,PACK_LIMIT=64*1024*1024,
      PACKET_LIMIT=2*1024*1024+256*1024,PACK_CURSOR=0x47535031};
typedef struct {unsigned at,id,source,source_crc,bytes,crc;unsigned char sha[32];} Entry;
static unsigned word(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static void put(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static int fail(FhInstallTransaction*t,const char*why){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",why);return 0;}
static int decode(const unsigned char*h,Entry*e,unsigned at){
    if(memcmp(h,"FHGSPL01",8)||word(h+64)!=(unsigned)crc32(0,h,64)||word(h+28))return 0;
    e->at=at;e->id=word(h+8);e->source=word(h+12);e->source_crc=word(h+16);e->bytes=word(h+20);e->crc=word(h+24);memcpy(e->sha,h+32,32);
    return e->source>=(e->id>>31?164u:252u)&&e->source<=4*1024*1024&&
        ((!e->bytes&&!e->crc)||(e->bytes>=128&&e->bytes<=PACKET_LIMIT));
}
static int packet(FILE*f,const Entry*e){
    if(!e->bytes)return 1;
    void*allocation=malloc(e->bytes+15);if(!allocation)return 0;
    unsigned char*p=(void*)(((uintptr_t)allocation+15)&~(uintptr_t)15);FhSceneAsset a;
    int ok=fread(p,1,e->bytes,f)==e->bytes&&(unsigned)crc32(0,p,e->bytes)==e->crc;
    unsigned meta=ok?word(p+68):0;
    ok=ok&&meta>=128&&meta<=e->bytes&&fh_native_scene_open(&a,p,meta,p+meta,e->bytes-meta,e->id,e->source,e->source_crc);
    free(allocation);return ok;
}
static int spool_read(FILE*f,Entry*entries,unsigned*count,unsigned verify){
    if(fseek(f,0,SEEK_END))return 0;long length=ftell(f);
    if(length<0||length>SPOOL_LIMIT||fseek(f,0,SEEK_SET))return 0;
    unsigned at=0,n=0;unsigned char h[HEADER];
    while(at<(unsigned)length){
        Entry e;if(n==FH_INSTALL_GEOMETRY_LIMIT||HEADER>(unsigned)length-at||fread(h,1,HEADER,f)!=HEADER||!decode(h,&e,at)||e.bytes>(unsigned)length-at-HEADER)return 0;
        if(verify){if(!packet(f,&e))return 0;}else if(fseek(f,e.bytes,SEEK_CUR))return 0;
        if(entries){
            for(unsigned i=0;i<n;i++)if(entries[i].id==e.id&&entries[i].source==e.source&&entries[i].source_crc==e.source_crc)return 0;
            entries[n]=e;
        }
        n++;at+=HEADER+e.bytes;
    }
    *count=n;return !ferror(f);
}
static int spool_validate(const char*path,void*context){
    (void)context;FILE*f=fopen(path,"rb");if(!f)return 0;unsigned n;
    int ok=spool_read(f,NULL,&n,1);if(fclose(f))ok=0;return ok;
}
void fh_install_geometry_spool_close(FhInstallGeometrySpool*s){if(s){free(s->entries);memset(s,0,sizeof(*s));}}
int fh_install_geometry_spool_begin(FhInstallGeometrySpool*s,FhInstallTransaction*t,unsigned artifact){
    if(!s||s->entries||!t||t->failed)return 0;
    if(!fh_install_artifact_begin(t,artifact))return 0;
    Entry*e=calloc(FH_INSTALL_GEOMETRY_LIMIT,sizeof(*e));if(!e)return fail(t,"Not enough startup memory for geometry directory.");
    unsigned count=0;if(!spool_read(t->output,e,&count,0)||fseek(t->output,t->written,SEEK_SET)){free(e);return fail(t,"Invalid geometry conversion checkpoint.");}
    s->entries=e;s->transaction=t;s->count=count;
    for(unsigned i=0;i<count;i++){s->packets+=e[i].bytes!=0;s->nonrendering+=e[i].bytes==0;}return 1;
}
int fh_install_geometry_spool_add(FhInstallGeometrySpool*s,const void*raw,unsigned bytes,unsigned id,unsigned next_file,unsigned next_index){
    if(!s||!s->entries||!s->transaction)return 0;FhInstallTransaction*t=s->transaction;
    if(t->failed||!t->output)return 0;
    if(!raw||bytes<(id>>31?164u:252u)||bytes>4*1024*1024)return fail(t,"Invalid geometry source size.");
    unsigned crc=(unsigned)crc32(0,raw,bytes);unsigned char sha[32];
    if(mbedtls_sha256_ret(raw,bytes,sha,0))return fail(t,"Cannot fingerprint source geometry.");
    if(!fh_install_cursor_boundary(t,&s->cursor,next_file))return 0;
    Entry*entries=s->entries;
    for(unsigned i=0;i<s->count;i++)if(entries[i].id==id&&entries[i].source==bytes&&entries[i].source_crc==crc){
        if(memcmp(sha,entries[i].sha,32))return fail(t,"Ambiguous runtime geometry identity (CRC collision).");
        if(!fh_install_cursor_duplicate(t,&s->cursor,next_file,next_index))return 0;s->duplicates++;return 1;
    }
    if(s->count==FH_INSTALL_GEOMETRY_LIMIT)return fail(t,"Too many source geometry records.");
    FhInstallScene scene={0};
    if(!fh_install_scene(&scene,raw,bytes,id)){fail(t,scene.error);fh_install_scene_close(&scene);return 0;}
    unsigned char h[HEADER]={0};memcpy(h,"FHGSPL01",8);put(h+8,id);put(h+12,bytes);put(h+16,crc);put(h+20,scene.bytes);
    put(h+24,(unsigned)crc32(0,scene.data,scene.bytes));memcpy(h+32,sha,32);put(h+64,(unsigned)crc32(0,h,64));Entry e;
    int ok=decode(h,&e,t->written)&&t->written<=SPOOL_LIMIT&&HEADER+e.bytes<=SPOOL_LIMIT-t->written;
    if(!ok)fail(t,"Geometry spool exceeds bounded storage.");
    else ok=fh_install_artifact_write(t,h,HEADER)&&fh_install_artifact_write(t,scene.data,scene.bytes)&&fh_install_cursor_record(t,&s->cursor,next_file,next_index);
    if(ok){entries[s->count++]=e;s->packets+=e.bytes!=0;s->nonrendering+=e.bytes==0;}
    fh_install_scene_close(&scene);return ok;
}
int fh_install_geometry_spool_finish(FhInstallGeometrySpool*s){
    if(!s||!s->entries||!s->transaction)return 0;
    int ok=fh_install_cursor_flush(s->transaction,&s->cursor)&&fh_install_artifact_finish(s->transaction,spool_validate,NULL);fh_install_geometry_spool_close(s);return ok;
}
static int compare(const void*a,const void*b){
    const Entry*x=a,*y=b;
    if(x->id!=y->id)return x->id<y->id?-1:1;
    if(x->source!=y->source)return x->source<y->source?-1:1;
    if(x->source_crc!=y->source_crc)return x->source_crc<y->source_crc?-1:1;return 0;
}
static int pack_validate(const char*path,void*context){
    (void)context;FILE*f=fopen(path,"rb");if(!f)return 0;
    unsigned char h[16],*directory=NULL;int ok=0;
    if(fseek(f,0,SEEK_END))goto done;long length=ftell(f);
    if(length<16||length>PACK_LIMIT||fseek(f,0,SEEK_SET)||fread(h,1,16,f)!=16||memcmp(h,"FHNSSET1",8))goto done;
    unsigned n=word(h+8);if(!n||n>FH_INSTALL_GEOMETRY_LIMIT||n*32>(unsigned)length-16)goto done;
    directory=malloc(n*32);if(!directory||fread(directory,32,n,f)!=n||(unsigned)crc32(0,directory,n*32)!=word(h+12))goto done;
    unsigned at=16+n*32;
    for(unsigned i=0;i<n;i++){
        const unsigned char*d=directory+i*32;Entry e={0};e.id=word(d);e.source=word(d+4);e.source_crc=word(d+8);e.bytes=word(d+16);e.crc=word(d+20);
        if(word(d+12)!=at||word(d+24)||word(d+28)||e.bytes<128||e.bytes>PACKET_LIMIT||at>(unsigned)length||e.bytes>(unsigned)length-at)goto done;
        if(i){const unsigned char*p=d-32;Entry prev={0};prev.id=word(p);prev.source=word(p+4);prev.source_crc=word(p+8);if(compare(&prev,&e)>=0)goto done;}
        if(!packet(f,&e))goto done;unsigned padding=(0u-e.bytes)&3u;
        for(unsigned j=0;j<padding;j++)if(fgetc(f)!=0)goto done;at+=e.bytes+padding;
    }
    ok=at==(unsigned)length&&!ferror(f);
done:free(directory);if(fclose(f))ok=0;return ok;
}
static int copy(FhInstallTransaction*t,FILE*f,unsigned at,unsigned bytes){
    unsigned char b[STAGE];if(fseek(f,at,SEEK_SET))return fail(t,"Cannot seek geometry spool.");
    while(bytes){unsigned n=bytes>sizeof(b)?sizeof(b):bytes;
        if(fread(b,1,n,f)!=n)return fail(t,"Cannot read geometry spool.");
        if(!fh_install_artifact_write(t,b,n))return 0;bytes-=n;
    }return 1;
}
int fh_install_geometry_pack(FhInstallTransaction*t,unsigned spool,unsigned output,FhInstallGeometryProgress progress,void*context){
    if(!t||t->failed||t->output)return 0;
    if(spool>=FH_INSTALL_ARTIFACTS||output>=FH_INSTALL_ARTIFACTS||spool==output||!(t->state.complete&(1u<<spool)))return fail(t,"Invalid geometry assembly recipe.");
    if(t->state.complete&(1u<<output))return 1;
    char path[FH_INSTALL_TX_PATH];if(!fh_install_artifact_path(t->root,&t->state,spool,path))return 0;
    FILE*f=fopen(path,"rb");if(!f)return fail(t,"Cannot open completed geometry spool.");
    Entry*e=malloc(FH_INSTALL_GEOMETRY_LIMIT*sizeof(*e));unsigned char*dir=NULL;unsigned count=0;int result=0;
    if(!e){fail(t,"Not enough memory for geometry assembly.");goto done;}
    /* The completed spool was fully validated before publication. Assembly
     * needs its bounded, checksummed directory, not another full payload
     * decode. The final pack validator checks every copied packet's CRC and
     * native layout before that output can become complete. Damaged inputs
     * therefore still cannot publish a completed pack. */
    if(!spool_read(f,e,&count,0)){fail(t,"Geometry spool failed validation.");goto done;}
    unsigned n=0;for(unsigned i=0;i<count;i++)if(e[i].bytes)e[n++]=e[i];
    if(!n){fail(t,"No rendering geometry in source collection.");goto done;}
    qsort(e,n,sizeof(*e),compare);unsigned directory_bytes=16+n*32,end=directory_bytes;
    dir=calloc(1,directory_bytes);if(!dir){fail(t,"Not enough memory for geometry pack directory.");goto done;}
    memcpy(dir,"FHNSSET1",8);put(dir+8,n);
    for(unsigned i=0;i<n;i++){
        unsigned bytes=(e[i].bytes+3)&~3u;unsigned char*d=dir+16+i*32;
        if(bytes>PACK_LIMIT-end){fail(t,"Native geometry pack exceeds runtime64MiB limit.");goto done;}
        put(d,e[i].id);put(d+4,e[i].source);put(d+8,e[i].source_crc);put(d+12,end);put(d+16,e[i].bytes);put(d+20,e[i].crc);end+=bytes;
    }
    put(dir+12,(unsigned)crc32(0,dir+16,n*32));if(!fh_install_artifact_begin(t,output))goto done;
    unsigned next=0;
    if(!t->written){if(!fh_install_artifact_write(t,dir,directory_bytes)||!fh_install_artifact_checkpoint(t,0,PACK_CURSOR))goto done;}
    else{
        next=t->state.cursor[0];unsigned expected=next<n?word(dir+16+next*32+12):end;
        if(t->state.cursor[1]!=PACK_CURSOR||next>n||t->written!=expected){fail(t,"Invalid geometry assembly cursor.");goto done;}
        unsigned char chunk[STAGE];
        for(unsigned at=0;at<directory_bytes;){unsigned bytes=directory_bytes-at;if(bytes>sizeof(chunk))bytes=sizeof(chunk);
            if(!fh_install_artifact_read(t,at,chunk,bytes)||memcmp(chunk,dir+at,bytes)){fail(t,"Geometry assembly directory changed.");goto done;}at+=bytes;
        }
    }
    if(progress&&!progress(context,next,n)){result=-1;goto done;}
    for(unsigned i=next;i<n;i++){
        const unsigned char zero[4]={0};unsigned padding=(0u-e[i].bytes)&3u;
        if(!copy(t,f,e[i].at+HEADER,e[i].bytes)||!fh_install_artifact_write(t,zero,padding)||!fh_install_assembly_checkpoint(t,i+1,PACK_CURSOR,n))goto done;
        if(progress&&!progress(context,i+1,n)){result=fh_install_artifact_checkpoint(t,i+1,PACK_CURSOR)?-1:0;goto done;}
    }
    result=fh_install_artifact_finish(t,pack_validate,NULL);
done:
    free(e);free(dir);if(fclose(f)&&result>0)result=fail(t,"Cannot close geometry spool.");
    if(result<=0)fh_install_transaction_close(t);return result;
}
