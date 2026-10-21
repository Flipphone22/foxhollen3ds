#ifndef FH_AUDIO_EFFECT_ROUTES_H
#define FH_AUDIO_EFFECT_ROUTES_H
#include "fh_audio_effect_epoch.h"

enum { FH_AUDIO_EFFECT_BUSES=2, FH_AUDIO_EFFECT_OBJECTS=4 };
typedef struct {
    uint32_t source_token,discard_surround;
    float parameters[5];
} FhAudioEffectKey;
typedef struct {
    void *bus[FH_AUDIO_EFFECT_BUSES];
    unsigned objects[FH_AUDIO_EFFECT_BUSES]; /* Producer references, index+1. */
    uint32_t token;
} FhAudioEffectRoute;
typedef struct {
    FhAudioEffectKey key;
    void *payload;
    unsigned references; /* Producer only; callback never reads this field. */
    unsigned source_ended; /* Keep detached history until the source destroys it. */
} FhAudioEffectObject;
typedef struct {
    FhAudioEffectEpochs epochs;
    FhAudioEffectRoute routes[FH_AUDIO_EFFECT_SLOTS];
    FhAudioEffectObject objects[FH_AUDIO_EFFECT_OBJECTS];
    void *(*create)(void *,const FhAudioEffectKey *);
    void (*destroy)(void *,void *);
    void *context;
    unsigned current; /* Producer only, route index+1. */
    uint32_t completed; /* Callback only; last successfully ended route. */
} FhAudioEffectRoutes;

/* Bounded native effect payload ownership. One synth producer creates/reclaims
 * objects; one SDK consumer processes returned input. No sample queue, thread,
 * allocator or source pointer is hidden here. A zero source token means no
 * effect. A source lifetime's immutable constructor values form its key.
 * Unchanged buses share the SAME delay/rate history across successive routes.
 * At most two live routes/four effect objects, including detached histories;
 * BUSY never destroys live state. Caller supplies keys from live source objects.
 * A failed factory rolls back the unpublished route only. No SDK calls here. */
int fh_audio_effect_routes_init(FhAudioEffectRoutes *,
    void *(*create)(void *,const FhAudioEffectKey *),void (*destroy)(void *,void *),void *);
int fh_audio_effect_routes_prepare(FhAudioEffectRoutes *,const FhAudioEffectKey [2],uint32_t *);
/* Reclaim only callback-retired routes. Their shared bus objects remain alive
 * until the LAST route reference retires. Does not imply retirement itself. */
unsigned fh_audio_effect_routes_collect(FhAudioEffectRoutes *);
/* Producer's actual source free/recreation event. A detached object can now be
 * freed; queued route references defer destruction until their retirement.
 * Unbinding a bus is NOT source destruction: its history must survive rebinding. */
int fh_audio_effect_routes_source_end(FhAudioEffectRoutes *,uint32_t source_token);
const FhAudioEffectRoute *fh_audio_effect_routes_begin(FhAudioEffectRoutes *,uint32_t);
int fh_audio_effect_routes_end(FhAudioEffectRoutes *,uint32_t);
/* SDK owner only AFTER proving BOTH buses and all queued/returned tags cannot
 * reference an earlier token. An upcoming publication is not such proof.
 * This API does not solve mixed-effect blocks, tails, source resets or pacing. */
int fh_audio_effect_routes_retire_before(FhAudioEffectRoutes *,uint32_t);
/* Only after SDK thread join. No future callbacks or producer operations. */
void fh_audio_effect_routes_shutdown(FhAudioEffectRoutes *);
#endif
