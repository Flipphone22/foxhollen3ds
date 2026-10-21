#ifndef FH_PICA_ACTOR_FOG_GAME_H
#define FH_PICA_ACTOR_FOG_GAME_H
#include "fh_pica_actor_fog_material.h"
struct ObjModel;
struct GameObject;
/* Ordinary actor adapter. No GX calls, allocations, GPU writes or retained game
 * pointers. Capture material facts after source textures resolve, and refresh
 * if the source material references change. Widths belong to original textures.
 * Call selection on the game thread once per object publication, only when
 * actor fog is admitted. The original selection helper updates light scores
 * and intensity; it is not a read-only or render-worker operation.
 * near_shadow is the current source culling result, not a previous draw's global.
 * Failure preserves output. Custom render callbacks are not admitted here. */
int fh_pica_actor_fog_game_material(FhPicaActorFogMaterial*,
    const struct ObjModel*, unsigned material_index, unsigned source_bump_enabled);
int fh_pica_actor_fog_game_selection(FhPicaActorFogSelection*,
    const struct ObjModel*, struct GameObject*, unsigned near_shadow);
#endif
