#ifndef FH_PICA_DROPS_H
#define FH_PICA_DROPS_H
#include <stdint.h>
enum {FH_PICA_DROP_CAPACITY=30,FH_PICA_DROP_STORAGE=640};
/* Immutable point/corner IDs, not a per-frame vertex stream. */
static inline void fh_pica_drops_template(uint8_t (*v)[2],uint16_t *index){
    const unsigned order[6]={0,1,2,2,1,3};
    for(unsigned i=0;i<FH_PICA_DROP_CAPACITY;i++){
        for(unsigned c=0;c<4;c++){v[i*4+c][0]=i;v[i*4+c][1]=c;}
        for(unsigned c=0;c<6;c++)index[i*6+c]=i*4+order[c];
    }
}
static inline unsigned fh_pica_drops_color(const uint8_t ambient[3]){
    return ((unsigned)(ambient[0]>>2)+128u)|
        (((unsigned)(ambient[1]>>2)+128u)<<8)|
        (((unsigned)(ambient[2]>>2)+128u)<<16)|0x80000000u;
}
void fh_pica_drops_init(void *shader_entry);
void fh_pica_drops_load(void);
void fh_pica_drops_release(void);
void fh_pica_drops_draw(const float (*centers)[3],unsigned count,
    const float view[3][4],const float projection[4][4],const unsigned clip[4],const uint8_t ambient[3]);
void fh_pica_drops_shutdown(void);
#endif
