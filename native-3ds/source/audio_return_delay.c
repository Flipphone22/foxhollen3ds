#include "fh_audio_return_delay.h"
int fh_audio_return_delay(FhAudioReturnDelay *d,int32_t *l,int32_t *r,int32_t *s,unsigned frames) {
    if(!d || !l || !r || !s || !frames || frames>160 || d->cursor>=160)return 0;
    int32_t *planes[3]={l,r,s};
    for(unsigned i=0;i<frames;i++) {
        for(unsigned c=0;c<3;c++) {
            int32_t current=planes[c][i];
            planes[c][i]=d->samples[c][d->cursor];
            d->samples[c][d->cursor]=current;
        }
        if(++d->cursor==160)d->cursor=0;
    }
    return 1;
}
