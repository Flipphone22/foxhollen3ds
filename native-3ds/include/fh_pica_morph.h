#ifndef FH_PICA_MORPH_H
#define FH_PICA_MORPH_H
#include "fh_native_scene.h"
/* Compile once at model admission. Only groups containing morphable source
 * positions need separate lanes. Static attributes are packed once into a
 * hardware-validated 32-byte stream; indices remain in the resident asset.
 * Each instance owns its fenced positions and immutable packed attributes. */
typedef struct {unsigned group,offset,vertices,first,count;} FhPicaMorphGroup;
typedef struct {uint16_t source,destination;} FhPicaMorphWrite;
typedef struct {unsigned groups,writes,bytes;} FhPicaMorphSize;
/* All counts validated before output; NULL outputs measure the exact plan.
 * Bit mask contains the union of source positions changed by all morphs. */
int fh_pica_morph_compile(const FhSceneAsset*,const uint8_t *mask,unsigned source_count,
    FhPicaMorphSize*,FhPicaMorphGroup*,unsigned,FhPicaMorphWrite*,unsigned);
/* Caller owns unpublished/retired storage. Initialize ALL slots once; subsequent
 * publication touches only the load-compiled affected positions. No allocations,
 * topology generation, source interpretation or live validation in apply(). */
void fh_pica_morph_initialize(void*,const FhSceneAsset*,const FhPicaMorphGroup*,unsigned);
void fh_pica_morph_pack_attributes(void*,const FhSceneAsset*,const FhPicaMorphGroup*,unsigned);
void fh_pica_morph_apply(void*,const unsigned char*,const FhPicaMorphGroup*,unsigned,const FhPicaMorphWrite*);
void fh_pica_morph_attach(const void *owner,const void *file,const void *poses0,const void *poses1,const uint8_t *mask,unsigned sources);
/* NULL selects the original base stream without overwriting queued readers. */
void fh_pica_morph_changed(const void *owner,const unsigned char *positions);
const void *fh_pica_morph_view(const FhSceneAsset*,const float *poses);
const void *fh_pica_morph_positions(const void *view,unsigned group);
const void *fh_pica_morph_attributes(const void *view,unsigned group);
void fh_pica_morph_forget(const void*,unsigned);
void fh_pica_morph_shutdown(void);
#endif
