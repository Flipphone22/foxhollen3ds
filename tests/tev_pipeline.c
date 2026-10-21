#include "fh_material.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "fur_fixture.h"
static unsigned seed=501,checks;
static float random_unit(void) { seed=seed*1664525+1013904223;return (seed>>8)*(1.f/16777216); }
static float sat(float x) { return fmaxf(0,fminf(1,x)); }
static float konst(const FhDrawState *s,unsigned selector,unsigned c) {
    if(selector<8) return (8-selector)/8.f;
    if(selector<16 && selector>=12) return s->konst_colors[selector-12][c];
    return selector>=16 && selector<32?s->konst_colors[selector&3][(selector-16)/4]:0;
}
static float gx_arg(const FhDrawState *s,const FhTevStage *t,float regs[4][4],float tex[8][4],float raster[2][4],unsigned arg,unsigned c) {
    if(c==3) {
        if(arg<4) return regs[arg][3];
        if(arg==4) return t->map<8?tex[t->map][s->swaps[t->tex_swap][3]]:1;
        if(arg==5) return t->channel<6?raster[t->channel&1][s->swaps[t->ras_swap][3]]:0;
        return arg==6?konst(s,t->kalpha,3):0;
    }
    if(arg<8) return regs[arg/2][arg&1?3:c];
    if(arg==8 || arg==9) return t->map<8?tex[t->map][s->swaps[t->tex_swap][arg==9?3:c]]:1;
    if(arg==10 || arg==11) return t->channel<6?raster[t->channel&1][s->swaps[t->ras_swap][arg==11?3:c]]:0;
    return arg==12?1:arg==13?.5f:arg==14?konst(s,t->kcolor,c):0;
}
static void reference(float result[4],const FhDrawState *s,float tex[8][4],float raster[2][4]) {
    float regs[4][4];memcpy(regs,s->tev_colors,sizeof(regs));
    const float scales[]={1,2,4,.5f};
    for(unsigned i=0;i<s->stages;i++) {
        const FhTevStage *t=&s->tev[i];float next[4];
        for(unsigned c=0;c<4;c++) {
            const unsigned *args=c==3?t->alpha:t->color;float a[4];
            for(unsigned j=0;j<4;j++) a[j]=gx_arg(s,t,regs,tex,raster,args[j],c);
            unsigned op=c==3?t->alpha_op:t->color_op,bias=c==3?t->alpha_bias:t->color_bias,scale=c==3?t->alpha_scale:t->color_scale,clamp=c==3?t->alpha_clamp:t->color_clamp;
            next[c]=(a[3]+(op?-1:1)*(a[0]*(1-a[2])+a[1]*a[2])+(bias==1?.5f:bias==2?-.5f:0))*scales[scale];
            if(clamp) next[c]=sat(next[c]);
        }
        for(unsigned c=0;c<4;c++) regs[c==3?t->alpha_dest:t->color_dest][c]=next[c];
    }
    memcpy(result,regs[0],16);
}
static void evaluate(float result[4],const FhPipeline *p,float tex[8][4],float raster[2][4]) {
    float primary[4],prev[4]={0},buffer[4]={0},next_buffer[4]={0};
    for(unsigned c=0;c<4;c++) primary[c]=p->primary[c]<0?0:((float*)raster)[p->primary[c]];
    for(unsigned i=0;i<p->count;i++) {
        const FhCombiner *s=&p->stages[i];float next[4];
        for(unsigned c=0;c<4;c++) {
            const FhCombinerChannel *ch=&s->channel[c==3];float a[3];
            for(unsigned j=0;j<3;j++) {
                unsigned source=ch->source[j],operand=ch->operand[j];
                const float *value=source==FH_PRIMARY?primary:source==FH_CONSTANT?s->constant:source==FH_PREVIOUS?prev:source==FH_BUFFER?buffer:tex[p->map[source-FH_TEXTURE0]];
                unsigned component=operand&7;a[j]=value[component==0?c:component==1?3:component-2];
                if(operand&8) a[j]=1-a[j];
            }
            float x=0;
            switch(ch->operation) {
            case FH_REPLACE:x=a[0];break;case FH_MODULATE:x=a[0]*a[1];break;case FH_ADD:x=a[0]+a[1];break;
            case FH_ADD_SIGNED:x=a[0]+a[1]-.5f;break;case FH_INTERPOLATE:x=a[0]*a[2]+a[1]*(1-a[2]);break;
            case FH_SUBTRACT:x=a[0]-a[1];break;case FH_MULTIPLY_ADD:x=a[0]*a[1]+a[2];break;default:assert(0);
            }
            next[c]=sat(x*ch->scale);
        }
        memcpy(prev,next,16);
        memcpy(buffer,next_buffer,16);
        for(unsigned c=0;c<4;c++) if(p->buffer_updates[c==3]&(1u<<i)) next_buffer[c]=next[c];
    }
    memcpy(result,prev,16);
}
int main(void) {
    for(unsigned sample=0;sample<10000;sample++){
        FhDrawState s={0},draw;FhMaterial plan;FhVertex v[3]={0},out[3];
        float tex[8][4]={{0}},raster[2][4],expected[4],actual[4];s.stages=2;
        for(unsigned i=0;i<2;i++)s.tev[i]=(FhTevStage){.color={15,10,11,i?0:8},.alpha={7,7,7,7},.color_clamp=1,.alpha_clamp=1,.channel=4+i};
        for(unsigned i=0;i<4;i++)s.swaps[0][i]=i;
        for(unsigned c=0;c<8;c++)((float*)raster)[c]=random_unit();
        for(unsigned c=0;c<4;c++)tex[0][c]=random_unit();
        for(unsigned i=0;i<3;i++)memcpy(v[i].color,raster,sizeof(raster));
        assert(fh_sphere_prepare(&plan,&draw,out,&s,v,3));
        reference(expected,&s,tex,raster);
        tex[6][0]=raster[1][0];tex[6][1]=raster[1][1];
        tex[7][2]=raster[1][2];tex[7][3]=raster[1][3];
        evaluate(actual,&plan.pipeline,tex,raster);
        for(unsigned c=0;c<4;c++){assert(fabsf(expected[c]-actual[c])<1e-6f);checks++;}
        for(unsigned j=0;j<2;j++)for(unsigned c=0;c<2;c++){
            assert(fabsf((out[0].uv[6+j][c]*16-.5f)/15-raster[1][2*j+c])<1e-6f);checks++;
        }
        for(unsigned j=0;j<2;j++)for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
            const unsigned char *pixel=(const unsigned char*)draw.textures[6+j].pixels+(y*16+x)*4;
            assert(pixel[j*2]==x*17 && pixel[j*2+1]==y*17);
        }
        v[0].color[1][0]=-0.01f;assert(!fh_sphere_prepare(&plan,&draw,out,&s,v,3));
    }
    for(unsigned mode=0;mode<12;mode++) for(unsigned sample=0;sample<10000;sample++) {
        FhDrawState s={0};s.stages=2;float tex[8][4],raster[2][4],expected[4],actual[4];
        for(unsigned i=0;i<32;i++) ((float*)tex)[i]=random_unit();
        for(unsigned i=0;i<8;i++) ((float*)raster)[i]=random_unit();
        for(unsigned i=0;i<4;i++) for(unsigned j=0;j<4;j++) { s.swaps[i][j]=j;s.konst_colors[i][j]=random_unit(); }
        s.tev[0]=(FhTevStage){.color={15,8,10,15},.alpha={7,4,5,7},.color_clamp=1,.alpha_clamp=1,.channel=4};
        s.tev[1]=(FhTevStage){.color={15,8,0,15},.alpha={7,4,0,7},.color_clamp=1,.alpha_clamp=1,.channel=4,.map=1,.coord=1};
        if(mode==1) {s.tev[0].color_scale=2;s.tev[1].color[1]=0;s.tev[1].color[2]=13;}
        if(mode==2) {s.tev[1].color[0]=0;s.tev[1].color[2]=14;s.tev[1].kcolor=12;}
        if(mode==3) {s.tev[1].color[3]=14;s.tev[1].kcolor=12;}
        if(mode==4) {s.tev[1].color[2]=12;s.tev[1].color[3]=0;s.tev[1].color_op=1;}
        if(mode==5) {
            s.constant_raster_mask=255;memcpy(s.constant_raster,raster,sizeof(raster));memset(s.constant_raster[0],0,12);memset(raster[0],0,12);
            s.tev[0].color[2]=14;s.tev[0].color[3]=10;s.tev[0].color_dest=2;
            s.tev[1].color[2]=4;s.tev[1].color[3]=6;
            s.tev[0].alpha[1]=s.tev[0].alpha[2]=7;s.tev[1].alpha[2]=6;
        }
        if(mode==6) { s.stages=3;s.tev[2]=s.tev[1];s.tev[2].map=s.tev[2].coord=2;s.tev[2].alpha[1]=0;s.tev[2].alpha[2]=6; }
        if(mode==7) {
            s.stages=4;s.tev[0].color_dest=2;s.tev[0].color[2]=14;s.tev[0].color[3]=10;
            s.tev[1].color[2]=4;s.tev[1].color_dest=3;s.tev[1].coord=0;
            s.tev[1].indirect[4]=s.tev[1].indirect[5]=6;s.tev[1].indirect[6]=1;
            s.tev[2]=s.tev[1];memset(s.tev[2].indirect,0,sizeof(s.tev[2].indirect));s.tev[2].color[2]=6;s.tev[2].map=2;s.tev[2].coord=1;
            s.tev[3]=s.tev[2];s.tev[3].color[2]=4;s.tev[3].color[3]=6;s.tev[3].color_dest=0;
        }
        if(mode>=8) {
            const unsigned colors[7][4]={{15,8,14,10},{15,8,4,15},{15,6,8,15},{15,8,14,15},{8,0,1,15},{15,0,4,6},{0,8,14,15}};
            const unsigned alphas[7][4]={{7,7,7,7},{7,4,3,7},{7,7,7,7},{7,4,6,7},{7,4,0,7},{7,7,7,0},{7,7,7,0}};
            const unsigned maps[]={0,1,2,2,3,255,4},coords[]={0,0,1,1,2,255,3},dest[]={2,3,3,0,0,0,0};
            s.stages=7;s.color_write=1;s.alpha_compare[0]=s.alpha_compare[3]=7;
            for(unsigned i=0;i<7;i++) {
                s.tev[i]=(FhTevStage){.color_clamp=1,.alpha_clamp=1,.map=maps[i],.coord=coords[i],.color_dest=dest[i],.channel=i==0?4:255};
                memcpy(s.tev[i].color,colors[i],16);memcpy(s.tev[i].alpha,alphas[i],16);
            }
            s.tev[1].indirect[4]=s.tev[1].indirect[5]=6;s.tev[1].indirect[6]=1;
            s.tev[1].tex_swap=3;for(unsigned c=0;c<4;c++)s.swaps[3][c]=c==3?1:2;
            s.textures[0].format=s.textures[1].format=s.textures[3].format=6;s.tev[6].kcolor=sample%8;
            if(mode==9) s.stages=6;
            if(mode==10){s.stages=5;s.tev[3].color[2]=4;s.tev[3].color[3]=6;}
            if(mode==11){
                s.tev[6]=(FhTevStage){.color={0,14,3,15},.alpha={7,7,7,0},.color_clamp=1,.alpha_clamp=1,.coord=255,.map=255,.channel=255,.kcolor=12};
                s.tev_colors[1][3]=random_unit();
            }
        }
        FhMaterial p;assert(mode>=8?fh_layered_material_compile(&p,&s):fh_pipeline_compile(&p,&s));
        reference(expected,&s,tex,raster);
        if(mode>=8) {
            float packed[8][4];memcpy(packed,tex,sizeof(packed));packed[0][3]=tex[1][2];
            if(mode!=10)for(unsigned c=0;c<3;c++) packed[3][c]*=.5f;
            evaluate(actual,&p.pipeline,packed,raster);
            for(unsigned c=0;c<3;c++) actual[c]=actual[c]*(1-p.movie_mix)+tex[4][c]*p.movie_mix;
        } else evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<4;c++) { if(fabsf(actual[c]-expected[c])>1e-5f) { fprintf(stderr,"mode=%u c=%u %.8f != %.8f\n",mode,c,actual[c],expected[c]);assert(0); }checks++; }
        if(mode==10){
            s.tev[3].color_clamp=0;assert(!fh_layered_material_compile(&p,&s));s.tev[3].color_clamp=1;
            s.textures[1].mag_filter=1;assert(!fh_layered_material_compile(&p,&s));s.textures[1].mag_filter=0;
            s.tev[0].kcolor=1;assert(!fh_layered_material_compile(&p,&s));
        }
    }
    for(unsigned k=0;k<100000;k++) {
        FhDrawState s={0};FhMaterial p;float tex[8][4]={{0}},raster[2][4],actual[4];
        for(unsigned c=0;c<4;c++) {
            raster[0][c]=random_unit();s.tev_colors[3][c]=random_unit();
            tex[0][c]=random_unit();tex[1][c]=random_unit();tex[2][c]=random_unit();
        }
        tex[0][0]*=.5f;fh_fur_pipeline(&p,&s);evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<3;c++) {
            float base=tex[1][c]*sat(tex[2][c]*s.tev_colors[3][c]+raster[0][c]);
            float expected=sat(.5f+.5f*base-2*tex[0][0]*(1-base));
            assert(fabsf(actual[c]-expected)<1e-5f);checks++;
        }
        assert(fabsf(actual[3]-tex[0][3])<1e-5f);checks++;
        s.tev_colors[3][3]=random_unit()-.5f;
        for(unsigned c=0;c<4;c++)tex[0][c]=random_unit();
        s.tev[3].channel=k&1?255:8;
        fh_fur_mesh_pipeline(&p,&s);evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<3;c++) {
            float base=tex[1][c]*sat(tex[2][c]*s.tev_colors[3][c]+raster[0][c]);
            assert(fabsf(actual[c]-sat(.5f+.5f*base-tex[0][c]*(1-base)))<1e-5f);checks++;
        }
        assert(fabsf(actual[3]-sat(tex[0][3]*(k&1?1:tex[1][3])+s.tev_colors[3][3]))<1e-5f);checks++;
        assert(p.pipeline.texture_mode[1]==(k&1?0u:3u));checks++;

    }
    /* Reduced fur changes coordinates only. Independently check the live
     * lighting/noise/opacity expression using original sampler numbering.
     * Packed mask alpha is represented by the already combined sampler here. */
    for(unsigned n=0;n<10000;n++) {
        FhDrawState s={0};FhMaterial p;float tex[8][4],raster[2][4],actual[4];
        fur_fixture(&s);s.textures[3].format=3;
        unsigned unmasked=n&1;
        if(unmasked){
            s.tev[3].channel=255;
            const unsigned alpha[]={4,7,7,0},indirect[]={1,0,7,2,0,0,1,0,0};
            memcpy(s.tev[3].alpha,alpha,sizeof(alpha));memcpy(s.tev[3].indirect,indirect,sizeof(indirect));
        }
        for(unsigned i=0;i<32;i++)((float*)tex)[i]=random_unit();
        for(unsigned i=0;i<8;i++)((float*)raster)[i]=random_unit();
        for(unsigned c=0;c<4;c++)s.tev_colors[3][c]=random_unit();
        s.tev_colors[3][3]-=.5f;
        assert(fh_fur_surface_pipeline(&p,&s));
        assert(p.pipeline.coord[0]==4 && p.pipeline.coord[1]==2 && p.pipeline.coord[2]==0);
        assert(p.pipeline.texture_mode[1]==(unmasked?0u:3u));
        if(!unmasked)assert(p.pipeline.secondary_map[1]==2);
        evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<3;c++){
            float base=tex[0][c]*sat(tex[1][c]*s.tev_colors[3][c]+raster[0][c]);
            assert(fabsf(actual[c]-sat(.5f+.5f*base-tex[3][c]*(1-base)))<1e-5f);checks++;
        }
        assert(fabsf(actual[3]-sat(tex[3][3]*(unmasked?1:tex[0][3])+s.tev_colors[3][3]))<1e-5f);checks++;
        s.tev[3].color_op=0;assert(!fh_fur_surface_pipeline(&p,&s));checks++;
    }
    /* Opening damaged-ship tint: one RGB constant plus an independent scalar
     * in the RGB equation; the final alpha is preserved from the prior stage. */
    for(unsigned n=0;n<10000;n++) {
        FhDrawState s={0};FhMaterial p;float tex[8][4],raster[2][4],actual[4],expected[4];
        for(unsigned i=0;i<32;i++)((float*)tex)[i]=random_unit();
        for(unsigned i=0;i<8;i++)((float*)raster)[i]=random_unit();
        for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++){s.swaps[i][j]=j;s.konst_colors[i][j]=random_unit();s.tev_colors[i][j]=random_unit();}
        s.swaps[3][0]=s.swaps[3][1]=s.swaps[3][2]=0;s.swaps[3][3]=1;s.stages=4;
        s.tev[0]=(FhTevStage){.color={15,8,10,15},.alpha={7,4,3,7},.color_clamp=1,.alpha_clamp=1,.color_dest=3,.channel=4,.tex_swap=3};
        s.tev[1]=(FhTevStage){.color={15,6,8,15},.alpha={7,7,7,7},.color_clamp=1,.alpha_clamp=1,.color_dest=3,.map=1,.coord=1,.channel=255};
        s.tev[2]=(FhTevStage){.color={15,8,10,6},.alpha={7,4,6,7},.color_clamp=1,.alpha_clamp=1,.map=1,.coord=1,.channel=4};
        s.tev[3]=(FhTevStage){.color={0,14,3,15},.alpha={7,7,7,0},.color_clamp=1,.alpha_clamp=1,.map=255,.coord=255,.channel=255,.kcolor=12};
        assert(fh_pipeline_compile(&p,&s));
        assert(p.pipeline.stages[3].channel[0].operand[2]==1);
        reference(expected,&s,tex,raster);evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<4;c++){assert(fabsf(expected[c]-actual[c])<1e-5f);checks++;}
        /* A genuinely conflicting alpha constant must still fall back. */
        s.tev[3].alpha[3]=6;s.tev[3].kalpha=28;
        if(s.konst_colors[0][3]!=s.tev_colors[1][3])assert(!fh_pipeline_compile(&p,&s));
    }
    /* Palace projected-alpha material: terminal K - previous-register *
     * independently sampled ramp. Vary every input, swap, scale and K. */
    for(unsigned n=0;n<10000;n++) {
        FhDrawState s={0};FhMaterial p;float tex[8][4],raster[2][4],actual[4],expected[4];
        for(unsigned i=0;i<32;i++)((float*)tex)[i]=random_unit();
        for(unsigned i=0;i<8;i++)((float*)raster)[i]=random_unit();
        for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++){s.swaps[i][j]=j;s.konst_colors[i][j]=random_unit();s.tev_colors[i][j]=random_unit();}
        s.stages=3;s.swaps[1][3]=n%4;
        s.tev[0]=(FhTevStage){.color={15,8,10,15},.alpha={7,4,5,7},.color_clamp=1,.alpha_clamp=1,.channel=4};
        s.tev[1]=(FhTevStage){.color={15,15,15,0},.alpha={7,7,7,4},.color_clamp=1,.alpha_clamp=1,.alpha_dest=2,.coord=1,.map=1,.channel=255,.tex_swap=1};
        s.tev[2]=(FhTevStage){.color={15,15,15,0},.alpha={7,2,4,6},.alpha_op=1,.color_clamp=1,.alpha_clamp=1,.coord=2,.map=2,.channel=255,.tex_swap=1,.kalpha=28,.alpha_scale=n%3};
        assert(fh_pipeline_compile(&p,&s));assert(p.pipeline.count==4);
        reference(expected,&s,tex,raster);evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<4;c++){assert(fabsf(expected[c]-actual[c])<1e-5f);checks++;}
        s.tev[2].alpha_clamp=0;assert(!fh_pipeline_compile(&p,&s));
        s.tev[2].alpha_clamp=1;s.konst_colors[0][3]=1.5f;assert(!fh_pipeline_compile(&p,&s));
    }
    /* Nonterminal subtract with a repeated multiplicand: live raster,
     * texture swaps, constants, RGB and alpha, plus subsequent modulation. */
    for(unsigned n=0;n<20000;n++) {
        FhDrawState s={0};FhMaterial p;float tex[8][4],raster[2][4],actual[4],expected[4];
        for(unsigned i=0;i<32;i++)((float*)tex)[i]=random_unit();
        for(unsigned i=0;i<8;i++)((float*)raster)[i]=random_unit();
        for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++){s.swaps[i][j]=j;s.konst_colors[i][j]=random_unit();}
        s.stages=2;s.swaps[1][3]=n%4;
        s.tev[0]=(FhTevStage){.color={15,8,10,10},.alpha={7,4,5,5},.color_op=1,.alpha_op=1,.color_clamp=1,.alpha_clamp=1,.channel=4,.tex_swap=1,.color_scale=n%3,.alpha_scale=n%3};
        if(n&1){s.tev[0].color[1]=10;s.tev[0].color[2]=8;s.tev[0].alpha[1]=5;s.tev[0].alpha[2]=4;}
        if(n&2){s.constant_raster_mask=255;memcpy(s.constant_raster,raster,sizeof(raster));}
        s.tev[1]=(FhTevStage){.color={15,0,14,15},.alpha={7,0,6,7},.color_clamp=1,.alpha_clamp=1,.coord=255,.map=255,.channel=255,.kcolor=12,.kalpha=28};
        assert(fh_pipeline_compile(&p,&s));assert(p.pipeline.count==2);
        reference(expected,&s,tex,raster);evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<4;c++){assert(fabsf(expected[c]-actual[c])<1e-5f);checks++;}
    }
    /* Chained coordinate material: texture RGB and independent subtractive
     * alpha remain the exact GX expression, without indirect sample inputs. */
    for(unsigned n=0;n<10000;n++) {
        FhDrawState s={0};FhMaterial p;float tex[8][4],raster[2][4],actual[4],expected[4];
        for(unsigned i=0;i<32;i++)((float*)tex)[i]=random_unit();
        for(unsigned i=0;i<8;i++)((float*)raster)[i]=random_unit();
        for(unsigned c=0;c<4;c++)s.swaps[0][c]=c;
        s.stages=s.texgens=3;s.indirect_count=2;s.konst_colors[0][3]=random_unit();
        for(unsigned i=0;i<2;i++)s.textures[i]=(FhTexture){.pixels=tex,.width=32,.height=32,.min_filter=1,.mag_filter=1};
        s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.color_clamp=1,.alpha_clamp=1,.channel=4,.indirect={0,0,7,1,0,0,0,0,0}};
        s.tev[1]=(FhTevStage){.color={15,15,15,8},.alpha={7,7,7,0},.color_clamp=1,.alpha_clamp=1,.coord=1,.channel=255,.indirect={1,0,7,1,0,0,1,0,0}};
        s.tev[2]=(FhTevStage){.color={15,15,15,0},.alpha={7,4,6,0},.alpha_op=1,.color_clamp=1,.alpha_clamp=1,.coord=2,.map=1,.channel=255,.kalpha=28};
        assert(fh_chained_material(&s));fh_chained_pipeline(&p,&s);
        reference(expected,&s,tex,raster);evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<4;c++){assert(fabsf(expected[c]-actual[c])<1e-5f);checks++;}
        s.indirect_matrix[0][0][0]=.001f;assert(!fh_chained_material(&s));
    }
    for(unsigned n=0;n<10000;n++){
        FhDrawState s={0};FhMaterial p;float tex[8][4],raster[2][4],actual[4],expected[4];
        for(unsigned i=0;i<32;i++)((float*)tex)[i]=random_unit();
        for(unsigned i=0;i<8;i++)((float*)raster)[i]=random_unit();
        for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s.swaps[i][c]=c;
        for(unsigned c=0;c<4;c++)s.konst_colors[0][c]=random_unit();
        s.stages=s.texgens=3;
        s.tev[0]=(FhTevStage){.color={15,15,15,15},.alpha={7,7,7,4},.color_clamp=1,.alpha_clamp=1,.alpha_dest=2,.channel=255};
        s.tev[1]=(FhTevStage){.color={15,15,15,0},.alpha={7,2,4,6},.alpha_op=1,.color_clamp=1,.alpha_clamp=1,.coord=1,.map=1,.channel=255};
        s.tev[2]=(FhTevStage){.color={15,8,14,15},.alpha={7,4,0,7},.color_clamp=1,.alpha_clamp=1,.coord=2,.map=2,.channel=255,.kcolor=12};
        assert(fh_layered_material_compile(&p,&s) && p.pipeline.count==3 && p.pipeline.textures==3);
        reference(expected,&s,tex,raster);evaluate(actual,&p.pipeline,tex,raster);
        for(unsigned c=0;c<4;c++){assert(fabsf(expected[c]-actual[c])<1e-5f);checks++;}
        FhDrawState bad=s;
        switch(n%8){
        case 0:bad.tev[1].alpha_op=0;break;
        case 1:bad.tev[1].kalpha=1;break;
        case 2:bad.tev[2].coord=1;break;
        case 3:bad.tev[0].alpha_dest=1;break;
        case 4:bad.swaps[0][3]=0;break;
        case 5:bad.tev[2].alpha_scale=1;break;
        case 6:bad.tev[0].indirect[3]=1;break;
        case 7:bad.konst_colors[0][0]=NAN;break;
        }
        assert(!fh_layered_material_compile(&p,&bad));checks++;
    }
    FhDrawState s={0};FhMaterial p;s.stages=7;assert(!fh_pipeline_compile(&p,&s));
    printf("Direct PICA combiner mapping: %u randomized component comparisons passed\n",checks);
}
