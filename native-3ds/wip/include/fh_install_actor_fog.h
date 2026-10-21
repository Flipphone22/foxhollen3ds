#ifndef FH_INSTALL_ACTOR_FOG_H
#define FH_INSTALL_ACTOR_FOG_H
#include "fh_install_descriptors.h"
#include "fh_pica_actor_fog.h"
#include "fh_install_vertex.h"

/* Group deduplication must include the independently selected fog matrix.
 * Equal visible vertices can have different fog transforms in two source lists.
 * Original position index remains in visible.position; do not use a baked or
 * rebased GPU position as the companion stream's source. No implicit padding. */
typedef FhPicaActorFogRef FhInstallActorFogRef;
int fh_install_actor_fog_reference(const FhInstallVertexSource*,
    const FhInstallDrawSource*,const FhPicaActorFogRouting*,
    const FhPicaActorFogMatrixBank*,unsigned coordinate,const void *packet,
    unsigned bytes,FhInstallActorFogRef*);

/* Compile one selected material/light/pass alternative during model loading.
 * Resolve is called only at authored descriptor changes, not at each shader
 * selection or draw. It supplies the actual source routing for that alternative.
 * Emit consumes the source bank immediately to build vertex/palette keys;
 * callbacks must not retain pointers into this bounded stack scratch. */
typedef int (*FhInstallActorFogRoute)(void*,const FhInstallDrawSource*,FhPicaActorFogRouting*);
typedef int (*FhInstallActorFogEmit)(void*,const FhInstallDrawSource*,
    const FhPicaActorFogRouting*,const FhPicaActorFogMatrixBank*);
/* Explicit initial bank models pre-stream matrix loads; unknown slots remain
 * unknown. skip_texture_loads is the original shadow-pass decision. Returns
 * 1 complete, 0 unsupported/invalid, -1 consumer cancellation. Discard any
 * unpublished emitted data on failure. No allocation, GPU calls or live state. */
int fh_install_actor_fog(const void*,uint32_t bytes,
    const FhPicaActorFogMatrixBank *initial,unsigned skip_texture_loads,
    FhInstallActorFogRoute,FhInstallActorFogEmit,void*,unsigned *draws);
#endif
