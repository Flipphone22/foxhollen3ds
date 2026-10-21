#ifndef FH_UNIFORM_ROWS_H
#define FH_UNIFORM_ROWS_H
#include <stdint.h>
#include <string.h>
/* Melee uniform_upload.h compares the rows actually sent, including rows
 * unused by intervening shader branches. Native little-endian words here.
 * Invalidate on every program bind: shader constants can overwrite registers
 * independently of the application's cached values. */
typedef struct { uint32_t words[96][4]; unsigned char valid[96]; } FhUniformRows;
static inline void fh_uniform_rows_reset(FhUniformRows *cache){
    memset(cache->valid,0,sizeof(cache->valid));
}
static inline int fh_uniform_row_changed(FhUniformRows *cache,unsigned row,const float values[4]){
    if(row>=96)return 1;
    uint32_t words[4];memcpy(words,values,sizeof(words));
    if(cache->valid[row] && !memcmp(cache->words[row],words,sizeof(words)))return 0;
    memcpy(cache->words[row],words,sizeof(words));cache->valid[row]=1;return 1;
}
#endif
