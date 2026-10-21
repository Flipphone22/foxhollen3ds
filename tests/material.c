#include "fh_material.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned seed=17,checks;
static float random_unit(void) { seed=seed*1664525+1013904223;return (seed>>8)*(1.f/16777216); }
static float clamp(float x) { return x<0?0:x>1?1:x; }
static unsigned interpolation_checks;
static void verify_triangle(const FhVertex triangle[3],void *unused) {
    (void)unused;
    for(unsigned point=0;point<12;point++) {
        float weights[3]={random_unit(),random_unit(),random_unit()},sum=weights[0]+weights[1]+weights[2];
        for(unsigned v=0;v<3;v++) weights[v]/=sum;
        for(unsigned c=0;c<3;c++) {
            float actual=0,expected=0;
            for(unsigned channel=0;channel<2;channel++) {
                float color=0,alpha=0;
                for(unsigned v=0;v<3;v++) {
                    color+=weights[v]*triangle[v].color[channel][c];alpha+=weights[v]*triangle[v].color[channel][3];
                    actual+=weights[v]*triangle[v].color[channel][c]*triangle[v].color[channel][3];
                }
                expected+=color*alpha;
            }
            assert(fabsf(actual-expected)<=1.f/255+0.000001f);interpolation_checks++;
        }
    }
}
static FhDrawState state(void) {
    FhDrawState s={0};s.stages=1;
    for(unsigned i=0;i<4;i++) for(unsigned j=0;j<4;j++) s.swaps[i][j]=j;
    s.tev[0]=(FhTevStage){.color={15,8,10,15},.alpha={7,4,5,7},.channel=4,.color_clamp=1,.alpha_clamp=1};
    return s;
}
static void verify(const FhMaterial *p,const float raster[2][4],const float texture[4],const float expected[4]) {
    float vertex[4];fh_material_color(vertex,p,raster);
    if(!p->sphere_lighting) {
        FhMaterial full=*p;full.sparse_colors=0;float reference[4];fh_material_color(reference,&full,raster);
        for(unsigned c=0;c<4;c++)assert(!memcmp(vertex+c,reference+c,4));
    }
    for(unsigned c=0;c<4;c++) {
        float actual=clamp(clamp(vertex[c])*(p->textured[c]?texture[c]:1)*p->scale[c==3]+p->bias[c]);
        if(p->sphere_lighting) actual=c==3?0:clamp(texture[c]+clamp(2*vertex[c]));
        if(fabsf(actual-clamp(expected[c]))>0.00001f) { fprintf(stderr,"component %u actual %.8f expected %.8f\n",c,actual,expected[c]);assert(0); }
        checks++;
    }
}
int main(void) {
    for(unsigned mode=0;mode<5;mode++) for(unsigned sample=0;sample<10000;sample++) {
        FhDrawState s=state();float raster[2][4],texture[4],expected[4];
        for(unsigned i=0;i<8;i++) ((float*)raster)[i]=random_unit();
        for(unsigned i=0;i<4;i++) { texture[i]=random_unit();s.konst_colors[0][i]=random_unit(); }
        if(mode==1 || mode==2) {
            s.tev[0].color[1]=14;s.tev[0].color[2]=8;s.tev[0].kcolor=12;
            s.tev[0].alpha[1]=4;s.tev[0].alpha[2]=6;s.tev[0].kalpha=28;
            if(mode==2) {s.konst_colors[0][1]=s.konst_colors[0][2]=0;s.tev[0].alpha_scale=2;}
        }
        if(mode==3) {
            s.stages=2;s.tev[1]=s.tev[0];
            s.tev[1].color[1]=0;s.tev[1].color[2]=14;s.tev[1].kcolor=12;
            s.tev[1].alpha[1]=0;s.tev[1].alpha[2]=6;s.tev[1].kalpha=28;
        }
        if(mode==4) { s.tev[0].color_scale=2;s.tev[0].alpha_scale=1; }
        for(unsigned c=0;c<4;c++) {
            expected[c]=texture[c]*raster[0][c];
            if(mode==1 || mode==2) expected[c]=texture[c]*s.konst_colors[0][c]*(mode==2 && c==3?4:1);
            if(mode==3) expected[c]*=s.konst_colors[0][c];
            if(mode==4) expected[c]*=c==3?2:4;
        }
        FhMaterial p;assert(fh_material_compile(&p,&s));verify(&p,raster,texture,expected);
    }
    FhMaterial p;FhDrawState s=state();s.stages=2;s.tev[1]=s.tev[0];s.tev[1].color[2]=0;s.tev[1].alpha[2]=0;
    assert(!fh_material_compile(&p,&s)); /* T * T * R cannot be moved into vertex color. */
    s=state();s.tev[0].color_scale=2;s.stages=2;s.tev[1]=s.tev[0];s.tev[1].color[1]=0;s.tev[1].color[2]=13;
    assert(!fh_material_compile(&p,&s)); /* Intermediate saturation must remain before halving. */
    s=state();s.tev[0].indirect[3]=1;assert(!fh_material_compile(&p,&s));
    for(unsigned i=0;i<10000;i++) {
        s=state();s.stages=2;s.constant_raster_mask=255;
        s.tev[0].color[1]=10;s.tev[0].color[2]=11;s.tev[0].color[3]=8;
        s.tev[1]=s.tev[0];s.tev[1].color[3]=0;s.tev[1].channel=5;
        s.tev[0].alpha[1]=s.tev[0].alpha[2]=7;s.tev[1].alpha[1]=s.tev[1].alpha[2]=7;
        float texture[4],expected[4];
        for(unsigned c=0;c<8;c++) ((float*)s.constant_raster)[c]=random_unit();
        for(unsigned c=0;c<4;c++) { texture[c]=random_unit();expected[c]=c==3?0:clamp(clamp(texture[c]+s.constant_raster[0][c]*s.constant_raster[0][3])+s.constant_raster[1][c]*s.constant_raster[1][3]); }
        assert(fh_material_compile(&p,&s));verify(&p,s.constant_raster,texture,expected);
    }
    s=state();assert(fh_material_compile(&p,&s) && p.sparse_colors);
    const unsigned exceptional[]={0x80000000u,0x7f800000u,0xff800000u,0x7fc00001u,0xbf000000u,0x40000000u};
    for(unsigned which=0;which<8;which++)for(unsigned k=0;k<sizeof(exceptional)/sizeof(*exceptional);k++) {
        float raster[2][4],actual[4],expected[4];
        for(unsigned c=0;c<8;c++)((float*)raster)[c]=.25f;
        memcpy((float*)raster+which,exceptional+k,4);
        FhMaterial full=p;full.sparse_colors=0;
        fh_material_color(actual,&p,raster);fh_material_color(expected,&full,raster);
        for(unsigned c=0;c<4;c++){assert((isnan(actual[c]) && isnan(expected[c])) || !memcmp(actual+c,expected+c,4));checks++;}
    }
    unsigned maximum_leaves=0;
    for(unsigned sample=0;sample<1000;sample++) {
        FhVertex triangle[3]={0};
        for(unsigned v=0;v<3;v++) for(unsigned c=0;c<8;c++) ((float*)triangle[v].color)[c]=random_unit();
        unsigned leaves=fh_material_subdivide(triangle,verify_triangle,NULL);
        assert(leaves<=256);if(leaves>maximum_leaves) maximum_leaves=leaves;
    }
    printf("PASS %u TEV components, %u interpolated-lighting samples, max %u subdivision leaves; nonlinear and intermediate clamp rejection\n",checks,interpolation_checks,maximum_leaves);
}
