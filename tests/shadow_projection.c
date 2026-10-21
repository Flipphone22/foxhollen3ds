#include "fh_pica_shadow_projection.h"
#include <assert.h>
#include <stdio.h>
static unsigned seed=190381;
static float rnd(void){seed=seed*1664525u+1013904223u;return (seed>>8)/8388608.f-1;}
static double at(const float m[4],const float p[3]){return (double)m[0]*p[0]+(double)m[1]*p[1]+(double)m[2]*p[2]+m[3];}
int main(void){
 unsigned cases=0;double maximum=0;
 for(unsigned sample=0;sample<32768;sample++){
  float center[3]={rnd()*2000,rnd()*2000,rnd()*2000},light[3]={rnd(),.71f+fabsf(rnd()),rnd()};
  float extent=10+fabsf(rnd())*500,fade=10+fabsf(rnd())*100;
  FhPicaShadowProjection p;assert(fh_pica_shadow_projection(&p,center,light,extent,fade));
  float a=rnd()*3,b=rnd()*3,h=rnd()*60,position[3];
  for(unsigned k=0;k<3;k++)position[k]=center[k]+a*extent*p.capture[0][k]+b*extent*p.capture[1][k]+h*p.capture[2][k];
  double expected[3]={.5+.5*a,.5+.5*b,h/fade};
  for(unsigned k=0;k<3;k++){
   double error=fabs(at(p.texture[k],position)-expected[k]);assert(error<.0001);if(error>maximum)maximum=error;
  }
  double capture[4]={at(p.capture[0],position),at(p.capture[1],position),at(p.capture[2],position),1};
  assert(fabs(capture[2]-(h-400))<.001);
  double clipz=p.projection[2][2]*capture[2]+p.projection[2][3];assert(clipz>=-1&&clipz<=0);
  /* Arbitrary invertible affine camera and receiver transform; recovered UV
   * must match direct world coordinates, including uneven receiver heights. */
  float ang=rnd()*3,c=cosf(ang),s=sinf(ang);
  float camera[3][4]={{c*1.1f,0,s*1.1f,93},{0,.8f,0,-40},{-s,0,c,-500}};
  float world[3][4]={{1,0,0,center[0]},{0,1,0,center[1]},{0,0,1,center[2]}},view[3][4],eye[3][4],local[3][4];
  fh_pica_shadow_concat(view,camera,world);assert(fh_pica_shadow_to_eye(eye,p.texture,camera));fh_pica_shadow_concat(local,eye,view);
  float v[3]={rnd()*40,rnd()*40,rnd()*40},w[3];for(unsigned k=0;k<3;k++)w[k]=v[k]+center[k];
  for(unsigned k=0;k<3;k++){double error=fabs(at(local[k],v)-at(p.texture[k],w));assert(error<.0001);if(error>maximum)maximum=error;}
  cases++;
 }
 FhPicaShadowProjection p;float zero[3]={0},one[3]={0,1,0};
 assert(!fh_pica_shadow_projection(&p,zero,zero,10,10));
 assert(!fh_pica_shadow_projection(&p,zero,one,0,10));assert(!fh_pica_shadow_projection(&p,zero,one,10,NAN));
 float singular[3][4]={{0}},out[3][4];assert(!fh_pica_shadow_to_eye(out,singular,singular));
 for(unsigned a=0;a<256;a++)assert(fh_pica_shadow_source_intensity(a)==a/2+a/4);
 printf("PASS %u capture/finite-receiver/affine-camera cases; source alpha256; max coordinate error %.9g\n",cases,maximum);
}
