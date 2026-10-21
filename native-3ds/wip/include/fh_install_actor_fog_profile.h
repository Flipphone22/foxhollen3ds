#ifndef FH_INSTALL_ACTOR_FOG_PROFILE_H
#define FH_INSTALL_ACTOR_FOG_PROFILE_H
#include "fh_pica_actor_fog_material.h"
#include "fh_pica_actor_fog_variants.h"
/* Authored texture-table ID, before ObjModel_Load applies its load flags. The
 * resolver must use the original image header, never a reduced native image.
 * It is called only when source bump routing actually depends on image width. */
typedef int (*FhActorFogTextureWidth)(void*,uint32_t texture_id,unsigned*width);
/* Converter/load-time facts equivalent to ResolveRenderOpTextures + shaderInit
 * + the ordinary game material adapter, assuming successful source texture loads
 * and the original initialized distortion slots. Does not allocate/load textures,
 * change reference counts, or admit callbacks/dynamic vertices/native materials.
 * Failure preserves output. Source remains immutable and unrelocated. */
int fh_install_actor_fog_profile(FhPicaActorFogMaterial*,const void *source,
    unsigned source_bytes,unsigned material,unsigned source_bump_enabled,
    FhActorFogTextureWidth,void*context);
/* Complete load-time input to variant_asset: only descriptor/draw-referenced
 * materials need resolving. Unused records are zeroed. Caller provides 64 facts;
 * count is the original shader count (<=64). Outputs publish together only on
 * success, including a valid empty plan. No allocation or frame-time lookup. */
int fh_install_actor_fog_prepare(FhPicaActorFogVariants*,
    FhPicaActorFogMaterial materials[64],unsigned *count,const void *source,
    unsigned source_bytes,unsigned source_bump_enabled,
    FhActorFogTextureWidth,void*context);
#endif
