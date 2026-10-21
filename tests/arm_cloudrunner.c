/* Exercise the actual game routine and the allocator's adjoining event table. */
#include FH_CLOUDRUNNER_SOURCE
#include <stdint.h>
volatile unsigned test_checks,test_failure,test_event_pointer;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){test_failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);++test_checks;}while(0)
float timeDelta;
float mathCosf(float x){return cosf(x);}
float mathSinf(float x){return sinf(x);}
static unsigned callbacks;
static void angle_callback(int angle){(void)angle;callbacks++;}
static void move_callback(float z,float x){CHECK(isfinite(z) && isfinite(x));callbacks++;}
static CloudActionInterface actions={.func10Nop=angle_callback,.func12Nop=move_callback};
static CloudActionInterface *action_pointer=&actions;
CloudActionInterface **gCloudActionInterface=&action_pointer;
static unsigned char storage[512] __attribute__((aligned(8)));
void test_main(void){
    for(unsigned target=0;target<2;target++)for(unsigned step=1;step<=6;step++)for(unsigned angle=0;angle<8;angle++){
        memset(storage,0,sizeof(storage));callbacks=0;timeDelta=step;
        SBCloudRunnerState *state=(void*)storage;
        ObjAnimEventTable *events=(void*)(storage+((sizeof(*state)+3)&~3u));
        void *expected=(void*)(((uintptr_t)events+sizeof(*events)+7)&~7u);
        events->byteCount=2;events->entries=expected;
        state->targetObj=target?(GameObject*)storage:NULL;
        state->rotZ=1000;state->steerSmoothed=2;
        ((SBCloudRunnerRideState*)state)->rotorAngle=angle*8192;
        SB_CloudRunner_UpdateCloudAction(NULL,(void*)state);
        test_event_pointer=(uintptr_t)events->entries;
#ifdef FH_REPRO_BASELINE
        CHECK(test_event_pointer==0x41400000);test_pass();
#else
        CHECK(events->entries==expected && events->byteCount==2);
        CHECK(callbacks==2 && state->steerZ==12.0f);
        float smoothed=2.0f+((target?1.0f:0.0f)-2.0f)*step*.0625f;
        CHECK(state->steerSmoothed==smoothed && state->steerX==smoothed);
        for(unsigned i=(unsigned)((uintptr_t)expected-(uintptr_t)storage);i<sizeof(storage);i++)CHECK(storage[i]==0);
#endif
    }
    test_pass();
}
