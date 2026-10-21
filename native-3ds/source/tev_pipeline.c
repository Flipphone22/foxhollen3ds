#include "fh_material.h"
#include <string.h>
#include <math.h>

/* Direct GX stages preserve intermediate saturation. Each accepted expression
 * fits one PICA combiner; stale GX registers and excess resources are rejected. */
typedef struct { int source,operand,epoch;float value[3]; } Arg;
typedef struct { FhPipeline *out;const FhDrawState *state;Arg registers[4][2];int buffer_reads[6][2];unsigned index;int failed; } Compiler;
static Arg scalar(float x) { return (Arg){.source=FH_CONSTANT,.value={x,x,x}}; }
static int is_value(Arg a,float x) { return a.source==FH_CONSTANT && a.value[0]==x && a.value[1]==x && a.value[2]==x; }
static int equal(Arg a,Arg b) { return a.source==b.source && a.operand==b.operand && a.epoch==b.epoch && (a.source!=FH_CONSTANT || !memcmp(a.value,b.value,sizeof(a.value))); }
static Arg inverse(Arg a) {
    if(a.source==FH_CONSTANT)for(unsigned i=0;i<3;i++)a.value[i]=1-a.value[i];
    else a.operand^=8;
    return a;
}
static float konst(const FhDrawState *s,unsigned selector,unsigned c) {
    if(selector<8) return (8-selector)/8.f;
    if(selector>=12 && selector<16) return s->konst_colors[selector-12][c];
    if(selector>=16 && selector<32) return s->konst_colors[selector&3][(selector-16)/4];
    return 0;
}
static Arg texture(Compiler *c,const FhTevStage *s,int alpha) {
    if(s->map>=8 || s->coord>=8) return scalar(1);
    unsigned slot=0;while(slot<c->out->textures && (c->out->map[slot]!=(int)s->map || c->out->coord[slot]!=(int)s->coord)) slot++;
    if(slot==3) { c->failed=1;return scalar(0); }
    if(slot==c->out->textures) { c->out->map[slot]=s->map;c->out->coord[slot]=s->coord;c->out->textures++; }
    const uint32_t *swap=c->state->swaps[s->tex_swap&3];
    int component=-1;
    if(alpha) component=swap[3]&3;
    else if(swap[0]==swap[1] && swap[0]==swap[2]) component=swap[0]&3;
    else if(swap[0]!=0 || swap[1]!=1 || swap[2]!=2) c->failed=1;
    return (Arg){.source=FH_TEXTURE0+(int)slot,.operand=component<0?0:component==3?1:component+2};
}
static Arg raster(Compiler *c,const FhTevStage *s,int alpha) {
    if(s->channel>=6) return scalar(0);
    int indices[3],known=1;
    for(unsigned i=0;i<(alpha?1u:3u);i++) {
        /* RASA selects the swapped alpha component, even when it is RGB. */
        unsigned component=c->state->swaps[s->ras_swap&3][alpha?3:i]&3;
        indices[i]=(s->channel&1)*4+component;
        if(!(c->state->constant_raster_mask&(1u<<indices[i]))) known=0;
    }
    Arg a=scalar(0);
    if(known) {
        for(unsigned i=0;i<3;i++) a.value[i]=((const float*)c->state->constant_raster)[indices[alpha?0:i]];
    } else {
        a.source=FH_PRIMARY;a.operand=alpha?1:0;
        for(unsigned i=0;i<(alpha?1u:3u);i++) {
            unsigned target=alpha?3:i;int *prior=&c->out->primary[target];
            if(*prior>=0 && *prior!=indices[i]) c->failed=1;
            *prior=indices[i];
        }
    }
    return a;
}
static Arg argument(Compiler *c,const FhTevStage *s,unsigned arg,int alpha) {
    if(arg<(alpha?4u:8u)) {
        Arg a=c->registers[alpha?arg:arg/2][alpha || (arg&1)];
        if(a.source==FH_PREVIOUS && a.epoch!=(int)c->index-1) {
            unsigned component=a.operand==1;
            if(a.epoch<0 || a.epoch>=4 || a.epoch>(int)c->index-2 || (c->buffer_reads[c->index][component]>=0 && c->buffer_reads[c->index][component]!=a.epoch)) c->failed=1;
            else {
                c->buffer_reads[c->index][component]=a.epoch;c->out->buffer_updates[component]|=1u<<a.epoch;a.source=FH_BUFFER;
            }
        }
        return a;
    }
    if(arg==(alpha?4u:8u) || (!alpha && arg==9)) return texture(c,s,alpha || arg==9);
    if(arg==(alpha?5u:10u) || (!alpha && arg==11)) return raster(c,s,alpha || arg==11);
    if(arg==(alpha?6u:14u)) {
        Arg a=scalar(0);for(unsigned i=0;i<3;i++) a.value[i]=konst(c->state,alpha?s->kalpha:s->kcolor,alpha?3:i);return a;
    }
    return scalar(!alpha && arg==12?1:!alpha && arg==13?.5f:0);
}
static int assign(FhCombiner *stage,FhCombinerChannel *channel,Arg a,unsigned slot,int alpha,unsigned *constant_mask) {
    channel->source[slot]=a.source;channel->operand[slot]=a.operand;
    if(a.source!=FH_CONSTANT) return 1;
    /* PICA constants expose independent RGB and alpha operands. A scalar
     * color factor can share a stage with an RGB tint by reading alpha, as
     * in pinned Melee renderer.c's layered tint combiner. Preserve rejection
     * if the alpha equation requires a different constant. */
    if(!alpha && a.value[0]==a.value[1] && a.value[0]==a.value[2]) {
        int conflict=0;
        for(unsigned i=0;i<3;i++)if((*constant_mask&(1u<<i)) && stage->constant[i]!=a.value[i])conflict=1;
        if(conflict && a.value[0]>=0 && a.value[0]<=1 && (!(*constant_mask&8) || stage->constant[3]==a.value[0])) {
            stage->constant[3]=a.value[0];*constant_mask|=8;channel->operand[slot]=1;return 1;
        }
    }
    channel->operand[slot]=alpha?1:0;
    for(unsigned i=0;i<(alpha?1u:3u);i++) {
        unsigned k=alpha?3:i;float x=a.value[i];
        if(x<0 || x>1 || ((*constant_mask&(1u<<k)) && stage->constant[k]!=x)) return 0;
        stage->constant[k]=x;*constant_mask|=1u<<k;
    }
    return 1;
}
static int compile_channel(FhCombiner *stage,int alpha,Arg a,Arg b,Arg c,Arg d,unsigned op,unsigned bias,unsigned scale,unsigned clamp,unsigned *mask) {
    FhCombinerChannel *out=&stage->channel[alpha];Arg args[3];unsigned n=0;
    if(op>1 || bias || scale==3) return 0;
    out->scale=1u<<scale;
    if(a.source==FH_CONSTANT && b.source==FH_CONSTANT && c.source==FH_CONSTANT && d.source==FH_CONSTANT) {
        for(unsigned i=0;i<3;i++) {
            float x=(d.value[i]+(op?-1:1)*(a.value[i]*(1-c.value[i])+b.value[i]*c.value[i]))*out->scale;
            a.value[i]=clamp?fmaxf(0,fminf(1,x)):x;
        }
        out->operation=FH_REPLACE;out->scale=1;args[n++]=a;
    } else {
        if(equal(a,b) || is_value(c,0)) { b=a;c=scalar(1);a=scalar(0); }
        else if(is_value(c,1)) a=scalar(0);
        if(is_value(a,0) && is_value(b,0)) { b=d;d=scalar(0);c=scalar(1);op=0; }
        if(is_value(a,0) && is_value(c,1)) {
            if(op) { out->operation=FH_SUBTRACT;args[n++]=d;args[n++]=b; }
            else if(is_value(d,0)) { out->operation=FH_REPLACE;args[n++]=b; }
            else { out->operation=FH_ADD;args[n++]=b;args[n++]=d; }
        } else if(is_value(a,0) && op && (equal(c,d) || equal(b,d))) {
            /* C-B*C = (1-B)*C, or B-B*C = (1-C)*B. Preserve the
             * same stage's clamp/scale and register epoch. This also works
             * before later stages, unlike splitting terminal subtraction. */
            out->operation=FH_MODULATE;
            args[n++]=inverse(equal(c,d)?b:c);args[n++]=d;
        } else if(is_value(a,0) && !op) {
            out->operation=is_value(d,0)?FH_MODULATE:FH_MULTIPLY_ADD;args[n++]=b;args[n++]=c;
            if(!is_value(d,0)) args[n++]=d;
        } else if(is_value(d,0) && !op) {
            out->operation=FH_INTERPOLATE;args[n++]=b;args[n++]=a;args[n++]=c;
        } else return 0;
        if(!clamp && (out->scale!=1 || (out->operation!=FH_REPLACE && out->operation!=FH_MODULATE && out->operation!=FH_INTERPOLATE))) return 0;
    }
    for(unsigned i=0;i<n;i++) if(!assign(stage,out,args[i],i,alpha,mask)) return 0;
    return 1;
}
int fh_pipeline_compile(FhMaterial *plan,const FhDrawState *state) {
    FhPipeline pipeline={0};for(unsigned i=0;i<4;i++) pipeline.primary[i]=-1;
    Compiler c={.out=&pipeline,.state=state};unsigned split_alpha=0;
    for(unsigned i=0;i<6;i++) for(unsigned j=0;j<2;j++) c.buffer_reads[i][j]=-1;
    if(!state->stages || state->stages>6) return 0;
    for(unsigned r=0;r<4;r++) {
        c.registers[r][0]=scalar(0);memcpy(c.registers[r][0].value,state->tev_colors[r],12);
        c.registers[r][1]=scalar(state->tev_colors[r][3]);
    }
    for(c.index=0;c.index<state->stages;c.index++) {
        FhTevStage normalized=state->tev[c.index];const FhTevStage *s=&normalized;unsigned mask=0;
        if(s->indirect[3] || s->indirect[8]) return 0;
        if(s->indirect[4] || s->indirect[5] || s->indirect[6]) {
            if(!c.index || s->indirect[4]!=6 || s->indirect[5]!=6 || !s->indirect[6]) return 0;
            const FhTevStage *previous=&state->tev[c.index-1];
            if(previous->map>=8 || s->map>=8 || previous->coord!=s->coord || previous->indirect[3] || previous->indirect[4] || previous->indirect[5] || previous->indirect[6]) return 0;
            const FhTexture *a=&state->textures[previous->map],*b=&state->textures[s->map];
            if(a->width!=b->width || a->height!=b->height) return 0;
            memset(normalized.indirect,0,sizeof(normalized.indirect));
        }
        for(unsigned alpha=0;alpha<2;alpha++) {
            const uint32_t *v=alpha?s->alpha:s->color;Arg args[4];
            for(unsigned j=0;j<4;j++) args[j]=argument(&c,s,v[j],alpha);
            if(c.failed)return 0;
            if(!compile_channel(&pipeline.stages[c.index],alpha,args[0],args[1],args[2],args[3],alpha?s->alpha_op:s->color_op,alpha?s->alpha_bias:s->color_bias,alpha?s->alpha_scale:s->color_scale,alpha?s->alpha_clamp:s->color_clamp,&mask)) {
                /* PICA has multiply-add but no multiply-subtract. A terminal
                 * saturated alpha K-B*C fits two stages without altering GX
                 * register lifetimes: first the bounded product, then K-PREV.
                 * Restrict K to a constant so no prior register is overwritten
                 * before being consumed. Keep all RGB work in its original
                 * stage, with an explicit pass-through in the added stage. */
                if(!alpha || c.index+1!=state->stages || state->stages>=6 ||
                   s->alpha_op!=1 || s->alpha_bias || !s->alpha_clamp || s->alpha_scale>2 ||
                   !is_value(args[0],0) || args[3].source!=FH_CONSTANT)return 0;
                if(!compile_channel(&pipeline.stages[c.index],1,scalar(0),args[1],args[2],scalar(0),0,0,0,1,&mask))return 0;
                FhCombiner *last=&pipeline.stages[c.index+1];unsigned extra_mask=0;
                last->channel[0]=(FhCombinerChannel){.source={FH_PREVIOUS},.operation=FH_REPLACE,.scale=1};
                Arg previous={.source=FH_PREVIOUS,.operand=1};
                if(!compile_channel(last,1,scalar(0),previous,scalar(1),args[3],1,0,s->alpha_scale,1,&extra_mask))return 0;
                split_alpha=1;
            }
        }
        c.registers[s->color_dest&3][0]=(Arg){.source=FH_PREVIOUS,.epoch=c.index,.operand=0};
        c.registers[s->alpha_dest&3][1]=(Arg){.source=FH_PREVIOUS,.epoch=c.index,.operand=1};
    }
    for(unsigned i=0;i<state->stages;i++) for(unsigned component=0;component<2;component++) if(c.buffer_reads[i][component]>=0) {
        int latest=-1;
        for(unsigned write=0;write+2<=i;write++) if(pipeline.buffer_updates[component]&(1u<<write)) latest=write;
        if(latest!=c.buffer_reads[i][component]) return 0;
    }
    const FhTevStage *last=&state->tev[state->stages-1];
    Arg rgb=c.registers[last->color_dest&3][0],alpha=c.registers[last->alpha_dest&3][1];
    if(rgb.source!=FH_PREVIOUS || alpha.source!=FH_PREVIOUS || rgb.epoch!=(int)state->stages-1 || alpha.epoch!=(int)state->stages-1)return 0;
    pipeline.count=state->stages+split_alpha;memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;plan->pipeline=pipeline;return 1;
}
