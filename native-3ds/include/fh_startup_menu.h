#ifndef FH_STARTUP_MENU_H
#define FH_STARTUP_MENU_H
#include <stdint.h>
#include <stddef.h>
#include "fh_graphics_presets.h"
#include "fh_install_eta.h"
enum {FH_STARTUP_UP=1,FH_STARTUP_DOWN=2,FH_STARTUP_ACCEPT=4,FH_STARTUP_BACK=8};
enum {FH_STARTUP_HOME,FH_STARTUP_PRESETS,FH_STARTUP_DIAGNOSTICS};
enum {FH_STARTUP_NONE,FH_STARTUP_PLAY,FH_STARTUP_PREPARE,FH_STARTUP_RUN_PROBE,FH_STARTUP_SAVE_SETTINGS,FH_STARTUP_CANCEL,FH_STARTUP_EXIT,FH_STARTUP_BENCHMARK};
typedef struct {
    unsigned page,row,hardware_new,assets_ready,busy,probe_count,probe;
    uint32_t done,total;
    FhInstallEta estimate;
    char build[13],phase[40],message[128];
} FhStartupMenu;
/* Portable UI state. The startup owner supplies real task progress and owns
 * all file/converter/probe resources. No hidden I/O or task-completion guess. */
void fh_startup_menu_init(FhStartupMenu*,const char *build,unsigned hardware_new,unsigned ready,unsigned probes);
unsigned fh_startup_menu_input(FhStartupMenu*,unsigned buttons);
int fh_startup_menu_progress(FhStartupMenu*,const char *phase,uint32_t done,uint32_t total);
void fh_startup_menu_finish(FhStartupMenu*,unsigned ready,const char *message);
int fh_startup_menu_text(const FhStartupMenu*,const char *const *probes,char*,size_t);
#endif
