#ifndef FH_HEAP_PROFILE_H
#define FH_HEAP_PROFILE_H
#include <stdint.h>
typedef struct {unsigned normal_mib,linear_mib,large;} FhHeapProfile;
static inline FhHeapProfile fh_heap_profile(int64_t maximum,int64_t current){
    /* Legacy fallback is the existing console-tested 46+6 MiB reservation.
     * Expanded profiles include code/current commit in the 15% process reserve.
     * The kernel grant, not an executable flag or emulator setting, is authority. */
    /* The palace transition exhausted the 10 MiB linear admission budget
     * with 1.5 MiB still reserved. Use otherwise uncommitted process space,
     * never the normal heap or the GPU resource reserve. Larger executables
     * that cannot retain 15% process headroom keep the smaller profile. */
    /* Old80 with the shipping executable has >8 MiB initial commit: 60 MiB
     * heaps exceed the 85% process gate and formerly fell to 48+10. A 47+12
     * split fits that grant with 15% process headroom and gives live GPU
     * assets two more MiB, retaining each allocator's own 15% admission gate.
     * New124 and the low-entitlement fallback remain independently selected. */
    const FhHeapProfile profiles[]={{68,24,1},{52,16,0},{47,12,0},{48,10,0}};
    if(current>=0 && maximum>current && maximum<=1024LL*1024*1024){
        for(unsigned i=0;i<sizeof(profiles)/sizeof(profiles[0]);i++){
            int64_t need=current+(int64_t)(profiles[i].normal_mib+profiles[i].linear_mib)*1024*1024;
            if(need*100<=maximum*85)return profiles[i];
        }
    }
    return (FhHeapProfile){46,6,0};
}
#endif
