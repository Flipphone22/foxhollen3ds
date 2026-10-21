#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
enum {GPU_GEQUAL=3,GPU_WRITE_COLOR=15};
static float scale,offset;
static int compare,mask;
static void C3D_DepthMap(bool enabled,float s,float o){assert(enabled);scale=s;offset=o;}
static void C3D_DepthTest(bool enabled,int f,int m){assert(enabled);compare=f;mask=m;}
#include "fh_pica_shadow_depth.h"
/* The harness prepends the actual Camera_ApplyDecalViewport function and SDK
 * viewport recorder from the pinned source; test both original field modes. */
static uint32_t quantize(double d){return (uint32_t)(fmax(0,fmin(1,d))*16777215.0);}
static int passes(double receiver,uint32_t stored){
    uint32_t incoming=quantize(scale*-receiver+offset);
    return compare==GPU_GEQUAL?incoming>=stored:incoming==stored;
}
int main(void){
    unsigned cases=0,old_misses=0;
    for(unsigned field=0;field<2;field++){
        source_mode.field_rendering=field;Camera_ApplyDecalViewport();
        fh_pica_shadow_depth_bind();
        assert(scale==source_near-source_far && offset==1.f-source_far);
        assert(compare==GPU_GEQUAL && mask==GPU_WRITE_COLOR);
        for(unsigned i=1;i<=65536;i++){
            /* Reversed normalized depth; lifting scales with depth, unlike an
             * absolute epsilon whose world-space tolerance grows with distance. */
            double ground=(i+.125)/70000.;
            double source=(source_far-source_near)*ground+1.-source_far;
            assert(quantize(scale*-ground+offset)==quantize(source));cases++;
            /* Collision/render surfaces within the source's tolerance remain
             * visible through a moving subpixel/depth phase. Beyond that
             * tolerance foreground geometry MUST continue to occlude. */
            double receiver=ground*.98;
            assert(passes(receiver,quantize(ground)));cases++;
            assert(!passes(ground,quantize(fmin(1.,source+.002))));cases++;
            double old=receiver+1./4194304.;
            old_misses+=quantize(old)<quantize(ground);
        }
    }
    assert(old_misses>120000);
    printf("PASS %u original viewport/depth/occlusion cases; old tiny-bias rejected %u coplanar-tolerance samples\n",cases,old_misses);
}
