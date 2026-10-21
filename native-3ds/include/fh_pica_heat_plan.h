#ifndef FH_PICA_HEAT_PLAN_H
#define FH_PICA_HEAT_PLAN_H
#include <stdint.h>
enum {FH_HEAT_X=80,FH_HEAT_Y=224,FH_HEAT_VERTICES=81*225,
      FH_HEAT_INDICES=80*224*6};
/* Full fetch extent12: xy plus ignored padding, four signed trig attributes. */
typedef struct {uint8_t x,y;uint16_t pad;int16_t basis[4];} FhHeatVertex;
typedef struct {float phase[4],offset_x[4],offset_y[4],alpha[4];} FhHeatParameters;
/* Immutable load preparation. No game-derived byte table is shipped. */
void fh_pica_heat_mesh(FhHeatVertex *vertices,uint16_t *indices);
void fh_pica_heat_parameters(FhHeatParameters *,float scroll_x,float scroll_y,
                            int16_t pitch,uint8_t alpha);
#endif
