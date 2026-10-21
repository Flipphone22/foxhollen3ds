#ifndef FH_VERTEX_COPY_H
#define FH_VERTEX_COPY_H
#include "fh_graphics.h"

/* Typed CPU staging buffers are word aligned. Like Melee's bounded multiword
 * memory operations, these loads/stores touch exactly the requested object.
 * No BE8 conversion, unaligned access, VFP state, or GPU memory is involved. */
_Static_assert(sizeof(FhVertex)==148 && _Alignof(FhVertex)>=4,"Vertex copy layout");
static __attribute__((noinline)) void fh_vertex_copy(FhVertex *out,const FhVertex *in) {
#ifdef __arm__
    __asm__ volatile(
        ".rept 4\n"
        "ldmia %1!, {r2-r9}\n"
        "stmia %0!, {r2-r9}\n"
        ".endr\n"
        "ldmia %1!, {r2-r5}\n"
        "stmia %0!, {r2-r5}\n"
        "ldr r2, [%1]\n"
        "str r2, [%0]\n"
        : "+&r"(out), "+&r"(in)
        : : "r2","r3","r4","r5","r6","r7","r8","r9","memory");
#else
    *out=*in;
#endif
}
#endif
