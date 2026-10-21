#include "fh_pica_draw_stream.h"
#include <string.h>

int fh_pica_stream_arena_init(FhPicaStreamArena *a,uint32_t *storage,unsigned words){
    if(!a || !storage || ((uintptr_t)storage&3) || words<1)return 0;
    *a=(FhPicaStreamArena){.words=storage,.capacity=words,.used=1};
    storage[0]=0;return 1; /* Offset zero is the empty bucket. */
}
static uint32_t key(const uint32_t *words,unsigned count){
    uint32_t h=2166136261u^count;
    for(unsigned i=0;i<count;i++)h=(h^words[i])*16777619u;
    return h;
}
static unsigned find(const FhPicaStreamArena *a,const uint32_t *w,unsigned n,uint32_t hash){
    for(unsigned off=a->buckets[hash&63];off;off=a->words[off]){
        const uint32_t *p=a->words+off;
        if(p[1]==hash && p[2]==n && !memcmp(p+3,w,n*4))return off;
    }
    return 0;
}
static int compile(FhPicaStreamArena *a,FhPicaResidentDraw *out,
                   const FhPicaDrawDescription *source,unsigned world){
    if(!a || !a->words || !a->used || a->used>a->capacity || !out || !source)return 0;
    FhPicaDrawCommands compiled;
    if(!fh_pica_stream_compile(&compiled,source))return 0;
    FhPicaResidentDraw draw={0};draw.effect_words=world?18:34;
    draw.texture_units=source->texture_units;
    /* Buffer address changes use a two-word patch when layout is retained.
     * The full layout retains all inactive SDK streams, not just bufCount. */
    for(unsigned i=0;i<12;i++){
        draw.offsets[i]=source->buffers.buffers[i].offset;
        if(draw.offsets[i])draw.offset_candidates|=1u<<i;
        unsigned word=i?15+3*i:14;compiled.words[word]=0;
    }
    for(unsigned i=0;i<4;i++)compiled.words[compiled.stages+i*6+4]=0;
    uint32_t world_effect[18];
    if(world){
        const uint32_t *effect=compiled.words+52;
        memcpy(world_effect,effect,8*4);
        world_effect[8]=source->effect.alphaTest;
        world_effect[9]=GPUCMD_HEADER(0,15,GPUREG_FRAGOP_ALPHA_TEST);
        world_effect[10]=source->effect.depthTest;
        world_effect[11]=GPUCMD_HEADER(0,15,GPUREG_FRAGOP_ALPHA_TEST+3);
        memcpy(world_effect+12,effect+14,2*4);
        memcpy(world_effect+14,effect+18,2*4);
        world_effect[16]=source->effect.fragOpMode&0xff00u;
        world_effect[17]=GPUCMD_HEADER(0,2,GPUREG_COLOR_OPERATION);
    }
    const unsigned sizes[5]={12,40,draw.effect_words,24,30};
    const uint32_t *chunks[5]={compiled.words,compiled.words+12,world?world_effect:compiled.words+52,
                              compiled.words+compiled.stages,compiled.words+compiled.count-30};
    uint32_t hashes[5],offsets[5];unsigned required=0;
    for(unsigned i=0;i<5;i++){
        hashes[i]=key(chunks[i],sizes[i]);offsets[i]=find(a,chunks[i],sizes[i],hashes[i]);
        if(!offsets[i])required+=sizes[i]+3;
    }
    if(required>a->capacity-a->used)return 0;
    /* All failures precede arena/output mutation. The five sizes differ, so
     * no two new chunks in this transaction can alias each other. */
    for(unsigned i=0;i<5;i++)if(!offsets[i]){
        unsigned off=a->used,bucket=hashes[i]&63;uint32_t *p=a->words+off;
        p[0]=a->buckets[bucket];p[1]=hashes[i];p[2]=sizes[i];
        memcpy(p+3,chunks[i],sizes[i]*4);a->buckets[bucket]=off;
        offsets[i]=off;a->used+=sizes[i]+3;
    }
    draw.attributes=a->words+offsets[0]+3;draw.layout=a->words+offsets[1]+3;
    draw.effect=a->words+offsets[2]+3;draw.stages=a->words+offsets[3]+3;
    draw.draw=a->words+offsets[4]+3;*out=draw;return 1;
}
int fh_pica_stream_resident_compile(FhPicaStreamArena *a,FhPicaResidentDraw *out,
                                   const FhPicaDrawDescription *source){return compile(a,out,source,0);}
int fh_pica_stream_world_compile(FhPicaStreamArena *a,FhPicaResidentDraw *out,
                                const FhPicaDrawDescription *source){return compile(a,out,source,1);}
void fh_pica_stream_state_reset(FhPicaStreamState *state){
    if(state)memset(state,0,sizeof(*state));
}
static unsigned reg(uint32_t *w,unsigned at,unsigned id,unsigned mask,uint32_t value){
    w[at]=value;w[at+1]=GPUCMD_HEADER(0,mask,id);return at+2;
}
static unsigned copy(uint32_t *dst,unsigned at,const uint32_t *src,unsigned words){
    memcpy(dst+at,src,words*4);return at+words;
}
unsigned fh_pica_stream_resident_append(uint32_t *dst,unsigned capacity,unsigned *cursor,
                                       FhPicaStreamState *state,const FhPicaResidentDraw *d,
                                       const FhPicaDrawLive *live,unsigned flush_textures){
    if(!dst || !cursor || !state || !d || !live || !d->attributes || !d->layout ||
       !d->effect || !d->stages || !d->draw || (*cursor&1) || *cursor>capacity ||
       (live->texture_config&7)!=d->texture_units ||
       ((d->texture_units&1) && !live->samplers[0]) ||
       ((d->texture_units&2) && !live->samplers[1]))return 0;
    unsigned attributes=state->attributes!=d->attributes,layout=state->layout!=d->layout;
    const uint32_t *raster=live->effect?live->effect:d->effect;
    unsigned effect=state->effect!=raster,stages=state->stages!=d->stages;
    unsigned offset_mask=0,color_mask=0,sampler_mask=0;
    for(unsigned bits=state->offset_candidates|d->offset_candidates;bits;bits&=bits-1){
        unsigned i=__builtin_ctz(bits);
        if(state->offsets[i]!=d->offsets[i])offset_mask|=1u<<i;
    }
    for(unsigned i=0;i<4;i++)if(state->colors[i]!=live->colors[i])color_mask|=1u<<i;
    unsigned valid=state->valid;
    for(unsigned i=0;i<2;i++)if((d->texture_units&(1u<<i)) &&
        (!valid || state->samplers[i]!=live->samplers[i]))sampler_mask|=1u<<i;
    unsigned config=live->texture_config&~(1u<<16);
    unsigned texture=!valid || sampler_mask || flush_textures ||
        state->texture_config!=config || state->texture_shadow!=live->texture_shadow;
    uint32_t environment[3]={live->environment_buffer,live->environment_color,live->fog_color};
    unsigned env=!valid || memcmp(state->environment,environment,sizeof environment);
    unsigned uniforms=!valid || memcmp(state->uniforms,live->material_uniforms,sizeof state->uniforms);
    unsigned words=30+attributes*12+effect*d->effect_words+(layout?40:2*__builtin_popcount(offset_mask))+
        (stages?24:2*__builtin_popcount(color_mask))+8*__builtin_popcount(sampler_mask)+texture*6+
        (env?6:0)+(uniforms?16:0);
    if(words>capacity-*cursor)return 0;
    uint32_t *w=dst+*cursor;unsigned at=0;
    if(attributes)at=copy(w,at,d->attributes,12);
    if(layout){
        unsigned base=at;at=copy(w,at,d->layout,40);
        for(unsigned i=0;i<12;i++)w[base+(i?3+3*i:2)]=d->offsets[i];
    }else for(unsigned i=0;i<12;i++)if(offset_mask&(1u<<i))
        at=reg(w,at,GPUREG_ATTRIBBUFFER0_OFFSET+3*i,15,d->offsets[i]);
    if(effect)at=copy(w,at,raster,d->effect_words);
    for(unsigned i=0;i<2;i++)if(sampler_mask&(1u<<i))at=copy(w,at,live->samplers[i]->words,8);
    if(texture){
        at=reg(w,at,GPUREG_TEXUNIT_CONFIG,11,config|(1u<<16));
        at=reg(w,at,GPUREG_TEXUNIT_CONFIG,4,1u<<16);
        at=reg(w,at,GPUREG_TEXUNIT0_SHADOW,15,live->texture_shadow);
    }
    if(env){
        at=reg(w,at,GPUREG_TEXENV_UPDATE_BUFFER,7,environment[0]);
        at=reg(w,at,GPUREG_TEXENV_BUFFER_COLOR,15,environment[1]);
        at=reg(w,at,GPUREG_FOG_COLOR,15,environment[2]);
    }
    if(stages){
        unsigned base=at;at=copy(w,at,d->stages,24);
        for(unsigned i=0;i<4;i++)w[base+i*6+4]=live->colors[i];
    }else for(unsigned i=0;i<4;i++)if(color_mask&(1u<<i))
        at=reg(w,at,GPUREG_TEXENV0_COLOR+i*8,15,live->colors[i]);
    if(uniforms){
        at=reg(w,at,GPUREG_VSH_FLOATUNIFORM_CONFIG,15,0x80000007u);
        w[at++]=live->material_uniforms[0];
        w[at++]=GPUCMD_HEADER(0,15,GPUREG_VSH_FLOATUNIFORM_DATA)|(11u<<20);
        memcpy(w+at,live->material_uniforms+1,11*4);at+=11;w[at++]=0;
    }
    at=copy(w,at,d->draw,30);
    state->attributes=d->attributes;state->layout=d->layout;state->effect=raster;state->stages=d->stages;
    for(unsigned i=0;i<2;i++)if(d->texture_units&(1u<<i))state->samplers[i]=live->samplers[i];
    for(unsigned bits=offset_mask;bits;bits&=bits-1){unsigned i=__builtin_ctz(bits);state->offsets[i]=d->offsets[i];}
    state->offset_candidates=d->offset_candidates;
    memcpy(state->colors,live->colors,sizeof state->colors);
    if(uniforms)memcpy(state->uniforms,live->material_uniforms,sizeof state->uniforms);
    if(env)memcpy(state->environment,environment,sizeof environment);
    state->texture_config=config;state->texture_shadow=live->texture_shadow;state->valid=1;
    *cursor+=at;return at;
}
