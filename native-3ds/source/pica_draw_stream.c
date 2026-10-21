#include "fh_pica_draw_stream.h"
#include <string.h>

/* Wire format follows pinned Citro3D attribs/buffers/effect/texenv/drawElements
 * and libctru GPUCMD_Add. Keep all draw side effects, including both primitive
 * cache writes. Adapted from pinned Melee's checked command publication; unlike
 * its per-draw packet this compiles the combined native state at admission.
 * This file alone is not the SDK/queue owner and is not game-integrated yet. */
typedef uint32_t alias_word __attribute__((may_alias));
static unsigned write_words(uint32_t *out,unsigned at,unsigned reg,unsigned mask,
                           unsigned incremental,const uint32_t *data,unsigned n){
    out[at++]=data[0];
    out[at++]=GPUCMD_HEADER(incremental,mask,reg)|((n-1)<<20);
    for(unsigned i=1;i<n;i++)out[at++]=data[i];
    if(at&1)out[at++]=0;
    return at;
}
static unsigned one(uint32_t *out,unsigned at,unsigned reg,unsigned mask,uint32_t data){
    return write_words(out,at,reg,mask,0,&data,1);
}
int fh_pica_stream_sampler(FhPicaSamplerCommands *out,unsigned unit,
                          const FhPicaStreamSampler *s){
    if(!out || !s || unit>1 || (s->address&7) || !s->address ||
       ((s->param>>28)&7)!=GPU_TEX_2D)return 0;
    const unsigned reg[2]={GPUREG_TEXUNIT0_BORDER_COLOR,GPUREG_TEXUNIT1_BORDER_COLOR};
    const unsigned fmt[2]={GPUREG_TEXUNIT0_TYPE,GPUREG_TEXUNIT1_TYPE};
    uint32_t values[5]={s->border,s->dim,s->param,s->lod,s->address>>3};
    unsigned n=write_words(out->words,0,reg[unit],15,1,values,5);
    one(out->words,n,fmt[unit],15,s->format);
    return 1;
}
int fh_pica_stream_compile(FhPicaDrawCommands *out,const FhPicaDrawDescription *s){
    if(!out || !s || s->attributes.attrCount<=0 || s->attributes.attrCount>12 ||
       s->buffers.bufCount<=0 || s->buffers.bufCount>12 ||
       (s->buffers.base_paddr&7) || s->index_address<s->buffers.base_paddr ||
       (s->index_address&1) || s->index_address-s->buffers.base_paddr>=0x80000000u ||
       !s->index_count || s->index_count>0x7fffffffu || (s->texture_units&~3u))return 0;
    _Static_assert(sizeof(C3D_TexEnv)==20,"PICA stage ABI");
    _Static_assert(sizeof(s->buffers.buffers)==144,"PICA buffer ABI");
    FhPicaDrawCommands p={0};uint32_t *w=p.words;unsigned n=0;
#define W(reg,mask,value) n=one(w,n,reg,mask,value)
#define WS(reg,ptr,count) n=write_words(w,n,reg,15,1,(const alias_word *)(const void *)(ptr),count)
    WS(GPUREG_ATTRIBBUFFERS_FORMAT_LOW,s->attributes.flags,2);
    W(GPUREG_VSH_INPUTBUFFER_CONFIG,11,0xa0000000u|(s->attributes.attrCount-1));
    W(GPUREG_VSH_NUM_ATTR,15,s->attributes.attrCount-1);
    WS(GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW,&s->attributes.permutation,2);
    W(GPUREG_ATTRIBBUFFERS_LOC,15,s->buffers.base_paddr>>3);
    WS(GPUREG_ATTRIBBUFFER0_OFFSET,s->buffers.buffers,36);
    const FhPicaStreamEffect *e=&s->effect;
    W(GPUREG_DEPTHMAP_ENABLE,15,e->zBuffer?1:0);
    W(GPUREG_FACECULLING_CONFIG,15,e->cullMode&3);
    WS(GPUREG_DEPTHMAP_SCALE,&e->zScale,2);
    WS(GPUREG_FRAGOP_ALPHA_TEST,&e->alphaTest,4);
    W(GPUREG_GAS_DELTAZ_DEPTH,8,(uint32_t)GPU_MAKEGASDEPTHFUNC((e->depthTest>>4)&7)<<24);
    W(GPUREG_BLEND_COLOR,15,e->blendClr);
    W(GPUREG_BLEND_FUNC,15,e->alphaBlend);
    W(GPUREG_LOGIC_OP,15,e->clrLogicOp);
    W(GPUREG_COLOR_OPERATION,7,e->fragOpMode);
    W(GPUREG_FRAGOP_SHADOW,15,e->fragOpShadow);
    W(GPUREG_EARLYDEPTH_TEST1,1,e->earlyDepth?1:0);
    W(GPUREG_EARLYDEPTH_TEST2,15,e->earlyDepth?1:0);
    W(GPUREG_EARLYDEPTH_FUNC,1,e->earlyDepthFunc);
    W(GPUREG_EARLYDEPTH_DATA,7,e->earlyDepthRef);
    for(unsigned i=0;i<2;i++)if(s->texture_units&(1u<<i)){
        p.sampler[i]=(uint16_t)n;n+=8; /* Replaced by a prepared sampler. */
    }
    p.texture=(uint16_t)n;
    W(GPUREG_TEXUNIT_CONFIG,11,0);
    W(GPUREG_TEXUNIT_CONFIG,4,1u<<16);
    W(GPUREG_TEXUNIT0_SHADOW,15,0);
    p.environment=(uint16_t)n;
    W(GPUREG_TEXENV_UPDATE_BUFFER,7,0);
    W(GPUREG_TEXENV_BUFFER_COLOR,15,0);
    W(GPUREG_FOG_COLOR,15,0);
    p.stages=(uint16_t)n;
    for(unsigned i=0;i<4;i++)WS(GPUREG_TEXENV0_SOURCE+i*8,s->stages+i,5);
    W(GPUREG_VSH_FLOATUNIFORM_CONFIG,15,0x80000007u);
    p.uniforms=(uint16_t)n;
    uint32_t uniforms[12]={0};
    n=write_words(w,n,GPUREG_VSH_FLOATUNIFORM_DATA,15,0,uniforms,12);
    W(GPUREG_PRIMITIVE_CONFIG,2,GPU_GEOMETRY_PRIM);
    W(GPUREG_RESTART_PRIMITIVE,15,1);
    W(GPUREG_INDEXBUFFER_CONFIG,15,(s->index_address-s->buffers.base_paddr)|0x80000000u);
    W(GPUREG_NUMVERTICES,15,s->index_count);
    W(GPUREG_VERTEX_OFFSET,15,0);
    W(GPUREG_GEOSTAGE_CONFIG,2,0x100);
    W(GPUREG_GEOSTAGE_CONFIG2,2,0x100);
    W(GPUREG_START_DRAW_FUNC0,1,0);
    W(GPUREG_DRAWELEMENTS,15,1);
    W(GPUREG_START_DRAW_FUNC0,1,1);
    W(GPUREG_GEOSTAGE_CONFIG,2,0);
    W(GPUREG_GEOSTAGE_CONFIG2,2,0);
    W(GPUREG_VTX_FUNC,15,1);
    W(GPUREG_PRIMITIVE_CONFIG,8,0);
    W(GPUREG_PRIMITIVE_CONFIG,8,0);
#undef WS
#undef W
    /* Fixed format: 168..184 words, not unbounded caller data. */
    if(n>FH_PICA_DRAW_STREAM_WORDS)return 0;
    p.count=(uint16_t)n;p.texture_units=(uint16_t)s->texture_units;*out=p;
    return 1;
}
unsigned fh_pica_stream_append(uint32_t *dst,unsigned capacity,unsigned *cursor,
                              const FhPicaDrawCommands *p,const FhPicaDrawLive *live){
    if(!dst || !cursor || !p || !live || !p->count ||
       (*cursor&1) || *cursor>capacity || p->count>capacity-*cursor ||
       (live->texture_config&7)!=p->texture_units ||
       ((p->texture_units&1) && !live->samplers[0]) ||
       ((p->texture_units&2) && !live->samplers[1]))return 0;
    uint32_t *w=dst+*cursor;
    memcpy(w,p->words,p->count*sizeof(*w));
    for(unsigned i=0;i<2;i++)if(p->texture_units&(1u<<i))
        memcpy(w+p->sampler[i],live->samplers[i]->words,8*sizeof(*w));
    w[p->texture]=live->texture_config|(1u<<16);
    w[p->texture+4]=live->texture_shadow;
    w[p->environment]=live->environment_buffer;
    w[p->environment+2]=live->environment_color;
    w[p->environment+4]=live->fog_color;
    for(unsigned i=0;i<4;i++)w[p->stages+i*6+4]=live->colors[i];
    w[p->uniforms]=live->material_uniforms[0];
    memcpy(w+p->uniforms+2,live->material_uniforms+1,11*sizeof(*w));
    *cursor+=p->count;return p->count;
}
