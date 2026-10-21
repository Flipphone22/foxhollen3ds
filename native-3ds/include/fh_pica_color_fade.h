#ifndef FH_PICA_COLOR_FADE_H
#define FH_PICA_COLOR_FADE_H
/* Live object tint after its native materials, before distance fog. The game's
 * alpha controls RGB interpolation only; existing surface opacity survives. */
void fh_pica_color_fade_set(unsigned rgba);
void fh_pica_color_fade_bind(unsigned actor);
void fh_pica_color_fade_invalidate(void);
void fh_pica_color_fade_report(void);
#endif
