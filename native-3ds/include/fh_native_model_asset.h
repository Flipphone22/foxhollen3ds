#ifndef FH_NATIVE_MODEL_ASSET_H
#define FH_NATIVE_MODEL_ASSET_H
#include "fh_native_model.h"
#include "fh_model_resources.h"
typedef struct {
    const unsigned char *data;unsigned bytes,id,source_bytes,source_crc;
    unsigned draws,groups,bones,bindings;
    unsigned tails_at,bindings_at,groups_at,draws_at,topology_at,topology_count;
} FhModelAsset;
typedef struct {
    unsigned shader,format,cull,first,draws,bones,vertices,indices;
    const uint16_t *bone_ids,*index;
    const FhModelRef *refs;
    unsigned normal_palette_matches;
} FhModelGroup;
int fh_native_model_asset_open(FhModelAsset *out,const void *data,unsigned bytes,unsigned id,unsigned source_bytes,unsigned source_crc);
int fh_native_model_asset_group(const FhModelAsset *asset,unsigned group,FhModelGroup *out);
int fh_native_model_asset_draw(const FhModelAsset *asset,unsigned ordinal,unsigned list,unsigned shader,unsigned *group);
const void *fh_native_model_asset_topology(const FhModelAsset *asset,unsigned index,unsigned *bytes);
/* Registration is optional and synchronous at model load, before relocation.
 * No SD reads take place from per-frame lookup. */
void fh_native_model_register(const void *owner,const void *raw,unsigned bytes,unsigned id);
void fh_native_model_seal(const void *owner,const FhModelResourceRange *ranges,unsigned lists,const void *instructions,unsigned instruction_bytes);
void fh_native_model_forget(const void *allocation,unsigned bytes);
const FhModelAsset *fh_native_model_find(const void *owner);
int fh_native_model_source_matches(const FhModelAsset *asset,unsigned index,const void *source,unsigned bytes);
void fh_native_model_shutdown(void);
extern unsigned fh_native_models_enabled,fh_native_models_loaded,fh_native_models_rejected,fh_native_models_bytes;
#endif
