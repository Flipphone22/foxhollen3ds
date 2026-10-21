#include "fh_native_model_asset.h"
#include <string.h>
#include <math.h>
_Static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"Native model packets require little-endian ARM");
static unsigned u16(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static unsigned u32(const unsigned char *p){return u16(p)|u16(p+2)<<16;}
static int range(unsigned at,unsigned n,unsigned bytes){return at>=64 && !(at&3) && at<=bytes && n<=bytes-at;}
const void *fh_native_model_asset_topology(const FhModelAsset *a,unsigned index,unsigned *bytes){
    if(!a || !a->data || !bytes || index>=a->topology_count)return NULL;
    const unsigned char *p=a->data+a->topology_at+index*16;*bytes=u32(p+4);return a->data+u32(p+8);
}
int fh_native_model_asset_group(const FhModelAsset *a,unsigned group,FhModelGroup *out){
    if(!a || !a->data || !out || group>=a->groups)return 0;
    const unsigned char *p=a->data+a->groups_at+group*48;
    *out=(FhModelGroup){u32(p),u32(p+4),u32(p+8),u32(p+12),u32(p+16),u32(p+20),u32(p+24),u32(p+28),
        (const uint16_t*)(a->data+u32(p+32)),(const uint16_t*)(a->data+u32(p+40)),(const FhModelRef*)(a->data+u32(p+36)),u32(p+44)};
    return 1;
}
int fh_native_model_asset_draw(const FhModelAsset *a,unsigned ordinal,unsigned list,unsigned shader,unsigned *group){
    if(!a || !a->data || !group || ordinal>=a->draws)return 0;
    const unsigned char *p=a->data+a->draws_at+ordinal*8;
    if(u16(p)!=list || u16(p+2)!=shader)return 0;
    *group=u16(p+4);return 1;
}
int fh_native_model_asset_open(FhModelAsset *out,const void *data,unsigned bytes,unsigned id,unsigned source_bytes,unsigned source_crc){
    const unsigned char *b=data;
    if(!out || !b || ((uintptr_t)b&3) || bytes<64 || bytes>256*1024 || memcmp(b,"FHNMESH2",8) ||
       u32(b+8)!=bytes || u32(b+12)!=id || u32(b+16)!=source_bytes || u32(b+20)!=source_crc)return 0;
    FhModelAsset a={b,bytes,id,source_bytes,source_crc,u32(b+24),u32(b+28),u32(b+32),u32(b+36),u32(b+40),u32(b+44),u32(b+48),u32(b+52),u32(b+56),u32(b+60)};
    if(!a.draws || a.draws>256 || !a.groups || a.groups>a.draws || !a.bones || a.bones>256 || a.bindings<a.bones || a.bindings>256 ||
       !range(a.tails_at,a.bones*12,bytes) || !range(a.bindings_at,a.bindings*4,bytes) ||
       !range(a.groups_at,a.groups*48,bytes) || !range(a.draws_at,a.draws*8,bytes) ||
       a.topology_count<2 || a.topology_count>257 || !range(a.topology_at,a.topology_count*16,bytes))return 0;
    if(a.tails_at!=64 || a.bindings_at!=a.tails_at+a.bones*12 ||
       a.groups_at!=a.bindings_at+a.bindings*4)return 0;
    unsigned cursor=a.groups_at+a.groups*48;
    for(unsigned i=0;i<a.bones*3;i++){float v;memcpy(&v,b+a.tails_at+i*4,4);if(!isfinite(v))return 0;}
    for(unsigned i=0;i<a.bindings;i++){
        const unsigned char *p=b+a.bindings_at+i*4;
        if(p[0]>=a.bones || p[1]>=a.bones || p[2]>4 || p[3]>1 ||
           (i<a.bones && (p[0]!=i || p[1]!=i || p[2]!=4 || p[3])) ||
           (i>=a.bones && !p[3]))return 0;
    }
    for(unsigned i=0;i<a.draws;i++){
        const unsigned char *p=b+a.draws_at+i*8;unsigned g=u16(p+4);
        if(u16(p)>=256 || u16(p+2)>=64 || u16(p+6) || (g!=UINT16_MAX && g>=a.groups))return 0;
    }
    unsigned previous_end=0;
    for(unsigned i=0;i<a.groups;i++){
        const unsigned char *p=b+a.groups_at+i*48;
        unsigned first=u32(p+12),draws=u32(p+16),bones=u32(p+20),vertices=u32(p+24),indices=u32(p+28);
        if(u32(p)>=64 || (u32(p+4)!=3 && u32(p+4)!=5 && u32(p+4)!=6 && u32(p+4)!=7) || u32(p+8)>1 ||
           first<previous_end || first>=a.draws || !draws || draws>a.draws-first ||
           !bones || bones>FH_MODEL_BONES || !vertices || vertices>FH_MODEL_VERTICES ||
           !indices || indices>FH_MODEL_INDICES || indices%3 || u32(p+44)>1 ||
           !range(u32(p+32),bones*2,bytes) || !range(u32(p+36),vertices*12,bytes) || !range(u32(p+40),indices*2,bytes))return 0;
        if(u32(p+32)!=((cursor+3)&~3u) || u32(p+36)!=((u32(p+32)+bones*2+3)&~3u) ||
           u32(p+40)!=u32(p+36)+vertices*12)return 0;
        cursor=u32(p+40)+indices*2;
        previous_end=first+draws;FhModelGroup g;fh_native_model_asset_group(&a,i,&g);
        for(unsigned j=0;j<g.bones;j++){
            if(g.bone_ids[j]>=a.bones)return 0;
            for(unsigned k=0;k<j;k++)if(g.bone_ids[j]==g.bone_ids[k])return 0;
        }
        for(unsigned j=0;j<g.vertices;j++){
            const FhModelRef *r=g.refs+j;
            if(r->position==UINT16_MAX || r->material!=g.shader || r->joint>=a.bindings)return 0;
            const unsigned char *binding=b+a.bindings_at+r->joint*4;unsigned found[2]={0,0};
            for(unsigned k=0;k<g.bones;k++){found[0]|=g.bone_ids[k]==binding[0];found[1]|=g.bone_ids[k]==binding[1];}
            if((binding[2] && !found[0]) || (binding[2]<4 && !found[1]))return 0;
        }
        for(unsigned j=0;j<g.indices;j++)if(g.index[j]>=g.vertices)return 0;
        for(unsigned j=first;j<first+draws;j++){
            const unsigned char *d=b+a.draws_at+j*8;
            if(u16(d+4)!=i || u16(d+2)!=g.shader)return 0;
        }
    }
    if(a.draws_at!=((cursor+3)&~3u) || a.topology_at!=a.draws_at+a.draws*8)return 0;
    cursor=a.topology_at+a.topology_count*16;
    for(unsigned i=0;i<a.topology_count;i++){
        const unsigned char *p=b+a.topology_at+i*16;unsigned n=u32(p+4),at=u32(p+8);
        if(u32(p)!=(i?i-1:UINT32_MAX) || !n || n>65535 || u32(p+12) || at!=((cursor+3)&~3u) || !range(at,n,bytes))return 0;
        cursor=at+n;
    }
    if(cursor!=bytes)return 0;
    for(unsigned i=0;i<a.draws;i++){
        unsigned group=u16(b+a.draws_at+i*8+4);
        if(group!=UINT16_MAX){FhModelGroup g;fh_native_model_asset_group(&a,group,&g);if(i<g.first || i>=g.first+g.draws)return 0;}
    }
    *out=a;return 1;
}
