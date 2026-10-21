#ifndef FH_PICA_ACTOR_COMMAND_OWNER_H
#define FH_PICA_ACTOR_COMMAND_OWNER_H
#include "fh_pica_actor_commands.h"
/* Retained native actor owner. Begin publishes shared SDK state; sync updates
 * live camera/light state without retiring compatible native geometry/material.
 * Every actor must force its first pose publication. Sync retires on shader,
 * code or target change; split/APT/SDK draw/teardown also end the owner.
 * State storage must outlive the batch, and resource retirement must end it.
 * No internal submit/wait/retry; failed draws must never omit geometry. */
int fh_pica_actor_stream_begin(FhPicaActorCommandState*);
int fh_pica_actor_stream_sync(FhPicaActorCommandState*);
int fh_pica_actor_stream_environment(FhPicaActorParameters*);
unsigned fh_pica_actor_stream_draw(const FhPicaActorGeometry*,const FhPicaActorParameters*,const FhPicaActorPose*,unsigned);
void fh_pica_actor_stream_end(void);
int fh_pica_actor_stream_active(void);
#endif
