#ifndef FH_PICA_STARS_H
#define FH_PICA_STARS_H
#include <stdint.h>
void fh_pica_stars_init(void *entry);
void fh_pica_stars_allocate(void);
void fh_pica_stars_point(unsigned group,unsigned point,int16_t x,int16_t y,int16_t z);
void fh_pica_stars_commit(void);
void fh_pica_stars_group(unsigned group,unsigned rgba,unsigned size);
void fh_pica_stars_draw(unsigned first,unsigned page_a,unsigned page_b,
    const float projection[4][4],const float view[3][4],const float viewport[6],const unsigned clip[4]);
void fh_pica_stars_shutdown(void);
#endif
