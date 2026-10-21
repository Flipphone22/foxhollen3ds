#ifndef FH_AUDIO_COST_H
#define FH_AUDIO_COST_H
#include <stdint.h>
/* Experimental elapsed-time accounting. Each bucket has one serialized owner;
 * report only after the synth worker and SDK thread have joined. SDK_ACTIVE
 * contains STATE/CALLBACK/AUX/PUBLISH: never add those children to its total.
 * System ticks include preemption; these are NOT thread CPU cycles. */
enum { FH_AC_PRODUCER, FH_AC_SDK_ACTIVE, FH_AC_STATE, FH_AC_CALLBACK,
       FH_AC_AUX, FH_AC_PUBLISH, FH_AC_COUNT };
uint64_t fh_audio_cost_now(void);
void fh_audio_cost_finish(unsigned bucket,uint64_t start);
void fh_audio_cost_report(void);
#endif
