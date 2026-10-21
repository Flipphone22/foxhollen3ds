#include "mixer.c"
#include <assert.h>
#include <stdio.h>
DSPstudioinfo dspStudio[8];
static DSPvoice voices[1];DSPvoice *dspVoice=voices;
u8 salMaxStudioNum=1,salNumVoices=1,salFrame,salAuxFrame;
static FhAudioSourceBridge bridge;
static s32 main_pcm[2][3*SAL_PC_MAX_FRAMES],aux_a[3][3*SAL_PC_MAX_FRAMES],aux_b[3][3*SAL_PC_MAX_FRAMES];
static unsigned native_run,trial,cycle,messages,stopped,begun,segment_calls,held_calls,total_segments,total_messages,total_stops;
static uint64_t serial,last_token,end_frame;
static DSPvoice expected[32];static unsigned expected_messages[32];static uint64_t expected_end[32];
static s16 expected_pcm[32][320];
static struct {DSPvoice voice;unsigned offset,frames,start,end;} slices[160];
static unsigned slice_count,expected_count[32],total_holds,total_declined;
void fh_test_segment_seen(const DSPvoice *v,unsigned offset,unsigned frames,unsigned start,unsigned end){
    if(native_run)return;
    assert(slice_count<160);slices[slice_count].voice=*v;slices[slice_count].offset=offset;
    slices[slice_count].frames=frames;slices[slice_count].start=start;slices[slice_count].end=end;slice_count++;
}
u32 salPCMixRate(void){return 32000;}
u32 salPCChannels(void){return 2;}
u32 salSynthSendMessage(DSPvoice *v,u32 message){
    (void)v;assert(message==HW_MESSAGE_SAMPLE_END);messages++;
    end_frame=salPCBoundary(cycleTick,32000)+renderOffset;return 0;
}
void salPCConvertCycle(s32 *a,u32 b,u32 c,u64 d,const s32 *e,u32 f,u32 g,u64 h,s32 i[32]) {
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;abort();
}
void salPCConvertStudioCycle(s32 *a,u32 b,u32 c,u64 d,const s32 *e,u32 f,u32 g,u64 h,s32 i[2]) {
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;abort();
}
static uint64_t begin(void *context,unsigned index,const DSPvoice *v,uint64_t frame){
    (void)context;assert(!index && v==voices && !last_token);
    assert(frame==trial%5*32);if(trial%13==0)return 0;
    begun++;last_token=++serial;return last_token;
}
static int segment(void *context,uint64_t token,const DSPvoice *v,const FhAudioSourceSegment *s){
    (void)context;assert(token && token==last_token && v==voices);
    assert(segment_calls<slice_count && !memcmp(v,&slices[segment_calls].voice,sizeof(*v)));
    assert(s->offset==slices[segment_calls].offset && s->frames==slices[segment_calls].frames);
    assert(s->env_start==slices[segment_calls].start && s->env_end==slices[segment_calls].end);
    assert(s->source_frame==cycle*160+s->offset && s->cycle_frames==160 && s->frames==32 && !(s->offset%32));
    unsigned pitch=v->playInfo.pitch?v->playInfo.pitch:v->pitch[0];
    assert(s->pitch==pitch && s->held==(!pitch && v->srcTypeSelect!=SAL_SRC_NONE));
    assert(s->env_end==(u16)CLAMP(v->adsr.currentVolume>>16,0,0x7fff));
    segment_calls++;held_calls+=s->held;return 1;
}
static int stop(void *context,uint64_t token,uint64_t frame){
    (void)context;assert(token && token==last_token);
    /* Source sample/ADSR notification occurs before deactivation; explicit
     * break does not emit SAMPLE_END, so frame is independently bounded. */
    assert(frame>=cycle*160 && frame<=(cycle+1)*160);
    stopped++;last_token=0;return 1;
}
static int reject_segment(void *context,uint64_t token,const DSPvoice *v,const FhAudioSourceSegment *s){
    (void)context;(void)token;(void)v;(void)s;return 0;
}
static int reject_stop(void *context,uint64_t token,uint64_t frame){
    (void)context;(void)token;(void)frame;return 0;
}
static void setup(unsigned native,unsigned char *encoded,DSPADPCMplusInfo *info){
    native_run=native;if(!native)slice_count=0;
    memset(voices,0,sizeof(voices));memset(dspStudio,0,sizeof(dspStudio));
    memset(voiceResampler,0,sizeof(voiceResampler));memset(lastContribution,0,sizeof(lastContribution));memset(depop,0,sizeof(depop));
    memset(main_pcm,0,sizeof(main_pcm));memset(aux_a,0,sizeof(aux_a));memset(aux_b,0,sizeof(aux_b));
    memset(rearMain,0,sizeof(rearMain));memset(&bridge,0,sizeof(bridge));
    bridge.backend=(FhAudioSourceBackend){begin,segment,stop,NULL};fh_audio_source_bridge=native?&bridge:NULL;
    messages=begun=stopped=segment_calls=held_calls=0;last_token=end_frame=0;depopCursor=0;
    DSPstudioinfo *studio=&dspStudio[0];studio->state=DSP_STUDIO_STATE_ACTIVE;studio->isMaster=1;
    for(unsigned i=0;i<2;i++)studio->main[i]=main_pcm[i];
    for(unsigned i=0;i<3;i++){studio->auxA[i]=aux_a[i];studio->auxB[i]=aux_b[i];}
    studio->voiceRoot=voices;
    DSPvoice *v=voices;v->state=DSP_VOICE_STATE_STARTING;v->virtualSampleID=UINT32_MAX;
    v->smp_info=(SAMPLE_INFO){.addr=encoded,.extraData=info,.length=trial%4?2048:49,
        .loop=14,.loopLength=trial%3?28:0,.compType=SAMPLE_TYPE_ADPCM};
    v->srcTypeSelect=trial%3;v->srcCoefSelect=trial%4;v->singleOffset=trial%5;
    v->adsr.mode=ADSR_MODE_LINEAR;
    v->adsr.data.linear.aTime=1+trial%7;v->adsr.data.linear.dTime=2+trial%11;
    v->adsr.data.linear.sLevel=10000+trial%20000;v->adsr.data.linear.rTime=1+trial%9;
    for(unsigned i=0;i<5;i++){v->pitch[i]=32768+i*8192;v->changed[i]=8;}
    v->volL=23000;v->volR=18000;v->volS=1000;v->volLa=2000;v->volRa=5000;v->volSa=400;
    v->volLb=1000;v->volRb=3000;v->volSb=700;v->volRear[0]=400;v->volRear[1]=800;
}
int main(void){
    unsigned char encoded[2048];for(unsigned i=0;i<sizeof(encoded);i++)encoded[i]=(unsigned char)(i*37);
    DSPADPCMplusInfo info={.initialPS=0x11,.loopPS=0x20,.loopY0=300,.loopY1=-700};
    for(trial=0;trial<512;trial++)for(unsigned native=0;native<2;native++){
        setup(native,encoded,&info);
        for(cycle=0;cycle<24;cycle++){
            DSPvoice *v=voices;
            if(cycle){
                memset(v->changed,0,sizeof(v->changed));
                for(unsigned i=0;i<5;i++){
                    v->changed[i]=8;v->pitch[i]=(cycle%4==1)?0:32768+((trial+i+cycle)%9)*16384;
                }
                v->volL=10000+cycle*201;v->volR=21000-cycle*303;
                if(cycle==7)v->changed[trial%5]|=0x40;
                if(cycle==4 && trial%7==0)v->changed[trial%5]|=0x10;
                if(cycle==5 && trial%11==0)v->changed[trial%5]|=0x20;
            }
            salFrame=cycle&1;salAuxFrame=cycle%3;salPCBeginCycle(cycle*5);s16 output[320];salCtrlDsp(output);
            if(!native){expected[cycle]=*v;expected_messages[cycle]=messages;expected_end[cycle]=end_frame;
                expected_count[cycle]=slice_count;memcpy(expected_pcm[cycle],output,sizeof(output));}
            else {
                assert(!memcmp(v,&expected[cycle],sizeof(*v)));
                assert(messages==expected_messages[cycle] && end_frame==expected_end[cycle]);
                assert(!bridge.faults);
                if(trial%13)assert(segment_calls==expected_count[cycle]);
                else assert(!memcmp(output,expected_pcm[cycle],sizeof(output)));
            }
        }
        assert(voices[0].state==DSP_VOICE_STATE_INACTIVE);
        if(native){
            if(trial%13)assert(begun==1 && stopped==1 && !last_token && segment_calls);
            else{assert(!begun && !stopped && !last_token && !segment_calls && bridge.declined==1);total_declined++;}
            total_segments+=segment_calls;total_messages+=messages;total_stops+=stopped;total_holds+=held_calls;}
    }
    assert(total_holds && total_declined);
    /* Explicit backend failure must retain ownership/state, never present a
     * software fallback after a note has stopped maintaining CPU history. */
    trial=1;cycle=0;setup(1,encoded,&info);
    assert(fh_audio_source_begin(0,voices,32,32000));uint64_t first=last_token;
    assert(fh_audio_source_begin(0,voices,32,32000));assert(last_token!=first && stopped==1 && begun==2);
    FhAudioSourceVoice before=bridge.voices[0];DSPvoice prior=voices[0];
    FhAudioSourceSegment span={.source_frame=32,.pitch=65536,.offset=32,.frames=32,.cycle_frames=160,.env_end=10000};
    bridge.backend.segment=reject_segment;
    assert(fh_audio_source_segment(0,voices,&span)==-2 && !memcmp(&before,&bridge.voices[0],sizeof(before)) && !memcmp(&prior,voices,sizeof(prior)));
    bridge.backend.stop=reject_stop;
    assert(!fh_audio_source_stop(0,64) && !memcmp(&before,&bridge.voices[0],sizeof(before)));
    bridge.backend.stop=stop;assert(fh_audio_source_stop(0,64));assert(!last_token);
    voices[0].smp_info.compType=SAMPLE_TYPE_ADPCM_VIRTUAL;
    assert(fh_audio_source_begin(0,voices,32,32000) && !bridge.voices[0].token);
    assert(fh_audio_source_segment(0,voices,&span)==-1);
    printf("trials=512 cycles=24576 native_segments=%u held_segments=%u source_messages=%u native_stops=%u declined=%u owner_bytes=%zu\n",
        total_segments,total_holds,total_messages,total_stops,total_declined,sizeof(bridge));
}
