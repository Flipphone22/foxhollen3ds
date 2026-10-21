/* Exact queue/AUX firmware integration; synthetic producer and identity effect. */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fh_audio_pcm_queue.h"
#include "fh_audio_dsp_aux.h"
#include "fh_audio_aux_native.h"
u32 __ctru_heap_size=4*1024*1024,__ctru_linear_heap_size=2*1024*1024;
static FhAudioPcmQueue queue;
static FhAudioDspAux aux;
static FhAudioAuxInput input;
static FhAudioAuxStream stream;
static FhAudioEffectRoutes routes;
static s16 pcm[320];
static int32_t raw[3][160];
static unsigned produced,processed,held,errors,first_error,callbacks,maxdrops;
static volatile unsigned finished;
static unsigned trace[320][8];
static FILE *progress;
static void stage(unsigned n){fprintf(progress,"stage %u\n",n);fflush(progress);}
int fh_audio_ndsp_processor(void){return 0;}
void fh_platform_log(const char *s){(void)s;}
__attribute__((noreturn)) void fh_platform_panic(const char *s){(void)s;exit(90);}
uint64_t fh_audio_cost_now(void){return svcGetSystemTick();}
void fh_audio_cost_finish(unsigned b,uint64_t t){(void)b;(void)t;}
void fh_audio_game_output_mix(unsigned ch,const float mix[12]){float copy[12];memcpy(copy,mix,sizeof(copy));ndspChnSetMix(ch,copy);}
static int flush(void *c,const void *p,size_t n){(void)c;return R_SUCCEEDED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)p,n));}
static void *create(void *c,const FhAudioEffectKey *k){(void)c;(void)k;return malloc(4);}
static void destroy(void *c,void *p){(void)c;free(p);}
static int admit(void *c,void *p,unsigned b){(void)c;(void)p;(void)b;return 1;}
static int effect(void *c,void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n){(void)c;(void)p;(void)l;(void)r;(void)s;(void)n;return 1;}
static void error(void){if(!errors)first_error=callbacks;errors++;}
static void returned(void *c,int32_t samples[4][160],uint32_t token,const FhAudioAuxWindow *w){
    (void)c;(void)token;
    if(finished)return;
    int result=fh_audio_aux_native_shared(&stream,&routes,w,0,1,&input,samples,admit,effect,NULL);
    if(result==FH_AUDIO_AUX_PROCESSED){
        processed++;
        for(unsigned i=0;i<160;i++)if(samples[0][i]!=100 || samples[1][i]!=200){error();break;}
    }else if(result==FH_AUDIO_AUX_HELD)held++;
    else error();
}
static void callback(void *unused){
    (void)unused;if(finished)return;
    unsigned n=callbacks++;
    /* Hold source production to exercise guarded pause/refill after the drop. */
    if(n<60 || n>=100)while(fh_audio_pcm_space(&queue,0)){
        if(input.head-input.tail>=FH_AUX_INPUT_SLOTS)break;
        FhAudioMap map={0};
        if(!fh_audio_map_append(&map,(FhAudioSpan){produced,1,0,160,1},0,160)){error();break;}
        if(fh_audio_aux_input_push(&input,1,1,produced,raw[0],raw[1],raw[2])){error();break;}
        if(fh_audio_pcm_queue_mapped(&queue,0,pcm,160,32000,&map)){error();break;}
        produced+=160;
    }
    fh_audio_pcm_service(&queue);
    FhAudioAuxWindow w=fh_audio_pcm_aux_window(&queue,0);
    fh_audio_dsp_aux_tag_mapped(&aux,0,w.source.valid?&w:NULL);
    unsigned drops=ndspGetDroppedFrames();if(drops>maxdrops)maxdrops=drops;
    if(n<320){
        unsigned *t=trace[n];t[0]=ndspGetFrameCount();t[1]=drops;t[2]=queue.channels[0].output_clock.valid;
        t[3]=queue.channels[0].hardware_paused;t[4]=stream.next_input;t[5]=processed;t[6]=errors;t[7]=queue.channels[0].underruns;
    }
    if(n==40)svcSleepThread(80000000);
    if(n==299)__atomic_store_n(&finished,1,__ATOMIC_RELEASE);
}
int main(void){
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/fh-audio-drop",0777);
    progress=fopen("sdmc:/3ds/fh-audio-drop/progress.txt","wb");if(!progress)return 1;
    stage(1);gfxInitDefault();stage(2);
    size_t bytes=FH_PCM_CHANNELS*FH_PCM_SLOTS*FH_PCM_FRAMES*4;
    s16 *storage=linearMemAlign(bytes,32),*silence=linearMemAlign(FH_PCM_SILENCE*4,32);
    size_t rawbytes=fh_audio_aux_input_bytes(2);int32_t *rawstore=malloc(rawbytes);
    if(!storage || !silence || !rawstore)return 2;
    stage(3);
    if(fh_audio_pcm_init(&queue,storage,bytes,silence,FH_PCM_SILENCE*4,flush,NULL))return 3;
    stage(4);queue.channels[0].output_guard=queue.channels[0].effect_order_guard=1;
    if(!fh_audio_aux_input_init(&input,rawstore,rawbytes,2))return 4;
    stage(5);if(!fh_audio_effect_routes_init(&routes,create,destroy,NULL))return 5;
    FhAudioEffectKey keys[2]={{.source_token=1,.parameters={.5f,3,.9f,.5f,.01f}},{0}};uint32_t token;
    if(fh_audio_effect_routes_prepare(&routes,keys,&token) || token!=1)return 6;
    stage(6);for(unsigned i=0;i<160;i++){raw[0][i]=100;raw[1][i]=200;}
    aux.mapped_process[0]=returned;if(!fh_audio_dsp_aux_install(&aux))return 7;
    stage(7);if(R_FAILED(ndspInit()))return 8;stage(8);
    ndspAuxSetEnable(0,true);ndspAuxSetFrontBypass(0,false);ndspAuxSetVolume(0,1);
    ndspSetCallback(callback,NULL);
    for(unsigned i=0;i<700 && !__atomic_load_n(&finished,__ATOMIC_ACQUIRE);i++)svcSleepThread(10000000);
    ndspExit();fh_audio_dsp_aux_uninstall();
    FILE *f=fopen("sdmc:/3ds/fh-audio-drop/result.json","wb");if(!f)return 9;
    fprintf(f,"{\"callbacks\":%u,\"processed\":%u,\"held\":%u,\"errors\":%u,\"first_error\":%u,\"drops\":%u,\"underruns\":%u,\"input\":%llu}\n",callbacks,processed,held,errors,first_error,maxdrops,queue.channels[0].underruns,(unsigned long long)stream.next_input);fclose(f);
    f=fopen("sdmc:/3ds/fh-audio-drop/trace.csv","wb");if(!f)return 10;
    fprintf(f,"callback,dropped,valid,paused,input,processed,errors,underruns\n");
    for(unsigned i=0;i<callbacks && i<320;i++){unsigned *t=trace[i];fprintf(f,"%u,%u,%u,%u,%u,%u,%u,%u\n",t[0],t[1],t[2],t[3],t[4],t[5],t[6],t[7]);}fclose(f);
    fh_audio_effect_routes_shutdown(&routes);free(rawstore);linearFree(storage);linearFree(silence);gfxExit();return 0;
}
