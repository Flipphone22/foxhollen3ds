#include "fh_audio_sample_pool.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static _Alignas(32) unsigned char arena[32768],encoded[4096];
static FhAudioSamplePool pool;
static unsigned flushes,deny_flush,checks,rng=92714;
static unsigned next(void){rng=rng*1664525u+1013904223u;return rng>>8;}
static FhAudioSampleInput input(unsigned id) {
    FhAudioSampleInput in={.encoded=encoded,.encoded_bytes=sizeof(encoded),.length=23+id%64*31};
    in.initial_ps=encoded[0];
    for(unsigned i=0;i<16;i++)in.coefficients[i]=(i&1)?-600:1200;
    if(id%3) {in.loop_start=3+id%14;in.loop_length=in.length-in.loop_start;in.loop_ps=0x26;in.loop_y1=150;in.loop_y2=-400;}
    return in;
}
static int flush(void *ctx,const void *data,size_t bytes) {
    assert(ctx==&pool);assert((const uint8_t *)data>=arena);
    assert(bytes && bytes<=sizeof(arena)-(size_t)((const uint8_t *)data-arena));
    assert(!((uintptr_t)data&31));flushes++;
    return !deny_flush;
}
static void basic(void) {
    FhAudioPoolToken bank,e,l1,l2,fresh_bank,fresh_entry;
    FhAudioSampleInput in=input(7);const void *p;const FhAudioDspSamplePlan *d;
    assert(fh_audio_pool_init(&pool,arena+1,sizeof(arena)-1)==FH_AUDIO_POOL_INVALID);
    assert(fh_audio_pool_init(&pool,arena,sizeof(arena))==0);
    assert(fh_audio_pool_init(&pool,arena,sizeof(arena))==FH_AUDIO_POOL_BUSY);
    assert(fh_audio_pool_bank_open(&pool,&bank)==0);
    assert(fh_audio_pool_prepare(&pool,bank,7,&in,&e)==0);
    assert(fh_audio_pool_acquire(&pool,bank,7,&l1)==FH_AUDIO_POOL_UNPUBLISHED);
    deny_flush=1;assert(fh_audio_pool_publish(&pool,e,flush,&pool)==FH_AUDIO_POOL_FLUSH_FAILED);
    assert(fh_audio_pool_acquire(&pool,bank,7,&l1)==FH_AUDIO_POOL_UNPUBLISHED);
    deny_flush=0;assert(fh_audio_pool_publish(&pool,e,flush,&pool)==0);
    unsigned calls=flushes;assert(fh_audio_pool_publish(&pool,e,flush,&pool)==0);assert(calls==flushes);
    assert(fh_audio_pool_acquire(&pool,bank,7,&l1)==0);
    assert(fh_audio_pool_acquire(&pool,bank,7,&l2)==0 && l1!=l2);
    assert(fh_audio_pool_view(&pool,l2,&p,&d)==0);
    size_t saved_bytes=d->storage_bytes;void *saved=malloc(saved_bytes);assert(saved);memcpy(saved,p,saved_bytes);
    unsigned builds=pool.compiles;
    assert(fh_audio_pool_prepare(&pool,bank,7,NULL,&fresh_entry)==0 && fresh_entry==e);
    assert(pool.compiles==builds);
    assert(fh_audio_pool_release(&pool,l1)==0);
    assert(fh_audio_pool_release(&pool,l1)==FH_AUDIO_POOL_INVALID);
    assert(fh_audio_pool_bank_close(&pool,bank)==0);
    assert(fh_audio_pool_acquire(&pool,bank,7,&l1)==FH_AUDIO_POOL_INVALID);
    assert(fh_audio_pool_prepare(&pool,bank,7,&in,&fresh_entry)==FH_AUDIO_POOL_INVALID);
    assert(fh_audio_pool_view(&pool,l2,&p,&d)==0 && d->storage_bytes==saved_bytes && !memcmp(p,saved,saved_bytes));
    assert(fh_audio_pool_bank_open(&pool,&fresh_bank)==0 && fresh_bank!=bank);
    encoded[1]^=0x71;
    assert(fh_audio_pool_prepare(&pool,fresh_bank,7,&in,&fresh_entry)==0 && fresh_entry!=e);
    assert(fh_audio_pool_view(&pool,l2,&p,&d)==0 && d->storage_bytes==saved_bytes && !memcmp(p,saved,saved_bytes));
    assert(fh_audio_pool_shutdown(&pool)==FH_AUDIO_POOL_BUSY);
    assert(fh_audio_pool_view(&pool,l2,&p,&d)==0 && d->storage_bytes==saved_bytes && !memcmp(p,saved,saved_bytes));
    assert(fh_audio_pool_bank_open(&pool,&bank)==FH_AUDIO_POOL_INVALID);
    assert(fh_audio_pool_release(&pool,l2)==0);free(saved);
    uint64_t serial=pool.serial;assert(fh_audio_pool_shutdown(&pool)==0);
    assert(pool.serial==serial);assert(fh_audio_pool_init(&pool,arena,sizeof(arena))==0);
    assert(pool.serial==serial);
    assert(fh_audio_pool_bank_open(&pool,&bank)==0 && bank!=fresh_bank);
    assert(fh_audio_pool_prepare(&pool,bank,7,&in,&e)==0);
    assert(fh_audio_pool_publish(&pool,e,flush,&pool)==0);
    assert(fh_audio_pool_acquire(&pool,bank,7,&l1)==0);
    assert(fh_audio_pool_release(&pool,l2)==FH_AUDIO_POOL_INVALID);
    assert(fh_audio_pool_release(&pool,l1)==0);assert(fh_audio_pool_shutdown(&pool)==0);
}
typedef struct {FhAudioPoolToken token;uint32_t bytes;const void *address;void *copy;} Held;
static Held held[128];
static void validate(void) {
    unsigned pins=0,leases=0;uint32_t used=0;
    for(unsigned i=0;i<128;i++)if(held[i].token) {
        const void *p;const FhAudioDspSamplePlan *d;
        assert(fh_audio_pool_view(&pool,held[i].token,&p,&d)==0);
        assert(p==held[i].address && d->storage_bytes==held[i].bytes);
        assert(!memcmp(p,held[i].copy,d->storage_bytes));checks++;
    }
    for(unsigned i=0;i<64;i++)if(pool.entries[i].token) {
        FhAudioPoolEntry *e=pool.entries+i;used+=e->bytes;pins+=e->pins;
        assert(e->offset<=pool.capacity && e->bytes<=pool.capacity-e->offset);
        for(unsigned j=i+1;j<64;j++)if(pool.entries[j].token) {
            FhAudioPoolEntry *b=pool.entries+j;
            assert(e->offset+e->bytes<=b->offset || b->offset+b->bytes<=e->offset);
        }
    }
    for(unsigned i=0;i<128;i++)leases+=!!pool.leases[i].token;
    assert(leases==pins && used==pool.used && pool.peak<=pool.capacity);
}
static void limits(void) {
    FhAudioPoolToken banks[64],all[128],e,extra;
    assert(fh_audio_pool_init(&pool,arena,sizeof(arena))==0);
    for(unsigned i=0;i<64;i++)assert(fh_audio_pool_bank_open(&pool,banks+i)==0);
    assert(fh_audio_pool_bank_open(&pool,&extra)==FH_AUDIO_POOL_FULL);
    FhAudioSampleInput in=input(7);
    assert(fh_audio_pool_prepare(&pool,banks[0],7,&in,&e)==0);
    assert(fh_audio_pool_publish(&pool,e,flush,&pool)==0);
    for(unsigned i=0;i<128;i++)assert(fh_audio_pool_acquire(&pool,banks[0],7,all+i)==0);
    assert(fh_audio_pool_acquire(&pool,banks[0],7,&extra)==FH_AUDIO_POOL_FULL);
    assert(fh_audio_pool_shutdown(&pool)==FH_AUDIO_POOL_BUSY);
    for(unsigned i=0;i<128;i++) {
        assert(fh_audio_pool_release(&pool,all[i])==0);
        assert(fh_audio_pool_release(&pool,all[i])==FH_AUDIO_POOL_INVALID);
        if(i<127)assert(fh_audio_pool_shutdown(&pool)==FH_AUDIO_POOL_BUSY);
    }
    assert(pool.used==0);assert(fh_audio_pool_shutdown(&pool)==0);
}
int main(void) {
    for(unsigned i=0;i<sizeof(encoded);i++)encoded[i]=(next()>>16)&0x77;
    basic();limits();assert(fh_audio_pool_init(&pool,arena,sizeof(arena))==0);
    FhAudioPoolToken banks[4];for(unsigned i=0;i<4;i++)assert(fh_audio_pool_bank_open(&pool,banks+i)==0);
    unsigned full=0,loads=0;
    for(unsigned step=0;step<25000;step++) {
        unsigned op=next()%10,b=next()%4,s=next()%128,slot=next()%128;
        if(op<6) {
            FhAudioSampleInput in=input(s);FhAudioPoolToken e;
            int r=fh_audio_pool_prepare(&pool,banks[b],s,&in,&e);
            if(r==FH_AUDIO_POOL_FULL)full++;
            else {
                assert(r==0);assert(fh_audio_pool_publish(&pool,e,flush,&pool)==0);loads++;
                if(!held[slot].token) {
                    FhAudioPoolToken l;r=fh_audio_pool_acquire(&pool,banks[b],s,&l);
                    assert(r==0 || r==FH_AUDIO_POOL_FULL);
                    if(!r) {
                        const void *p;const FhAudioDspSamplePlan *d;
                        assert(fh_audio_pool_view(&pool,l,&p,&d)==0);
                        void *copy=malloc(d->storage_bytes);assert(copy);memcpy(copy,p,d->storage_bytes);
                        held[slot]=(Held){l,d->storage_bytes,p,copy};
                    }
                }
            }
        } else if(op<9) {
            if(held[slot].token) {
                assert(fh_audio_pool_release(&pool,held[slot].token)==0);
                assert(fh_audio_pool_release(&pool,held[slot].token)==FH_AUDIO_POOL_INVALID);
                free(held[slot].copy);held[slot].token=0;
            }
        } else {
            assert(fh_audio_pool_bank_close(&pool,banks[b])==0);
            assert(fh_audio_pool_bank_open(&pool,banks+b)==0);
        }
        if(step%31==0)validate();
    }
    validate();unsigned peak=pool.peak,compiles=pool.compiles,evictions=pool.evictions;
    for(unsigned i=0;i<128;i++)if(held[i].token) {
        assert(fh_audio_pool_release(&pool,held[i].token)==0);free(held[i].copy);held[i].token=0;
    }
    assert(fh_audio_pool_shutdown(&pool)==0);
    assert(full && evictions && checks>1000);
    printf("operations=25000 held_byte_checks=%u loads=%u full=%u compiles=%u evictions=%u peak=%u metadata=%zu\n",
        checks,loads,full,compiles,evictions,peak,sizeof(pool));
}
