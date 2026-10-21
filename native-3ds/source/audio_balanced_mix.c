#include "fh_audio_balanced_mix.h"
#include <math.h>
#include <string.h>
int fh_audio_balanced_mix(float output[12],const float input[12]) {
    if(!output || !input)return 0;
    for(unsigned i=0;i<12;i++)if(!isfinite(input[i]))return 0;
    for(unsigned i=8;i<12;i++)if(input[i]!=0)return 0;
    float routed[12]={0};
    for(unsigned i=0;i<4;i++){routed[i+4]=input[i+4];routed[i+8]=input[i];}
    memcpy(output,routed,sizeof(routed));return 1;
}
