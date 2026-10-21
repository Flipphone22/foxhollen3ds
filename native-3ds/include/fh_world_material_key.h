#ifndef FH_WORLD_MATERIAL_KEY_H
#define FH_WORLD_MATERIAL_KEY_H
#include <stdint.h>
#include <string.h>
#include "fh_minimal_graphics.h"
/* Exact TEV inputs for unlit world materials. Image addresses, dimensions,
 * UVs and raster colour remain live bindings, not part of this state key. */
typedef struct {uint32_t mode,konst;} FhWorldMaterialKey;
static inline FhWorldMaterialKey fh_world_material_key(const FhMinimalWorldMaterial *m){
    FhWorldMaterialKey key={0,0};
    if(m->layered){
        key.mode=4u|(m->texture[0].format<=1?1u:0u)|(m->texture[1].format<=1?2u:0u);
        memcpy(&key.konst,m->konst,4);
    }else if(m->texture[0].pixels)key.mode=8u|(m->texture[0].format<=1?1u:0u);
    return key;
}
static inline int fh_world_material_equal(FhWorldMaterialKey a,FhWorldMaterialKey b){
    return a.mode==b.mode && a.konst==b.konst;
}
#endif
