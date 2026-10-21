#ifndef FH_PICA_BUFFERS_H
#define FH_PICA_BUFFERS_H
#include <3ds/types.h>

/* Immutable, program-lifetime layouts. The shared SDK owner consumes live
 * addresses while retaining format state; never use stack/recycled layouts. */
typedef struct { unsigned count; u32 flags[3][2]; } FhPicaBufferLayout;
int fh_pica_buffers_bind(const FhPicaBufferLayout *layout,const void *const data[3]);
void fh_pica_buffers_invalidate(void);
extern unsigned long long fh_prepared_buffer_counts[5];
/* binds, CPU layout changes, full publications, offset writes, ordinary binds */
#endif
