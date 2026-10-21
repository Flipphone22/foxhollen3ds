#ifndef FH_PICA_DIFFUSE_MAP_H
#define FH_PICA_DIFFUSE_MAP_H
/* Original mode-0 map inputs. Positions are source GX view-space positions,
 * not normalized directions. Unused lights are ignored by the owner. */
typedef struct {
    float ambient[4],position[8][4],color[8][4];
    unsigned count;
} FhPicaDiffuseMapState;
/* Explicit load-boundary preparation. No gameplay caller until material
 * routing and measured budgets are accepted. 8852 linear + 12288 VRAM bytes. */
void fh_pica_diffuse_map_prepare(void);
void fh_pica_diffuse_map_update(unsigned mask,const FhPicaDiffuseMapState state[3]);
/* Only after the frame owner's completion fence, as with other GPU owners. */
void fh_pica_diffuse_map_shutdown(void);
#ifdef __3DS__
#include <citro3d.h>
const C3D_Tex *fh_pica_diffuse_map_sampler(unsigned slot);
#endif
#endif
