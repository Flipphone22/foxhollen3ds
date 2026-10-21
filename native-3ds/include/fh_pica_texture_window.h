#pragma once
/* Renderer-owned CPU file pages only. Safe to release at load-time heap
 * pressure: published textures own independent GPU allocations. */
unsigned fh_pica_texture_window_release(void);
