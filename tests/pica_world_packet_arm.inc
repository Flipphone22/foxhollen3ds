/* Appended to the existing independent masked-register/float-uniform oracle. */
static Device snapshots[16],expected_snapshots[16];static unsigned snapshot_count;
static void record_draw(const Device *d){
 CHECK(snapshot_count<16);snapshots[snapshot_count]=*d;
 snapshots[snapshot_count].cache_clears=0;snapshots[snapshot_count].draws=0;snapshot_count++;
}
static FhPicaWorldPacket packet;
static uint32_t packet_words[3072] __attribute__((aligned(16))),packet_before[3072],reference_words[3072];
static FhPicaDrawLive lives[16];static FhPicaSamplerCommands packet_samplers[8][2];
static const FhPicaResidentDraw *draw_refs[16];static uint32_t alternate_effects[16][18];
static FhPicaWorldPacket saved_packet;
volatile unsigned packet_draws,packet_updates,packet_zero_updates,packet_writes,packet_reference_words,packet_gpu_words;
__attribute__((noinline)) void measured_begin(unsigned mode){__asm__ volatile(""::"r"(mode):"memory");}
__attribute__((noinline)) void measured_end(void){__asm__ volatile("":::"memory");}
void packet_test_main(void){
 FhPicaStreamArena arena;CHECK(fh_pica_stream_arena_init(&arena,arena_words,8192));
 for(unsigned i=0;i<16;i++){
  FhPicaDrawDescription *s=descriptions+i;
  AttrInfo_Init(&s->attributes);AttrInfo_AddLoader(&s->attributes,0,GPU_SHORT,3);
  AttrInfo_AddLoader(&s->attributes,1,GPU_UNSIGNED_BYTE,4);AttrInfo_AddLoader(&s->attributes,2,GPU_SHORT,2);
  BufInfo_Init(&s->buffers);s->buffers.bufCount=1;
  s->buffers.buffers[0].offset=0x1000u+i*0x1000u;
  s->buffers.buffers[0].flags[0]=0x210;s->buffers.buffers[0].flags[1]=(20<<16)|(3u<<28);
  s->effect=(FhPicaStreamEffect){.zBuffer=1,.zScale=0xbf0000,.cullMode=i%2,.depthTest=0x1f01,.alphaBlend=0x01010000,.fragOpMode=0x100};
  for(unsigned j=0;j<4;j++){C3D_TexEnvInit(s->stages+j);s->stages[j].srcRgb=(i&2)?0x300:0x500;}
  s->texture_units=i%4;s->index_address=0x18020000u+1024*i;s->index_count=3+3*i;
  CHECK(fh_pica_stream_world_compile(&arena,resident+i,s));draw_refs[i]=resident+i;
  memcpy(alternate_effects[i],resident[i].effect,18*4);alternate_effects[i][8]^=1;
 }
 for(unsigned frame=0;frame<8;frame++)for(unsigned unit=0;unit<2;unit++){
  FhPicaStreamSampler s={0x12345678,0x00400040,0,0,0x18200000+frame*0x1000+unit*0x800,0};
  CHECK(fh_pica_stream_sampler(&packet_samplers[frame][unit],unit,&s));
 }
 for(unsigned i=0;i<16;i++){
  lives[i].texture_config=resident[i].texture_units;
  for(unsigned unit=0;unit<2;unit++)lives[i].samplers[unit]=packet_samplers[i%8]+unit;
  for(unsigned j=0;j<12;j++)lives[i].material_uniforms[j]=rnd();
 }
 CHECK(fh_pica_world_packet_prepare(&packet,packet_words,3072,draw_refs,lives,16));
 CHECK(!(packet.words&3)&&packet.words<=3072);packet_gpu_words=packet.words;
 for(unsigned frame=0;frame<64;frame++){
  for(unsigned i=0;i<16;i++)if(frame){
   FhPicaDrawLive *v=lives+i;
   switch(frame%8){
   case 0:break; /* Frozen material image: no writes allowed. */
   case 1:v->colors[frame%4]=rnd();break;
   case 2:v->material_uniforms[frame%12]=rnd();break;
   case 3:v->samplers[frame%2]=packet_samplers[(frame+i)%8]+frame%2;break;
   case 4:v->effect=v->effect?NULL:alternate_effects[i];break;
   case 5:v->texture_config^=1u<<8;v->texture_shadow=rnd();break;
   case 6:v->environment_buffer=rnd();v->environment_color=rnd();v->fog_color=rnd();break;
   case 7:for(unsigned j=0;j<12;j++)v->material_uniforms[j]=rnd();
          for(unsigned j=0;j<4;j++)v->colors[j]=rnd();
          break;
   }
  }
  memcpy(packet_before,packet_words,sizeof packet_words);FhPicaWorldPacketDirty dirty;
  measured_begin(1);
  int ok=fh_pica_world_packet_update(&packet,lives,&dirty);
  measured_end();CHECK(ok);packet_updates++;packet_writes+=dirty.writes;
  if(!dirty.writes){packet_zero_updates++;CHECK(!memcmp(packet_words,packet_before,sizeof packet_words));}
  else{
   CHECK(dirty.first<dirty.end&&dirty.end<=packet.words);
   CHECK(!memcmp(packet_words,packet_before,dirty.first*4));
   CHECK(!memcmp(packet_words+dirty.end,packet_before+dirty.end,(3072-dirty.end)*4));
  }
  CHECK(fh_pica_world_packet_update(&packet,lives,&dirty)&&!dirty.writes&&dirty.first==0&&dirty.end==0);
  FhPicaStreamState state={0};unsigned at=0;
  measured_begin(0);
  for(unsigned i=0;i<16;i++)if(!fh_pica_stream_resident_append(reference_words,3072,&at,&state,resident+i,lives+i,0))test_fail(__LINE__);
  measured_end();packet_reference_words+=at;
  memset(&reference_device,0,sizeof reference_device);memset(&native_device,0,sizeof native_device);
  /* Shared state outside the world owner must survive both paths. */
  reference_device.regs[GPUREG_STENCIL_TEST]=native_device.regs[GPUREG_STENCIL_TEST]=0x12345678;
  reference_device.uniforms[0]=native_device.uniforms[0]=0x23456789;
  reference_device.regs[GPUREG_TEXENV4_COLOR]=native_device.regs[GPUREG_TEXENV4_COLOR]=0x87654321;
  snapshot_count=0;execute(&reference_device,reference_words,at);CHECK(snapshot_count==16);
  memcpy(expected_snapshots,snapshots,sizeof snapshots);
  snapshot_count=0;execute(&native_device,packet_words,packet.words-2);CHECK(snapshot_count==16);
  CHECK(!memcmp(expected_snapshots,snapshots,sizeof snapshots));packet_draws+=16;
  memcpy(packet_before,packet_words,sizeof packet_words);saved_packet=packet;
  lives[0].colors[0]^=0x13579bdf;lives[15].texture_config^=4;dirty=(FhPicaWorldPacketDirty){7,8,9};
  CHECK(!fh_pica_world_packet_update(&packet,lives,&dirty));
  lives[0].colors[0]^=0x13579bdf;lives[15].texture_config^=4;
  CHECK(!memcmp(packet_before,packet_words,sizeof packet_words)&&!memcmp(&saved_packet,&packet,sizeof packet));
  CHECK(dirty.writes==7&&dirty.first==8&&dirty.end==9);
 }
 test_pass();
}
