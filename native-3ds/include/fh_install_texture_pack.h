#ifndef FH_INSTALL_TEXTURE_PACK_H
#define FH_INSTALL_TEXTURE_PACK_H
#include "fh_install_transaction.h"
#include "fh_install_texture.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {FH_INSTALL_TEXTURE_LIMIT=4096};
enum {FH_INSTALL_TEXTURE_FULL,FH_INSTALL_TEXTURE_REDUCED};
enum {FH_INSTALL_TEXTURE_EXACT=4}; /* ISO scan/assembly already own phases2/3. */
typedef int (*FhInstallTextureStageProgress)(void*,unsigned phase,uint32_t done,uint32_t total);
typedef struct {
    FhInstallTransaction *transaction;
    void *entries;
    unsigned count,admitted,reduced,rejected,duplicates,uncompressed;
    unsigned duplicate_batch,duplicate_pending,duplicate_entry,duplicate_offset;
    unsigned payload_batch_bytes;
    void *mips;
    unsigned mip_count,mip_capacity,mip_admitted,mip_duplicates,mip_rejected;
} FhInstallTextureSpool;
/* The transaction and spool are caller owned and initially zero initialized.
 * Source identity/recipe/cursors are owned by the ISO enumeration stage.
 * Resumes an unfinished spool, including all completed image conversions. */
int fh_install_texture_spool_begin(FhInstallTextureSpool*,FhInstallTransaction*,unsigned artifact);
void fh_install_texture_spool_close(FhInstallTextureSpool*);
/* Optional bounded cursor/payload batching. Zero payload_batch_bytes keeps
 * new images immediately durable; zero duplicate_batch keeps all updates eager.
 * A nonzero byte limit and record limit group only complete image records.
 * Flush on bank boundaries, finish and orderly cancellation. Close alone may
 * discard the uncommitted tail; reopening truncates it to the durable cursor.
 * Input pixels and encoding/quality checks are unchanged. */
int fh_install_texture_spool_flush(FhInstallTextureSpool*);
/* Add one original CMPR image, not the enclosing texture header. max_edge is
 * 0 (no reduction) or a power of two 8..512. Returns 1 completed/duplicate,
 * 0 error, -1 cancelled. RMSE14 rejects lossy candidates; these are prepared
 * as exact RGBA8 instead. No source disappears on a quality rejection. */
int fh_install_texture_spool_add(FhInstallTextureSpool*,const void*,unsigned bytes,unsigned w,unsigned h,unsigned max_edge,
                                uint32_t next_entry,uint32_t next_offset,FhInstallTextureStageProgress,void*);
/* All original non-paletted source formats, including NPOT dimensions and I4
 * opacity planes. Eligible CMPR retains quality-gated compression/reduction;
 * other sources retain exact native precision. The caller's cursor must keep
 * the enclosing source record replayable until all of its planes are durable. */
int fh_install_texture_spool_add_typed(FhInstallTextureSpool*,const void*,unsigned bytes,unsigned w,unsigned h,unsigned source_format,unsigned max_edge,
                                      uint32_t next_entry,uint32_t next_offset,FhInstallTextureStageProgress,void*);
int fh_install_texture_spool_add_mip(FhInstallTextureSpool*,const void*,unsigned bytes,unsigned parent_w,unsigned parent_h,unsigned source_format,
                                    unsigned parent_crc,unsigned level,uint32_t next_entry,uint32_t next_offset,FhInstallTextureStageProgress,void*);
int fh_install_texture_spool_advance(FhInstallTextureSpool*,uint32_t next_entry,uint32_t next_offset);
int fh_install_texture_spool_finish(FhInstallTextureSpool*);
/* Consume a completed spool into FHTXPK2. Resumes at output-entry boundaries,
 * preserves full/reduced variants and exact-source identity. Returns1/0/-1. */
int fh_install_texture_pack(FhInstallTransaction*,unsigned spool,unsigned output,FhInstallTextureProgress,void*);
#ifdef __cplusplus
}
#endif
#endif
