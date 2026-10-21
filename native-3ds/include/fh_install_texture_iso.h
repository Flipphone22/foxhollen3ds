#ifndef FH_INSTALL_TEXTURE_ISO_H
#define FH_INSTALL_TEXTURE_ISO_H
#include "fh_install_record.h"
#include "fh_install_texture_pack.h"
enum {FH_INSTALL_TEX_BANKS=256,FH_INSTALL_TEX_OFFSETS=8192,FH_INSTALL_TEX_TABLE_BYTES=65536};
enum {FH_INSTALL_TEX_ERROR=-1,FH_INSTALL_TEX_DONE=0,FH_INSTALL_TEX_IMAGE=1,FH_INSTALL_TEX_SKIP=2};
enum {FH_INSTALL_TEX_SCAN=2,FH_INSTALL_TEX_ASSEMBLE=3,FH_INSTALL_TEX_COLLECTION=5};
typedef struct {unsigned file,table,bytes;} FhInstallTextureBank;
typedef struct {
    FhInstallDisc *disc;
    FhInstallTextureBank banks[FH_INSTALL_TEX_BANKS];
    unsigned offsets[FH_INSTALL_TEX_OFFSETS];
    unsigned banks_count,bank,count,index,resume_offset,loaded;
    unsigned decoded_records,eligible_records,skipped_records,short_records;
    void *decoded;
    char error[112];
} FhInstallTextureScan;
typedef struct {
    unsigned file,offset,next_file,next_offset,width,height,bytes;
    const void *pixels;
} FhInstallTextureImage;
typedef struct {
    unsigned file,offset,next_file,next_offset,bytes;
    const void *data;
} FhInstallTextureRecord;
/* Zero initialize. ISO is borrowed read-only. Returned pixels remain valid
 * until next/close; one decoded record at a time. Resume uses the exact owned
 * file/end-of-record cursor published by the texture spool. */
int fh_install_texture_scan_begin(FhInstallTextureScan*,FhInstallDisc*,unsigned file,unsigned offset);
int fh_install_texture_scan_next(FhInstallTextureScan*,FhInstallTextureImage*);
/* Complete table stream, including non-CMPR records and preload wrappers.
 * Does not infer a pixel/palette layout or silently mark wrappers converted. */
int fh_install_texture_scan_record(FhInstallTextureScan*,FhInstallTextureRecord*);
void fh_install_texture_scan_close(FhInstallTextureScan*);
/* Full texture stage, synchronous startup task. Progress phases FULL/REDUCED
 * count tiles, SCAN counts banks, ASSEMBLE counts output entries. 1/0/-1 means
 * complete/error/cancelled. Does not activate an otherwise incomplete recipe. */
int fh_install_texture_prepare(FhInstallTransaction*,FhInstallDisc*,unsigned spool,unsigned output,unsigned max_edge,FhInstallTextureStageProgress,void*);
typedef struct {unsigned records,planes,preload_wrappers,incomplete_mip_chains;} FhInstallTextureCoverage;
/* All source base planes used by the current native renderer; mip chains are
 * inventoried but not uploaded by this recipe. Coverage counts this invocation,
 * not earlier resumed invocations. A record cursor advances only after both
 * colour and opacity are durable. No complete-game activation. */
int fh_install_texture_prepare_bases(FhInstallTransaction*,FhInstallDisc*,unsigned spool,unsigned output,unsigned max_edge,FhInstallTextureCoverage*,FhInstallTextureStageProgress,void*);
#endif
