#ifndef FH_PICA_VIEW_POSE_H
#define FH_PICA_VIEW_POSE_H
#include <stdint.h>
#include <string.h>
/* Prototype: concatenate camera once per palette bone, never per vertex.
 * Input/output may alias. Caller owns publication and pose/camera lifetime. */
static inline void fh_pica_view_pose(float out[3][4],const float view[3][4],const float pose[3][4]){
    float result[3][4];
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)
        result[r][c]=view[r][0]*pose[0][c]+view[r][1]*pose[1][c]+view[r][2]*pose[2][c]+(c==3?view[r][3]:0);
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)out[r][c]=result[r][c];
}
/* One actor invocation owns this cache. Do not retain across camera/pose
 * changes or lend it to the GPU: the SDK copies each published row. */
typedef struct {
    uint32_t valid[9];
    float rows[257][3][4];
} FhPicaViewPoseCache;
static inline void fh_pica_view_pose_cache_reset(FhPicaViewPoseCache *cache){
    memset(cache->valid,0,sizeof(cache->valid));
}
static inline float (*fh_pica_view_pose_cache_slot(FhPicaViewPoseCache *cache,unsigned bone,unsigned *fresh))[4]{
    if(bone>256)return 0;
    uint32_t bit=1u<<(bone&31);*fresh=(cache->valid[bone>>5]&bit)==0;
    cache->valid[bone>>5]|=bit;
    return cache->rows[bone];
}
#endif
