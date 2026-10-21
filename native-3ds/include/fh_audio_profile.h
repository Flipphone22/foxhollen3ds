#ifndef FH_AUDIO_PROFILE_H
#define FH_AUDIO_PROFILE_H
#include <stdint.h>
extern uint64_t fh_audio_profile_ticks[4];
#ifdef FH_AUDIO_PROFILE
#include "fh_platform.h"
static inline uint64_t fh_mix_begin(unsigned stage) {
    static unsigned sequence[4];
    return !(sequence[stage]++&15)?fh_platform_ticks():0;
}
static inline void fh_mix_stage(unsigned stage,uint64_t start) { if(start)fh_audio_profile_ticks[stage]+=(fh_platform_ticks()-start)*16; }
#else
static inline uint64_t fh_mix_begin(unsigned stage) { (void)stage;return 0; }
static inline void fh_mix_stage(unsigned stage,uint64_t start) { (void)stage;(void)start; }
#endif
#endif
