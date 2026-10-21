#include "gx_internal.h"
#include "fh_native_model_draw.h"
#include "fh_light_lod.h"
#include "fh_scene_texture_identity.h"
#include <assert.h>
#include <stdio.h>
FhGx fh_gx;
static float clamp(float x){return x<0?0:x>1?1:x;}
#include "../native-3ds/source/gx_native_scene_state.h"
static FhNativeModelDraw fresh(void){
    FhNativeModelDraw d={0};d.source.normal_count=1;d.group.bones=1;
    return d;
}
int main(void){
    FhSceneTextureIdentity identities[3]={
        {(void*)0x1000,32,7,1},{(void*)0x2000,64,9,1},{(void*)0x3000,16,11,1}};
    fh_scene_texture_identity_invalidate(identities,3,(void*)0x1020,1);
    assert(identities[0].valid && identities[1].valid && identities[2].valid);
    fh_scene_texture_identity_invalidate(identities,3,(void*)0xfff,2);
    assert(!identities[0].valid && identities[1].valid && identities[2].valid);
    fh_scene_texture_identity_invalidate(identities,3,(void*)0x203f,1);
    assert(!identities[1].valid && identities[2].valid);
    fh_scene_texture_identity_invalidate(identities,3,NULL,0);
    assert(!identities[2].valid);
    /* Stale legacy texture matrices and projection cannot affect native UVs
     * or the current camera. This caught the first integration rejection. */
    fh_gx.descriptors[GX_VA_TEX0MTXIDX]=GX_DIRECT;
    fh_gx.texgens[0].matrix=GX_TEXMTX0;
    fh_gx.projection[0][0]=3;fh_gx.draw.position_matrix[0][0]=8;
    fh_gx.channel[0].material_source=GX_SRC_VTX;
    fh_gx.channel[2].material_source=GX_SRC_REG;
    fh_gx.material[0][3]=.75f;
    FhNativeModelDraw d=fresh();assert(scene_model_state(&d));
    assert(d.state.position_matrix[0][0]==3 && d.coordinates[0]==1);
    assert(d.color_from_vertex[0] && !d.color_from_vertex[3] && d.color[3]==.75f);
    assert(!d.lit && fh_gx.texgens[0].matrix==GX_TEXMTX0);
    /* Noncontiguous light slots compact without dropping or changing their
     * spot/distance coefficients; ambient and material remain separate. */
    FhChannel *ch=&fh_gx.channel[0];
    *ch=(FhChannel){1,GX_SRC_REG,GX_SRC_VTX,(1u<<2)|(1u<<7),GX_DF_CLAMP,GX_AF_SPOT};
    for(unsigned i=0;i<8;i++){
        fh_gx.lights[i].position[2]=i+1;
        fh_gx.lights[i].direction[2]=1;
        fh_gx.lights[i].color[0]=.1f*(i+1);
        fh_gx.lights[i].a[0]=.5f;fh_gx.lights[i].k[0]=2;
    }
    fh_gx.ambient[0][0]=.25f;
    FhGx before=fh_gx;d=fresh();assert(scene_model_state(&d));
    assert(d.lit && (d.lighting.mask&255)==3 && d.lighting.ambient[0]==.25f);
    assert(d.lighting.lights[0][0][2]==3 && d.lighting.lights[1][0][2]==8);
    assert(d.lighting.lights[0][3][0]==.5f && d.lighting.lights[1][4][0]==2);
    assert(!memcmp(&before,&fh_gx,sizeof(before)));
    ch->attenuation=GX_AF_NONE;d=fresh();assert(scene_model_state(&d));
    assert(d.lighting.lights[0][3][0]==1 && d.lighting.lights[0][4][0]==1);
    ch->mask=255;d=fresh();assert(scene_model_state(&d));
    assert((d.lighting.mask&FH_LIGHT_DIFFUSE_FILL) && (d.lighting.mask&255)==3);
    float fill=0;for(unsigned c=0;c<3;c++)fill+=fabsf(d.lighting.lights[1][c][3]);
    assert(fill>0); /* Extra lights contribute, rather than being skipped. */
    ch->attenuation=GX_AF_SPEC;d=fresh();assert(!scene_model_state(&d));
    ch->attenuation=GX_AF_SPOT;ch->ambient_source=GX_SRC_VTX;
    d=fresh();assert(!scene_model_state(&d));
    ch->ambient_source=GX_SRC_REG;ch->mask=0;
    d=fresh();assert(scene_model_state(&d) && !d.lit && d.color[0]==.25f);
    fh_gx.channel[2].enabled=1;fh_gx.channel[2].diffuse=GX_DF_CLAMP;
    d=fresh();assert(!scene_model_state(&d));
    puts("native scene state: camera, legacy independence, live channels, light compaction/fill and fallback passed");
}
