#include "fh_indirect_lighting.h"
#include "fh_fragment.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned seed=147,count;
static float random_unit(void){seed=seed*1664525+1013904223;return (seed>>8)*(1.f/16777216);}
static double area_sum;
static double area(const FhVertex *v){
    double x[3],y[3];for(unsigned i=0;i<3;i++){x[i]=v[i].position[0]/v[i].position[3];y[i]=v[i].position[1]/v[i].position[3];}
    return ((x[1]-x[0])*(y[2]-y[0])-(y[1]-y[0])*(x[2]-x[0]))*.5;
}
static void sink(const FhVertex v[3],void *unused){(void)unused;count++;assert(count<=256);area_sum+=area(v);}
int main(void){
    FhDrawState s={0},baked;FhMaterial plan;unsigned char pixels[3][8*8*4];
    for(unsigned i=0;i<sizeof(pixels);i++)((unsigned char*)pixels)[i]=(unsigned)(random_unit()*256);
    for(unsigned i=0;i<3;i++)s.textures[i]=(FhTexture){.pixels=pixels[i],.width=8,.height=8,.format=0x46,.mag_filter=1,.wrap_s=1,.wrap_t=1};
    static const unsigned ind[4][9]={{0,0,3,5,6,6,0,0,0},{0,0,3,9,6,6,1,0,0},{0,0,0,0,0,0,1,0,0},{0}};
    static const unsigned color[4][4]={{15,15,15,10},{15,15,15,0},{15,8,14,10},{15,8,4,6}};
    static const unsigned alpha[4][4]={{7,7,7,5},{7,7,7,0},{7,7,7,7},{7,4,6,7}};
    s.stages=4;s.texgens=5;s.indirect_count=1;s.indirect_order[0][0]=2;
    for(unsigned i=0;i<4;i++){
        FhTevStage *t=s.tev+i;memcpy(t->indirect,ind[i],sizeof(ind[i]));memcpy(t->color,color[i],16);memcpy(t->alpha,alpha[i],16);
        t->color_clamp=t->alpha_clamp=1;t->color_dest=i==2?2:0;t->coord=i==3?4:i;t->map=i<2?257:i==2?1:2;t->channel=i==2?4:255;
        for(unsigned j=0;j<4;j++)s.swaps[i][j]=j;
    }
    s.alpha_compare[0]=s.alpha_compare[3]=7;
    for(unsigned trial=0;trial<4096;trial++){
        s.textures[0].width=trial&1?8:4;s.textures[1].height=trial&2?8:4;
        s.textures[0].mag_filter=!!(trial&4);s.textures[0].wrap_s=trial%3;
        s.indirect_exponent[0]=(int)(trial%9)-4;
        for(unsigned c=0;c<4;c++){s.konst_colors[0][c]=random_unit();s.tev_colors[3][c]=random_unit()*.2f;}
        assert(fh_indirect_lighting_plan(&plan,&baked,&s));
        FhVertex v={.matrix_index=-1},out;v.position[3]=1;
        for(unsigned i=0;i<8;i++){v.uv[i][0]=(random_unit()-.5f)*3;v.uv[i][1]=(random_unit()-.5f)*3;v.uv[i][2]=.5f+random_unit()*2;}
        for(unsigned c=0;c<4;c++)v.color[0][c]=random_unit()*.3f;
        assert(fh_indirect_lighting_vertex(&out,&v,&s));
        float reference[4],candidate[4];assert(fh_fragment_evaluate(reference,&s,&v));assert(fh_fragment_evaluate(candidate,&baked,&out));
        for(unsigned c=0;c<4;c++)assert(fabsf(reference[c]-candidate[c])<1e-6f);
        assert(!memcmp(v.uv[4],out.uv[4],sizeof(v.uv[4])));
    }
    FhVertex v[3]={0};
    for(unsigned i=0;i<3;i++){v[i].matrix_index=-1;v[i].position[3]=1;v[i].position[0]=i==0?-1:i==1?1:0;v[i].position[1]=i==2?1:-1;for(unsigned c=0;c<8;c++)v[i].uv[c][2]=1;}
    assert(fh_indirect_lighting_mesh(v,&s,sink,NULL));assert(count==1);assert(fabs(area_sum-area(v))<1e-6);
    count=0;area_sum=0;fh_indirect_lighting_detail=4;
    assert(fh_indirect_lighting_mesh(v,&s,sink,NULL));assert(count==256);assert(fabs(area_sum-area(v))<1e-6);
    count=0;v[1].uv[0][2]=-1;assert(!fh_indirect_lighting_mesh(v,&s,sink,NULL));assert(!count);
    s.tev[0].indirect[4]=1;assert(!fh_indirect_lighting_plan(&plan,&baked,&s));s.tev[0].indirect[4]=6;
    s.tev[2].channel=7;s.tev[2].indirect[8]=1;assert(!fh_indirect_lighting_plan(&plan,&baked,&s));
    /* Different stage programs and coordinate chains, including disabled
     * sampling, non-square SU scales, multiple indirect inputs and direct
     * coordinates shared with remapped stages. The original scalar evaluator
     * independently samples every stage; do not compare our own UV formula. */
    unsigned admitted=0;
    for(unsigned trial=0;trial<8192;trial++){
        memset(&s,0,sizeof(s));s.stages=3;s.texgens=5;s.indirect_count=2;
        s.alpha_compare[0]=s.alpha_compare[3]=7;
        for(unsigned i=0;i<3;i++){
            s.textures[i]=(FhTexture){.pixels=pixels[i],.width=trial&1?8:4,.height=trial&2?4:8,.format=0x46,
                .mag_filter=!!(trial&4),.wrap_s=trial%3,.wrap_t=(trial/3)%3};
            for(unsigned j=0;j<4;j++)s.swaps[i][j]=j;
            s.indirect_exponent[i]=(int)(trial%7)-3;
            for(unsigned a=0;a<2;a++)for(unsigned b=0;b<3;b++)s.indirect_matrix[i][a][b]=(random_unit()-.5f)*.1f;
            FhTevStage *t=s.tev+i;t->coord=i?2:0;t->map=i?i:257;t->channel=255;
            for(unsigned j=0;j<4;j++){t->color[j]=15;t->alpha[j]=7;}
            t->color[3]=i?8:12;t->alpha[3]=i?4:6;t->color_clamp=t->alpha_clamp=1;
            if(i==2){t->color[1]=8;t->color[2]=0;t->color[3]=15;t->alpha[1]=4;t->alpha[2]=0;t->alpha[3]=7;}
            t->indirect[0]=i%2;t->indirect[1]=(trial>>3)%4;t->indirect[2]=(trial>>5)%8;
            static const unsigned selections[]={1,2,3,5,6,7,9,10,11};
            t->indirect[3]=selections[(trial+i)%9];
            t->indirect[4]=trial&256?6:0;t->indirect[5]=trial&512?6:0;t->indirect[6]=i && (trial&1024);
        }
        for(unsigned i=0;i<2;i++){
            s.indirect_order[i][0]=i;s.indirect_order[i][1]=i;
            s.indirect_scale[i][0]=trial%4;s.indirect_scale[i][1]=(trial/4)%4;
        }
        if(trial&2048)memset(s.tev[2].indirect,0,sizeof(s.tev[2].indirect));
        FhIndirectLighting p;assert(fh_indirect_lighting_prepare(&p,&s));
        if(!fh_indirect_lighting_plan(&plan,&baked,&s))continue;
        admitted++;
        FhVertex in={.matrix_index=-1},out;
        for(unsigned i=0;i<8;i++){
            in.uv[i][0]=(random_unit()-.5f)*2;in.uv[i][1]=(random_unit()-.5f)*2;in.uv[i][2]=.5f+random_unit();
        }
        assert(fh_indirect_lighting_vertex_prepared(&out,&in,&s,&p));
        float reference[4],candidate[4];assert(fh_fragment_evaluate(reference,&s,&in));assert(fh_fragment_evaluate(candidate,&baked,&out));
        for(unsigned j=0;j<4;j++)assert(fabsf(reference[j]-candidate[j])<1e-6f);
        if(trial&2048)assert(!memcmp(in.uv[2],out.uv[2],sizeof(in.uv[2])));
    }
    assert(admitted==8192);
    memset(&s,0,sizeof(s));s.stages=s.texgens=3;s.indirect_count=2;
    static const unsigned chain_color[3][4]={{15,15,15,10},{15,15,15,8},{15,15,15,0}};
    static const unsigned chain_alpha[3][4]={{7,7,7,5},{7,7,7,0},{7,4,6,0}};
    static const unsigned chain_ind[3][9]={{0,0,7,1,0,0,0,0,0},{1,0,7,1,0,0,1,0,0},{0}};
    for(unsigned i=0;i<3;i++){
        FhTevStage *t=s.tev+i;memcpy(t->color,chain_color[i],16);memcpy(t->alpha,chain_alpha[i],16);memcpy(t->indirect,chain_ind[i],36);
        t->color_clamp=t->alpha_clamp=1;t->alpha_op=i==2;t->coord=i;t->map=i==2?1:0;t->channel=i?255:4;
        for(unsigned c=0;c<4;c++)s.swaps[i][c]=c;
    }
    s.tev[2].kalpha=28;s.konst_colors[0][3]=.6f;
    for(unsigned i=0;i<2;i++){
        s.textures[i]=(FhTexture){.pixels=pixels[i],.width=8,.height=8,.format=0x46,.mag_filter=1,.min_filter=1,.wrap_s=1,.wrap_t=1};
        s.indirect_order[i][0]=i;s.indirect_order[i][1]=i;
    }
    assert(fh_chained_material(&s));assert(fh_indirect_lighting_plan(&plan,&baked,&s));
    assert(plan.pipeline.count==2 && plan.pipeline.coord[0]==baked.tev[1].coord && plan.pipeline.coord[1]==2);
    for(unsigned trial=0;trial<2048;trial++){
        FhVertex in={.matrix_index=-1},out;in.color[0][3]=random_unit();
        for(unsigned c=0;c<3;c++){in.uv[c][0]=random_unit();in.uv[c][1]=random_unit();in.uv[c][2]=1+random_unit()*8;}
        assert(fh_indirect_lighting_vertex(&out,&in,&s));
        float ref[4],got[4];assert(fh_fragment_evaluate(ref,&s,&in));assert(fh_fragment_evaluate(got,&baked,&out));
        for(unsigned c=0;c<4;c++)assert(fabsf(ref[c]-got[c])<1e-6f);
        assert(!memcmp(in.uv[2],out.uv[2],sizeof(in.uv[2])));
    }
    printf("PASS 4096 original and %u generalized indirect vertex/oracle comparisons; direct UV, bounded coverage and rejection\n",admitted);
    puts("PASS 2048 chained material coordinate/opacity oracle comparisons and specialized pipeline binding");
}
