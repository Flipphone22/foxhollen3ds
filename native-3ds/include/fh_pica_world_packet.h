#ifndef FH_PICA_WORLD_PACKET_H
#define FH_PICA_WORLD_PACKET_H
#include "fh_pica_draw_stream.h"
/* Experimental native ordered world packet. Not connected to gameplay yet.
 * Prepare once while unpublished. Update ONLY after the owning GPU fence.
 * The caller owns SDK handoffs, linear storage/publication, live-state input,
 * per-draw visibility and common shader/camera/fog/target compatibility.
 * This API must not be used to render culled or hidden draws indiscriminately.
 */
enum { FH_PICA_WORLD_PACKET_DRAWS=16, FH_PICA_WORLD_PACKET_WORDS=3072 };
typedef struct {
    const FhPicaResidentDraw *draw;
    /* Scalar state lives in the CPU command template itself. These immutable
     * resource identities avoid comparing sampler/raster command bodies. */
    const FhPicaSamplerCommands *samplers[2];
    const uint32_t *effect_key;
    uint16_t effect,sampler[2],texture,environment,stages,uniforms;
} FhPicaWorldPacketEntry;
typedef struct {
    uint32_t *commands;
    unsigned words,count;
    /* Owner-provided storage; capacity checked before any preparation writes. */
    FhPicaWorldPacketEntry *entry;
    unsigned entry_capacity;
} FhPicaWorldPacket;
typedef struct { unsigned writes,first,end; } FhPicaWorldPacketDirty;
/* Prepare may modify its unpublished storage on failure; it never publishes a
 * valid packet until complete. All source descriptors/sampler/raster objects
 * remain immutable and borrowed for the packet lifetime. */
int fh_pica_world_packet_prepare(FhPicaWorldPacket *,uint32_t *,unsigned capacity,
    const FhPicaResidentDraw *const *,const FhPicaDrawLive *,unsigned count);
/* Requires successful prepare and its immutable borrowed descriptors alive.
 * Validates all LIVE input before changing a packet; immutable command/source
 * validation belongs to preparation. Equal live values write
 * nothing. Dirty range is half-open; flush it before the next GPU use. Caller
 * must additionally publish texture pixel changes even if sampler is unchanged.
 * Returns 0 for invalid input, 1 for a valid (possibly unchanged) update. */
int fh_pica_world_packet_update(FhPicaWorldPacket *,const FhPicaDrawLive *,
                                FhPicaWorldPacketDirty *);
#endif
