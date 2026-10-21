#ifndef FH_PICA_HUD_H
#define FH_PICA_HUD_H
#include "fh_graphics.h"
#include "fh_pica_glow_packed.h"
const FhGlowMeshView *fh_pica_hud_glow_mesh(unsigned page,const int16_t uv[4]);
/* Pages are uploaded only at their producer's load/commit boundary. */
void fh_pica_hud_init(void *vertex_entry);
void fh_pica_hud_shutdown(void);
void fh_pica_hud_load(const void *owner,const FhTexture*);
void fh_pica_hud_load_image(const void *owner,const FhTexture*,const FhTexture *mask);
unsigned fh_pica_hud_find(const void *owner);
void fh_pica_hud_forget(const void *allocation,unsigned bytes);
void fh_pica_hud_relocate(const void *old_owner,const void *new_owner);
void fh_pica_hud_release(const void *owner);
void fh_pica_hud_release_many(const void *const *owners,unsigned count);
unsigned fh_pica_hud_page(const void *owner);
extern unsigned fh_pica_hud_text_page;
extern unsigned fh_pica_hud_text_alpha_only;
void fh_pica_hud_quad(unsigned page,const float matrix[4][4],const float rect[4],
    const float uv[4],unsigned rgba,unsigned alpha_only,const unsigned scissor[4]);
void fh_pica_hud_image(unsigned page,const float matrix[4][4],const float rect[4],
    const float uv[4],unsigned rgba,unsigned additive,const unsigned scissor[4]);
void fh_pica_hud_colored(unsigned page,const float matrix[4][4],const float rect[4],
    const float uv[4],unsigned rgba,unsigned additive,const unsigned scissor[4]);
void fh_pica_hud_projected(unsigned page,const float projection[4][4],const float view[3][4],
    const float rect[4],float depth,const float uv[4],unsigned rgba,unsigned alpha_only,
    unsigned additive,const unsigned scissor[4]);
/* Game-side descriptor extraction is restricted to load/commit producers. */
void fh_pica_game_hud_loaded(void *texture);
void fh_pica_game_glow_mask_loaded(void *texture);
int fh_pica_hud_glow_mask_load(const void *owner,const FhTexture*);
int fh_pica_hud_glow_mask_ready(const void *owner);
void fh_pica_game_hud_loading_done(void);
struct Texture;
void fh_pica_game_hud_colored(struct Texture*,int x,int y,unsigned *color,int scale,int flag);
void fh_pica_hud_flush(void);
/* Native ground decal: immutable quad, resident source mask, live plane matrix.
 * Independent of HUD visibility; preserves original decal depth and alpha. */
void fh_pica_ground_shadow(unsigned page,const float projection[4][4],
    const float view[3][4],unsigned alpha,const unsigned clip[4]);
/* Opaque sky gradient: resident phase pair and optional frozen transition. */
void fh_pica_hud_gradient(unsigned a,unsigned b,unsigned previous,unsigned phase,unsigned fade,unsigned tint,
    const float projection[4][4],const float rect[4],float depth,const float uv[4],const unsigned clip[4]);
extern unsigned fh_pica_sky_snapshots,fh_pica_sky_snapshot_bytes;
extern uint64_t fh_pica_sky_snapshot_ticks;
unsigned fh_pica_game_sky_tint(void);
void fh_pica_hud_invalidate(void);
/* Verified host provides frame ownership, clipping and accounting only. */
int fh_pica_hud_target(const unsigned scissor[4],unsigned clip[4]);
void fh_pica_hud_account(unsigned vertices);
#endif
