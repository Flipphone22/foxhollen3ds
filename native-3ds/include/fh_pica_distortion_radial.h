#ifndef FH_PICA_DISTORTION_RADIAL_H
#define FH_PICA_DISTORTION_RADIAL_H
#include <stddef.h>
#include <stdint.h>
enum { FH_DISTORTION_RADIAL_BYTES=128*128 };
/* Load-time conversion of the game's 128x128 I8 radial image to PICA A8.
 * Separate buffers required. No allocation, GPU calls or publication.
 * Reverse storage rows so shader radial UVs retain source orientation. */
int fh_pica_distortion_radial(uint8_t *output,size_t output_bytes,
    const uint8_t *source,size_t source_bytes);
#endif
