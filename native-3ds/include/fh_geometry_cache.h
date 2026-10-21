#ifndef FH_GEOMETRY_CACHE_H
#define FH_GEOMETRY_CACHE_H
#include "fh_graphics.h"
unsigned fh_geometry_candidate(const void *list,unsigned bytes);
unsigned fh_geometry_vertex_count(const void *list,unsigned bytes);
int fh_geometry_begin(const void *list,unsigned bytes,unsigned vertices,unsigned raster_channels);
void fh_geometry_record(const FhVertex *vertices,unsigned count,const uint32_t *identities);
void fh_geometry_end(void);
void fh_geometry_counters(unsigned *hits,unsigned *misses,unsigned *vertices,unsigned *used);
void fh_geometry_reset(void);
#endif
