#include "fh_audio_aux_input.h"
#include <limits.h>
#include <string.h>

size_t fh_audio_aux_input_bytes(unsigned channels) {
    return channels==2 || channels==3?(size_t)FH_AUX_INPUT_SLOTS*FH_AUX_INPUT_FRAMES*channels*sizeof(int32_t):0;
}
int fh_audio_aux_input_init(FhAudioAuxInput *q,int32_t *storage,size_t bytes,unsigned channels) {
    size_t needed=fh_audio_aux_input_bytes(channels);
    if(!q || q->samples || !storage || ((uintptr_t)storage&3) || !needed || bytes<needed)return 0;
    memset(q,0,sizeof(*q));q->samples=storage;q->channels=channels;return 1;
}
static int32_t *plane(const FhAudioAuxInput *q,uint32_t serial,unsigned channel) {
    return q->samples+((serial%FH_AUX_INPUT_SLOTS)*q->channels+channel)*FH_AUX_INPUT_FRAMES;
}
int fh_audio_aux_input_push(FhAudioAuxInput *q,uint32_t generation,uint32_t effect,
    uint64_t first,const int32_t *l,const int32_t *r,const int32_t *s) {
    if(!q || !q->samples || !generation || !effect || !l || !r || (q->channels==3 && !s) ||
       first>UINT64_MAX-FH_AUX_INPUT_FRAMES ||
       (q->write_started && (generation<q->generation ||
        (generation==q->generation && first!=q->next_frame))))return FH_AUX_INPUT_INVALID;
    uint32_t head=q->head,tail=__atomic_load_n(&q->tail,__ATOMIC_ACQUIRE);
    if(head-tail>FH_AUX_INPUT_SLOTS)return FH_AUX_INPUT_INVALID;
    if(head-tail==FH_AUX_INPUT_SLOTS)return FH_AUX_INPUT_BUSY;
    const int32_t *input[3]={l,r,s};
    for(unsigned c=0;c<q->channels;c++)memcpy(plane(q,head,c),input[c],FH_AUX_INPUT_FRAMES*sizeof(int32_t));
    q->stamps[head%FH_AUX_INPUT_SLOTS]=(FhAudioAuxInputStamp){first,generation,effect};
    q->next_frame=first+FH_AUX_INPUT_FRAMES;q->generation=generation;q->write_started=1;
    __atomic_store_n(&q->head,head+1,__ATOMIC_RELEASE);return FH_AUX_INPUT_OK;
}
typedef struct { uint32_t tail; unsigned offset,frames; } Range;
static int range_at(const FhAudioAuxInput *q,unsigned ahead,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames,Range *out) {
    if(!q || !q->samples || !generation || !effect || !frames || frames>FH_AUX_INPUT_FRAMES ||
       first>UINT64_MAX-frames || q->read_offset>=FH_AUX_INPUT_FRAMES)return FH_AUX_INPUT_INVALID;
    uint32_t tail=q->tail,head=__atomic_load_n(&q->head,__ATOMIC_ACQUIRE);
    unsigned count=head-tail;
    if(count>FH_AUX_INPUT_SLOTS)return FH_AUX_INPUT_INVALID;
    if(!count)return FH_AUX_INPUT_BUSY;
    uint64_t distance=(uint64_t)q->read_offset+ahead;
    if(distance>=(uint64_t)count*FH_AUX_INPUT_FRAMES ||
       frames>(uint64_t)count*FH_AUX_INPUT_FRAMES-distance)return FH_AUX_INPUT_BUSY;
    uint32_t serial=tail+(unsigned)(distance/FH_AUX_INPUT_FRAMES);
    unsigned offset=(unsigned)(distance%FH_AUX_INPUT_FRAMES),remaining=frames;
    *out=(Range){serial,offset,frames};
    uint64_t at=first;
    while(remaining) {
        const FhAudioAuxInputStamp *stamp=&q->stamps[serial%FH_AUX_INPUT_SLOTS];
        if(stamp->generation!=generation || stamp->effect!=effect || at<offset || stamp->first!=at-offset)
            return FH_AUX_INPUT_INVALID;
        unsigned n=FH_AUX_INPUT_FRAMES-offset;if(n>remaining)n=remaining;
        at+=n;remaining-=n;serial++;offset=0;
    }
    return FH_AUX_INPUT_OK;
}
int fh_audio_aux_input_check(const FhAudioAuxInput *q,unsigned ahead,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames) {
    Range selected;return range_at(q,ahead,generation,effect,first,frames,&selected);
}
int fh_audio_aux_input_ready(const FhAudioAuxInput *q,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames) {
    if(!q || !q->samples || !generation || !effect || !frames || frames>FH_AUX_INPUT_FRAMES ||
       first>UINT64_MAX-frames || q->read_offset>=FH_AUX_INPUT_FRAMES)return FH_AUX_INPUT_INVALID;
    uint32_t tail=q->tail,head=__atomic_load_n(&q->head,__ATOMIC_ACQUIRE);
    unsigned count=head-tail;
    if(count>FH_AUX_INPUT_SLOTS)return FH_AUX_INPUT_INVALID;
    if(!count)return FH_AUX_INPUT_BUSY;
    const FhAudioAuxInputStamp *base=&q->stamps[tail%FH_AUX_INPUT_SLOTS];
    if(generation==base->generation) {
        if(first<base->first || first-base->first<q->read_offset)return FH_AUX_INPUT_INVALID;
        uint64_t ahead=first-base->first-q->read_offset;
        if(ahead>FH_AUX_INPUT_SLOTS*FH_AUX_INPUT_FRAMES)return FH_AUX_INPUT_BUSY;
        return fh_audio_aux_input_check(q,(unsigned)ahead,generation,effect,first,frames);
    }
    /* Only explicit future-range queries search. Ordinary consumption and
     * complete-block admission use the ordered cursor, including resets. */
    for(unsigned i=1;i<count;i++) {
        const FhAudioAuxInputStamp *stamp=&q->stamps[(tail+i)%FH_AUX_INPUT_SLOTS];
        if(stamp->generation==generation && first>=stamp->first && first-stamp->first<FH_AUX_INPUT_FRAMES)
            return fh_audio_aux_input_check(q,i*FH_AUX_INPUT_FRAMES+(unsigned)(first-stamp->first)-q->read_offset,
                generation,effect,first,frames);
    }
    return FH_AUX_INPUT_INVALID;
}
int fh_audio_aux_input_next(const FhAudioAuxInput *q,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames) {
    return fh_audio_aux_input_check(q,0,generation,effect,first,frames);
}
int fh_audio_aux_input_add(FhAudioAuxInput *q,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames,int32_t *l,int32_t *r,int32_t *s) {
    if(!q || !l || !r || (q->channels==3 && !s))return FH_AUX_INPUT_INVALID;
    Range selected;int status=range_at(q,0,generation,effect,first,frames,&selected);
    if(status!=FH_AUX_INPUT_OK)return status;
    if(selected.tail!=q->tail || selected.offset!=q->read_offset)return FH_AUX_INPUT_INVALID;
    int32_t *output[3]={l,r,s};
    /* Preserve every destination on error, including a cross-slot overflow. */
    for(unsigned c=0;c<q->channels;c++)for(unsigned i=0;i<frames;i++) {
        unsigned at=selected.offset+i;
        int64_t sum=(int64_t)output[c][i]+plane(q,selected.tail+at/FH_AUX_INPUT_FRAMES,c)[at%FH_AUX_INPUT_FRAMES];
        if(sum>INT32_MAX || sum<INT32_MIN)return FH_AUX_INPUT_INVALID;
    }
    for(unsigned c=0;c<q->channels;c++)for(unsigned i=0;i<frames;i++) {
        unsigned at=selected.offset+i;
        output[c][i]+=plane(q,selected.tail+at/FH_AUX_INPUT_FRAMES,c)[at%FH_AUX_INPUT_FRAMES];
    }
    unsigned end=selected.offset+frames;
    q->read_offset=end%FH_AUX_INPUT_FRAMES;
    __atomic_store_n(&q->tail,selected.tail+end/FH_AUX_INPUT_FRAMES,__ATOMIC_RELEASE);
    return FH_AUX_INPUT_OK;
}
