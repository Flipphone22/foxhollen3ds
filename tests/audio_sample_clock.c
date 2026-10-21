/* Include the unmodified original mixer so its actual private resampler is the
 * state oracle. Its waveform still executes; this test compares only the state
 * fields the native clock promises, NOT reconstructed ADPCM/filter history. */
#include "hw_pc.c"
#include "fh_audio_sample_clock.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static unsigned seed=0x803216;
static unsigned random_word(void){seed=1664525*seed+1013904223;return seed;}
static unsigned choice(unsigned n){return (random_word()>>8)%n;}
static volatile uint32_t waveform_checksum;
static unsigned calls,frames_total,ended_calls,loops;
static void check(DSPvoice *voice,FhAudioSampleClock *clock,unsigned frames,unsigned pitch) {
    VoiceResamplerState *s=&voiceResampler[0];
    s32 output[160]={0};FhAudioSampleStep step;
    assert(resampleVoice(voice,0,output,frames,pitch)==(int)frames);
    assert(fh_audio_sample_clock_step(clock,frames,pitch,voice->srcTypeSelect,&step));
    assert(clock->position==s->reader.position);
    assert(clock->phase==s->phase && clock->history_index==s->historyIndex);
    assert(clock->tail==s->tail && clock->ended==s->reader.ended);
    assert(voice->playInfo.posHi==clock->position && voice->playInfo.posLo==clock->phase);
    unsigned taps=voice->srcTypeSelect==SAL_SRC_NONE?1:voice->srcTypeSelect==SAL_SRC_LINEAR?2:4;
    assert(step.sample_done==(unsigned)(s->tail>=taps));
    for(unsigned i=0;i<frames;i++)waveform_checksum+=(uint32_t)output[i];
    calls++;frames_total+=frames;ended_calls+=s->reader.ended;
}
int main(void) {
    _Static_assert(FH_AUDIO_CLOCK_NONE==SAL_SRC_NONE && FH_AUDIO_CLOCK_LINEAR==SAL_SRC_LINEAR &&
                   FH_AUDIO_CLOCK_POLYPHASE==SAL_SRC_POLYPHASE,"Source interpolation mapping");
    unsigned char encoded[1024];DSPADPCMplusInfo info={0};
    for(unsigned i=0;i<sizeof(encoded);i++)encoded[i]=random_word()>>24;
    for(unsigned i=0;i<8;i++){info.coefTab[i][0]=(s16)(random_word()>>16);info.coefTab[i][1]=(s16)(random_word()>>16);}
    info.initialPS=0x31;info.loopPS=0x42;info.loopY0=-300;info.loopY1=1234;
    for(unsigned f=0;f<3;f++)for(unsigned p=0;p<128;p++)for(unsigned t=0;t<4;t++)
        voicePolyphaseTable[f][p][t]=(s32)(random_word()&0x1fffff);
    for(unsigned trial=0;trial<4096;trial++) {
        unsigned length=1+choice(512),loop_start=choice(length),loop_length=trial%3?1+choice(length-loop_start):0;
        loops+=loop_length!=0;
        DSPvoice voice={0};voice.smp_info=(SAMPLE_INFO){.addr=encoded,.extraData=&info,.length=length,
            .loop=loop_start,.loopLength=loop_length,.compType=SAMPLE_TYPE_ADPCM};
        VoiceResamplerState *s=&voiceResampler[0];memset(s,0,sizeof(*s));
        assert(salPCInitReader(&s->reader,&voice.smp_info));
        FhAudioSampleClock clock;assert(fh_audio_sample_clock_init(&clock,length,loop_start,loop_length));
        clock.phase=s->phase=trial%5?choice(65536):65535;
        clock.history_index=s->historyIndex=trial%7?random_word():UINT32_MAX-3;
        for(unsigned part=0;part<16;part++) {
            voice.srcTypeSelect=choice(3);voice.srcCoefSelect=choice(4);
            unsigned frames=part?choice(161):0,pitch=choice(6*65536+1);
            if(part%7==0)pitch=0;
            if(voice.srcTypeSelect==SAL_SRC_NONE && part%3==0)pitch=UINT32_MAX;
            check(&voice,&clock,frames,pitch);
        }
    }
    /* Exact end is not a wrap/end notification. The next source read is. */
    for(unsigned looping=0;looping<2;looping++) {
        DSPvoice voice={0};voice.smp_info=(SAMPLE_INFO){.addr=encoded,.extraData=&info,.length=28,
            .loop=14,.loopLength=looping?14:0,.compType=SAMPLE_TYPE_ADPCM};
        voice.srcTypeSelect=SAL_SRC_LINEAR;memset(voiceResampler,0,sizeof(voiceResampler));
        assert(salPCInitReader(&voiceResampler[0].reader,&voice.smp_info));
        FhAudioSampleClock clock;assert(fh_audio_sample_clock_init(&clock,28,14,looping?14:0));
        check(&voice,&clock,28,65536);assert(clock.position==28 && !clock.ended && !clock.tail);
        check(&voice,&clock,0,65536);check(&voice,&clock,1,65536);
        assert(clock.position==(looping?15:28) && clock.tail==(looping?0:1));
        check(&voice,&clock,1,65536);
    }
    FhAudioSampleClock clock,before;FhAudioSampleStep step={0xa5a5,0x7171},old_step=step;
    assert(fh_audio_sample_clock_init(&clock,128,0,0));before=clock;
    assert(!fh_audio_sample_clock_init(&clock,0,0,0) && !memcmp(&clock,&before,sizeof(clock)));
    assert(!fh_audio_sample_clock_init(&clock,UINT32_MAX,UINT32_MAX,1) && !memcmp(&clock,&before,sizeof(clock)));
    assert(!fh_audio_sample_clock_step(&clock,161,65536,SAL_SRC_LINEAR,&step));
    assert(!fh_audio_sample_clock_step(&clock,1,6*65536+1,SAL_SRC_LINEAR,&step));
    assert(!fh_audio_sample_clock_step(&clock,1,65536,3,&step));
    assert(!memcmp(&clock,&before,sizeof(clock)) && !memcmp(&step,&old_step,sizeof(step)));
    printf("calls=%u output_samples=%u ended_calls=%u loop_cases=%u waveform_checksum=%u state_bytes=%zu\n",
           calls,frames_total,ended_calls,loops,waveform_checksum,sizeof(clock));
    return 0;
}
