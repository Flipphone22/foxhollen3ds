#include "fh_native_scene.h"
#include <stdlib.h>
#include <string.h>

/* Admission-time permutations only. Neither geometry count nor palette length
 * grows. All dependent native plans are compiled after this publication. */
static unsigned differences(const uint16_t *a,unsigned an,const uint16_t *b,unsigned bn){
    unsigned count=0;
    for(unsigned i=0;i<an;i++)count+=i>=bn || a[i]!=b[i];
    return count;
}
static void costs(const FhSceneAsset *asset,const unsigned char *map,unsigned *ordinary,unsigned *fur){
    uint16_t previous[22],previous_fur[22];unsigned pn=0,fn=0;
    *ordinary=*fur=0;
    for(unsigned i=0;i<asset->groups;i++){
        FhSceneGroup g;FhSceneSection s;uint16_t palette[22];
        fh_native_scene_group(asset,i,&g);fh_native_scene_section(asset,g.first_section,&s);
        for(unsigned j=0;j<g.bones;j++)palette[j]=g.bone_ids[map?map[i*22+j]:j];
        *ordinary+=differences(palette,g.bones,previous,pn);
        memcpy(previous,palette,g.bones*2);pn=g.bones;
        if(s.shader_flags&512){
            *fur+=differences(palette,g.bones,previous_fur,fn);
            memcpy(previous_fur,palette,g.bones*2);fn=g.bones;
        }
    }
}
static void candidate(const FhSceneAsset *asset,unsigned char *map,unsigned prefer_fur){
    uint16_t previous[22],previous_fur[22];unsigned pn=0,fn=0;
    for(unsigned i=0;i<asset->groups;i++){
        FhSceneGroup g;FhSceneSection s;uint16_t palette[22];
        fh_native_scene_group(asset,i,&g);fh_native_scene_section(asset,g.first_section,&s);
        unsigned is_fur=(s.shader_flags&512)!=0,used=0;
        unsigned char *slots=map+i*22;memset(slots,255,22);
        for(unsigned pass=0;pass<2;pass++){
            unsigned use_fur=(prefer_fur && is_fur)?pass==0:pass==1;
            const uint16_t *prior=use_fur?previous_fur:previous;
            unsigned count=use_fur?(is_fur?fn:0):pn;
            if(count>g.bones)count=g.bones;
            for(unsigned slot=0;slot<count;slot++)if(slots[slot]==255)
                for(unsigned b=0;b<g.bones;b++)if(!(used&(1u<<b)) && g.bone_ids[b]==prior[slot]){
                    slots[slot]=b;used|=1u<<b;break;
                }
        }
        unsigned next=0;
        for(unsigned slot=0;slot<g.bones;slot++){
            if(slots[slot]==255){while(used&(1u<<next))next++;slots[slot]=next;used|=1u<<next;}
            palette[slot]=g.bone_ids[slots[slot]];
        }
        memcpy(previous,palette,g.bones*2);pn=g.bones;
        if(is_fur){memcpy(previous_fur,palette,g.bones*2);fn=g.bones;}
    }
}


typedef struct {
    const uint16_t *bones;
    unsigned short previous,next;
    unsigned char count,fur;
} PaletteNeighbors;
static unsigned edge_slot(const PaletteNeighbors *rows,const unsigned char *map,
                          unsigned left,unsigned right,unsigned slot){
    if(right==256 || slot>=rows[right].count)return 0;
    return left==256 || slot>=rows[left].count ||
        rows[left].bones[map[left*22+slot]]!=rows[right].bones[map[right*22+slot]];
}
static unsigned pair_cost(const PaletteNeighbors *rows,const unsigned char *map,
                          unsigned left,unsigned at,unsigned right,unsigned a,unsigned b){
    return edge_slot(rows,map,left,at,a)+edge_slot(rows,map,left,at,b)+
        edge_slot(rows,map,at,right,a)+edge_slot(rows,map,at,right,b);
}
static unsigned refine(const FhSceneAsset *asset,unsigned char *map){
    PaletteNeighbors rows[256];unsigned last=256,swaps=0;
    for(unsigned i=0;i<asset->groups;i++){
        FhSceneGroup g;FhSceneSection s;
        fh_native_scene_group(asset,i,&g);fh_native_scene_section(asset,g.first_section,&s);
        rows[i]=(PaletteNeighbors){g.bone_ids,last,256,g.bones,(s.shader_flags&512)!=0};
        if(rows[i].fur)last=i;
    }
    last=256;
    for(unsigned k=asset->groups;k;k--){unsigned i=k-1;rows[i].next=last;if(rows[i].fur)last=i;}
    for(unsigned pass=0;pass<4;pass++){
        unsigned changes=0;
        for(unsigned k=0;k<asset->groups;k++){
            unsigned i=(pass&1)?asset->groups-1-k:k;
            unsigned prev=i?i-1:256,next=i+1<asset->groups?i+1:256;
            for(unsigned a=0;a<rows[i].count;a++)for(unsigned b=a+1;b<rows[i].count;b++){
                unsigned before=pair_cost(rows,map,prev,i,next,a,b);
                unsigned fur_before=rows[i].fur?pair_cost(rows,map,rows[i].previous,i,rows[i].next,a,b):0;
                unsigned char temp=map[i*22+a];map[i*22+a]=map[i*22+b];map[i*22+b]=temp;
                unsigned after=pair_cost(rows,map,prev,i,next,a,b);
                unsigned fur_after=rows[i].fur?pair_cost(rows,map,rows[i].previous,i,rows[i].next,a,b):0;
                if(after<=before && fur_after<=fur_before && (after<before || fur_after<fur_before)){
                    changes++;swaps++;
                }else{temp=map[i*22+a];map[i*22+a]=map[i*22+b];map[i*22+b]=temp;}
            }
        }
        if(!changes)break;
    }
    return swaps;
}


static unsigned fur_cycle_cost(const FhSceneAsset *asset,const unsigned char *map){
    uint16_t first[22],previous[22];unsigned first_n=0,pn=0,cost=0;
    for(unsigned i=0;i<asset->groups;i++){
        FhSceneGroup g;FhSceneSection s;uint16_t palette[22];
        fh_native_scene_group(asset,i,&g);fh_native_scene_section(asset,g.first_section,&s);
        if(!(s.shader_flags&512))continue;
        for(unsigned b=0;b<g.bones;b++)palette[b]=g.bone_ids[map?map[i*22+b]:b];
        if(first_n)cost+=differences(palette,g.bones,previous,pn);
        else{memcpy(first,palette,g.bones*2);first_n=g.bones;}
        memcpy(previous,palette,g.bones*2);pn=g.bones;
    }
    return cost+differences(first,first_n,previous,pn);
}

unsigned fh_native_scene_schedule_palettes(FhSceneAsset *asset){
    /* Contract: asset has passed fh_native_scene_open and is exclusively owned,
     * writable and not yet GPU-published. No alternate render path is created. */
    if(!asset || asset->block || !asset->direct || !asset->compact || asset->groups<2 ||
       asset->groups>256 || (asset->meta[7]!='A' && asset->meta[7]!='B'))return 0;
    unsigned bytes=asset->groups*22;
    unsigned char *scratch=malloc(bytes*2);if(!scratch)return 0;
    unsigned char *trial=scratch,*best=scratch+bytes;
    unsigned before,fur_before,best_cost,best_fur,selected=0;
    costs(asset,NULL,&before,&fur_before);best_cost=before;best_fur=fur_before;
    for(unsigned preference=0;preference<2;preference++){
        unsigned cost,fur;candidate(asset,trial,preference);costs(asset,trial,&cost,&fur);
        if(cost<=before && fur<=fur_before && (cost<best_cost || (cost==best_cost && fur<best_fur))){
            memcpy(best,trial,bytes);best_cost=cost;best_fur=fur;selected=1;
        }
    }
    if(!selected)for(unsigned i=0;i<asset->groups;i++)for(unsigned b=0;b<22;b++)best[i*22+b]=b;
    if(refine(asset,best)){
        unsigned cost,fur;costs(asset,best,&cost,&fur);
        /* Publication remains transactional if an unexpected schedule fails
         * the independent full traversal cost check. */
        if(cost>best_cost || fur>best_fur){free(scratch);return 0;}
        selected=1;
    }
    /* First-shell linear cost is insufficient: later GPU shells wrap from
     * the last fur draw to the first. Reject before touching asset bytes. */
    if(selected && fur_cycle_cost(asset,best)>fur_cycle_cost(asset,NULL)){
        free(scratch);return 0;
    }
    unsigned changed=0;
    if(selected)for(unsigned i=0;i<asset->groups;i++){
        FhSceneGroup g;fh_native_scene_group(asset,i,&g);
        unsigned char inverse[22];uint16_t palette[22];unsigned different=0;
        for(unsigned b=0;b<g.bones;b++){
            unsigned old=best[i*22+b];inverse[old]=b;palette[b]=g.bone_ids[old];different|=old!=b;
        }
        if(!different)continue;
        FhModelCompactVertex *v=(FhModelCompactVertex*)(uintptr_t)g.vertex;
        for(unsigned j=0;j<g.vertices;j++){
            v[j].skin[0]=inverse[v[j].skin[0]/3]*3;
            v[j].skin[1]=inverse[v[j].skin[1]/3]*3;
        }
        memcpy((void*)(uintptr_t)g.bone_ids,palette,g.bones*2);changed++;
    }
    free(scratch);return changed;
}
