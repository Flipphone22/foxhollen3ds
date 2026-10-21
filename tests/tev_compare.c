
#include "fh_fragment.h"
#include "fh_material.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned seed=571;
static unsigned next(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void){
 unsigned checks=0;
 for(unsigned op=8;op<16;op++)for(unsigned alpha=8;alpha<16;alpha++)for(unsigned n=0;n<2048;n++){
  FhDrawState s={0};FhVertex v={0};int value[4][4];float out[4];s.stages=1;
  for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++){value[r][c]=(int)(next()%2048)-1024;s.tev_colors[r][c]=value[r][c]/255.f;}
  if(n%3==0)for(unsigned c=0;c<4;c++){value[2][c]=value[1][c];s.tev_colors[2][c]=s.tev_colors[1][c];}
  s.tev[0]=(FhTevStage){.color={2,4,6,0},.alpha={1,2,3,0},.map=255,.coord=255,.channel=255,
    .color_op=op,.alpha_op=alpha,.color_bias=2,.alpha_bias=1,.color_scale=3,.alpha_scale=2,
    .color_clamp=n&1,.alpha_clamp=n&1,.color_dest=n&3,.alpha_dest=(n>>2)&3};
  assert(fh_fragment_supported(&s));assert(fh_fragment_evaluate(out,&s,&v));
  for(unsigned c=0;c<4;c++){
   unsigned mode=c==3?alpha:op,kind=(mode-8)/2,a=0,b=0;
   if(kind==3){a=value[1][c]&255;b=value[2][c]&255;}
   else for(unsigned k=0;k<=kind;k++){a|=(value[1][k]&255)<<(8*k);b|=(value[2][k]&255)<<(8*k);}
   int pass=mode&1?a==b:a>b,expected=value[0][c]+(pass?(value[3][c]&255):0);
   int lo=n&1?0:-1024,hi=n&1?255:1023;expected=expected<lo?lo:expected>hi?hi:expected;
   assert(fabsf(out[c]-expected/255.f)<1e-6f);checks++;
  }
 }
 /* Last-stage destinations supply final output independently for RGB/alpha. */
 for(unsigned rgb=0;rgb<4;rgb++)for(unsigned alpha=0;alpha<4;alpha++){
  FhDrawState s={0};FhVertex v={0};FhMaterial plan;float scalar[4],compiled[4];s.stages=1;
  for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)s.tev_colors[r][c]=(1+r*4+c)/17.f;
  s.tev[0]=(FhTevStage){.color={15,15,15,2},.alpha={7,7,7,2},.map=255,.coord=255,.channel=255,.color_clamp=1,.alpha_clamp=1,.color_dest=rgb,.alpha_dest=alpha};
  assert(fh_fragment_evaluate(scalar,&s,&v));assert(fh_material_compile(&plan,&s));fh_material_color(compiled,&plan,v.color);
  for(unsigned c=0;c<4;c++){float expected=s.tev_colors[c==3?2:1][c];assert(fabsf(scalar[c]-expected)<1e-6f);assert(fabsf(compiled[c]-expected)<1e-6f);checks+=2;}
 }
 printf("PASS %u TEV comparison and final-register checks\n",checks);
}
