#ifndef FH_AUDIO_BALANCED_MIX_H
#define FH_AUDIO_BALANCED_MIX_H
/* Preserve signed dry coefficients but send them through AUX B's identical
 * transport latency. AUX A remains the wet bus. B must be unoccupied; never
 * combine an unrelated effect or silently discard it. Input/output may alias. */
int fh_audio_balanced_mix(float output[12],const float input[12]);
#endif
