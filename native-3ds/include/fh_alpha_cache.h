#ifndef FH_ALPHA_CACHE_H
#define FH_ALPHA_CACHE_H
#include "../vendor/melee/alpha_test.h"
/* Adapt pinned Melee renderer.c::draw_with_alpha. The complete GX predicate
 * is the key; collisions recompute, including predicates without one PICA
 * equivalent. No approximation or relaxed alpha threshold is introduced. */
static int fh_alpha_reduce_cached(unsigned packed,unsigned *function,unsigned *reference){
    static struct {unsigned key,function,reference;int simple,used;} cache[64];
    unsigned slot=(packed^(packed>>8)^(packed>>16))&63;
    if(!cache[slot].used || cache[slot].key!=packed){
        cache[slot].key=packed;cache[slot].used=1;
        cache[slot].simple=mp_alpha_reduce(packed,&cache[slot].function,&cache[slot].reference);
    }
    *function=cache[slot].function;*reference=cache[slot].reference;return cache[slot].simple;
}
#endif
