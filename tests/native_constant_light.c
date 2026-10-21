#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fh_native_constant_light.h"
static unsigned seed=197;
static float random_value(void){seed=1664525*seed+1013904223;return (seed>>8)*(1.f/16777216);}
static float clamp(float a){return fmaxf(0,fminf(1,a));}
int main(void){
    unsigned checked=0,rejected=0;
    for(unsigned trial=0;trial<4096;trial++){
        FhNativeLighting p={0};p.lights=trial%9;
        p.uniform[18][0]=(trial>>4)&1;p.uniform[18][1]=1;
        p.uniform[18][2]=(trial>>5)&1;p.uniform[18][3]=1;
        p.booleans=((trial>>6)&15)<<9;
        for(unsigned c=0;c<4;c++){
            p.uniform[16][c]=random_value();p.uniform[17][c]=random_value();
            for(unsigned i=0;i<p.lights;i++)p.uniform[8+i][c]=
                p.uniform[18][c==3?2:0]?random_value()*.3f:0;
        }
        float fill[4];assert(fh_native_constant_light(&p,fill));
        for(unsigned sample=0;sample<7;sample++)for(unsigned c=0;c<4;c++){
            float v=random_value();unsigned ambient_bit=c==3?10:9,material_bit=c==3?12:11;
            float ambient=p.booleans&(1u<<ambient_bit)?v:p.uniform[16][c];
            float material=p.booleans&(1u<<material_bit)?v:p.uniform[17][c];
            /* Independent scalar original: each light evaluates its normal
             * response even if its color is zero. All seven responses differ. */
            float old=ambient;
            for(unsigned i=0;i<p.lights;i++){
                float normal_dot=2*random_value()-1;
                float response=p.uniform[18][c==3?2:0]?1:fmaxf(normal_dot,0);
                old+=p.uniform[8+i][c]*response;
            }
            float reference=clamp(old)*material,lowered=clamp(ambient+fill[c])*material;
            assert(fabsf(reference-lowered)<=5e-7f);checked++;
        }
        FhNativeLighting bad=p;bad.complex=1;assert(!fh_native_constant_light(&bad,fill));rejected++;
        bad=p;bad.lights=1;bad.uniform[8][0]=.5f;bad.uniform[18][0]=0;
        assert(!fh_native_constant_light(&bad,fill));rejected++;
        bad=p;bad.lights=1;bad.uniform[8][3]=.5f;bad.uniform[18][2]=0;
        assert(!fh_native_constant_light(&bad,fill));rejected++;
    }
    FhNativeLighting invalid={.lights=9};float fill[4]={123,123,123,123};
    assert(!fh_native_constant_light(&invalid,fill) && fill[0]==123);
    invalid.lights=1;invalid.uniform[8][0]=NAN;assert(!fh_native_constant_light(&invalid,fill));
    printf("PASS: %u channel/sample comparisons, %u directional/attenuation rejections; no zero-light normal dependency\n",checked,rejected);
}
