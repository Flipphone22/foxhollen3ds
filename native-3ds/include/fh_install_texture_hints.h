#ifndef FH_INSTALL_TEXTURE_HINTS_H
#define FH_INSTALL_TEXTURE_HINTS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Startup-only borrowed immutable search metadata. No texture payloads.
 * Caller retains storage through all encoder worker joins. No allocation,
 * filesystem or GPU operations. Open verifies a compiled-in blob fingerprint,
 * source-disc identity, encoder contract and all record bounds before lookup. */
typedef struct {const unsigned char*data;size_t bytes;unsigned count;} FhInstallTextureHints;
typedef struct {
 const unsigned char*modes;const unsigned char*output_sha;
 unsigned blocks,bytes,format;
} FhInstallTextureHint;
int fh_install_texture_hints_open(FhInstallTextureHints*,const void*,size_t,
 const unsigned char blob_sha[32],const unsigned char disc_sha[32],const unsigned char codec_sha[32]);
/* 1 matching source+dimensions; 0 uncovered; -1 invalid input/hash failure. */
int fh_install_texture_hints_find(const FhInstallTextureHints*,const void*,size_t,
 unsigned w,unsigned h,unsigned gw,unsigned gh,unsigned format,FhInstallTextureHint*);
int fh_install_texture_hint_verify(const FhInstallTextureHint*,const void*,size_t,unsigned format);
#ifdef __cplusplus
}
#endif
#endif
