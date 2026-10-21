#ifndef FH_TEXGEN_CACHE_H
#define FH_TEXGEN_CACHE_H
#include "fh_hash_bytes.h"
/* CPU result reuse across fur shells. Own arithmetic inputs, never array
 * pointers or write epochs. Pinned Melee's geometry-key separation applies
 * here to one generated coordinate rather than an entire moving mesh. */
typedef struct {float matrix[3][4],post[3][4];unsigned flags,fpscr;} FhTexgenState;
typedef struct {FhTexgenState key;unsigned id,stamp;} FhTexgenStateEntry;
typedef struct {float input[3],output[3];unsigned id,hash;} FhTexgenValue;
enum { TEXGEN_STATES=16,TEXGEN_VALUES=2048 };
static FhTexgenStateEntry texgen_states[TEXGEN_STATES];
static FhTexgenValue texgen_values[TEXGEN_VALUES];
static unsigned texgen_serial,texgen_clock;
unsigned fh_texgen_cache_disabled,fh_texgen_cache_hits,fh_texgen_cache_misses;
const unsigned fh_texgen_cache_bytes=sizeof(texgen_states)+sizeof(texgen_values);
_Static_assert(sizeof(texgen_states)+sizeof(texgen_values)<=68*1024,"CPU texgen cache budget");
static unsigned texgen_state(const float matrix[3][4],const float post[3][4],unsigned q_one,unsigned normalized){
    if(fh_texgen_cache_disabled)return 0;
    if(texgen_serial>0xffff0000u || texgen_clock>0xffff0000u){
        memset(texgen_states,0,sizeof(texgen_states));memset(texgen_values,0,sizeof(texgen_values));
        texgen_serial=texgen_clock=0;
    }
    FhTexgenState key={.flags=!!matrix|((!!post)<<1)|(!!q_one<<2)|(!!normalized<<3)};
    if(matrix)fh_copy_bytes(key.matrix,matrix,sizeof(key.matrix));
    if(post)fh_copy_bytes(key.post,post,sizeof(key.post));
#ifdef __arm__
    __asm__ volatile("vmrs %0, fpscr":"=r"(key.fpscr));key.fpscr&=0x03c00000u;
#endif
    FhTexgenStateEntry *oldest=texgen_states;
    for(unsigned i=0;i<TEXGEN_STATES;i++){
        FhTexgenStateEntry *s=texgen_states+i;
        if(s->id && !fh_memory_compare(&s->key,&key,sizeof(key))){s->stamp=++texgen_clock;return s->id;}
        if(s->stamp<oldest->stamp)oldest=s;
    }
    oldest->key=key;oldest->id=++texgen_serial;oldest->stamp=++texgen_clock;return oldest->id;
}
static FhTexgenValue *texgen_probe(unsigned id,const float source[3],unsigned *hash){
    *hash=fh_hash_bytes(id,source,12);unsigned bucket=*hash^(*hash>>16);
    bucket*=0x85ebca6bu;bucket^=bucket>>13;
    return texgen_values+(bucket&(TEXGEN_VALUES-1));
}
#endif
