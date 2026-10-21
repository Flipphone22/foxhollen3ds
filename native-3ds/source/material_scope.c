#include "fh_material_scope.h"
#include "fh_platform.h"
#include <string.h>
unsigned fh_material_scope_mode,fh_material_scope_hits,fh_material_scope_checks;
unsigned fh_material_scope_class_hits,fh_material_scope_class_checks;
static struct {
    const FhDrawState *owner;
    FhMaterial plan;
    unsigned depth,known,available,fur_known,epoch,version;
    int fur;
} scope;
/* The producer may service callbacks while waiting for the render worker.
 * It only publishes invalidation; cached storage belongs to the renderer. */
static unsigned invalidation_epoch;
static int valid_owner(const FhDrawState *state,unsigned version){
    return state && scope.owner==state && scope.version==version &&
        scope.epoch==__atomic_load_n(&invalidation_epoch,__ATOMIC_ACQUIRE);
}
unsigned fh_renderer_material_scope_begin(const FhDrawState *state){
    if(!fh_material_scope_mode)return 0;
    ++scope.depth;++scope.version;scope.owner=scope.depth==1?state:NULL;
    scope.epoch=__atomic_load_n(&invalidation_epoch,__ATOMIC_ACQUIRE);
    scope.known=scope.available=scope.fur_known=0;return 1;
}
void fh_renderer_material_scope_end(unsigned *entered){
    if(!*entered)return;
    if(!scope.depth)fh_platform_panic("Material scope underflow");
    --scope.depth;++scope.version;scope.owner=NULL;scope.known=scope.available=scope.fur_known=0;
}
void fh_renderer_material_scope_invalidate(void){__atomic_add_fetch(&invalidation_epoch,1,__ATOMIC_RELEASE);}
int fh_material_scope_fur(const FhDrawState *state){
    unsigned version=scope.version;
    if(!valid_owner(state,version))return fh_fur_material(state);
    if(scope.fur_known){
        ++fh_material_scope_class_hits;
        if(fh_material_scope_mode==2){
            ++fh_material_scope_class_checks;
            int fresh=fh_fur_material(state);
            if(!valid_owner(state,version))return fresh;
            if(scope.fur!=fresh)fh_platform_panic("Scoped fur classification changed");
        }
        return scope.fur;
    }
    int result=fh_fur_material(state);
    if(valid_owner(state,version)){scope.fur=result;scope.fur_known=1;}
    return result;
}
const FhMaterial *fh_material_scope_get(const FhDrawState *state,const FhVertex *vertices,unsigned count,int probe,FhMaterialOracle oracle){
    unsigned version=scope.version;
    if(!valid_owner(state,version))return oracle(state,vertices,count,probe);
    unsigned bit=probe?2:1;
    if(scope.known&bit){
        const FhMaterial *result=scope.available&bit?&scope.plan:NULL;
        ++fh_material_scope_hits;
        if(fh_material_scope_mode==2){
            ++fh_material_scope_checks;
            const FhMaterial *fresh=oracle(state,vertices,count,probe);
            if(!valid_owner(state,version))return fresh;
            if(!!fresh!=!!result || (fresh && memcmp(fresh,result,sizeof(*fresh))))
                fh_platform_panic("Scoped material differs from fresh preparation");
        }
        return result;
    }
    const FhMaterial *result=oracle(state,vertices,count,probe);
    if(!valid_owner(state,version))return result;
    scope.known|=bit;
    if(!result)return NULL;
    scope.plan=*result;scope.available|=bit;
    /* A successful probe is the same complete plan as a normal lookup.
     * CPU fragment fallback is probe-null except the explicit fur family. */
    if(probe || !result->cpu_fragment || result->cpu_fragment==2)scope.known=scope.available=3;
    return &scope.plan;
}
