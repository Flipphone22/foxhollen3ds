#ifndef FH_NATIVE_LIGHTING_H
#define FH_NATIVE_LIGHTING_H
#include <stdint.h>
typedef struct { float a[3],k[3],position[3],direction[3],color[4]; } FhLight;
typedef struct { uint32_t enabled,ambient_source,material_source,mask,diffuse,attenuation; } FhChannel;
/* PICA c74..94 and fixed v6..11. Eight lights; three may have nonconstant
 * attenuation. No vertex work or game-buffer ownership on the CPU. */
typedef struct { float uniform[21][4],fixed[6][4]; unsigned booleans,lights,complex; } FhNativeLighting;
int fh_native_lighting_pack(FhNativeLighting *out,const FhLight lights[8],
    const FhChannel channels[4],const float ambient[4],const float material[4]);
int fh_gx_native_lighting(FhNativeLighting *out);
#endif
