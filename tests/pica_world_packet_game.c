#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_world_packet_owner.h"
volatile unsigned checks,failure;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noclone,noreturn)) void test_fail(unsigned n){failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);checks++;}while(0)
typedef struct FhWorldPacketGroup FhWorldPacketGroup;
typedef struct {FhPicaResidentDraw draw;unsigned ready;FhWorldPacketGroup *group;} TestDraw;
typedef struct {unsigned count;TestDraw draw[64];} FhPicaWorldCommands;
typedef struct {FhPicaWorldCommands *world_commands;} Material;
typedef struct {Material *items;unsigned count;} FhPicaMaterialSet;
typedef struct {unsigned format;} TestGeometry;
typedef struct {TestGeometry draw[64];} FhPicaWorldPlan;
typedef struct {const FhPicaResidentDraw *draw;unsigned fog,buffer_mask;} FhPicaWorldSelection;
static unsigned normal_used,linear_used,allocations,frees,flushes,published,deny_publish;
static unsigned hidden=~0u,wrong_fog=~0u,world_stream_draws,bindings;
static uint64_t epoch=1;
static unsigned char normal_memory[65536] __attribute__((aligned(16)));
static unsigned char linear_memory[262144] __attribute__((aligned(16)));
u32 __ctru_linear_heap_size=10*1024*1024;
static void *allocate(unsigned bytes){
    bytes=(bytes+15)&~15u;CHECK(normal_used+bytes<=sizeof normal_memory);
    void *p=normal_memory+normal_used;normal_used+=bytes;allocations++;memset(p,0,bytes);return p;
}
static void deallocate(void *p,unsigned bytes){CHECK(p&&bytes);frees++;}
void *linearAlloc(size_t bytes){
    CHECK(linear_used+bytes<=sizeof linear_memory);void *p=linear_memory+linear_used;linear_used+=bytes;return p;
}
void linearFree(void *p){CHECK(p!=NULL);}
u32 linearSpaceFree(void){return 2*1024*1024;}
uint64_t fh_pica_packet_epoch(void){return epoch;}
unsigned fh_pica_stream_packet(const FhPicaWorldPacket *p){CHECK(p->count>=4);if(deny_publish)return 0;published++;return 12;}
void fh_gpu_cache_flush(const void *p,unsigned bytes){CHECK(p&&bytes);flushes++;}
void fh_platform_panic(const char *s){(void)s;test_fail(__LINE__);}
void fh_platform_log(const char *s){CHECK(s!=NULL);}
static unsigned world_stream_select(const FhPicaMaterialSet *s,unsigned i,FhPicaDrawLive *l,FhPicaWorldSelection *o,unsigned account){
    CHECK(account==0);if(i==hidden)return 2;
    o->draw=&s->items->world_commands->draw[i].draw;o->fog=i==wrong_fog;o->buffer_mask=i&3;
    l->colors[0]=i;l->material_uniforms[0]=0x3f800000;return 1;
}
#include "platform_pica_world_packets.h"
static uint32_t attributes[12],layout[40],effect[18],stages[24],draw_words[30],saved[3072];
static FhPicaWorldCommands commands;static Material material={&commands};
static FhPicaMaterialSet set={&material,1};static FhPicaWorldPlan plan;
void test_main(void){
    commands.count=32;
    for(unsigned i=0;i<32;i++){
        commands.draw[i].ready=1;
        commands.draw[i].draw=(FhPicaResidentDraw){.attributes=attributes,.layout=layout,.effect=effect,.stages=stages,.draw=draw_words,.effect_words=18};
    }
    world_packets_prepare(&plan,&set);CHECK(packet_groups==2&&allocations==2);
    CHECK(fh_pica_world_packet_count(&set,0)==16&&fh_pica_world_packet_count(&set,16)==16);
    CHECK(!fh_pica_world_packet_count(&set,1));
    FhPicaDrawLive env={0};FhWorldPacketGroup *g=commands.draw[0].group;
    CHECK(fh_pica_world_packet_submit(&set,0,&env,0)==16&&published==1);
    CHECK(world_stream_draws==15&&bindings==15); /* Caller counts first draw. */
    unsigned before_flush=flushes;
    memcpy(saved,g->packet.commands,g->packet.words*4);
    CHECK(!fh_pica_world_packet_count(&set,0));env.colors[0]=0xffffffff;
    CHECK(!fh_pica_world_packet_submit(&set,0,&env,0));
    CHECK(flushes==before_flush&&!memcmp(saved,g->packet.commands,g->packet.words*4));
    epoch++;
    hidden=15;env.environment_color=0x12345678;
    CHECK(!fh_pica_world_packet_submit(&set,0,&env,0));
    CHECK(flushes==before_flush&&!memcmp(saved,g->packet.commands,g->packet.words*4));
    hidden=~0u;wrong_fog=15;
    CHECK(!fh_pica_world_packet_submit(&set,0,&env,0));
    CHECK(flushes==before_flush&&!memcmp(saved,g->packet.commands,g->packet.words*4));
    wrong_fog=~0u;deny_publish=1;
    CHECK(!fh_pica_world_packet_submit(&set,0,&env,0));
    CHECK(g->used_epoch==1&&fh_pica_world_packet_count(&set,0)==16);
    deny_publish=0;CHECK(fh_pica_world_packet_submit(&set,0,&env,0)==16);
    CHECK(g->used_epoch==2&&published==2);
    epoch++;before_flush=flushes;
    CHECK(fh_pica_world_packet_submit(&set,0,&env,0)==16&&flushes==before_flush);
    CHECK(packet_unchanged>=2&&packet_live_reject==2&&packet_call_reject==1);
    world_packets_release(&commands);CHECK(!packet_cpu&&!packet_linear&&frees==allocations);
    world_packets_report();
    /* A compatible run of three is not compiled; format boundaries split it. */
    normal_used=linear_used=0;commands.count=7;
    for(unsigned i=0;i<7;i++)plan.draw[i].format=i<3?0:5;
    world_packets_prepare(&plan,&set);
    CHECK(!commands.draw[0].group&&commands.draw[3].group&&commands.draw[3].group->packet.count==4);
    world_packets_release(&commands);CHECK(!packet_cpu&&!packet_linear&&frees==allocations);
    /* The global resource ceiling prevents new groups without removing draws. */
    packet_linear=WORLD_PACKET_LINEAR_LIMIT;world_packets_prepare(&plan,&set);
    CHECK(!commands.draw[3].group);packet_linear=0;
    CHECK(commands.draw[3].ready);test_pass();
}
