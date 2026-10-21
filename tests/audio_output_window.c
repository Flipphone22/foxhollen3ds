#include "fh_audio_output_clock.h"
#include "fh_audio_timeline.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { SOC=16756991 };
int main(void) {
    unsigned checks=0;
    /* Reproduce the live game boundary: predictor reaches queued_end before
     * returned PCM completion; new PCM arrives before an underrun is reported.
     * Guarded publication holds both consumers, preserving phase and samples. */
    for(unsigned phase=0;phase<157;phase++) {
        FhAudioOutputClock c={0};
        uint64_t published=453760,consumed=453673-phase;
        (void)fh_audio_output_step(&c,100,0,1,32000,SOC,1,consumed,0,published);
        c.fraction=(phase*7919u)%SOC;
        unsigned fraction=c.fraction;
        assert(!fh_audio_output_gate(&c,SOC,published,640));
        FhAudioOutputWindow w=fh_audio_output_step(&c,101,0,1,32000,SOC,0,0,0,published);
        assert(!w.available && c.valid && c.frame==consumed && c.fraction==fraction);
        published+=640;
        assert(fh_audio_output_gate(&c,SOC,published,640));
        w=fh_audio_output_step(&c,102,0,1,32000,SOC,0,0,1,published);
        assert(w.available && w.first==consumed && c.valid && c.starvations==1);
        assert(c.fraction==(fraction+UINT64_C(160)*32000*512)%SOC);
        checks++;
    }
    /* Deterministic bursty producer. No duplicated/dropped input frames, no
     * metadata invalidation, bounded pause hysteresis, many refill boundaries. */
    FhAudioOutputClock burst={0};uint64_t produced=2560,consumed=0;unsigned seed=17;
    for(unsigned tick=0;tick<50000;tick++) {
        seed=1664525u*seed+1013904223u;
        if((seed>>24)<60)produced+=640;
        int running=tick==0 || fh_audio_output_gate(&burst,SOC,produced,2560);
        FhAudioOutputWindow w=fh_audio_output_step(&burst,tick,0,1,32000,SOC,tick==0,0,running,produced);
        if(w.available){assert(w.first==consumed && w.end<=produced);consumed=w.end;}
        assert(burst.valid && burst.frame==consumed);checks++;
    }
    assert(burst.starvations>100);
    FhAudioOutputClock wide={.valid=1,.rate=32000,.frame=1234,.fraction=971};
    assert(!fh_audio_output_gate(&wide,SOC,1400,2560));
    assert(!fh_audio_output_gate(&wide,SOC,1234+640,2560));
    assert(!fh_audio_output_gate(&wide,SOC,1234+2559,2560));
    assert(fh_audio_output_gate(&wide,SOC,1234+2560,2560));
    assert(wide.frame==1234 && wide.fraction==971 && wide.starvations==1);
    const unsigned rates[]={32000,48000,192000};
    for(unsigned rate_index=0;rate_index<3;rate_index++) {
        unsigned rate=rates[rate_index];FhAudioOutputClock c={0};uint64_t blocks=0;
        for(unsigned step=0;step<50000;step++) {
            int running=(step%41)<35;
            uint64_t first=blocks*UINT64_C(160)*rate*512/SOC;
            if(running)blocks++;
            uint64_t end=blocks*UINT64_C(160)*rate*512/SOC;
            uint32_t tick=UINT32_MAX-1000+step; /* wraps naturally */
            FhAudioOutputWindow w=fh_audio_output_step(&c,tick,0,9,rate,SOC,step==0,0,running,UINT64_MAX);
            assert(w.available==(unsigned)running);
            if(running)assert(w.first==first && w.end==end);
            assert(c.frame==end);
            FhAudioOutputClock prior=c;
            assert(!fh_audio_output_step(&c,tick,0,9,rate,SOC,0,0,running,UINT64_MAX).available);
            assert(!memcmp(&prior,&c,sizeof(c)));checks++;
        }
        assert(!fh_audio_output_step(&c,c.last_tick+2,0,9,rate,SOC,0,0,1,UINT64_MAX).available);
        assert(!fh_audio_output_step(&c,c.last_tick+1,0,9,rate,SOC,0,0,1,UINT64_MAX).available);
        FhAudioOutputWindow w=fh_audio_output_step(&c,c.last_tick+1,0,10,rate,SOC,1,1234,1,UINT64_MAX);
        assert(w.available && w.first==1234);
        assert(!fh_audio_output_step(&c,c.last_tick+1,1,10,rate,SOC,0,0,1,UINT64_MAX).available);
        w=fh_audio_output_step(&c,c.last_tick+1,1,11,rate,SOC,1,9000,1,9007);
        assert(w.available && w.first==9000 && w.end==9007 && !c.valid);
        assert(!fh_audio_output_step(&c,c.last_tick+1,1,11,rate,SOC,0,0,1,10000).available);
    }
    /* Two packets straddle silence, a real source lifetime, and a new lifetime.
     * Reverse slot order represents ring wrap. Every sample is checked against
     * independent absolute-interval membership, including unknown coverage. */
    FhAudioMap maps[2]={{0},{0}};
    assert(fh_audio_map_append(&maps[0],(FhAudioSpan){0},0,480));
    assert(fh_audio_map_append(&maps[0],(FhAudioSpan){.source_frame=0,.generation=7,.frames=160},480,160));
    assert(fh_audio_map_append(&maps[1],(FhAudioSpan){.source_frame=160,.generation=7,.frames=320},0,320));
    assert(fh_audio_map_append(&maps[1],(FhAudioSpan){.source_frame=5000,.generation=8,.frames=320},320,320));
    for(unsigned first=0;first<1120;first++) {
        unsigned end=first+157;FhAudioSourceWindow w={0};
        assert(fh_audio_source_window_append(&w,&maps[1],640,640,first,end));
        assert(fh_audio_source_window_append(&w,&maps[0],0,640,first,end));
        assert(fh_audio_source_window_finish(&w,first,end));
        for(unsigned at=0;at<157;at++) {
            unsigned absolute=first+at,gen=absolute<480?0:absolute<960?7:8;
            uint64_t frame=gen==7?absolute-480:gen==8?5000+absolute-960:0;
            unsigned matches=0;
            for(unsigned part=0;part<w.count;part++) {
                FhAudioSpan s=w.spans[part];
                if(at>=s.offset && at-s.offset<s.frames) {
                    matches++;assert(s.generation==gen);
                    if(gen)assert(s.source_frame+at-s.offset==frame);
                }
            }
            assert(matches==1);checks++;
        }
    }
    FhAudioSourceWindow w={0};
    assert(fh_audio_source_window_append(&w,&maps[0],0,640,600,700));
    assert(!fh_audio_source_window_finish(&w,600,700)); /* incomplete queued data */
    assert(!fh_audio_source_window_append(&w,&maps[0],0,640,600,700)); /* overlap */
    memset(&w,0,sizeof(w));
    assert(fh_audio_source_window_append(&w,NULL,0,640,20,40));
    assert(fh_audio_source_window_finish(&w,20,40) && w.count==1 && !w.spans[0].generation);
    assert(!fh_audio_source_window_append(&w,NULL,UINT64_MAX-10,640,20,40));
    printf("PASS %u output-clock and mapped-window checks\n",checks);
    return 0;
}
