#include "fh_pica_receiver_plane.h"
#include "fh_pica_planar_shadow.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
static float q(float x){uint32_t u;memcpy(&u,&x,4);u&=~127u;memcpy(&x,&u,4);return x;}
static unsigned seed=99179;
static float randomf(void){seed=seed*1664525u+1013904223u;return (seed>>8)/8388608.f-1;}
static double depth(const float p[4][4],const float n[3],double d,double x,double y){
 double rx=(p[3][2]*y-p[1][2])/p[1][0],ry=(p[3][2]*x-p[0][2])/p[0][1];
 return -(p[2][2]+p[2][3]*(n[0]*rx+n[1]*ry+n[2])/(-d))/p[3][2];
}
int main(void){
 const float cam[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
 const float p[4][4]={{0,1.8f,0,0},{-1.3f,0,0,0},{0,0,-.001001001f,-10.01001f},{0,0,-1,0}};
 unsigned valid=0,rejected[4]={0};double maximum[4]={0};
 for(unsigned i=0;i<50000;i++){
  float n[3]={randomf(),randomf(),randomf()},l=sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(l<.1f)continue;
  for(unsigned c=0;c<3;c++)n[c]/=l;
  float d=5+fabsf(randomf())*2000,point[3],view[3][4];for(unsigned c=0;c<3;c++)point[c]=-d*n[c];
  assert(fh_pica_planar_shadow_view(view,cam,point,n,n));FhPicaReceiverPlane a;assert(fh_pica_receiver_plane(&a,view,p,0,1));
  FhPicaReceiverPlane b=a;float s=q(a.depth[0]),o=q(a.depth[1]);
  for(unsigned c=0;c<3;c++)b.equation[c]=(double)a.depth[0]*a.equation[c]/s;
  b.equation[3]=((double)a.depth[0]*a.equation[3]+a.depth[1]-o)/s;
  b.depth[0]=s;b.depth[1]=o;
  for(unsigned j=0;j<16;j++){
   float x=randomf(),y=randomf();double ground=depth(p,n,d+.1,x,y),near=depth(p,n,d,x,y),far=depth(p,n,d+.25,x,y);
   if(!(ground>0 && near<1 && far<ground && ground<near))continue;
   valid++;
   float za=a.equation[0]*x+a.equation[1]*y+a.equation[3];
   float zb=b.equation[0]*x+b.equation[1]*y+b.equation[3];
   float zqa=q(a.equation[0])*x+q(a.equation[1])*y+q(a.equation[3]);
   float zqb=q(b.equation[0])*x+q(b.equation[1])*y+q(b.equation[3]);
   double result[4]={s*za+o,s*zb+o,s*zqa+o,s*zqb+o};
   for(unsigned c=0;c<4;c++){if(!(result[c]<ground))rejected[c]++;double err=fabs(result[c]-far);if(err>maximum[c])maximum[c]=err;}
  }
 }
 printf("samples=%u rejected old/map-compensated/old+uniform-quantized/compensated+uniform-quantized=%u/%u/%u/%u maxerror=%.9g/%.9g/%.9g/%.9g\n",valid,rejected[0],rejected[1],rejected[2],rejected[3],maximum[0],maximum[1],maximum[2],maximum[3]);
 return !valid;
}
