#include "fh_audio_voice_window.h"

int fh_audio_voice_window(const FhAudioSourceWindow *w,FhAudioSpan *result) {
    if(!w || !result)return -1;
    if(!w->valid)return 0;
    if(w->valid!=1 || !w->frames || w->frames>160 || !w->count || w->count>FH_AUDIO_MAP_SPANS)return -1;
    FhAudioSpan region={0};unsigned covered=0;uint32_t last_effect=0;
    for(unsigned i=0;i<w->count;i++) {
        const FhAudioSpan *s=&w->spans[i];
        if(!s->frames || s->offset!=covered || s->frames>w->frames-covered)return -1;
        covered+=s->frames;
        if(!s->generation) {
            if(s->effect_token || region.generation)return -1;
            continue;
        }
        if(s->source_frame>UINT64_MAX-s->frames)return -1;
        if(!region.generation)region=*s;
        else {
            if(s->generation!=region.generation || s->source_frame!=region.source_frame+region.frames ||
               (!s->effect_token)!=(!region.effect_token) || s->effect_token<last_effect)return -1;
            region.frames+=s->frames;
        }
        last_effect=s->effect_token;
    }
    if(covered!=w->frames)return -1;
    if(!region.generation)return 0;
    *result=region;return 1;
}
