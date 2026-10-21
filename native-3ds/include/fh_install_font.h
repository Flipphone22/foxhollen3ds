#ifndef FH_INSTALL_FONT_H
#define FH_INSTALL_FONT_H
#include "fh_install_disc.h"
#include "fh_install_texture_pack.h"
enum {FH_INSTALL_FONT_BYTES=128*1024,FH_INSTALL_FONT_IMAGES=3};
#define FH_INSTALL_FONT_CURSOR 0x80000000u
typedef struct {unsigned offset,bytes,width,height,format;} FhInstallFontImage;
typedef struct {
    FhInstallFontImage images[FH_INSTALL_FONT_IMAGES];
    unsigned count,end;
} FhInstallFontLayout;
/* Original big-endian gameTextFinalizeLoad layout. Offsets point to pixel
 * bytes; end is the terminating image header. Validates every image before
 * returning any. No pointer relocation, mutation or allocation. */
int fh_install_font_layout(const void*,unsigned,FhInstallFontLayout*);
/* Append all language/charset atlases after TEX banks in the same spool.
 * Tagged FST-entry/next-header cursors distinguish font and TEX phases.
 * Cancellation retains only complete, durable image conversions. */
int fh_install_font_prepare(FhInstallTextureSpool*,FhInstallDisc*,unsigned,
                            FhInstallTextureStageProgress,void*);
#endif
