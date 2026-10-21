#include "fh_audio_sample_pool.h"
#include <string.h>

static int overlap(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y || (x<y+nb && y<x+na);
}
static unsigned index_of(FhAudioPoolToken token){return (unsigned)(token&255)-1;}
static int fresh(const FhAudioSamplePool *p){return p->serial<(UINT64_MAX>>8);}
static FhAudioPoolToken token(FhAudioSamplePool *p,unsigned index){return (++p->serial<<8)|(index+1);}
static int open_bank(FhAudioSamplePool *p,FhAudioPoolToken bank) {
    unsigned i=index_of(bank);
    return p && p->arena && !p->closing && i<FH_AUDIO_POOL_BANKS && p->banks[i]==bank;
}
static FhAudioPoolEntry *entry(FhAudioSamplePool *p,FhAudioPoolToken id) {
    unsigned i=index_of(id);
    return p && p->arena && i<FH_AUDIO_POOL_SAMPLES && p->entries[i].token==id?&p->entries[i]:NULL;
}
static FhAudioPoolLease *lease(FhAudioSamplePool *p,FhAudioPoolToken id) {
    unsigned i=index_of(id);
    return p && p->arena && i<FH_AUDIO_POOL_LEASES && p->leases[i].token==id?&p->leases[i]:NULL;
}
static void discard(FhAudioSamplePool *p,FhAudioPoolEntry *e) {
    p->used-=e->bytes;memset(e,0,sizeof(*e));
}
int fh_audio_pool_init(FhAudioSamplePool *p,void *arena,size_t bytes) {
    if(!p || !arena || ((uintptr_t)arena&31) || !bytes || bytes%32 ||
       bytes>FH_AUDIO_POOL_MAX_BYTES || overlap(p,sizeof(*p),arena,bytes))return FH_AUDIO_POOL_INVALID;
    if(p->arena)return FH_AUDIO_POOL_BUSY;
    uint64_t serial=p->serial;memset(p,0,sizeof(*p));p->serial=serial;
    p->arena=arena;p->capacity=bytes;return FH_AUDIO_POOL_OK;
}
int fh_audio_pool_bank_open(FhAudioSamplePool *p,FhAudioPoolToken *out) {
    if(!p || !p->arena || p->closing || !out)return FH_AUDIO_POOL_INVALID;
    if(!fresh(p))return FH_AUDIO_POOL_FULL;
    for(unsigned i=0;i<FH_AUDIO_POOL_BANKS;i++)if(!p->banks[i]) {
        p->banks[i]=token(p,i);*out=p->banks[i];return FH_AUDIO_POOL_OK;
    }
    return FH_AUDIO_POOL_FULL;
}
int fh_audio_pool_bank_close(FhAudioSamplePool *p,FhAudioPoolToken bank) {
    if(!open_bank(p,bank))return FH_AUDIO_POOL_INVALID;
    p->banks[index_of(bank)]=0;
    for(unsigned i=0;i<FH_AUDIO_POOL_SAMPLES;i++) {
        FhAudioPoolEntry *e=&p->entries[i];if(!e->token || e->bank!=bank)continue;
        if(e->pins)e->state=3;else discard(p,e);
    }
    return FH_AUDIO_POOL_OK;
}
/* First-fit over immutable extents. At most 64 jumps; never compact/move a live
 * allocation. This scan runs at preparation, not at per-sample/per-frame mix. */
static int hole(FhAudioSamplePool *p,uint32_t bytes,uint32_t *offset) {
    uint32_t at=0;
    for(unsigned pass=0;pass<=FH_AUDIO_POOL_SAMPLES;pass++) {
        if(at>p->capacity || bytes>p->capacity-at)return 0;
        uint32_t next=at;
        for(unsigned i=0;i<FH_AUDIO_POOL_SAMPLES;i++) {
            FhAudioPoolEntry *e=&p->entries[i];
            if(e->token && at<e->offset+e->bytes && e->offset<at+bytes && e->offset+e->bytes>next)
                next=e->offset+e->bytes;
        }
        if(next==at){*offset=at;return 1;}at=next;
    }
    return 0;
}
int fh_audio_pool_prepare(FhAudioSamplePool *p,FhAudioPoolToken bank,uint32_t sample,
                          const FhAudioSampleInput *in,FhAudioPoolToken *out) {
    if(!out || !open_bank(p,bank))return FH_AUDIO_POOL_INVALID;
    for(unsigned i=0;i<FH_AUDIO_POOL_SAMPLES;i++) {
        FhAudioPoolEntry *e=&p->entries[i];
        if(e->token && e->bank==bank && e->sample==sample) {
            e->last_use=++p->clock;++p->hits;*out=e->token;return FH_AUDIO_POOL_OK;
        }
    }
    FhAudioDspSamplePlan plan;
    if(!in || fh_audio_dsp_sample_measure(in,&plan)!=FH_AUDIO_PLAN_OK ||
       overlap(in,sizeof(*in),p,sizeof(*p)) ||
       overlap(in,sizeof(*in),p->arena,p->capacity) ||
       overlap(in->encoded,in->encoded_bytes,p->arena,p->capacity) ||
       overlap(in->encoded,in->encoded_bytes,p,sizeof(*p)))return FH_AUDIO_POOL_INVALID;
    if(plan.storage_bytes>p->capacity || !fresh(p))return FH_AUDIO_POOL_FULL;
    uint32_t bytes=(plan.storage_bytes+31)&~31u,offset=0;
    unsigned slot=FH_AUDIO_POOL_SAMPLES;
    for(;;) {
        for(unsigned i=0;i<FH_AUDIO_POOL_SAMPLES;i++)if(!p->entries[i].token){slot=i;break;}
        if(slot<FH_AUDIO_POOL_SAMPLES && hole(p,bytes,&offset))break;
        FhAudioPoolEntry *victim=NULL;
        for(unsigned i=0;i<FH_AUDIO_POOL_SAMPLES;i++) {
            FhAudioPoolEntry *e=&p->entries[i];
            if(e->token && !e->pins && (!victim || e->last_use<victim->last_use))victim=e;
        }
        if(!victim)return FH_AUDIO_POOL_FULL;
        discard(p,victim);++p->evictions;
    }
    if(fh_audio_dsp_sample_compile(in,(uint8_t *)p->arena+offset,bytes,&plan)!=FH_AUDIO_PLAN_OK)
        return FH_AUDIO_POOL_INVALID;
    FhAudioPoolEntry *e=&p->entries[slot];
    *e=(FhAudioPoolEntry){.token=token(p,slot),.bank=bank,.plan=plan,
        .last_use=++p->clock,.offset=offset,.bytes=bytes,.sample=sample,.state=1};
    p->used+=bytes;if(p->used>p->peak)p->peak=p->used;++p->compiles;
    *out=e->token;return FH_AUDIO_POOL_OK;
}
int fh_audio_pool_publish(FhAudioSamplePool *p,FhAudioPoolToken id,FhAudioPoolFlush flush,void *ctx) {
    FhAudioPoolEntry *e=entry(p,id);
    if(!e || !open_bank(p,e->bank))return FH_AUDIO_POOL_INVALID;
    if(e->state==2)return FH_AUDIO_POOL_OK;
    if(e->state!=1 || !flush)return FH_AUDIO_POOL_INVALID;
    if(!flush(ctx,(uint8_t *)p->arena+e->offset,e->plan.storage_bytes)) {
        ++p->flush_failures;return FH_AUDIO_POOL_FLUSH_FAILED;
    }
    e->state=2;return FH_AUDIO_POOL_OK;
}
int fh_audio_pool_acquire(FhAudioSamplePool *p,FhAudioPoolToken bank,uint32_t sample,FhAudioPoolToken *out) {
    if(!out || !open_bank(p,bank))return FH_AUDIO_POOL_INVALID;
    FhAudioPoolEntry *e=NULL;
    for(unsigned i=0;i<FH_AUDIO_POOL_SAMPLES;i++)if(p->entries[i].token &&
        p->entries[i].bank==bank && p->entries[i].sample==sample){e=&p->entries[i];break;}
    if(!e)return FH_AUDIO_POOL_INVALID;
    if(e->state!=2)return FH_AUDIO_POOL_UNPUBLISHED;
    if(!fresh(p))return FH_AUDIO_POOL_FULL;
    for(unsigned i=0;i<FH_AUDIO_POOL_LEASES;i++)if(!p->leases[i].token) {
        p->leases[i]=(FhAudioPoolLease){token(p,i),e->token};++e->pins;
        e->last_use=++p->clock;*out=p->leases[i].token;return FH_AUDIO_POOL_OK;
    }
    return FH_AUDIO_POOL_FULL;
}
int fh_audio_pool_view(FhAudioSamplePool *p,FhAudioPoolToken id,const void **data,const FhAudioDspSamplePlan **plan) {
    FhAudioPoolLease *l=lease(p,id);FhAudioPoolEntry *e=l?entry(p,l->entry):NULL;
    if(!e || !e->pins || !data || !plan)return FH_AUDIO_POOL_INVALID;
    *data=(uint8_t *)p->arena+e->offset;*plan=&e->plan;return FH_AUDIO_POOL_OK;
}
int fh_audio_pool_release(FhAudioSamplePool *p,FhAudioPoolToken id) {
    FhAudioPoolLease *l=lease(p,id);FhAudioPoolEntry *e=l?entry(p,l->entry):NULL;
    if(!e || !e->pins)return FH_AUDIO_POOL_INVALID;
    memset(l,0,sizeof(*l));--e->pins;if(!e->pins && e->state==3)discard(p,e);
    return FH_AUDIO_POOL_OK;
}
int fh_audio_pool_shutdown(FhAudioSamplePool *p) {
    if(!p || !p->arena)return FH_AUDIO_POOL_INVALID;
    if(!p->closing) {
        for(unsigned i=0;i<FH_AUDIO_POOL_BANKS;i++)if(p->banks[i])fh_audio_pool_bank_close(p,p->banks[i]);
        p->closing=1;
    }
    for(unsigned i=0;i<FH_AUDIO_POOL_LEASES;i++)if(p->leases[i].token)return FH_AUDIO_POOL_BUSY;
    uint64_t serial=p->serial;memset(p,0,sizeof(*p));p->serial=serial;return FH_AUDIO_POOL_OK;
}
