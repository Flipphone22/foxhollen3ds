#include "main/dll/baddie_state.h"
#include "main/dll/dll_0015_curves.h"
#include <stdint.h>

volatile unsigned test_checks;
volatile unsigned test_failure_line;
volatile uintptr_t observed_radius;
__attribute__((noinline)) void test_fail(unsigned line) { test_failure_line=line;for(;;){} }
__attribute__((noinline)) void test_pass(void) { for(;;){} }
#define CHECK(x) do {++test_checks;if(!(x))test_fail(__LINE__);}while(0)
static BaddieState actor;
static float points[3]={2,5,7};

void test_main(void) {
    curves_clear(&actor.curvesCollision,1,0x400a7,1);
    /* This was the live radius symbol before the high byte was overwritten.
     * The fixture never dereferences this diagnostic pointer. */
    curves_setLocalPointCollision(&actor.curvesCollision,1,points,(float*)0x00406e98,1);
    actor.physicsActive=1;
    observed_radius=(uintptr_t)actor.curvesCollision.localPointRadii;
    CHECK(observed_radius==0x00406e98);
    CHECK(actor.curvesCollision.localPointPositions==points);
    for(unsigned value=0;value<256;value++) {
        actor.physicsActive=value;
        CHECK((unsigned char)actor.curvesCollision.subtype==value);
        actor.contactSfxFlags=value;
        CHECK(actor.curvesCollision.pointCounts==value);
        actor.bboxTraceFlags=value;
        CHECK((unsigned char)actor.curvesCollision.primaryHitType==value);
        actor.groundContact=value;
        CHECK(actor.curvesCollision.localPointHitMask==value);
        actor.surfaceFlags=value;
        CHECK(actor.curvesCollision.surfaceFlags==value);
        CHECK((uintptr_t)actor.curvesCollision.localPointRadii==0x00406e98);
        CHECK(actor.curvesCollision.localPointPositions==points);
    }
    actor.unk118=123;actor.unk11C=456;actor.unk120=789;
    CHECK(actor.curvesCollision.localPointTarget[0][0]==123);
    CHECK(actor.curvesCollision.localPointTarget[0][1]==456);
    CHECK(actor.curvesCollision.localPointTarget[0][2]==789);
    actor.spawnRotY=-32768;actor.spawnRotZ=32767;
    CHECK(actor.curvesCollision.tiltPitch==-32768);
    CHECK(actor.curvesCollision.tiltRoll==32767);
    test_pass();
}
