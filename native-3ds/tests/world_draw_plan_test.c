#include "fh_native_scene.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned word(const unsigned char *p) {
    return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;
}
int main(int argc,char **argv) {
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    unsigned char header[20];unsigned packets=0,checks=0;
    while(fread(header,1,20,f)==20) {
        unsigned id=word(header),size=word(header+4),crc=word(header+8);
        unsigned bytes=word(header+12),length=word(header+16);
        unsigned char *packet=malloc(length),*source=malloc(bytes);
        assert(packet && source && fread(packet,1,length,f)==length && fread(source,1,bytes,f)==bytes);
        unsigned split=word(packet+68);void *gpu=aligned_alloc(16,(length-split+15)&~15u);assert(gpu);
        memcpy(gpu,packet+split,length-split);FhSceneAsset asset;
        assert(fh_native_scene_open(&asset,packet,split,gpu,length-split,id,size,crc));
        unsigned count=fh_world_plan_compile(&asset,source,bytes,NULL,0,NULL,0);
        unsigned draws=word(packet+80);assert(count);
        FhWorldPlanOp *ops=malloc(count*sizeof(*ops));FhSceneGroup *groups=malloc(draws*sizeof(*groups));
        assert(ops && groups && fh_world_plan_compile(&asset,source,bytes,ops,count,groups,draws)==count);
        /* Every truncated source is an exactly sized allocation: padding reads
         * are sanitizer failures, even when the stream terminates early. */
        for(unsigned n=0;n<bytes;n++) {
            unsigned char *short_source=malloc(n?n:1);assert(short_source);
            memcpy(short_source,source,n);
            fh_world_plan_compile(&asset,short_source,n,ops,count,groups,draws);
            free(short_source);checks++;
        }
        /* Unknown opcodes, altered material/list bindings and matrix counts. */
        for(unsigned n=0;n<bytes;n++) {
            unsigned old=source[n];source[n]^=0xff;
            fh_world_plan_compile(&asset,source,bytes,ops,count,groups,draws);
            source[n]=old;checks++;
        }
        for(unsigned n=0;n<count;n++) {
            FhWorldPlanOp *short_ops=malloc(n?n*sizeof(*ops):1);assert(short_ops);
            assert(!fh_world_plan_compile(&asset,source,bytes,short_ops,n,groups,draws));
            free(short_ops);checks++;
        }
        free(groups);free(ops);free(gpu);free(source);free(packet);packets++;
    }
    assert(feof(f));fclose(f);
    printf("world draw plan sanitizer: packets=%u bounded_cases=%u PASS\n",packets,checks);
}
