#include <dolphin/ai.h>
#include <dolphin/dvd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_platform.h"
extern int fhAIPrepareStream(void *);
static FILE *source,*output;
static unsigned available,bytes_read,maximum_read,queued,finished;
uint64_t fh_platform_ticks(void) { return 0; }
void fh_platform_log(const char *text) { (void)text; }
void fh_platform_panic(const char *message) { fprintf(stderr,"PANIC: %s\n",message);abort(); }
s32 DVDReadPrio(DVDFileInfo *file,void *memory,s32 bytes,s32 offset,s32 priority) {
    (void)priority;
    assert(bytes>0 && bytes<=32768 && offset>=0 && (unsigned)offset+(unsigned)bytes<=file->length);
    assert(fseek(source,offset,SEEK_SET)==0);assert(fread(memory,1,bytes,source)==(unsigned)bytes);
    bytes_read+=bytes;if((unsigned)bytes>maximum_read) maximum_read=bytes;return bytes;
}
int fh_platform_audio_space(unsigned channel) { assert(channel==1);return available!=0; }
void fh_platform_audio_queue(unsigned channel,const int16_t *pcm,unsigned frames,unsigned rate) {
    assert(channel==1 && available && frames>0 && frames<=504 && rate==48043);
    assert(fwrite(pcm,4,frames,output)==frames);--available;queued+=frames;
}
void fh_platform_audio_pause(unsigned channel,int paused) { (void)channel;(void)paused; }
void fh_platform_audio_reset(unsigned channel) { assert(channel==1); }
void fh_platform_audio_finish(unsigned channel) { assert(channel==1);++finished; }
int main(int argc,char **argv) {
    assert(argc==3);source=fopen(argv[1],"rb");output=fopen(argv[2],"wb");assert(source && output);
    assert(fseek(source,0,SEEK_END)==0);long bytes=ftell(source);assert(bytes==48000);
    DVDFileInfo file={.length=bytes};assert(fhAIPrepareStream(&file));
    AISetStreamVolLeft(173);AISetStreamVolRight(231);AISetStreamPlayState(AI_STREAM_START);
    unsigned loops=0;
    while(AIGetStreamPlayState()==AI_STREAM_START && loops++<1000) { available=2;fh_audio_pump(); }
    assert(loops<1000 && queued==42000 && bytes_read==48000 && maximum_read==32768 && finished);
    assert(AIGetStreamSampleCount()==42000);
    assert(fclose(output)==0);fclose(source);
    puts("PASS: bounded ADPCM stream reads and 42,000 stereo frames");return 0;
}

void fh_audio_lock(void) {}
void fh_audio_unlock(void) {}
int fh_audio_worker_active(void) {return 0;}
void fh_audio_worker_start(int (*step)(void)) {(void)step;}
void fh_audio_worker_wake(void) {}
unsigned fh_platform_audio_buffered(unsigned channel){(void)channel;return 0;}
void AttractMovieAudio_DmaCallback(void){abort();}

int fh_audio_try_lock(void) {return 1;}

volatile u32 fh_audio_voice_starts,fh_audio_voice_stops;
bool sndPCRender(s16 *output,size_t frames) {(void)output;(void)frames;abort();}

void fh_dvd_stream_prepare(DVDFileInfo *file){(void)file;}

