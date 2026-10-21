#ifndef FH_INSTALL_ACTOR_FOG_WIDTH_H
#define FH_INSTALL_ACTOR_FOG_WIDTH_H
#include "fh_install_disc.h"
/* Startup only. Explicit source TEX1 table/data FST entries belong to the
 * caller's selected model-bank mapping. Never infer a bank from reduced GPU
 * images. Reinitializing clears the small cache before changing bank identity. */
typedef struct {
    FhInstallDisc*disc;
    unsigned table,file,entries,cursor,requests,hits,workspace_peak;
    struct {unsigned index_plus_one,width;} cached[64];
} FhInstallActorFogWidths;
int fh_install_actor_fog_width_begin(FhInstallActorFogWidths*,FhInstallDisc*,unsigned table,unsigned data);
/* Compatible with FhActorFogTextureWidth. Original model loading forces TEX1
 * and masks to 15 bits; out-of-table indices select zero like textureLoadAsset.
 * Reads only the original base-level header, including mip-table indirection.
 * Does not validate/decode pixels or a compressed stream's trailing checksum;
 * full source fingerprint and texture conversion remain separate requirements.
 * Zlib workspace is capped at 64 KiB, including its aligned allocation headers
 * (excluding libc allocator bookkeeping). Invalid input leaves *width
 * unchanged. No geometry or GPU allocation. */
int fh_install_actor_fog_width(void*context,uint32_t texture_id,unsigned*width);
#endif
