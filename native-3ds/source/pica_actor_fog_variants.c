#include "fh_pica_actor_fog_variants.h"
#include <string.h>

static int cell_selection(FhPicaActorFogSelection*out,unsigned cell){
    if(cell>=FH_ACTOR_FOG_VARIANT_CELLS)return 0;
    unsigned lights=cell/9,projected=cell%9;
    FhPicaActorFogSelection s={.selected_lights=lights};
    if(projected<=lights)s.projection_images=(1u<<projected)-1;
    else if(projected==1)s.render_flags=16;
    else if(projected==2)s.shadow_near=1;
    else return 0;
    *out=s;return 1;
}
static unsigned hash_byte(unsigned hash,unsigned byte){return (hash^(byte&255u))*16777619u;}
static int signature(unsigned*out,unsigned joints,unsigned matrices,
    const FhPicaActorFogMaterial*m,unsigned count,const uint32_t used[2],unsigned cell){
    FhPicaActorFogSelection selection;if(!cell_selection(&selection,cell))return 0;
    unsigned hash=2166136261u;
    for(unsigned i=0;i<count;i++)if(used[i/32]&(1u<<(i%32))){
        FhPicaActorFogAlternative a;
        if(!fh_pica_actor_fog_material_alternative(&a,joints,matrices,m+i,&selection))return 0;
        for(unsigned k=0;k<8;k++)hash=hash_byte(hash,(uint8_t)a.route.matrix_lane[k]);
        hash=hash_byte(hash,a.route.packet_prefix_bytes);
        hash=hash_byte(hash,a.route.texture_loads);hash=hash_byte(hash,a.coordinate);
    }
    *out=hash;return 1;
}
static int equivalent(unsigned joints,unsigned matrices,const FhPicaActorFogMaterial*m,
    unsigned count,const uint32_t used[2],unsigned first,unsigned second){
    FhPicaActorFogSelection x,y;
    if(!cell_selection(&x,first)||!cell_selection(&y,second))return 0;
    for(unsigned i=0;i<count;i++)if(used[i/32]&(1u<<(i%32))){
        FhPicaActorFogAlternative a,b;
        if(!fh_pica_actor_fog_material_alternative(&a,joints,matrices,m+i,&x)||
           !fh_pica_actor_fog_material_alternative(&b,joints,matrices,m+i,&y)||
           a.coordinate!=b.coordinate||a.route.texture_loads!=b.route.texture_loads||
           a.route.packet_prefix_bytes!=b.route.packet_prefix_bytes||
           memcmp(a.route.matrix_lane,b.route.matrix_lane,8))return 0;
    }
    return 1;
}
int fh_pica_actor_fog_variants(FhPicaActorFogVariants*out,unsigned joints,
    unsigned matrices,const FhPicaActorFogMaterial*m,unsigned count,const uint32_t used[2]){
    if(!out||!m||!used||!count||count>64||joints>255||matrices>8)return 0;
    for(unsigned i=count;i<64;i++)if(used[i/32]&(1u<<(i%32)))return 0;
    FhPicaActorFogVariants plan={0};unsigned hashes[FH_ACTOR_FOG_VARIANT_CELLS];
    if(used[0]||used[1])for(unsigned cell=0;cell<FH_ACTOR_FOG_VARIANT_CELLS;cell++){
        unsigned hash;if(!signature(&hash,joints,matrices,m,count,used,cell))continue;
        unsigned slot=0;
        while(slot<plan.count){
            if(hashes[slot]==hash && equivalent(joints,matrices,m,count,used,
                    cell,plan.representative[slot]))break;
            slot++;
        }
        if(slot==plan.count){hashes[slot]=hash;plan.representative[slot]=cell;plan.count++;}
        plan.slots[cell]=(uint8_t)(slot+1);
    }
    *out=plan;return 1;
}
unsigned fh_pica_actor_fog_variant(const FhPicaActorFogVariants*p,const FhPicaActorFogSelection*s){
    if(!p||!s||p->reserved||p->count>FH_ACTOR_FOG_VARIANT_CELLS||s->selected_lights>8||
       s->projection_images>=(1u<<s->selected_lights)||s->shadow_near>1||s->cast_shadow>1||s->render_flags>255)return 0;
    unsigned projected=0;
    if(s->shadow_near||((s->render_flags&4)&&s->cast_shadow))projected=2;
    else if(s->render_flags&16)projected=1;
    else if(!(s->render_flags&4)){
        unsigned bits=s->projection_images;while(bits){projected+=bits&1;bits>>=1;}
    }
    unsigned slot=p->slots[s->selected_lights*9+projected];
    return slot<=p->count?slot:0;
}
int fh_pica_actor_fog_variant_selection(FhPicaActorFogSelection*out,
    const FhPicaActorFogVariants*p,unsigned variant){
    if(!out||!p||p->reserved||!variant||variant>p->count||p->count>FH_ACTOR_FOG_VARIANT_CELLS)return 0;
    unsigned cell=p->representative[variant-1];FhPicaActorFogSelection selection;
    if(!cell_selection(&selection,cell)||p->slots[cell]!=variant)return 0;
    *out=selection;return 1;
}
