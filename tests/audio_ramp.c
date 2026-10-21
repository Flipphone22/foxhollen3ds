#include "fh_audio_ramp.h"
#include "fh_audio_itd.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned seed=74561;
static unsigned random_value(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void) {
    int32_t src[320],actual[320],expected[320],shared[320],envelope[320];unsigned checks=0;
    const int16_t endpoints[]={-32768,-32767,-1,0,1,32766,32767};
    for(unsigned a=0;a<7;a++)for(unsigned b=0;b<7;b++)for(unsigned f=0;f<65536;f++){
        int32_t expected=endpoints[a]+(int32_t)((int64_t)(endpoints[b]-endpoints[a])*f/65536);
        assert(fh_audio_lerp16(endpoints[a],endpoints[b],f)==expected);checks++;
    }
    for(unsigned n=0;n<100000;n++) {
        int samples=random_value()%160+1,cycle=samples+random_value()%161,offset=random_value()%(cycle-samples+1);
        int16_t start=random_value(),end=random_value();uint16_t es=random_value()%32769,ee=random_value()%32769;
        if(n%10==0)end=start;if(n%11==0)ee=es;if(n%17==0)start=end=0;
        for(int i=0;i<samples;i++){src[i]=(int16_t)random_value();shared[i]=actual[i]=expected[i]=(int16_t)random_value();}
        int32_t last=0;
        if(start || end)for(int i=0;i<samples;i++) {
            int32_t env=es+((int32_t)ee-es)*i/samples,vol=start+((int32_t)end-start)*(offset+i)/cycle;
            last=(src[i]*((vol*env)>>15))>>15;expected[i]+=last;
        }
        assert(fh_mix_ramp(actual,src,samples,start,end,es,ee,offset,cycle)==last);
        fh_audio_envelope(envelope,samples,es,ee);
        assert(fh_mix_envelope(shared,src,samples,start,end,envelope,offset,cycle)==last);
        for(int i=0;i<samples;i++){assert(actual[i]==expected[i] && shared[i]==expected[i]);checks+=2;}
    }
    for(unsigned n=0;n<10000;n++) {
        int16_t history[128],original[128];int32_t left[320],right[320];uint32_t cursor=random_value(),index=cursor;
        unsigned samples=random_value()%160+1,cycle=samples+random_value()%161,offset=random_value()%(cycle-samples+1);
        uint32_t delay[2]={random_value()%(32*65536),random_value()%(32*65536)},target[2]={random_value()%(32*65536),random_value()%(32*65536)};
        if(n%3==0)memset(delay,0,sizeof(delay)),memset(target,0,sizeof(target));
        if(n%5==0)for(unsigned c=0;c<2;c++)delay[c]=target[c]=(random_value()%33)*65536;
        if(n%7==0)memcpy(target,delay,sizeof(delay)); /* also steady fractional delays */
        if(n%4==0){unsigned ear=(n/4)&1;delay[ear]=target[ear]=(random_value()%33)*65536;}
        for(unsigned i=0;i<128;i++)history[i]=original[i]=(int16_t)random_value();
        for(unsigned i=0;i<samples;i++)src[i]=(int16_t)random_value();
        fh_audio_itd(left,right,src,history,&cursor,delay,target,samples,offset,cycle);
        for(unsigned i=0;i<samples;i++) {
            original[index&127]=src[i];
            for(unsigned c=0;c<2;c++) {
                int64_t d=delay[c]+((int64_t)target[c]-delay[c])*(offset+i)/cycle;
                unsigned pos=index-(unsigned)(d>>16);int32_t newer=original[pos&127],older=original[(pos-1)&127];
                int32_t expected=newer+(int32_t)((int64_t)(older-newer)*(d&65535)/65536);
                assert((c?right:left)[i]==expected);checks++;
            }
            ++index;
        }
        assert(cursor==index && !memcmp(history,original,sizeof(history)));
    }
    /* Successive zero/nonzero delay segments must preserve future ring reads. */
    {
        int16_t history[128]={0},reference_history[128]={0};
        uint32_t cursor=UINT32_MAX-37,reference_cursor=cursor,delay[2]={0},target[2]={0};
        for(unsigned n=0;n<20000;n++) {
            unsigned samples=random_value()%320,cycle=samples+17,offset=7;
            int32_t left[320],right[320],ref_left[320],ref_right[320];const int32_t *ears[2];
            for(unsigned i=0;i<samples;i++)src[i]=(int16_t)random_value();
            for(unsigned c=0;c<2;c++)target[c]=n%5<2?0:random_value()%(32*65536+1);
            fh_audio_itd_prepare(ears,left,right,src,history,&cursor,delay,target,samples,offset,cycle);
            fh_audio_itd(ref_left,ref_right,src,reference_history,&reference_cursor,delay,target,samples,offset,cycle);
            for(unsigned i=0;i<samples;i++){assert(ears[0][i]==ref_left[i] && ears[1][i]==ref_right[i]);checks+=2;}
            assert(cursor==reference_cursor && !memcmp(history,reference_history,sizeof(history)));
            memcpy(delay,target,sizeof(delay));
        }
    }
    for(unsigned n=0;n<10000;n++) {
        int32_t ring[1024]={0},reference[1024]={0};unsigned cursor=random_value(),frames=random_value()%512+1;
        int32_t last=(int32_t)random_value();if(n%5==0)last=0;if(n%7==0)last=INT32_MIN;
        fh_depop_tail(ring,1023,cursor,frames,last);
        for(unsigned i=0;i<frames;i++)reference[(cursor+i)&1023]=(int32_t)((int64_t)last*(frames-i-1)/frames);
        assert(!memcmp(ring,reference,sizeof(ring)));checks+=1024;
    }
    printf("PASS %u exact signed ramp, interaural delay and stop-tail samples\n",checks);
}
