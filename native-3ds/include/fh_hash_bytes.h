#ifndef FH_HASH_BYTES_H
#define FH_HASH_BYTES_H
#include <stddef.h>
#include <stdint.h>
/* Preserve the existing word-wise FNV cache key exactly. GCC's ARM11 cost
 * model expands this constant multiply to five serial shifted additions. */
static inline uint32_t fh_hash_step(uint32_t value){
#ifdef __arm__
    uint32_t result,prime=16777619u;
    __asm__("mul %0, %1, %2":"=r"(result):"r"(value),"r"(prime));
    return result;
#else
    return value*16777619u;
#endif
}
static inline uint32_t fh_hash_bytes(uint32_t hash,const void *memory,size_t size){
    const unsigned char *p=memory;
    if(!((uintptr_t)p&3)){
        while(size>=4){uint32_t word;__builtin_memcpy(&word,p,4);hash=fh_hash_step(hash^word);p+=4;size-=4;}
    }else{
        /* Explicit byte loads retain bounds on unaligned CPU input. */
        while(size>=4){const volatile unsigned char *q=p;
            uint32_t word=q[0]|(uint32_t)q[1]<<8|(uint32_t)q[2]<<16|(uint32_t)q[3]<<24;
            hash=fh_hash_step(hash^word);p+=4;size-=4;}
    }
    while(size--)hash=fh_hash_step(hash^*p++);
    return hash;
}
#endif
