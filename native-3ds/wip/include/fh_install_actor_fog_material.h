#ifndef FH_INSTALL_ACTOR_FOG_MATERIAL_H
#define FH_INSTALL_ACTOR_FOG_MATERIAL_H
#include "fh_install_actor_fog_asset.h"
#include "fh_pica_actor_fog_material.h"
#include "fh_pica_actor_fog_variants.h"
extern const unsigned fh_install_actor_fog_material_context_bytes;
/* Collect both descriptor-setting and drawing materials from the immutable
 * source stream, then prepare its variant map. No geometry allocation. */
int fh_install_actor_fog_variants(FhPicaActorFogVariants*,const void *source,
    unsigned bytes,const FhPicaActorFogMaterial*,unsigned materials);
/* Ordinary actor load entry. The caller supplies original resolved material
 * facts and an explicit light-state alternative, not GPU-reduced image sizes.
 * The caller owns variant identity and storage lifetime. No callbacks from the
 * original renderer or GX interpretation run here. Effect admission, live state
 * capture and residency remain responsibilities of the native scene owner. */
int fh_install_actor_fog_material_asset(FhInstallActorFogAsset*,const void *source,
    unsigned bytes,unsigned id,unsigned variant,const FhPicaActorFogMaterial*,
    unsigned materials,const FhPicaActorFogSelection*);
/* Plan and facts must belong to this same immutable source-material generation.
 * Compile only variants selected for bounded residency, never all slots blindly.
 * Call outside rendering; frame lookup must not invoke this compiler. */
int fh_install_actor_fog_variant_asset(FhInstallActorFogAsset*,const void *source,
    unsigned bytes,unsigned id,const FhPicaActorFogVariants*,unsigned variant,
    const FhPicaActorFogMaterial*,unsigned materials);
#endif
