#include <3ds.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fh_platform.h"
#include "fh_audio_worker.h"
u32 __ctru_heap_size=4*1024*1024;
u32 __ctru_linear_heap_size=4*1024*1024;
unsigned fh_hardware_is_new=
#ifdef FH_TEST_OLD
0;
#else
1;
#endif
static FILE *logfile;
static s16 capture_samples[32000*2],snapshot[32000*2];
static ndspWaveBuf capture;
static volatile unsigned request,complete;
#ifdef FH_SHARED_WORKER_TEST
unsigned expected_audio_fp;
static unsigned extra_posted,extra_done,extra_checks,extra_payload[64];
static int extra_work(void){
    unsigned next=__atomic_load_n(&extra_posted,__ATOMIC_ACQUIRE);
    if(next==__atomic_load_n(&extra_done,__ATOMIC_RELAXED))return 0;
    if(!fh_audio_worker_is_current())fh_platform_panic("Attached work ran on producer");
    for(unsigned i=0;i<64;i++)if(extra_payload[i]!=(next^i))fh_platform_panic("Attached payload lifetime");
    unsigned fp;__asm__ volatile("vmrs %0, fpscr":"=r"(fp));fp=(fp&~0x00c00000u)|((next&3)<<22);
    __asm__ volatile("vmsr fpscr, %0"::"r"(fp));
    fh_audio_worker_service_current(); /* Must not block if producer holds synth lock. */
    extra_checks++;__atomic_store_n(&extra_done,next,__ATOMIC_RELEASE);return 1;
}
static void extra_publish_and_join(void){
    /* Deliberately wait with the synth lock held: rendering must still run. */
    fh_audio_lock();unsigned next=__atomic_load_n(&extra_posted,__ATOMIC_RELAXED)+1;
    for(unsigned i=0;i<64;i++)extra_payload[i]=next^i;
    __atomic_store_n(&extra_posted,next,__ATOMIC_RELEASE);fh_audio_worker_wake();
    u64 begin=svcGetSystemTick();
    while(__atomic_load_n(&extra_done,__ATOMIC_ACQUIRE)!=next){
        if(svcGetSystemTick()-begin>2ULL*SYSCLOCK_ARM11)fh_platform_panic("Shared owner deadlocked on producer synth lock");
        svcSleepThread(100000);
    }
    fh_audio_unlock();
}
#endif
extern void fh_platform_audio_shutdown(void);
void fh_platform_log(const char *text) { if(logfile) { fputs(text,logfile);fflush(logfile); } }
void fh_platform_panic(const char *text) { fh_platform_log(text);exit(2); }
uint64_t fh_platform_ticks(void){return svcGetSystemTick()*40500000/SYSCLOCK_ARM11;}
static void snapshot_callback(void *unused) {
    (void)unused;if(!request) return;
    unsigned offset=capture.offset;
    memcpy(snapshot,capture_samples+offset*2,(32000-offset)*4);
    memcpy(snapshot+(32000-offset)*2,capture_samples,offset*4);
    __sync_synchronize();complete=1;request=0;
}
#ifdef FH_AUDIO_SYNTH_JOBS_TEST
unsigned sample;
extern void fh_jobs_device_start(void),fh_jobs_device_restart(void);
#else
static unsigned sample;
#endif
static int fill_audio(void) {
    if(!fh_platform_audio_space(0))return 0;
    s16 pcm[640];
    for(unsigned n=0;n<320;n++,sample++) {
        pcm[2*n]=(s16)(12000*sin(6.283185307179586*440*sample/32000));
        pcm[2*n+1]=(s16)(8000*sin(6.283185307179586*660*sample/32000));
    }
    fh_platform_audio_queue(0,pcm,320,32000);return 1;
}
int main(void) {
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow-audio-test",0777);
    logfile=fopen("sdmc:/3ds/foxhollow-audio-test/progress.log","wb");
    gfxInitDefault();bool is_new=false;APT_CheckNew3DS(&is_new);
    if(is_new) osSetSpeedupEnable(true);
    fh_platform_audio_pause(0,0);
    (void)fh_platform_audio_space(0);
    capture.data_pcm16=capture_samples;capture.nsamples=32000;
    ndspSetCapture(&capture);ndspSetCallback(snapshot_callback,NULL);
#ifdef FH_AUDIO_WORKER_TEST
        extern void fh_audio_worker_test_hook(unsigned);
#ifdef FH_SHARED_WORKER_TEST
    fh_audio_shared_executor_enabled=1;
    __asm__ volatile("vmrs %0, fpscr":"=r"(expected_audio_fp));expected_audio_fp&=0x03c00000u;
#endif
#ifdef FH_AUDIO_SYNTH_JOBS_TEST
    fh_jobs_device_start();
#else
    fh_audio_worker_start(fill_audio);
#endif
    if(!fh_audio_worker_active())fh_platform_panic("Audio fixture worker unavailable");
#ifdef FH_SHARED_WORKER_TEST
    if(!fh_audio_worker_attach(extra_work) || fh_audio_worker_attach(extra_work))fh_platform_panic("Shared owner attachment contract");
#endif
#endif
    u64 started=svcGetSystemTick();
    for(unsigned frame=0;frame<300 && aptMainLoop();frame++) {
#ifdef FH_SHARED_WORKER_TEST
        extra_publish_and_join();
#endif
#ifdef FH_AUDIO_SYNTH_JOBS_TEST
        for(unsigned poll=0;poll<20;poll++){fh_audio_pump();svcSleepThread(250000);}
#endif
        if(!fh_audio_worker_active())for(unsigned i=0;i<8 && fill_audio();i++){}
#ifdef FH_AUDIO_WORKER_TEST
        if(frame==60) {
            fh_audio_lock();fh_audio_lock();fh_platform_audio_pause(0,1);unsigned paused_sample=sample;fh_audio_unlock();fh_audio_unlock();
            for(unsigned i=0;i<3;i++)gspWaitForVBlank();
            fh_audio_lock();if(sample!=paused_sample)fh_platform_panic("Audio worker advanced while paused");
            fh_platform_audio_reset(0);fh_platform_audio_pause(0,0);fh_audio_unlock();
        }
        if(frame==80) {
            fh_audio_worker_test_hook(APTHOOK_ONSLEEP);
            unsigned frozen_sample=sample;
            for(unsigned i=0;i<3;i++)gspWaitForVBlank();
            if(sample!=frozen_sample)fh_platform_panic("Audio worker advanced after suspend barrier");
            fh_audio_worker_test_hook(APTHOOK_ONWAKEUP);
        }
        if(frame==100) {
#ifdef FH_SHARED_WORKER_TEST
            fh_audio_worker_detach();
#endif
            fh_audio_worker_stop();
            unsigned frozen_sample=sample;
            for(unsigned i=0;i<2;i++)gspWaitForVBlank();
            if(fh_audio_worker_active() || sample!=frozen_sample)fh_platform_panic("Audio worker survived stop");
#ifdef FH_AUDIO_SYNTH_JOBS_TEST
            fh_jobs_device_restart();
#else
            fh_audio_worker_start(fill_audio);
#endif
            if(!fh_audio_worker_active())fh_platform_panic("Audio worker restart failed");
#ifdef FH_SHARED_WORKER_TEST
            if(!fh_audio_worker_attach(extra_work))fh_platform_panic("Shared owner reattachment failed");
#endif
        }
        if(frame==120) {
            fh_audio_worker_test_hook(APTHOOK_ONSUSPEND);
            unsigned frozen_sample=sample;
            for(unsigned i=0;i<3;i++)gspWaitForVBlank();
            if(sample!=frozen_sample)fh_platform_panic("Audio worker advanced after second suspend barrier");
            fh_audio_worker_test_hook(APTHOOK_ONRESTORE);
        }
#endif
        if(frame==180) request=1;
        if(frame%30==0) { fh_audio_lock();char line[64];snprintf(line,sizeof(line),"frame=%u samples=%u captured=%u\n",frame,sample,complete);fh_platform_log(line);fh_audio_unlock(); }
        if(complete) break;
        gspWaitForVBlank();
    }
    double seconds=(svcGetSystemTick()-started)/(double)SYSCLOCK_ARM11;
    ndspSetCallback(NULL,NULL);ndspSetCapture(NULL);
    int ok=complete!=0;
    if(ok) {
        FILE *pcm=fopen("sdmc:/3ds/foxhollow-audio-test/output.pcm","wb");
        if(!pcm) ok=0;
        else { ok=fwrite(snapshot,1,sizeof(snapshot),pcm)==sizeof(snapshot);if(fclose(pcm)) ok=0; }
    }
    unsigned active,jobs,stack,core;fh_audio_worker_stats(&active,&jobs,&stack,&core);
    unsigned stack_bytes=fh_audio_worker_stack_bytes();
#ifdef FH_SHARED_WORKER_TEST
    fh_audio_worker_detach();
    if(extra_checks<180 || extra_posted!=extra_done)fh_platform_panic("Shared jobs not retired");
    FILE *extra_report=fopen("sdmc:/3ds/foxhollow-audio-test/shared.json","wb");
    if(extra_report){fprintf(extra_report,"{\"jobs\":%u,\"completed\":%u,\"checks\":%u}\n",extra_posted,extra_done,extra_checks);fclose(extra_report);}
#endif
    fh_platform_audio_shutdown();
    if(fh_audio_worker_active())fh_platform_panic("Audio worker survived joined shutdown");
#ifdef FH_AUDIO_SYNTH_JOBS_TEST
    extern unsigned fh_audio_synth_jobs_queued,fh_audio_synth_jobs_worker,fh_audio_synth_jobs_inline;
    if(fh_audio_synth_jobs_queued!=fh_audio_synth_jobs_worker+fh_audio_synth_jobs_inline || fh_audio_synth_jobs_worker<128)fh_platform_panic("Synth jobs not exercised or drained");
    FILE *synth_report=fopen("sdmc:/3ds/foxhollow-audio-test/synth-jobs.json","wb");
    if(synth_report){fprintf(synth_report,"{\"queued\":%u,\"worker\":%u,\"inline\":%u}\n",fh_audio_synth_jobs_queued,fh_audio_synth_jobs_worker,fh_audio_synth_jobs_inline);fclose(synth_report);}
#endif
    FILE *worker_report=fopen("sdmc:/3ds/foxhollow-audio-test/worker.json","wb");
    if(worker_report){fprintf(worker_report,"{\"active\":%u,\"jobs\":%u,\"stack_peak_upper_bound\":%u,\"stack_bytes\":%u,\"reported_core\":%u,\"joined\":true}\n",active,jobs,stack,stack_bytes,core);fclose(worker_report);}
    FILE *result=fopen("sdmc:/3ds/foxhollow-audio-test/result.json","wb");
    if(result) { fprintf(result,"{\"pass\":%s,\"frames\":32000,\"rate\":32728,\"hle_fixture\":true,\"submitted_samples\":%u,\"guest_seconds\":%.9f}\n",ok?"true":"false",sample,seconds);fclose(result); }
    if(logfile) fclose(logfile);
    gfxExit();return ok?0:1;
}
