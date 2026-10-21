#ifndef FH_PICA_FUR_H
#define FH_PICA_FUR_H
#include <math.h>
#include "fh_pica_model.h"
/* Borrowed CPU inputs live only for this synchronous submission. Shader rows
 * are copied into the command owner; no queued GPU pointer references them. */
typedef struct {
    const float *joint_shape; /* original jointBlendData: pivot XYZ, radius */
    float pivot[3],divisor;
    unsigned stream_skin,step,layer,noise_page,alpha;
    unsigned layers,source_layers,noise_pages[16];
} FhPicaFur;
/* Shell ordering is original inner-to-outer. The reduced mode samples the
 * endpoints; source mode restores every distance-admitted source layer. */
static inline unsigned fh_pica_fur_layer(unsigned shell,unsigned layers,unsigned source_layers){
    return layers==source_layers?shell:layers==1?0:shell*(source_layers-1);
}
static inline float fh_pica_fur_scale(float radius,unsigned step,unsigned layer){
    float value=1.f,increment=1.f+step*(.08f/radius);
    for(unsigned i=0;i<=layer;i++)value*=increment;
    return value;
}
static inline void fh_pica_fur_pose(float rows[3][4],const float joint[12],
    const float tail[3],const float world[3][4],const FhPicaFur *fur,unsigned bone,unsigned joints){
    if(fur->stream_skin){
        /* Source CPU-skinned family expands its completed model-space stream
         * about vertexAnimPivot. Compose the equivalent world transform with
         * each live native bone row; vertices still skin on the GPU. */
        float s=1.f+(1.5f*((fur->layer+1)*.08f))/fur->divisor;
        for(unsigned r=0;r<3;r++){
            float center=world[r][3];for(unsigned c=0;c<3;c++)center+=world[r][c]*fur->pivot[c];
            rows[r][3]=joint[r*4+3]*s+center*(1.f-s);
            for(unsigned c=0;c<3;c++){rows[r][c]=joint[r*4+c]*s;rows[r][3]-=rows[r][c]*tail[c];}
        }
    }else{
        const float *shape=bone<joints?fur->joint_shape+bone*4:NULL;
        float s=shape?fh_pica_fur_scale(shape[3],fur->step,fur->layer):1.f;
        for(unsigned r=0;r<3;r++){
            rows[r][3]=joint[r*4+3];
            for(unsigned c=0;c<3;c++){
                rows[r][c]=joint[r*4+c]*s;
                rows[r][3]+=joint[r*4+c]*(shape?shape[c]*(1.f-s):0)-rows[r][c]*tail[c];
            }
        }
    }
}
void fh_pica_scene_fur(const FhSceneAsset*,const float*,unsigned,
    const float[3][4],const float[3][4],const float[4][4],const unsigned[4],const FhPicaFur*);
int fh_pica_material_fur_mask(const FhSceneAsset*,unsigned,const FhTexture*);
void fh_pica_fur_material_init(void);
int fh_pica_fur_material_bind(const FhPicaMaterialSet*,unsigned,unsigned,unsigned,const FhPicaFur*);
#endif
