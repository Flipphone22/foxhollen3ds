#ifndef FH_WORLD_INDEX_STREAM_H
#define FH_WORLD_INDEX_STREAM_H
#include "fh_world_bounds.h"
/* Append only; caller retires every GPU reader before resetting used. Each
 * publication owns whole 128-byte cache lines. No allocation or implicit wrap. */
typedef struct {uint16_t *data;unsigned capacity,used,peak,published;} FhWorldIndexStream;
/* GPU readers are admitted at command-list publication, not DrawElements.
 * Clean the appended prefix once at that boundary. End on a whole owned line;
 * the next append also starts on a new line, never mutating GPU-visible bytes.
 * Only the rendering owner calls this function, append and retirement. */
static inline unsigned fh_world_index_publish(FhWorldIndexStream *s,
    void (*flush)(const void*,unsigned)){
    if(!s->data || s->used>s->capacity || s->published>s->capacity ||
       s->used<=s->published || s->used>UINT32_MAX-63)return 0;
    unsigned end=(s->used+63)&~63u;
    if(end>s->capacity)end=s->capacity;
    unsigned bytes=(end-s->published)*sizeof(uint16_t);
    flush(s->data+s->published,bytes);
    s->published=end;
    return bytes;
}
static inline const uint16_t *fh_world_index_append(FhWorldIndexStream *s,
    const uint16_t *source,unsigned total,const FhWorldRange *runs,unsigned n,
    unsigned *written){
    *written=0;
    if(!s->data || !source || !runs || !n || s->used>s->capacity)return NULL;
    unsigned count=0,previous=0;
    for(unsigned i=0;i<n;i++){
        unsigned first=runs[i].first,size=runs[i].count;
        if(first<previous || first>total || size>total-first || first%3 || size%3)return NULL;
        previous=first+size;
        if(size>s->capacity-count)return NULL;
        count+=size;
    }
    if(!count || s->used>UINT32_MAX-63)return NULL;
    unsigned at=(s->used+63)&~63u;
    if(at>s->capacity || count>s->capacity-at)return NULL;
    unsigned cursor=at;
    for(unsigned i=0;i<n;i++){
        memcpy(s->data+cursor,source+runs[i].first,runs[i].count*sizeof(uint16_t));
        cursor+=runs[i].count;
    }
    s->used=cursor;if(cursor>s->peak)s->peak=cursor;
    *written=count;return s->data+at;
}
#endif
