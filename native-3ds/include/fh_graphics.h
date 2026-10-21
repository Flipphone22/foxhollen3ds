#ifndef FH_GRAPHICS_H
#define FH_GRAPHICS_H
#include <stdint.h>
#include <stddef.h>
#include "fh_screen.h"
/* Explicit diagnostic: no geometry translation or GPU work. This is never a
 * playable renderer and its cadence must not be reported as rendered FPS. */
#include "fh_render_floor.h"

typedef struct {
    const void *pixels,*palette;
    uint32_t width,height,format,palette_format,palette_count;
    uint32_t wrap_s,wrap_t,min_filter,mag_filter;
    float min_lod,max_lod,lod_bias;
    uint32_t serial;
} FhTexture;
typedef struct {
    float position[4],color[2][4],uv[8][3];
    float matrix_index;
} FhVertex;
typedef struct {
    uint32_t color[4],alpha[4],color_op,alpha_op,color_bias,alpha_bias;
    uint32_t color_scale,alpha_scale,color_clamp,alpha_clamp,color_dest,alpha_dest;
    uint32_t coord,map,channel,kcolor,kalpha,ras_swap,tex_swap;
    uint32_t indirect[9];
} FhTevStage;
typedef struct {
    FhTexture textures[8];
    FhTevStage tev[16];
    float tev_colors[4][4],konst_colors[4][4];
    uint32_t swaps[4][4],stages,texgens;
    uint32_t depth_test,depth_write,depth_compare,cull;
    uint32_t blend_mode,blend_src,blend_dst,logic_op,color_write,alpha_write;
    uint32_t alpha_compare[5],scissor[4];
    float viewport[6];
    uint32_t fog_type,fog_color;
    float fog_start,fog_end,fog_near,fog_far;
    uint32_t indirect_count,indirect_order[4][2],indirect_scale[4][2];
    float indirect_matrix[3][2][3];
    int32_t indirect_exponent[3];
    uint32_t pixel_format;
    uint32_t constant_raster_mask;
    float constant_raster[2][4];
    uint32_t raw_position;
    float position_matrix[4][4];
    float model_palette[30][4];
} FhDrawState;
size_t fh_texture_source_bytes(unsigned format,unsigned width,unsigned height);
unsigned fh_texture_native_format(const FhTexture *texture);
size_t fh_texture_native_bytes(const FhTexture *texture,unsigned width,unsigned height);
int fh_texture_convert(void *output,size_t output_bytes,unsigned width,unsigned height,const FhTexture *texture,size_t source_bytes);
/* Bounded-colour-error CMPR fallback; alpha and texture coordinates unchanged. */
int fh_texture_convert_cmpr16(void *output,size_t output_bytes,unsigned width,unsigned height,const FhTexture *texture,size_t source_bytes);
void fh_renderer_init(void);
int fh_renderer_ready(void);
int fh_renderer_transform_available(void);
int fh_renderer_points_available(const FhDrawState *state);
void fh_renderer_draw_points(const FhVertex *vertices,unsigned count,const FhDrawState *state,unsigned width,unsigned offsets,float texture_offset);
int fh_renderer_cpu_cull_available(const FhDrawState *state);
int fh_renderer_gpu_cull_available(const FhDrawState *state);
extern unsigned fh_bounds_pair_enabled,fh_bounds_pair_calls;
unsigned fh_vertex_bounds_pair(const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float pica[4][4],unsigned gx,unsigned gpu);
unsigned fh_vertex_bounds_planes(const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float projection[4][4],unsigned candidates);
unsigned fh_vertex_bounds_any_plane(const float low[3],const float high[3],unsigned matrix,const FhDrawState *state,const float projection[4][4],unsigned candidates);
void fh_vertex_pica_projection(float matrix[4][4],const FhDrawState *state);
unsigned fh_vertex_conservative_outside(const float position[4],unsigned matrix,const FhDrawState *state,const float pica[4][4]);
void fh_vertex_clip_position(float clip[4],const float position[4],unsigned matrix,const FhDrawState *state);
void fh_gx_vertex_counters(unsigned *decoded,unsigned *reused);
void fh_renderer_draw(const FhVertex *vertices,unsigned count,const FhDrawState *state);
/* Nonzero equal identities must denote identical complete input vertices.
 * Both arrays are consumed synchronously; neither is retained by the GPU. */
void fh_renderer_draw_identified(const FhVertex *vertices,unsigned count,const FhDrawState *state,const uint32_t *identities);
void fh_renderer_present(void);
void fh_renderer_sync(void);
void fh_renderer_black(int black);
void fh_renderer_clear(uint32_t rgba,uint32_t depth);
void fh_renderer_invalidate(const void *address,size_t bytes);
void fh_renderer_texture_hint(void);
void fh_renderer_cache_read(const void *address,size_t bytes);
/* Only audited persistent texture destinations may defer their CPU pixels.
 * CPU consumers must materialize before reading; ordinary GXCopyTex does not. */
int fh_renderer_copy_gpu_color(void *destination,const uint32_t source[4],unsigned width,unsigned height,unsigned format,int half_scale,int clear);
int fh_renderer_copy_gpu_depth(void *destination,const uint32_t source[4],unsigned width,unsigned height,unsigned format,int half_scale,int clear);
int fh_renderer_copy_gpu_shadow(void *destination,const uint32_t source[4],unsigned width,unsigned height,unsigned format,int half_scale,int clear,uint32_t color,uint32_t depth,const FhDrawState *state);
void fh_renderer_materialize_texture(const void *pixels);
void fh_renderer_shutdown(void);
enum { FH_EFB_HEIGHT=528,FH_EFB_NATIVE_HEIGHT=264,FH_EFB_NATIVE_WIDTH=400 };
size_t fh_efb_bytes(unsigned width,unsigned height,unsigned format);
int fh_efb_convert(void *destination,size_t capacity,const uint32_t *pixels,const uint32_t rect[4],unsigned width,unsigned height,unsigned format,int has_alpha);
/* Stride is in uint32_t words along the rotated framebuffer's physical rows.
 * The valid visible span remains 264 words per row. Sources stay read-only. */
int fh_efb_convert_filtered_stride(void *destination,size_t capacity,const uint32_t *pixels,unsigned stride,const uint32_t rect[4],unsigned width,unsigned height,unsigned format,int has_alpha,int half_scale);
uint32_t fh_efb_depth_pixel_stride(const uint32_t *pixels,unsigned stride,unsigned x,unsigned y);
int fh_efb_depth_convert_stride(void *destination,size_t capacity,const uint32_t *pixels,unsigned stride,const uint32_t rect[4],unsigned width,unsigned height,unsigned format);
int fh_efb_convert_filtered(void *destination,size_t capacity,const uint32_t *pixels,const uint32_t rect[4],unsigned width,unsigned height,unsigned format,int has_alpha,int half_scale);
uint32_t fh_efb_depth_pixel(const uint32_t *pixels,unsigned x,unsigned y);
int fh_efb_depth_convert(void *destination,size_t capacity,const uint32_t *pixels,const uint32_t rect[4],unsigned width,unsigned height,unsigned format);
void fh_renderer_copy(void *destination,const uint32_t source[4],unsigned width,unsigned height,unsigned format,int half_scale,int clear,uint32_t color,uint32_t depth,const FhDrawState *state);
uint32_t fh_renderer_depth(unsigned x,unsigned y);
#endif
