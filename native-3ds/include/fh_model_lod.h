#ifndef FH_MODEL_LOD_H
#define FH_MODEL_LOD_H
#include <stddef.h>
#include <stdint.h>
/* Applies prepared render topology before relocation/publication. Collision,
 * animation, joints, attributes and instruction streams retain original data. */
int fh_model_lod_patch(void *model,size_t bytes,unsigned id,const void *patch,size_t patch_bytes);
void fh_model_lod_prepare(void *model,unsigned bytes,unsigned id);
void fh_model_lod_shutdown(void);
extern unsigned fh_model_lod_applied,fh_model_lod_rejected;
#endif
