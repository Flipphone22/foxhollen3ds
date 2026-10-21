#include "fh_replay_timing.h"
#include <dolphin/vi.h>
#include "fh_platform.h"
#include "fh_graphics.h"
#include "fh_gx_dispatch.h"
static VIRetraceCallback before,after;
static u32 retraces;
static void *next_framebuffer;
static GXRenderModeObj mode;
void VIInit(void) { retraces=0; }
void VIConfigure(const GXRenderModeObj *configuration) { if(configuration) mode=*configuration; }
void VIWaitForRetrace(void) {
    fh_gx_dispatch_join_deferred();
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    unsigned replay_phase=fh_replay_phase(FH_PHASE_DVD_CALLBACKS);
#endif
    fh_dvd_pump();
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(FH_PHASE_AUDIO_PUMP);
#endif
    fh_audio_pump();
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(replay_phase);
#endif
    fh_platform_frame();
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(FH_PHASE_AUDIO_PUMP);
#endif
    fh_audio_pump();
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(replay_phase);
#endif
    {
        FH_REPLAY_SCOPE(RETRACE_CALLBACKS);
        fh_gx_frame_complete();
        ++retraces;
        if (before) before(retraces);
        if (after) after(retraces);
    }
}
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback callback) { VIRetraceCallback old=before;before=callback;return old; }
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback callback) { VIRetraceCallback old=after;after=callback;return old; }
void VISetNextFrameBuffer(void *framebuffer) { next_framebuffer=framebuffer; }
void *VIGetNextFrameBuffer(void) { return next_framebuffer; }
void *VIGetCurrentFrameBuffer(void) { return next_framebuffer; }
void VISetBlack(BOOL black) { fh_gx_dispatch_join_deferred();fh_renderer_black(black); }
void VIFlush(void) {}
u32 VIGetRetraceCount(void) { return retraces; }
u32 VIGetNextField(void) { return retraces&1; }
u32 VIGetDTVStatus(void) { return 0; }
u32 VIGetTvFormat(void) { return mode.viTVmode>>2; }
