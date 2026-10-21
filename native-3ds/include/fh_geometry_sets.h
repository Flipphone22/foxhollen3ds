#ifndef FH_GEOMETRY_SETS_H
#define FH_GEOMETRY_SETS_H
#include <stdint.h>
static inline unsigned fh_geometry_primary_set(uintptr_t address,unsigned mask){
    return ((address>>4)^(address>>13))&mask;
}
static inline unsigned fh_geometry_secondary_set(uintptr_t address,unsigned mask){
    uint32_t h=(uint32_t)address;h^=h>>16;h*=0x85ebca6bu;h^=h>>13;h*=0xc2b2ae35u;h^=h>>16;
    unsigned first=fh_geometry_primary_set(address,mask),second=h&mask;
    return second==first?second^1u:second;
}
#endif
