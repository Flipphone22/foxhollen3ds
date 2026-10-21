#include <3ds.h>
#include <stdio.h>
#include "fh_platform.h"
#include "fh_audio_cost.h"
typedef struct { uint64_t ticks,maximum,calls; } Cost;
static Cost costs[FH_AC_COUNT];
uint64_t fh_audio_cost_now(void) { return svcGetSystemTick(); }
void fh_audio_cost_finish(unsigned bucket,uint64_t start) {
    uint64_t elapsed=svcGetSystemTick()-start;
    if(bucket>=FH_AC_COUNT)fh_platform_panic("Invalid audio cost bucket");
    Cost *c=&costs[bucket];c->ticks+=elapsed;c->calls++;
    if(elapsed>c->maximum)c->maximum=elapsed;
}
void fh_audio_cost_report(void) {
    static const char *names[FH_AC_COUNT]={"producer","sdk_active","state","callback","aux","publish"};
    for(unsigned i=0;i<FH_AC_COUNT;i++) {
        char line[256];const Cost *c=&costs[i];
        snprintf(line,sizeof(line),"audio cost bucket=%s calls=%llu ticks=%llu maximum=%llu hz=%lu elapsed_includes_preemption=1\n",
            names[i],(unsigned long long)c->calls,(unsigned long long)c->ticks,
            (unsigned long long)c->maximum,(unsigned long)SYSCLOCK_ARM11);
        fh_platform_log(line);
    }
}
