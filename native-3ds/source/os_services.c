#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "fh_platform.h"
#include "foxhollow_menu.h"

static int interrupts_enabled=1;
static u32 sound_mode=1,progressive;
static u8 language;
static OSErrorHandler error_handlers[17];
BOOL OSDisableInterrupts(void) { int old=interrupts_enabled; interrupts_enabled=0; return old; }
BOOL OSRestoreInterrupts(BOOL enabled) {
    int old=interrupts_enabled;
    interrupts_enabled=enabled;
    if (enabled) fh_dvd_pump();
    return old;
}
BOOL OSEnableInterrupts(void) { return OSRestoreInterrupts(1); }
OSTick OSGetTick(void) { return (OSTick)fh_platform_ticks(); }
OSTime OSGetTime(void) { return (OSTime)fh_platform_ticks(); }
void OSPanic(const char *file,int line,const char *format,...) {
    char message[768];
    int count=snprintf(message,sizeof(message),"%s:%d: ",file,line);
    if (count<0 || count>=(int)sizeof(message)) count=0;
    va_list args; va_start(args,format); vsnprintf(message+count,sizeof(message)-count,format,args); va_end(args);
    fh_platform_panic(message);
}
u32 OSGetSoundMode(void) { return sound_mode; }
void OSSetSoundMode(u32 mode) { sound_mode=mode; }
u32 OSGetProgressiveMode(void) { return progressive; }
void OSSetProgressiveMode(u32 mode) { progressive=mode; }
u8 OSGetLanguage(void) { return language; }
void OSSetLanguage(u8 value) { language=value; }
u32 OSGetResetCode(void) { return 0; }
BOOL OSGetResetButtonState(void) { return 0; }
OSErrorHandler OSSetErrorHandler(OSError error,OSErrorHandler handler) {
    if ((unsigned)error>=17) return NULL;
    OSErrorHandler old=error_handlers[error]; error_handlers[error]=handler; return old;
}
void OSSetSaveRegion(void *start,void *end) { (void)start;(void)end; }
void OSResetSystem(int reset,u32 code,BOOL menu) { (void)reset;(void)code;(void)menu; fh_platform_quit(); }
void DCFlushRange(void *memory,u32 bytes) { fh_gx_cache_range(memory,bytes,1); }
void DCFlushRangeNoSync(void *memory,u32 bytes) { fh_gx_cache_range(memory,bytes,1); }
void DCStoreRange(void *memory,u32 bytes) { fh_gx_cache_range(memory,bytes,1); }
void DCStoreRangeNoSync(void *memory,u32 bytes) { fh_gx_cache_range(memory,bytes,1); }
void DCInvalidateRange(void *memory,u32 bytes) { fh_gx_cache_range(memory,bytes,0); }
void LCEnable(void) {}
void LCDisable(void) {}
void LCQueueWait(u32 count) { (void)count; }
void LCLoadBlocks(void *destination,void *source,u32 count) { memcpy(destination,source,count?count*32:4096); }
void LCStoreBlocks(void *destination,void *source,u32 count) { memcpy(destination,source,count?count*32:4096); }
void fhRequestQuit(void) { fh_platform_quit(); }
const char *fhMenuTextForId(int id) { return id==FH_MENU_TEXT_ID_EXIT_TO_DESKTOP?"Exit to Homebrew":NULL; }
