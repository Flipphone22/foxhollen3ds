/* Execute the exact production service body with captured call boundaries.
 * SDK publication and retirement themselves are covered by the ARM fixture. */
#include "fh_audio_timeline.h"
#include <assert.h>
#include <stdio.h>
typedef struct FhAudioPcmQueue { unsigned unused; } FhAudioPcmQueue;
static FhAudioSourceWindow window;
static unsigned source_epoch,playback_epoch,callback_fault,callback_windows,callback_gaps;
static struct {unsigned faults,prefix_faults;} voices;
static struct {unsigned faults;} aux;
static unsigned effect_route_token=1,tagged,playback_effect_token;
static void fh_audio_dsp_aux_tag(void *p,uint32_t token){assert(p==&aux);tagged=token;}
static struct {uint64_t epoch,first,end;unsigned leading,available,calls;} served;
static FhAudioSourceWindow fh_audio_pcm_source_window(const FhAudioPcmQueue *q,unsigned channel){(void)q;assert(channel==0);return window;}
static void mapping_journal(const FhAudioPcmQueue *q,const FhAudioSourceWindow *w){(void)q;(void)w;}
static void fh_audio_dsp_voices_service_region(void *v,uint64_t epoch,uint64_t first,uint64_t end,unsigned leading,int available){
    assert(v==&voices);served.epoch=epoch;served.first=first;served.end=end;
    served.leading=leading;served.available=available;served.calls++;
}
#include "service.c"
int main(void){
    FhAudioPcmQueue q={0};unsigned checks=0;
    for(unsigned generation=1;generation<1000;generation++){
        /* New generation is prepared while output still has no provenance. */
        source_epoch=generation;window=(FhAudioSourceWindow){0};
        fh_audio_game_dsp_service(&q);
        assert(served.epoch==generation-1 && !served.available && !callback_fault);checks++;
        window=(FhAudioSourceWindow){.frames=156,.count=1,.valid=1,
            .spans={{.source_frame=0,.generation=generation,.offset=0,.frames=156,.effect_token=generation}}};
        fh_audio_game_dsp_service(&q);
        assert(served.epoch==generation && served.available && served.first==0 && served.end==156);checks++;
        /* Same-generation movie/silence prefix is retained verbatim. */
        window=(FhAudioSourceWindow){.frames=157,.count=2,.valid=1,
            .spans={{.generation=0,.offset=0,.frames=4},
                {.source_frame=156,.generation=generation,.offset=4,.frames=153,.effect_token=generation}}};
        fh_audio_game_dsp_service(&q);
        assert(served.epoch==generation && served.available && served.first==156 && served.end==309 && served.leading==4);checks++;
        source_epoch=generation+1;
        window=(FhAudioSourceWindow){.frames=156,.count=1,.valid=1,.spans={{.frames=156}}};
        fh_audio_game_dsp_service(&q);
        assert(served.epoch==generation && !served.available && !callback_fault);checks++;
    }
    /* Two audible generations in one DSP block remain explicitly unsupported;
     * do not claim this queue repair makes their waveform boundary correct. */
    window=(FhAudioSourceWindow){.frames=156,.count=2,.valid=1,
        .spans={{.generation=999,.frames=4},{.generation=1000,.offset=4,.frames=152}}};
    fh_audio_game_dsp_service(&q);assert(callback_fault==3 && !served.available && served.epoch==999);checks++;
    assert(callback_windows==1998 && callback_gaps==1999);
    printf("{\"checks\":%u,\"producer_epoch_not_used_for_gaps\":true,\"mixed_epoch_still_rejected\":true}\n",checks);
}
