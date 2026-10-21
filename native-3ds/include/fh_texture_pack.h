#ifndef FH_TEXTURE_PACK_H
#define FH_TEXTURE_PACK_H
#include <stdio.h>
#include <stdint.h>
enum { FH_TEXTURE_WINDOW_PAGE=32768,FH_TEXTURE_WINDOW_PAGES=8 };
enum { FH_TEXTURE_MIP_STRIDE=36,FH_TEXTURE_MIP_ENTRIES=4096 };
typedef struct {
    FILE *file;
    unsigned char *directory;
    unsigned count, directory_bytes, hits, misses, rejected, uploads, stride;
    uint64_t read_bytes, saved_upload_bytes, read_calls;
    unsigned char *resident;
    unsigned *resident_offsets;
    unsigned resident_bytes;
    uint64_t resident_reads,resident_read_bytes;
    unsigned file_bytes;
    unsigned char *window;
    unsigned window_pages,window_tags[FH_TEXTURE_WINDOW_PAGES],window_lengths[FH_TEXTURE_WINDOW_PAGES];
    uint64_t window_age[FH_TEXTURE_WINDOW_PAGES],window_clock,window_hits,window_misses,window_bytes;
    unsigned char *mips;
    unsigned mip_count,mip_reads;
} FhTexturePack;
typedef struct { unsigned offset, format, bytes, crc, original_bytes, width, height, source_crc; } FhPackedTexture;
typedef struct { unsigned offset, chain_bytes, crc, skip, bytes, levels; } FhPackedMips;
/* Single renderer-owner API. An absent/invalid pack leaves original textures usable. */
int fh_texture_pack_open(FhTexturePack *pack,const char *path,unsigned directory_budget);
int fh_texture_pack_find(FhTexturePack *pack,const void *source,unsigned width,unsigned height,FhPackedTexture *out);
int fh_texture_pack_find_surface(FhTexturePack *pack,const void *source,unsigned width,unsigned height,FhPackedTexture *out);
/* Typed source identity for FHTXPK3. Supports the disc's non-paletted formats;
 * source_bytes must match tiled GX extent. CMPR also searches older packs. */
int fh_texture_pack_find_typed(FhTexturePack*,const void *source,unsigned source_bytes,unsigned width,unsigned height,unsigned source_format,unsigned reduced,FhPackedTexture*);
int fh_texture_pack_read(FhTexturePack *pack,const FhPackedTexture *entry,void *output,unsigned capacity);
void fh_texture_pack_close(FhTexturePack *pack);
int fh_texture_pack_mips(FhTexturePack *pack,const FhPackedTexture *base,unsigned width,unsigned height,unsigned source_format,FhPackedMips *out);
int fh_texture_pack_read_mips(FhTexturePack *pack,const FhPackedMips *mips,void *output,unsigned capacity);
/* Optional caller-owned CPU-readable storage; no GPU references are retained.
 * Duplicate source candidates are verified byte-for-byte before sharing. */
unsigned fh_texture_pack_resident_size(const FhTexturePack *pack);
int fh_texture_pack_resident_load(FhTexturePack *pack,void *storage,unsigned capacity);
void fh_texture_pack_resident_drop(FhTexturePack *pack);
/* Optional bounded CPU-only file-page cache; storage belongs to the caller.
 * It may be discarded at any time without affecting published GPU textures. */
int fh_texture_pack_window(FhTexturePack *pack,void *storage,unsigned bytes);
void fh_texture_pack_window_drop(FhTexturePack *pack);
#endif
