#include <stdio.h>
#include "fh_audio_buffer.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Failed %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    const unsigned fps[]={60,30,20,15,10};
    for(unsigned f=0;f<5;f++)for(unsigned channel=0;channel<2;channel++){
        FhAudioBuffer p;fh_audio_buffer_init(&p);unsigned samples=channel?504:640,rate=channel?48043:32000;
        uint64_t tick=100;unsigned minimum=~0u,underruns=0,queued=0;
        /* Deterministic steady service model; count real samples, no silence
         * insertion. It tests buffer scheduling, not physical mixer throughput. */
        for(unsigned i=0;i<600;i++){
            fh_audio_buffer_observe(&p,tick,samples,rate);
            CHECK(p.target>=8&&p.target<=16&&p.prefill<p.target);
            queued=p.target*samples;
            unsigned consumed=(rate+fps[f]-1)/fps[f];
            if(i>0){if(queued<consumed)underruns++;else if(queued-consumed<minimum)minimum=queued-consumed;}
            tick+=40500000/fps[f];
        }
        CHECK(!underruns);if(channel&&fps[f]==15)CHECK(p.target==11&&p.prefill==9);
        printf("%u FPS channel%u: target%u packets, %u ms capacity, minimum residual %u samples\n",fps[f],channel,p.target,p.target*samples*1000/rate,minimum);
        fh_audio_buffer_observe(&p,tick+40500000*5ULL,samples,rate);CHECK(p.target==16);
        fh_audio_buffer_init(&p);CHECK(p.target==8&&p.prefill==4&&!p.changes);
    }
    puts("PASS: steady 60/30/20/15/10 FPS sample accounting, bounded growth and reset");return 0;
}
