#ifndef FH_COMMAND_COPY_H
#define FH_COMMAND_COPY_H
#include <stdint.h>
/* CPU-only copy into the SDK command segment, not a GPU submission or flush.
 * Both regions must be word aligned, nonoverlapping, and contain words*4 bytes.
 * No reads beyond the published payload; existing owners validate capacity. */
static inline void fh_command_copy(void *destination,const void *source,unsigned words){
    uint32_t *d=destination;const uint32_t *s=source;
#if defined(__arm__)
    while(words>=8){
        __asm__ volatile("ldmia %1!, {r3-r10}\n\tstmia %0!, {r3-r10}"
            : "+r"(d), "+r"(s) :
            : "r3","r4","r5","r6","r7","r8","r9","r10","memory");
        words-=8;
    }
#endif
    while(words--)*d++=*s++;
}
#endif
