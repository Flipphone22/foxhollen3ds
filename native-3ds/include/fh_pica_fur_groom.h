#ifndef FH_PICA_FUR_GROOM_H
#define FH_PICA_FUR_GROOM_H
#include "fh_pica_model.h"
/* Four-byte stride avoids padded/unaligned additional PICA loaders. Only XY
 * are authored directions. Immutable after load, owned by the parent scene. */
typedef struct {int8_t x,y,z,w;} FhPicaFurGroomVertex;
typedef struct {unsigned offset,ready;} FhPicaFurGroomDraw;
typedef struct {unsigned count,bytes;void *gpu;FhPicaFurGroomDraw draw[];} FhPicaFurGroomPlan;
/* Original RGB5A3 base-mip linear repeat sampling, at load only. Zero rejects
 * an unsupported source/layout; NULL output validates and returns vertex count. */
unsigned fh_pica_fur_groom_compile(const FhSceneAsset*,const FhPicaModelDraw*,
    const FhTexture*,FhPicaFurGroomVertex*,unsigned);
void fh_native_scene_fur_groom_prepare(const FhSceneAsset*,unsigned,const FhTexture*);
#endif
