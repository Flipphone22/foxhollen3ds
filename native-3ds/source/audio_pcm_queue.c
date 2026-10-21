#include "fh_audio_game_dsp.h"
#include "fh_audio_pcm_queue.h"
#include <string.h>

enum { FREE=0,READY=1,ACTIVE=2 };
static int overlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y || (x<y+nb && y<x+na);
}
static FhAudioPcmChannel *channel(FhAudioPcmQueue *q,unsigned c) {
    return q && q->initialized && c<FH_PCM_CHANNELS?&q->channels[c]:NULL;
}
int fh_audio_pcm_init(FhAudioPcmQueue *q,int16_t *storage,size_t storage_bytes,
    int16_t *silence,size_t silence_bytes,FhAudioPoolFlush flush,void *context) {
    const size_t bytes=FH_PCM_CHANNELS*FH_PCM_SLOTS*FH_PCM_FRAMES*4;
    if(!q || !storage || !silence || ((uintptr_t)storage&31) || ((uintptr_t)silence&31) ||
       storage_bytes<bytes || silence_bytes<FH_PCM_SILENCE*4 || !flush ||
       overlap(storage,bytes,silence,FH_PCM_SILENCE*4) || overlap(q,sizeof(*q),storage,bytes) ||
       overlap(q,sizeof(*q),silence,FH_PCM_SILENCE*4))return FH_AUDIO_POOL_INVALID;
    if(q->initialized)return FH_AUDIO_POOL_BUSY;
    memset(silence,0,FH_PCM_SILENCE*4);
    if(!flush(context,silence,FH_PCM_SILENCE*4))return FH_AUDIO_POOL_FLUSH_FAILED;
    memset(q,0,sizeof(*q));q->storage=storage;q->silence=silence;q->flush=flush;q->flush_context=context;
    for(unsigned c=0;c<FH_PCM_CHANNELS;c++) {
        FhAudioPcmChannel *p=&q->channels[c];
        p->requested_epoch=p->acknowledged_epoch=p->active_epoch=1;p->prebuffer=p->start_pending=1;
        p->cursor.epoch=1;
        for(unsigned i=0;i<FH_PCM_SLOTS;i++)p->slots[i].wave.data_vaddr=storage+(c*FH_PCM_SLOTS+i)*FH_PCM_FRAMES*2;
    }
    q->initialized=1;return FH_AUDIO_POOL_OK;
}
static int settled(FhAudioPcmChannel *p) {
    return p->requested_epoch==__atomic_load_n(&p->acknowledged_epoch,__ATOMIC_ACQUIRE);
}
int fh_audio_pcm_space(FhAudioPcmQueue *q,unsigned c) {
    FhAudioPcmChannel *p=channel(q,c);
    return p && settled(p) && !p->paused &&
        __atomic_load_n(&p->slots[p->write_index%FH_PCM_SLOTS].state,__ATOMIC_ACQUIRE)==FREE;
}
unsigned fh_audio_pcm_buffered(FhAudioPcmQueue *q,unsigned c) {
    FhAudioPcmChannel *p=channel(q,c);if(!p || !settled(p))return 0;
    unsigned frames=0;
    for(unsigned i=0;i<FH_PCM_SLOTS;i++)if(__atomic_load_n(&p->slots[i].state,__ATOMIC_ACQUIRE)!=FREE)frames+=p->slots[i].frames;
    return frames;
}
FhAudioPcmStatus fh_audio_pcm_status(FhAudioPcmQueue *q,unsigned c) {
    FhAudioPcmStatus result={0};FhAudioPcmChannel *p=channel(q,c);if(!p)return result;
    result.submitted=__atomic_load_n(&p->submitted,__ATOMIC_RELAXED);
    result.underruns=__atomic_load_n(&p->underruns,__ATOMIC_RELAXED);
    result.faults=__atomic_load_n(&p->faults,__ATOMIC_RELAXED);
    result.paused=p->paused;
    if(!settled(p))return result;
    if(p->write_index)result.rate=p->slots[(p->write_index-1)%FH_PCM_SLOTS].rate;
    for(unsigned i=0;i<FH_PCM_SLOTS;i++) {
        FhAudioPcmSlot *s=&p->slots[i];
        if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)!=FREE){result.frames+=s->frames;result.buffers++;}
    }
    return result;
}
int fh_audio_pcm_queue(FhAudioPcmQueue *q,unsigned c,const int16_t *samples,unsigned frames,unsigned rate) {
    return fh_audio_pcm_queue_mapped(q,c,samples,frames,rate,NULL);
}
int fh_audio_pcm_queue_mapped(FhAudioPcmQueue *q,unsigned c,const int16_t *samples,unsigned frames,unsigned rate,const FhAudioMap *map) {
    FhAudioPcmChannel *p=channel(q,c);
    if(!fh_audio_map_valid(map,frames) || !p || !samples || !frames || frames>FH_PCM_FRAMES || !rate || rate>192000 ||
       frames>UINT64_MAX-p->next_frame)return FH_AUDIO_POOL_INVALID;
    unsigned next_effect=p->queued_effect_token;
    if(p->effect_order_guard && map)for(unsigned i=0;i<map->count;i++) {
        unsigned token=map->spans[i].effect_token;
        if(token && token<next_effect)return FH_AUDIO_POOL_INVALID;
        if(token)next_effect=token; /* Unknown gaps do not authorize going back. */
    }
    if(!fh_audio_pcm_space(q,c))return FH_AUDIO_POOL_BUSY;
    /* The prior queue API permits rate changes only with no retained buffers. */
    if(p->write_index && rate!=p->slots[(p->write_index-1)%FH_PCM_SLOTS].rate && fh_audio_pcm_buffered(q,c))return FH_AUDIO_POOL_INVALID;
    FhAudioPcmSlot *s=&p->slots[p->write_index%FH_PCM_SLOTS];
    memcpy((void *)s->wave.data_vaddr,samples,frames*4);
    if(!q->flush(q->flush_context,s->wave.data_vaddr,frames*4))return FH_AUDIO_POOL_FLUSH_FAILED;
    if(map)s->map=*map;else s->map.count=0;
    s->frames=frames;s->rate=rate;s->first_frame=p->next_frame;p->next_frame+=frames;
    p->queued_effect_token=next_effect;
    __atomic_store_n(&s->state,READY,__ATOMIC_RELEASE);p->write_index++;
    return FH_AUDIO_POOL_OK;
}
int fh_audio_pcm_reset(FhAudioPcmQueue *q,unsigned c) {
    FhAudioPcmChannel *p=channel(q,c);if(!p || p->requested_epoch==UINT32_MAX)return FH_AUDIO_POOL_INVALID;
    p->write_index=0;p->next_frame=0;
    __atomic_store_n(&p->finish,0,__ATOMIC_RELEASE);
    __atomic_store_n(&p->requested_epoch,p->requested_epoch+1,__ATOMIC_RELEASE);
    return FH_AUDIO_POOL_OK;
}
int fh_audio_pcm_pause(FhAudioPcmQueue *q,unsigned c,int paused) {
    FhAudioPcmChannel *p=channel(q,c);if(!p)return FH_AUDIO_POOL_INVALID;
    __atomic_store_n(&p->paused,paused!=0,__ATOMIC_RELEASE);return FH_AUDIO_POOL_OK;
}
int fh_audio_pcm_finish(FhAudioPcmQueue *q,unsigned c) {
    FhAudioPcmChannel *p=channel(q,c);if(!p)return FH_AUDIO_POOL_INVALID;
    if(fh_audio_pcm_buffered(q,c))__atomic_store_n(&p->finish,1,__ATOMIC_RELEASE);
    return FH_AUDIO_POOL_OK;
}
static void pause_channel(unsigned c,FhAudioPcmChannel *p,unsigned pause) {
    if(pause!=p->hardware_paused){ndspChnSetPaused(c,pause!=0);p->hardware_paused=pause;}
}
static void defaults(unsigned c,FhAudioPcmChannel *p) {
    ndspChnReset(c);ndspChnSetFormat(c,NDSP_FORMAT_STEREO_PCM16);ndspChnSetInterp(c,NDSP_INTERP_LINEAR);
    const float mix[12]={1,1};fh_audio_game_output_mix(c,mix);
    ndspChnSetPaused(c,true);p->hardware_paused=1;p->rate=0;p->configured=1;
}
static int reset_step(FhAudioPcmQueue *q,unsigned c,FhAudioPcmChannel *p,unsigned epoch) {
    if(epoch==p->active_epoch && !p->resetting)return 0;
    p->source_window=(FhAudioSourceWindow){0};p->output_clock.valid=0;p->output_draining=0;
    p->active_epoch=epoch;p->cursor=(FhAudioPcmCursor){.epoch=epoch};p->source_cursor=(FhAudioSourceCursor){0};
    if(!p->resetting) {
        ndspChnWaveBufClear(c);ndspChnSetFormat(c,NDSP_FORMAT_STEREO_PCM16);
        ndspChnSetInterp(c,NDSP_INTERP_NONE);ndspChnSetRate(c,32000);
        float mute[12]={0};ndspChnSetMix(c,mute);
        memset(&p->sentinel,0,sizeof(p->sentinel));p->sentinel.data_pcm16=q->silence;
        p->sentinel.nsamples=FH_PCM_SILENCE;p->sentinel.looping=true;
        ndspChnWaveBufAdd(c,&p->sentinel);ndspChnSetPaused(c,false);
        p->hardware_paused=0;p->resetting=1;return 1;
    }
    if(p->sentinel.status!=NDSP_WBUF_PLAYING || !p->sentinel.sequence_id ||
       ndspChnGetWaveBufSeq(c)!=p->sentinel.sequence_id)return 1;
    /* Only now may the producer overwrite old PCM. Silence itself is permanent
     * through ndspExit; clearing it is not a fence for the silence allocation. */
    defaults(c,p);p->read_index=0;p->prebuffer=p->start_pending=1;p->resetting=0;
    for(unsigned i=0;i<FH_PCM_SLOTS;i++) {
        ndspWaveBuf *w=&p->slots[i].wave;w->status=NDSP_WBUF_FREE;w->next=NULL;
        __atomic_store_n(&p->slots[i].state,FREE,__ATOMIC_RELEASE);
    }
    /* A newer request may have arrived while resetting; it will be handled on
     * the next callback. Don't acknowledge an epoch not acquired above. */
    __atomic_store_n(&p->acknowledged_epoch,epoch,__ATOMIC_RELEASE);
    return 1;
}
void fh_audio_pcm_service(FhAudioPcmQueue *q) {
    if(!q || !q->initialized)return;
    for(unsigned c=0;c<FH_PCM_CHANNELS;c++) {
        FhAudioPcmChannel *p=&q->channels[c];
        unsigned epoch=__atomic_load_n(&p->requested_epoch,__ATOMIC_ACQUIRE);
        if(!p->configured)defaults(c,p);
        if(reset_step(q,c,p,epoch))continue;
        unsigned user_paused=__atomic_load_n(&p->paused,__ATOMIC_ACQUIRE);
        u16 sequence=ndspChnGetWaveBufSeq(c);unsigned position=ndspChnGetSamplePos(c);
        p->cursor.available=0;p->source_cursor=(FhAudioSourceCursor){0};
        /* Observe before releasing any DONE slots to the producer. */
        for(unsigned i=0;i<FH_PCM_SLOTS;i++) {
            FhAudioPcmSlot *s=&p->slots[i];
            if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)==ACTIVE && sequence && s->wave.sequence_id==sequence) {
                if(position>s->frames){__atomic_add_fetch(&p->faults,1,__ATOMIC_RELAXED);break;}
                p->cursor=(FhAudioPcmCursor){s->first_frame+position,epoch,s->rate,!user_paused && !p->prebuffer};
                p->source_cursor=fh_audio_map_cursor(&s->map,position,p->cursor.available);break;
            }
        }
        unsigned queued=0;uint64_t first_ready=UINT64_MAX;unsigned first_rate=0;FhAudioPcmSlot *first_slot=NULL;
        for(unsigned i=0;i<FH_PCM_SLOTS;i++) {
            FhAudioPcmSlot *s=&p->slots[i];unsigned state=__atomic_load_n(&s->state,__ATOMIC_ACQUIRE);
            if(state==ACTIVE && s->wave.status==NDSP_WBUF_DONE) {
                __atomic_store_n(&s->state,FREE,__ATOMIC_RELEASE);
            } else if(state==ACTIVE) {
                queued++;
                if(s->first_frame<first_ready){first_ready=s->first_frame;first_rate=s->rate;first_slot=s;}
            }
        }
        if(!queued && !p->prebuffer){p->prebuffer=p->start_pending=1;__atomic_add_fetch(&p->underruns,1,__ATOMIC_RELAXED);}
        unsigned submitted_before=p->submitted;
        for(unsigned count=0;count<FH_PCM_SLOTS;count++) {
            FhAudioPcmSlot *s=&p->slots[p->read_index%FH_PCM_SLOTS];
            if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)!=READY)break;
            if(s->first_frame<first_ready){first_ready=s->first_frame;first_rate=s->rate;first_slot=s;}
            if(s->rate!=p->rate){ndspChnSetRate(c,s->rate);p->rate=s->rate;}
            s->wave.nsamples=s->frames;s->wave.looping=false;s->wave.status=NDSP_WBUF_FREE;s->wave.next=NULL;
            ndspChnWaveBufAdd(c,&s->wave);
            __atomic_store_n(&s->state,ACTIVE,__ATOMIC_RELEASE);p->read_index++;__atomic_add_fetch(&p->submitted,1,__ATOMIC_RELAXED);queued++;
        }
        unsigned paired_start=0;
        unsigned finish=__atomic_exchange_n(&p->finish,0,__ATOMIC_ACQ_REL);
        if(p->prebuffer && (queued>=4 || (finish && queued)))p->prebuffer=0;
        if(!p->prebuffer && !user_paused && p->start_pending && first_rate) {
            /* Paired submission: native voices due at the first PCM frame can
             * join this same SDK update. Subsequent progress uses returned state. */
            p->cursor=(FhAudioPcmCursor){first_ready,epoch,first_rate,1};p->start_pending=0;paired_start=1;
            p->source_cursor=fh_audio_map_cursor(&first_slot->map,0,1);
        }
        if(p->prebuffer || user_paused){p->cursor.available=0;p->source_cursor.available=0;}
        if(p->submitted!=submitted_before)p->output_draining=0;
        if(finish)p->output_draining=1;
        uint64_t queued_end=0;unsigned refill_frames=0;
        for(unsigned i=0;i<FH_PCM_SLOTS;i++) {
            const FhAudioPcmSlot *slot=&p->slots[i];
            if(__atomic_load_n(&slot->state,__ATOMIC_ACQUIRE)==ACTIVE) {
                if(slot->first_frame+slot->frames>queued_end)queued_end=slot->first_frame+slot->frames;
                if(4*slot->frames>refill_frames)refill_frames=4*slot->frames;
            }
        }
        unsigned guarded_pause=0;
        if(p->output_guard && !paired_start && !user_paused && !p->prebuffer && !p->output_draining)
            guarded_pause=!fh_audio_output_gate(&p->output_clock,SYSCLOCK_SOC,queued_end,refill_frames);
        if(guarded_pause)p->cursor.available=0;
        pause_channel(c,p,user_paused || p->prebuffer || guarded_pause);
        FhAudioOutputWindow window=fh_audio_output_step(&p->output_clock,
            ndspGetFrameCount(),ndspGetDroppedFrames(),epoch,p->rate,SYSCLOCK_SOC,
            paired_start,first_ready,!p->hardware_paused,queued_end);
        p->source_window=(FhAudioSourceWindow){0};
        if(window.available) {
            /* ACTIVE metadata is immutable until this callback frees it. No
             * pointer into a producer-owned FREE/READY slot escapes. */
            for(unsigned i=0;i<FH_PCM_SLOTS;i++) {
                const FhAudioPcmSlot *slot=&p->slots[i];
                if(__atomic_load_n(&slot->state,__ATOMIC_ACQUIRE)==ACTIVE &&
                   !fh_audio_source_window_append(&p->source_window,&slot->map,slot->first_frame,
                       slot->frames,window.first,window.end)) {
                    p->source_window=(FhAudioSourceWindow){0};break;
                }
            }
            fh_audio_source_window_finish(&p->source_window,window.first,window.end);
        }
    }
}
FhAudioPcmCursor fh_audio_pcm_cursor(const FhAudioPcmQueue *q,unsigned c) {
    return q && q->initialized && c<FH_PCM_CHANNELS?q->channels[c].cursor:(FhAudioPcmCursor){0};
}

FhAudioSourceCursor fh_audio_pcm_source_cursor(const FhAudioPcmQueue *q,unsigned c) {
    return q && q->initialized && c<FH_PCM_CHANNELS?q->channels[c].source_cursor:(FhAudioSourceCursor){0};
}

FhAudioSourceWindow fh_audio_pcm_source_window(const FhAudioPcmQueue *q,unsigned c) {
    return q && q->initialized && c<FH_PCM_CHANNELS?q->channels[c].source_window:(FhAudioSourceWindow){0};
}

FhAudioAuxWindow fh_audio_pcm_aux_window(const FhAudioPcmQueue *q,unsigned c) {
    FhAudioAuxWindow out={0};
    if(!q || !q->initialized || c>=FH_PCM_CHANNELS)return out;
    const FhAudioPcmChannel *p=&q->channels[c];
    const FhAudioSourceWindow *w=&p->source_window;
    if(!w->valid || !w->count || w->count>FH_AUDIO_MAP_SPANS ||
       !p->output_clock.valid || p->rate!=32000 || !p->active_epoch ||
       w->input_first>UINT64_MAX-w->frames)return out;
    uint64_t end=w->input_first+w->frames;
    if(end!=p->output_clock.frame || end==UINT64_MAX)return out;
    uint64_t advance=UINT64_C(160)*p->rate*512;
    uint32_t phase=(p->output_clock.fraction+SYSCLOCK_SOC-advance%SYSCLOCK_SOC)%SYSCLOCK_SOC;
    if((phase+advance)/SYSCLOCK_SOC!=w->frames)return out; /* No truncated block. */
    out.source=*w;out.queue_epoch=p->active_epoch;out.phase=phase;out.rate=p->rate;
    for(unsigned i=0;i<FH_PCM_SLOTS;i++) {
        const FhAudioPcmSlot *slot=&p->slots[i];
        if(__atomic_load_n(&slot->state,__ATOMIC_ACQUIRE)!=ACTIVE ||
           end<slot->first_frame || end-slot->first_frame>=slot->frames)continue;
        if(out.has_lookahead)return (FhAudioAuxWindow){0}; /* Overlapping owners. */
        unsigned position=(unsigned)(end-slot->first_frame);
        FhAudioSpan next={.frames=1};unsigned found=0;
        if(slot->map.count>FH_AUDIO_MAP_SPANS)return (FhAudioAuxWindow){0};
        if(!slot->map.count)found=1; /* A valid untagged streaming source. */
        for(unsigned j=0;j<slot->map.count;j++) {
            const FhAudioSpan *s=&slot->map.spans[j];
            if(position<s->offset || position-s->offset>=s->frames)continue;
            if(found)return (FhAudioAuxWindow){0};
            found=1;next=*s;
            if(s->generation)next.source_frame+=position-s->offset;
            else next.source_frame=0;
        }
        if(!found)return (FhAudioAuxWindow){0};
        next.offset=(uint16_t)w->frames;next.frames=1;
        out.lookahead=next;out.has_lookahead=1;
    }
    return out;
}
