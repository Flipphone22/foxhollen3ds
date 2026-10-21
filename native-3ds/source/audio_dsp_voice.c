#include "fh_audio_game_dsp.h"
#include "fh_audio_dsp_voice.h"
#include <math.h>
#include <string.h>

enum { START=1,CONTROL=2,STOP=3,CURVE=4 };
_Static_assert(sizeof(FhAudioDspCurve)<=sizeof(FhAudioDspControls),"curve fits command payload");
static int valid_curve(const FhAudioDspCurve *c) {
    return c && isfinite(c->rate) && c->rate>0 && c->rate<=192000 &&
        c->interpolation<=NDSP_INTERP_NONE && c->paused<=1 && c->frames &&
        c->cycle_frames==160 && c->offset+c->frames<=160 && c->routing<=FH_AUDIO_DSP_ROUTE_DPL2_STEREO;
}
static int constant_curve(const FhAudioDspCurve *c) {
    if(c->env_start!=c->env_end)return 0;
    for(unsigned i=0;i<9;i++)if(c->volume_start[i]!=c->volume_end[i])return 0;
    return 1;
}
static int same_constant_curve(const FhAudioDspCurve *a,const FhAudioDspCurve *b) {
    if(a->rate!=b->rate || a->interpolation!=b->interpolation ||
       a->paused!=b->paused || a->routing!=b->routing ||
       a->env_start!=b->env_start || a->env_end!=b->env_end ||
       b->env_start!=b->env_end)return 0;
    /* All four endpoints must agree. Read each 16-bit lane once instead of
     * three overlapping byte comparisons (volume_end is not word-aligned).
     * Keep exact unsigned bit equality, including signed volume encodings. */
    for(unsigned i=0;i<9;i++) {
        unsigned value=b->volume_start[i];
        if(value!=b->volume_end[i] || value!=a->volume_start[i] || value!=a->volume_end[i])return 0;
    }
    return 1;
}
static int curve_gains(const FhAudioDspCurve *c,unsigned sample,int32_t *gain,unsigned count) {
    if(!gain || !valid_curve(c) || sample>c->frames)return 0;
    int32_t env=c->env_start+((int32_t)c->env_end-c->env_start)*(int32_t)sample/c->frames;
    for(unsigned ch=0;ch<count;ch++) {
        int32_t a=(int16_t)c->volume_start[ch],b=(int16_t)c->volume_end[ch];
        if(!a && !b){gain[ch]=0;continue;}
        int32_t vol=a+(b-a)*(int32_t)(c->offset+sample)/c->cycle_frames;
        gain[ch]=(vol*env)>>15;
    }
    return 1;
}
int fh_audio_dsp_curve_gain(const FhAudioDspCurve *c,unsigned sample,int32_t gain[2]) {
    return curve_gains(c,sample,gain,2);
}
int fh_audio_dsp_curve_gains(const FhAudioDspCurve *c,unsigned sample,int32_t gain[9]) {
    return curve_gains(c,sample,gain,9);
}
int fh_audio_dsp_curve_mix(const FhAudioDspCurve *c,unsigned sample,float mix[12]) {
    int32_t gains[9];
    if(!mix || !fh_audio_dsp_curve_gains(c,sample,gains))return 0;
    memset(mix,0,12*sizeof(float));
    mix[0]=gains[0]*(1.0f/32768.0f);mix[1]=gains[1]*(1.0f/32768.0f);
    mix[2]=mix[3]=gains[2]*(23170.0f/(32768.0f*32768.0f));
    for(unsigned bus=0;bus<2;bus++)for(unsigned ch=0;ch<3;ch++)
        mix[4+bus*4+ch]=gains[3+bus*3+ch]*(1.0f/32768.0f);
    if(c->routing==FH_AUDIO_DSP_ROUTE_DPL2_STEREO) {
        /* Original two-channel DPL2: main S is not mixed; AUX B L/R are
         * direct stereo, not an effects bus. Preserve their signed sums. */
        mix[0]+=mix[8];mix[1]+=mix[9];mix[2]=mix[3]=0;
        mix[8]=mix[9]=mix[10]=mix[11]=0;
    }
    return 1;
}
static int overlaps(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y || (x<y+nb && y<x+na);
}
static int valid_controls(const FhAudioDspControls *c) {
    /* Explicit candidate admission bounds, not claims about DSP limits. No
     * silent clamping: unsupported source controls must retain software audio. */
    if(!c || !isfinite(c->rate) || c->rate<=0 || c->rate>192000 ||
       c->interpolation>NDSP_INTERP_NONE || c->paused>1)return 0;
    for(unsigned i=0;i<12;i++)if(!isfinite(c->mix[i]) || c->mix[i]<-1 || c->mix[i]>1)return 0;
    return 1;
}
static FhAudioDspVoice *voice_at(FhAudioDspVoices *r,unsigned channel) {
    return r && r->initialized && channel>=FH_AUDIO_DSP_FIRST_CHANNEL && channel<24?
        &r->voices[channel-FH_AUDIO_DSP_FIRST_CHANNEL]:NULL;
}
static int space(FhAudioDspVoice *v) {
    return (unsigned)(v->head-__atomic_load_n(&v->tail,__ATOMIC_ACQUIRE))<FH_AUDIO_DSP_COMMANDS;
}
static void push(FhAudioDspVoice *v,unsigned kind,FhAudioPoolToken generation,const FhAudioDspControls *c,uint64_t due) {
    FhAudioDspCommand *cmd=&v->commands[v->head%FH_AUDIO_DSP_COMMANDS];
    cmd->kind=kind;cmd->generation=generation;cmd->due=due;
    if(c)cmd->controls=*c;
    __atomic_store_n(&v->head,v->head+1,__ATOMIC_RELEASE);
}
int fh_audio_dsp_voices_init(FhAudioDspVoices *r,FhAudioSamplePool *pool,int16_t *silence,
                            size_t bytes,FhAudioPoolFlush flush,void *context) {
    unsigned prefix_bytes=bytes>=2048?2048:FH_AUDIO_DSP_SILENCE_SAMPLES*2;
    if(!r || !pool || !pool->arena || !silence || !flush || ((uintptr_t)silence&31) || bytes<prefix_bytes ||
       overlaps(r,sizeof(*r),pool,sizeof(*pool)) ||
       overlaps(r,sizeof(*r),pool->arena,pool->capacity) ||
       overlaps(r,sizeof(*r),silence,prefix_bytes) ||
       overlaps(silence,prefix_bytes,pool,sizeof(*pool)) ||
       overlaps(silence,prefix_bytes,pool->arena,pool->capacity))return FH_AUDIO_POOL_INVALID;
    if(r->initialized)return FH_AUDIO_POOL_BUSY;
    if(prefix_bytes>FH_AUDIO_DSP_SILENCE_SAMPLES*2) {
        memset(silence,0,prefix_bytes);
        if(!flush(context,silence,prefix_bytes))return FH_AUDIO_POOL_FLUSH_FAILED;
    }
    int status=fh_audio_dsp_retire_init(&r->retirement,pool,silence,bytes,flush,context);
    if(status)return status;
    memset(r->voices,0,sizeof(r->voices));r->prefix_bytes=prefix_bytes;r->initialized=1;
    return FH_AUDIO_POOL_OK;
}
static int voice_start(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken bank,
                            uint32_t sample,const FhAudioDspControls *c,uint64_t epoch,uint64_t due,unsigned ramped,FhAudioPoolToken *id) {
    FhAudioDspVoice *v=voice_at(r,channel);
    if(!v || !id || !valid_controls(c))return FH_AUDIO_POOL_INVALID;
    if(v->lease || !space(v))return FH_AUDIO_POOL_BUSY;
    FhAudioPoolToken lease;
    int status=fh_audio_pool_acquire(r->retirement.pool,bank,sample,&lease);
    if(status)return status;
    const void *data;const FhAudioDspSamplePlan *plan;
    status=fh_audio_pool_view(r->retirement.pool,lease,&data,&plan);
    if(!status)status=fh_audio_dsp_retire_arm(&r->retirement,channel,lease);
    if(status){fh_audio_pool_release(r->retirement.pool,lease);return status;}
    v->lease=lease;v->samples=data;v->plan=*plan;v->stopping=0;
    v->epoch=epoch;v->timed=epoch!=0;v->last_due=due;v->curve_end=0;
    v->ramped=ramped;v->curve_valid=0;
    push(v,START,lease,c,due);*id=lease;return FH_AUDIO_POOL_OK;
}
int fh_audio_dsp_voice_start(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken bank,
                            uint32_t sample,const FhAudioDspControls *c,FhAudioPoolToken *id) {
    return voice_start(r,channel,bank,sample,c,0,0,0,id);
}
int fh_audio_dsp_voice_start_at(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken bank,
                            uint32_t sample,const FhAudioDspControls *c,uint64_t epoch,uint64_t due,FhAudioPoolToken *id) {
    if(!epoch)return FH_AUDIO_POOL_INVALID;
    return voice_start(r,channel,bank,sample,c,epoch,due,0,id);
}
int fh_audio_dsp_voice_start_ramped_at(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken bank,
    uint32_t sample,const FhAudioDspControls *c,uint64_t epoch,uint64_t due,FhAudioPoolToken *id) {
    if(!epoch)return FH_AUDIO_POOL_INVALID;
    return voice_start(r,channel,bank,sample,c,epoch,due,1,id);
}
int fh_audio_dsp_voice_curve_at(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken id,
    const FhAudioDspCurve *c,uint64_t due) {
    FhAudioDspVoice *v=voice_at(r,channel);
    if(!v || !id || v->lease!=id || !v->ramped || !v->timed ||
       !valid_curve(c) || due<v->last_due || due>UINT64_MAX-c->frames)return FH_AUDIO_POOL_INVALID;
    if(v->stopping)return FH_AUDIO_POOL_BUSY;
    /* Only this producer writes command slots. The last published slot stays
     * immutable until this producer publishes another command, even after the
     * callback consumes it. Retain contiguous identical constant controls;
     * ramps, pitch/pause/route changes, gaps and generation changes still queue.
     * Advance the producer deadline too, so a later STOP cannot move backwards.
     * No consumer-visible command is edited or extended in place. */
    if(v->head && due==v->curve_end) {
        const FhAudioDspCommand *last=&v->commands[(v->head-1)%FH_AUDIO_DSP_COMMANDS];
        if(last->kind==CURVE && last->generation==id && same_constant_curve(&last->curve,c)) {
            v->last_due=due;v->curve_end=due+c->frames;r->retained_controls++;
            return FH_AUDIO_POOL_OK;
        }
    }
    if(!space(v))return FH_AUDIO_POOL_BUSY;
    FhAudioDspCommand *cmd=&v->commands[v->head%FH_AUDIO_DSP_COMMANDS];
    cmd->kind=CURVE;cmd->generation=id;cmd->due=due;cmd->curve=*c;
    __atomic_store_n(&v->head,v->head+1,__ATOMIC_RELEASE);v->last_due=due;v->curve_end=due+c->frames;
    return FH_AUDIO_POOL_OK;
}
static int voice_control(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken id,const FhAudioDspControls *c,unsigned timed,uint64_t due) {
    FhAudioDspVoice *v=voice_at(r,channel);
    if(!v || !id || v->lease!=id || !valid_controls(c) || v->timed!=timed || due<v->last_due)return FH_AUDIO_POOL_INVALID;
    if(v->stopping || !space(v))return FH_AUDIO_POOL_BUSY;
    push(v,CONTROL,id,c,due);v->last_due=due;return FH_AUDIO_POOL_OK;
}
int fh_audio_dsp_voice_control(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken id,const FhAudioDspControls *c) {
    return voice_control(r,channel,id,c,0,0);
}
int fh_audio_dsp_voice_control_at(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken id,const FhAudioDspControls *c,uint64_t due) {
    return voice_control(r,channel,id,c,1,due);
}
static int voice_stop(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken id,unsigned timed,uint64_t due) {
    FhAudioDspVoice *v=voice_at(r,channel);
    if(!v || !id || v->lease!=id || v->timed!=timed || due<v->last_due)return FH_AUDIO_POOL_INVALID;
    if(v->stopping || !space(v))return FH_AUDIO_POOL_BUSY;
    push(v,STOP,id,NULL,due);v->last_due=due;v->stopping=1;return FH_AUDIO_POOL_OK;
}
int fh_audio_dsp_voice_stop(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken id) {
    return voice_stop(r,channel,id,0,0);
}
int fh_audio_dsp_voice_stop_at(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken id,uint64_t due) {
    return voice_stop(r,channel,id,1,due);
}
int fh_audio_dsp_voice_abort(FhAudioDspVoices *r,unsigned channel,FhAudioPoolToken id) {
    FhAudioDspVoice *v=voice_at(r,channel);
    if(!v || !id || v->lease!=id)return FH_AUDIO_POOL_INVALID;
    v->stopping=1;__atomic_store_n(&v->cancel,1,__ATOMIC_RELEASE);return FH_AUDIO_POOL_OK;
}
static void apply(unsigned channel,FhAudioDspControls *c) {
    ndspChnSetRate(channel,c->rate);ndspChnSetInterp(channel,(ndspInterpType)c->interpolation);
    fh_audio_game_output_mix(channel,c->mix);ndspChnSetPaused(channel,c->paused!=0);
}
static void start(FhAudioDspVoices *r,unsigned channel,FhAudioDspVoice *v,FhAudioDspCommand *cmd,unsigned delay) {
    /* Reuse only after the preceding generation's sentinel acknowledgement.
     * Reset prevents an inherited filter from affecting a new admitted voice. */
    ndspChnReset(channel);
    memset(&v->first,0,sizeof(v->first));memset(&v->loop,0,sizeof(v->loop));
    v->first.data_vaddr=v->samples;v->first.nsamples=v->plan.first_samples;
    v->loop.data_vaddr=(const unsigned char *)v->samples+v->plan.loop_offset_bytes;
    v->loop.nsamples=v->plan.loop_samples;v->loop.looping=true;
    if(v->plan.format==FH_AUDIO_DSP_ADPCM) {
        ndspChnSetFormat(channel,NDSP_FORMAT_MONO_ADPCM | (v->ramped?BIT(5):0));
        u16 coefficients[16];memcpy(coefficients,v->plan.coefficients,sizeof(coefficients));
        ndspChnSetAdpcmCoefs(channel,coefficients);
        v->first_history=(ndspAdpcmData){*(const uint8_t *)v->samples,0,0};
        v->first.adpcm_data=&v->first_history;
        if(v->plan.loop_samples) {
            v->loop_history=(ndspAdpcmData){*(const uint8_t *)v->loop.data_vaddr,v->plan.loop_y1,v->plan.loop_y2};
            v->loop.adpcm_data=&v->loop_history;
        }
    } else ndspChnSetFormat(channel,NDSP_FORMAT_MONO_PCM16 | (v->ramped?BIT(5):0));
    apply(channel,&cmd->controls);
    v->applied=cmd->controls;v->clock_held=0;
    if(delay) {
        memset(&v->delay,0,sizeof(v->delay));memset(&v->delay_history,0,sizeof(v->delay_history));
        v->delay.data_vaddr=r->retirement.silence;v->delay.nsamples=delay;
        if(v->plan.format==FH_AUDIO_DSP_ADPCM)v->delay.adpcm_data=&v->delay_history;
        ndspChnWaveBufAdd(channel,&v->delay);r->prefix_starts++;r->prefix_samples+=delay;
    }
    ndspChnWaveBufAdd(channel,&v->first);
    if(v->plan.loop_samples)ndspChnWaveBufAdd(channel,&v->loop);
    v->current=cmd->generation;v->live=1;
}
static void stop(FhAudioDspVoices *r,unsigned channel,FhAudioDspVoice *v) {
    if(fh_audio_dsp_retire_begin(&r->retirement,channel))++r->faults;
    v->live=0;
}
static void service_region(FhAudioDspVoices *r,uint64_t epoch,uint64_t cursor,uint64_t window_end,unsigned leading,int subblock,int available) {
    if(!r || !r->initialized)return;
    if(window_end<cursor)available=0;
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++) {
        FhAudioDspVoice *v=&r->voices[i];unsigned channel=i+FH_AUDIO_DSP_FIRST_CHANNEL;
        uint64_t control_cursor=cursor;
        int voice_available=available;
        if(leading && v->live && (!v->timed || v->epoch==epoch)){voice_available=0;r->prefix_faults++;}
        unsigned end=__atomic_load_n(&v->head,__ATOMIC_ACQUIRE);
        if(__atomic_load_n(&v->cancel,__ATOMIC_ACQUIRE)) {
            /* Abort's release follows every earlier queue publication. Reload
             * head AFTER acquiring it, including a START posted since the first
             * snapshot; otherwise that stale START could revive the cancelled voice. */
            end=__atomic_load_n(&v->head,__ATOMIC_ACQUIRE);
            /* Already retiring is harmless. No producer descriptor is touched
             * after the acknowledgement at the end of this callback. */
            (void)fh_audio_dsp_retire_begin(&r->retirement,channel);
            v->live=0;r->cancelled++;
            __atomic_store_n(&v->tail,end,__ATOMIC_RELEASE);
            __atomic_store_n(&v->cancel,0,__ATOMIC_RELEASE);
            continue;
        }
        /* Source generations increase, but preparation can lead playback by
         * several PCM packets. A future START is not stale: retain the entire
         * snapshot until that epoch reaches the output. Epoch zero denotes an
         * unknown/gap cursor and holds timed voices as well. Read immutable
         * descriptors only after acquiring a pending START, or while live. */
        if((v->live || v->tail!=end) && v->timed && v->epoch>epoch) {
            if(v->live && !v->clock_held) {
                ndspChnSetPaused(channel,true);v->clock_held=1;r->held++;
            }
            continue;
        }
        if(v->live && v->timed) {
            if(v->epoch!=epoch){stop(r,channel,v);r->epoch_rejected++;}
            else if(!voice_available) {
                if(!v->clock_held){ndspChnSetPaused(channel,true);v->clock_held=1;r->held++;}
            } else if(v->clock_held) {
                ndspChnSetPaused(channel,v->applied.paused!=0);v->clock_held=0;
            }
        }
        /* Consume at most one bounded queue snapshot; no unbounded producer chase. */
        while(v->tail!=end) {
            FhAudioDspCommand *cmd=&v->commands[v->tail%FH_AUDIO_DSP_COMMANDS];
            if(cmd->kind!=START && (!v->live || cmd->generation!=v->current))++r->discarded;
            else if(v->timed && v->epoch!=epoch) {
                stop(r,channel,v);r->epoch_rejected++;
            } else {
                int early_start=subblock && v->ramped && cmd->kind==START && cmd->due<window_end;
                if(v->timed && (!voice_available || (cmd->due>control_cursor && !early_start)))break;
                if(v->timed && cmd->due<cursor)r->late_samples+=cursor-cmd->due;
                if(cmd->kind==START) {
                    FhAudioDspCommand initial=*cmd;unsigned delay=0;
                    if(early_start && (leading || cmd->due>cursor)) {
                        /* Initial curve supplies the actual pitch; following
                         * subframe pitch changes stay queued. Snapshot is bounded. */
                        if(v->tail+1==end){r->curve_shortages++;break;}
                        const FhAudioDspCommand *next=&v->commands[(v->tail+1)%FH_AUDIO_DSP_COMMANDS];
                        if(next->kind!=CURVE || next->generation!=cmd->generation || next->due!=cmd->due) {
                            r->curve_shortages++;break;
                        }
                        initial.controls.rate=next->curve.rate;
                        initial.controls.interpolation=next->curve.interpolation;
                        initial.controls.paused=next->curve.paused;
                        uint64_t offset=leading+(cmd->due>cursor?cmd->due-cursor:0);
                        if(offset>160){r->prefix_faults++;break;}
                        float exact_delay=(float)offset*initial.controls.rate/32000.0f;
                        delay=(unsigned)exact_delay;
                        if((float)delay<exact_delay)delay++;
                        unsigned needed=v->plan.format==FH_AUDIO_DSP_ADPCM?((delay+13)/14)*8:delay*2;
                        if(needed>r->prefix_bytes){r->prefix_faults++;break;}
                    }
                    start(r,channel,v,&initial,delay);++r->starts;
                    if(cmd->due>control_cursor)control_cursor=cmd->due;
                }
                else if(cmd->kind==CONTROL){apply(channel,&cmd->controls);v->applied=cmd->controls;++r->controls;}
                else if(cmd->kind==CURVE) {
                    v->curve=cmd->curve;v->curve_due=cmd->due;v->curve_valid=1;
                    ndspChnSetRate(channel,v->curve.rate);
                    ndspChnSetInterp(channel,(ndspInterpType)v->curve.interpolation);
                    ndspChnSetPaused(channel,v->curve.paused!=0);
                    v->applied.rate=v->curve.rate;v->applied.interpolation=v->curve.interpolation;
                    v->applied.paused=v->curve.paused;++r->controls;
                }
                else if(cmd->kind==STOP){stop(r,channel,v);++r->stops;}
            }
            __atomic_store_n(&v->tail,v->tail+1,__ATOMIC_RELEASE);
        }
        if(v->live && v->ramped && voice_available && v->curve_valid) {
            FhAudioDspCurve curve=v->curve;uint64_t due=v->curve_due;
            int stopped=0;
            /* A single acquired snapshot bounds work. Do not release future
             * rate/pause events when looking ahead at their gain curve. */
            for(unsigned at=v->tail;at!=end;at++) {
                const FhAudioDspCommand *cmd=&v->commands[at%FH_AUDIO_DSP_COMMANDS];
                if(cmd->due>window_end)break;
                if(cmd->generation!=v->current)break;
                if(cmd->kind==STOP){stopped=1;break;}
                if(cmd->kind==CURVE){curve=cmd->curve;due=cmd->due;}
            }
            float mix[12]={0};
            if(!stopped) {
                uint64_t sample=window_end-due;
                if(sample>curve.frames){sample=curve.frames;if(!constant_curve(&curve))++r->curve_shortages;}
                if(!fh_audio_dsp_curve_mix(&curve,(unsigned)sample,mix))++r->faults;
            }
            fh_audio_game_output_mix(channel,mix);memcpy(v->applied.mix,mix,sizeof(mix));
        }
        /* A timed voice belongs to the source synth until its STOP arrives.
         * DSP EOF may precede source deactivation: retiring here lets polling
         * recycle the generation while the source bridge still owns its token.
         * The finished wave emits no audio; retain only ownership, not playback.
         * Untimed standalone voices retain automatic EOF retirement. */
        if(v->live && !v->timed && !v->plan.loop_samples && v->first.status==NDSP_WBUF_DONE) {
            stop(r,channel,v);++r->natural_ends;
        }
    }
    /* Last operation: producer may reuse a completed channel after this publishes
     * ACK. Do not access its voice descriptors/immutable plan afterwards. */
    fh_audio_dsp_retire_service(&r->retirement);
}
void fh_audio_dsp_voices_service_window(FhAudioDspVoices *r,uint64_t epoch,uint64_t cursor,uint64_t end,int available) {
    service_region(r,epoch,cursor,end,0,0,available);
}
void fh_audio_dsp_voices_service_region(FhAudioDspVoices *r,uint64_t epoch,uint64_t cursor,uint64_t end,unsigned leading,int available) {
    if(end<cursor || end-cursor>160 || leading>160 || end-cursor+leading>160)available=0;
    service_region(r,epoch,cursor,end,leading,1,available);
}
void fh_audio_dsp_voices_service_at(FhAudioDspVoices *r,uint64_t epoch,uint64_t cursor,int available) {
    fh_audio_dsp_voices_service_window(r,epoch,cursor,cursor,available);
}
void fh_audio_dsp_voices_service(FhAudioDspVoices *r) {
    fh_audio_dsp_voices_service_at(r,0,0,0);
}
int fh_audio_dsp_voices_poll(FhAudioDspVoices *r,uint32_t *completed) {
    if(!r || !r->initialized || !completed)return FH_AUDIO_POOL_INVALID;
    /* An abort can arrive after a natural-end acknowledgement. Consume that
     * cancellation before allowing channel reuse, or it could kill a new voice. */
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(__atomic_load_n(&r->voices[i].cancel,__ATOMIC_ACQUIRE)) {
        *completed=0;return FH_AUDIO_POOL_OK;
    }
    int status=fh_audio_dsp_retire_drain(&r->retirement,completed);
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(*completed&(1u<<(i+FH_AUDIO_DSP_FIRST_CHANNEL))) {
        r->voices[i].lease=0;r->voices[i].stopping=0;
    }
    return status;
}
