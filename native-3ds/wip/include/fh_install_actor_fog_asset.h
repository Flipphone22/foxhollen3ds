#ifndef FH_INSTALL_ACTOR_FOG_ASSET_H
#define FH_INSTALL_ACTOR_FOG_ASSET_H
#include "fh_install_actor_fog_groups.h"
#include "fh_native_actor_fog.h"
typedef struct {
    void *allocation;
    unsigned char *metadata,*gpu;
    unsigned metadata_bytes,gpu_bytes;
    FhInstallGroupResult groups;
    char error[112];
} FhInstallActorFogAsset;
/* Zero initialize. One preselected source material/light alternative. No
 * filesystem writes or live-GPU ownership; caller publishes only a successful
 * complete generation. A logic-only model succeeds with zero packet bytes. */
int fh_install_actor_fog_asset(FhInstallActorFogAsset*,const void *source,
    unsigned source_bytes,unsigned id,unsigned variant,
    const FhPicaActorFogMatrixBank *initial,unsigned skip_texture_loads,
    FhInstallActorFogRoute,FhInstallActorFogCoordinate,void *context);
void fh_install_actor_fog_asset_close(FhInstallActorFogAsset*);
#endif
