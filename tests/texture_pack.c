#include "fh_texture_pack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static unsigned checks;
#define CHECK(x) do{assert(x);checks++;}while(0)
static void put(unsigned char *p,unsigned x){p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;}
static unsigned make(unsigned char *p,const unsigned char *source,unsigned fmt,unsigned collision){
    unsigned n=collision?2:1,gpu=fmt==12?32:64,at=16+32*n;
    memset(p,0,512);memcpy(p,"FHTXPK1",8);put(p+8,n);
    for(unsigned i=0;i<n;i++){
        unsigned char *e=p+16+32*i;
        put(e,crc32(0,source,32));put(e+4,8);put(e+8,8);put(e+12,32);put(e+16,at);put(e+20,fmt);put(e+24,gpu);
        memcpy(p+at,source,32);if(collision && !i)p[at+17]^=1;
        for(unsigned j=0;j<gpu;j++)p[at+32+j]=j*3+7;
        put(e+28,crc32(0,p+at+32,gpu));at+=32+gpu;
    }
    put(p+12,crc32(0,p+16,n*32));return at;
}
static void write_pack(const char *path,const void *data,unsigned bytes){
    FILE *f=fopen(path,"wb");assert(f);assert(fwrite(data,1,bytes,f)==bytes);assert(!fclose(f));
}
int main(int argc,char **argv){
    assert(argc==2);const char *path=argv[1];unsigned char blob[512],source[128],other[32],out[160];
    for(unsigned i=0;i<sizeof(source);i++)source[i]=i*7;
    FhTexturePack p={0};FhPackedTexture e;unsigned char window[FH_TEXTURE_WINDOW_PAGE];
    for(unsigned fmt=12;fmt<=13;fmt++)for(unsigned collision=0;collision<2;collision++){
        unsigned size=make(blob,source,fmt,collision),gpu=fmt==12?32:64;write_pack(path,blob,size);
        CHECK(fh_texture_pack_open(&p,path,128));CHECK(p.directory_bytes==(collision?64:32));
        memcpy(other,source,32);other[3]^=32;CHECK(!fh_texture_pack_find(&p,other,8,8,&e));
        CHECK(!fh_texture_pack_find(&p,source,7,8,&e));CHECK(!fh_texture_pack_find(&p,source,8,16,&e));
        CHECK(fh_texture_pack_find(&p,source,8,8,&e));CHECK(e.format==fmt && e.bytes==gpu);
        memset(out,0xaa,sizeof(out));CHECK(!fh_texture_pack_read(&p,&e,out+8,gpu-1));
        for(unsigned j=0;j<sizeof(out);j++)CHECK(out[j]==0xaa);
        CHECK(fh_texture_pack_read(&p,&e,out+8,gpu));
        for(unsigned j=0;j<gpu;j++)CHECK(out[8+j]==(unsigned char)(j*3+7));
        for(unsigned j=0;j<8;j++)CHECK(out[j]==0xaa && out[8+gpu+j]==0xaa);
        CHECK(!fh_texture_pack_window(&p,window,sizeof(window)-1));
        CHECK(fh_texture_pack_window(&p,window,sizeof(window)));
        CHECK(fh_texture_pack_find(&p,source,8,8,&e));
        CHECK(fh_texture_pack_read(&p,&e,out,gpu));uint64_t cached_disk=p.read_bytes;
        p.window_clock=UINT64_MAX;
        CHECK(fh_texture_pack_find(&p,source,8,8,&e));
        CHECK(fh_texture_pack_read(&p,&e,out,gpu));
        /* One slot cannot retain both deliberately colliding records. */
        if(!collision)CHECK(p.read_bytes==cached_disk);else CHECK(p.read_bytes>=cached_disk);
        for(unsigned j=0;j<gpu;j++)CHECK(out[j]==(unsigned char)(j*3+7));
        fh_texture_pack_window_drop(&p);CHECK(!p.window && !p.window_pages);
        unsigned char resident[512];unsigned required=fh_texture_pack_resident_size(&p);
        CHECK(required==32+gpu*(collision?2:1));
        CHECK(!fh_texture_pack_resident_load(&p,resident,required-1));
        if(collision)CHECK(!fh_texture_pack_resident_load(&p,resident,required));
        CHECK(fh_texture_pack_resident_load(&p,resident,sizeof(resident)));
        uint64_t disk=p.read_bytes;
        CHECK(fh_texture_pack_find(&p,source,8,8,&e));
        CHECK(fh_texture_pack_read(&p,&e,out,gpu));
        CHECK(p.read_bytes==disk && p.resident_reads>=2);
        for(unsigned j=0;j<gpu;j++)CHECK(out[j]==(unsigned char)(j*3+7));
        /* Cache memory is caller-owned. Dropping it restores ordinary reads. */
        fh_texture_pack_resident_drop(&p);
        CHECK(fh_texture_pack_find(&p,source,8,8,&e));
        CHECK(fh_texture_pack_read(&p,&e,out,gpu));CHECK(p.read_bytes>disk);
        fh_texture_pack_close(&p);CHECK(!p.file && !p.directory && !p.count);
        CHECK(!fh_texture_pack_open(&p,path,31));
        /* Original-source collision cannot admit the wrong bytes even with a
         * matching directory hash. Discard the second, matching record. */
        if(collision){
            size=make(blob,source,fmt,0);blob[48+17]^=1;write_pack(path,blob,size);
            CHECK(fh_texture_pack_open(&p,path,128));CHECK(fh_texture_pack_window(&p,window,sizeof(window)));
            CHECK(!fh_texture_pack_find(&p,source,8,8,&e));fh_texture_pack_close(&p);
        }
        /* A damaged GPU payload is never published; caller must discard it. */
        size=make(blob,source,fmt,0);blob[size-1]^=1;write_pack(path,blob,size);
        CHECK(fh_texture_pack_open(&p,path,128));CHECK(fh_texture_pack_window(&p,window,sizeof(window)));
        CHECK(fh_texture_pack_find(&p,source,8,8,&e));
        CHECK(!fh_texture_pack_read(&p,&e,out,gpu));CHECK(!p.file && !p.directory);
        CHECK(!p.window && !p.window_pages);
    }
    /* Every truncation and each malformed directory field is rejected. */
    unsigned size=make(blob,source,12,0);
    for(unsigned n=0;n<size;n++){write_pack(path,blob,n);CHECK(!fh_texture_pack_open(&p,path,128));}
    for(unsigned field=0;field<8;field++){
        size=make(blob,source,12,0);
        if(field==0)blob[0]^=1;
        else if(field==1)put(blob+8,0xffffffff);
        else if(field==2)put(blob+20,7);
        else if(field==3)put(blob+24,2048);
        else if(field==4)put(blob+28,31);
        else if(field==5)put(blob+32,0xffffffff);
        else if(field==6)put(blob+36,14);
        else put(blob+40,0xffffffff);
        put(blob+12,crc32(0,blob+16,32));write_pack(path,blob,size);
        CHECK(!fh_texture_pack_open(&p,path,128));CHECK(!p.file && !p.directory);
    }
    /* Same exact source has reduced surface and original UI variants. */
    memset(blob,0,sizeof(blob));memcpy(blob,"FHTXPK2",8);put(blob+8,2);
    unsigned end=96;
    for(unsigned i=0;i<2;i++){
        unsigned char *d=blob+16+40*i;unsigned side=i?16:8,gpu=side*side/2;
        put(d,crc32(0,source,128));put(d+4,16);put(d+8,16);put(d+12,128);
        put(d+16,end);put(d+20,12);put(d+24,gpu);put(d+32,side);put(d+36,side);
        memcpy(blob+end,source,128);memset(blob+end+128,0x35+i,gpu);
        put(d+28,crc32(0,blob+end+128,gpu));end+=128+gpu;
    }
    put(blob+12,crc32(0,blob+16,80));write_pack(path,blob,end);
    CHECK(fh_texture_pack_open(&p,path,80));
    unsigned char compact[288];
    CHECK(fh_texture_pack_resident_size(&p)==288); /* one source, two GPU images */
    CHECK(fh_texture_pack_resident_load(&p,compact,sizeof(compact)));
    CHECK(p.resident_offsets[0]==p.resident_offsets[2]);
    CHECK(p.resident_bytes==288);
    CHECK(fh_texture_pack_find(&p,source,16,16,&e));
    CHECK(e.width==16 && e.height==16 && e.bytes==128);
    CHECK(fh_texture_pack_read(&p,&e,out,sizeof(out)) && out[0]==0x36);
    CHECK(fh_texture_pack_find_surface(&p,source,16,16,&e));
    CHECK(e.width==8 && e.height==8 && e.bytes==32);
    CHECK(fh_texture_pack_read(&p,&e,out,sizeof(out)) && out[0]==0x35);
    source[77]^=1;CHECK(!fh_texture_pack_find_surface(&p,source,16,16,&e));source[77]^=1;
    compact[p.resident_offsets[1]]^=1;
    CHECK(fh_texture_pack_find_surface(&p,source,16,16,&e));
    CHECK(!fh_texture_pack_read(&p,&e,out,sizeof(out)));CHECK(!p.file && !p.resident);
    fh_texture_pack_close(&p);
    unsigned char valid[512];memcpy(valid,blob,end);
    for(unsigned n=0;n<end;n++){write_pack(path,valid,n);CHECK(!fh_texture_pack_open(&p,path,80));}
    for(unsigned bad=0;bad<6;bad++){
        memcpy(blob,valid,end);
        if(bad==0)put(blob+48,0);
        if(bad==1)put(blob+48,7);
        if(bad==2)put(blob+48,32);
        if(bad==3)put(blob+36,13); /* Reduced alpha image is forbidden. */
        if(bad==4)put(blob+40,128);
        put(blob+12,crc32(0,blob+16,80));
        if(bad==5)blob[12]^=1;
        write_pack(path,blob,end);CHECK(!fh_texture_pack_open(&p,path,80));
    }
    remove(path);CHECK(!fh_texture_pack_open(&p,path,128));
    printf("texture_pack checks=%u uploads=%u rejected=%u\n",checks,p.uploads,p.rejected);return 0;
}
