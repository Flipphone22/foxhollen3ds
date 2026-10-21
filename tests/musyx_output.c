#include "fh_musyx_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
u8 sndActive;
u32 last_rnd;
static unsigned locks,cycles;
void hwIRQEnterCritical(void){++locks;}
void hwIRQLeaveCritical(void){assert(locks);--locks;}
void salPCBeginCycle(u64 tick){assert(tick==cycles*5);}
static void control(void) {
    s16 *dest=salAiGetDest();
    for(unsigned i=0;i<160;i++){dest[i*2]=(s16)((cycles*160+i)*13);dest[i*2+1]=(s16)((cycles*160+i)*29);}
    ++cycles;
}
int main(void) {
    _Static_assert(SAL_PC_MAX_FRAMES==160,"Native cycle allocation");
    _Static_assert(SAL_PC_MAX_CHANNELS==2,"Native output channels");
    for(unsigned rate=8000;rate<=96000;rate+=1000)for(unsigned channels=1;channels<=8;channels++) {
        SND_PC_CONFIG config={rate,channels};
        assert(sndPCConfigure(&config)==(rate==32000 && channels==2));
    }
    SND_PC_CONFIG config={32000,2};assert(sndPCConfigure(&config));
    salPCResetOutput(control);sndActive=1;assert(!sndPCConfigure(&config));
    unsigned total=0;
    for(unsigned n=0;n<1000;n++) {
        unsigned frames=(n*337)%641+1;s16 *pcm=malloc(frames*4);
        assert(sndPCRender(pcm,frames));
        for(unsigned i=0;i<frames;i++){assert(pcm[i*2]==(s16)((total+i)*13));assert(pcm[i*2+1]==(s16)((total+i)*29));}
        total+=frames;free(pcm);
    }
    SND_PC_RENDER_INFO info=sndPCGetRenderInfo();
    assert(info.outputFrames==total && info.renderedFrames==cycles*160 && info.controlTicks==cycles*5 && !locks);
    printf("PASS %u stereo frames across partial native MusyX cycles\n",total);
}
