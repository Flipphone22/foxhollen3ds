#ifndef FH_MINIMAL_GRAPHICS_H
#define FH_MINIMAL_GRAPHICS_H
#include "fh_native_scene.h"
void fh_minimal_coverage_failure(unsigned site,unsigned asset,unsigned detail);
#include "fh_graphics.h"
/* Latch the original copy-clear state for the next native frame. */
void fh_minimal_clear(uint32_t color,uint32_t depth);
/* Independent PICA geometry, base textures/masks and batched HUD primitives.
 * Missing geometry/materials/effects still make this an incomplete renderer. */
extern unsigned fh_minimal_cloud_active,fh_minimal_cloud_depth;
#include "fh_render_floor.h"
extern unsigned fh_minimal_frames,fh_minimal_draws,fh_minimal_indices;
extern unsigned fh_minimal_missing,fh_minimal_rejected;
void fh_minimal_init(void);
int fh_minimal_ready(void);
void fh_minimal_present(void);
void fh_minimal_sync(void);
unsigned fh_minimal_prepare_write(unsigned last_used);
void fh_minimal_shutdown(void);
void fh_minimal_worker_stop(void);
unsigned fh_minimal_worker_executed(void);
int fh_minimal_reflection_update(void *destination,const uint32_t source[4],unsigned width,unsigned height,unsigned format,int half,int clear);
void fh_minimal_projection(const float matrix[4][4]);
/* Read the existing texture object ABI without constructing GX materials. */
int fh_gx_texture_view(const void *object,FhTexture *out);
void fh_minimal_texture_written(const void *address,size_t bytes);
void fh_minimal_texture_hint(void);
/* Only the audited file loader/relocator registers CMPR source lifetimes. */
void fh_minimal_texture_asset(const void *owner,const FhTexture *source);
void fh_minimal_texture_asset_forget(const void *allocation,size_t bytes);
/* Audited synchronous world loop only; end before any external callback. */
void fh_minimal_texture_scope_begin(void);
void fh_minimal_texture_scope_end(void);
int fh_minimal_linear_room(unsigned bytes);
unsigned fh_minimal_texture_cache_release(void);
void fh_minimal_texture_cache_trim(unsigned required_free);
/* Original RCP light selection has already populated GX channel0. Render its
 * diffuse sphere map directly to owned PICA storage; no CPU readback. */
int fh_minimal_sphere_update(unsigned slot,const FhTexture *destination);
extern unsigned fh_minimal_text_color;
extern unsigned fh_minimal_text_alpha_only;
extern FhTexture fh_minimal_text_texture;
/* Material-local movie replacement, captured by the original THP callback.
 * Reset before each material constructor; never retain its borrowed source
 * beyond the synchronous draw's private texture upload. */
extern FhTexture fh_minimal_movie_texture;
extern unsigned fh_minimal_movie_active;
/* Audited callback result, reset at every model material constructor. Borrowed
 * images are consumed synchronously by the existing fenced texture cache. */
typedef struct {
    FhTexture texture,mask;
    unsigned active,constant_rgb,has_texture,has_mask,blend,depth_test,depth_write;
    unsigned char color[4];
    unsigned hologram,cull_front,mask_alpha,normal_disk;
    float projected[3][4],normal_uv[2][4];
} FhMinimalCallbackMaterial;
extern FhMinimalCallbackMaterial fh_minimal_callback;
extern unsigned fh_minimal_hologram_calls;
/* Original HUD projection and scissor, not a replacement menu/text system. */
/* Original perspective menu text/icons, homogeneous clipping, ordered alpha. */
void fh_minimal_projected_image(const FhTexture *texture,const FhTexture *mask,
    const float view[3][4],const float rect[4],float depth,const float uv[4],
    unsigned rgba,unsigned alpha_only,unsigned additive,const unsigned scissor[4]);
void fh_minimal_quad(const FhTexture *texture,const float matrix[4][4],
    const float rect[4],const float uv[4],unsigned rgba,unsigned alpha_only,const unsigned scissor[4]);
enum { FH_MINIMAL_SKY_TINT=2 };
typedef struct {float position[4],color[4],uv[2];} FhMinimalEffectVertex;
/* Ordered original particle quad. Blend: 0 opaque cutout, 1 additive, 2 alpha.
 * Projection is the current native camera; view and viewport range are copied. */
void fh_minimal_effect(const FhTexture *texture,const FhMinimalEffectVertex vertices[4],
    const float view[3][4],unsigned blend,float near_z,float far_z,const unsigned scissor[4]);
void fh_minimal_effect_triangle(const FhTexture *texture,const FhMinimalEffectVertex vertices[3],
    const float view[3][4],unsigned blend,float near_z,float far_z,const unsigned scissor[4]);
/* Blend 0: alpha, 1: additive, SKY_TINT: opaque texture/tint interpolation. */
void fh_minimal_image(const FhTexture *texture,const FhTexture *mask,const float matrix[4][4],
    const float rect[4],const float uv[4],unsigned rgba,unsigned alpha_only,unsigned additive,const unsigned scissor[4]);
typedef struct {
    FhTexture second;
    float scroll[2][2];
    unsigned layered,both_before_lighting,alpha,masked,constant_alpha;
    unsigned constant_rgb,rgb;
} FhMinimalModelLayers;
int fh_minimal_group(const FhSceneAsset *asset,const FhSceneGroup *group,
    const float *poses,unsigned pose_count,const float view[3][4],float alpha,
    const FhTexture *texture,const FhTexture *sphere,unsigned sphere_intensity,
    unsigned material_flags,unsigned model_flags,const unsigned char *unlit_rgba,const unsigned scissor[4],
    const FhMinimalModelLayers *layers,const unsigned char *fade_rgba);
typedef struct {
    float position[3],radius;
    unsigned char color[4];
} FhMinimalWorldLight;
typedef struct {
    FhMinimalWorldLight light[2];
    unsigned count,mode; /* 0 additive, 1 multiply, 2 vertex-alpha blend. */
} FhMinimalWorldLights;
typedef struct {
    FhTexture texture[2]; /* Original radial falloff and inverse depth ramp. */
    float distance;
    unsigned active;
} FhMinimalWorldFalloff;
typedef struct {
    FhTexture texture[2];
    float scroll[2][2],raster[4],near_z;
    unsigned char konst[4];
    unsigned layered,flags;
    FhMinimalWorldLights lighting;
    FhMinimalWorldFalloff falloff;
} FhMinimalWorldMaterial;
/* Synchronous flat-world block; no callbacks/state changes between draws. */
void fh_minimal_world_batch_begin(void);
void fh_minimal_world_batch_end(void);
int fh_minimal_world_group(const FhSceneAsset *asset,const FhSceneGroup *group,
    const float view[3][4],const FhMinimalWorldMaterial *material,const unsigned scissor[4]);
typedef struct {
    FhTexture caustic;
    float scroll[2];
    unsigned char tint[4];
    unsigned valid,heavy_fog;
} FhMinimalWaterMaterial;
extern FhMinimalWaterMaterial fh_minimal_water;
int fh_minimal_water_group(const FhSceneAsset *asset,const FhSceneGroup *group,
    const float view[3][4],const unsigned scissor[4]);
/* Engine19 ripples/wakes retain their original mask and lifetime alpha.
 * Ripple indirect U is green; wake indirect S is alpha (GX indirect ABG). */
typedef struct {
    FhTexture mask;
    unsigned char tint[4],mix[4];
    unsigned valid,heavy_fog,green_mask;
} FhMinimalWaterEffect;
extern FhMinimalWaterEffect fh_minimal_water_effect;
void fh_minimal_water_effect_draw(const FhMinimalEffectVertex vertices[6],
    const float view[3][4],const unsigned scissor[4]);
/* Engine19 splash bands: original per-band transforms/alpha and foam texture.
 * Three distorted reflection samples are replaced by one projective image. */
typedef struct {
    FhTexture mask,foam;
    unsigned char ambient[4],weight[3][4];
    unsigned valid;
} FhMinimalSplashMaterial;
extern FhMinimalSplashMaterial fh_minimal_splash;
void fh_minimal_splash_draw(const FhMinimalEffectVertex vertices[630],const unsigned scissor[4]);
#endif
