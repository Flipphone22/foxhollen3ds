#include "fh_actor_fog_bundle.h"
#include "fh_native_actor_fog.h"
#include <string.h>
#include <zlib.h>
static unsigned word(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
int fh_actor_fog_bundle_decode(FhActorFogBundle*out,const void*header,unsigned length,
    unsigned id,unsigned source_bytes,unsigned source_crc,unsigned bump){
    if(!out||!header||bump>1||id>>31||source_bytes<252||source_bytes>4*1024*1024||
       length<FH_ACTOR_FOG_BUNDLE_HEADER||length>FH_ACTOR_FOG_BUNDLE_LIMIT)return 0;
    const unsigned char*h=header;
    if(memcmp(h,"FHFGB001",8)||word(h+8)!=id||
       word(h+12)!=source_bytes||word(h+16)!=source_crc||word(h+20)!=bump||word(h+24)!=(unsigned)length)return 0;
    const unsigned char zero[4]={0};unsigned crc=(unsigned)crc32(0,h,28);
    crc=(unsigned)crc32(crc,zero,4);crc=(unsigned)crc32(crc,h+32,FH_ACTOR_FOG_BUNDLE_HEADER-32);
    if(crc!=word(h+28))return 0;
    FhActorFogBundle b={0};memcpy(&b.plan,h+32,sizeof(b.plan));b.bytes=(unsigned)length;
    if(b.plan.reserved||b.plan.count>FH_ACTOR_FOG_VARIANT_CELLS)return 0;
    for(unsigned i=0;i<FH_ACTOR_FOG_VARIANT_CELLS;i++){
        if(b.plan.slots[i]>b.plan.count)return 0;
        if(i>=b.plan.count&&b.plan.representative[i])return 0;
    }
    unsigned cursor=FH_ACTOR_FOG_BUNDLE_HEADER;
    for(unsigned i=0;i<FH_ACTOR_FOG_VARIANT_CELLS;i++){
        const unsigned char*p=h+196+i*16;
        if(i>=b.plan.count){for(unsigned k=0;k<16;k++)if(p[k])return 0;continue;}
        FhPicaActorFogSelection selection;
        if(!fh_pica_actor_fog_variant_selection(&selection,&b.plan,i+1))return 0;
        FhActorFogBundleEntry e={word(p),word(p+4),word(p+8),word(p+12)};
        if(e.offset!=cursor||e.metadata<128||e.metadata>FH_ACTOR_FOG_META_CAP||
           !e.geometry||e.geometry>FH_ACTOR_FOG_GPU_CAP||cursor>b.bytes||
           e.metadata>b.bytes-cursor||e.geometry>b.bytes-cursor-e.metadata)return 0;
        b.entries[i]=e;cursor+=e.metadata+e.geometry;
    }
    for(unsigned i=1492;i<FH_ACTOR_FOG_BUNDLE_HEADER;i++)if(h[i])return 0;
    if(cursor!=b.bytes)return 0;
    *out=b;return 1;
}
int fh_actor_fog_bundle_open(FhActorFogBundle*out,FILE*f,unsigned id,
    unsigned source_bytes,unsigned source_crc,unsigned bump){
    if(!out||!f||fseek(f,0,SEEK_END))return 0;
    long length=ftell(f);
    if(length<FH_ACTOR_FOG_BUNDLE_HEADER||length>FH_ACTOR_FOG_BUNDLE_LIMIT||fseek(f,0,SEEK_SET))return 0;
    unsigned char h[FH_ACTOR_FOG_BUNDLE_HEADER];
    return fread(h,1,sizeof(h),f)==sizeof(h)&&
        fh_actor_fog_bundle_decode(out,h,(unsigned)length,id,source_bytes,source_crc,bump);
}
int fh_actor_fog_bundle_read(void*context,unsigned at,void*out,unsigned bytes){
    FhActorFogBundleRead*r=context;
    if(!r||!r->file||!out||r->entry.metadata>FH_ACTOR_FOG_META_CAP||
       r->entry.geometry>FH_ACTOR_FOG_GPU_CAP)return 0;
    unsigned size=r->entry.metadata+r->entry.geometry;
    if(at>size||bytes>size-at||r->entry.offset>FH_ACTOR_FOG_BUNDLE_LIMIT||
       size>FH_ACTOR_FOG_BUNDLE_LIMIT-r->entry.offset)return 0;
    return !fseek(r->file,r->entry.offset+at,SEEK_SET)&&fread(out,1,bytes,r->file)==bytes;
}
