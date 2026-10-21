#include "fh_gx_ring.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
static FhGxRing queue;
static uint32_t storage[128] __attribute__((aligned(32)));
enum { RECORDS=200000 };
static void *produce(void *unused){
    (void)unused;
    for(unsigned seq=1;seq<=RECORDS;seq++){
        unsigned words=3+(seq*17u)%29;uint32_t*r;
        while(!(r=fh_gx_ring_reserve(&queue,words))){fh_gx_ring_publish(&queue);sched_yield();}
        r[1]=seq;r[2]=words;
        for(unsigned i=3;i<words;i++)r[i]=seq^(0x9e3779b9u*i);
        assert(fh_gx_ring_commit(&queue,1+seq%240));
        if(seq%7==0)fh_gx_ring_publish(&queue);
    }
    fh_gx_ring_publish(&queue);return NULL;
}
static void *consume(void *unused){
    (void)unused;
    for(unsigned seq=1;seq<=RECORDS;seq++){
        const uint32_t*r;unsigned words,op;int status;
        while((status=fh_gx_ring_peek(&queue,&r,&words,&op))==FH_GX_RING_EMPTY)sched_yield();
        assert(status==FH_GX_RING_READY && words==3+(seq*17u)%29 && op==1+seq%240);
        assert(r[1]==seq && r[2]==words);
        for(unsigned i=3;i<words;i++)assert(r[i]==(seq^(0x9e3779b9u*i)));
        assert(!fh_gx_ring_release(&queue,r,words+1));
        assert(fh_gx_ring_release(&queue,r,words));
    }
    return NULL;
}
static void *consume_batches(void *unused){
    (void)unused;unsigned seq=1;
    while(seq<=RECORDS){
        FhGxRingBatch batch;assert(fh_gx_ring_batch_begin(&queue,&batch));
        for(unsigned n=0;n<64 && seq<=RECORDS;n++){
            const uint32_t *r;unsigned words,op;
            int status=fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op);
            if(status==FH_GX_RING_EMPTY)break;
            assert(status==FH_GX_RING_READY && words==3+(seq*17u)%29 && op==1+seq%240);
            assert(r[1]==seq && r[2]==words);
            for(unsigned i=3;i<words;i++)assert(r[i]==(seq^(0x9e3779b9u*i)));
            assert(!fh_gx_ring_batch_release(&queue,&batch,r,words+1));
            assert(fh_gx_ring_batch_release(&queue,&batch,r,words));
            assert(__atomic_load_n(&queue.read,__ATOMIC_RELAXED)==batch.origin);
            ++seq;
        }
        assert(fh_gx_ring_batch_finish(&queue,&batch));
        sched_yield();
    }
    return NULL;
}
static void batch_ownership(void){
    assert(fh_gx_ring_init(&queue,storage,128));queue.cache_reads=1;
    uint32_t *a=fh_gx_ring_reserve(&queue,64);assert(a);a[1]=0x12345678;
    assert(fh_gx_ring_commit(&queue,1));fh_gx_ring_publish(&queue);
    FhGxRingBatch batch;assert(fh_gx_ring_batch_begin(&queue,&batch));
    const uint32_t *r;unsigned words,op;
    assert(fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op)==FH_GX_RING_READY);
    assert(r==a && r[1]==0x12345678 && fh_gx_ring_batch_release(&queue,&batch,r,words));
    uint32_t *b=fh_gx_ring_reserve(&queue,64);assert(b && b!=a);b[1]=0x87654321;
    assert(fh_gx_ring_commit(&queue,2));fh_gx_ring_publish(&queue);
    /* New publication is invisible to the old snapshot, and no consumed
     * storage is reusable until its release. This also exercises cached-read
     * refresh exactly at the full-ring boundary. */
    assert(fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op)==FH_GX_RING_EMPTY);
    assert(!fh_gx_ring_reserve(&queue,1) && a[1]==0x12345678);
    assert(fh_gx_ring_batch_finish(&queue,&batch));
    assert(!fh_gx_ring_batch_finish(&queue,&batch));
    assert(fh_gx_ring_reserve(&queue,64)==a);assert(fh_gx_ring_cancel(&queue));
    assert(fh_gx_ring_batch_begin(&queue,&batch));
    assert(fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op)==FH_GX_RING_READY && r==b && r[1]==0x87654321);
    assert(fh_gx_ring_batch_release(&queue,&batch,r,words));assert(fh_gx_ring_batch_finish(&queue,&batch));
    assert(fh_gx_ring_init(&queue,storage,128));
    storage[0]=(1u<<24)|8;queue.published=4;
    assert(fh_gx_ring_batch_begin(&queue,&batch));
    assert(fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op)==FH_GX_RING_BAD);
    storage[0]=(255u<<24)|4;
    assert(fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op)==FH_GX_RING_BAD);
    storage[0]=1u<<24;
    assert(fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op)==FH_GX_RING_BAD);
    queue.published=129;assert(!fh_gx_ring_batch_begin(&queue,&batch));
    assert(fh_gx_ring_init(&queue,storage,128));queue.write=queue.read=queue.published=126;queue.cache_reads=1;
    assert(fh_gx_ring_reserve(&queue,4)==storage);assert(fh_gx_ring_cancel(&queue));fh_gx_ring_publish(&queue);
    assert(fh_gx_ring_batch_begin(&queue,&batch));
    assert(fh_gx_ring_batch_peek(&queue,&batch,&r,&words,&op)==FH_GX_RING_EMPTY);
    assert(queue.read==126 && fh_gx_ring_batch_finish(&queue,&batch) && queue.read==128);
}
int main(void){
    assert(!fh_gx_ring_init(&queue,storage,127));assert(fh_gx_ring_init(&queue,storage,128));
    assert(!fh_gx_ring_reserve(&queue,65));
    uint32_t*r=fh_gx_ring_reserve(&queue,16);assert(r);
    const uint32_t *read;unsigned words,op;
    fh_gx_ring_publish(&queue);assert(fh_gx_ring_peek(&queue,&read,&words,&op)==FH_GX_RING_EMPTY);
    memset(r+1,0xa5,15*4);assert(!fh_gx_ring_reserve(&queue,1));assert(fh_gx_ring_commit(&queue,1));
    assert(fh_gx_ring_peek(&queue,&read,&words,&op)==FH_GX_RING_EMPTY);
    fh_gx_ring_publish(&queue);assert(fh_gx_ring_peek(&queue,&read,&words,&op)==FH_GX_RING_READY);
    assert(read[15]==0xa5a5a5a5 && fh_gx_ring_release(&queue,read,words));
    assert(!fh_gx_ring_cancel(&queue));
    queue.write=queue.read=queue.published=126;
    r=fh_gx_ring_reserve(&queue,4);assert(r==storage && queue.write==128);
    r[1]=0x12345678;fh_gx_ring_publish(&queue);
    assert(queue.published==126 && fh_gx_ring_cancel(&queue));fh_gx_ring_publish(&queue);
    assert(fh_gx_ring_peek(&queue,&read,&words,&op)==FH_GX_RING_EMPTY && queue.read==128);
    r=fh_gx_ring_reserve(&queue,4);assert(r==storage);r[1]=0xabcdef01;
    assert(fh_gx_ring_commit(&queue,9));fh_gx_ring_publish(&queue);
    assert(fh_gx_ring_peek(&queue,&read,&words,&op)==FH_GX_RING_READY && op==9 && read[1]==0xabcdef01);
    assert(fh_gx_ring_release(&queue,read,words));
    assert(fh_gx_ring_init(&queue,storage,128));
    /* Deliberately truncated/malformed published records must fail closed. */
    storage[0]=(1u<<24)|8;queue.published=4;assert(fh_gx_ring_peek(&queue,&read,&words,&op)==FH_GX_RING_BAD);
    storage[0]=1u<<24;assert(fh_gx_ring_peek(&queue,&read,&words,&op)==FH_GX_RING_BAD);
    storage[0]=(255u<<24)|4;assert(fh_gx_ring_peek(&queue,&read,&words,&op)==FH_GX_RING_BAD);
    batch_ownership();
    for(unsigned batching=0;batching<2;batching++){
        assert(fh_gx_ring_init(&queue,storage,128));queue.cache_reads=batching;
        queue.write=queue.read=queue.published=UINT32_MAX-63u;
        pthread_t a,b;assert(!pthread_create(&a,NULL,produce,NULL));assert(!pthread_create(&b,NULL,batching?consume_batches:consume,NULL));
        assert(!pthread_join(a,NULL));assert(!pthread_join(b,NULL));
        assert(queue.write==queue.read && queue.read==queue.published);
    }
    printf("PASS %u concurrent variable records per mode across counter/storage wraps; batch ownership, malformed records and retirement checked\n",RECORDS);
}
