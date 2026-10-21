#ifndef FH_PICA_GLOW_H
#define FH_PICA_GLOW_H
/* Source scene boundary only; sampler borrow never captures or allocates. */
void fh_pica_glow_reflection_capture(void);
const void *fh_pica_glow_reflection_image(void);
#include <stdint.h>
typedef struct {
    float position[4]; /* center and scale */
    int16_t angles[4]; /* signed yaw/pitch/phase, shape*8 + template(0/4) */
    uint8_t color[4];
    int16_t uv[4]; /* opposite source corners, in 1/128 texture units */
} FhPicaGlowParticle;
void fh_pica_glow_init(void *vertex_entry);
void fh_pica_glow_invalidate(void);
/* Publish append-only instance ranges before any command-list segment executes. */
void fh_pica_glow_publish(void);
void fh_pica_glow_templates(const int16_t a[4][3],const int16_t b[4][3]);
/* Only contiguous source glow calls may share a group; close before any other
 * pipeline. A resource retirement inside a group must use the barrier first. */
void fh_pica_glow_group_begin(void);
void fh_pica_glow_group_end(void);
void fh_pica_glow_barrier(void);
void fh_pica_glow_begin(const float view[3][4],const float projection[4][4],float near_fade,const unsigned clip[4]);
/* PICA converts packed angles/UV and evaluates rotation; no CPU sin/cos. */
/* Ordered producer lease, <=32 instances. NULL means discard, still commit.
 * End the lease before another renderer call, resource lookup or frame change.
 * Commit compacts conservative visibility in place; GPU fences are owner-only. */
FhPicaGlowParticle *fh_pica_glow_stream_begin(unsigned,unsigned,float,float,unsigned *capacity);
void fh_pica_glow_stream_commit(unsigned count);
void fh_pica_glow_quad(unsigned page,unsigned raster,float near_z,float far_z,const FhPicaGlowParticle *particle);
void fh_pica_glow_dynamic(unsigned page,unsigned raster,float near_z,float far_z,
                          const FhPicaGlowParticle *particle,const int16_t corners[4][3]);
void fh_pica_glow_unsupported(unsigned reason);
void fh_pica_glow_end(void);
void fh_pica_glow_shutdown(void);
#endif
