#ifndef FH_INSTALL_ACTOR_FOG_POSE_H
#define FH_INSTALL_ACTOR_FOG_POSE_H
#include "fh_install_vertex.h"
#include "fh_pica_actor_fog_plan.h"
/* Compile selected source keys, not all original joint matrices. Source must
 * come from fh_install_vertex_source. World preloads require distinct keys and
 * are not admitted here as fake extra joints. Failure leaves output untouched. */
int fh_install_actor_fog_pose_plan(FhPicaActorFogPosePlan*,unsigned capacity,
    const FhInstallVertexSource*,const FhPicaActorFogMatrix*,unsigned count);
#endif
