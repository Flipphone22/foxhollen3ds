/* Exercise the real source studio mixer around the new capture hook. */
#define main source_control_tests
#include "source-fixture.c"
#undef main
static unsigned capture_mode,capture_calls,legacy_calls,nonzero_samples,current_bus_mode;
static int32_t raw_expected[2][24][3][160];
static int16_t dry_expected[24][320];
static DSPvoice state_expected[24];
int fh_audio_game_capture_aux(unsigned studio,unsigned bus,uint64_t frame,unsigned frames,
    const int32_t *l,const int32_t *r,const int32_t *s) {
    assert(studio==0 && bus<2 && frame==cycle*160 && frames==160);
    assert(!bus || !current_bus_mode);
    const int32_t *base=(bus?dspStudio[0].auxB:dspStudio[0].auxA)[salAuxFrame];
    assert(l==base && r==base+SAL_SAMPLES_PER_FRAME && s==base+2*SAL_SAMPLES_PER_FRAME);
    const int32_t *planes[3]={l,r,s};
    for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++) {
        if(!capture_mode)raw_expected[bus][cycle][c][i]=planes[c][i];
        else assert(raw_expected[bus][cycle][c][i]==planes[c][i]);
        nonzero_samples+=planes[c][i]!=0;
    }
    capture_calls++;return capture_mode==2;
}
static void legacy_effect(u8 reason,SND_AUX_INFO *info,void *user) {
    assert(reason==SND_AUX_REASON_BUFFERUPDATE);unsigned bus=(unsigned)(uintptr_t)user;
    int32_t *p[3]={info->data.bufferUpdate.left,info->data.bufferUpdate.right,info->data.bufferUpdate.surround};
    for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++) {
        assert(p[c][i]==(cycle?raw_expected[bus][cycle-1][c][i]:0));
        p[c][i]=capture_mode?0:p[c][i]/2;
    }
    legacy_calls++;
}
int main(void) {
    unsigned char encoded[2048];for(unsigned i=0;i<sizeof(encoded);i++)encoded[i]=(unsigned char)(i*37);
    DSPADPCMplusInfo info={.initialPS=0x11,.loopPS=0x20,.loopY0=300,.loopY1=-700};
    for(current_bus_mode=0;current_bus_mode<2;current_bus_mode++)for(trial=0;trial<32;trial++)
    for(capture_mode=0;capture_mode<3;capture_mode++) {
        setup(0,encoded,&info);unsigned start_capture=capture_calls,start_legacy=legacy_calls;
        DSPstudioinfo *studio=&dspStudio[0];
        studio->type=current_bus_mode?SND_STUDIO_TYPE_DPL2:SND_STUDIO_TYPE_STD;
        studio->auxAHandler=studio->auxBHandler=legacy_effect;
        studio->auxAUser=(void*)0;studio->auxBUser=(void*)1;
        for(cycle=0;cycle<24;cycle++) {
            /* The source loop/callback keeps running after voices end: retain tails. */
            salFrame=cycle&1;salAuxFrame=cycle%3;salPCBeginCycle(cycle*5);
            int16_t output[320];salCtrlDsp(output);
            if(!capture_mode)state_expected[cycle]=voices[0];
            else assert(!memcmp(&state_expected[cycle],voices,sizeof(*voices)));
            if(capture_mode==1)memcpy(dry_expected[cycle],output,sizeof(output));
            if(capture_mode==2)assert(!memcmp(dry_expected[cycle],output,sizeof(output)));
        }
        assert(capture_calls-start_capture==24*(current_bus_mode?1u:2u));
        assert(legacy_calls-start_legacy==(capture_mode==2?0:24*(current_bus_mode?1u:2u)));
    }
    assert(nonzero_samples);
    printf("{\"passed\":true,\"captured_bus_cycles\":%u,\"legacy_calls\":%u,\"nonzero_observations\":%u}\n",
        capture_calls,legacy_calls,nonzero_samples);
}
