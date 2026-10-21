#include "fh_install_movie_frame.h"
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
enum{PRECISION=22,UNIT=1<<PRECISION};
typedef struct{unsigned first,count;int weights[12];}Kernel;
static double sinc(double x){if(!x)return 1;x*=3.1415926535897932384626433832795;return sin(x)/x;}
static int kernels(Kernel*out,unsigned source,unsigned target){
 int interior[12],have_interior=0;
 for(unsigned i=0;i<target;i++){
  double center=(i+.5)*2;int first=(int)(center-6+.5),end=(int)(center+6+.5);if(first<0)first=0;if(end>(int)source)end=source;
  Kernel*k=out+i;k->first=first;k->count=end-first;if(!k->count||k->count>12)return 0;
  /* Half-size interior pixels have exactly the same 12 sample coordinates.
   * Reuse their proven coefficients instead of repeating double-precision
   * sine/normalization thousands of times per frame. Border kernels retain
   * their original independent normalization. Scratch is private to this call. */
  int full=k->count==12&&center-first==6;
  if(full&&have_interior){for(unsigned j=0;j<12;j++)k->weights[j]=interior[j];continue;}
  double weights[12],sum=0;
  for(unsigned j=0;j<k->count;j++){double x=(j+first-center+.5)*.5;weights[j]=fabs(x)<3?sinc(x)*sinc(x/3):0;sum+=weights[j];}
  if(!isfinite(sum)||sum<=0)return 0;
  int64_t magnitude=0;
  for(unsigned j=0;j<k->count;j++){double w=weights[j]/sum*UNIT;k->weights[j]=(int)(w+(w<0?-.5:.5));magnitude+=llabs(k->weights[j]);}
  /* Both passes consume clamped bytes. Prove every partial accumulator fits
   * signed 32-bit before using ARM's single-word multiply/add. This keeps the
   * exact coefficients, tap order, intermediate clamp and final rounding. */
  if(magnitude*255+UNIT/2>INT32_MAX)return 0;
  if(full){for(unsigned j=0;j<12;j++)interior[j]=k->weights[j];have_interior=1;}
 }return 1;
}
static unsigned char clip(int32_t n){if(n<=0)return 0;n/=UNIT;return n>255?255:(unsigned char)n;}
int fh_install_movie_half_rgb(void*target,unsigned capacity,const void*source,unsigned bytes,FhInstallMovieDecodeProgress progress,void*context){
 enum{W=512,H=384,OW=256,OH=192,INPUT=W*H*3,OUTPUT=OW*OH*3};
 if(!target||!source||bytes!=INPUT||capacity<OUTPUT)return 0;
 uintptr_t a=(uintptr_t)source,b=(uintptr_t)target;
 if(a>UINTPTR_MAX-INPUT||b>UINTPTR_MAX-OUTPUT||(a<b+OUTPUT&&b<a+INPUT))return 0;
 Kernel*k=malloc((OW+OH)*sizeof(*k));unsigned char*middle=malloc(OW*H*3);int result=0;
 if(!k||!middle||!kernels(k,W,OW)||!kernels(k+OW,H,OH))goto done;
 const unsigned char*in=source;unsigned char*out=target;
 if(progress&&!progress(context,0,H+OH)){result=-1;goto done;}
 for(unsigned y=0;y<H;y++){
  for(unsigned x=0;x<OW;x++){
   int32_t red=UNIT/2,green=UNIT/2,blue=UNIT/2;
   const unsigned char*p=in+(y*W+k[x].first)*3;
   for(unsigned j=0;j<k[x].count;j++,p+=3){int w=k[x].weights[j];red+=p[0]*w;green+=p[1]*w;blue+=p[2]*w;}
   unsigned char*q=middle+(y*OW+x)*3;q[0]=clip(red);q[1]=clip(green);q[2]=clip(blue);
  }
  if(progress&&(y+1)%16==0&&!progress(context,y+1,H+OH)){result=-1;goto done;}
 }
 for(unsigned y=0;y<OH;y++){
  const Kernel*row=k+OW+y;
  for(unsigned x=0;x<OW;x++){
   int32_t red=UNIT/2,green=UNIT/2,blue=UNIT/2;
   for(unsigned j=0;j<row->count;j++){const unsigned char*p=middle+((row->first+j)*OW+x)*3;int w=row->weights[j];red+=p[0]*w;green+=p[1]*w;blue+=p[2]*w;}
   unsigned char*q=out+(y*OW+x)*3;q[0]=clip(red);q[1]=clip(green);q[2]=clip(blue);
  }
  if(progress&&(y+1)%16==0&&!progress(context,H+y+1,H+OH)){result=-1;goto done;}
 }result=1;
done:free(k);free(middle);return result;
}
