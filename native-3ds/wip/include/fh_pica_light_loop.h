#ifndef FH_PICA_LIGHT_LOOP_H
#define FH_PICA_LIGHT_LOOP_H

/* Preserve summation order, including sparse/custom source light lists.
 * Called once per source lighting snapshot, never per vertex or material. */
static inline unsigned fh_pica_light_pack(FhPicaActorLighting *state){
    unsigned count=0;
    for(unsigned i=0;i<8;i++)if(state->active&(1u<<i)){
        if(count!=i){
            memcpy(state->position[count],state->position[i],sizeof(state->position[i]));
            memcpy(state->color[count],state->color[i],sizeof(state->color[i]));
        }
        count++;
    }
    state->active=(1u<<count)-1;
    return count;
}

/* Publish after binding: another entry can install an integer constant.
 * Keep the SDK's existing dirty bit when the cached value is already right.
 * A new scene publication still writes the value after frame/split handoff. */
static inline void fh_pica_light_loop_publish(unsigned count,unsigned force){
    C3D_IVec value=IVec_Pack(count?count-1:0,0,1,0);
    if(force || memcmp(&C3D_IVUnif[GPU_VERTEX_SHADER][0],&value,sizeof(value)))
        C3D_IVUnifSet(GPU_VERTEX_SHADER,0x60,count?count-1:0,0,1,0);
}
#endif
