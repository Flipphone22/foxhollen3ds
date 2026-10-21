#include "fh_audio_aux_output_delay.h"
#include <string.h>
int fh_audio_aux_output_delay(FhAudioAuxOutputDelay *delay,int32_t samples[4][160]) {
    if(!delay || !samples)return 0;
    for(unsigned c=0;c<4;c++) {
        int32_t last[2]={samples[c][158],samples[c][159]};
        memmove(samples[c]+2,samples[c],158*sizeof(int32_t));
        samples[c][0]=delay->samples[c][0];samples[c][1]=delay->samples[c][1];
        delay->samples[c][0]=last[0];delay->samples[c][1]=last[1];
    }
    return 1;
}
