/* Independent oracle is the unmodified MusyX sample reader, including loop
 * history resets. Actual private bank assets stay local and are never emitted. */
#include "fh_audio_sample_plan.h"
#include "hw_pc_decode.h"
#include "hw_pc_assets.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <malloc.h>
static void *allocate_pcm(size_t n){return _aligned_malloc(n,32);}
static void release_pcm(void *p){_aligned_free(p);}
#else
static void *allocate_pcm(size_t n){return aligned_alloc(32,n);}
static void release_pcm(void *p){free(p);}
#endif
static uint64_t compared,total_bytes,max_bytes;
static unsigned cases,loops,unaligned_loops,formats[6];
static uint64_t dsp_bytes;static unsigned direct_count,pcm_count;
static FILE *inventory;static unsigned bank_id,sample_id;
void *salMalloc(size_t n){return malloc(n);}
void salFree(void *p){free(p);}
static void check(SAMPLE_INFO *s,size_t bytes) {
    const DSPADPCMplusInfo *a=s->extraData;
    FhAudioSampleInput in={.encoded=s->addr,.encoded_bytes=bytes,.length=s->length,
        .loop_start=s->loop,.loop_length=s->loopLength,
        .initial_ps=a?a->initialPS:((const uint8_t *)s->addr)[0],
        .loop_ps=a?a->loopPS:0,.loop_y1=a?a->loopY1:0,.loop_y2=a?a->loopY0:0};
    /* No-extra-data loops retain history in the original. The static bank
     * adapter must reject that class; this input contract carries explicit
     * reset history and must not guess it. */
    assert(a || !s->loopLength);
    if(a)memcpy(in.coefficients,a->coefTab,sizeof(in.coefficients));
    FhAudioSamplePlan plan,before;memset(&plan,0xa5,sizeof(plan));before=plan;
    assert(fh_audio_sample_measure(&in,&plan)==FH_AUDIO_PLAN_OK);
    assert(plan.first_samples==(s->loopLength?s->loop+s->loopLength:s->length));
    assert(plan.loop_samples==s->loopLength);
    size_t n=plan.storage_samples,rounded=((n+31)*2+31)&~(size_t)31;
    int16_t *pcm=allocate_pcm(rounded);assert(pcm);memset(pcm,0xa5,rounded);
    FhAudioSamplePlan actual=before;
    assert(fh_audio_sample_compile(&in,pcm,n-1,&actual)==FH_AUDIO_PLAN_CAPACITY);
    assert(!memcmp(&actual,&before,sizeof(before)));
    for(size_t i=0;i<rounded;i++)assert(((unsigned char *)pcm)[i]==0xa5);
    assert(fh_audio_sample_compile(&in,pcm,n,&actual)==FH_AUDIO_PLAN_OK);
    assert(!memcmp(&plan,&actual,sizeof(plan)));
    MusyPCMReader reader;assert(salPCInitReader(&reader,s));
    for(uint32_t i=0;i<plan.first_samples;i++) {
        assert(pcm[i]==salPCReadSample(&reader,s,0));compared++;
    }
    for(unsigned repeat=0;repeat<3;repeat++)for(uint32_t i=0;i<plan.loop_samples;i++) {
        assert(pcm[plan.loop_offset+i]==salPCReadSample(&reader,s,0));compared++;
    }
    if(!plan.loop_samples)assert(salPCReadSample(&reader,s,0)==0 && reader.ended);
    for(size_t i=n*2;i<rounded;i++)assert(((unsigned char *)pcm)[i]==0xa5);
    for(uint32_t i=plan.first_samples;i<plan.loop_offset;i++)assert(!pcm[i]);
    if(plan.loop_samples)assert(!(plan.loop_offset&15));
    FhAudioDspSamplePlan dsp;
    assert(fh_audio_dsp_sample_measure(&in,&dsp)==FH_AUDIO_PLAN_OK);
    assert(dsp.first_samples==plan.first_samples && dsp.loop_samples==plan.loop_samples);
    size_t size=(dsp.storage_bytes+63)&~(size_t)31;
    void *data=allocate_pcm(size);assert(data);memset(data,0xa5,size);
    FhAudioDspSamplePlan frozen=dsp;
    assert(fh_audio_dsp_sample_compile(&in,data,dsp.storage_bytes-1,&dsp)==FH_AUDIO_PLAN_CAPACITY);
    assert(!memcmp(&dsp,&frozen,sizeof(dsp)));
    for(size_t i=0;i<size;i++)assert(((unsigned char *)data)[i]==0xa5);
    assert(fh_audio_dsp_sample_compile(&in,data,dsp.storage_bytes,&dsp)==FH_AUDIO_PLAN_OK);
    assert(!memcmp(&dsp,&frozen,sizeof(dsp)));
    if(dsp.format==FH_AUDIO_DSP_PCM16) {
        assert(dsp.storage_bytes==n*2 && !memcmp(data,pcm,n*2));pcm_count++;
    } else {
        assert(dsp.format==FH_AUDIO_DSP_ADPCM);direct_count++;
        DSPADPCMplusInfo info={0};memcpy(info.coefTab,dsp.coefficients,sizeof(info.coefTab));
        info.initialPS=((unsigned char *)data)[0];
        SAMPLE_INFO segment={.addr=data,.length=dsp.first_samples,.extraData=&info,.compType=SAMPLE_TYPE_ADPCM};
        assert(salPCInitReader(&reader,&segment));
        for(uint32_t i=0;i<dsp.first_samples;i++)assert(pcm[i]==salPCReadSample(&reader,&segment,0));
        if(dsp.loop_samples) {
            segment.addr=(unsigned char *)data+dsp.loop_offset_bytes;segment.length=dsp.loop_samples;
            info.initialPS=*(unsigned char *)segment.addr;
            assert(salPCInitReader(&reader,&segment));reader.yn1=dsp.loop_y1;reader.yn2=dsp.loop_y2;
            for(uint32_t i=0;i<dsp.loop_samples;i++)assert(pcm[plan.loop_offset+i]==salPCReadSample(&reader,&segment,0));
        }
    }
    for(size_t i=dsp.storage_bytes;i<size;i++)assert(((unsigned char *)data)[i]==0xa5);
    dsp_bytes+=dsp.storage_bytes;
    if(inventory)fprintf(inventory,"%u,%u,%u,%u,%u,%u,%u\n",bank_id,sample_id,s->length,s->loop,s->loopLength,dsp.format,dsp.storage_bytes);
    release_pcm(data);
    total_bytes+=n*2;if(n*2>max_bytes)max_bytes=n*2;
    cases++;loops+=!!s->loopLength;unaligned_loops+=s->loopLength && s->loop%14;
    release_pcm(pcm);
}
static SND_PC_SPAN load(const char *base,const char *ext) {
    char path[1024];snprintf(path,sizeof(path),"%s.%s",base,ext);
    FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));
    long n=ftell(f);assert(n>0);rewind(f);void *p=malloc(n);assert(p);
    assert(fread(p,1,n,f)==(size_t)n);fclose(f);return (SND_PC_SPAN){p,(size_t)n};
}
static unsigned rng=0x55b119;
static unsigned next(void){rng=rng*1664525u+1013904223u;return rng;}
static void rejection(void) {
    _Alignas(32) unsigned char bytes[96]={0};
    _Alignas(32) int16_t pcm[96];memset(pcm,0xa5,sizeof(pcm));
    FhAudioSampleInput in={.encoded=bytes,.encoded_bytes=8,.length=14};
    FhAudioSamplePlan plan,old;memset(&old,0x19,sizeof(old));plan=old;
    assert(fh_audio_sample_compile(NULL,pcm,96,&plan)==FH_AUDIO_PLAN_INVALID);
    assert(fh_audio_sample_compile(&in,pcm+1,95,&plan)==FH_AUDIO_PLAN_INVALID);
    assert(fh_audio_sample_compile(&in,(int16_t *)bytes,48,&plan)==FH_AUDIO_PLAN_INVALID);
    assert(fh_audio_sample_compile(&in,pcm,96,(FhAudioSamplePlan *)pcm)==FH_AUDIO_PLAN_INVALID);
    in.encoded_bytes=7;assert(fh_audio_sample_compile(&in,pcm,96,&plan)==FH_AUDIO_PLAN_INVALID);
    in.encoded_bytes=8;in.loop_start=15;
    assert(fh_audio_sample_compile(&in,pcm,96,&plan)==FH_AUDIO_PLAN_INVALID);
    in.loop_start=2;in.loop_length=13;
    assert(fh_audio_sample_compile(&in,pcm,96,&plan)==FH_AUDIO_PLAN_INVALID);
    in.loop_start=0;in.loop_length=0;in.length=0;
    assert(fh_audio_sample_compile(&in,pcm,96,&plan)==FH_AUDIO_PLAN_INVALID);
    in.length=UINT32_MAX;in.encoded_bytes=SIZE_MAX;
    assert(fh_audio_sample_compile(&in,pcm,96,&plan)!=FH_AUDIO_PLAN_OK);
    assert(!memcmp(&plan,&old,sizeof(plan)));
    for(size_t i=0;i<sizeof(pcm);i++)assert(((unsigned char *)pcm)[i]==0xa5);
}
int main(int argc,char **argv) {
    assert(argc==4);rejection();
    unsigned char bytes[1024];DSPADPCMplusInfo a={0};
    for(unsigned trial=0;trial<4096;trial++) {
        for(unsigned i=0;i<sizeof(bytes);i++)bytes[i]=next()>>16;
        for(unsigned i=0;i<8;i++)for(unsigned j=0;j<2;j++)a.coefTab[i][j]=next()>>16;
        a.initialPS=next()>>16;a.loopPS=next()>>16;a.loopY1=next()>>16;a.loopY0=next()>>16;
        unsigned length=1+next()%1000,start=next()%length;
        if(trial%5==0){a.initialPS=bytes[0];start=start/14*14;a.loopPS=bytes[(start/14)*8];}
        SAMPLE_INFO s={.addr=bytes,.length=length,.loop=start,
            .loopLength=trial%3?1+next()%(length-start):0,.compType=SAMPLE_TYPE_ADPCM,
            .extraData=&a};
        check(&s,sizeof(bytes));
    }
    printf("synthetic_cases=%u synthetic_samples=%llu\n",cases,(unsigned long long)compared);
    cases=loops=unaligned_loops=0;compared=total_bytes=max_bytes=0;
    dsp_bytes=0;direct_count=pcm_count=0;
    inventory=fopen(argv[3],"wb");assert(inventory);
    fprintf(inventory,"bank,id,length,loop,loop_length,format,storage_bytes\n");
    for(int bank=1;bank<3;bank++) {
        bank_id=bank;
        SND_PC_GROUP_ASSETS input={load(argv[bank],"pro"),load(argv[bank],"poo"),load(argv[bank],"sdi"),load(argv[bank],"sam")};
        MusyPCGroupData g;SND_PC_ASSET_ERROR err;assert(salPCDecodeGroup(&input,&g,&err));
        for(size_t i=0;i<g.sampleCount;i++) {
            SDIR_DATA *e=g.directory+i;SAMPLE_HEADER *h=&e->header;
            sample_id=e->id;
            unsigned type=h->length>>24;assert(type<6);formats[type]++;
            if(type!=SAMPLE_TYPE_ADPCM)continue;
            SAMPLE_INFO s={.addr=(unsigned char *)input.samples.data+e->offset,
                .length=h->length&0xffffff,.loop=h->loopOffset,.loopLength=h->loopLength,
                .compType=type,.extraData=e->extraData?(unsigned char *)g.directory+e->extraData:NULL};
            assert(e->offset<input.samples.size);check(&s,input.samples.size-e->offset);
        }
        salPCFreeGroupData(&g);free((void *)input.project.data);free((void *)input.pool.data);
        free((void *)input.directory.data);free((void *)input.samples.data);
    }
    printf("bank_cases=%u bank_samples=%llu loops=%u unaligned_loops=%u inventory_bytes=%llu largest_bytes=%llu\n",
        cases,(unsigned long long)compared,loops,unaligned_loops,
        (unsigned long long)total_bytes,(unsigned long long)max_bytes);
    for(unsigned i=0;i<6;i++)printf("format%u=%u\n",i,formats[i]);
    printf("direct=%u pcm=%u dsp_inventory_bytes=%llu\n",direct_count,pcm_count,(unsigned long long)dsp_bytes);
    fclose(inventory);
}
