/* Native DMA + actual worker/NDSP fixture. Synth input is a known stereo tone;
 * movie wrapping preserves the game's ring-copy ownership pattern. */
#include <dolphin/ai.h>
#include <dolphin/dvd.h>
#include <musyx/pc.h>
#include <math.h>
#include <string.h>
#include "fh_platform.h"
#include "fh_audio_worker.h"
extern unsigned sample,fh_audio_synth_jobs_enabled;
extern void fhMusyxStartOutput(void);
extern uintptr_t fhAIGetDMAStartAddr(void);
static AIDCallback previous;
static s16 movie[2][320];static unsigned movie_index;
volatile u32 fh_audio_voice_starts,fh_audio_voice_stops;
bool sndPCRender(s16 *pcm,size_t frames){
#ifdef FH_SHARED_WORKER_TEST
    extern unsigned expected_audio_fp;unsigned fp;__asm__ volatile("vmrs %0, fpscr":"=r"(fp));
    if((fp&0x03c00000u)!=expected_audio_fp)fh_platform_panic("Render FP controls leaked into synth");
#endif
    for(unsigned n=0;n<frames;n++,sample++){
        pcm[2*n]=(s16)(12000*sin(6.283185307179586*440*sample/32000));
        pcm[2*n+1]=(s16)(8000*sin(6.283185307179586*660*sample/32000));
    }
    return true;
}
void AttractMovieAudio_DmaCallback(void){
    previous();s16 *dest=movie[(movie_index++)&1];
    memcpy(dest,(void*)fhAIGetDMAStartAddr(),640);AIInitDMA((uintptr_t)dest,640);
}
s32 DVDReadPrio(DVDFileInfo*f,void*d,s32 n,s32 o,s32 p){(void)f;(void)d;(void)n;(void)o;(void)p;fh_platform_panic("Unexpected fixture disc read");}
void fh_jobs_device_start(void){
    fh_audio_synth_jobs_enabled=1;AISetDSPSampleRate(AI_SAMPLERATE_32KHZ);
    fhMusyxStartOutput();previous=AIRegisterDMACallback(AttractMovieAudio_DmaCallback);
}
/* Restart through the real initialization path after stopping DMA: the final
 * output-frequency window begins well after this intentional reset. */
void fh_jobs_device_restart(void){AIStopDMA();fh_jobs_device_start();}

void fh_dvd_stream_prepare(DVDFileInfo *file){(void)file;}
