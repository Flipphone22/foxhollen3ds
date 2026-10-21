#ifndef FH_PICA_SPECULAR_MAP_H
#define FH_PICA_SPECULAR_MAP_H
#include <stdint.h>
/* Load-time immutable topology for Rcp_DrawWarpDistortionMesh. The runtime
 * target/material owner is not connected yet; this is not rendered parity. */
enum { FH_SPECULAR_MAP_VERTICES=17*17, FH_SPECULAR_MAP_INDICES=16*16*6 };
typedef struct {float position[2],normal[3];} FhPicaSpecularMapVertex;
typedef struct {
    FhPicaSpecularMapVertex vertices[FH_SPECULAR_MAP_VERTICES];
    uint16_t indices[FH_SPECULAR_MAP_INDICES];
} FhPicaSpecularMapMesh;
void fh_pica_specular_map_prepare(FhPicaSpecularMapMesh*);
#endif
