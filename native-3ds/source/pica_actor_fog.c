#include "fh_pica_actor_fog.h"
#include <math.h>
#include <string.h>

int fh_pica_actor_fog_gpu_vertices(FhPicaActorFogGpuVertex *out,unsigned capacity,
    const int16_t (*positions)[3],unsigned position_count,const uint16_t *references,
    const FhPicaActorFogBinding *bindings,unsigned count,float scale,unsigned palette_bones){
    if(!positions || !position_count || !references || !bindings || !count ||
       !isfinite(scale) || scale<=0 || !palette_bones || palette_bones>22 ||
       (out && capacity<count))return 0;
    /* Validate all lanes before writing; caller supplies disjoint backing. */
    for(unsigned i=0;i<count;i++){
        const FhPicaActorFogBinding *b=bindings+i;
        if(references[i]>=position_count || b->first>=palette_bones*3 || b->second>=palette_bones*3 ||
           b->first%3 || b->second%3 || b->weight>4 || b->reserved)return 0;
        for(unsigned c=0;c<3;c++)
            if(!isfinite(positions[references[i]][c]*scale+b->offset[c]))return 0;
    }
    if(out)for(unsigned i=0;i<count;i++){
        for(unsigned c=0;c<3;c++)
            out[i].position[c]=positions[references[i]][c]*scale+bindings[i].offset[c];
        out[i].skin[0]=bindings[i].first;out[i].skin[1]=bindings[i].second;
        out[i].skin[2]=bindings[i].weight;out[i].skin[3]=0;
    }
    return 1;
}

int fh_pica_actor_fog_matrix_valid(const FhPicaActorFogMatrix*k,unsigned source_poses,unsigned allow_world){
    if(!k || k->reserved || source_poses>256 || allow_world>1)return 0;
    if(k->kind==FH_ACTOR_FOG_POSITION || k->kind==FH_ACTOR_FOG_NORMAL)return k->joint<source_poses;
    return allow_world && !k->joint &&
        (k->kind==FH_ACTOR_FOG_WORLD_POSITION || k->kind==FH_ACTOR_FOG_WORLD_NORMAL);
}
int fh_pica_actor_fog_matrix_world(FhPicaActorFogMatrixBank*bank){
    if(!bank)return 0;
    bank->slot[9]=(FhPicaActorFogMatrix){0,FH_ACTOR_FOG_WORLD_POSITION,0};
    bank->slot[19]=(FhPicaActorFogMatrix){0,FH_ACTOR_FOG_WORLD_NORMAL,0};
    return 1;
}
int fh_pica_actor_fog_matrix_load(FhPicaActorFogMatrixBank *bank,
    const uint8_t *joints,unsigned count,unsigned texture_loads,unsigned skip){
    if(!bank || (count&&!joints) || count>12 || texture_loads>1 || skip>1)return 0;
    for(unsigned i=0;i<count;i++){
        unsigned slot=i<10?i:0;
        bank->slot[slot]=(FhPicaActorFogMatrix){joints[i],FH_ACTOR_FOG_POSITION,0};
        if(texture_loads&&!skip){
            unsigned texture_slot=i<10?i+10:0;
            bank->slot[texture_slot]=(FhPicaActorFogMatrix){joints[i],FH_ACTOR_FOG_NORMAL,0};
        }
    }
    return 1;
}
int fh_pica_actor_fog_matrix_select(FhPicaActorFogMatrix *out,
    const FhPicaActorFogMatrixBank *bank,const FhPicaActorFogRouting *route,
    unsigned coordinate,const uint8_t *packet,unsigned packet_bytes){
    if(!out || !bank || !route || coordinate>=8 || route->packet_prefix_bytes>9 ||
       packet_bytes<route->packet_prefix_bytes || (route->packet_prefix_bytes&&!packet))return 0;
    int lane=route->matrix_lane[coordinate];unsigned matrix=0;
    if(lane!=-1){
        if(lane<1 || (unsigned)lane>=route->packet_prefix_bytes)return 0;
        matrix=packet[lane];
    }
    /* Authored palette rows are aligned triples. Reject unsupported/unknown
     * state explicitly; never substitute visible skinning or an identity. */
    if(matrix>=60 || matrix%3)return 0;
    FhPicaActorFogMatrix selected=bank->slot[matrix/3];
    if(!fh_pica_actor_fog_matrix_valid(&selected,256,1))return 0;
    *out=selected;return 1;
}

int fh_pica_actor_fog_coordinate(unsigned *out,unsigned environment,
    unsigned bump_coordinates,unsigned projected_coordinates,unsigned layers,
    unsigned textured_layers,unsigned decal){
    if(!out || environment>1 ||
       (bump_coordinates!=0 && bump_coordinates!=2 && bump_coordinates!=3) ||
       (bump_coordinates && !environment) || projected_coordinates>8 ||
       layers>2 || textured_layers>=(1u<<layers) || decal>1 ||
       (decal && layers<2))return 0;
    unsigned coordinate=environment+bump_coordinates+projected_coordinates;
    for(unsigned layer=0;layer<layers;layer++)
        if((textured_layers&(1u<<layer)) && !(decal && layer==1))coordinate++;
    coordinate+=decal; /* Warped-noise stage replaces layer one. */
    if(coordinate>=8)return 0;
    *out=coordinate;
    return 1;
}

int fh_pica_actor_fog_routing(FhPicaActorFogRouting *out,unsigned joints,
    unsigned has_environment,unsigned has_auxiliary,unsigned texture_matrices,
    unsigned projected_lights,unsigned pass_mask,int first_projection_mode){
    if(!out || joints>255 || has_environment>1 || has_auxiliary>1 ||
       texture_matrices>255 || projected_lights>255 || pass_mask>255)return 0;
    FhPicaActorFogRouting route;
    memset(&route,0xff,sizeof(route));
    route.packet_prefix_bytes=0;
    route.texture_loads=1;
    if(joints>1){
        unsigned front=has_environment?(has_auxiliary?3u:1u):0u;
        if(texture_matrices>8-front)return 0;
        unsigned mask=(1u<<front)-1u,back=7;
        for(unsigned i=0;i<texture_matrices;i++){
            unsigned forward=(pass_mask==4 && i==0)?
                (projected_lights!=0 && first_projection_mode==0):
                (i<projected_lights && pass_mask==0);
            unsigned index=forward?front++:back--;
            mask|=1u<<index;
        }
        route.texture_loads=front!=0;
        route.packet_prefix_bytes=1; /* PNMTXIDX precedes TEXnMTXIDX. */
        for(unsigned coordinate=0;coordinate<8;coordinate++)
            if(mask&(1u<<coordinate))
                route.matrix_lane[coordinate]=(int8_t)route.packet_prefix_bytes++;
    }
    memcpy(out,&route,sizeof(route));
    return 1;
}

int fh_pica_actor_fog_vertices(FhPicaActorFogVertex *out,unsigned capacity,
    const int16_t (*positions)[3],unsigned position_count,
    const uint16_t *references,unsigned count){
    if(!positions || !references || !count || !position_count ||
       (out && capacity<count))return 0;
    /* Validate the complete plan before publishing any data. Caller owns
     * disjoint source/destination backing extents; no source pointers retained. */
    for(unsigned i=0;i<count;i++)if(references[i]>=position_count)return 0;
    if(out)for(unsigned i=0;i<count;i++){
        memcpy(out[i].position,positions[references[i]],sizeof(out[i].position));
        out[i].padding=0;
    }
    return 1;
}

int fh_pica_actor_fog_rows(float out[2][4],const float planes[2][4],
    const float pnmtx0[3][4]){
    if(!out || !planes || !pnmtx0)return 0;
    float value[2][4];
    for(unsigned r=0;r<2;r++)for(unsigned c=0;c<4;c++){
        value[r][c]=planes[r][0]*pnmtx0[0][c]+planes[r][1]*pnmtx0[1][c]+
                    planes[r][2]*pnmtx0[2][c]+(c==3?planes[r][3]:0.f);
        if(!isfinite(value[r][c]))return 0;
    }
    memcpy(out,value,sizeof(value));
    return 1;
}
