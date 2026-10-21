#ifndef FH_INSTALL_SIMPLIFY_H
#define FH_INSTALL_SIMPLIFY_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum {FH_INSTALL_SIMPLIFY_SCRATCH=8*1024*1024};
typedef struct {unsigned*indices;unsigned count;float error;size_t scratch_peak;char message[112];} FhInstallSimplification;
/* Startup-only pinned meshoptimizer kernel, bounded private arena. No source
 * attributes or vertices are rewritten. Caller must supply deformation locks
 * and validate surviving protected faces plus bidirectional surface fidelity.
 * Success is a candidate, not permission to publish a reduced game asset.
 * Non-reentrant; zero initialize and close the owner. C++ exceptions are
 * contained inside this adapter, including bounded-arena exhaustion. */
int fh_install_simplify(FhInstallSimplification*,const unsigned*indices,unsigned count,const float*positions,unsigned vertices,
                        const float*attributes,unsigned channels,const float*weights,const unsigned char*locks,
                        float ratio,float error,unsigned permissive,size_t scratch_capacity);
void fh_install_simplify_close(FhInstallSimplification*);
#ifdef __cplusplus
}
#endif
#endif
