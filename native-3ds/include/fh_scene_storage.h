#ifndef FH_SCENE_STORAGE_H
#define FH_SCENE_STORAGE_H
#include "fh_graphics.h"
/* Renderer-owned pages count against the existing texture and linear budgets.
 * Allocate returns unpublished storage; only commit makes it drawable. */
void *fh_renderer_scene_page_allocate(unsigned page);
void fh_renderer_scene_page_commit(unsigned page);
void fh_renderer_scene_page_cancel(unsigned page);
const FhTexture *fh_renderer_scene_page(unsigned page);
void fh_renderer_scene_storage_join(void);
unsigned fh_renderer_scene_upload_begin(unsigned last_used);
unsigned fh_renderer_scene_frame(void);
#endif
