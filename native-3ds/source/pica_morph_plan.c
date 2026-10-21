#include "fh_pica_morph.h"
#include <string.h>
#include <limits.h>
int fh_pica_morph_compile(const FhSceneAsset *a,const uint8_t *mask,unsigned sources,
    FhPicaMorphSize *size,FhPicaMorphGroup *out,unsigned capacity,FhPicaMorphWrite *writes,unsigned write_capacity){
    if(!a || !mask || !size || a->block || !a->compact || !a->direct ||
       !sources || sources>8191 || a->groups>256 || !a->groups || (!!out!=!!writes))return 0;
    FhPicaMorphSize need={0};
    /* Complete admission before publishing any part of the plan. */
    for(unsigned i=0;i<a->groups;i++){
        FhSceneGroup g;if(!fh_native_scene_group(a,i,&g) || !g.refs || !g.vertices || g.vertices>65535)return 0;
        unsigned changed=0;
        for(unsigned v=0;v<g.vertices;v++){
            unsigned p=g.refs[v].position;if(p>=sources)return 0;
            changed+=(mask[p>>3]>>(p&7))&1;
        }
        if(changed){
            if(need.bytes>UINT_MAX-127-g.vertices*8 || need.writes>UINT_MAX-changed)return 0;
            need.bytes=(need.bytes+127)&~127u;
            need.bytes+=g.vertices*8;need.groups++;need.writes+=changed;
        }
    }
    if(need.bytes>UINT_MAX-127)return 0;
    need.bytes=(need.bytes+127)&~127u;
    if(out && (capacity<need.groups || write_capacity<need.writes))return 0;
    if(out){
        unsigned at=0,w=0,offset=0;
        for(unsigned i=0;i<a->groups;i++){
            FhSceneGroup g;fh_native_scene_group(a,i,&g);unsigned first=w;
            for(unsigned v=0;v<g.vertices;v++){
                unsigned p=g.refs[v].position;
                if((mask[p>>3]>>(p&7))&1)writes[w++]=(FhPicaMorphWrite){p,v};
            }
            if(w!=first){offset=(offset+127)&~127u;out[at++]=(FhPicaMorphGroup){i,offset,g.vertices,first,w-first};offset+=g.vertices*8;}
        }
    }
    *size=need;return 1;
}
void fh_pica_morph_initialize(void *storage,const FhSceneAsset *asset,const FhPicaMorphGroup *groups,unsigned count){
    for(unsigned i=0;i<count;i++){
        FhSceneGroup g;fh_native_scene_group(asset,groups[i].group,&g);
        const FhModelCompactVertex *v=(const FhModelCompactVertex*)g.vertex;
        int16_t (*positions)[4]=(int16_t(*)[4])((unsigned char*)storage+groups[i].offset);
        for(unsigned j=0;j<g.vertices;j++)memcpy(positions[j],v[j].position,8);
    }
}
void fh_pica_morph_pack_attributes(void *storage,const FhSceneAsset *asset,const FhPicaMorphGroup *groups,unsigned count){
    _Static_assert(sizeof(FhModelCompactVertex)==40,"compact morph source ABI");
    for(unsigned i=0;i<count;i++){
        FhSceneGroup g;fh_native_scene_group(asset,groups[i].group,&g);
        const unsigned char *v=(const unsigned char*)g.vertex;
        unsigned char *out=(unsigned char*)storage+groups[i].offset*4;
        for(unsigned j=0;j<g.vertices;j++)memcpy(out+j*32,v+j*40+8,32);
    }
}
void fh_pica_morph_apply(void *storage,const unsigned char *source,const FhPicaMorphGroup *groups,unsigned count,const FhPicaMorphWrite *writes){
    for(unsigned i=0;i<count;i++){
        const FhPicaMorphGroup *g=groups+i;
        int16_t (*positions)[4]=(int16_t(*)[4])((unsigned char*)storage+g->offset);
        for(unsigned j=0;j<g->count;j++){
            const FhPicaMorphWrite *w=writes+g->first+j;
            const unsigned char *p=source+w->source*6;
            for(unsigned c=0;c<3;c++)positions[w->destination][c]=(int16_t)(((unsigned)p[c*2]<<8)|p[c*2+1]);
        }
    }
}
