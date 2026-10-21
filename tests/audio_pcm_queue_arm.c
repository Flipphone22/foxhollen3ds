#include "fh_audio_pcm_queue.h"
#include <string.h>
#include <math.h>
extern unsigned char ndspChn[];
extern void ndspiUpdateChn(void),ndspiReadChnState(void);
void *ndspVars[16];u16 ndspBufferId,ndspFrameId;
static unsigned char shared[16][8192] __attribute__((aligned(32)));
static int16_t storage[2*8*2048*2] __attribute__((aligned(32)));
static int16_t silence[320] __attribute__((aligned(32)));
static int16_t source[4096];
static FhAudioPcmQueue queue;
static unsigned flush_fail,flushes;
volatile unsigned failure,checks,finished;
const volatile unsigned abi_sizes[]={sizeof(FhAudioPcmQueue),sizeof(FhAudioPcmChannel),sizeof(ndspWaveBuf)};
_Static_assert(sizeof(ndspWaveBuf)==24 && offsetof(ndspWaveBuf,sequence_id)==18,"SDK ABI");
void LightLock_Lock(LightLock *p){(void)p;}
void LightLock_Unlock(LightLock *p){(void)p;}
u32 osConvertVirtToPhys(const void *p){return (uintptr_t)p;}
__attribute__((noinline,noreturn)) void done(void){for(;;){}}
#define CHECK(x) do{++checks;if(!(x)){failure=__LINE__;done();}}while(0)
static int flush(void *p,const void *data,size_t bytes){
    (void)p;CHECK(data && bytes && bytes<=sizeof(source));flushes++;return !flush_fail;
}
static u16 sync_id(unsigned ch){u16 s;memcpy(&s,ndspChn+132*ch+8,2);return s;}
static void reply(unsigned ch,u16 sync,u16 sequence,u16 last,unsigned position){
    unsigned char *p=shared[4+(ndspBufferId&1)]+12*ch;u16 flags=sequence?0x101:0x100;
    position=(position<<16)|(position>>16); /* DSP shared-u32 halfword order. */
    memcpy(p,&flags,2);memcpy(p+2,&sync,2);memcpy(p+4,&position,4);
    memcpy(p+8,&sequence,2);memcpy(p+10,&last,2);ndspiReadChnState();
}
static FhAudioPcmCursor cursor(unsigned ch){return fh_audio_pcm_cursor(&queue,ch);}
static void submit(unsigned ch,unsigned frames){
    unsigned index=queue.channels[ch].write_index%8;
    CHECK(fh_audio_pcm_space(&queue,ch));CHECK(!fh_audio_pcm_queue(&queue,ch,source,frames,32000));
    CHECK(!memcmp(source,storage+(ch*8+index)*2048*2,frames*4));
}
static void reset(unsigned ch){
    FhAudioPcmChannel *p=&queue.channels[ch];unsigned prior=p->requested_epoch;
    CHECK(!fh_audio_pcm_reset(&queue,ch));CHECK(!fh_audio_pcm_space(&queue,ch));
    CHECK(!fh_audio_pcm_buffered(&queue,ch));
    CHECK(fh_audio_pcm_queue(&queue,ch,source,160,32000)==FH_AUDIO_POOL_BUSY);
    fh_audio_pcm_service(&queue);CHECK(!cursor(ch).available && cursor(ch).epoch==prior+1);
    CHECK(!fh_audio_pcm_space(&queue,ch));ndspiUpdateChn();
    CHECK(p->sentinel.status==NDSP_WBUF_PLAYING);
    fh_audio_pcm_service(&queue);CHECK(!fh_audio_pcm_space(&queue,ch));
    /* SDK-local PLAYING and same sequence from the former sync are not fences. */
    reply(ch,sync_id(ch)-1,p->sentinel.sequence_id,0,0);
    fh_audio_pcm_service(&queue);CHECK(!fh_audio_pcm_space(&queue,ch));
    reply(ch,sync_id(ch),p->sentinel.sequence_id,0,0);
    fh_audio_pcm_service(&queue);CHECK(fh_audio_pcm_space(&queue,ch));
    CHECK(!cursor(ch).available && p->acknowledged_epoch==prior+1);
    CHECK(!fh_audio_pcm_buffered(&queue,ch));
    float mix[12];ndspChnGetMix(ch,mix);CHECK(mix[0]==1 && mix[1]==1);
}
void run(void){
    CHECK(abi_sizes[2]==24);
    for(unsigned i=0;i<16;i++)ndspVars[i]=shared[i];
    for(unsigned i=0;i<4096;i++)source[i]=(int16_t)(i*71);
    CHECK(fh_audio_pcm_init(&queue,storage,sizeof(storage),storage,sizeof(storage),flush,NULL)==FH_AUDIO_POOL_INVALID);
    flush_fail=1;CHECK(fh_audio_pcm_init(&queue,storage,sizeof(storage),silence,sizeof(silence),flush,NULL)==FH_AUDIO_POOL_FLUSH_FAILED);
    flush_fail=0;CHECK(!fh_audio_pcm_init(&queue,storage,sizeof(storage),silence,sizeof(silence),flush,NULL));
    CHECK(fh_audio_pcm_init(&queue,storage,sizeof(storage),silence,sizeof(silence),flush,NULL)==FH_AUDIO_POOL_BUSY);
    CHECK(!fh_audio_pcm_space(&queue,2));fh_audio_pcm_service(&queue);
    CHECK(ndspChnIsPaused(0) && ndspChnIsPaused(1));
    for(unsigned round=0;round<10;round++)for(unsigned ch=0;ch<2;ch++){
        ndspBufferId=ndspFrameId=round&1;FhAudioPcmChannel *p=&queue.channels[ch];
        unsigned epoch=p->requested_epoch;
        for(unsigned i=0;i<3;i++){
            submit(ch,160+i*32);fh_audio_pcm_service(&queue);ndspiUpdateChn();
            CHECK(!cursor(ch).available && ndspChnIsPaused(ch));
        }
        submit(ch,256);fh_audio_pcm_service(&queue);
        CHECK(cursor(ch).available && cursor(ch).frame==0 && cursor(ch).epoch==epoch);
        CHECK(!ndspChnIsPaused(ch) && cursor(ch).rate==32000);
        CHECK(fh_audio_pcm_buffered(&queue,ch)==832);
        FhAudioPcmStatus status=fh_audio_pcm_status(&queue,ch);
        CHECK(status.frames==832 && status.buffers==4 && status.rate==32000 && !status.paused && !status.faults);
        CHECK(fh_audio_pcm_queue(&queue,ch,source,160,48000)==FH_AUDIO_POOL_INVALID);
        ndspiUpdateChn();
        unsigned base=0;u16 last=0;
        for(unsigned i=0;i<4;i++){
            FhAudioPcmSlot *s=&p->slots[i];unsigned pos=s->frames/2;
            reply(ch,sync_id(ch),s->wave.sequence_id,last,pos);fh_audio_pcm_service(&queue);
            CHECK(cursor(ch).available && cursor(ch).frame==base+pos);
            CHECK(!fh_audio_pcm_pause(&queue,ch,1));fh_audio_pcm_service(&queue);
            CHECK(!cursor(ch).available && ndspChnIsPaused(ch) && !fh_audio_pcm_space(&queue,ch));
            CHECK(!fh_audio_pcm_pause(&queue,ch,0));fh_audio_pcm_service(&queue);
            CHECK(cursor(ch).available && !ndspChnIsPaused(ch));
            base+=s->frames;last=s->wave.sequence_id;ndspiUpdateChn();
        }
        reply(ch,sync_id(ch),0,last,0);fh_audio_pcm_service(&queue);
        CHECK(!cursor(ch).available && !fh_audio_pcm_buffered(&queue,ch) && ndspChnIsPaused(ch));
        submit(ch,128);fh_audio_pcm_service(&queue);CHECK(!cursor(ch).available);
        CHECK(!fh_audio_pcm_finish(&queue,ch));fh_audio_pcm_service(&queue);
        CHECK(cursor(ch).available && cursor(ch).frame==832);
        /* Reset while active, repeat with identical descriptor sequence IDs. */
        reset(ch);
    }
    FhAudioPcmChannel *p=&queue.channels[0];
    flush_fail=1;unsigned old=p->write_index;uint64_t first=p->next_frame;
    CHECK(fh_audio_pcm_queue(&queue,0,source,160,32000)==FH_AUDIO_POOL_FLUSH_FAILED);
    CHECK(p->write_index==old && p->next_frame==first && fh_audio_pcm_space(&queue,0));flush_fail=0;
    for(unsigned i=0;i<8;i++)submit(0,160);
    CHECK(!fh_audio_pcm_space(&queue,0));CHECK(fh_audio_pcm_queue(&queue,0,source,160,32000)==FH_AUDIO_POOL_BUSY);
    fh_audio_pcm_service(&queue);ndspiUpdateChn();CHECK(cursor(0).available);
    reply(0,sync_id(0),p->slots[0].wave.sequence_id,0,161);fh_audio_pcm_service(&queue);
    CHECK(!cursor(0).available && p->faults==1);
    CHECK(fh_audio_pcm_status(&queue,0).faults==1);
    /* Several reset requests while silence is in flight acknowledge only the
     * acquired epoch, then the producer starts from its new origin. */
    CHECK(!fh_audio_pcm_reset(&queue,0));fh_audio_pcm_service(&queue);ndspiUpdateChn();
    CHECK(!fh_audio_pcm_reset(&queue,0));CHECK(!fh_audio_pcm_reset(&queue,0));
    reply(0,sync_id(0),p->sentinel.sequence_id,0,0);fh_audio_pcm_service(&queue);
    CHECK(fh_audio_pcm_space(&queue,0) && p->acknowledged_epoch==p->requested_epoch);
    submit(0,160);CHECK(p->slots[0].first_frame==0);
    CHECK(!fh_audio_pcm_finish(&queue,0));CHECK(!fh_audio_pcm_pause(&queue,0,1));fh_audio_pcm_service(&queue);
    CHECK(!cursor(0).available && ndspChnIsPaused(0));
    CHECK(!fh_audio_pcm_pause(&queue,0,0));fh_audio_pcm_service(&queue);
    CHECK(cursor(0).available && cursor(0).frame==0);
    finished=1;done();
}
