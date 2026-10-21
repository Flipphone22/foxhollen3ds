#include "fh_install_boot.h"
#include "fh_replay.h"
#include "fh_install_task.h"
#include "fh_install_bindings.h"
#include "fh_install_disc.h"
#include "fh_graphics_options.h"
static unsigned ready(void*context){
    FhInstallTask*t=context;
    return fh_install_task_ready(t)&&fh_install_bindings_select(t->root,&t->spec,t->disc_path);
}
int fh_install_boot(const FhInstallBundleSpec*spec,const char*build,unsigned hardware_new,const FhStartupDiagnostic*diagnostics,unsigned count){
    if(!spec||!build)return 0;
    FhInstallTask task={.disc_path=FH_INSTALL_DISC_PATH,.root="sdmc:/3ds/foxhollow/prepared",.spec=*spec};
    FhStartupTasks tasks={&task,ready,fh_install_task_prepare,diagnostics,count};
    for(;;){
        fh_graphics_settings_load("sdmc:/3ds/foxhollow");
        int choice=fh_platform_startup(build,hardware_new,&tasks);
        if(choice==1)break;
        if(choice!=2)return 0;
        /* Startup LCD ownership has ended before the case picker acquires it.
         * A cancelled picker returns here before any game/save allocation. */
        extern volatile unsigned fh_startup_input_polls;
        if(fh_replay_menu(&fh_startup_input_polls))break;
    }
    /* The menu's last action already rechecked readiness and copied all paths.
     * No post-menu IO can switch the selection before game startup. */
    fh_install_bindings_lock();return 1;
}
