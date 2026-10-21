#include <dolphin/os.h>
#include <dolphin/pad.h>
#include <stdint.h>
#include <string.h>
#include "fh_platform.h"
#include "fh_stack_stats.h"
static OSThread producer,consumer,cancelled;
static unsigned char stacks[3][8192] __attribute__((aligned(8)));
static OSMessageQueue messages,finished,waiting;
static OSMessage message_slots[3],finish_slots[1],wait_slots[1];
static FhPadState input;
volatile unsigned test_checks,test_failure;
__attribute__((noinline,noreturn)) void test_pass(void) { for(;;) __asm__ volatile("nop"); }
__attribute__((noinline,noreturn)) void test_fail(unsigned line) { test_failure=line;for(;;) __asm__ volatile("nop"); }
#define CHECK(v) do { if(!(v)) test_fail(__LINE__);++test_checks; } while(0)
void fh_platform_panic(const char *message) { (void)message;test_fail(9999); }
void fh_dvd_pump(void) {}
void fh_platform_pad(FhPadState *state) { *state=input; }
static void *produce(void *arg) {
    CHECK(arg==(void*)0x1234);
    unsigned lo=0x12345678,hi=0xabcdef98;
    __asm__ volatile("vmov d8,%0,%1"::"r"(lo),"r"(hi):"d8");
    for(unsigned i=1;i<=1000;i++) CHECK(OSSendMessage(&messages,(void*)(uintptr_t)i,OS_MESSAGE_BLOCK));
    __asm__ volatile("vmov %0,%1,d8":"=r"(lo),"=r"(hi));
    CHECK(lo==0x12345678 && hi==0xabcdef98);
    return (void*)0x5678;
}
static void *consume(void *arg) {
    (void)arg;
    unsigned lo=0xfedcba98,hi=0x87654321;
    __asm__ volatile("vmov d8,%0,%1"::"r"(lo),"r"(hi):"d8");
    for(unsigned i=1;i<=1000;i++) {
        OSMessage message=NULL;
        CHECK(OSReceiveMessage(&messages,&message,OS_MESSAGE_BLOCK));
        CHECK((uintptr_t)message==i);
    }
    __asm__ volatile("vmov %0,%1,d8":"=r"(lo),"=r"(hi));
    CHECK(lo==0xfedcba98 && hi==0x87654321);
    CHECK(OSSendMessage(&finished,(void*)1,OS_MESSAGE_BLOCK));
    return NULL;
}
static void *waiter(void *arg) {
    (void)arg;OSMessage message;
    OSReceiveMessage(&waiting,&message,OS_MESSAGE_BLOCK);
    test_fail(__LINE__);
}
void test_main(void) {
    OSInitMessageQueue(&messages,message_slots,3);
    OSInitMessageQueue(&finished,finish_slots,1);
    OSInitMessageQueue(&waiting,wait_slots,1);
    CHECK(!OSReceiveMessage(&messages,NULL,OS_MESSAGE_NOBLOCK));
    CHECK(OSCreateThread(&producer,produce,(void*)0x1234,stacks[0]+8192,8192,12,1));
    CHECK(OSCreateThread(&consumer,consume,NULL,stacks[1]+8192,8192,13,1));
    FhGameStackStats stats[15];
    CHECK(fh_game_stack_stats(stats,15)==2);
    CHECK(stats[0].bytes==8192 && stats[0].observed_used==0 && stats[0].canary_ok);
    CHECK(fh_game_stack_stats(stats,1)==1 && fh_game_stack_stats(NULL,0)==0);
    CHECK(OSResumeThread(&producer)==1);
    CHECK(producer.state==OS_THREAD_STATE_WAITING);
    CHECK(fh_game_stack_stats(stats,15)==2 && stats[0].observed_used>0 && stats[0].observed_used<4096 && stats[0].canary_ok);
    CHECK(OSResumeThread(&consumer)==1);
    OSMessage message;
    CHECK(OSReceiveMessage(&finished,&message,OS_MESSAGE_BLOCK) && message==(void*)1);
    CHECK(producer.state==OS_THREAD_STATE_MORIBUND && producer.val==(void*)0x5678);
    CHECK(consumer.state==OS_THREAD_STATE_MORIBUND);
    CHECK(fh_game_stack_stats(stats,15)==0);
    CHECK(OSCreateThread(&cancelled,waiter,NULL,stacks[2]+8192,8192,8,1));
    OSResumeThread(&cancelled);
    CHECK(waiting.queueReceive.head==&cancelled);
    CHECK(fh_game_stack_stats(stats,15)==1 && stats[0].canary_ok && stats[0].observed_used>0);
    stacks[2][100]=0;CHECK(fh_game_stack_stats(stats,15)==1 && stats[0].observed_used==8192-100);
    stacks[2][100]=0xa5;CHECK(fh_game_stack_stats(stats,15)==1 && stats[0].observed_used==8192-100);
    OSCancelThread(&cancelled);
    memset(stacks[2],0,sizeof(stacks[2]));CHECK(fh_game_stack_stats(stats,15)==0);
    CHECK(waiting.queueReceive.head==NULL && waiting.queueReceive.tail==NULL);
    CHECK(OSSendMessage(&waiting,(void*)2,OS_MESSAGE_NOBLOCK));
    CHECK(!OSSendMessage(&waiting,(void*)3,OS_MESSAGE_NOBLOCK));
    CHECK(OSReceiveMessage(&waiting,&message,OS_MESSAGE_NOBLOCK) && message==(void*)2);
    PADStatus pads[4];
    input.x=127;input.y=0;input.cx=-127;input.cy=0;input.left=255;input.right=30;input.buttons=PAD_BUTTON_A;
    PADRead(pads);PADClamp(pads);
    CHECK(pads[0].stickX==72 && pads[0].stickY==0 && pads[0].substickX==-59);
    CHECK(pads[0].triggerLeft==150 && pads[0].triggerRight==0 && pads[0].button==PAD_BUTTON_A);
    CHECK(pads[1].err==PAD_ERR_NO_CONTROLLER && pads[3].err==PAD_ERR_NO_CONTROLLER);
    input.x=-15;input.y=15;PADRead(pads);PADClamp(pads);
    CHECK(!pads[0].stickX && !pads[0].stickY);
    input.x=127;input.y=127;PADRead(pads);PADClamp(pads);
    CHECK(pads[0].stickX==40 && pads[0].stickY==40);
    test_pass();
}
