/* Startup-only LCD owner. Adapted from pinned Melee gfx/console ordering and
 * our verified replay menu's separate gfx lifetime. No Citro3D owner exists
 * here; a diagnostic must retire its GPU resources before its callback returns.
 */
#include <3ds.h>
#include <sys/iosupport.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "fh_startup_runner.h"
#include "fh_cpu.h"
#include "fh_install_profile.h"
#include "fh_install_worker.h"

typedef struct {PrintConsole console;unsigned failed;} Screen;
static const devoptab_t *console_out,*console_err;
void fh_startup_console_bind(void){
    /* Pinned SDK consoleInit installs dotab_stdout on its first call only.
     * Preserve that immutable SDK table, never a stack-owned PrintConsole. */
    if(!console_out){console_out=devoptab_list[STD_OUT];console_err=devoptab_list[STD_ERR];}
    else {devoptab_list[STD_OUT]=console_out;devoptab_list[STD_ERR]=console_err;}
}
/* Read-only observation clock for scripted menu checks. This counts actual
 * menu polls, never game frames/simulation and never drives either clock. */
volatile unsigned fh_startup_input_polls;
static uint64_t milliseconds(void*unused){(void)unused;return svcGetSystemTick()*1000ULL/SYSCLOCK_ARM11;}
typedef struct {
    const FhStartupTasks*original;
    const char*build;
    unsigned hardware_new;
    FhStartupProgress progress;
    void*progress_context;
    FhInstallProfile profile;
} MeasuredInstall;
static unsigned measured_ready(void*context){
    MeasuredInstall*m=context;return m->original->ready(m->original->context);
}
static int measured_progress(void*context,const char*label,uint32_t done,uint32_t total){
    MeasuredInstall*m=context;
    fh_install_profile_progress(&m->profile,label,milliseconds(NULL));
    return !m->progress||m->progress(m->progress_context,label,done,total);
}
static int measured_prepare(void*context,FhStartupProgress progress,void*opaque,char*error,size_t capacity){
    MeasuredInstall*m=context;m->progress=progress;m->progress_context=opaque;
    fh_install_profile_begin(&m->profile,milliseconds(NULL));
    fh_install_worker_start(m->hardware_new);
    int result=m->original->prepare(m->original->context,measured_progress,m,error,capacity);
    fh_install_worker_stop();
    fh_install_profile_finish(&m->profile,milliseconds(NULL));
    /* One small append after completion/cancellation and converter cleanup.
     * No per-tile diagnostic writes or changes to checkpoint transactions. */
    FILE*f=fopen("sdmc:/3ds/foxhollow/install-timing.csv","ab");
    if(f){
        static const char*names[]={"source","original","performance","textures","movie","publication","other"};
        fprintf(f,"run,%.12s,%s,result=%d,callbacks=%lu,inclusive_wall_ms\n",m->build?m->build:"unknown",m->hardware_new?"new":"old",result,(unsigned long)m->profile.callbacks);
        FhInstallWorkerStats worker;fh_install_worker_stats(&worker);
        fprintf(f,"worker,core=%u,jobs=%u,stack_peak_upper=%u,retired=1\n",worker.core,worker.jobs,worker.stack_peak);
        for(unsigned i=0;i<FH_INSTALL_PROFILE_PHASES;i++)
            fprintf(f,"phase,%s,%u,%llu\n",names[i],(m->profile.seen>>i)&1,(unsigned long long)m->profile.milliseconds[i]);
        fclose(f);
    }
    return result;
}
static int poll(void*context,unsigned*buttons){
    Screen*s=context;*buttons=0;if(s->failed||!aptMainLoop())return 0;
    fh_platform_cpu_poll();
    fh_startup_input_polls++;
    hidScanInput();u32 keys=hidKeysDown();
    if(keys&KEY_UP)*buttons|=FH_STARTUP_UP;
    if(keys&KEY_DOWN)*buttons|=FH_STARTUP_DOWN;
    if(keys&KEY_A)*buttons|=FH_STARTUP_ACCEPT;
    if(keys&(KEY_B|KEY_SELECT))*buttons|=FH_STARTUP_BACK;
    return 1;
}
static void present(void*context,const char*text){
    Screen*s=context;consoleSelect(&s->console);consoleClear();fputs(text,stdout);fflush(stdout);
    /* consoleInit uses a single bottom buffer. Publish only changed text. */
    u16 w,h;void*p=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&w,&h);
    if(R_FAILED(GSPGPU_FlushDataCache(p,(size_t)w*h*2)))s->failed=1;
}
static void wait_frame(void*unused){(void)unused;gspWaitForVBlank();}
int fh_platform_startup(const char*build,unsigned hardware_new,const FhStartupTasks*tasks){
    if(!tasks||!tasks->ready)return -1;
    fh_startup_input_polls=0;
    /* Same pinned Melee-derived CPU/L2 policy as gameplay. Its APT hook is
     * owned only until this menu returns; gameplay creates its own afterward. */
    fh_platform_cpu_init(hardware_new);
    const devoptab_t*saved_out=devoptab_list[STD_OUT],*saved_err=devoptab_list[STD_ERR];
    Screen screen={0};PrintConsole*previous=consoleSelect(consoleGetDefault());
    gfxInit(GSP_RGBA8_OES,GSP_RGB565_OES,false);
    /* Initialize both LCD buffers before enabling a menu; this avoids the
     * uninitialized blue-bar startup failure seen in earlier console builds. */
    int result=-1;
    for(unsigned i=0;i<2;i++){
        for(unsigned screen=GFX_TOP;screen<=GFX_BOTTOM;screen++){
            u16 w,h;void*p=gfxGetFramebuffer((gfxScreen_t)screen,GFX_LEFT,&w,&h);
            size_t bytes=(size_t)w*h*(screen==GFX_TOP?4:2);memset(p,0,bytes);
            if(R_FAILED(GSPGPU_FlushDataCache(p,bytes)))goto done;
            gfxScreenSwapBuffers((gfxScreen_t)screen,false);
        }gspWaitForVBlank();
    }
    consoleInit(GFX_BOTTOM,&screen.console);fh_startup_console_bind();
    {FhStartupDisplay display={&screen,milliseconds,poll,present,wait_frame};
     MeasuredInstall measured={.original=tasks,.build=build,.hardware_new=hardware_new};
     FhStartupTasks instrumented=*tasks;
     instrumented.context=&measured;instrumented.ready=measured_ready;
     if(tasks->prepare)instrumented.prepare=measured_prepare;
     result=fh_startup_run(build,hardware_new,&instrumented,&display);}
    if(screen.failed)result=-1;
done:
    fflush(stdout);fflush(stderr);devoptab_list[STD_OUT]=saved_out;devoptab_list[STD_ERR]=saved_err;
    consoleSelect(previous);gfxExit();fh_platform_cpu_exit();
    /* One startup-only receipt after LCD retirement; no per-poll SD traffic. */
    FILE*receipt=fopen("sdmc:/3ds/foxhollow/startup-result.txt","wb");
    if(receipt){fprintf(receipt,"build=%.12s\nresult=%d\npolls=%u\nlcd_retired=1\n",build?build:"unknown",result,fh_startup_input_polls);fclose(receipt);}
    return result;
}
