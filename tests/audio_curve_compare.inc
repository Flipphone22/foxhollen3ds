static unsigned seed=41;
static unsigned random_word(void){seed=seed*1664525u+1013904223u;return seed;}
static unsigned checks;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %u\n",__LINE__);return 1;}checks++;}while(0)
int main(void){
 FhAudioDspCurve a,b;
 for(unsigned n=0;n<100000;n++){
  memset(&a,0,sizeof a);a.rate=32000;a.env_start=a.env_end=(uint16_t)random_word();
  for(unsigned i=0;i<9;i++)a.volume_start[i]=a.volume_end[i]=(uint16_t)random_word();
  b=a;
  CHECK(prior_same_constant_curve(&a,&b)==candidate_same_constant_curve(&a,&b));
  CHECK(prior_constant_curve(&a)==candidate_constant_curve(&a));
  for(unsigned lane=0;lane<9;lane++){
   for(unsigned which=0;which<4;which++){
    uint16_t *p=which==0?&a.volume_start[lane]:which==1?&a.volume_end[lane]:which==2?&b.volume_start[lane]:&b.volume_end[lane];
    *p^=1u<<(n%16);
    CHECK(prior_same_constant_curve(&a,&b)==candidate_same_constant_curve(&a,&b));
    CHECK(prior_constant_curve(&a)==candidate_constant_curve(&a));
    CHECK(prior_constant_curve(&b)==candidate_constant_curve(&b));
    *p^=1u<<(n%16);
   }
  }
  unsigned field=n%9;
  if(field==0)b.rate=-a.rate;
  if(field==1)b.interpolation++;
  if(field==2)b.paused++;
  if(field==3)b.routing++;
  if(field==4)b.env_start++;
  if(field==5)b.env_end++;
  if(field==6)b.rate=NAN;
  if(field==7)a.rate=b.rate=0;
  if(field==8)b.rate=-0.0f;
  CHECK(prior_same_constant_curve(&a,&b)==candidate_same_constant_curve(&a,&b));
 }
 printf("PASS %u exact comparison cases\n",checks);return 0;
}
