#ifndef FH_INSTALL_TEXTURE_H
#define FH_INSTALL_TEXTURE_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t bytes,format,width,height,visible_components;
    uint64_t squared_error;
} FhInstallTextureResult;
typedef int (*FhInstallTextureProgress)(void*,uint32_t tiles,uint32_t total);
/* Startup owner calls once, before encoding; no threads or heap allocations. */
void fh_install_texture_init(void);
/* CMPR source pixels, excluding the original 96-byte texture header. Caller
 * owns source and output. Power-of-two extents 8..1024. Reduced dimensions
 * cannot exceed source dimensions; only fully opaque images may be reduced.
 * Returns 1 encoded, 0 invalid, -1 cancelled. Partial output is not publishable.
 * RGB error is measured against the full/BOX-filtered target; alpha is exact.
 * The recipe must reject error >196*visible_components (RMSE14), as before. */
int fh_install_texture_cmpr(const void*,size_t,unsigned w,unsigned h,unsigned gw,unsigned gh,
                           void*,size_t,FhInstallTextureResult*,FhInstallTextureProgress,void*);
/* Optional immutable two-bit search choices in native tiled block order.
 * NULL/0 keeps full search. Caller binds source identity before use and checks
 * the final output hash before publication. Borrowed modes remain live until
 * return; both worker halves are joined on success/error/cancellation. */
int fh_install_texture_cmpr_modes(const void*,size_t,unsigned w,unsigned h,unsigned gw,unsigned gh,
                           void*,size_t,FhInstallTextureResult*,FhInstallTextureProgress,void*,const unsigned char*,size_t);
#ifdef __cplusplus
}
#endif
#endif
