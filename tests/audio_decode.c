#include "hw_pc_decode.h"
#include "fh_audio_span.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
extern s16 referenceDecode(u8,u8,const s16 [8][2],s16*,s16*);
extern bool referenceInit(MusyPCMReader*,const SAMPLE_INFO*);
extern s16 referenceRead(MusyPCMReader*,const SAMPLE_INFO*,u8);
static uint32_t random_state=0x534641;
static uint32_t next(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
int main(void){
    static const s16 limits[]={-32768,-32767,-2048,-1,0,1,2047,32766,32767};
    unsigned checks=0;uint32_t hash=2166136261u;
    for(unsigned config=0;config<10;config++){
        s16 coefficients[8][2];
        for(unsigned p=0;p<8;p++)for(unsigned c=0;c<2;c++)coefficients[p][c]=limits[(config+p+c)%9];
        const s16 (*table)[2]=config==9?NULL:coefficients;
        for(unsigned ps=0;ps<256;ps++)for(unsigned nibble=0;nibble<16;nibble++)
        for(unsigned h=0;h<9;h++)for(unsigned j=0;j<9;j++){
            s16 a=limits[h],b=limits[j],x=a,y=b;
            s16 expected=referenceDecode(nibble,ps,table,&a,&b);
            s16 actual=salPCDecodeNibble(nibble,ps,table,&x,&y);
            assert(actual==expected && a==x && b==y);checks++;hash=(hash^(uint16_t)actual)*16777619u;
        }
    }
    s16 coefficients[8][2],a=0,b=0,x=0,y=0;
    for(unsigned i=0;i<1000000;i++){
        if(!(i&255))for(unsigned p=0;p<8;p++)for(unsigned c=0;c<2;c++)coefficients[p][c]=(s16)next();
        u8 ps=next(),nibble=next();
        s16 expected=referenceDecode(nibble,ps,coefficients,&a,&b);
        s16 actual=salPCDecodeNibble(nibble,ps,coefficients,&x,&y);
        assert(actual==expected && a==x && b==y);checks++;hash=(hash^(uint16_t)actual)*16777619u;
    }
    printf("ADPCM exact samples=%u hash=%08x\n",checks,hash);
    /* Cursor transitions must remain identical when the reader is inlined into
     * the resampler: static/stream loops, changing ring data and end-of-sample. */
    unsigned reader_checks=0;unsigned char bytes[512];DSPADPCMplusInfo info={0};
    for(unsigned i=0;i<sizeof(bytes);i++)bytes[i]=next();
    for(unsigned i=0;i<8;i++)for(unsigned j=0;j<2;j++)info.coefTab[i][j]=(s16)next();
    info.initialPS=0x35;info.loopPS=0x26;info.loopY0=-135;info.loopY1=412;
    info.blk[0]=(DSPADPCMblock){-122,231,0x17,0};
    for(unsigned type=0;type<=6;type++)for(unsigned looping=0;looping<2;looping++)
    for(unsigned extra=0;extra<2;extra++){
        SAMPLE_INFO sample={.addr=bytes,.extraData=extra?&info:NULL,.length=113,.loop=looping?17:0,.loopLength=looping?89:0,.compType=type};
        MusyPCMReader a,b,c;FhPCMSpan span={0};assert(referenceInit(&a,&sample)==salPCInitReader(&b,&sample));c=b;
        for(unsigned n=0;n<1024;n++){
            if(n%37==0)bytes[next()%sizeof(bytes)]=next();
            u8 ps=next();s16 expected=referenceRead(&a,&sample,ps),actual=salPCReadSample(&b,&sample,ps);
            if(n%160==0)span=(FhPCMSpan){0};
            s16 spanned=fh_pcm_span_read(&span,&c,&sample,ps);
            assert(actual==expected && spanned==expected && !memcmp(&a,&b,sizeof(a)) && !memcmp(&a,&c,sizeof(a)));reader_checks++;
        }
    }
    printf("Exact reader samples and cursor states=%u\n",reader_checks);
    unsigned span_checks=0;
    struct { DSPADPCMplusInfo base;DSPADPCMblock more[2]; } extended={.base=info};
    _Static_assert(offsetof(typeof(extended),more)==sizeof(DSPADPCMplusInfo),"Contiguous ADPCM-plus fixture blocks");
    extended.more[0]=extended.more[1]=info.blk[0];
    for(unsigned type=0;type<6;type++)for(unsigned begin=0;begin<28;begin++)
    for(unsigned length=1;length<=28;length++)for(unsigned offset=0;offset<3;offset++) {
        SAMPLE_INFO sample={.addr=bytes,.extraData=&extended.base,.length=113,.loop=begin,.loopLength=length,.offset=offset,.compType=type};
        MusyPCMReader a,b;FhPCMSpan span={0};
        assert(referenceInit(&a,&sample)==salPCInitReader(&b,&sample));
        for(unsigned n=0;n<97;n++) {
            if(n%11==0){bytes[next()%sizeof(bytes)]=next();span=(FhPCMSpan){0};}
            u8 ps=next();s16 expected=referenceRead(&a,&sample,ps);
            s16 actual=fh_pcm_span_read(&span,&b,&sample,ps);
            assert(actual==expected && !memcmp(&a,&b,sizeof(a)));span_checks++;
        }
    }
    printf("Exact span boundary samples and cursor states=%u\n",span_checks);
}
