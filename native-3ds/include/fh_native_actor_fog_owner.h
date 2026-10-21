#ifndef FH_NATIVE_ACTOR_FOG_OWNER_H
#define FH_NATIVE_ACTOR_FOG_OWNER_H
#include "fh_native_scene.h"
#include "fh_native_actor_fog.h"
#include "fh_pica_actor_fog_variants.h"
#include "fh_actor_fog_resident.h"
#include "fh_actor_fog_collection.h"
/* Prepared-asset load boundary. The ordinary scene must already be registered;
 * packet identity must match its original bytes/CRC. Variant describes an
 * explicitly resolved source material/light alternative, not a guessed index.
 * No source compiler runs here. Only the explicit load entry invokes a reader;
 * lookups and draw dispatch perform no filesystem access. */
/* Load-time immutable source-material generation. Nonzero generation changes
 * when resolved source facts change. No geometry is allocated by lookup. */
int fh_native_scene_actor_fog_prepare(const FhSceneAsset*,
    const FhPicaActorFogVariants*,unsigned generation);
int fh_native_scene_actor_fog_publish(const FhSceneAsset*,unsigned generation,const void *metadata,
    unsigned metadata_bytes,const void *geometry,unsigned geometry_bytes,
    unsigned variant);
/* Constant-time scene handle lookup. The caller still matches the current
 * material/light alternative and uses the live actor admission checks. A
 * source write or switched model view makes the immutable fog packet unusable.
 * Returned storage survives until the usual scene/GPU retirement boundary. */
const FhActorFogAsset *fh_native_scene_actor_fog_find(const FhSceneAsset*,unsigned generation,unsigned variant);
const FhActorFogAsset *fh_native_scene_actor_fog_select(const FhSceneAsset*,unsigned generation,
    const FhPicaActorFogSelection*);
/* Zero until at least one packet has been published. Respects the same source
 * mutation/model-view gates as selection. Does not compile or inspect materials. */
unsigned fh_native_scene_actor_fog_generation(const FhSceneAsset*);
/* Prepared packet stream, outside drawing. Failed read/CRC/budget preserves an
 * existing variant; successful publication alone exposes a new generation. */
int fh_native_scene_actor_fog_load(const FhSceneAsset*,unsigned generation,
    unsigned metadata_bytes,unsigned geometry_bytes,unsigned packet_crc,
    FhActorFogRead,void *context,unsigned variant);
/* One-shot model-load admission from a prepared file. Returns resident count,
 * zero for a missing/empty file or budget rejection, -1 for invalid input.
 * Partial budget admission is explicit; absent variants use ordinary rendering.
 * Does not replace an existing bank. Call before child material compilation. */
int fh_native_scene_actor_fog_file(const FhSceneAsset*,const char*path,unsigned bump_enabled);
/* Same load admission/lifetime from an already indexed collection. Borrows it
 * only until the call returns; resident data retains no FILE/index pointers. */
int fh_native_scene_actor_fog_collection(const FhSceneAsset*,
    const FhActorFogCollection*,unsigned bump_enabled);
/* Selected-scene archive indexed once per renderer session. Paths cannot
 * change mid-session; geometry selection already requires restart. */
int fh_native_scene_actor_fog_archive(const FhSceneAsset*,const char*path,unsigned bump_enabled);
#endif
