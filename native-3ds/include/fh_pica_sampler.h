#ifndef FH_PICA_SAMPLER_H
#define FH_PICA_SAMPLER_H
#include <citro3d.h>
/* GX minification enums 0..5 alternate nearest/linear within a mip level.
 * These resident images currently have one level; mip selection is not
 * restored by this conversion. Citro3D takes MAG first, then MIN. */
static inline void fh_pica_sampler_filter(C3D_Tex *sampler,unsigned min_filter,unsigned mag_filter){
    C3D_TexSetFilter(sampler,mag_filter==1?GPU_LINEAR:GPU_NEAREST,
        (min_filter==1 || min_filter==3 || min_filter==5)?GPU_LINEAR:GPU_NEAREST);
    if(sampler->maxLevel)C3D_TexSetFilterMipmap(sampler,(min_filter==4 || min_filter==5)?GPU_LINEAR:GPU_NEAREST);
}
#endif
