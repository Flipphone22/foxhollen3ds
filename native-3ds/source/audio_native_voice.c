#include "fh_audio_native_voice.h"
#include "musyx/hardware.h"
#include "musyx/sal.h"
#include <string.h>
/* MusyX PC backend ABI (declared in its private hw_pc_internal.h). */
extern u32 salPCChannels(void);

static int route(const FhAudioNativeVoices *n,const DSPvoice *v) {
    if(!v || v->studio>=8 || (v->flags&0x80000000u) ||
       v->volRear[0] || v->lastVolRear[0] || v->volRear[1] || v->lastVolRear[1])return 0;
    if(n->experimental_direct_stereo<3 &&
       (v->volS || v->lastVolS || v->volLa || v->lastVolLa || v->volRa || v->lastVolRa ||
        v->volSa || v->lastVolSa || v->volLb || v->lastVolLb || v->volRb || v->lastVolRb ||
        v->volSb || v->lastVolSb))return 0;
#if MUSY_VERSION >= MUSY_VERSION_CHECK(2,0,1)
    if(v->filter.on)return 0;
#endif
    const DSPstudioinfo *s=&dspStudio[v->studio];
    if(n->experimental_direct_stereo>=3 && (!n->aux_bound || n->aux_epoch!=n->epoch ||
       n->aux_studio!=v->studio || salPCChannels()!=2 ||
       n->aux_handlers[0]!=s->auxAHandler || n->aux_handlers[1]!=s->auxBHandler ||
       n->aux_users[0]!=s->auxAUser || n->aux_users[1]!=s->auxBUser))return 0;
    /* A studio's output may also feed another studio. Reject that graph until
     * native bus routing exists; isMaster alone does not prove direct output. */
    unsigned type=n->experimental_direct_stereo==4?SND_STUDIO_TYPE_DPL2:SND_STUDIO_TYPE_STD;
    if(s->state!=DSP_STUDIO_STATE_ACTIVE || !s->isMaster || s->type!=type || s->numInputs)return 0;
    for(unsigned i=0;i<8;i++)if(dspStudio[i].state==DSP_STUDIO_STATE_ACTIVE) {
        if(dspStudio[i].numInputs>7)return 0;
        for(unsigned j=0;j<dspStudio[i].numInputs;j++)if(dspStudio[i].in[j].studio==v->studio)return 0;
    }
    return 1;
}
static unsigned pending(const FhAudioDspVoice *v) {
    return v->head-__atomic_load_n(&v->tail,__ATOMIC_ACQUIRE);
}
static uint64_t begin(void *context,unsigned index,const DSPvoice *v,uint64_t frame) {
    FhAudioNativeVoices *n=context;
    if(!n->experimental_direct_stereo || !route(n,v)){n->declined++;return 0;}
    FhAudioPoolToken bank;
    if(fh_audio_sample_registry_find(n->registry,&v->smp_info,&bank)){n->declined++;return 0;}
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(!n->dsp->voices[i].lease) {
        FhAudioDspControls c={.rate=32000,.interpolation=NDSP_INTERP_LINEAR};
        /* Muted until the first source envelope/pitch segment is published. */
        FhAudioPoolToken token;
        int status=n->experimental_direct_stereo>=2?
            fh_audio_dsp_voice_start_ramped_at(n->dsp,i+FH_AUDIO_DSP_FIRST_CHANNEL,bank,0,&c,n->epoch,frame,&token):
            fh_audio_dsp_voice_start_at(n->dsp,i+FH_AUDIO_DSP_FIRST_CHANNEL,bank,0,&c,n->epoch,frame,&token);
        if(status){n->declined++;return 0;}
        n->voices[i]=(FhAudioNativeVoice){token,index,v->studio,c};n->admitted++;return token;
    }
    n->declined++;return 0;
}
static float gain(uint16_t old,uint16_t target,const FhAudioSourceSegment *s) {
    /* Preserve signed source volumes and division toward zero. The endpoint
     * itself is exact before conversion; NDSP interpolation between endpoints
     * is a separate, currently unaccepted approximation. */
    int32_t a=(int16_t)old,b=(int16_t)target;
    int32_t volume=a+(b-a)*(s->offset+s->frames)/(int32_t)s->cycle_frames;
    int32_t product=(volume*s->env_end)>>15;
    return (float)product*(1.0f/32768.0f);
}
static int segment(void *context,uint64_t token,const DSPvoice *v,const FhAudioSourceSegment *s) {
    FhAudioNativeVoices *n=context;
    if(!route(n,v) || !s->frames || s->cycle_frames!=160){n->faults++;return 0;}
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(n->voices[i].token==token) {
        FhAudioNativeVoice *a=&n->voices[i];
        if(v->studio!=a->studio){n->faults++;return 0;}
        FhAudioDspControls c=a->controls;
        c.paused=s->held;
        /* SRC_NONE disables the source voice's pitch conversion, not the final
         * 32kHz PCM -> NDSP output conversion. That final stage is linear in
         * platform_audio; nearest here audibly changes even a fixed-pitch tone. */
        if(v->srcTypeSelect==SAL_SRC_NONE){c.rate=32000;c.interpolation=NDSP_INTERP_LINEAR;}
        else {
            if(!s->held)c.rate=(float)s->pitch*(32000.0f/65536.0f);
            c.interpolation=v->srcTypeSelect==SAL_SRC_LINEAR?NDSP_INTERP_LINEAR:NDSP_INTERP_POLYPHASE;
        }
        /* Curve mode sends source coefficients; endpoint-only gains are never
         * consumed there. Keep that calculation in its diagnostic mode only. */
        if(n->experimental_direct_stereo<2) {
            c.mix[0]=gain(v->lastVolL,v->volL,s);c.mix[1]=gain(v->lastVolR,v->volR,s);
        }
        int status;
        if(n->experimental_direct_stereo>=2) {
            FhAudioDspCurve curve={.rate=c.rate,
                .volume_start={v->lastVolL,v->lastVolR,v->lastVolS,v->lastVolLa,v->lastVolRa,v->lastVolSa,v->lastVolLb,v->lastVolRb,v->lastVolSb},
                .volume_end={v->volL,v->volR,v->volS,v->volLa,v->volRa,v->volSa,v->volLb,v->volRb,v->volSb},
                .env_start=s->env_start,.env_end=s->env_end,.offset=s->offset,
                .frames=s->frames,.cycle_frames=s->cycle_frames,
                .interpolation=c.interpolation,.paused=c.paused,
                .routing=n->experimental_direct_stereo==4?FH_AUDIO_DSP_ROUTE_DPL2_STEREO:FH_AUDIO_DSP_ROUTE_STD};
            /* The original does not return a bus with no handler. Do not let
             * the DSP's disabled-AUX bypass accidentally turn it into dry audio. */
            if(n->experimental_direct_stereo>=3)for(unsigned bus=0;bus<(n->experimental_direct_stereo==4?1u:2u);bus++)if(!n->aux_handlers[bus])
                for(unsigned ch=0;ch<3;ch++)curve.volume_start[3+bus*3+ch]=curve.volume_end[3+bus*3+ch]=0;
            status=fh_audio_dsp_voice_curve_at(n->dsp,i+FH_AUDIO_DSP_FIRST_CHANNEL,token,&curve,s->source_frame);
        } else status=fh_audio_dsp_voice_control_at(n->dsp,i+FH_AUDIO_DSP_FIRST_CHANNEL,token,&c,s->source_frame);
        if(status) {
            n->faults++;return 0;
        }
        a->controls=c;n->segments++;
        unsigned count=pending(&n->dsp->voices[i]);if(count>n->peak_pending)n->peak_pending=count;
        return 1;
    }
    n->faults++;return 0;
}
static int stop(void *context,uint64_t token,uint64_t frame) {
    FhAudioNativeVoices *n=context;
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(n->voices[i].token==token) {
        /* No pretend source depop: this candidate stops at the source deadline.
         * A source-compatible release/tail is required before game admission. */
        if(fh_audio_dsp_voice_stop_at(n->dsp,i+FH_AUDIO_DSP_FIRST_CHANNEL,token,frame)){n->faults++;return 0;}
        n->voices[i].token=0;n->stops++;return 1;
    }
    n->faults++;return 0;
}
int fh_audio_native_voices_init(FhAudioNativeVoices *n,FhAudioDspVoices *dsp,
    FhAudioSampleRegistry *registry,uint64_t epoch,unsigned experimental) {
    if(!n || !dsp || !dsp->initialized || !registry || registry->pool!=dsp->retirement.pool ||
       !epoch || experimental>4 || FH_AUDIO_DSP_COMMANDS<256 || n->initialized)return 0;
    memset(n,0,sizeof(*n));n->dsp=dsp;n->registry=registry;n->epoch=epoch;
    n->experimental_direct_stereo=experimental;
    n->bridge.backend=(FhAudioSourceBackend){begin,segment,stop,n};n->initialized=1;return 1;
}
int fh_audio_native_voices_bind_aux(FhAudioNativeVoices *n,unsigned studio) {
    if(!n || !n->initialized || n->experimental_direct_stereo<3 || studio>=8)return 0;
    /* A reset clears only the synth mappings. Older immutable playback leases
     * can still be audible: binding the NEW source must neither wait for nor
     * modify them. Same/future-epoch and immediate leases are not covered by
     * that separation. Caller owns the independently retained effect route. */
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++) {
        const FhAudioDspVoice *v=&n->dsp->voices[i];
        if(n->voices[i].token || (v->lease && (!v->timed || v->epoch>=n->epoch)))return 0;
    }
    for(unsigned i=0;i<SYNTH_MAX_VOICES;i++)if(n->bridge.voices[i].token)return 0;
    const DSPstudioinfo *s=&dspStudio[studio];
    unsigned type=n->experimental_direct_stereo==4?SND_STUDIO_TYPE_DPL2:SND_STUDIO_TYPE_STD;
    if(s->state!=DSP_STUDIO_STATE_ACTIVE || !s->isMaster || s->type!=type || s->numInputs)return 0;
    n->aux_studio=studio;n->aux_epoch=n->epoch;
    n->aux_handlers[0]=s->auxAHandler;n->aux_handlers[1]=s->auxBHandler;
    n->aux_users[0]=s->auxAUser;n->aux_users[1]=s->auxBUser;n->aux_bound=1;return 1;
}
int fh_audio_native_voices_refresh_aux(FhAudioNativeVoices *n,unsigned studio) {
    if(!n || !n->initialized || n->experimental_direct_stereo<3 || !n->aux_bound ||
       n->aux_epoch!=n->epoch || studio>=8 || studio!=n->aux_studio)return 0;
    const DSPstudioinfo *s=&dspStudio[studio];
    unsigned type=n->experimental_direct_stereo==4?SND_STUDIO_TYPE_DPL2:SND_STUDIO_TYPE_STD;
    if(s->state!=DSP_STUDIO_STATE_ACTIVE || !s->isMaster || s->type!=type || s->numInputs)return 0;
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)
        if(n->voices[i].token && n->voices[i].studio!=studio)return 0;
    n->aux_handlers[0]=s->auxAHandler;n->aux_handlers[1]=s->auxBHandler;
    n->aux_users[0]=s->auxAUser;n->aux_users[1]=s->auxBUser;return 1;
}
int fh_audio_native_voices_reset(FhAudioNativeVoices *n,uint64_t epoch) {
    if(!n || !n->initialized || !epoch || epoch<=n->epoch)return 0;
    /* These two arrays belong exclusively to the original synth owner. The
     * callback owns playback and still needs the old immutable DSP queues and
     * sample leases. Do not abort them merely because preparation ran ahead. */
    memset(n->bridge.voices,0,sizeof(n->bridge.voices));
    memset(n->voices,0,sizeof(n->voices));
    n->epoch=epoch;
    /* Old source effect objects can have been freed/replaced by the reset.
     * No native admission may borrow their identity for the new generation. */
    n->aux_bound=0;n->aux_epoch=0;
    memset(n->aux_handlers,0,sizeof(n->aux_handlers));
    memset(n->aux_users,0,sizeof(n->aux_users));
    return 1;
}
int fh_audio_native_voices_available(const FhAudioNativeVoices *n) {
    if(!n || !n->initialized)return 0;
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(n->voices[i].token &&
        pending(&n->dsp->voices[i])>FH_AUDIO_DSP_COMMANDS-7)return 0;
    return 1;
}
int fh_audio_native_voices_poll(FhAudioNativeVoices *n) {
    if(!n || !n->initialized)return FH_AUDIO_POOL_INVALID;
    uint32_t completed=0;return fh_audio_dsp_voices_poll(n->dsp,&completed);
}
