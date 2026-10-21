#include "fh_audio_timeline.h"
#include <string.h>

int fh_audio_timeline_bind(FhAudioTimeline *t,const int16_t *const buffers[FH_AUDIO_MAP_BUFFERS]) {
    if(!t || !buffers)return 0;
    for(unsigned i=0;i<FH_AUDIO_MAP_BUFFERS;i++) {
        if(!buffers[i])return 0;
        for(unsigned j=0;j<i;j++)if(buffers[i]==buffers[j])return 0;
    }
    memset(t,0,sizeof(*t));
    for(unsigned i=0;i<FH_AUDIO_MAP_BUFFERS;i++)t->buffers[i].address=buffers[i];
    return 1;
}
int fh_audio_timeline_stamp(FhAudioTimeline *t,const int16_t *address,unsigned frames,
    uint64_t source_frame,unsigned generation) {
    return fh_audio_timeline_stamp_effect(t,address,frames,source_frame,generation,0);
}
int fh_audio_timeline_stamp_effect(FhAudioTimeline *t,const int16_t *address,unsigned frames,
    uint64_t source_frame,unsigned generation,uint32_t effect_token) {
    if(!t || !address || !frames || frames>UINT16_MAX || frames>UINT64_MAX-source_frame ||
       (!generation && effect_token))return 0;
    for(unsigned i=0;i<FH_AUDIO_MAP_BUFFERS;i++)if(t->buffers[i].address==address) {
        t->buffers[i].span=(FhAudioSpan){source_frame,generation,0,(uint16_t)frames,effect_token};return 1;
    }
    return 0;
}
FhAudioSpan fh_audio_timeline_read(const FhAudioTimeline *t,const int16_t *address,unsigned frames) {
    if(t && address && frames && frames<=UINT16_MAX)
        for(unsigned i=0;i<FH_AUDIO_MAP_BUFFERS;i++)if(t->buffers[i].address==address) {
            FhAudioSpan s=t->buffers[i].span;
            if(s.frames==frames)return s;
            break;
        }
    return (FhAudioSpan){0};
}
int fh_audio_timeline_copy(FhAudioTimeline *t,const int16_t *destination,const int16_t *source,unsigned frames) {
    FhAudioSpan s=fh_audio_timeline_read(t,source,frames);
    return fh_audio_timeline_stamp_effect(t,destination,frames,s.source_frame,s.generation,s.effect_token);
}
int fh_audio_map_append(FhAudioMap *m,FhAudioSpan s,unsigned offset,unsigned frames) {
    if(!m || m->count>=FH_AUDIO_MAP_SPANS || !frames || offset>UINT16_MAX ||
       frames>UINT16_MAX-offset || (!s.generation && s.effect_token) ||
       (s.generation && (s.frames!=frames || frames>UINT64_MAX-s.source_frame)))return 0;
    unsigned end=m->count?m->spans[m->count-1].offset+m->spans[m->count-1].frames:0;
    if(offset!=end)return 0;
    s.offset=(uint16_t)offset;s.frames=(uint16_t)frames;
    if(!s.generation)s.source_frame=0;
    m->spans[m->count++]=s;return 1;
}
int fh_audio_map_valid(const FhAudioMap *m,unsigned frames) {
    if(!m)return 1; /* Existing streaming/PCM callers have no source timeline. */
    if(!frames || frames>UINT16_MAX || !m->count || m->count>FH_AUDIO_MAP_SPANS)return 0;
    unsigned end=0;
    for(unsigned i=0;i<m->count;i++) {
        const FhAudioSpan *s=&m->spans[i];
        if(s->offset!=end || !s->frames || s->frames>frames-end ||
           (!s->generation && s->effect_token) ||
           (s->generation && s->frames>UINT64_MAX-s->source_frame))return 0;
        end+=s->frames;
    }
    return end==frames;
}
FhAudioSourceCursor fh_audio_map_cursor(const FhAudioMap *m,unsigned offset,unsigned available) {
    if(m && available && m->count<=FH_AUDIO_MAP_SPANS)
        for(unsigned i=0;i<m->count;i++) {
            const FhAudioSpan *s=&m->spans[i];
            if(s->generation && offset>=s->offset && offset-s->offset<s->frames)
                return (FhAudioSourceCursor){s->source_frame+offset-s->offset,s->generation,1};
        }
    return (FhAudioSourceCursor){0};
}
static int window_add(FhAudioSourceWindow *w,FhAudioSpan s) {
    if(w->count>FH_AUDIO_MAP_SPANS)return 0;
    FhAudioSpan sorted[FH_AUDIO_MAP_SPANS+1];unsigned count=0,inserted=0;
    for(unsigned i=0;i<w->count;i++) {
        if(!inserted && s.offset<w->spans[i].offset){sorted[count++]=s;inserted=1;}
        sorted[count++]=w->spans[i];
    }
    if(!inserted)sorted[count++]=s;
    unsigned result=0;
    for(unsigned i=0;i<count;i++) {
        if(result) {
            FhAudioSpan *prior=&sorted[result-1];unsigned end=prior->offset+prior->frames;
            if(sorted[i].offset<end)return 0;
            if(sorted[i].offset==end && prior->generation==sorted[i].generation &&
               prior->effect_token==sorted[i].effect_token &&
               (!prior->generation || prior->source_frame+prior->frames==sorted[i].source_frame)) {
                prior->frames+=sorted[i].frames;continue;
            }
        }
        sorted[result++]=sorted[i];
    }
    if(result>FH_AUDIO_MAP_SPANS)return 0;
    memcpy(w->spans,sorted,result*sizeof(*sorted));w->count=result;return 1;
}
int fh_audio_source_window_append(FhAudioSourceWindow *w,const FhAudioMap *map,
    uint64_t packet_first,unsigned packet_frames,uint64_t first,uint64_t end) {
    if(!w || !packet_frames || packet_frames>UINT16_MAX || packet_frames>UINT64_MAX-packet_first ||
       end<=first || end-first>UINT16_MAX || (map && map->count && !fh_audio_map_valid(map,packet_frames)))return 0;
    if(w->count && (w->input_first!=first || w->frames!=end-first))return 0;
    w->valid=0;w->input_first=first;w->frames=(unsigned)(end-first);
    if(packet_first>=end || packet_first+packet_frames<=first)return 1;
    FhAudioMap unknown={.spans={{.frames=(uint16_t)packet_frames}},.count=1};
    if(!map || !map->count)map=&unknown;
    for(unsigned i=0;i<map->count;i++) {
        FhAudioSpan s=map->spans[i];uint64_t a=packet_first+s.offset,b=a+s.frames;
        uint64_t lo=a>first?a:first,hi=b<end?b:end;
        if(lo>=hi)continue;
        if(s.generation)s.source_frame+=lo-a;else s.source_frame=0;
        s.offset=(uint16_t)(lo-first);s.frames=(uint16_t)(hi-lo);
        if(!window_add(w,s))return 0;
    }
    return 1;
}
int fh_audio_source_window_finish(FhAudioSourceWindow *w,uint64_t first,uint64_t end) {
    if(!w)return 0;
    w->valid=0;
    if(w->input_first!=first || end<=first || end-first!=w->frames ||
       !w->count || w->count>FH_AUDIO_MAP_SPANS)return 0;
    unsigned offset=0;
    for(unsigned i=0;i<w->count;i++) {
        if(w->spans[i].offset!=offset || !w->spans[i].frames)return 0;
        offset+=w->spans[i].frames;
    }
    return w->valid=offset==w->frames;
}
