#include "fh_pica_actor_commands.h"
#include <string.h>

int fh_pica_actor_geometry_compile(FhPicaStreamArena *a,FhPicaActorGeometry *out,const FhPicaDrawDescription *s){
    if(!out || !s)return 0;
    FhPicaResidentDraw full;
    if(!fh_pica_stream_resident_compile(a,&full,s))return 0;
    FhPicaActorGeometry g={.attributes=full.attributes,.layout=full.layout,.draw=full.draw,.cull=s->effect.cullMode&3};
    memcpy(g.offsets,full.offsets,sizeof g.offsets);g.offset_candidates=full.offset_candidates;*out=g;return 1;
}
void fh_pica_actor_material_compile(FhPicaActorMaterial *out,const C3D_TexEnv stages[4],unsigned mask){
    if(!out || !stages)return;
    for(unsigned i=0;i<4;i++){
        uint32_t values[5];memcpy(values,stages+i,sizeof values);
        uint32_t *w=out->words+6*i;
        w[0]=values[0];w[1]=GPUCMD_HEADER(1,15,GPUREG_TEXENV0_SOURCE+i*8)|(4u<<20);
        w[2]=values[1];w[3]=values[2];w[4]=0;w[5]=values[4];
    }
    out->buffer_mask=mask&255;
}
void fh_pica_actor_command_reset(FhPicaActorCommandState *s){if(s)memset(s,0,sizeof *s);}
static unsigned reg(uint32_t *w,unsigned at,unsigned id,unsigned mask,uint32_t value){
    w[at]=value;w[at+1]=GPUCMD_HEADER(0,mask,id);return at+2;
}
static unsigned copy(uint32_t *w,unsigned at,const uint32_t *src,unsigned count){
    memcpy(w+at,src,count*4);return at+count;
}
static void dirty(FhPicaActorCommandState *s,unsigned first,unsigned rows){
    while(rows){
        unsigned offset=first&31, count=32-offset;
        if(count>rows)count=rows;
        uint32_t mask=(UINT32_MAX>>(32-count))<<offset;
        s->uniform_dirty[first>>5]|=mask;
        first+=count;rows-=count;
    }
}
static unsigned uniform_header(uint32_t *w,unsigned at,unsigned row,unsigned words){
    at=reg(w,at,GPUREG_VSH_FLOATUNIFORM_CONFIG,15,0x80000000u|row);
    /* Caller fills data0 at at; header is followed by data1..N. */
    w[at+1]=GPUCMD_HEADER(0,15,GPUREG_VSH_FLOATUNIFORM_DATA)|((words-1)<<20);
    return at;
}
static unsigned uniform(uint32_t *w,unsigned at,unsigned row,const uint32_t *values,unsigned words){
    at=uniform_header(w,at,row,words);w[at]=values[0];
    memcpy(w+at+2,values+1,(words-1)*4);w[at+words+1]=0;return at+words+2;
}
static inline unsigned actor_words_equal3(const uint32_t *a,const uint32_t *b){return !((a[0]^b[0]) | (a[1]^b[1]) | (a[2]^b[2]));}
static inline unsigned actor_words_equal4(const uint32_t *a,const uint32_t *b){return !((a[0]^b[0]) | (a[1]^b[1]) | (a[2]^b[2]) | (a[3]^b[3]));}
static inline unsigned actor_words_equal12(const uint32_t *a,const uint32_t *b){return !((a[0]^b[0]) | (a[1]^b[1]) | (a[2]^b[2]) | (a[3]^b[3]) | (a[4]^b[4]) | (a[5]^b[5]) | (a[6]^b[6]) | (a[7]^b[7]) | (a[8]^b[8]) | (a[9]^b[9]) | (a[10]^b[10]) | (a[11]^b[11]));}
static unsigned pose_run(unsigned mask,unsigned first){
    unsigned bits=(mask>>first)&((1u<<(22-first))-1);
    unsigned n=__builtin_ctz(~bits);
    return n>21?21:n; /* 21 bones =252 float words; PICA packet count is eight bits. */
}
unsigned fh_pica_actor_command_append(uint32_t *dst,unsigned capacity,unsigned *cursor,
    FhPicaActorCommandState *s,const FhPicaActorGeometry *g,const FhPicaActorParameters *p,
    const FhPicaActorPose *pose,unsigned refresh){
    if(!dst || !cursor || !s || !g || !p || !pose || !g->attributes || !g->layout || !g->draw || !p->material ||
       (*cursor&1) || *cursor>capacity || !pose->joints || !pose->world || !pose->tails || !pose->bones ||
       !pose->count || pose->count>22 || pose->joint_count>256 || (pose->changed>>pose->count))return 0;
    unsigned units=p->texture_config&7;
    for(unsigned i=0;i<3;i++)if((units&(1u<<i)) && !p->samplers[i])return 0;
    unsigned changed=s->valid?pose->changed:(1u<<pose->count)-1;
    unsigned palette_words=0;
    for(unsigned remaining=changed;remaining;){
        unsigned first=__builtin_ctz(remaining),count=pose_run(remaining,first);
        for(unsigned i=first;i<first+count;i++)if(pose->bones[i]>pose->joint_count)return 0;
        palette_words+=4+count*12;remaining&=~(((1u<<count)-1)<<first);
    }
    const uint32_t *offsets=p->offsets?p->offsets:g->offsets;
    unsigned attrs=s->attributes!=g->attributes,layout=s->layout!=g->layout,material=s->material!=p->material;
    unsigned offset_mask=0,color_mask=0,sampler_mask=0;
    /* Load-compiled membership covers immutable geometry; live morph
     * fetch overrides conservatively cover every register, including zeros. */
    unsigned candidates=p->offsets?0xfffu:g->offset_candidates;
    for(unsigned bits=s->offset_candidates|candidates;bits;bits&=bits-1){
        unsigned i=__builtin_ctz(bits);
        if(s->offsets[i]!=offsets[i])offset_mask|=1u<<i;
    }
    for(unsigned i=0;i<4;i++)if(s->colors[i]!=p->colors[i])color_mask|=1u<<i;
    for(unsigned i=0;i<3;i++)if((units&(1u<<i)) && (!s->valid || s->samplers[i]!=p->samplers[i]))sampler_mask|=1u<<i;
    uint32_t raster[4]={g->cull,p->alpha_test,p->depth_test,p->blend};
    unsigned raster_changed=!s->valid || !actor_words_equal4(s->raster,raster);
    uint32_t env[3]={(p->environment_buffer&~0xff00u)|(p->material->buffer_mask<<8),p->environment_color,p->fog_color};
    unsigned environment=!s->valid || !actor_words_equal3(s->environment,env);
    unsigned texture=!s->valid || sampler_mask || refresh || s->texture_config!=(p->texture_config&~(1u<<16)) || s->texture_shadow!=p->texture_shadow;
    unsigned uv=!s->valid || !actor_words_equal12(s->uniforms,p->uniforms);
    unsigned scale=!s->valid || !actor_words_equal4(s->scale,p->scale);
    unsigned words=30+attrs*12+(layout?40:2*__builtin_popcount(offset_mask))+raster_changed*12+
        8*__builtin_popcount(sampler_mask)+texture*6+environment*6+(material?24:2*__builtin_popcount(color_mask))+
        uv*16+scale*8+palette_words;
    if(words>capacity-*cursor)return 0;
    uint32_t *w=dst+*cursor;unsigned at=0;
    if(attrs)at=copy(w,at,g->attributes,12);
    if(layout){unsigned base=at;at=copy(w,at,g->layout,40);for(unsigned i=0;i<12;i++)w[base+(i?3+3*i:2)]=offsets[i];}
    else for(unsigned i=0;i<12;i++)if(offset_mask&(1u<<i))at=reg(w,at,GPUREG_ATTRIBBUFFER0_OFFSET+3*i,15,offsets[i]);
    if(raster_changed){
        at=reg(w,at,GPUREG_FACECULLING_CONFIG,15,g->cull);
        at=reg(w,at,GPUREG_FRAGOP_ALPHA_TEST,15,p->alpha_test);
        at=reg(w,at,GPUREG_FRAGOP_ALPHA_TEST+3,15,p->depth_test);
        at=reg(w,at,GPUREG_GAS_DELTAZ_DEPTH,8,(uint32_t)GPU_MAKEGASDEPTHFUNC((p->depth_test>>4)&7)<<24);
        at=reg(w,at,GPUREG_BLEND_FUNC,15,p->blend);
        at=reg(w,at,GPUREG_COLOR_OPERATION,2,0x100);
    }
    static const unsigned sampler_reg[3]={GPUREG_TEXUNIT0_BORDER_COLOR,GPUREG_TEXUNIT1_BORDER_COLOR,GPUREG_TEXUNIT2_BORDER_COLOR};
    static const unsigned sampler_fmt[3]={GPUREG_TEXUNIT0_TYPE,GPUREG_TEXUNIT1_TYPE,GPUREG_TEXUNIT2_TYPE};
    for(unsigned i=0;i<3;i++)if(sampler_mask&(1u<<i)){
        unsigned base=at;at=copy(w,at,p->samplers[i]->words,8);
        w[base+1]=GPUCMD_HEADER(1,15,sampler_reg[i])|(4u<<20);w[base+7]=GPUCMD_HEADER(0,15,sampler_fmt[i]);
    }
    if(texture){
        at=reg(w,at,GPUREG_TEXUNIT_CONFIG,11,p->texture_config|(1u<<16));
        at=reg(w,at,GPUREG_TEXUNIT_CONFIG,4,1u<<16);
        at=reg(w,at,GPUREG_TEXUNIT0_SHADOW,15,p->texture_shadow);
    }
    if(environment){
        at=reg(w,at,GPUREG_TEXENV_UPDATE_BUFFER,7,env[0]);
        at=reg(w,at,GPUREG_TEXENV_BUFFER_COLOR,15,env[1]);at=reg(w,at,GPUREG_FOG_COLOR,15,env[2]);
    }
    if(material){unsigned base=at;at=copy(w,at,p->material->words,24);for(unsigned i=0;i<4;i++)w[base+6*i+4]=p->colors[i];}
    else for(unsigned i=0;i<4;i++)if(color_mask&(1u<<i))at=reg(w,at,GPUREG_TEXENV0_COLOR+i*8,15,p->colors[i]);
    if(uv){at=uniform(w,at,7,p->uniforms,12);dirty(s,7,3);}
    if(scale){at=uniform(w,at,76,p->scale,4);dirty(s,76,1);}
    for(unsigned remaining=changed;remaining;){
        unsigned first=__builtin_ctz(remaining),count=pose_run(remaining,first);
        unsigned base=uniform_header(w,at,10+first*3,count*12);
        uint32_t header=w[base+1],*output=w+base+1;
        /* This command range is CPU-owned and unpublished. Fill contiguous
         * rows, then move data0 before the saved SDK header. Avoid a header
         * gap test and temporary row array for each of the 12 bone words. */
        for(unsigned i=first;i<first+count;i++){
            unsigned bone=pose->bones[i];const float *joint=bone<pose->joint_count?pose->joints+bone*16:pose->world;
            const float *tail=pose->tails+bone*3;
            {
                const float *j=joint+0;
                float translation=j[3]-j[0]*tail[0]-j[1]*tail[1]-j[2]*tail[2];
                memcpy(output,&translation,4);memcpy(output+1,j+2,4);
                memcpy(output+2,j+1,4);memcpy(output+3,j,4);output+=4;
            }
            {
                const float *j=joint+4;
                float translation=j[3]-j[0]*tail[0]-j[1]*tail[1]-j[2]*tail[2];
                memcpy(output,&translation,4);memcpy(output+1,j+2,4);
                memcpy(output+2,j+1,4);memcpy(output+3,j,4);output+=4;
            }
            {
                const float *j=joint+8;
                float translation=j[3]-j[0]*tail[0]-j[1]*tail[1]-j[2]*tail[2];
                memcpy(output,&translation,4);memcpy(output+1,j+2,4);
                memcpy(output+2,j+1,4);memcpy(output+3,j,4);output+=4;
            }
        }
        w[base]=w[base+1];w[base+1]=header;
        w[base+count*12+1]=0;at=base+count*12+2;dirty(s,10+first*3,count*3);
        remaining&=~(((1u<<count)-1)<<first);
    }
    at=copy(w,at,g->draw,30);
    s->attributes=g->attributes;s->layout=g->layout;s->material=p->material;
    for(unsigned bits=offset_mask;bits;bits&=bits-1){unsigned i=__builtin_ctz(bits);s->offsets[i]=offsets[i];}
    s->offset_candidates=candidates;
    memcpy(s->colors,p->colors,sizeof s->colors);
    memcpy(s->raster,raster,sizeof raster);
    for(unsigned i=0;i<3;i++)if(units&(1u<<i))s->samplers[i]=p->samplers[i];
    if(uv)memcpy(s->uniforms,p->uniforms,sizeof s->uniforms);
    if(scale)memcpy(s->scale,p->scale,sizeof s->scale);
    if(environment)memcpy(s->environment,env,sizeof env);
    s->texture_config=p->texture_config&~(1u<<16);s->texture_shadow=p->texture_shadow;s->valid=1;
    *cursor+=at;return at;
}
