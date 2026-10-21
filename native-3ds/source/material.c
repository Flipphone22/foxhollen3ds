#include "fh_material.h"
#include <math.h>
#include <string.h>
#ifndef FH_MATERIAL_GENERIC
#include "fh_material_register.h"
#endif

/* Prove a TEV expression can be represented as texture * affine raster +
 * constant before emitting it. Unsupported expressions are never substituted. */
typedef struct { float c[2][9];int map,coord,component,valid,clipped; } Expression;
static Expression constant(float value) { Expression e={.map=-1,.valid=1};e.c[0][0]=value;return e; }
static int zero(const Expression *e) { if(!e->valid) return 0;for(unsigned i=0;i<18;i++) if(((const float*)e->c)[i]!=0) return 0;return 1; }
static int scalar(const Expression *e) { if(!e->valid || e->clipped) return 0;for(unsigned i=1;i<18;i++) if(((const float*)e->c)[i]!=0) return 0;return 1; }
static Expression scale(Expression e,float factor) {
    if(factor==0) return constant(0);
    if(e.clipped && factor!=1) e.valid=0;
    for(unsigned i=0;i<18;i++) ((float*)e.c)[i]*=factor;return e;
}
static Expression add(Expression a,Expression b,float sign) {
    if(zero(&b)) return a;if(zero(&a)) return scale(b,sign);
    if(!a.valid || !b.valid || a.clipped || b.clipped) return (Expression){0};
    if(a.map>=0 && b.map>=0 && (a.map!=b.map || a.coord!=b.coord || a.component!=b.component)) return (Expression){0};
    if(a.map<0 && b.map>=0) { a.map=b.map;a.coord=b.coord;a.component=b.component; }
    for(unsigned i=0;i<18;i++) ((float*)a.c)[i]+=sign*((float*)b.c)[i];return a;
}
static int affine_multiply(float out[9],const float a[9],const float b[9]) {
    int ac=1,bc=1;
    for(unsigned i=1;i<9;i++) { if(a[i]) ac=0;if(b[i]) bc=0; }
    if(!ac && !bc) return 0;
    for(unsigned i=0;i<9;i++) out[i]=ac?a[0]*b[i]:b[0]*a[i];return 1;
}
static Expression multiply(Expression a,Expression b) {
    if(zero(&a) || zero(&b)) return constant(0);
    if(scalar(&a)) return scale(b,a.c[0][0]);if(scalar(&b)) return scale(a,b.c[0][0]);
    if(!a.valid || !b.valid || a.clipped || b.clipped || (a.map>=0 && b.map>=0)) return (Expression){0};
    Expression result=a.map>=0?a:b;memset(result.c,0,sizeof(result.c));
    float first[9],second[9];
    if(!affine_multiply(result.c[0],a.c[0],b.c[0]) || !affine_multiply(first,a.c[1],b.c[0]) || !affine_multiply(second,a.c[0],b.c[1])) return (Expression){0};
    for(unsigned i=0;i<9;i++) result.c[1][i]=first[i]+second[i];return result;
}
static void range(const Expression *e,float *minimum,float *maximum) {
    float lo[2]={e->c[0][0],e->c[1][0]},hi[2]={lo[0],lo[1]};
    for(unsigned part=0;part<2;part++) for(unsigned i=1;i<9;i++) { if(e->c[part][i]<0) lo[part]+=e->c[part][i];else hi[part]+=e->c[part][i]; }
    *minimum=lo[0]+fminf(0,lo[1]);*maximum=hi[0]+fmaxf(0,hi[1]);
}
static Expression operation(Expression a,Expression b,Expression c,Expression d,unsigned op,unsigned bias,unsigned factor,unsigned clamp) {
    Expression result;
    if(op>=8) return (Expression){0};
    else {
        result=add(a,multiply(add(b,a,-1),c),1);
        if(d.valid && d.clipped && scalar(&result) && result.c[0][0]>=0 && op==0 && bias!=2 && factor!=3 && clamp) {
            float minimum,maximum;range(&d,&minimum,&maximum);
            if(minimum>=0) d.clipped=0;
        }
        result=add(d,result,op==1?-1:1);
        result=add(result,constant(bias==1?.5f:bias==2?-.5f:0),1);
    }
    const float scales[]={1,2,4,.5f};result=scale(result,scales[factor&3]);
    if(clamp && result.valid) {
        float minimum,maximum;range(&result,&minimum,&maximum);
        if(maximum<=0) return constant(0);if(minimum>=1) return constant(1);
        if(minimum<0 || maximum>1) result.clipped=1;
    }
    return result;
}
static float konst(const FhDrawState *state,unsigned selector,unsigned component) {
    if(selector<8) return (8-selector)/8.f;
    if(selector>=12 && selector<16) return state->konst_colors[selector-12][component];
    if(selector>=16 && selector<32) return state->konst_colors[selector&3][(selector-16)/4];
    return 0;
}
static Expression raster(const FhDrawState *state,const FhTevStage *stage,unsigned component) {
    if(stage->channel>=6) return constant(0);
    Expression value=constant(0);unsigned part=stage->channel&1;
    /* GXSetTevOrder maps COLOR/ALPHA/COLORA aliases to the same raster. */
    unsigned channel=state->swaps[stage->ras_swap&3][component];
    if(state->constant_raster_mask&(1u<<(part*4+(channel&3)))) return constant(state->constant_raster[part][channel&3]);
    value.c[0][1+part*4+(channel&3)]=1;return value;
}
static Expression texture(const FhDrawState *state,const FhTevStage *stage,unsigned component) {
    if(stage->map>=8 || stage->coord>=8) return constant(1);
    Expression value=constant(0);value.map=stage->map;value.coord=stage->coord;value.component=state->swaps[stage->tex_swap&3][component]&3;value.c[1][0]=1;return value;
}
static Expression color_arg(const FhDrawState *state,const FhTevStage *stage,Expression registers[4][4],unsigned argument,unsigned component) {
    if(argument<8) return registers[argument/2][argument&1?3:component];
    if(argument==8 || argument==9) return texture(state,stage,argument==9?3:component);
    if(argument==10 || argument==11) return raster(state,stage,argument==11?3:component);
    return constant(argument==12?1:argument==13?.5f:argument==14?konst(state,stage->kcolor,component):0);
}
static Expression alpha_arg(const FhDrawState *state,const FhTevStage *stage,Expression registers[4][4],unsigned argument) {
    if(argument<4) return registers[argument][3];
    if(argument==4) return texture(state,stage,3);
    if(argument==5) return raster(state,stage,3);
    return constant(argument==6?konst(state,stage->kalpha,3):0);
}
static int fail(FhMaterial *plan,const char *reason) { plan->failure=reason;return 0; }
static int sphere_lighting(const FhDrawState *state) {
    if(state->stages!=2 || state->tev[0].map>=8 || state->tev[0].coord>=8) return 0;
    for(unsigned i=0;i<2;i++) {
        const FhTevStage *s=&state->tev[i];
        if(s->color[0]!=15 || s->color[1]!=10 || s->color[2]!=11 || s->color[3]!=(i?0u:8u) || s->channel!=4+i) return 0;
        if(s->color_op || s->color_bias || s->color_scale || !s->color_clamp || s->color_dest || s->alpha_dest || s->alpha_op || s->alpha_bias || !s->alpha_clamp) return 0;
        for(unsigned j=0;j<4;j++) if(s->alpha[j]!=7 || state->swaps[s->ras_swap&3][j]!=j || state->swaps[s->tex_swap&3][j]!=j) return 0;
        for(unsigned j=0;j<9;j++) if(s->indirect[j]) return 0;
    }
    return 1;
}
int fh_material_compile(FhMaterial *plan,const FhDrawState *state) {
#ifndef FH_MATERIAL_GENERIC
    if(fh_material_register(plan,state))return 1;
#endif
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;plan->scale[0]=plan->scale[1]=1;
    if(!state->stages || state->stages>16) return fail(plan,"invalid TEV stage count");
    if(sphere_lighting(state)) { plan->sphere_lighting=1;plan->map=state->tev[0].map;plan->coord=state->tev[0].coord;return 1; }
    Expression registers[4][4];
    for(unsigned i=0;i<4;i++) for(unsigned c=0;c<4;c++) registers[i][c]=constant(state->tev_colors[i][c]);
    for(unsigned index=0;index<state->stages;index++) {
        const FhTevStage *s=&state->tev[index];Expression out[4];
        if(s->indirect[3] || s->indirect[4] || s->indirect[5] || s->indirect[6] || s->indirect[8]) return fail(plan,"indirect texture expression");
        for(unsigned c=0;c<3;c++) out[c]=operation(color_arg(state,s,registers,s->color[0],c),color_arg(state,s,registers,s->color[1],c),color_arg(state,s,registers,s->color[2],c),color_arg(state,s,registers,s->color[3],c),s->color_op,s->color_bias,s->color_scale,s->color_clamp);
        out[3]=operation(alpha_arg(state,s,registers,s->alpha[0]),alpha_arg(state,s,registers,s->alpha[1]),alpha_arg(state,s,registers,s->alpha[2]),alpha_arg(state,s,registers,s->alpha[3]),s->alpha_op,s->alpha_bias,s->alpha_scale,s->alpha_clamp);
        for(unsigned c=0;c<3;c++) registers[s->color_dest&3][c]=out[c];registers[s->alpha_dest&3][3]=out[3];
    }
    for(unsigned c=0;c<4;c++) {
        const FhTevStage *last=&state->tev[state->stages-1];
        Expression e=registers[(c==3?last->alpha_dest:last->color_dest)&3][c];
        if(!e.valid) return fail(plan,"non-affine or intermediate-clamped TEV expression");
        int textured=0;for(unsigned j=0;j<9;j++) if(e.c[1][j]) textured=1;
        if(textured) {
            if(e.component!=(int)c) return fail(plan,"texture channel swizzle");
            if(plan->map>=0 && (plan->map!=e.map || plan->coord!=e.coord)) return fail(plan,"multiple sampled textures");
            plan->map=e.map;plan->coord=e.coord;
            for(unsigned j=1;j<9;j++) if(e.c[0][j]) return fail(plan,"raster contribution after textured term");
            plan->bias[c]=e.c[0][0];memcpy(plan->color[c],e.c[1],sizeof(e.c[1]));plan->textured[c]=1;
        } else memcpy(plan->color[c],e.c[0],sizeof(e.c[0]));
        float lo=plan->color[c][0],hi=lo;
        for(unsigned j=1;j<9;j++) { if(plan->color[c][j]<0) lo+=plan->color[c][j];else hi+=plan->color[c][j]; }
        if(textured && (lo<0 || plan->bias[c]<0)) return fail(plan,"negative texture factor or bias");
        if(textured && (hi>4 || plan->bias[c]>4)) return fail(plan,"texture factor exceeds PICA scale");
        if(textured) { unsigned required=(hi>2 || plan->bias[c]>2)?4:(hi>1 || plan->bias[c]>1)?2:1;if(required>plan->scale[c==3]) plan->scale[c==3]=required; }
    }
    if(plan->textured[0] || plan->textured[1] || plan->textured[2]) for(unsigned c=0;c<3;c++) if(!plan->textured[c]) {
        for(unsigned j=1;j<9;j++) if(plan->color[c][j]) return fail(plan,"mixed RGB texture dependence");
        float value=plan->color[c][0];
        plan->bias[c]=fmaxf(0,fminf(1,value));plan->color[c][0]=0;plan->textured[c]=1;
    }
    plan->sparse_colors=1;
    for(unsigned c=0;c<4;c++) {
        uint32_t bits;memcpy(&bits,&plan->color[c][0],4);
        if(bits==0x80000000u)plan->sparse_colors=0; /* preserve signed-zero arithmetic */
        for(int j=7;j>=0;j--)if(plan->color[c][j+1]!=0)
            plan->color_terms[c]=(plan->color_terms[c]<<4)|(unsigned)(j+1);
    }
    return 1;
}
void fh_material_color(float output[4],const FhMaterial *plan,const float raster[2][4]) {
    if(plan->sphere_lighting) {
        for(unsigned c=0;c<3;c++) output[c]=.5f*(raster[0][c]*raster[0][3]+raster[1][c]*raster[1][3]);
        output[3]=0;return;
    }
    unsigned sparse=plan->sparse_colors;
    /* Exceptional/out-of-range inputs retain the original full expression,
     * including 0*NaN/Inf behavior and signed zeros. Normal raster colors are
     * finite nonnegative values, so omitted zero terms cannot affect rounding. */
    if(sparse) {
        unsigned exceptional=0;
        for(unsigned j=0;j<8;j++) {
            uint32_t bits;memcpy(&bits,(const float*)raster+j,4);
            exceptional|=bits>0x3f800000u;
        }
        sparse=!exceptional;
    }
    for(unsigned c=0;c<4;c++) {
        float value=plan->color[c][0];
        if(sparse)for(unsigned terms=plan->color_terms[c];terms;terms>>=4) {
            unsigned j=(terms&15)-1;value+=plan->color[c][j+1]*((const float*)raster)[j];
        }
        else for(unsigned j=0;j<8;j++)value+=plan->color[c][j+1]*((const float*)raster)[j];
        output[c]=value/plan->scale[c==3];
    }
}
static float product_error(const FhVertex triangle[3]) {
    float maximum=0;
    for(unsigned c=0;c<3;c++) {
        float bound=0;
        for(unsigned channel=0;channel<2;channel++) {
            float lo=triangle[0].color[channel][c],hi=lo,alo=triangle[0].color[channel][3],ahi=alo;
            for(unsigned v=1;v<3;v++) {
                lo=fminf(lo,triangle[v].color[channel][c]);hi=fmaxf(hi,triangle[v].color[channel][c]);
                alo=fminf(alo,triangle[v].color[channel][3]);ahi=fmaxf(ahi,triangle[v].color[channel][3]);
            }
            bound+=(hi-lo)*(ahi-alo)*.25f;
        }
        maximum=fmaxf(maximum,bound);
    }
    return maximum;
}
unsigned fh_material_subdivide(const FhVertex triangle[3],FhTriangleSink sink,void *context) {
    /* Covariance bound for each product of interpolated colors. Saturation
     * cannot increase this error. At most four levels for inputs in [0,1]. */
    if(product_error(triangle)<=1.f/255) { sink(triangle,context);return 1; }
    FhVertex mid[3],child[3];
    for(unsigned edge=0;edge<3;edge++) for(unsigned value=0;value<sizeof(FhVertex)/sizeof(float);value++)
        ((float*)&mid[edge])[value]=.5f*(((const float*)&triangle[edge])[value]+((const float*)&triangle[(edge+1)%3])[value]);
    unsigned leaves=0;
    for(unsigned corner=0;corner<3;corner++) {
        child[0]=triangle[corner];child[1]=mid[corner];child[2]=mid[(corner+2)%3];leaves+=fh_material_subdivide(child,sink,context);
    }
    leaves+=fh_material_subdivide(mid,sink,context);return leaves;
}
