#ifndef FH_RETAINED_GEOMETRY_H
#define FH_RETAINED_GEOMETRY_H
#include "fh_graphics.h"
enum { FH_NATIVE_GEOMETRY_BUDGET=6*1024*1024 };
extern unsigned fh_retained_disabled;
/* A changed previously retained list can use indexed decoding while keeping
 * its old immutable snapshot. Mode 2 forces ordinary recording if declined. */
extern unsigned fh_retained_prefer_compiled;
/* Development experiments may raise admission limits, never the fixed arena. */
extern unsigned fh_retained_max_vertices,fh_retained_max_list_bytes;
void *fh_renderer_retained_alloc(unsigned bytes);
/* CPU-only metadata/snapshots: never pass this storage to PICA or DMA. */
void *fh_renderer_cpu_cache_alloc(unsigned bytes);
/* IDs denote immutable complete decoded batches. GPU replay may decline if
 * material packing changes; the owner must retain a fresh scalar fallback. */
int fh_renderer_replay_retained(uint64_t id,unsigned count,const FhDrawState *state);
void fh_renderer_draw_retained(uint64_t id,const FhVertex *vertices,unsigned count,const FhDrawState *state,const uint32_t *identities);
int fh_renderer_replay_retained_points(uint64_t id,unsigned count,const FhDrawState *state,unsigned width,unsigned offsets,float texture_offset);
void fh_renderer_draw_retained_points(uint64_t id,const FhVertex *vertices,unsigned count,const FhDrawState *state,unsigned width,unsigned offsets,float texture_offset);
void fh_renderer_native_geometry_stats(unsigned *hits,unsigned *vertices,unsigned *bytes);
int fh_retained_begin(const void *list,unsigned bytes,unsigned vertices,unsigned format,unsigned raster,unsigned enabled);
/* Read decoded unique packets into caller-owned CPU staging. Uses the same
 * source verification and arena; it never submits or retains GPU commands. */
int fh_retained_read_compiled(const void *list,unsigned bytes,unsigned vertices,unsigned format,unsigned raster,FhVertex *output);
uint64_t fh_retained_compiled_identity(void);
uint64_t fh_retained_probe_compiled(const void *list,unsigned bytes,unsigned vertices,unsigned format,unsigned raster);
void fh_retained_record(const FhVertex *vertices,unsigned count,const uint32_t *identities);
void fh_retained_end(void);
void fh_retained_reset(void);
void fh_retained_stats(unsigned *hits,unsigned *misses,unsigned *vertices,unsigned *used);
/* Owned source snapshots shared by geometry and conservative local bounds.
 * Tokens expire on replacement; validation includes every declared byte. */
unsigned fh_retained_source_acquire(const void *data,unsigned bytes);
int fh_retained_source_valid(unsigned token,const void *data,unsigned bytes);
/* Only the audited, synchronous SFA fur loop may declare these immutable
 * attribute ranges, after its first pass has completed animation/blending.
 * Matrices/materials remain live; lists are still compared on every reuse. */
void fh_retained_scope_begin(const void *const addresses[4],const unsigned bytes[4]);
/* Audited model instruction loop only, after animation and callbacks.
 * Declines nested scopes. With dispatch, the caller registers immutable lists
 * before publishing commands and joins readers before leaving the loop. */
unsigned fh_retained_model_scope_begin(const void *const addresses[4],const unsigned bytes[4]);
/* 0 off; 1 may dispatch leased draws; 2 source verification only, synchronous. */
extern unsigned fh_retained_model_scope_enabled;
extern unsigned fh_retained_two_sets,fh_retained_secondary_lookups,fh_retained_secondary_hits,fh_retained_secondary_records;
extern unsigned fh_retained_ranges_enabled,fh_retained_range_scans,fh_retained_range_fallbacks;
extern unsigned fh_retained_key_mode,fh_retained_key_calls,fh_retained_key_checks;
extern uint64_t fh_retained_key_bytes,fh_retained_key_dense_bytes;
extern uint64_t fh_retained_range_full_bytes,fh_retained_range_used_bytes,fh_retained_validation_bytes;
void fh_retained_scope_end(void);
void fh_retained_scope_invalidate(const void *address,size_t bytes);
int fh_retained_scope_contains(const void *address,unsigned bytes);
/* Producer admission and reader verification have separate storage. Reader
 * operations execute in command order, never by inspecting producer globals. */
int fh_retained_producer_scope_contains(const void *address,unsigned bytes);
void fh_retained_reader_scope_begin(const void *const addresses[4],const unsigned bytes[4]);
void fh_retained_reader_scope_end(void);
extern unsigned fh_retained_scope_disabled,fh_retained_scope_validate;
extern unsigned fh_retained_scope_reuses,fh_retained_scope_checks,fh_retained_scope_violations;
extern uint64_t fh_retained_scope_reused_bytes;
#endif
