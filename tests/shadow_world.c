#include "fh_pica_shadow_projection.h"
#include <assert.h>
#include <float.h>
#include <stdio.h>
static unsigned seed=18397;
static float rnd(void){seed=seed*1664525u+1013904223u;return (seed>>8)/8388608.f-1;}
static double at(const float m[4],const double p[3]){return (double)m[0]*p[0]+(double)m[1]*p[1]+(double)m[2]*p[2]+m[3];}
int main(void){
 double worst=0,oldworst=0;unsigned cases=0;
 for(unsigned n=0;n<65536;n++){
  float a=rnd()*3,c=cosf(a),s=sinf(a),y=(float)(int)(rnd()*300);
  float block[3][4]={{c,0.1f,s,rnd()*8192},{.04f,1.2f,.1f,rnd()*1000},{-s,.15f,c,rnd()*8192}};
  float camera[3][4]={{.9f*c,.1f,1.1f*s,rnd()*8192},{0,.8f,.03f,rnd()*1000},{-s,.07f,c,rnd()*8192}};
  float center[3]={block[0][3]+rnd()*100,block[1][3]+rnd()*100,block[2][3]+rnd()*100};
  float light[3]={rnd(),1.1f,rnd()};FhPicaShadowProjection p;
  assert(fh_pica_shadow_projection(&p,center,light,20+fabsf(rnd())*200,10+fabsf(rnd())*50));
  float world[3][4],local[3][4],view[3][4],corrected[3][4],eye[3][4],previous[3][4];
  fh_pica_shadow_receiver_world(world,block,y);
  fh_pica_shadow_concat(local,p.texture,world);
  fh_pica_shadow_concat(view,camera,block);
  fh_pica_shadow_receiver_world(corrected,view,y);
  assert(fh_pica_shadow_to_eye(eye,p.texture,camera));fh_pica_shadow_concat(previous,eye,corrected);
  /* Live collision points already contain y. Source world projection must
   * remove it exactly once, including its effect through non-axis bases. */
  double point[3]={rnd()*640,rnd()*300+y,rnd()*640},raw[3]={point[0],point[1]-y,point[2]},wp[3];
  for(unsigned k=0;k<3;k++)wp[k]=at(block[k],raw);
  for(unsigned row=0;row<3;row++){
   double expected=at(p.texture[row],wp),direct=at(local[row],point),old=at(previous[row],point);
   double e=fabs(direct-expected),oe=fabs(old-expected);if(e>worst)worst=e;if(oe>oldworst)oldworst=oe;
   /* Absolute texture-coordinate bound at these large map origins, not pixel
    * identity. A missing/duplicated offset or transposed basis fails this. */
   assert(e<.0002);assert(oe<.001);
  }
  /* Changing a shared matrix address must publish fresh contents; aliasing
   * the correction helper itself is explicitly supported. */
  float alias[3][4];memcpy(alias,block,sizeof(alias));fh_pica_shadow_receiver_world(alias,alias,y);
  assert(!memcmp(alias,world,sizeof(world)));
  cases++;
 }
 printf("PASS %u affine block/camera/collision-offset cases; direct max %.9g, inverse-roundtrip max %.9g\n",cases,worst,oldworst);
}
