#ifndef FH_INSTALL_COMPACT_H
#define FH_INSTALL_COMPACT_H
#include "fh_install_skin.h"
#include "fh_native_scene.h"
/* Direct installation output: no intermediate float mesh. The group compiler
 * supplies its bounded palette. Two-UV references use material as UV1. */
int fh_install_compact_world(const FhInstallVertexSource*,const FhModelRef*,unsigned two_uv,FhWorldCompactVertex*);
/* Source bone IDs and byte/128 weights used by installation-time grouping.
 * Does not map them to a group palette or alter the original coefficients. */
int fh_install_compact_weights(const FhInstallVertexSource*,const FhModelRef*,
    const FhInstallSkinWeights*,const FhInstallSkinWeights*,unsigned out[4]);
/* Fixed-world actors require complete position/normal maps from skin_stream;
 * output then uses FHNSCN0B byte/128 weights. Other actors use FHNSCN0A quarters.
 * Independent normal deformation or unrepresentable weights reject, never
 * silently share/normalize. A failing call leaves output unchanged. */
int fh_install_compact_actor(const FhInstallVertexSource*,const FhModelRef*,unsigned two_uv,
    const uint16_t *bones,unsigned count,const FhInstallSkinWeights *positions,
    const FhInstallSkinWeights *normals,FhModelCompactVertex*);
#endif
