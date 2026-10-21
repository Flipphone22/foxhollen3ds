#include "fh_graphics.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned rng=0x364573ab;
static float random_value(void){rng=rng*1664525u+1013904223u;return ((int)(rng>>8)-8388608)/1048576.f;}
/* Independent stress model: 1/7/16 format, denormals flushed, rounding after
 * every product and addition. Test truncation, nearest and away-from-zero.
 * Azahar f24 arithmetic currently retains float32; this adds a tighter check
 * without claiming to reproduce all physical PICA arithmetic. */
static float q24(float v,unsigned mode){
 uint32_t b;memcpy(&b,&v,4);unsigned e=(b>>23)&255;
 if(e<=64)return copysignf(0,v);
 if(e>=191)return copysignf(INFINITY,v);
 if(mode==1)b+=64;else if(mode==2 && (b&127))b+=127;
 b&=~127u;memcpy(&v,&b,4);return v;
}
static unsigned oracle(const float p[4],const FhDrawState*s,const float m[4][4],unsigned mode){
 float eye[4]={0,0,0,1},q[4]={0};
 for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)eye[r]=q24(eye[r]+q24(q24(s->model_palette[r][c],mode)*q24(p[c],mode),mode),mode);
 for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)q[r]=q24(q[r]+q24(q24(m[r][c],mode)*eye[c],mode),mode);
 return (q[3]+q[0]<0?1:0)|(q[3]-q[0]<0?2:0)|(q[3]+q[1]<0?4:0)|(q[3]-q[1]<0?8:0)|(q[3]+q[2]<0?16:0)|(-q[2]<0?32:0);
}
#include "fh_bounds_hint.h"
static void test_hint(void){
 FhDrawState s={0};float low[10][3],high[10][3],pica[4][4];unsigned checks=0,hits=0,hint=255;
 for(unsigned trial=0;trial<20000;trial++){
  unsigned used=1+trial%1023;
  for(unsigned row=0;row<30;row++)for(unsigned c=0;c<4;c++)s.model_palette[row][c]=random_value();
  for(unsigned row=0;row<4;row++)for(unsigned c=0;c<4;c++)s.position_matrix[row][c]=random_value();
  s.viewport[2]=640;s.viewport[3]=528;s.viewport[5]=1;
  for(unsigned slot=0;slot<10;slot++)for(unsigned c=0;c<3;c++){
   low[slot][c]=random_value()*100;high[slot][c]=low[slot][c]+fabsf(random_value()*20);
  }
  if(trial&1){
   memset(s.model_palette,0,sizeof(s.model_palette));memset(s.position_matrix,0,sizeof(s.position_matrix));
   for(unsigned slot=0;slot<10;slot++){for(unsigned c=0;c<3;c++)s.model_palette[slot*3+c][c]=1;s.model_palette[slot*3][3]=-1000;}
   for(unsigned c=0;c<4;c++)s.position_matrix[c][c]=1;
  }
  fh_vertex_pica_projection(pica,&s);
  for(unsigned cpu=0;cpu<2;cpu++)for(unsigned repeat=0;repeat<3;repeat++){
   unsigned candidates=cpu?63:255,reused=0;fh_bounds_pair_enabled=repeat&1;
   unsigned original=!!fh_bounds_set_original(low,high,used,&s,cpu?NULL:pica,cpu,candidates);
   unsigned actual=fh_bounds_set_hinted(low,high,used,&s,cpu?NULL:pica,cpu,candidates,&hint,&reused);
   assert(original==actual);checks++;hits+=reused;
  }
  /* A learned rejecting hint must not mask invalid later rows. */
  if(trial%100==1)for(unsigned row=0;row<4;row++)for(unsigned c=0;c<4;c++){
   unsigned seed=0,seed_reused;
   assert(fh_bounds_set_hinted(low,high,used,&s,pica,0,255,&seed,&seed_reused));hint=seed;
   float saved=s.position_matrix[row][c];s.position_matrix[row][c]=NAN;
   unsigned reused,old=!!fh_bounds_set_original(low,high,used,&s,pica,0,255);
   assert(fh_bounds_set_hinted(low,high,used,&s,pica,0,255,&hint,&reused)==old);
   s.position_matrix[row][c]=saved;checks++;
  }
 }
 assert(hits>20000);printf("%u live-input hint/scalar decisions agree; %u hints retested successfully\n",checks,hits);
}
/* Direct pair vs the two public scalar calls, including invalid boxes,
 * nonfinite model/projection lanes, changing palette slots and candidate sets. */
static void test_pair(void){
 unsigned saved_rng=rng,checks=0;FhDrawState s={0};float low[3],high[3],pica[4][4];
 for(unsigned t=0;t<2000;t++){
  unsigned slot=t%10;
  for(unsigned r=0;r<30;r++)for(unsigned c=0;c<4;c++)s.model_palette[r][c]=random_value();
  for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)s.position_matrix[r][c]=random_value();
  s.viewport[0]=random_value()*100;s.viewport[1]=random_value()*100;
  s.viewport[2]=320;s.viewport[3]=264;s.viewport[5]=1;
  for(unsigned c=0;c<3;c++){low[c]=random_value()*100;high[c]=low[c]+fabsf(random_value()*20);}
  fh_vertex_pica_projection(pica,&s);
  switch(t%16){
  case 0:low[t%3]=NAN;break;
  case 1:high[t%3]=INFINITY;break;
  case 2:high[t%3]=low[t%3]-1;break;
  case 3:s.model_palette[slot*3+t%3][t%4]=NAN;break;
  case 4:s.model_palette[slot*3+t%3][t%4]=1e20f;break;
  case 5:s.position_matrix[(t/16)%4][(t/64)%4]=NAN;break;
  case 6:pica[(t/16)%4][(t/64)%4]=INFINITY;break;
  case 7:low[t%3]=high[t%3]=-0.f;break;
  }
  for(unsigned gx=0;gx<16;gx++)for(unsigned gpu=0;gpu<16;gpu++){
   unsigned old=!!fh_vertex_bounds_planes(low,high,slot*3,&s,s.position_matrix,gx) &&
                !!fh_vertex_bounds_planes(low,high,slot*3,&s,pica,gpu);
   assert(old==fh_vertex_bounds_pair(low,high,slot*3,&s,pica,gx,gpu));checks++;
  }
 }
 rng=saved_rng;printf("%u paired/scalar decisions agree\n",checks);
}
int main(void){
 test_pair();
 test_hint();
 FhDrawState state={0};unsigned rejected=0,checks=0;
 for(unsigned trial=0;trial<100000;trial++){
  float p[4]={random_value()*100,random_value()*100,random_value()*100,1},matrix[4][4];
  for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)state.model_palette[r][c]=random_value()*(trial%5?1:10000);
  for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)state.position_matrix[r][c]=random_value();
  state.viewport[0]=random_value()*40;state.viewport[1]=random_value()*20;
  state.viewport[2]=320+random_value()*20;state.viewport[3]=264+random_value()*20;
  state.viewport[4]=0;state.viewport[5]=1;fh_vertex_pica_projection(matrix,&state);
  unsigned mask=fh_vertex_conservative_outside(p,0,&state,matrix);
  for(unsigned mode=0;mode<3;mode++){
   unsigned actual=(oracle(p,&state,state.position_matrix,mode)&15)|((oracle(p,&state,matrix,mode)&15)<<4);
   assert((mask&actual)==mask);checks++;
  }
  if(trial%10==0){
   float low[3],high[3];for(unsigned c=0;c<3;c++){float extent=fabsf(random_value()*10);low[c]=p[c]-extent;high[c]=p[c]+extent;}
   unsigned gx=fh_vertex_bounds_planes(low,high,0,&state,state.position_matrix,63);
   unsigned gpu=fh_vertex_bounds_planes(low,high,0,&state,matrix,15);
   for(unsigned candidates=0;candidates<64;candidates++){
    unsigned full=fh_vertex_bounds_planes(low,high,0,&state,state.position_matrix,candidates);
    unsigned first=fh_vertex_bounds_any_plane(low,high,0,&state,state.position_matrix,candidates);
    assert(!!full==!!first && !(first&~full) && !(first&(first-1)));
   }
   for(unsigned corner=0;corner<8;corner++)for(unsigned mode=0;mode<3;mode++){
    float v[4]={0,0,0,1};for(unsigned c=0;c<3;c++)v[c]=corner&(1u<<c)?high[c]:low[c];
    assert((gx&oracle(v,&state,state.position_matrix,mode))==gx);
    assert((gpu&oracle(v,&state,matrix,mode))==gpu);checks+=2;
   }
  }
  if((mask&15) && (mask&240))rejected++;
 }
 assert(rejected>10000);
 /* A later invalid projection row must prevent the first-plane shortcut. */
 memset(&state,0,sizeof(state));
 for(unsigned i=0;i<3;i++)state.model_palette[i][i]=1;
 for(unsigned i=0;i<4;i++)state.position_matrix[i][i]=1;
 float low[3]={-10,-1,-1},high[3]={-9,1,1};
 assert(fh_vertex_bounds_any_plane(low,high,0,&state,state.position_matrix,15)==1);
 for(unsigned row=0;row<4;row++)for(unsigned col=0;col<4;col++){
  float old=state.position_matrix[row][col];state.position_matrix[row][col]=NAN;
  if(row!=2)assert(!fh_vertex_bounds_any_plane(low,high,0,&state,state.position_matrix,15));
  state.position_matrix[row][col]=old;
 }
 printf("%u float24 stress comparisons; %u conservative rejections; no false outside planes\n",checks,rejected);
 return 0;
}
