#ifndef FH_STACK_WATERMARK_H
#define FH_STACK_WATERMARK_H
#include <stdint.h>
#include <string.h>
/* Exact first changed byte, with no reads outside the painted interval.
 * memcpy preserves aliasing legality; aligned blocks become ARM word loads. */
static inline unsigned fh_stack_watermark_prefix(const void *memory,unsigned bytes){
    const unsigned char *p=memory;unsigned n=0;
    while(n<bytes && ((uintptr_t)(p+n)&3)){
        if(p[n]!=0xa5)return n;
        n++;
    }
    while(bytes-n>=16){
        uint32_t w[4];memcpy(w,__builtin_assume_aligned(p+n,4),sizeof w);
        if((w[0]^0xa5a5a5a5u)|(w[1]^0xa5a5a5a5u)|(w[2]^0xa5a5a5a5u)|(w[3]^0xa5a5a5a5u))break;
        n+=16;
    }
    while(bytes-n>=4){
        uint32_t w;memcpy(&w,__builtin_assume_aligned(p+n,4),sizeof w);
        if(w!=0xa5a5a5a5u)break;
        n+=4;
    }
    while(n<bytes && p[n]==0xa5)n++;
    return n;
}
#endif
