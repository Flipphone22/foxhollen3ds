#include "fh_pica_model.h"
#include <string.h>
unsigned fh_pica_model_compile(const FhSceneAsset *a,FhPicaModelDraw *out,unsigned capacity){
    if(!a || a->block || !a->direct || !a->groups || !a->draws ||
       !a->bones || a->bones>256 || (out && capacity<a->groups))return 0;
    /* Validate the whole plan before publication. This runs at load, never in
     * submission. Material boundaries and original ordering are preserved. */
    unsigned ordinal=0;
    for(unsigned i=0;i<a->groups;i++){
        FhSceneGroup g;FhSceneSection first;
        if(!fh_native_scene_group(a,i,&g) || g.first!=ordinal || !g.draws ||
           g.draws>a->draws-ordinal || !g.indices || !g.bones || g.bones>FH_MODEL_BONES ||
           !fh_native_scene_section(a,g.first_section,&first))return 0;
        unsigned indices=0;
        for(unsigned j=0;j<g.draws;j++){
            FhSceneSection s;unsigned group;
            if(!fh_native_scene_section(a,g.first_section+j,&s) || s.first!=indices ||
               s.count>g.indices-indices || s.shader!=first.shader || s.shader_flags!=first.shader_flags ||
               !fh_native_scene_draw(a,ordinal+j,s.list,s.shader,&group) || group!=i)return 0;
            indices+=s.count;
        }
        if(indices!=g.indices)return 0;
        for(unsigned b=0;b<g.bones;b++)if(g.bone_ids[b]>=a->bones)return 0;
        ordinal+=g.draws;
    }
    if(ordinal!=a->draws)return 0;
    const FhPicaModelDraw *last_fur=NULL;
    if(out)for(unsigned i=0;i<a->groups;i++){
        FhSceneGroup g;FhSceneSection s;
        fh_native_scene_group(a,i,&g);fh_native_scene_section(a,g.first_section,&s);
        FhPicaModelDraw d={0};
        d.vertex_offset=(unsigned)((const unsigned char*)g.vertex-a->gpu);
        d.index_offset=(unsigned)((const unsigned char*)g.index-a->gpu);
        d.indices=g.indices;d.vertices=g.vertices;d.format=g.format;d.cull=g.cull;
        d.material=s.shader;d.flags=s.shader_flags;d.first_ordinal=g.first;d.ordinals=g.draws;
        d.two_uv=g.two_uv;d.bones=g.bones;
        memcpy(d.bone,g.bone_ids,g.bones*sizeof(*d.bone));
        for(unsigned b=0;b<d.bones;b++)
            if(!i || b>=out[i-1].bones || d.bone[b]!=out[i-1].bone[b])d.palette_changed|=1u<<b;
        out[i]=d;
        if(d.flags&0x200){
            for(unsigned b=0;b<d.bones;b++)
                if(!last_fur || b>=last_fur->bones || d.bone[b]!=last_fur->bone[b])out[i].fur_palette_changed|=1u<<b;
            last_fur=out+i;
        }
    }
    return a->groups;
}
int fh_pica_model_pose(float out[3][4],const float joint[12],const float tail[3]){
    if(!out || !joint || !tail)return 0;
    /* Translate away the fixed bind tail. Rotation/scaling stay live. The
     * uniform rows are the only transformed data; every vertex stays on GPU. */
    for(unsigned r=0;r<3;r++){
        const float *j=joint+r*4;
        out[r][0]=j[0];out[r][1]=j[1];out[r][2]=j[2];
        out[r][3]=j[3]-j[0]*tail[0]-j[1]*tail[1]-j[2]*tail[2];
    }
    return 1;
}
