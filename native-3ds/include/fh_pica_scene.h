#ifndef FH_PICA_SCENE_H
#define FH_PICA_SCENE_H
#include "fh_native_scene.h"
/* Development stage: native diffuse vertex and fragment environment lighting.
 * Specular, spot attenuation and complete material fidelity remain incomplete. */
#define FH_PICA_ACTOR_LIGHTING_EXPERIMENT 1
typedef struct {
    float ambient[4],direction[8][4],color[8][4];unsigned count;
} FhPicaEnvironment;
void fh_pica_environment_init(void);
void fh_pica_environment_shutdown(void);
void fh_pica_environment_update(unsigned,const FhPicaEnvironment*);
void fh_pica_environment_bind(unsigned,unsigned);
void fh_pica_material_environment(const FhSceneAsset*,unsigned,unsigned,unsigned,unsigned);
int fh_pica_material_actor_unlit(const FhSceneAsset*,unsigned,unsigned);
int fh_pica_material_environment_compile(const FhSceneAsset*,unsigned,unsigned);
void fh_pica_scene_environment(unsigned);
void fh_pica_game_environment(void);
#include "fh_graphics.h"
#include "fh_render_floor.h"
extern unsigned fh_pica_scene_textured,fh_pica_scene_actors;
extern unsigned fh_pica_unimplemented_actor_callbacks,fh_pica_unimplemented_world_passes;
typedef struct {unsigned count;void *items;} FhPicaMaterialSet;
typedef struct {
    const void *vertex;const uint16_t *index;
    unsigned indices,vertices,format,cull,material,flags;
} FhPicaWorldDraw;
typedef struct {
    unsigned count;FhPicaMaterialSet materials;
    const void *positions;unsigned position_count,positions_dirty;
    FhPicaWorldDraw draw[];
} FhPicaWorldPlan;
/* Compile once from validated prepared geometry; never interpret GX streams. */
unsigned fh_pica_world_compile(const FhSceneAsset*,FhPicaWorldDraw*,unsigned);
unsigned fh_pica_world_compile_stream(const FhSceneAsset*,unsigned,FhPicaWorldDraw*,unsigned);
const FhPicaWorldPlan *fh_native_scene_pica_plan(const FhSceneAsset*);
int fh_pica_world_attach_positions(const FhSceneAsset*,const void*,unsigned);
void fh_pica_world_positions_changed(const void *owner);
int fh_pica_world_prepare_positions(const FhSceneAsset*);
void fh_pica_scene_init(void);
void fh_pica_scene_shutdown(void);
void fh_pica_scene_invalidate(void);
/* Host owns frame fences and resident asset lifetimes. No GX material input. */
unsigned fh_pica_scene_draw(const FhSceneAsset*,const FhPicaWorldPlan*,const float[3][4],const float[4][4],const unsigned[4],unsigned*);
void fh_pica_scene_world(const FhSceneAsset*,const float[3][4],const float[4][4],const unsigned[4]);
void fh_pica_scene_reserve(void);
/* Returns1 only when reserve handed ownership to the queue/reset path. */
unsigned fh_pica_scene_reserve_changed(void);
int fh_pica_materials_begin(const FhSceneAsset*,unsigned count);
int fh_pica_material_begin(const FhSceneAsset*,unsigned material,unsigned layers,unsigned frames0,unsigned frames1,unsigned flags);
int fh_pica_material_frame(const FhSceneAsset*,unsigned material,unsigned layer,unsigned frame,const FhTexture*);
void fh_pica_material_live(const FhSceneAsset*,unsigned material,const float color[4],const float scroll[2][2],const unsigned frames[2],unsigned konst,unsigned flags);
enum {FH_PICA_POST_NONE,FH_PICA_POST_ADDITIVE,FH_PICA_POST_OPAQUE};
/* Original post-material overrides: both retain depth testing without writes. */
void fh_pica_material_post(const FhSceneAsset*,unsigned material,unsigned state);
void fh_pica_material_movie(const FhSceneAsset*,unsigned material,const void *pixels);
int fh_pica_game_moon_matches(const void *callback);
float fh_pica_game_moon_phase(void);
void fh_pica_material_moon(const FhSceneAsset*,unsigned material,float phase,unsigned alpha);
FhPicaMaterialSet *fh_native_scene_pica_materials(const FhSceneAsset*);
/* SPECULAR_PREFETCH_V1: constant-time load-compiled material demand. */
void fh_pica_material_specular_require(const FhPicaMaterialSet*,unsigned compact);
void fh_pica_materials_release(FhPicaMaterialSet*); /* GPU readers must be retired. */
int fh_pica_material_actor(const FhSceneAsset*,unsigned material,unsigned masked);
int fh_pica_material_actor_alpha_source(const FhSceneAsset*,unsigned,unsigned);
int fh_pica_material_actor_raster(const FhSceneAsset*,unsigned,unsigned);
void fh_pica_material_actor_alpha(const FhSceneAsset*,unsigned,unsigned,unsigned);
int fh_pica_material_actor_solid(const FhSceneAsset *asset,unsigned index,unsigned lit);
int fh_pica_material_actor_layered(const FhSceneAsset*,unsigned material,unsigned mode,unsigned light_after);
unsigned fh_pica_material_actor_has_layer(const FhSceneAsset*,unsigned material);
void fh_pica_scene_actor_second_uv(unsigned enabled);
typedef struct {
    float ambient[4],position[8][4],color[8][4],normal_scale;
    unsigned enabled,vertex_material,active,partial_lights,alpha_lighting;
} FhPicaActorLighting;
/* Direct live game light parameters. No GX state interpreter or vertex loop. */
void fh_pica_scene_actor_lighting(const FhPicaActorLighting*);
void fh_pica_materials_init(void);
int fh_pica_materials_compile(const FhSceneAsset*);
void fh_pica_materials_invalidate(void);
void fh_pica_materials_shutdown(void);
int fh_pica_material_bind(const FhPicaMaterialSet*,unsigned material,unsigned format,unsigned compact);

unsigned fh_pica_material_height_fog_eligible(const FhPicaMaterialSet*,unsigned);
/* Ordinary actor family only. Prepare during admission, outside draw binding.
 * Caller selects actor-height shader and owns prepared geometry/fog planes.
 * The shared height-fog sampler must be prepared before bind. */
int fh_pica_material_actor_height_prepare(const FhPicaMaterialSet*,unsigned);
int fh_pica_material_actor_height_ready(const FhPicaMaterialSet*,unsigned);
/* Load-time, allocation-free potential admission; not live readiness. */
int fh_pica_material_actor_height_capable(const FhPicaMaterialSet*,unsigned);
int fh_pica_material_actor_height_bind(const FhPicaMaterialSet*,unsigned,unsigned,unsigned,unsigned);
int fh_pica_material_swizzle_compile(const FhSceneAsset*,unsigned,unsigned,unsigned);
void fh_pica_material_swizzle_live(const FhSceneAsset*,unsigned,unsigned);
#endif
