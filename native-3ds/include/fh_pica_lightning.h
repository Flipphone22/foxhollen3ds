#ifndef FH_PICA_LIGHTNING_H
#define FH_PICA_LIGHTNING_H
#include <stdint.h>
enum { FH_LIGHTNING_BATCH=40, FH_LIGHTNING_STORAGE=1152 };
/* Source owns endpoint generation and RNG. PICA owns corner expansion. */
void fh_pica_lightning_init(void);
void fh_pica_lightning_load(void);
void fh_pica_lightning_invalidate(void);
void fh_pica_lightning_begin(const float[3][4],const float[4][4],unsigned,uint32_t,const unsigned[4]);
void fh_pica_lightning_strand(unsigned width);
void fh_pica_lightning_point(float,float,float);
void fh_pica_lightning_end(void);
void fh_pica_lightning_shutdown(void);
#endif
