#include "fh_audio_dsp_aux.h"
#include <3ds.h>
#include <string.h>

/* Exported by the exact SDK ndsp.o verified by the fixture/build tool. These
 * pointers are populated by DSP_ConvertProcessAddressFromDspDram at init/wake. */
extern void *ndspVars[16][2];
extern u16 ndspBufferId,ndspBufferCurId,ndspFrameId;
static FhAudioDspAux *owner;
int fh_audio_dsp_aux_install(FhAudioDspAux *p) {
    if(!p || owner || (!p->process[0] && !p->process[1] &&
        !p->tagged_process[0] && !p->tagged_process[1] && !p->mapped_process[0] && !p->mapped_process[1]))return 0;
    p->next_token=p->last_frame=0;
    p->bank_token[0]=p->bank_token[1]=0;
    p->bank_frame[0]=p->bank_frame[1]=0;
    p->bank_enabled[0]=p->bank_enabled[1]=0;
    memset(&p->next_window,0,sizeof(p->next_window));memset(p->bank_window,0,sizeof(p->bank_window));
    owner=p;return 1;
}
void fh_audio_dsp_aux_uninstall(void) { owner=NULL; }
void fh_audio_dsp_aux_tag(FhAudioDspAux *p,uint32_t token) {
    if(p && p==owner){p->next_token=token;memset(&p->next_window,0,sizeof(p->next_window));}
}
void fh_audio_dsp_aux_tag_mapped(FhAudioDspAux *p,uint32_t token,const FhAudioAuxWindow *window) {
    if(p && p==owner) {
        p->next_token=token;
        if(window && window->source.valid)p->next_window=*window;
        else memset(&p->next_window,0,sizeof(p->next_window));
    }
}
static unsigned following(unsigned frame) { return frame==65535?2:frame+1; }
void fh_audio_dsp_aux_service(void) {
    FhAudioDspAux *p=owner;
    if(p) {
        unsigned read=ndspBufferId,write=ndspBufferCurId,frame=ndspFrameId;
        uint32_t token=0;unsigned returned_enabled=0,published_enabled=0;
        int tagged=p->tagged_process[0] || p->tagged_process[1] || p->mapped_process[0] || p->mapped_process[1];
        const FhAudioAuxWindow *returned_window=NULL;
        if(tagged) {
            if(frame<2 || (p->last_frame && frame!=following(p->last_frame))) {
                p->bank_token[0]=p->bank_token[1]=0;
                memset(p->bank_window,0,sizeof(p->bank_window));
                p->bank_frame[0]=p->bank_frame[1]=0;
            }
            /* Both fields must agree: bank parity alone can reuse stale state
             * across sleep/reset, repeated service or a skipped SDK reply. */
            if(frame>=2 && read<2 && p->bank_frame[read]>=2 &&
                following(following(p->bank_frame[read]))==frame) {
                token=p->bank_token[read];
                if(p->bank_window[read].source.valid)returned_window=&p->bank_window[read];
                returned_enabled=p->bank_enabled[read];
            }
        }
        if(read>1 || write>1 || !ndspVars[7][read] || !ndspVars[7][write])p->faults++;
        else for(unsigned bus=0;bus<2;bus++)if((p->process[bus] || p->tagged_process[bus] || p->mapped_process[bus]) && ndspAuxIsEnabled(bus)) {
            published_enabled|=1u<<bus;
            /* Both buses contain four planar little-endian PCM32 channels.
             * Snapshot before callback/publication, even when IDs coincide. */
            const unsigned char *src=ndspVars[7][read];
            unsigned char *dst=ndspVars[7][write];
            memcpy(p->work,src+bus*sizeof(p->work),sizeof(p->work));
            if(p->mapped_process[bus])p->mapped_process[bus](p->context[bus],p->work,
                returned_enabled&(1u<<bus)?token:0,returned_enabled&(1u<<bus)?returned_window:NULL);
            else if(p->tagged_process[bus])p->tagged_process[bus](p->context[bus],p->work,
                returned_enabled&(1u<<bus)?token:0);
            else p->process[bus](p->context[bus],p->work);
            memcpy(dst+bus*sizeof(p->work),p->work,sizeof(p->work));
            p->calls[bus]++;
        }
        /* Read both buses before replacing the bank's publication identity.
         * A missing tag call never repeats the previous submission's token. */
        if(tagged) {
            if(frame>=2 && read<2 && write<2 && ndspVars[7][read] && ndspVars[7][write]) {
                p->bank_window[write]=p->next_window;
                p->bank_token[write]=p->next_token;p->bank_frame[write]=frame;
                p->bank_enabled[write]=published_enabled;
            } else {
                p->bank_token[0]=p->bank_token[1]=0;
                memset(p->bank_window,0,sizeof(p->bank_window));
                p->bank_frame[0]=p->bank_frame[1]=0;
            }
            p->last_frame=frame;
        }
        p->next_token=0;memset(&p->next_window,0,sizeof(p->next_window));
        if(p->after_publish)p->after_publish(p->after_publish_context);
    }
}
uint32_t fh_audio_dsp_aux_effect_floor(const FhAudioDspAux *p) {
    if(!p || p!=owner || p->faults || p->next_token || p->next_window.source.valid)return 0;
    uint32_t floor=UINT32_MAX;unsigned found=0;
    for(unsigned bank=0;bank<2;bank++) {
        const FhAudioAuxWindow *w=&p->bank_window[bank];
        unsigned enabled=p->bank_enabled[bank];
        if(p->bank_frame[bank]<2 || !enabled)continue;
        for(unsigned bus=0;bus<2;bus++)if((enabled&(1u<<bus)) && !p->mapped_process[bus])return 0;
        if(!w->source.valid)continue; /* No mapped callback may select an effect here. */
        if(!w->source.count || w->source.count>FH_AUDIO_MAP_SPANS || w->has_lookahead>1)return 0;
        for(unsigned i=0;i<w->source.count+w->has_lookahead;i++) {
            const FhAudioSpan *s=i<w->source.count?&w->source.spans[i]:&w->lookahead;
            if(s->effect_token && !s->generation)return 0;
            if(s->effect_token) {
                if(s->effect_token<floor)floor=s->effect_token;
                found=1;
            }
        }
    }
    return found?floor:0;
}
