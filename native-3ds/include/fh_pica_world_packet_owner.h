#ifndef FH_PICA_WORLD_PACKET_OWNER_H
#define FH_PICA_WORLD_PACKET_OWNER_H
#include "fh_pica_world_packet.h"
/* One render owner, same ownership as the SDK command cursor. No other thread
 * may call these concurrently. Application integration is still experimental.
 */
unsigned fh_pica_packet_append(const FhPicaWorldPacket *);
/* Called on each completed SDK command segment, before its cache flush. */
int fh_pica_packet_publish(uint32_t *segment,unsigned bytes);
/* Called only after successful FrameBegin retires ALL previous GPU readers.
 * Refuses an unsubmitted pending call. Does not wait or establish a fence. */
int fh_pica_packet_frame_retired(void);
uint64_t fh_pica_packet_epoch(void);
/* Implemented beside the private native SDK owner. Requires an active ordinary
 * world lane with no pending shader/target/shared-state changes. */
unsigned fh_pica_stream_packet(const FhPicaWorldPacket *);
#endif
