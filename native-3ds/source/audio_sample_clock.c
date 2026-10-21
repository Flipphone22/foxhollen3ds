#include "fh_audio_sample_clock.h"

int fh_audio_sample_clock_init(FhAudioSampleClock *s,uint32_t length,uint32_t loop_start,uint32_t loop_length) {
    if(!s || !length || loop_start>length || loop_length>length-loop_start)return 0;
    *s=(FhAudioSampleClock){.end=loop_length?loop_start+loop_length:length,
                           .loop_start=loop_start,.loop_length=loop_length};
    return 1;
}
int fh_audio_sample_clock_step(FhAudioSampleClock *s,uint32_t frames,uint32_t pitch,
                               unsigned mode,FhAudioSampleStep *step) {
    if(!s || !step || frames>160 || mode>FH_AUDIO_CLOCK_NONE ||
       (mode!=FH_AUDIO_CLOCK_NONE && pitch>6u*65536))return 0;
    if(mode==FH_AUDIO_CLOCK_NONE)pitch=65536;
    /* Under the admitted pitch bound, each source u32 phase addition cannot
     * overflow. Summing in 64 bits exactly matches all per-output additions. */
    uint64_t accumulated=s->phase+(uint64_t)frames*pitch;
    uint32_t count=accumulated>>16;
    s->phase=(uint32_t)accumulated&65535;
    s->history_index+=count; /* Intentional original u32 ring-index wrap. */
    uint32_t remaining=s->end-s->position;
    if(count<=remaining && !s->ended)s->position+=count;
    else if(s->loop_length) {
        uint32_t wrapped=count-remaining;
        /* Source wraps BEFORE the next read. Exactly reaching the end keeps
         * position=end until another sample is requested; don't modulo early. */
        s->position=s->loop_start+1+(wrapped-1)%s->loop_length;
    } else {
        uint32_t zeros=s->ended?count:count-remaining;
        s->position=s->end;
        if(zeros)s->ended=1;
        unsigned tail=s->tail+zeros;s->tail=tail>4?4:tail;
    }
    unsigned taps=mode==FH_AUDIO_CLOCK_NONE?1:mode==FH_AUDIO_CLOCK_LINEAR?2:4;
    *step=(FhAudioSampleStep){count,s->tail>=taps};
    return 1;
}
