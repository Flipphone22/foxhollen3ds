#ifndef FH_GX_INTERNAL_H
#define FH_GX_INTERNAL_H
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include "fh_graphics.h"
#include "fh_platform.h"
#include "fh_native_lighting.h"
typedef struct { uint32_t count,type,fraction; } FhVtxFormat;
typedef struct { const unsigned char *data;uint32_t bytes,stride,little_endian; } FhVtxArray;
typedef struct { uint32_t type,source,matrix,normalize,postmatrix; } FhTexGen;
typedef struct {
    FhDrawState draw;
    uint32_t descriptors[26];
    FhVtxFormat formats[8][26];
    FhVtxArray arrays[26];
    float position[64][3][4],normal[64][3][4],texture[64][3][4],post[64][3][4],projection[4][4];
    uint32_t current_matrix,projection_type,channels;
    FhTexGen texgens[8];
    FhLight lights[8];
    FhChannel channel[4];
    float ambient[2][4],material[2][4];
    const void *palettes[32];
    uint32_t palette_formats[32],palette_counts[32];
    uint32_t clear_color,clear_depth,copy_source[4],copy_width,copy_height,copy_format,copy_mipmap;
    uint32_t display_source[4],display_width,display_height,pixel_format,z_format,dither,z_before,line_width,point_size;
    uint32_t normal_enabled,binormal_enabled,normal_type;
    uint32_t line_offset,point_offset,line_offsets,point_offsets;
} FhGx;
extern FhGx fh_gx;
extern unsigned fh_gx_active_texgens;
extern unsigned fh_gx_texgen_inputs;
void fh_gx_require(int condition,const char *message);
void fh_gx_reset(void);
void fh_gx_bounds_reset(void);
void fh_gx_end_primitive(void);
void fh_gx_fifo_write(const void *bytes,unsigned count);
#if defined(FH_GX_DISPATCH_IMPL) || defined(FH_GX_PRODUCER_IMPL)
#include "fh_gx_dispatch_ops.h"
#endif
#endif
