#ifndef FH_RESOURCE_TABLES_H
#define FH_RESOURCE_TABLES_H
#include <stdint.h>
/* Engine-thread owner only. No pointers are dereferenced by this cache.
 * Input reads/swaps revoke all entries before mutation. Pending DVD work
 * prevents publication, including writes completed before their callback. */
void fh_resource_tables_invalidate(void);
int fh_resource_merge_hit(const void *dst,const void *a,const void *b,int busy);
void fh_resource_merge_publish(const void *dst,const void *a,const void *b,int busy);
int fh_dvd_resource_pending(void);
extern unsigned fh_resource_merge_hits,fh_resource_merge_misses;
#endif
