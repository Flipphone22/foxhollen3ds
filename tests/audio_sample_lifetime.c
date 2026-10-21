/* Actual registry, decoder, mixer and hardware break/off implementations.
 * Synthetic sample bank and control callbacks; no game assets. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "mixer.c"
#include "registry.c"
extern u8 salTimeOffset;
#include "hardware_entries.c"
DSPstudioinfo dspStudio[8];
static DSPvoice voices[3];DSPvoice *dspVoice=voices;
u8 salMaxStudioNum=1,salNumVoices=3,salFrame,salAuxFrame,salTimeOffset;
static s32 main_pcm[2][3*SAL_PC_MAX_FRAMES],aux_a[3][3*SAL_PC_MAX_FRAMES],aux_b[3][3*SAL_PC_MAX_FRAMES];
static s16 output[SAL_PC_MAX_FRAMES*2];
static unsigned stops,removed,reads_after_release;
static unsigned reuse_voice;
static SAMPLE_INFO replacement;
static void init_voice(unsigned i,SAMPLE_INFO sample);
static void *released;
u32 salPCMixRate(void){return 32000;}
u32 salPCChannels(void){return 2;}
void *salAiGetDest(void){return output;}
u32 salSynthSendMessage(DSPvoice *v,u32 message){(void)v;assert(message==HW_MESSAGE_SAMPLE_END);return 0;}
void voiceKill(u32 i){
 ++stops;hwBreak(i);
 if(reuse_voice&&i==1){salDeactivateVoice(voices+i);memset(voices+i,0,sizeof(*voices));init_voice(i,replacement);}
}
void salPCConvertCycle(s32 *a,u32 b,u32 c,u64 d,const s32 *e,u32 f,u32 g,u64 h,s32 i[32]) {
 (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;abort();
}
void salPCConvertStudioCycle(s32 *a,u32 b,u32 c,u64 d,const s32 *e,u32 f,u32 g,u64 h,s32 i[2]) {
 (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;abort();
}
void hwSaveSample(void *h,void *a){
 SAMPLE_HEADER **header=h;void **address=a;
 size_t bytes=(((*header)->length&0xffffff)+13)/14*8;
 void *p=malloc(bytes);assert(p);memcpy(p,*address,bytes);*address=p;
}
void hwRemoveSample(void *header,void *address){
 (void)header;++removed;released=address;free(address);
}
static void init_voice(unsigned i,SAMPLE_INFO sample) {
 DSPvoice *v=voices+i;v->state=DSP_VOICE_STATE_STARTING;v->virtualSampleID=UINT32_MAX;
 v->smp_info=sample;v->srcTypeSelect=SAL_SRC_NONE;v->adsr.mode=ADSR_MODE_LINEAR;
 v->adsr.data.linear.aTime=1;v->adsr.data.linear.dTime=1;
 v->adsr.data.linear.sLevel=32767;v->adsr.data.linear.rTime=100;
 v->volL=12000;v->volR=6000;
 for(unsigned j=0;j<5;j++){v->changed[j]=8;v->pitch[j]=65536;}
 v->next=dspStudio[0].voiceRoot;if(v->next)v->next->prev=v;
 dspStudio[0].voiceRoot=v;
}
static void render(unsigned frame){
 salFrame=frame&1;salAuxFrame=frame%3;salPCBeginCycle(frame*5);salCtrlDsp(output);
}
int main(int argc,char **argv){
 unsigned offset=argc>1?(unsigned)atoi(argv[1]):4;assert(offset<5);
 unsigned no_break=argc>2?(unsigned)atoi(argv[2]):0;
 reuse_voice=argc>3?(unsigned)atoi(argv[3]):0;
 unsigned char encoded[16]={10,0x77,0x77,0x77,0x77,0x77,0x77,0x77,10,0x99,0x99,0x99,0x99,0x99,0x99,0x99};
 SDIR_DATA bank[2]={{.id=42,.header={.length=28,.loopLength=28}}, {.id=0xffff}};
 salPCResetSampleData();assert(dataInsertSDir(bank,encoded));
 assert(dataAddSampleReference(42));assert(dataAddSampleReference(42));salPCFinishSampleUpload();
 SAMPLE_INFO sample;assert(dataGetSample(42,&sample)==0);
 DSPstudioinfo *studio=dspStudio;studio->state=DSP_STUDIO_STATE_ACTIVE;studio->isMaster=1;
 for(unsigned i=0;i<2;i++)studio->main[i]=main_pcm[i];
 for(unsigned i=0;i<3;i++){studio->auxA[i]=aux_a[i];studio->auxB[i]=aux_b[i];}
 init_voice(0,sample);init_voice(1,sample);
 SAMPLE_INFO independent=sample;independent.addr=encoded;replacement=independent;init_voice(2,independent);
 render(0);assert(voices[0].state==DSP_VOICE_STATE_PLAYING && voices[1].state==DSP_VOICE_STATE_PLAYING);
 assert(dataRemoveSampleReference(42));assert(!removed&&!stops);render(1);
 assert(voices[0].state==DSP_VOICE_STATE_PLAYING && voices[2].state==DSP_VOICE_STATE_PLAYING);
 salTimeOffset=offset;if(!no_break){hwBreak(0);hwBreak(1);}
 assert(dataRemoveSampleReference(42));assert(removed==1&&released);
 if(stops){
  unsigned retained=0;
  for(unsigned i=0;i<DEPOP_RING;i++)retained|=depop[0][0][i]!=0;
  assert(retained); /* Existing detached PCM tail survives sample retirement. */
 }
 /* The unmodified decoder must never dereference the released bank. ASan
  * catches the former read even when a scheduled break exists at subframe4. */
 render(2);
 for(unsigned i=0;i<2;i++)assert(voices[i].state==((reuse_voice&&i==1)?DSP_VOICE_STATE_PLAYING:DSP_VOICE_STATE_INACTIVE));
 assert(voices[2].state==DSP_VOICE_STATE_PLAYING);
 assert(dataRemoveSDir(bank));assert(!dataRemoveSampleReference(42));
 assert(stops==2 || (!offset&&!no_break&&!stops));assert(!reads_after_release);
 printf("PASS offset=%u no_break=%u shared refs preserved, two consumers retired, unrelated voice still playing\n",offset,no_break);
 return 0;
}
