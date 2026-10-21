#include "fh_material.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned seed=7419;
static float random_unit(void){seed=seed*1664525+1013904223;return (seed>>8)*(1.f/16777216);}
static double area(const FhVertex *v){
    double x[3],y[3];for(unsigned i=0;i<3;i++){x[i]=v[i].position[0]/v[i].position[3];y[i]=v[i].position[1]/v[i].position[3];}
    return (x[1]-x[0])*(y[2]-y[0])-(y[1]-y[0])*(x[2]-x[0]);
}
int main(void){
    FhDrawState s={0};s.textures[0].width=s.textures[0].height=32;
    struct {uint64_t before;FhVertex data[384];uint64_t after;} guarded={.before=0x789abcdef,.after=0x456789abc};
    unsigned checks=0,accepted=0,fallback=0;double maximum=0;
    for(unsigned trial=0;trial<2048;trial++){
        FhVertex input[3]={0};
        for(unsigned i=0;i<3;i++){
            input[i].position[0]=i==0?-1:i==1?1:0;input[i].position[1]=i==2?1:-1;
            input[i].position[2]=-.5f;input[i].position[3]=1+(trial&1)*random_unit();
            input[i].uv[2][2]=1;input[i].color[0][3]=random_unit();
            for(unsigned k=0;k<2;k++){
                input[i].uv[k][0]=(random_unit()-.5f)*.75f;
                input[i].uv[k][1]=(random_unit()-.5f)*.75f;
                input[i].uv[k][2]=1+k*4+random_unit()*(trial%3)*.25f;
            }
        }
        if(trial>=128){
            s.textures[0].width=s.textures[0].height=1u<<(trial%7);
            for(unsigned i=0;i<3;i++)for(unsigned k=0;k<2;k++){
                input[i].uv[k][0]=(random_unit()-.5f)*4;
                input[i].uv[k][1]=(random_unit()-.5f)*4;
                input[i].uv[k][2]=(trial%4==0)?1.f:(trial%4==1)?1.f+k*7:1.f+random_unit()*3;
            }
            /* Correlated and nearly singular denominator edge products. */
            if(trial%4==3)for(unsigned i=0;i<3;i++)input[i].uv[1][2]=input[i].uv[0][2]+1e-5f;
        }
        if(trial>=512){
            /* Large/oppositely varying Q, cancelling coordinates and constant
             * true coordinates. Check the emitted rational values directly,
             * independently of the renderer's subdivision error formula. */
            for(unsigned i=0;i<3;i++){
                float q=1+random_unit()*15;
                input[i].uv[0][2]=q;
                input[i].uv[1][2]=trial%3==0?17-q:1+random_unit()*15;
                for(unsigned k=0;k<2;k++)for(unsigned c=0;c<2;c++){
                    float ratio=trial%3==1?(k?-.375f:.5f):(random_unit()-.5f);
                    input[i].uv[k][c]=ratio*input[i].uv[k][2];
                }
            }
        }
        unsigned count=fh_chained_triangle(guarded.data,input,&s);
        assert(guarded.before==0x789abcdef && guarded.after==0x456789abc);
        if(!count){fallback++;continue;}accepted++;assert(count<=384 && !(count%3));
        double total=0;
        for(unsigned n=0;n<count;n+=3){
            FhVertex *v=guarded.data+n;double a=area(v);assert(a>=0);total+=a;
            for(unsigned sample=0;sample<40;sample++){
                double weights[3]={random_unit(),random_unit(),random_unit()},sum=weights[0]+weights[1]+weights[2];
                for(unsigned i=0;i<3;i++)weights[i]/=sum;
                if(sample>=32){
                    unsigned edge=sample%3;
                    weights[edge]=sample<35?1:sample<38?.5:1-1e-7;
                    weights[(edge+1)%3]=1-weights[edge];weights[(edge+2)%3]=0;
                }
                double uv[2][3]={{0}},approx[3]={0};
                for(unsigned i=0;i<3;i++)for(unsigned c=0;c<3;c++){
                    approx[c]+=v[i].uv[7][c]*weights[i];
                    for(unsigned k=0;k<2;k++)uv[k][c]+=v[i].uv[k][c]*weights[i];
                }
                for(unsigned c=0;c<2;c++){
                    double exact=uv[0][c]/uv[0][2]+uv[1][c]/uv[1][2];
                    double error=fabs(approx[c]/approx[2]-exact)*s.textures[0].width;
                    if(error>maximum)maximum=error;assert(error<=1.0/8);checks++;
                }
            }
        }
        assert(fabs(total-area(input))<1e-5);checks++;
        if(trial%16==0){
            FhVertex reference[384];memcpy(reference,guarded.data,count*sizeof(*reference));
            const unsigned prefixes[]={3,381,384};
            for(unsigned p=0;p<3;p++){
                unsigned prefix=prefixes[p];memset(guarded.data,0xa5,sizeof(guarded.data));
                unsigned appended=fh_chained_triangle_append(guarded.data,prefix,input,&s);
                const unsigned char *bytes=(const unsigned char*)guarded.data;
                for(unsigned b=0;b<prefix*sizeof(FhVertex);b++)assert(bytes[b]==0xa5);
                if(prefix+count<=384){
                    assert(appended==prefix+count);
                    assert(!memcmp(guarded.data+prefix,reference,count*sizeof(FhVertex)));
                }else assert(appended==385);
                assert(guarded.before==0x789abcdef && guarded.after==0x456789abc);checks++;
            }
            assert(!fh_chained_triangle_append(guarded.data,1,input,&s));
            assert(!fh_chained_triangle_append(guarded.data,385,input,&s));checks+=2;
        }
        input[0].uv[0][2]=.5f;assert(!fh_chained_triangle(guarded.data,input,&s));checks++;
    }
    assert(accepted>=80);
    printf("PASS %u coordinate/coverage checks; accepted=%u fallback=%u max_texel_error=%.9f\n",checks,accepted,fallback,maximum);
}
