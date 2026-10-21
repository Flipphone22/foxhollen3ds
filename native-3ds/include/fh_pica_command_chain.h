#ifndef FH_PICA_COMMAND_CHAIN_H
#define FH_PICA_COMMAND_CHAIN_H
#include <stdint.h>
/* Experimental resident command execution, not enabled in the game.
 * A leaf uses channel 1 to return; nested calls and terminal commands are not
 * permitted. Caller owns aligned GPU-visible allocations and their fences.
 * Seal the complete SDK segment BEFORE cache publication/submission. Never
 * modify a submitted segment or retire a leaf before that segment completes.
 */
typedef struct { uint32_t word; } FhPicaChainReturn;
/* Validates a native draw packet at load time, copies once and appends return.
 * Output is exactly 32 words, suitable for repeated calls without mutation. */
int fh_pica_chain_draw(uint32_t destination[32], const uint32_t source[30]);
/* Appends an aligned call, recording its return relocation. The target must
 * be a prepared immutable 128-byte draw leaf. No SDK state or cache operations.
 * Returns words appended (12 or 14); rejection leaves all outputs unchanged. */
unsigned fh_pica_chain_call(uint32_t *, unsigned capacity, unsigned *cursor,
                           uint32_t target_physical, FhPicaChainReturn *);
/* Same owned call mechanism for a prepared ordered group. Size includes the
 * return command and alignment padding, is 16-byte aligned and at most 64KiB.
 * Caller establishes that the group has no nested jump/finalize command. */
unsigned fh_pica_chain_group_call(uint32_t *,unsigned capacity,unsigned *cursor,
    uint32_t target_physical,unsigned target_bytes,FhPicaChainReturn *);
/* Final segment length must include the SDK finalization command. Returns are
 * relative to this segment, not the entire SDK command arena. Relocations must
 * be ordered and belong to this segment. Validates ALL before changing any.
 * Flush leaves once at preparation; flush the segment only AFTER sealing.
 */
int fh_pica_chain_seal(uint32_t *, unsigned words, uint32_t physical,
                       const FhPicaChainReturn *, unsigned count);
#endif
