#ifndef FH_SCREEN_H
#define FH_SCREEN_H
/* Logical GX coordinates remain 640x528. Render all 400 LCD columns;
 * retain the 24 offscreen PAL rows needed by texture-copy users. */
static inline unsigned fh_screen_x(unsigned logical) { return logical*5/8; }
static inline unsigned fh_screen_edge(unsigned logical) { return (logical*5+7)/8; }
/* Aligned half-size EFB copies use nearest native samples, including on GPU.
 * A pixel center at output x+.5 must sample native floor(5*x/4). */
/* Keep all four 5:4 phases away from a texel boundary despite PICA float24. */
static inline float fh_screen_copy_u(float x) { return 1.25f*(x-.5f)+.125f; }
/* Half-size copies consume reversed PAL rows. At output center y+.5,
 * source center 263.5-y selects row263-y, with half a texel of headroom.
 * A tiny epsilon below an integer boundary disappears in PICA float24. */
static inline float fh_screen_copy_v(float y) { return 264.f-y; }
#endif
