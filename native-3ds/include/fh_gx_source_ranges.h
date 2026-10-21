#ifndef FH_GX_SOURCE_RANGES_H
#define FH_GX_SOURCE_RANGES_H
#include "gx_internal.h"
typedef struct { unsigned offset,bytes; } FhGxSourceRange;
/* The complete list remains byte-validated on every hit. Decode its indexed
 * address coverage only when recording; keep every byte between the extrema.
 * NBT3's three independent indices have separate within-element offsets. */
static inline int fh_gx_source_ranges(const void *list,unsigned bytes,unsigned format,unsigned mask,FhGxSourceRange ranges[12]){
    struct Field { unsigned attr,mode,offset,width,slices; } fields[12];
    unsigned used=0,stride=0,low[12],high[12]={0};
    for(unsigned i=0;i<12;i++)low[i]=~0u;
    if(!list || format>=8)return 0;
    for(unsigned attr=0;attr<=GX_VA_TEX7;attr++){
        unsigned mode=fh_gx.descriptors[attr];if(!mode)continue;
        if(mode>GX_INDEX16)return 0;
        const FhVtxFormat *f=&fh_gx.formats[format][attr];
        unsigned width=1,slices=1;
        if(attr<GX_VA_POS){if(mode!=GX_DIRECT)return 0;}
        else if(attr==GX_VA_CLR0 || attr==GX_VA_CLR1){
            static const unsigned sizes[]={2,3,4,2,3,4};if(f->type>=6)return 0;width=sizes[f->type];
        }else{
            if(f->type>GX_F32)return 0;
            unsigned components=attr==GX_VA_POS?(f->count==GX_POS_XY?2:3):
                attr==GX_VA_NRM?(f->count==GX_NRM_XYZ?3:9):(f->count==GX_TEX_S?1:2);
            width=components*(f->type<2?1:f->type<4?2:4);
            if(attr==GX_VA_NRM && mode!=GX_DIRECT && f->count==GX_NRM_NBT3){slices=3;width/=3;}
        }
        if(attr>=GX_VA_POS && mode>=GX_INDEX8 && (mask&(1u<<(attr-GX_VA_POS))))
            fields[used++]=(struct Field){attr,mode,stride,width,slices};
        stride+=mode==GX_DIRECT?width:(mode==GX_INDEX8?1:2)*slices;
        if(stride>256)return 0;
    }
    if(!stride)return 0;
    const unsigned char *p=list;unsigned at=0;
    while(at<bytes){
        unsigned op=p[at++];if(!op)continue;unsigned kind=op&0xf8;
        if((op&7)!=format || (kind!=GX_TRIANGLES && kind!=GX_TRIANGLESTRIP &&
           kind!=GX_TRIANGLEFAN && kind!=GX_QUADS && kind!=GX_POINTS) || bytes-at<2)return 0;
        unsigned count=(unsigned)p[at]<<8|p[at+1];at+=2;
        if(count>(bytes-at)/stride)return 0;
        for(unsigned vertex=0;vertex<count;vertex++,at+=stride)for(unsigned f=0;f<used;f++){
            const struct Field *field=fields+f;unsigned slot=field->attr-GX_VA_POS;
            const FhVtxArray *array=&fh_gx.arrays[field->attr];
            const unsigned char *indices=p+at+field->offset;
            for(unsigned slice=0;slice<field->slices;slice++){
                unsigned index=*indices++;
                if(field->mode==GX_INDEX16)index=index<<8|*indices++;
                uint64_t begin=(uint64_t)index*array->stride+slice*field->width;
                if(!array->data || begin>array->bytes || field->width>array->bytes-begin)return 0;
                unsigned end=(unsigned)begin+field->width;
                if(begin<low[slot])low[slot]=(unsigned)begin;if(end>high[slot])high[slot]=end;
            }
        }
    }
    for(unsigned i=0;i<12;i++)if(mask&(1u<<i)){
        if(low[i]==~0u || high[i]<=low[i])return 0;
        ranges[i]=(FhGxSourceRange){low[i],high[i]-low[i]};
    }
    return 1;
}
#endif
