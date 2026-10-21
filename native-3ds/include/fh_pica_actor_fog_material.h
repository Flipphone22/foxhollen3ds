#ifndef FH_PICA_ACTOR_FOG_MATERIAL_H
#define FH_PICA_ACTOR_FOG_MATERIAL_H
#include "fh_pica_actor_fog.h"
/* Source material facts, compiled after original texture references resolve.
 * Widths are original GX image widths, not reduced native texture dimensions.
 * Environment bits correspond to the actual texture0/texture1 references. */
typedef struct {
    unsigned environment,auxiliary,bump_enabled,bump_parameters;
    unsigned layers,textured_layers,decal,base_width,auxiliary_width;
} FhPicaActorFogMaterial;
/* Facts from the original light-selection/game-state owner. A selected light
 * with no projection image still affects descriptor routing, but consumes no
 * projected texture coordinate. No GX state or vertex stream is interpreted. */
typedef struct {
    unsigned selected_lights,projection_images,shadow_near,render_flags,cast_shadow;
} FhPicaActorFogSelection;
typedef struct {FhPicaActorFogRouting route;unsigned coordinate;} FhPicaActorFogAlternative;
/* Ordinary actor pass only. Used to select/build immutable alternatives when
 * their compact state key changes, not per draw. A failure preserves output.
 * Does not admit a native material, perform rendering, or replace light logic. */
int fh_pica_actor_fog_material_alternative(FhPicaActorFogAlternative*,unsigned joints,
    unsigned texture_matrices,const FhPicaActorFogMaterial*,const FhPicaActorFogSelection*);
#endif
