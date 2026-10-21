#include "fh_audio_source_bridge.h"
#include "musyx/hardware.h"
#include <string.h>
FhAudioSourceBridge *fh_audio_source_bridge;
int fh_audio_source_stop(unsigned index,uint64_t frame) {
    FhAudioSourceBridge *b=fh_audio_source_bridge;
    if(!b)return 1;
    if(index>=SYNTH_MAX_VOICES){b->faults++;return 0;}
    FhAudioSourceVoice *v=&b->voices[index];if(!v->token)return 1;
    if(!b->backend.stop || !b->backend.stop(b->backend.context,v->token,frame)){b->faults++;return 0;}
    memset(v,0,sizeof(*v));b->stops++;return 1;
}
int fh_audio_source_begin(unsigned index,const DSPvoice *voice,uint64_t frame,unsigned rate) {
    FhAudioSourceBridge *b=fh_audio_source_bridge;if(!b)return 1;
    if(!voice || index>=SYNTH_MAX_VOICES){b->faults++;return 0;}
    /* Reusing a source slot cannot overwrite its preceding native generation. */
    if(!fh_audio_source_stop(index,frame))return 0;
    FhAudioSourceVoice *v=&b->voices[index];const SAMPLE_INFO *s=&voice->smp_info;
    if(rate!=32000 || s->compType!=SAMPLE_TYPE_ADPCM || !s->addr || !s->extraData ||
       !b->backend.begin || !b->backend.segment || !b->backend.stop ||
       !fh_audio_sample_clock_init(&v->clock,s->length,s->loop,s->loopLength)) {
        b->declined++;return 1;
    }
    uint64_t token=b->backend.begin(b->backend.context,index,voice,frame);
    if(!token){b->declined++;return 1;}
    v->sample=*s;v->token=token;b->accepted++;return 1;
}
int fh_audio_source_segment(unsigned index,DSPvoice *voice,const FhAudioSourceSegment *segment) {
    FhAudioSourceBridge *b=fh_audio_source_bridge;if(!b)return -1;
    if(index>=SYNTH_MAX_VOICES || !voice || !segment){b->faults++;return -2;}
    FhAudioSourceVoice *v=&b->voices[index];if(!v->token)return -1;
    const SAMPLE_INFO *s=&voice->smp_info,*saved=&v->sample;
    if(s->addr!=saved->addr || s->extraData!=saved->extraData || s->compType!=saved->compType ||
       s->length!=saved->length || s->loop!=saved->loop || s->loopLength!=saved->loopLength ||
       segment->cycle_frames!=160 || segment->offset>160 || segment->frames>160-segment->offset) {
        b->faults++;return -2;
    }
    FhAudioSampleClock next=v->clock;FhAudioSampleStep step={0};
    if(!segment->held && !fh_audio_sample_clock_step(&next,segment->frames,segment->pitch,voice->srcTypeSelect,&step)) {
        b->faults++;return -2;
    }
    if(!b->backend.segment(b->backend.context,v->token,voice,segment)){b->faults++;return -2;}
    v->clock=next;
    /* A zero-pitch source segment returns before modifying playInfo. Still
     * publish its hold so a previously playing hardware voice does not run on. */
    if(!segment->held){voice->playInfo.posHi=next.position;voice->playInfo.posLo=next.phase;}
    b->segments++;return step.sample_done;
}
