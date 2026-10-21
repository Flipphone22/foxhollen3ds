#include "fh_movie_disjoint.h"
#include <stdio.h>
#include <stdlib.h>
static unsigned checks,accepted,seed=731;
#define CHECK(v) do{checks++;if(!(v)){fprintf(stderr,"failed line %d\n",__LINE__);exit(1);}}while(0)
static float random_coord(void){seed=1664525u*seed+1013904223u;return ((int)(seed>>16)-32768)/32768.f;}
static double side(const FhVertex *a,const FhVertex *b,const FhVertex *c){
    return ((double)b->position[0]-a->position[0])*((double)c->position[1]-a->position[1])-
           ((double)b->position[1]-a->position[1])*((double)c->position[0]-a->position[0]);
}
static int interior(const FhVertex *a,const FhVertex *p){
    double x=side(a,a+1,p),y=side(a+1,a+2,p),z=side(a+2,a,p);
    return (x>1e-12 && y>1e-12 && z>1e-12)||(x<-1e-12 && y<-1e-12 && z<-1e-12);
}
static int overlap(const FhVertex *a,const FhVertex *b){
    FhVertex center={0};
    for(unsigned axis=0;axis<2;axis++)center.position[axis]=(a[0].position[axis]+a[1].position[axis]+a[2].position[axis])/3;
    if(interior(b,&center))return 1;
    for(unsigned i=0;i<3;i++){
        if(interior(b,a+i)||interior(a,b+i))return 1;
        for(unsigned j=0;j<3;j++){
            const FhVertex *a0=a+i,*a1=a+(i+1)%3,*b0=b+j,*b1=b+(j+1)%3;
            if(side(a0,a1,b0)*side(a0,a1,b1)<-1e-24 && side(b0,b1,a0)*side(b0,b1,a1)<-1e-24)return 1;
        }
    }
    return 0;
}
int main(void){
    FhDrawState s={0};s.viewport[2]=640;s.viewport[3]=528;s.viewport[5]=1;
    for(unsigned i=0;i<4;i++)s.position_matrix[i][i]=1;
    for(unsigned i=0;i<3;i++)s.model_palette[i][i]=1;
    FhVertex v[6]={0};
    for(unsigned i=0;i<6;i++){v[i].position[2]=-.5f;v[i].position[3]=1;}
    for(unsigned raw=0;raw<2;raw++){
        s.raw_position=raw;
        for(unsigned n=0;n<10000;n++){
            for(unsigned i=0;i<6;i++)for(unsigned c=0;c<2;c++)v[i].position[c]=random_coord();
            if(fh_movie_disjoint(v,6,&s)){accepted++;CHECK(!overlap(v,v+3));}
        }
        const float quad[6][2]={{-1,-1},{1,-1},{1,1},{-1,-1},{1,1},{-1,1}};
        for(unsigned i=0;i<6;i++)memcpy(v[i].position,quad[i],8);
        CHECK(fh_movie_disjoint(v,6,&s));
        FhVertex saved=v[5];v[5]=v[1];CHECK(!fh_movie_disjoint(v,6,&s));v[5]=saved;
        v[0].position[3]=0;CHECK(!fh_movie_disjoint(v,6,&s));v[0].position[3]=1;
        v[0].position[0]=NAN;CHECK(!fh_movie_disjoint(v,6,&s));v[0].position[0]=-1;
        v[0].position[1]=INFINITY;CHECK(!fh_movie_disjoint(v,6,&s));v[0].position[1]=-1;
        if(raw){v[0].matrix_index=30;CHECK(!fh_movie_disjoint(v,6,&s));v[0].matrix_index=1;CHECK(!fh_movie_disjoint(v,6,&s));v[0].matrix_index=0;}
    }
    CHECK(accepted>500);CHECK(!fh_movie_disjoint(v,5,&s));CHECK(!fh_movie_disjoint(v,387,&s));
    /* An overlapping or invalid triangle only splits its adjacent groups;
     * it cannot justify changing the order of triangles across that split. */
    FhVertex mixed[12];memcpy(mixed,v,sizeof(v));memcpy(mixed+6,v,sizeof(v));
    unsigned groups[128],n=fh_movie_groups(groups,mixed,12,&s);
    CHECK(n==2 && groups[0]==6 && groups[1]==6);
    mixed[6].position[0]=NAN;n=fh_movie_groups(groups,mixed,12,&s);
    CHECK(n==3 && groups[0]==6 && groups[1]==3 && groups[2]==3);
    printf("checks=%u accepted=%u\n",checks,accepted);return 0;
}
