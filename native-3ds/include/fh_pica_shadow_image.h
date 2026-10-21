#ifndef FH_PICA_SHADOW_IMAGE_H
#define FH_PICA_SHADOW_IMAGE_H
/* Main renderer queue only. Persistent colour targets, no CPU image writes.
 * Init before gameplay; shutdown only after the parent has retired all readers.
 * The caller clears with a GPU draw, then draws resident caster geometry.
 * Call rebind after any command-capacity join before continuing capture draws.
 * End publishes the completed image and restores the main target. */
enum { FH_SHADOW_IMAGE_SIZE=128, FH_SHADOW_IMAGE_SLOTS=3,
       FH_SHADOW_IMAGE_BYTES=FH_SHADOW_IMAGE_SLOTS*FH_SHADOW_IMAGE_SIZE*FH_SHADOW_IMAGE_SIZE*2 };
void fh_pica_shadow_image_init(void);
void fh_pica_shadow_image_begin(void);
void fh_pica_shadow_image_rebind(void);
void fh_pica_shadow_image_end(unsigned draws,unsigned indices);
/* Borrowed C3D_Tex; NULL before publication or during capture. */
const void *fh_pica_shadow_image_sampler(void);
void fh_pica_shadow_image_describe(unsigned asset,unsigned frame,unsigned alpha,unsigned receivers);
/* Only before parent scene/UI teardown, after worker stop and initial join. */
void fh_pica_shadow_image_capture_exit(void);
void fh_pica_shadow_image_shutdown(void);
#endif
