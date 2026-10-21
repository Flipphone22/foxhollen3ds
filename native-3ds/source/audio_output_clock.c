#include "fh_audio_output_clock.h"
#include <string.h>

int fh_audio_output_gate(FhAudioOutputClock *c,unsigned soc_clock,uint64_t queued_end,unsigned refill_frames) {
    if(!c || !c->valid || !soc_clock || !c->rate)return 1;
    uint64_t block=(UINT64_C(160)*c->rate*512+soc_clock-1)/soc_clock;
    uint64_t remaining=queued_end>=c->frame?queued_end-c->frame:0;
    if(!c->buffering && remaining<2*block){c->buffering=1;c->starvations++;}
    uint64_t refill=refill_frames>4*block?refill_frames:4*block;
    if(c->buffering && remaining>=refill)c->buffering=0;
    return !c->buffering;
}

FhAudioOutputWindow fh_audio_output_step(FhAudioOutputClock *c,uint32_t tick,
    uint32_t dropped,uint32_t epoch,unsigned rate,unsigned soc_clock,
    int start,uint64_t first,int running,uint64_t queued_end) {
    FhAudioOutputWindow out={0};
    if(!c || !epoch || !soc_clock || !rate || rate>192000)return out;
    if(c->observed && c->last_tick==tick)return out; /* no double publication */
    /* dropped counts missed output deadlines, not consumed input blocks.
     * Each observed SDK submission still advances input exactly once. Missing
     * callbacks remain invalid; AUX additionally validates firmware bank IDs. */
    unsigned consecutive=c->observed && tick-c->last_tick==1;
    if(start) {
        c->frame=first;c->fraction=0;c->epoch=epoch;c->rate=rate;c->valid=1;c->buffering=0;
    } else if(!consecutive || c->epoch!=epoch || c->rate!=rate)c->valid=0;
    c->observed=1;c->last_tick=tick;c->last_dropped=dropped;c->running=running!=0;
    if(!c->valid || !running)return out;
    /* The stored frame/fraction is the next block to submit. A pause never
     * advances it. A valid block advances exactly once at publication. */
    uint64_t numerator=(uint64_t)c->fraction+UINT64_C(160)*rate*512;
    uint64_t advance=numerator/soc_clock;
    if(advance>UINT64_MAX-c->frame || c->frame>queued_end){c->valid=0;return out;}
    out.first=c->frame;out.end=c->frame+advance;
    if(out.end>queued_end) {
        out.end=queued_end;c->frame=queued_end;c->fraction=0;c->valid=0;
    } else {
        c->frame=out.end;c->fraction=(uint32_t)(numerator%soc_clock);
    }
    out.available=out.end>out.first;
    return out;
}
