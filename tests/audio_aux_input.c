#include "fh_audio_aux_input.h"
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t value(uint64_t frame,unsigned channel) {
    return (int32_t)((frame*17+channel*7919)%100003)-50001;
}
static void fill(int32_t p[3][160],uint64_t first) {
    for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)p[c][i]=value(first+i,c);
}
static void consume(FhAudioAuxInput *q,uint64_t first,unsigned count) {
    int32_t p[3][160];
    for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)p[c][i]=123;
    assert(fh_audio_aux_input_add(q,7,9,first,count,p[0],p[1],p[2])==FH_AUX_INPUT_OK);
    for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)
        assert(p[c][i]==123+((c<q->channels && i<count)?value(first+i,c):0));
}
static void cases(unsigned channels) {
    FhAudioAuxInput q={0};size_t bytes=fh_audio_aux_input_bytes(channels);
    int32_t *storage=malloc(bytes);assert(storage);
    assert(!fh_audio_aux_input_init(&q,storage,bytes-1,channels));
    assert(fh_audio_aux_input_init(&q,storage,bytes,channels));
    assert(!fh_audio_aux_input_init(&q,storage,bytes,channels));
    q.head=q.tail=UINT32_MAX-31; /* Cross serial wrap with a full queue. */
    assert(fh_audio_aux_input_ready(&q,7,9,4000,1)==FH_AUX_INPUT_BUSY);
    int32_t p[3][160];
    for(unsigned b=0;b<64;b++) {
        fill(p,4000+b*160);
        assert(fh_audio_aux_input_push(&q,7,9,4000+b*160,p[0],p[1],p[2])==FH_AUX_INPUT_OK);
        memset(p,0,sizeof(p)); /* Source buffer is immediately reusable. */
    }
    FhAudioAuxInput before=q;
    assert(fh_audio_aux_input_push(&q,7,9,14240,p[0],p[1],p[2])==FH_AUX_INPUT_BUSY);
    assert(!memcmp(&q,&before,sizeof(q)));
    assert(fh_audio_aux_input_ready(&q,7,9,14100,160)==FH_AUX_INPUT_BUSY);
    assert(fh_audio_aux_input_ready(&q,7,9,4150,160)==FH_AUX_INPUT_OK);
    assert(fh_audio_aux_input_add(&q,7,9,4150,160,p[0],p[1],p[2])==FH_AUX_INPUT_INVALID);
    assert(fh_audio_aux_input_ready(&q,8,9,4000,1)==FH_AUX_INPUT_INVALID);
    assert(fh_audio_aux_input_ready(&q,7,8,4000,1)==FH_AUX_INPUT_INVALID);
    assert(fh_audio_aux_input_push(&q,7,9,14241,p[0],p[1],p[2])==FH_AUX_INPUT_INVALID);
    assert(fh_audio_aux_input_push(&q,6,9,14240,p[0],p[1],p[2])==FH_AUX_INPUT_INVALID);
    assert(fh_audio_aux_input_push(&q,8,9,0,p[0],p[1],p[2])==FH_AUX_INPUT_BUSY);
    assert(fh_audio_aux_input_ready(&q,7,9,UINT64_MAX,1)==FH_AUX_INPUT_INVALID);
    assert(!memcmp(&q,&before,sizeof(q)));
    consume(&q,4000,159);
    assert(q.tail==before.tail && q.read_offset==159);
    assert(fh_audio_aux_input_push(&q,7,9,14240,p[0],p[1],p[2])==FH_AUX_INPUT_BUSY);
    /* Overflow in a later channel/sample must preserve ALL output and cursors. */
    memset(p,0,sizeof(p));p[channels-1][159]=value(4318,channels-1)>0?INT32_MAX:INT32_MIN;
    int32_t unchanged[3][160];memcpy(unchanged,p,sizeof(p));before=q;
    assert(fh_audio_aux_input_add(&q,7,9,4159,160,p[0],p[1],p[2])==FH_AUX_INPUT_INVALID);
    assert(!memcmp(p,unchanged,sizeof(p)) && !memcmp(&q,&before,sizeof(q)));
    consume(&q,4159,160);
    fill(p,14240);assert(fh_audio_aux_input_push(&q,7,9,14240,p[0],p[1],p[2])==FH_AUX_INPUT_OK);
    uint64_t at=4319;
    while(at<14400) { unsigned n=(unsigned)(14400-at);if(n>127)n=127;consume(&q,at,n);at+=n; }
    assert(q.tail==q.head && q.read_offset==0);
    fill(p,14400);assert(fh_audio_aux_input_push(&q,7,9,14400,p[0],p[1],p[2])==FH_AUX_INPUT_OK);
    fill(p,14560);assert(fh_audio_aux_input_push(&q,7,10,14560,p[0],p[1],p[2])==FH_AUX_INPUT_OK);
    assert(fh_audio_aux_input_ready(&q,7,9,14559,2)==FH_AUX_INPUT_INVALID);
    consume(&q,14400,160);
    assert(fh_audio_aux_input_ready(&q,7,10,14560,160)==FH_AUX_INPUT_OK);
    free(storage);
}
static void generations(unsigned channels) {
    FhAudioAuxInput q={0};size_t bytes=fh_audio_aux_input_bytes(channels);
    int32_t *storage=malloc(bytes);assert(storage);
    assert(fh_audio_aux_input_init(&q,storage,bytes,channels));
    q.head=q.tail=UINT32_MAX-1;
    int32_t p[3][160],result[3][160]={0};
    /* All three generations are present at once, with identical clock values.
     * Their samples and route identities must stay distinct after serial wrap. */
    for(unsigned g=7;g<=9;g++) {
        for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)p[c][i]=g*1000+c*160+i;
        assert(!fh_audio_aux_input_push(&q,g,g+2,0,p[0],p[1],p[2]));
    }
    FhAudioAuxInput before=q;
    assert(fh_audio_aux_input_push(&q,8,10,160,p[0],p[1],p[2])==FH_AUX_INPUT_INVALID);
    assert(!memcmp(&q,&before,sizeof(q)));
    assert(!fh_audio_aux_input_ready(&q,9,11,0,160));
    assert(fh_audio_aux_input_next(&q,9,11,0,160)==FH_AUX_INPUT_INVALID);
    assert(fh_audio_aux_input_check(&q,0,7,9,159,2)==FH_AUX_INPUT_INVALID);
    for(unsigned g=7;g<=9;g++) {
        for(unsigned at=0;at<160;) {
            unsigned n=at?159:1;memset(result,0,sizeof(result));
            if(g<9)assert(!fh_audio_aux_input_check(&q,160-at,g+1,g+3,0,160));
            assert(!fh_audio_aux_input_add(&q,g,g+2,at,n,result[0],result[1],result[2]));
            for(unsigned c=0;c<channels;c++)for(unsigned i=0;i<n;i++)
                assert(result[c][i]==(int32_t)(g*1000+c*160+at+i));
            at+=n;
        }
    }
    assert(q.head==q.tail && !q.read_offset);free(storage);
}
enum { BLOCKS=12000 };
static void *produce(void *arg) {
    FhAudioAuxInput *q=arg;int32_t p[3][160];
    for(unsigned b=0;b<BLOCKS;b++) {
        fill(p,160*(uint64_t)b);int result;
        while((result=fh_audio_aux_input_push(q,7,9,160*(uint64_t)b,p[0],p[1],p[2]))==FH_AUX_INPUT_BUSY)sched_yield();
        assert(result==FH_AUX_INPUT_OK);
    }
    return NULL;
}
static void threaded(unsigned channels) {
    FhAudioAuxInput q={0};size_t bytes=fh_audio_aux_input_bytes(channels);
    int32_t *storage=malloc(bytes);assert(storage);
    assert(fh_audio_aux_input_init(&q,storage,bytes,channels));
    q.head=q.tail=UINT32_MAX-256;
    pthread_t producer;assert(!pthread_create(&producer,NULL,produce,&q));
    uint64_t first=0;unsigned random=123;
    while(first<BLOCKS*160) {
        random=random*1664525+1013904223;unsigned n=1+random%160;
        if(n>BLOCKS*160-first)n=(unsigned)(BLOCKS*160-first);
        int result;
        while((result=fh_audio_aux_input_ready(&q,7,9,first,n))==FH_AUX_INPUT_BUSY)sched_yield();
        assert(result==FH_AUX_INPUT_OK);consume(&q,first,n);first+=n;
    }
    assert(!pthread_join(producer,NULL));assert(q.head==q.tail && !q.read_offset);free(storage);
}
int main(void) {
    assert(!fh_audio_aux_input_bytes(1) && !fh_audio_aux_input_bytes(4));
    cases(2);cases(3);generations(2);generations(3);threaded(2);threaded(3);
    printf("{\"passed\":true,\"threaded_source_frames\":%u,\"host_queue_bytes\":%zu}\n",BLOCKS*160*2,sizeof(FhAudioAuxInput));
}
