#ifndef FH_GROUND_RECEIVER_H
#define FH_GROUND_RECEIVER_H
#include <stdint.h>
#include <string.h>
#include <math.h>

/* CPU collision-plane data only. No GPU allocations, object pointers, retained
 * map addresses or cross-thread shared scratch. Keys are the exact nine live
 * s16 coordinates consumed by the source collision routine, including offsets.
 * Recycled maps and deformed vertices therefore cannot return stale planes. */
enum { FH_GROUND_WAYS=4,FH_GROUND_SETS=128,FH_GROUND_CELLS=FH_GROUND_WAYS*FH_GROUND_SETS };
typedef struct {
    uint32_t hash;
    int16_t key[9];
    uint8_t populated,valid;
    float normal[3],d,edge[3][4];
} FhGroundCell;
typedef struct {
    FhGroundCell cells[FH_GROUND_CELLS];
    uint8_t next[FH_GROUND_SETS];
    uint64_t lookups,hits,queries,triangles,receivers;
} FhGroundCache;
typedef struct {
    float x,y,z,best_distance,best_height,normal[3];
    unsigned triangles,hits;
} FhGroundQuery;

static inline void fh_ground_cross(const float*a,const float*b,float*c){
    c[0]=a[1]*b[2]-a[2]*b[1];c[1]=a[2]*b[0]-a[0]*b[2];c[2]=a[0]*b[1]-a[1]*b[0];
}
static inline float fh_ground_length(const float*a){return sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);}
static inline const FhGroundCell *fh_ground_plane(FhGroundCache*cache,const int16_t key[9]){
    uint32_t hash=2166136261u;
    for(unsigned i=0;i<9;i++)hash=(hash^(uint16_t)key[i])*16777619u;
    /* Grid-aligned coordinates correlate low bits strongly. Mix high bits
     * before bounded four-way lookup instead of thrashing one direct slot. */
    hash^=hash>>16;hash*=0x7feb352du;hash^=hash>>15;
    unsigned set=hash&(FH_GROUND_SETS-1);FhGroundCell*base=&cache->cells[set*FH_GROUND_WAYS];
    cache->lookups++;
    for(unsigned way=0;way<FH_GROUND_WAYS;way++){
        FhGroundCell*c=base+way;
        if(c->populated&&c->hash==hash&&!memcmp(c->key,key,sizeof(c->key))){cache->hits++;return c;}
    }
    FhGroundCell*c=base+(cache->next[set]++&(FH_GROUND_WAYS-1));
    memcpy(c->key,key,sizeof(c->key));c->hash=hash;c->populated=1;c->valid=0;
    float v[3][3],e[3][3];
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++)v[i][j]=key[i*3+j];
    for(unsigned j=0;j<3;j++){e[0][j]=v[0][j]-v[1][j];e[1][j]=v[1][j]-v[2][j];e[2][j]=v[2][j]-v[0][j];}
    fh_ground_cross(e[0],e[1],c->normal);float length=fh_ground_length(c->normal);
    if(!(length>0))return c;
    float inv=1.f/length;for(unsigned j=0;j<3;j++)c->normal[j]*=inv;
    c->d=-(c->normal[0]*v[0][0]+c->normal[1]*v[0][1]+c->normal[2]*v[0][2]);
    /* Preserve the source broadphase's degenerate-edge admission, even for
     * downward faces that count toward its 1200-triangle truncation. */
    for(unsigned i=0;i<3;i++){float n[3];fh_ground_cross(c->normal,e[i],n);if(!(fh_ground_length(n)>0))return c;}
    c->valid=1;
    if(!(c->normal[1]>0))return c;
    /* Exact source trackCollectGroundHits extrusion/normalization order.
     * This deliberately retains its 0.2 edge tolerance and float rounding. */
    for(unsigned i=0;i<3;i++){
        unsigned next=(i+1)%3;float w[3];
        for(unsigned j=0;j<3;j++)w[j]=10.f*c->normal[j]+v[i][j];
        float nx=w[1]*(v[i][2]-v[next][2])+(v[i][1]*(v[next][2]-w[2])+v[next][1]*(w[2]-v[i][2]));
        float ny=w[2]*(v[i][0]-v[next][0])+(v[i][2]*(v[next][0]-w[0])+v[next][2]*(w[0]-v[i][0]));
        float nz=w[0]*(v[i][1]-v[next][1])+(v[i][0]*(v[next][1]-w[1])+v[next][0]*(w[1]-v[i][1]));
        float mag=sqrtf(nx*nx+ny*ny+nz*nz);
        if(mag>0){float s=1.f/mag;nx*=s;ny*=s;nz*=s;}
        c->edge[i][0]=nx;c->edge[i][1]=ny;c->edge[i][2]=nz;
        c->edge[i][3]=-(nx*v[i][0]+ny*v[i][1]+nz*v[i][2]);
    }
    return c;
}
/* Returns true when the source's triangle or hit limit has been reached. */
static inline int fh_ground_consider(FhGroundCache*cache,FhGroundQuery*q,
        const int16_t key[9],int origin_x,int origin_z){
    const FhGroundCell*c=fh_ground_plane(cache,key);if(!c->valid)return 0;
    q->triangles++;cache->triangles++;
    if(c->normal[1]>0){
        float x=q->x-(float)origin_x,z=q->z-(float)origin_z;
        float height=-(c->normal[0]*x+c->normal[2]*z+c->d)/c->normal[1];
        unsigned inside=1;
        for(unsigned i=0;i<3;i++){
            const float*e=c->edge[i];
            if(e[3]+(e[0]*x+e[1]*height+e[2]*z)>.2f){inside=0;break;}
        }
        if(inside){
            float distance=fabsf(q->y-height);
            /* Source stable descending-height sort then nearest-absolute
             * selection: equal distance prefers higher, equal height first. */
            if(!q->hits||distance<q->best_distance||(distance==q->best_distance&&height>q->best_height)){
                q->best_distance=distance;q->best_height=height;memcpy(q->normal,c->normal,sizeof(q->normal));
            }
            q->hits++;cache->receivers++;
        }
    }
    return q->triangles>=1200||q->hits>=35;
}
int fh_native_ground_receiver(float x,float y,float z,float*offset,float normal[3]);
void fh_native_ground_receiver_report(void);
#endif
