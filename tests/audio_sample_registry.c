#include "fh_audio_sample_registry.h"
#include "hw_pc_assets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
extern void salPCResetSampleData(void),salPCFinishSampleUpload(void);
static FhAudioSamplePool pool;
static FhAudioSampleRegistry registry;
static unsigned char arena[8192] __attribute__((aligned(32)));
static unsigned char uploads[128][128],occupied[128];
static unsigned char source_bytes[128*128];
static unsigned flushes,deny_flush,loads,frees;
static int flush(void *context,const void *data,size_t bytes) {
    assert(context==&registry && data && bytes);flushes++;return !deny_flush;
}
/* Only original hardware upload storage is substituted. The real source
 * directory/reference lookup, registration and removal implementation executes.
 * First-fit fixed slots deliberately reuse exactly the same source address. */
void hwSaveSample(void *header,void *data) {
    SAMPLE_HEADER *h=*(SAMPLE_HEADER **)header;unsigned length=h->length&0xffffff;
    unsigned bytes=(h->length>>24)==SAMPLE_TYPE_ADPCM?((length+13)/14)*8:length*2;
    assert(bytes<=128);
    for(unsigned i=0;i<128;i++)if(!occupied[i]) {
        occupied[i]=1;memcpy(uploads[i],*(void **)data,bytes);*(void **)data=uploads[i];loads++;return;
    }
    assert(!"upload capacity");
}
void hwRemoveSample(void *header,void *data) {
    (void)header;
    /* Invalidation must precede freeing/reusing source data. */
    for(unsigned j=0;j<FH_AUDIO_REGISTRY_ENTRIES;j++)
        assert(!registry.entries[j].key || registry.entries[j].sample.addr!=data);
    for(unsigned i=0;i<128;i++)if(uploads[i]==data) {
        assert(occupied[i]);occupied[i]=0;memset(uploads[i],0xa5,128);frees++;return;
    }
    assert(!"unknown upload");
}
typedef struct {SDIR_DATA entries[70];DSPADPCMplusInfo info;} Directory;
static void directory(Directory *d,unsigned count,unsigned first) {
    memset(d,0,sizeof(*d));
    d->info.initialPS=0x11;d->info.loopPS=0x21;d->info.loopY1=300;d->info.loopY0=-900;
    for(unsigned i=0;i<count;i++) {
        d->entries[i].id=first+i;d->entries[i].offset=i*128;
        d->entries[i].header=(SAMPLE_HEADER){.length=28,.loopOffset=7,.loopLength=21};
        d->entries[i].extraData=(uintptr_t)&d->info-(uintptr_t)d->entries;
    }
    d->entries[count].id=0xffff;
}
static FhAudioPoolToken acquire(unsigned id,SAMPLE_INFO *sample,FhAudioPoolToken *bank) {
    assert(dataGetSample(id,sample)==0);
    assert(!fh_audio_sample_registry_find(&registry,sample,bank));
    FhAudioPoolToken lease;assert(!fh_audio_pool_acquire(&pool,*bank,0,&lease));return lease;
}
static void bytes_of(FhAudioPoolToken id,unsigned char *copy,size_t *bytes) {
    const void *data;const FhAudioDspSamplePlan *plan;
    assert(!fh_audio_pool_view(&pool,id,&data,&plan));
    *bytes=plan->storage_bytes;assert(*bytes<256);memcpy(copy,data,*bytes);
}
int main(void) {
    for(unsigned i=0;i<sizeof(source_bytes);i++)source_bytes[i]=(i*19+7)&255;
    for(unsigned i=0;i<128;i++)source_bytes[i*128]=0x11;
    assert(!fh_audio_pool_init(&pool,arena,sizeof(arena)));
    assert(fh_audio_sample_registry_init((FhAudioSampleRegistry *)arena,&pool,flush,&registry)==FH_AUDIO_POOL_INVALID);
    assert(!fh_audio_sample_registry_init(&registry,&pool,flush,&registry));
    fh_audio_native_samples=&registry;salPCResetSampleData();
    Directory a;directory(&a,2,10);assert(dataInsertSDir(a.entries,source_bytes));
    assert(dataAddSampleReference(10) && dataAddSampleReference(11));
    assert(registry.prepared==2 && pool.compiles==2);
    assert(dataAddSampleReference(10));salPCFinishSampleUpload();
    SAMPLE_INFO first,current;FhAudioPoolToken bank,new_bank;
    FhAudioPoolToken lease=acquire(10,&first,&bank);
    unsigned char old_copy[256],copy[256];size_t old_bytes,n;
    bytes_of(lease,old_copy,&old_bytes);unsigned compiled=pool.compiles,flushed=flushes;
    for(unsigned i=0;i<100;i++){assert(dataGetSample(10,&current)==0);assert(!memcmp(&current,&first,sizeof(first)));}
    assert(pool.compiles==compiled && flushes==flushed);
    assert(!dataRemoveSDir(a.entries));
    assert(dataRemoveSampleReference(10));assert(!fh_audio_sample_registry_find(&registry,&first,&new_bank) && new_bank==bank);
    assert(dataRemoveSampleReference(10));assert(fh_audio_sample_registry_find(&registry,&first,&new_bank)==FH_AUDIO_POOL_INVALID);
    FhAudioPoolToken dummy;assert(fh_audio_pool_acquire(&pool,bank,0,&dummy)==FH_AUDIO_POOL_INVALID);
    bytes_of(lease,copy,&n);assert(n==old_bytes && !memcmp(copy,old_copy,n));
    /* Same directory record, same uploaded address, different source content. */
    source_bytes[1]^=0x73;assert(dataInsertSDir(a.entries,source_bytes));assert(dataAddSampleReference(10));
    FhAudioPoolToken newer=acquire(10,&current,&new_bank);
    assert(current.addr==first.addr && new_bank!=bank && newer!=lease);
    bytes_of(newer,copy,&n);assert(n==old_bytes && memcmp(copy,old_copy,n));
    bytes_of(lease,copy,&n);assert(!memcmp(copy,old_copy,n));
    assert(!fh_audio_pool_release(&pool,lease) && !fh_audio_pool_release(&pool,newer));
    assert(dataRemoveSampleReference(10) && dataRemoveSampleReference(11) && dataRemoveSDir(a.entries));
    assert(!pool.used && !registry.faults);

    /* Compaction moves SDIR_TAB, not the canonical SDIR_DATA keys. Duplicate
     * sample IDs add references to the already-active owner, not uploading. */
    Directory b;directory(&a,1,200);directory(&b,2,200);
    assert(dataInsertSDir(a.entries,source_bytes));
    assert(dataInsertSDir(b.entries,source_bytes));
    assert(dataAddSampleReference(200) && dataAddSampleReference(201));
    lease=acquire(200,&first,&bank);assert(first.extraData==&b.info);
    compiled=pool.compiles;
    assert(dataRemoveSDir(a.entries)); /* moves b's source table */
    assert(dataGetSample(200,&current)==0 && current.extraData==&b.info && pool.compiles==compiled);
    assert(dataInsertSDir(a.entries,source_bytes) && dataAddSampleReference(200));
    assert(!a.entries[0].ref_cnt && b.entries[0].ref_cnt==2 && pool.compiles==compiled);
    assert(dataRemoveSDir(a.entries));
    assert(!fh_audio_pool_release(&pool,lease));
    assert(dataRemoveSampleReference(200) && dataRemoveSampleReference(200) && dataRemoveSampleReference(201));
    assert(dataRemoveSDir(b.entries) && !pool.used);

    /* More live source records than native-cache slots. Source loading remains
     * successful; evicted, still-live source data can be prepared on lookup. */
    directory(&a,65,100);assert(dataInsertSDir(a.entries,source_bytes));
    for(unsigned i=0;i<65;i++)assert(dataAddSampleReference(100+i));
    assert(registry.evicted && pool.used<=pool.capacity);
    lease=acquire(100,&first,&bank);bytes_of(lease,old_copy,&old_bytes);
    for(unsigned i=1;i<65;i++){assert(dataGetSample(100+i,&current)==0);}
    bytes_of(lease,copy,&n);assert(n==old_bytes && !memcmp(copy,old_copy,n));
    assert(!fh_audio_pool_release(&pool,lease));
    for(unsigned i=0;i<65;i++)assert(dataRemoveSampleReference(100+i));
    assert(dataRemoveSDir(a.entries) && !pool.used && !registry.faults);

    /* Publication failure cannot fail source loading or admit unpublished data.
     * The next canonical lookup can retry when publication is available. */
    directory(&a,1,1);deny_flush=1;assert(dataInsertSDir(a.entries,source_bytes));
    assert(dataAddSampleReference(1));assert(dataGetSample(1,&current)==0);
    assert(fh_audio_sample_registry_find(&registry,&current,&bank)!=0 && !pool.used);
    deny_flush=0;lease=acquire(1,&current,&bank);assert(!fh_audio_pool_release(&pool,lease));
    assert(dataRemoveSampleReference(1) && dataRemoveSDir(a.entries));

    /* Carry-history loops and streaming/virtual samples are not static plans. */
    for(unsigned mode=0;mode<3;mode++) {
        directory(&a,1,3);if(!mode)a.entries[0].extraData=0;
        else a.entries[0].header.length|=(mode==1?SAMPLE_TYPE_ADPCM_STREAM:SAMPLE_TYPE_ADPCM_VIRTUAL)<<24;
        assert(dataInsertSDir(a.entries,source_bytes) && dataAddSampleReference(3));
        assert(dataGetSample(3,&current)==0 && fh_audio_sample_registry_find(&registry,&current,&bank)!=0);
        assert(!pool.used);assert(dataRemoveSampleReference(3) && dataRemoveSDir(a.entries));
    }

    /* Original registry reset is separate from source-byte destruction. Native
     * copies retire at reset, but DSP pins survive until their owner releases. */
    directory(&a,1,8);assert(dataInsertSDir(a.entries,source_bytes) && dataAddSampleReference(8));
    lease=acquire(8,&first,&bank);bytes_of(lease,old_copy,&old_bytes);
    salPCResetSampleData();assert(fh_audio_sample_registry_find(&registry,&first,&new_bank)!=0);
    bytes_of(lease,copy,&n);assert(n==old_bytes && !memcmp(copy,old_copy,n));
    assert(!fh_audio_pool_release(&pool,lease) && !pool.used);
    /* The source reset itself does not free uploads; fixture explicitly cleans
     * the storage it owns, without claiming otherwise about the source API. */
    hwRemoveSample(&a.entries[0].header,first.addr);
    assert(!fh_audio_sample_registry_reset(&registry));
    fh_audio_native_samples=NULL;
    directory(&a,1,9);compiled=pool.compiles;
    assert(dataInsertSDir(a.entries,source_bytes) && dataAddSampleReference(9));
    assert(dataGetSample(9,&current)==0 && pool.compiles==compiled && !pool.used);
    assert(dataRemoveSampleReference(9) && dataRemoveSDir(a.entries));
    unsigned peak=pool.peak;
    assert(!fh_audio_pool_shutdown(&pool));fh_audio_native_samples=NULL;
    assert(loads==frees && !registry.faults);
    printf("loads=%u frees=%u prepared=%u hits=%u declined=%u evicted=%u peak=%u registry_bytes=%zu\n",
           loads,frees,registry.prepared,registry.hits,registry.declined,registry.evicted,peak,sizeof(registry));
    return 0;
}
