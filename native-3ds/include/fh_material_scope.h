#ifndef FH_MATERIAL_SCOPE_H
#define FH_MATERIAL_SCOPE_H
#include "fh_material.h"
/* One vertex-only display-list invocation. Owned plan storage survives renderer
 * material-cache eviction; alternate state pointers always use the oracle. */
typedef const FhMaterial *(*FhMaterialOracle)(const FhDrawState*,const FhVertex*,unsigned,int);
extern unsigned fh_material_scope_mode,fh_material_scope_hits,fh_material_scope_checks;
extern unsigned fh_material_scope_class_hits,fh_material_scope_class_checks;
unsigned fh_renderer_material_scope_begin(const FhDrawState *state);
void fh_renderer_material_scope_end(unsigned *entered);
void fh_renderer_material_scope_invalidate(void);
const FhMaterial *fh_material_scope_get(const FhDrawState *state,const FhVertex *vertices,unsigned count,int probe,FhMaterialOracle oracle);
int fh_material_scope_fur(const FhDrawState *state);
#endif
