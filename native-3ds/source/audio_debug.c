#include <musyx/debugger.h>

/* Native builds retain counters but omit the desktop debugger UI/hooks. */
static bool8 paused,step;
volatile u32 fh_audio_control_ticks,fh_audio_macro_steps,fh_audio_voice_starts,fh_audio_voice_stops;
void musyxDebuggerSetPaused(bool8 value) { paused=value; }
bool8 musyxDebuggerIsPaused(void) { return paused; }
bool8 musyxDebuggerConsumeStepFrame(void) { bool8 value=step;step=0;return value; }
void musyxDebuggerRuntimeFrame(u32 delta) { (void)delta;++fh_audio_control_ticks; }
void musyxDebuggerRuntimeMacroStep(SYNTH_VOICE *voice,const MSTEP *instruction) { (void)voice;(void)instruction;++fh_audio_macro_steps; }
void musyxDebuggerRuntimeVoiceStart(SYNTH_VOICE *voice) { (void)voice;++fh_audio_voice_starts; }
void musyxDebuggerRuntimeVoiceStop(SYNTH_VOICE *voice) { (void)voice;++fh_audio_voice_stops; }
