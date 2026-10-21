#ifndef FH_PICA_ACTOR_FOG_VARIANTS_H
#define FH_PICA_ACTOR_FOG_VARIANTS_H
#include "fh_pica_actor_fog_material.h"
/* One immutable material generation. Slot zero means no prepared variant;
 * valid IDs are 1..count. Used-material bits must include descriptor changes,
 * not just materials that ultimately emit triangles (source descriptors persist).
 * Rebuild outside submission if resolved source material facts change. */
enum {FH_ACTOR_FOG_VARIANT_CELLS=81};
typedef struct {
    uint8_t slots[FH_ACTOR_FOG_VARIANT_CELLS];
    uint8_t representative[FH_ACTOR_FOG_VARIANT_CELLS];
    uint8_t count,reserved;
} FhPicaActorFogVariants;
/* Load-time only. Deduplicates full routing and coordinate for every used
 * material. Hashes accelerate comparison but never establish equivalence.
 * Invalid arguments preserve output; unsupported routes have slot zero. */
int fh_pica_actor_fog_variants(FhPicaActorFogVariants*,unsigned joints,
    unsigned texture_matrices,const FhPicaActorFogMaterial*,unsigned materials,
    const uint32_t used[2]);
/* Frame-time fixed table lookup; no material scan, allocation, GPU or GX work.
 * Selection comes from the original game-state adapter, including shadow policy.
 * This returns a key, not residency or native-material admission. */
unsigned fh_pica_actor_fog_variant(const FhPicaActorFogVariants*,
    const FhPicaActorFogSelection*);
/* Representative selection for compiling the immutable packet at conversion/
 * load. It is not a replacement for actual game light selection or simulation. */
int fh_pica_actor_fog_variant_selection(FhPicaActorFogSelection*,
    const FhPicaActorFogVariants*,unsigned variant);
#endif
