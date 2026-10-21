#include "fh_native_scene.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned u32(const unsigned char *p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static unsigned u16(const unsigned char *p){return p[0]|(unsigned)p[1]<<8;}
int main(int argc,char **argv){
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));long length=ftell(f);rewind(f);
    unsigned char *pack=malloc(length);assert(pack && fread(pack,1,length,f)==(size_t)length);fclose(f);
    unsigned checked=0,vertices=0,negative=0;
    for(unsigned i=0;i<u32(pack+8);i++){
        const unsigned char *entry=pack+16+i*32,*packet=pack+u32(entry+12);
        if(memcmp(packet,"FHNSCN08",8))continue;
        unsigned mb=u32(packet+68),gb=u32(packet+72),count=u32(packet+92);
        unsigned char *meta=malloc(mb),*gpu=aligned_alloc(16,(gb+15)&~15u),*before=malloc(gb),*expected=malloc(gb);
        assert(meta && gpu && before && expected);memcpy(meta,packet,mb);memcpy(gpu,packet+mb,gb);memcpy(before,gpu,gb);memcpy(expected,gpu,gb);
        FhSceneAsset a;assert(fh_native_scene_open(&a,meta,mb,gpu,gb,u32(entry),u32(entry+4),u32(entry+8)));
        /* Unaligned native sources and distinct motions for coincident source
         * positions exercise identity/seam retention, not coordinate matching. */
        unsigned char *allocation=malloc(count*6+1),*source=allocation+1;assert(allocation);
        for(unsigned j=0;j<count*3;j++){int16_t value=(int16_t)(j*997+i*313);memcpy(source+j*2,&value,2);}
        assert(!fh_native_scene_refresh_world_positions(&a,source,count+1) && !memcmp(before,gpu,gb));negative++;
        for(unsigned g=0;g<a.groups;g++){
            const unsigned char *group=meta+96+g*96;unsigned n=u32(group+20),at=u32(group+28),refs=u32(group+44);
            for(unsigned j=0;j<n;j++){unsigned index=u16(meta+refs+j*2);assert(index<count);memcpy(expected+at+j*20,source+index*6,6);vertices++;}
        }
        assert(fh_native_scene_refresh_world_positions(&a,source,count) && !memcmp(expected,gpu,gb));
        /* A malformed source reference must be rejected before publication. */
        unsigned refs=u32(meta+96+44);meta[refs]=count&255;meta[refs+1]=count>>8;
        FhSceneAsset sentinel;memset(&sentinel,0x5a,sizeof(sentinel));FhSceneAsset old=sentinel;
        assert(!fh_native_scene_open(&sentinel,meta,mb,gpu,gb,u32(entry),u32(entry+4),u32(entry+8)) && !memcmp(&sentinel,&old,sizeof(old)));negative++;
        free(allocation);free(expected);free(before);free(gpu);free(meta);checked++;
    }
    free(pack);assert(checked && vertices);printf("World position refresh: %u packets, %u vertices, %u negative checks; UV/color/topology bytes unchanged\n",checked,vertices,negative);
}
