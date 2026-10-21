#ifndef FH_PICA_HEAT_H
#define FH_PICA_HEAT_H
unsigned fh_pica_heat_enabled(void);
void fh_pica_heat(float scroll_x,float scroll_y,int pitch,unsigned alpha,
    unsigned threshold,const float projection[4][4]);
#endif
