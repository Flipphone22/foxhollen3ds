#ifndef FH_INSTALL_BUNDLE_H
#define FH_INSTALL_BUNDLE_H
#include "fh_install_transaction.h"
enum {FH_INSTALL_BUNDLE_STAGES=4,FH_INSTALL_BUNDLE_ASSETS=8};
enum {FH_BUNDLE_ORIGINAL,FH_BUNDLE_PERFORMANCE,FH_BUNDLE_TEXTURES,FH_BUNDLE_MOVIE};
enum {FH_BUNDLE_ORIGINAL_MODEL,FH_BUNDLE_ORIGINAL_SCENE,FH_BUNDLE_ORIGINAL_CACHE,
      FH_BUNDLE_PERFORMANCE_MODEL,FH_BUNDLE_PERFORMANCE_SCENE,FH_BUNDLE_PERFORMANCE_CACHE,
      FH_BUNDLE_TEXTURE_PACK,FH_BUNDLE_PREPARED_MOVIE};
typedef struct {
    unsigned char stage_recipe[FH_INSTALL_BUNDLE_STAGES][32],source[32];
    uint32_t disc_bytes,fst_crc;
} FhInstallBundleSpec;
typedef struct {
    char root[240];FhInstallGeneration stages[FH_INSTALL_BUNDLE_STAGES];
    unsigned generation,serial;
} FhInstallBundle;
typedef int (*FhInstallBundleProgress)(void*,const char*,uint32_t,uint32_t);
/* Fixed generated subdirectories; no caller-supplied per-asset filenames. */
int fh_install_bundle_stage_root(const char*,unsigned,char out[FH_INSTALL_TX_PATH]);
unsigned fh_install_bundle_stage_mask(unsigned);
/* Small manifest plus file-size checks, no whole-pack reads on normal startup.
 * Falls back to the other valid committed manifest if the newer one is damaged.
 * Full byte verification occurs at publication and in the existing asset readers. */
int fh_install_bundle_open(const char*,const FhInstallBundleSpec*,FhInstallBundle*);
int fh_install_bundle_path(const FhInstallBundle*,unsigned,char out[FH_INSTALL_TX_PATH]);
/* Verify all eight required game files before publishing the root manifest.
 * Stage outputs must already have passed their actual format validators.
 * Cancellation/failure leaves the preceding whole bundle active. 1/0/-1. */
int fh_install_bundle_publish(const char*,const FhInstallBundleSpec*,const FhInstallGeneration stages[4],FhInstallBundleProgress,void*,char*,size_t);
#endif
