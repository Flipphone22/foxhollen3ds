#ifndef FH_PICA_ACTOR_COMMANDS_H
#define FH_PICA_ACTOR_COMMANDS_H
#include "fh_pica_draw_stream.h"

/* Native load descriptions, independent of the source renderer. Material and
 * fetch variants are separate so animated lights/textures do not duplicate
 * geometry packets. Queue/shader/light/fog ownership belongs to the caller. */
typedef struct {
    const uint32_t *attributes,*layout,*draw;
    uint32_t offsets[12],cull,offset_candidates;
} FhPicaActorGeometry;
typedef struct {uint32_t words[24],buffer_mask;} FhPicaActorMaterial;
typedef struct {
    const FhPicaSamplerCommands *samplers[3];
    const FhPicaActorMaterial *material;
    /* Prepared alternate instance fetch addresses, or NULL for geometry's. */
    const uint32_t *offsets;
    uint32_t colors[4],uniforms[12],scale[4]; /* GPU w,z,y,x order */
    uint32_t texture_config,texture_shadow,environment_buffer,environment_color,fog_color;
    uint32_t alpha_test,depth_test,blend;
} FhPicaActorParameters;
typedef struct {
    const float *joints,*world,*tails; /* joints have sixteen-float stride */
    const uint16_t *bones;
    unsigned joint_count,count,changed;
} FhPicaActorPose;
typedef struct {
    const uint32_t *attributes,*layout;
    const FhPicaActorMaterial *material;
    const FhPicaSamplerCommands *samplers[3];
    uint32_t offsets[12],colors[4],uniforms[12],scale[4],environment[3],raster[4];
    uint32_t texture_config,texture_shadow,valid,uniform_dirty[3],offset_candidates;
} FhPicaActorCommandState;

int fh_pica_actor_geometry_compile(FhPicaStreamArena*,FhPicaActorGeometry*,const FhPicaDrawDescription*);
void fh_pica_actor_material_compile(FhPicaActorMaterial*,const C3D_TexEnv[4],unsigned);
void fh_pica_actor_command_reset(FhPicaActorCommandState*);
/* Atomic bounded publication. Changed pose slots are streamed directly into
 * commands; no temporary palette, SDK shadow, source validation/interpreter,
 * resource allocation, wait or submission. Force all pose slots at each new
 * instance; retain other state until its owner ends or is invalidated.
 * Samplers may all use the unit0 encoding; destinations are routed explicitly.
 * All resource and palette pointers must remain owned for this call. */
unsigned fh_pica_actor_command_append(uint32_t*,unsigned,unsigned*,FhPicaActorCommandState*,
    const FhPicaActorGeometry*,const FhPicaActorParameters*,const FhPicaActorPose*,unsigned);
#endif
