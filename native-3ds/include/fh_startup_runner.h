#ifndef FH_STARTUP_RUNNER_H
#define FH_STARTUP_RUNNER_H
#include "fh_startup_menu.h"

typedef int (*FhStartupProgress)(void*,const char*,uint32_t,uint32_t);
/* Tasks return 1 success, 0 failure, -1 cancellation, only after relinquishing
 * every file/worker/GPU resource they own. Readiness comes from the committed
 * full recipe, never a task's return code or a partially prepared artifact. */
typedef int (*FhStartupTask)(void*,FhStartupProgress,void*,char*,size_t);
typedef struct {const char*name;FhStartupTask run;void*context;} FhStartupDiagnostic;
typedef struct {
    void *context;
    unsigned (*ready)(void*);
    FhStartupTask prepare;
    const FhStartupDiagnostic *diagnostics;
    unsigned diagnostic_count;
} FhStartupTasks;
typedef struct {
    void *context;
    uint64_t (*milliseconds)(void*);
    int (*poll)(void*,unsigned*buttons);
    void (*present)(void*,const char*text);
    void (*wait)(void*);
} FhStartupDisplay;
/* Runs before game allocation. No worker, game clock, save or GPU ownership.
 * Returns1 for play,2 for benchmarks (both verified),0 to exit,-1 invalid interface. */
int fh_startup_run(const char*build,unsigned hardware_new,const FhStartupTasks*,const FhStartupDisplay*);
/* SDK LCD lifetime wrapper, called before the game initializes gfx/Citro3D.
 * Graphics settings must be loaded by the application owner beforehand. */
/* SDK installs console stdio only once; rebind after a menu handoff. */
void fh_startup_console_bind(void);
int fh_platform_startup(const char*build,unsigned hardware_new,const FhStartupTasks*);
#endif
