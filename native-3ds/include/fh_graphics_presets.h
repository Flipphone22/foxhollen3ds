#ifndef FH_GRAPHICS_PRESETS_H
#define FH_GRAPHICS_PRESETS_H
#include "fh_graphics_options.h"
enum {FH_PRESET_NEW_FANCY,FH_PRESET_NEW_FAST,FH_PRESET_OLD_FANCY,FH_PRESET_OLD_FAST,FH_PRESET_CUSTOM,FH_PRESET_COUNT};
typedef struct {const char *name;unsigned mask,original_geometry,original_textures,effects;} FhGraphicsPreset;
const FhGraphicsPreset *fh_graphics_preset(unsigned);
/* Custom deliberately leaves live settings unchanged. Display counters are
 * independent preferences, not part of a preset or game timing recipe. */
int fh_graphics_preset_apply(unsigned);
unsigned fh_graphics_preset_match(unsigned mask,unsigned geometry,unsigned textures);
#endif
