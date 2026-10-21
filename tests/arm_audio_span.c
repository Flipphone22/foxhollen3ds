#include "fh_audio_span.h"
#include <stdint.h>
#include <string.h>
volatile unsigned test_checks,test_failure;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned line){test_failure=line;for(;;)__asm__ volatile("nop");}
#define CHECK(v) do{if(!(v))test_fail(__LINE__);++test_checks;}while(0)
/* This fixture enters only the bounded span. General cursor/loop behavior
 * remains covered by the existing full reader and resampler comparisons. */
s16 salPCReadSample(MusyPCMReader *r,const SAMPLE_INFO *s,u8 ps){(void)r;(void)s;(void)ps;test_fail(__LINE__);}
void test_main(void){
    const s16 limits[]={-32768,-1,0,32767};
    unsigned char block[8]={0};SAMPLE_INFO sample={0};unsigned overflows=0;
    for(unsigned a=0;a<4;a++)for(unsigned b=0;b<4;b++)
    for(unsigned c=0;c<4;c++)for(unsigned d=0;d<4;d++)
    for(unsigned exponent=0;exponent<16;exponent++)for(unsigned nibble=0;nibble<16;nibble++){
        block[1]=nibble<<4;
        FhPCMSpan span={.remaining=1,.block=block,.predictor={limits[a],limits[b]},.scale=(1u<<exponent)*2048};
        MusyPCMReader reader={.yn1=limits[c],.yn2=limits[d],.position=17};
        int value=nibble<8?(int)nibble:(int)nibble-16;
        int64_t expected=(int64_t)value*((1u<<exponent)*2048)+1024;
        expected+=(int64_t)limits[a]*limits[c]+(int64_t)limits[b]*limits[d];
        overflows+=expected>INT32_MAX || expected<INT32_MIN;
        expected>>=11;if(expected<-32768)expected=-32768;if(expected>32767)expected=32767;
        s16 actual=fh_pcm_span_read(&span,&reader,&sample,0);
        CHECK(actual==expected);CHECK(reader.yn1==expected && reader.yn2==limits[c]);
        CHECK(reader.position==18 && span.remaining==0 && span.within==1);
    }
    CHECK(overflows>0);test_pass();
}
