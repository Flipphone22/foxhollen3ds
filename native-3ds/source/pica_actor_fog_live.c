#include "fh_pica_actor_fog_live.h"
#include "fh_graphics_options.h"
#include "fh_pica_height_fog.h"
#include "game/objects/object.h"
#include "main/model.h"
#include "main/texture.h"
#include "main/camera.h"
#include <math.h>

/* These are original game owners, not values retained by the last world draw. */
extern float gHeavyFogDepthScale,gHeavyFogDepthOffset,gHeavyFogTop,gHeavyFogBottom;
extern unsigned char gHeavyFogMode,gNewShadowHeavyFogIntensity;
extern int isHeavyFogEnabled(void);
extern void getNewShadowHeavyFogTexture(Texture**);
extern void getFogColorRgb(unsigned char*);

int fh_pica_actor_fog_game_live(FhPicaActorFogSnapshot*out,const ObjModel*model,
    const GameObject*obj,const float*poses,const float world[3][4],
    const float view[3][4]){
    if(!(fh_graphics_options&FH_GRAPHICS_HEIGHT_FOG) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS)) ||
       !out || !model || !model->file || !obj || !poses || !world || !view)return 0;
    const ModelFileHeader*h=model->file;
    /* Original modelDoRenderInstrs applies source flags before renderHeavyFog.
     * Static packet positions cannot stand in for CPU-written vertex streams. */
    if(model->renderCallback || (h->flags&0x100) || !h->jointCount ||
       h->vertexAnimEntries || h->morphTargetCount || model->vtxBufDirty ||
       gHeavyFogMode)return 0;
    float scale=obj->anim.rootMotionScale;
    if(!isfinite(scale) || scale==0)return 0;
    float inverse=1.f/scale;
    if(!isfinite(inverse) || !isHeavyFogEnabled())return 0;
    Texture*image=NULL;getNewShadowHeavyFogTexture(&image);
    if(!image || gNewShadowHeavyFogIntensity>64)return 0;
    FhPicaActorFogSnapshot s={0};
    if(!fh_pica_height_fog_planes(s.planes,
        (const float(*)[4])Camera_GetInverseViewMatrix(),
        gHeavyFogDepthScale,gHeavyFogDepthOffset,gHeavyFogTop,gHeavyFogBottom))return 0;
    unsigned char rgba[4]={0,0,0,255};getFogColorRgb(rgba);
    s.live.color=(unsigned)rgba[0]|((unsigned)rgba[1]<<8)|((unsigned)rgba[2]<<16)|((unsigned)rgba[3]<<24);
    s.live.poses=poses;s.live.pose_count=h->jointCount;
    /* The main source pass unconditionally runs modelInitMtxs here. Its count
     * selects the normal-matrix arithmetic order. The native renderer bypasses
     * that cache producer, so gModelMtxCacheState would be stale. */
    unsigned count=(unsigned)h->jointCount+h->extraJointCount;
    s.live.cached_normal=count>=2 && count<=100;
    s.live.world=world;s.live.view=view;
    for(unsigned i=0;i<3;i++)s.adjustment[i][i]=inverse;
    s.live.image=(const unsigned char*)image+sizeof(Texture);
    s.live.intensity=gNewShadowHeavyFogIntensity;
    *out=s;
    out->live.adjustment=(const float(*)[4])out->adjustment;
    out->live.planes=(const float(*)[4])out->planes;
    return 1;
}
