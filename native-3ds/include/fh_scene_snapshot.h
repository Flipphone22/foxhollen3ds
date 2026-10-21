#ifndef FH_SCENE_SNAPSHOT_H
#define FH_SCENE_SNAPSHOT_H
#include <stdint.h>
#include <string.h>
/* Each snapshot has its own aligned slot plus three spare bytes. Match the
 * live source's low address bits so the bounded ARM comparator can use words.
 * This changes storage alignment, never source validation or source ownership. */
static inline unsigned fh_scene_snapshot_space(unsigned bytes){return (bytes+6u)&~3u;}
static inline unsigned fh_scene_snapshot_align(unsigned char *snapshots,unsigned at,
    const void *source,unsigned bytes){
    unsigned next=at-((uintptr_t)(snapshots+at)&3u)+((uintptr_t)source&3u);
    if(next!=at && bytes)memmove(snapshots+next,snapshots+at,bytes);
    return next;
}
#endif
