#include "fh_install_actor_fog_topology.h"
#include "fh_model_lod.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int compare(const void*x,const void*y){
    const unsigned char*a=x,*b=y;
    for(unsigned i=0;i<3;i++){unsigned x=word(a+4*i),y=word(b+4*i);if(x!=y)return x<y?-1:1;}return 0;
}
void fh_install_actor_fog_topology_close(FhInstallActorFogTopology*s){
    if(s){if(s->file)fclose(s->file);free(s->directory);memset(s,0,sizeof(*s));}
}
int fh_install_actor_fog_topology_open(FhInstallActorFogTopology*s,const char*path){
    if(!s||!path||s->file||s->directory)return 0;
    FhInstallActorFogTopology next={0};unsigned char header[16];
    next.file=fopen(path,"rb");if(!next.file)return 0;
    if(fseek(next.file,0,SEEK_END))goto fail;
    long size=ftell(next.file);if(size<16||size>32*1024*1024||fseek(next.file,0,SEEK_SET)||
       fread(header,1,16,next.file)!=16||memcmp(header,"FHMLPK1",8))goto fail;
    next.bytes=(unsigned)size;next.count=word(header+8);
    if(next.count>4096||next.count*24>next.bytes-16)goto fail;
    if(next.count){
        next.directory=malloc(next.count*24);if(!next.directory||
           fread(next.directory,24,next.count,next.file)!=next.count)goto fail;
    }
    if((unsigned)crc32(0,next.directory,next.count*24)!=word(header+12))goto fail;
    unsigned at=16+next.count*24;
    for(unsigned i=0;i<next.count;i++){
        const unsigned char*p=next.directory+i*24;unsigned bytes=word(p+16),source=word(p+4);
        if(word(p+12)!=at||bytes<24||bytes>1024*1024||bytes>next.bytes-at||
           source<(word(p)>>31?164u:252u)||source>4*1024*1024)goto fail;
        at+=bytes;
    }
    if(at!=next.bytes)goto fail;
    /* Earlier prepared packs retain discovery order, while native packs are
     * key-sorted. Validate file layout first, then build our own lookup order. */
    if(next.count)qsort(next.directory,next.count,24,compare);
    for(unsigned i=1;i<next.count;i++)if(!compare(next.directory+(i-1)*24,next.directory+i*24))goto fail;
    *s=next;return 1;
fail:fh_install_actor_fog_topology_close(&next);return 0;
}
int fh_install_actor_fog_topology_apply(FhInstallActorFogTopology*s,void*raw,unsigned bytes,unsigned id){
    if(!s||!s->file||!raw||bytes<(id>>31?164u:252u)||bytes>4*1024*1024)return 0;
    unsigned key[3]={id,bytes,(unsigned)crc32(0,raw,bytes)},low=0,high=s->count;
    while(low<high){
        unsigned mid=low+(high-low)/2;const unsigned char*p=s->directory+mid*24;int cmp=0;
        for(unsigned i=0;i<3;i++){unsigned value=word(p+i*4);if(value!=key[i]){cmp=value<key[i]?-1:1;break;}}
        if(cmp<0)low=mid+1;else if(cmp>0)high=mid;else{
            unsigned n=word(p+16);void*patch=malloc(n);if(!patch)return 0;
            int ok=!fseek(s->file,word(p+12),SEEK_SET)&&fread(patch,1,n,s->file)==n&&
                (unsigned)crc32(0,patch,n)==word(p+20)&&fh_model_lod_patch(raw,bytes,id,patch,n);
            free(patch);if(ok)s->applied++;return ok;
        }
    }
    return 1; /* Pack explicitly contains only changed models. */
}
