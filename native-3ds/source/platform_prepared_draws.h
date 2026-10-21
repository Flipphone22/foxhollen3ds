/* Native submission boundary. Descriptors own draw state; referenced GPU
 * storage remains borrowed until prepared_join() retires CPU descriptors,
 * followed by the existing GPU fence before write/free. The experimental
 * shared worker is opt-in; physical performance/correctness remain unvalidated.
 * Adapted ownership discipline: pinned Melee render_snapshot.h/render_worker.c.
 */
#ifndef FH_PLATFORM_PREPARED_DRAWS_H
#define FH_PLATFORM_PREPARED_DRAWS_H
typedef struct {
    unsigned type,color;
    float start,end,near_z,far_z;
} FhPreparedFog;
static FhPreparedFog prepared_fog(void){
    const FhDrawState *s=fh_gx_native_fog_state();
    return (FhPreparedFog){s->fog_type,s->fog_color,s->fog_start,s->fog_end,s->fog_near,s->fog_far};
}
static void prepared_fog_apply(const FhPreparedFog *p,unsigned enable){
    /* The fog controller reads only these six fields. Avoid clearing unrelated
     * GX state; this local object is never passed to the general GX renderer. */
    FhDrawState s;
    s.fog_type=p->type;s.fog_color=p->color;s.fog_start=p->start;
    s.fog_end=p->end;s.fog_near=p->near_z;s.fog_far=p->far_z;
    fh_native_fog_apply_state(enable?&s:NULL);
}
typedef struct {
    FhSceneAsset asset;
    FhSceneGroup group;
    float palette[FH_MODEL_BONES][3][4],projection[4][4];
    FhNativeLighting lighting;
    FhMinimalCallbackMaterial callback;
    FhMinimalModelLayers layers;
    FhTexture texture,sphere;
    C3D_Tex samplers[3];
    unsigned base,illumination,second,has_texture,has_sphere,has_callback,has_layers;
    unsigned has_unlit,has_fade,cloud_depth,cloud_active,lit;
    unsigned char unlit[4],fade[4];
    float alpha;
    unsigned sphere_intensity,material_flags,model_flags,clip[4],movie;
    FhPreparedFog fog;
    u64 start;
#ifdef FH_EMULATOR_TEST
    u64 profile_stamp;
    unsigned capture_scissor[4];
#endif
} FhPreparedModel;
typedef struct {
    FhSceneAsset asset;
    FhSceneGroup group;
    FhMinimalWorldMaterial material;
    float view[3][4],projection[4][4];
    FhTexture image;
    C3D_Tex samplers[3];
    unsigned base,second,third,movie,falloff,clip[4],visible_count;
    FhWorldRange visible[FH_WORLD_MAX_RUNS];
    FhPreparedFog fog;
    u64 start;
} FhPreparedWorld;
typedef struct {
    const void *vertices;
    unsigned count,texture,mask,alpha_only,additive,scissored,effect,effect_blend;
    unsigned clip[4],effect_clip[4];
    FhTexture source,mask_source;
    C3D_Tex samplers[2];
    float view[3][4],projection[4][4],near_z,far_z;
} FhPreparedUi;
#endif
