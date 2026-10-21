#ifndef FH_TEXTURE_PACK_PADDED_H
#define FH_TEXTURE_PACK_PADDED_H
#include "fh_texture_pack_layout.h"
/* FHTXPK4 explicitly adds edge-clamped compressed CMPR whose source is not
 * a supported power-of-two extent. Earlier pack versions stay strict. */
static inline int fh_pack_entry_valid_padded(unsigned w,unsigned h,unsigned source_format,unsigned source_bytes,
    unsigned format,unsigned gw,unsigned gh,unsigned bytes){
    if(fh_pack_entry_valid(w,h,source_format,source_bytes,format,gw,gh,bytes))return 1;
    return source_format==14 && fh_pack_source_bytes(source_format,w,h)==source_bytes && source_bytes &&
        (format==12||format==13) && (!fh_pack_dimensions(w)||!fh_pack_dimensions(h)) &&
        gw==fh_pack_extent(w) && gh==fh_pack_extent(h) && bytes==fh_pack_native_bytes(format,gw,gh);
}
#endif
