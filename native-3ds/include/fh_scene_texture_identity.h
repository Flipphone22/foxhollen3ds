#ifndef FH_SCENE_TEXTURE_IDENTITY_H
#define FH_SCENE_TEXTURE_IDENTITY_H
#include <stdint.h>
#include <stddef.h>
/* Same publication contract as the renderer's texture cache. Callers must
 * join the CPU reader before querying; all visibility writes and allocation
 * releases revoke identities. This caches checksums, never mutable GPU data. */
typedef struct {const void *data;unsigned bytes,crc,valid;} FhSceneTextureIdentity;
static inline void fh_scene_texture_identity_invalidate(FhSceneTextureIdentity *entries,unsigned count,const void *address,size_t bytes){
    uintptr_t changed=(uintptr_t)address;
    for(unsigned i=0;i<count;i++){
        FhSceneTextureIdentity *e=entries+i;uintptr_t base=(uintptr_t)e->data;
        if(!address || (bytes && e->bytes && (changed>=base?changed-base<e->bytes:base-changed<bytes)))e->valid=0;
    }
}
#endif
