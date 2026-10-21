#include "fh_pica_world_packet.h"
#include <string.h>
static unsigned valid_live(unsigned units,const FhPicaDrawLive *v){
    return v && (v->texture_config&7)==units &&
        (!(units&1)||v->samplers[0]) && (!(units&2)||v->samplers[1]);
}
static unsigned valid(const FhPicaResidentDraw *d,const FhPicaDrawLive *v){
    return d && v && d->effect_words==18 && d->attributes && d->layout &&
        d->effect && d->stages && d->draw && d->texture_units<=3 &&
        valid_live(d->texture_units,v);
}
static unsigned copy(uint32_t *w,unsigned at,const uint32_t *p,unsigned n){memcpy(w+at,p,n*4);return at+n;}
static unsigned reg(uint32_t *w,unsigned at,unsigned id,unsigned mask,uint32_t value){
    w[at]=value;w[at+1]=GPUCMD_HEADER(0,mask,id);return at+2;
}
int fh_pica_world_packet_prepare(FhPicaWorldPacket *out,uint32_t *w,unsigned capacity,
    const FhPicaResidentDraw *const *draws,const FhPicaDrawLive *live,unsigned count){
    if(!out||!w||((uintptr_t)w&15)||!draws||!live||!count||count>FH_PICA_WORLD_PACKET_DRAWS||
       !out->entry || count>out->entry_capacity || capacity<count*184+4)return 0;
    for(unsigned i=0;i<count;i++)if(!valid(draws[i],live+i))return 0;
    out->count=0;unsigned at=0;
    const FhPicaResidentDraw *previous=NULL;
    for(unsigned i=0;i<count;i++){
        const FhPicaResidentDraw *d=draws[i];const FhPicaDrawLive *v=live+i;
        FhPicaWorldPacketEntry *e=out->entry+i;memset(e,0,sizeof(*e));e->draw=d;
        if(!previous||previous->attributes!=d->attributes)at=copy(w,at,d->attributes,12);
        if(!previous||previous->layout!=d->layout){
            unsigned base=at;at=copy(w,at,d->layout,40);
            for(unsigned j=0;j<12;j++)w[base+(j?3+3*j:2)]=d->offsets[j];
        }else for(unsigned j=0;j<12;j++)if(d->offsets[j]!=previous->offsets[j])
            at=reg(w,at,GPUREG_ATTRIBBUFFER0_OFFSET+3*j,15,d->offsets[j]);
        e->effect=at;at=copy(w,at,v->effect?v->effect:d->effect,18);
        for(unsigned j=0;j<2;j++)if(d->texture_units&(1u<<j)){
            e->sampler[j]=at;at=copy(w,at,v->samplers[j]->words,8);
        }
        e->texture=at;
        at=reg(w,at,GPUREG_TEXUNIT_CONFIG,11,v->texture_config|(1u<<16));
        at=reg(w,at,GPUREG_TEXUNIT_CONFIG,4,1u<<16);
        at=reg(w,at,GPUREG_TEXUNIT0_SHADOW,15,v->texture_shadow);
        e->environment=at;
        at=reg(w,at,GPUREG_TEXENV_UPDATE_BUFFER,7,v->environment_buffer);
        at=reg(w,at,GPUREG_TEXENV_BUFFER_COLOR,15,v->environment_color);
        at=reg(w,at,GPUREG_FOG_COLOR,15,v->fog_color);
        e->stages=at;at=copy(w,at,d->stages,24);
        for(unsigned j=0;j<4;j++)w[e->stages+j*6+4]=v->colors[j];
        at=reg(w,at,GPUREG_VSH_FLOATUNIFORM_CONFIG,15,0x80000007);
        e->uniforms=at;w[at++]=v->material_uniforms[0];
        w[at++]=GPUCMD_HEADER(0,15,GPUREG_VSH_FLOATUNIFORM_DATA)|(11u<<20);
        at=copy(w,at,v->material_uniforms+1,11);w[at++]=0;
        at=copy(w,at,d->draw,30);e->effect_key=v->effect;e->samplers[0]=v->samplers[0];e->samplers[1]=v->samplers[1];previous=d;
    }
    /* Return must fit inside a 16-byte-sized command buffer. Zero-mask write
     * is a no-op on an owned register, not an unknown reserved register. */
    if(!(at&2))at=reg(w,at,GPUREG_PRIMITIVE_CONFIG,0,0);
    at=reg(w,at,GPUREG_CMDBUF_JUMP1,15,1);
    if(at>capacity||at>FH_PICA_WORLD_PACKET_WORDS)return 0;
    out->commands=w;out->words=at;out->count=count;return 1;
}
static void changed(FhPicaWorldPacketDirty *dirty,unsigned at,unsigned n){
    dirty->writes+=n;if(at<dirty->first)dirty->first=at;
    if(at+n>dirty->end)dirty->end=at+n;
}
static void value(uint32_t *w,unsigned at,uint32_t v,FhPicaWorldPacketDirty *dirty){
    if(w[at]!=v){w[at]=v;changed(dirty,at,1);}
}
int fh_pica_world_packet_update(FhPicaWorldPacket *p,const FhPicaDrawLive *live,FhPicaWorldPacketDirty *result){
    if(!p||!p->commands||!live||!result||!p->count||p->count>FH_PICA_WORLD_PACKET_DRAWS||!p->entry||p->count>p->entry_capacity)return 0;
    for(unsigned i=0;i<p->count;i++)if(!valid_live(p->entry[i].draw->texture_units,live+i))return 0;
    FhPicaWorldPacketDirty dirty={0,p->words,0};uint32_t *w=p->commands;
    for(unsigned i=0;i<p->count;i++){
        FhPicaWorldPacketEntry *e=p->entry+i;
        const FhPicaDrawLive *v=live+i;
        if(e->effect_key!=v->effect){
            memcpy(w+e->effect,v->effect?v->effect:e->draw->effect,18*4);
            e->effect_key=v->effect;changed(&dirty,e->effect,18);
        }
        for(unsigned j=0;j<2;j++)if((e->draw->texture_units&(1u<<j))&&e->samplers[j]!=v->samplers[j]){
            memcpy(w+e->sampler[j],v->samplers[j]->words,8*4);e->samplers[j]=v->samplers[j];changed(&dirty,e->sampler[j],8);
        }
        if((w[e->texture]&~(1u<<16))!=(v->texture_config&~(1u<<16))){
            w[e->texture]=v->texture_config|(1u<<16);changed(&dirty,e->texture,1);
        }
        value(w,e->texture+4,v->texture_shadow,&dirty);
        value(w,e->environment,v->environment_buffer,&dirty);
        value(w,e->environment+2,v->environment_color,&dirty);
        value(w,e->environment+4,v->fog_color,&dirty);
        for(unsigned j=0;j<4;j++)value(w,e->stages+j*6+4,v->colors[j],&dirty);
        /* The packet header splits data0 from the remaining contiguous words.
         * Handle it once; do not branch around that fixed gap for every value. */
        value(w,e->uniforms,v->material_uniforms[0],&dirty);
        for(unsigned j=1;j<12;j++)value(w,e->uniforms+j+1,v->material_uniforms[j],&dirty);
    }
    if(!dirty.writes)dirty.first=0;
    *result=dirty;return 1;
}
