#ifndef FH_PICA_DRAW_STREAM_H
#define FH_PICA_DRAW_STREAM_H
#include <3ds.h>
#include <c3d/attribs.h>
#include <c3d/buffers.h>
#include <c3d/texenv.h>

/* Ordinary world lane prototype. These are native PICA descriptors, not GX
 * inputs. Compilation happens when resident geometry/material state changes.
 * The caller owns all image/geometry lifetimes through the last GPU fence.
 * Texture types other than 2D and geometry shaders are outside this lane.
 * Shader, target, fog LUT, lighting, TEV4/5, bools and camera are lane owners.
 * Do not submit these commands through the SDK without a coherent handoff. */
enum { FH_PICA_DRAW_STREAM_WORDS = 192 };
typedef struct {
    uint32_t border,dim,param,lod,address,format;
} FhPicaStreamSampler;
typedef struct { uint32_t words[8]; } FhPicaSamplerCommands;
typedef struct {
    uint32_t zBuffer,zScale,zOffset,cullMode;
    uint32_t alphaTest,stencilMode,stencilOp,depthTest;
    uint32_t blendClr,alphaBlend;
    GPU_LOGICOP clrLogicOp;
    uint32_t fragOpMode,fragOpShadow,earlyDepth;
    GPU_EARLYDEPTHFUNC earlyDepthFunc;
    uint32_t earlyDepthRef;
} FhPicaStreamEffect;
typedef struct {
    C3D_AttrInfo attributes;
    C3D_BufInfo buffers;
    FhPicaStreamEffect effect;
    C3D_TexEnv stages[4];
    uint32_t index_address,index_count,texture_units;
} FhPicaDrawDescription;
typedef struct {
    uint32_t words[FH_PICA_DRAW_STREAM_WORDS];
    uint16_t count,sampler[2],texture,environment,stages,uniforms;
    uint16_t texture_units;
} FhPicaDrawCommands;
typedef struct {
    const FhPicaSamplerCommands *samplers[2];
    /* Optional load-compiled raster alternative, of draw.effect_words words.
     * Allows depth/alpha variants to share one immutable geometry descriptor. */
    const uint32_t *effect;
    /* Preserve SDK config outside the two selected unit-enable bits. Cache
     * invalidation is emitted after enable routing, even for same-address
     * dynamic content. The caller publishes modified image bytes separately. */
    uint32_t texture_config,texture_shadow;
    uint32_t environment_buffer,environment_color,fog_color;
    uint32_t colors[4];
    /* Native register ordering w,z,y,x; shader registers 7..9. */
    uint32_t material_uniforms[12];
} FhPicaDrawLive;

int fh_pica_stream_sampler(FhPicaSamplerCommands *,unsigned unit,
                          const FhPicaStreamSampler *);
int fh_pica_stream_compile(FhPicaDrawCommands *,const FhPicaDrawDescription *);
/* Does not allocate, wait, rebuild descriptors, update SDK state, flush or
 * submit. Appends to a caller-owned, unsubmitted command segment. Failure is
 * atomic: destination and cursor are unchanged. Immutable commands can be
 * shared by frames; live values only patch the new segment, never the plan.
 * Returns words appended, or zero. Source and destination must not overlap. */
unsigned fh_pica_stream_append(uint32_t *destination,unsigned capacity,
                              unsigned *cursor,const FhPicaDrawCommands *,
                              const FhPicaDrawLive *);

/* Load-time interning makes equality an immutable identity comparison during
 * drawing. Arena storage is caller-supplied normal RAM (word-aligned); it must
 * remain immutable/resident while any plan or batch borrows it. No reclaim or
 * address reuse until batches are invalidated and GPU readers are retired. */
typedef struct { uint32_t *words,capacity,used,buckets[64]; } FhPicaStreamArena;
typedef struct {
    const uint32_t *attributes,*layout,*effect,*stages,*draw;
    uint32_t offsets[12],effect_words,texture_units,offset_candidates;
} FhPicaResidentDraw;
typedef struct {
    const uint32_t *attributes,*layout,*effect,*stages;
    const FhPicaSamplerCommands *samplers[2];
    uint32_t offsets[12],colors[4],uniforms[12],texture_config,texture_shadow;
    uint32_t environment[3],valid,offset_candidates;
} FhPicaStreamState;
int fh_pica_stream_arena_init(FhPicaStreamArena *,uint32_t *,unsigned words);
int fh_pica_stream_resident_compile(FhPicaStreamArena *,FhPicaResidentDraw *,
                                   const FhPicaDrawDescription *);
/* Ordinary world owns depth/cull/alpha/blending only. Preserve inherited
 * stencil, early-depth, blend constant, logic op and fragment-mode fields. */
int fh_pica_stream_world_compile(FhPicaStreamArena *,FhPicaResidentDraw *,
                                const FhPicaDrawDescription *);
/* Reset at every external SDK/program/target/queue context handoff. The
 * corresponding SDK shadow invalidation remains the caller's responsibility.
 * Sampler command objects must be immutable for the entire batch. */
void fh_pica_stream_state_reset(FhPicaStreamState *);
unsigned fh_pica_stream_resident_append(uint32_t *,unsigned,unsigned *,
                                       FhPicaStreamState *,const FhPicaResidentDraw *,
                                       const FhPicaDrawLive *,unsigned flush_textures);
#endif
