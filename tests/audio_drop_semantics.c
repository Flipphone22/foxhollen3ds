/* Local firmware fixture. No game, save, or console deployment. */
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
u32 __ctru_heap_size=4*1024*1024,__ctru_linear_heap_size=2*1024*1024;
static ndspWaveBuf wave;
static unsigned rows[160][4];
static volatile unsigned count;
static void callback(void *unused) {
    (void)unused;
    unsigned n=count;
    if(n>=160)return;
    rows[n][0]=ndspGetFrameCount();rows[n][1]=ndspGetDroppedFrames();
    rows[n][2]=ndspChnGetSamplePos(0);rows[n][3]=ndspChnGetWaveBufSeq(0);
    if(n==0)ndspChnWaveBufAdd(0,&wave);
    /* Deliberately delay one publication, not the game or its simulation. */
    if(n==40)svcSleepThread(80000000);
    __atomic_store_n(&count,n+1,__ATOMIC_RELEASE);
}
int main(void) {
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/fh-audio-drop",0777);
    gfxInitDefault();
    s16 *data=linearAlloc(64000);if(!data)return 2;
    for(unsigned i=0;i<32000;i++)data[i]=(i%32<16)?1000:-1000;
    if(R_FAILED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)data,64000)))return 3;
    if(R_FAILED(ndspInit()))return 4;
    ndspChnReset(0);ndspChnSetFormat(0,NDSP_FORMAT_MONO_PCM16);
    ndspChnSetRate(0,32000);ndspChnSetInterp(0,NDSP_INTERP_LINEAR);
    float mix[12]={1,1};ndspChnSetMix(0,mix);
    wave.data_pcm16=data;wave.nsamples=32000;wave.looping=true;
    ndspSetCallback(callback,NULL);
    for(unsigned i=0;i<500 && __atomic_load_n(&count,__ATOMIC_ACQUIRE)<160;i++)svcSleepThread(10000000);
    ndspExit();
    FILE *f=fopen("sdmc:/3ds/fh-audio-drop/trace.csv","wb");if(!f)return 5;
    fprintf(f,"callback,dropped,position,sequence\n");
    for(unsigned i=0;i<count;i++)fprintf(f,"%u,%u,%u,%u\n",rows[i][0],rows[i][1],rows[i][2],rows[i][3]);
    fclose(f);linearFree(data);gfxExit();return count==160?0:6;
}
