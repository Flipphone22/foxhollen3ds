#pragma once
/* Original addShaderLayerStages selects Swizzled before the object override.
 * Without C1/C2, its white-selector mode-0 color is texture * white, not
 * texture * raster lighting. Alpha keeps its independent source policy.
 * Projected lights remain a separately unsupported material contribution. */
static inline unsigned fh_pica_actor_unlit_texture(unsigned diffuse,
    unsigned secondary,unsigned red,unsigned green,unsigned blue)
{
    return !diffuse && !secondary && red==255 && green==255 && blue==255;
}
