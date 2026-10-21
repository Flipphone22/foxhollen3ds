/* Actual effect routine, real object byte member, neighboring bytes poisoned. */
#include FH_EFFECT_SOURCE
#include <stdint.h>
volatile unsigned test_checks, test_failure;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){test_failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);++test_checks;}while(0)
static unsigned expected_alpha, calls;
static int capture(struct ExpgfxSpawnConfig *config,int pool,int id,int flags){
    PartFxSpawn *p=(void*)config;
    CHECK(pool==-1 && id==0x714 && flags==0);
    CHECK(p->initialAlpha==expected_alpha);
    CHECK(p->textureId==0xc0f && p->renderFlags==0x4002800);
    CHECK(p->behaviorFlags==0x80206 && p->lifetimeFrames==8);
    ++calls;return 123;
}
static ExpgfxInterface api={.spawnEffect=capture};
static ExpgfxInterface *api_ptr=&api;
ExpgfxInterface **gExpgfxInterface=&api_ptr;
int randomGetRange(int lo,int hi){(void)hi;return lo;}
static GameObject object;
static unsigned char arg_bytes[8] __attribute__((aligned(8)));
void test_main(void){
    _Static_assert(sizeof(object.anim.alpha)==1,"Producer owns one alpha byte");
    memset(&object,0,sizeof(object));
    for(unsigned a=0;a<256;a++)for(unsigned neighbor=0;neighbor<2;neighbor++){
        memset(arg_bytes,neighbor?0xa5:0,sizeof(arg_bytes));
        arg_bytes[2]=a; /* Exact physical address residue (object+0x36). */
        expected_alpha=(unsigned)(30.0f*((float)a/255.0f));
        CHECK(Effect18_spawnObject(&object,0x714,0,2,255,arg_bytes+2)==123);
        object.anim.alpha=a;
        CHECK(Effect18_spawnObject(&object,0x714,0,2,255,&object.anim.alpha)==123);
    }
    expected_alpha=30;
    CHECK(Effect18_spawnObject(&object,0x714,0,2,255,0)==123);
    CHECK(calls==1025);test_pass();
}
