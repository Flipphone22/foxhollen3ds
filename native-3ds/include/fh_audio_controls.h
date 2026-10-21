#ifndef FH_AUDIO_CONTROLS_H
#define FH_AUDIO_CONTROLS_H
#include <stdint.h>
/* Main producer; opt-in only after the existing Old core1 owner is created. */
extern unsigned fh_audio_controls_enabled;
extern unsigned fh_audio_controls_queued,fh_audio_controls_applied,fh_audio_controls_backpressure;
int fh_audio_controls_can_queue(void);
void fh_audio_spatial_controls(uint32_t voice,unsigned pan,unsigned surround,unsigned volume,unsigned spatial);
/* Existing recursive synth lock must be held. Also used by API barriers. */
void fh_audio_controls_drain(void);
#endif
