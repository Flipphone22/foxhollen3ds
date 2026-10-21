#ifndef FH_LIGHT_WORKLOAD_H
#define FH_LIGHT_WORKLOAD_H
#include <stdint.h>
/* Private little-endian ARM lighting workload; no texture/game data included. */
typedef struct {
    char magic[8];uint32_t count;
    uint32_t channels[4][6]; /* enabled, ambient/material source, mask, diffuse, attenuation */
    float lights[8][16]; /* a[3], k[3], position[3], direction[3], color[4] */
    float ambient[2][4],material[2][4];
} FhLightWorkload;
typedef struct {float eye[3],normal[3],input[2][4],output[2][4];} FhLightSample;
_Static_assert(sizeof(FhLightWorkload)==684 && sizeof(FhLightSample)==88,"Lighting fixture format");
#endif
