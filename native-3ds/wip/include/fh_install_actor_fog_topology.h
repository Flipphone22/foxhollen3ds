#ifndef FH_INSTALL_ACTOR_FOG_TOPOLOGY_H
#define FH_INSTALL_ACTOR_FOG_TOPOLOGY_H
#include <stdio.h>
/* Startup-owned reader for the existing prepared render-topology pack.
 * Reapply validated patches to one decoded ISO model; never repeat reduction.
 * At most 96 KiB directory and 1 MiB transient patch, no GPU allocation. */
typedef struct {FILE*file;unsigned char*directory;unsigned count,bytes,applied;} FhInstallActorFogTopology;
int fh_install_actor_fog_topology_open(FhInstallActorFogTopology*,const char*);
int fh_install_actor_fog_topology_apply(FhInstallActorFogTopology*,void*,unsigned,unsigned);
void fh_install_actor_fog_topology_close(FhInstallActorFogTopology*);
#endif
