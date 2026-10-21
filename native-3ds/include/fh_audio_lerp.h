#ifndef FH_AUDIO_LERP_H
#define FH_AUDIO_LERP_H
#include <stdint.h>
/* Signed 16-bit endpoints, 16-bit unsigned fraction. The unsigned product
 * fits in 32 bits (65535 squared); restore sign after the shift to preserve
 * C's truncation toward zero, including negative slopes. */
static inline int32_t fh_audio_lerp16(int16_t first,int16_t second,uint16_t fraction) {
    int32_t delta=(int32_t)second-first;
    uint32_t magnitude=delta<0?-delta:delta;
    int32_t step=(magnitude*fraction)>>16;
    return first+(delta<0?-step:step);
}
#endif
