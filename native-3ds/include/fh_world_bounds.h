#ifndef FH_WORLD_BOUNDS_H
#define FH_WORLD_BOUNDS_H
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
/* Pinned Melee geometry_bounds.h: conservative homogeneous side planes.
 * Static world chunks only; no animation bounds or perspective division.
 * Four triangles per chunk retain original triangle/material ordering. */
enum { FH_WORLD_CHUNK_INDICES=12, FH_WORLD_MAX_RUNS=344, FH_WORLD_COARSE_CHUNKS=16 };
typedef struct {float lo[3],hi[3];} FhWorldBounds;
typedef struct {unsigned first,count;} FhWorldRange;
/* Prepare corner selection once per set of clip planes, not for every box.
 * Byte offsets select existing float subobjects without indexing past lo[3].
 * Preserve the scalar multiply/add order and the original conservative margin. */
_Static_assert(sizeof(FhWorldBounds)==24 && offsetof(FhWorldBounds,hi)==12,"World bounds float layout");
typedef struct {float p[4],magnitude;unsigned char maximum[3];} FhWorldPlane;
static inline void fh_world_planes_prepare(FhWorldPlane out[4],const float p[4][4]){
    for(unsigned i=0;i<4;i++){
        memcpy(out[i].p,p[i],sizeof(out[i].p));out[i].magnitude=fabsf(p[i][3])+1;
        for(unsigned k=0;k<3;k++)out[i].maximum[k]=(unsigned char)(p[i][k]>=0?12+k*4:k*4);
    }
}
static inline float fh_world_corner(const FhWorldBounds *b,unsigned offset){
    float v;memcpy(&v,(const unsigned char*)b+offset,4);return v;
}
static inline int fh_world_prepared_outside_mask(const FhWorldBounds *b,const FhWorldPlane p[4],unsigned mask){
    for(unsigned i=0;i<4;i++){
        if(!(mask&(1u<<i)))continue;
        float x=p[i].p[0]*fh_world_corner(b,p[i].maximum[0]);
        float y=p[i].p[1]*fh_world_corner(b,p[i].maximum[1]);
        float z=p[i].p[2]*fh_world_corner(b,p[i].maximum[2]);
        float maximum=((p[i].p[3]+x)+y)+z;
        float magnitude=((p[i].magnitude+fabsf(x))+fabsf(y))+fabsf(z);
        uint32_t mb,ab;memcpy(&mb,&maximum,4);memcpy(&ab,&magnitude,4);
        if((mb&0x7f800000u)!=0x7f800000u && (ab&0x7f800000u)!=0x7f800000u &&
           maximum<-.002f*magnitude)return 1;
    }
    return 0;
}
static inline int fh_world_prepared_outside(const FhWorldBounds *b,const FhWorldPlane p[4]){
    return fh_world_prepared_outside_mask(b,p,15);
}
/* Contiguous parents preserve material/triangle order. Nonfinite children
 * poison the parent, forcing the original fine checks rather than rejection. */
static inline void fh_world_parent_build(FhWorldBounds *out,const FhWorldBounds *children,unsigned count){
    *out=children[0];
    for(unsigned i=0;i<count;i++)for(unsigned k=0;k<3;k++){
        if(!isfinite(children[i].lo[k]) || !isfinite(children[i].hi[k]) || children[i].lo[k]>children[i].hi[k]){
            for(unsigned j=0;j<3;j++)out->lo[j]=out->hi[j]=NAN;
            return;
        }
        if(children[i].lo[k]<out->lo[k])out->lo[k]=children[i].lo[k];
        if(children[i].hi[k]>out->hi[k])out->hi[k]=children[i].hi[k];
    }
}
/* -1: every child rejected, +1: every child retained, 0: inspect children.
 * Bound the magnitude used by the original fine test across the entire box.
 * The additional .0001 margin covers float32 evaluation rounding; decisions
 * near a plane keep the original checks. No PICA visibility assumption. */
/* -1 rejects the parent; otherwise return the planes still requiring child
 * tests. An inside plane cannot reject any contained child. Preserve the
 * existing .0021 margin, scalar evaluation order and fail-open NaN handling.
 * No temporal cache or inference from a stable game pointer is involved. */
static inline int fh_world_parent_planes(const FhWorldBounds *b,const float p[4][4]){
    unsigned mask=0;
    for(unsigned i=0;i<4;i++){
        float minimum=p[i][3],maximum=minimum,magnitude=fabsf(minimum)+1;
        for(unsigned k=0;k<3;k++){
            float a=p[i][k]*(p[i][k]>=0?b->lo[k]:b->hi[k]);
            float c=p[i][k]*(p[i][k]>=0?b->hi[k]:b->lo[k]);
            minimum+=a;maximum+=c;
            a=fabsf(a);c=fabsf(c);magnitude+=a>c?a:c;
        }
        /* Avoid ARM libm classification/min/max calls in this hot loop.
         * IEEE exponent checks retain NaN/Inf rejection, including overflow.
         * Invalid parents contain NaN in every coordinate. */
        uint32_t low_bits,high_bits,mag_bits;
        memcpy(&low_bits,&minimum,4);memcpy(&high_bits,&maximum,4);memcpy(&mag_bits,&magnitude,4);
        if((low_bits&0x7f800000u)==0x7f800000u || (high_bits&0x7f800000u)==0x7f800000u ||
           (mag_bits&0x7f800000u)==0x7f800000u)return 15;
        float margin=.0021f*magnitude;
        if(maximum < -margin)return -1;
        if(minimum < margin)mask|=1u<<i;
    }
    return (int)mask;
}
static inline int fh_world_parent_classify(const FhWorldBounds *b,const float p[4][4]){
    int mask=fh_world_parent_planes(b,p);
    return mask<0?-1:mask==0?1:0;
}
static inline void fh_world_bounds_build(FhWorldBounds *b,const void *vertices,
    const uint16_t *indices,unsigned count,unsigned packed,unsigned format){
    float scale=packed?(format==7?1.f:format==5?1.f/8:1.f/256):1.f;
    for(unsigned i=0;i<count;i++)for(unsigned k=0;k<3;k++){
        const unsigned char *p=(const unsigned char*)vertices+indices[i]*(packed?20:64);
        float value;
        if(packed){int16_t raw;memcpy(&raw,p+k*2,2);value=raw*scale;}
        else memcpy(&value,p+k*4,4);
        if(!i || value<b->lo[k])b->lo[k]=value;
        if(!i || value>b->hi[k])b->hi[k]=value;
    }
}
static inline void fh_world_clip_planes(float planes[4][4],const float view[3][4],
    const float projection[4][4]){
    for(unsigned i=0;i<4;i++){
        float p[4],sign=(i&1)?-1.f:1.f;
        for(unsigned k=0;k<4;k++)p[k]=projection[3][k]+sign*projection[i/2][k];
        for(unsigned k=0;k<4;k++){
            planes[i][k]=k==3?p[3]:0;
            for(unsigned j=0;j<3;j++)planes[i][k]+=p[j]*view[j][k];
        }
    }
}
static inline int fh_world_bounds_outside(const FhWorldBounds *b,const float p[4][4]){
    for(unsigned i=0;i<4;i++){
        float maximum=p[i][3],magnitude=fabsf(p[i][3])+1;
        for(unsigned k=0;k<3;k++){
            float term=p[i][k]*(p[i][k]>=0?b->hi[k]:b->lo[k]);
            maximum+=term;magnitude+=fabsf(term);
        }
        if(isfinite(maximum) && isfinite(magnitude) && maximum<-.002f*magnitude)return 1;
    }
    return 0;
}
/* Emit adjacent surviving spans as one draw, preserving original indices.
 * Bounds may include triangles outside this subsection, which only reduces
 * culling effectiveness. Invalid bounds never authorize a missing draw. */
static inline unsigned fh_world_visible_ranges(FhWorldRange *runs,unsigned capacity,
    const FhWorldBounds *bounds,unsigned total,unsigned first,unsigned count,const float planes[4][4]){
    if(!runs || !capacity)return 0;
    if(!bounds || first>total || count>total-first || first%3 || count%3){runs[0]=(FhWorldRange){0,count};return count?1:0;}
    unsigned n=0,end=first+count;
    for(unsigned at=first;at<end;){
        unsigned next=(at/FH_WORLD_CHUNK_INDICES+1)*FH_WORLD_CHUNK_INDICES;
        if(next>end)next=end;
        if(!fh_world_bounds_outside(bounds+at/FH_WORLD_CHUNK_INDICES,planes)){
            if(n && runs[n-1].first+runs[n-1].count==at-first)runs[n-1].count+=next-at;
            else {
                /* A caller capacity mistake retains the entire draw. */
                if(n==capacity){runs[0]=(FhWorldRange){0,count};return 1;}
                runs[n++]=(FhWorldRange){at-first,next-at};
            }
        }
        at=next;
    }
    return n;
}
static inline unsigned fh_world_visible_hierarchy(FhWorldRange *runs,unsigned capacity,
    const FhWorldBounds *bounds,const FhWorldBounds *parents,unsigned total,unsigned first,
    unsigned count,const float planes[4][4]){
    if(!parents || !bounds || first>total || count>total-first || first%3 || count%3)
        return fh_world_visible_ranges(runs,capacity,bounds,total,first,count,planes);
    if(!runs || !capacity)return 0;
    FhWorldPlane prepared[4];fh_world_planes_prepare(prepared,planes);
    unsigned n=0,end=first+count,at=first;
    while(at<end){
        unsigned parent=at/(FH_WORLD_CHUNK_INDICES*FH_WORLD_COARSE_CHUNKS);
        unsigned stop=(parent+1)*(FH_WORLD_CHUNK_INDICES*FH_WORLD_COARSE_CHUNKS);
        if(stop>end)stop=end;
        int mask=fh_world_parent_planes(parents+parent,planes);
        if(mask<0){at=stop;continue;}
        while(at<stop){
            unsigned next=mask==0?stop:(at/FH_WORLD_CHUNK_INDICES+1)*FH_WORLD_CHUNK_INDICES;
            if(next>stop)next=stop;
            if(mask==0 || !fh_world_prepared_outside_mask(bounds+at/FH_WORLD_CHUNK_INDICES,prepared,(unsigned)mask)){
                if(n && runs[n-1].first+runs[n-1].count==at-first)runs[n-1].count+=next-at;
                else{
                    if(n==capacity){runs[0]=(FhWorldRange){0,count};return 1;}
                    runs[n++]=(FhWorldRange){at-first,next-at};
                }
            }
            at=next;
        }
    }
    return n;
}
#endif
