#include "fh_audio_dsp_retire.h"
#include <string.h>
/* Only the test object globalizes this private SDK symbol. Layout is guarded
 * by the exact installed object SHA and wave-buffer ABI in the runner. */
extern unsigned char ndspChn[];
extern void ndspiUpdateChn(void),ndspiReadChnState(void);
void *ndspVars[16];u16 ndspBufferId,ndspFrameId;
static unsigned char shared[16][8192] __attribute__((aligned(32)));
static unsigned char arena[4096] __attribute__((aligned(32)));
static int16_t silence[160] __attribute__((aligned(32)));
static unsigned char encoded[16]={0x11,0x23,0x65,0x43,0x12};
static FhAudioSamplePool pool;
static FhAudioDspRetirement retirement;
static ndspWaveBuf old_wave;
volatile unsigned failure,checks,finished;
const volatile unsigned abi_sizes[]={sizeof(FhAudioDspRetirement),sizeof(FhAudioDspRetiringVoice),sizeof(ndspWaveBuf)};
_Static_assert(sizeof(ndspWaveBuf)==24 && offsetof(ndspWaveBuf,status)==17 && offsetof(ndspWaveBuf,sequence_id)==18,"Installed SDK wave ABI");
static unsigned deny_flush;
void LightLock_Lock(LightLock *p){(void)p;}
void LightLock_Unlock(LightLock *p){(void)p;}
u32 osConvertVirtToPhys(const void *p){return (uintptr_t)p;}
static int flush(void *p,const void *data,size_t n){(void)p;(void)data;(void)n;return !deny_flush;}
__attribute__((noinline,noreturn)) void done(void){for(;;){}}
#define CHECK(x) do {++checks;if(!(x)){failure=__LINE__;done();}}while(0)
static u16 local_sync(unsigned ch){u16 s;memcpy(&s,ndspChn+132*ch+8,2);return s;}
static int pump(FhAudioDspRetirement *r,uint32_t *completed) {
    fh_audio_dsp_retire_service(r);return fh_audio_dsp_retire_drain(r,completed);
}
static void reply(unsigned ch,u16 sync,u16 sequence,u16 flags) {
    unsigned char *p=shared[4+(ndspBufferId&1)]+12*ch;
    memcpy(p,&flags,2);memcpy(p+2,&sync,2);memcpy(p+8,&sequence,2);
    ndspiReadChnState();
}
void run(void) {
    CHECK(abi_sizes[2]==24);
    for(unsigned i=0;i<16;i++)ndspVars[i]=shared[i];
    CHECK(fh_audio_pool_init(&pool,arena,sizeof(arena))==0);
    CHECK(fh_audio_dsp_retire_init(&retirement,&pool,(int16_t *)arena,sizeof(arena),flush,NULL)==FH_AUDIO_POOL_INVALID);
    CHECK(fh_audio_dsp_retire_init((FhAudioDspRetirement *)arena,&pool,silence,sizeof(silence),flush,NULL)==FH_AUDIO_POOL_INVALID);
    deny_flush=1;CHECK(fh_audio_dsp_retire_init(&retirement,&pool,silence,sizeof(silence),flush,NULL)==FH_AUDIO_POOL_FLUSH_FAILED);
    CHECK(!retirement.initialized);deny_flush=0;
    CHECK(fh_audio_dsp_retire_init(&retirement,&pool,silence,sizeof(silence),flush,NULL)==0);
    CHECK(fh_audio_dsp_retire_init(&retirement,&pool,silence,sizeof(silence),flush,NULL)==FH_AUDIO_POOL_BUSY);
    for(unsigned cycle=0;cycle<3;cycle++)for(unsigned ch=2;ch<24;ch++) {
        ndspBufferId=ndspFrameId=cycle&1;
        FhAudioSampleInput input={.encoded=encoded,.encoded_bytes=sizeof(encoded),.length=28,
            .loop_start=7,.loop_length=21,.initial_ps=0x10,.loop_ps=0x23,.loop_y1=173,.loop_y2=-409};
        FhAudioPoolToken bank,e,lease;
        CHECK(fh_audio_pool_bank_open(&pool,&bank)==0);
        CHECK(fh_audio_pool_prepare(&pool,bank,9,&input,&e)==0);
        CHECK(fh_audio_pool_publish(&pool,e,flush,NULL)==0);
        CHECK(fh_audio_pool_acquire(&pool,bank,9,&lease)==0);
        const void *samples;const FhAudioDspSamplePlan *plan;
        CHECK(fh_audio_pool_view(&pool,lease,&samples,&plan)==0);
        ndspChnReset(ch);
        if(cycle==1){u16 wrap=65535;memcpy(ndspChn+132*ch+8,&wrap,2);}
        ndspChnSetFormat(ch,NDSP_FORMAT_MONO_PCM16);ndspChnSetInterp(ch,NDSP_INTERP_NONE);
        ndspChnSetRate(ch,32000.f);ndspChnSetPaused(ch,false);
        memset(&old_wave,0,sizeof(old_wave));old_wave.data_vaddr=samples;old_wave.nsamples=plan->first_samples;
        ndspChnWaveBufAdd(ch,&old_wave);ndspiUpdateChn();
        u16 before=local_sync(ch);reply(ch,before,old_wave.sequence_id,0x101);
        CHECK(old_wave.status==NDSP_WBUF_PLAYING);
        CHECK(fh_audio_dsp_retire_request(&retirement,0,lease)==FH_AUDIO_POOL_INVALID);
        CHECK(fh_audio_dsp_retire_request(&retirement,ch,lease)==0);
        CHECK(old_wave.status==NDSP_WBUF_PLAYING);
        fh_audio_dsp_retire_service(&retirement);
        CHECK(old_wave.status==NDSP_WBUF_DONE && !ndspChnIsPlaying(ch));
        CHECK(fh_audio_dsp_retire_request(&retirement,ch,lease)==FH_AUDIO_POOL_BUSY);
        CHECK(fh_audio_dsp_retire_request(&retirement,ch==23?2:ch+1,lease)==FH_AUDIO_POOL_INVALID);
        CHECK(fh_audio_pool_bank_close(&pool,bank)==0);
        uint32_t completed=~0u;
        CHECK(pump(&retirement,&completed)==0 && !completed);
        CHECK(fh_audio_pool_view(&pool,lease,&samples,&plan)==0 && pool.used>0);
        ndspiUpdateChn();
        FhAudioDspRetiringVoice *v=&retirement.voices[ch-2];
        /* SDK UpdateChn marks PLAYING BEFORE the DSP has acknowledged it. */
        CHECK(v->sentinel.status==NDSP_WBUF_PLAYING && v->sentinel.sequence_id);
        CHECK(ndspChnGetWaveBufSeq(ch)==0);
        CHECK(pump(&retirement,&completed)==0 && !completed);
        CHECK(local_sync(ch)==(u16)(before+1));
        /* Old reply even with the NEW sequence ID must not release the lease. */
        reply(ch,before,v->sentinel.sequence_id,0x101);
        CHECK(v->sentinel.status==NDSP_WBUF_PLAYING && ndspChnGetWaveBufSeq(ch)==0);
        CHECK(pump(&retirement,&completed)==0 && !completed);
        CHECK(fh_audio_pool_view(&pool,lease,&samples,&plan)==0);
        reply(ch,local_sync(ch),0,1);
        CHECK(pump(&retirement,&completed)==0 && !completed);
        reply(ch,local_sync(ch),v->sentinel.sequence_id,0x101);
        CHECK(v->sentinel.status==NDSP_WBUF_PLAYING);
        fh_audio_dsp_retire_service(&retirement);
        CHECK(fh_audio_pool_view(&pool,lease,&samples,&plan)==0);
        CHECK(pump(&retirement,&completed)==0 && completed==(1u<<ch));
        CHECK(fh_audio_pool_view(&pool,lease,&samples,&plan)==FH_AUDIO_POOL_INVALID);
        CHECK(!pool.used && !fh_audio_dsp_retire_pending(&retirement));
        CHECK(pump(&retirement,&completed)==0 && !completed);
    }
    CHECK(retirement.requests==66 && retirement.acknowledged==66 && !retirement.faults);
    /* One shared resource with 22 outstanding channel cancellations. Late
     * responses must retire only their own lease; the last response frees it. */
    FhAudioSampleInput in={.encoded=encoded,.encoded_bytes=sizeof(encoded),.length=28,.initial_ps=0x10};
    FhAudioPoolToken bank,e,leases[22];
    CHECK(fh_audio_pool_bank_open(&pool,&bank)==0);
    CHECK(fh_audio_pool_prepare(&pool,bank,3,&in,&e)==0);
    CHECK(fh_audio_pool_publish(&pool,e,flush,NULL)==0);
    for(unsigned ch=2;ch<24;ch++) {
        CHECK(fh_audio_pool_acquire(&pool,bank,3,&leases[ch-2])==0);
        CHECK(fh_audio_dsp_retire_request(&retirement,ch,leases[ch-2])==0);
    }
    CHECK(fh_audio_pool_bank_close(&pool,bank)==0);
    CHECK(fh_audio_dsp_retire_pending(&retirement)==0xfffffcu);
    fh_audio_dsp_retire_service(&retirement);ndspiUpdateChn();uint32_t completed;
    CHECK(pump(&retirement,&completed)==0 && !completed);
    for(unsigned i=0;i<22;i++) {
        unsigned ch=23-i;FhAudioDspRetiringVoice *v=&retirement.voices[ch-2];
        CHECK(pool.used>0);
        reply(ch,(u16)(local_sync(ch)-1),v->sentinel.sequence_id,0x101);
        CHECK(pump(&retirement,&completed)==0 && !completed);
        reply(ch,local_sync(ch),v->sentinel.sequence_id,0x101);
        CHECK(pump(&retirement,&completed)==0 && completed==(1u<<ch));
    }
    CHECK(!pool.used && retirement.acknowledged==88 && !fh_audio_dsp_retire_pending(&retirement));
    CHECK(fh_audio_pool_shutdown(&pool)==0);finished=1;done();
}
