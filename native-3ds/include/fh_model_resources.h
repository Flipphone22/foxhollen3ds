#ifndef FH_MODEL_RESOURCES_H
#define FH_MODEL_RESOURCES_H
#include <stddef.h>
#include <stdint.h>
typedef struct { const void *data;size_t bytes; } FhModelResourceRange;
/* Producer operations join outstanding CPU readers before changing the
 * directory. This directory does not authorize asynchronous game pointers. */
void fh_model_resources_register(const void *owner,size_t bytes,const FhModelResourceRange *ranges,unsigned count);
void fh_model_resources_escape(const void *owner);
void fh_model_resources_forget(const void *owner);
/* Model instances can be embedded in a larger game-object allocation. */
void fh_model_resources_forget_allocation(const void *allocation,size_t bytes);
/* Producer-side notification before an audited deformation writes its bytes.
 * Join outstanding CPU readers before advancing the ownership generation. */
void fh_model_resources_write_begin(const void *address,size_t bytes);
/* Reader-owner operations: invoke writes in the existing ordered cache
 * notification path, never directly from a concurrent producer. */
void fh_model_resources_written(const void *address,size_t bytes);
unsigned fh_model_resource_version(const void *address,size_t bytes);
/* Caller keys must already match the exact address and byte count. */
int fh_model_resource_match(unsigned *observed,const void *data,const void *snapshot,size_t bytes);
extern unsigned fh_model_resources_mode; /* 0 off, 1 fast, 2 verify every hit */
extern unsigned fh_model_resources_registered,fh_model_resources_rejected,fh_model_resources_escaped,fh_model_resources_writes;
extern unsigned fh_model_resources_hits,fh_model_resources_checks,fh_model_resources_violations;
extern uint64_t fh_model_resources_reused_bytes;
extern const unsigned fh_model_resources_storage_bytes;
#endif
