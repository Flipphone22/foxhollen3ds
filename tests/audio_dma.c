#include <dolphin/ai.h>
#include <dolphin/dvd.h>
#include <musyx/pc.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_platform.h"
extern void fhMusyxStartOutput(void);
extern uintptr_t fhAIGetDMAStartAddr(void);
volatile u32 fh_audio_voice_starts,fh_audio_voice_stops;
static unsigned available,produced,consumed,submissions,wrapped;
static AIDCallback previous;
static s16 movie[2][160*2];
void fh_platform_log(const char *text){(void)text;}
void fh_platform_panic(const char *text){fprintf(stderr,"%s\n",text);abort();}
uint64_t fh_platform_ticks(void){static uint64_t ticks;return ++ticks;}
void fh_platform_audio_pause(unsigned channel,int paused){(void)channel;(void)paused;}
void fh_platform_audio_reset(unsigned channel){(void)channel;}
void fh_platform_audio_finish(unsigned channel){assert(channel==1);}
int fh_platform_audio_space(unsigned channel){return channel==0 && available;}
unsigned fh_platform_audio_buffered(unsigned channel){(void)channel;return 8*640;}
void fh_platform_audio_queue(unsigned channel,const int16_t *pcm,unsigned frames,unsigned rate){
    assert(channel==0 && frames==640 && rate==32000 && available);--available;++submissions;
    for(unsigned i=0;i<frames;i++,consumed++) {
        unsigned index=consumed<480?0:consumed-480;
        s16 expected=consumed<480?0:(s16)(1000+index*13);
        assert(pcm[i*2]==expected && pcm[i*2+1]==expected);
    }
}
bool sndPCRender(s16 *pcm,size_t frames){
    assert(frames==160);
    for(unsigned i=0;i<frames;i++,produced++)pcm[i*2]=pcm[i*2+1]=(s16)(1000+produced*13);
    return true;
}
s32 DVDReadPrio(DVDFileInfo *file,void *data,s32 size,s32 offset,s32 priority){(void)file;(void)data;(void)size;(void)offset;(void)priority;abort();}
void AttractMovieAudio_DmaCallback(void){
    previous();assert(AIGetDMALength()==640);
    s16 *source=(s16*)fhAIGetDMAStartAddr();
    s16 *dest=movie[(wrapped++)&1];memcpy(dest,source,640);AIInitDMA((uintptr_t)dest,640);
}
int main(void){
    AISetDSPSampleRate(AI_SAMPLERATE_32KHZ);fhMusyxStartOutput();previous=AIRegisterDMACallback(AttractMovieAudio_DmaCallback);
    for(unsigned i=0;i<100;i++){available=3;fh_audio_pump();assert(!available);}
    assert(submissions==300 && consumed==192000 && produced==consumed && wrapped==1200);
    puts("PASS: 192000 contiguous stereo frames through 160-frame movie DMA callbacks and 640-frame NDSP submissions");
}

void fh_audio_lock(void) {}
void fh_audio_unlock(void) {}
int fh_audio_worker_active(void) {return 0;}
void fh_audio_worker_start(int (*step)(void)) {(void)step;}
void fh_audio_worker_wake(void) {}

int fh_audio_try_lock(void) {return 1;}

void fh_dvd_stream_prepare(DVDFileInfo *file){(void)file;}

