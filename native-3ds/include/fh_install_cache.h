#ifndef FH_INSTALL_CACHE_H
#define FH_INSTALL_CACHE_H
#include "fh_install_transaction.h"
/* Called after each source block; false cancels. This is startup work, not a
 * gameplay worker. Progress is uncompressed bytes, never a predicted timer. */
typedef int (*FhInstallCacheProgress)(void*,uint32_t done,uint32_t total);
/* Build the existing FHBCACH1 runtime format from a completed native artifact.
 * Result: 1 complete, 0 error, -1 cancelled. Input remains untouched. This
 * stage restarts at zero after interruption, retaining previous stages.
 * level=0 (fast) or 3..12 (HC); output decodes identically for every level. */
int fh_install_cache(FhInstallTransaction*,unsigned input,unsigned output,int level,FhInstallCacheProgress,void*);
#endif
