#ifndef FH_GAME_MEMORY_H
#define FH_GAME_MEMORY_H
/* Shared game archive calls the target-specific support implementation. */
unsigned fh_game_region_reserve(void);
unsigned fh_game_region_size(unsigned region);
#endif
