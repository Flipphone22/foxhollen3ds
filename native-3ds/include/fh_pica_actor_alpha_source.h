#pragma once
/* addShaderLayerStages chooses Swizzled before examining vertex attributes
 * when a diffuse map or white channel selector is present. Remaining
 * no-vertex-colour branches use Swizzled/KAlpha; both use constant alpha.
 * This choice is immutable per loaded material, not a per-draw GX interpreter. */
static inline unsigned fh_pica_actor_constant_alpha(unsigned diffuse,
    unsigned red,unsigned green,unsigned blue,unsigned vertex_flags)
{
    return diffuse || (red==255 && green==255 && blue==255) || !(vertex_flags&0x10);
}
