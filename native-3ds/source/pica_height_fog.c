#include "fh_pica_height_fog.h"
#include <math.h>
void fh_pica_height_fog_atlas_prepare(uint8_t *destination){
    /* Source alpha=min((row+intensity)*255,64*255)*column >> 12.
     * Eight phases cover intensity modulo 8. Moving a view by one tile row
     * handles the quotient without changing pixels or clamping coordinates
     * in the vertex shader. Full source quantization is retained. */
    for(unsigned phase=0;phase<8;phase++)
    for(unsigned ty=0;ty<16;ty++)for(unsigned tx=0;tx<8;tx++)
        for(unsigned lane=0;lane<64;lane++){
            unsigned x=tx*8+(lane&1)+((lane>>1)&2)+((lane>>2)&4);
            unsigned row=127-(ty*8+((lane>>1)&1)+((lane>>2)&2)+((lane>>3)&4))+phase;
            unsigned scaled=(row<64?row:64)*255;
            destination[phase*8192+(ty*8+tx)*64+lane]=(uint8_t)((scaled*x)>>12);
        }
}
unsigned fh_pica_height_fog_atlas_offset(unsigned intensity){
    if(intensity>=FH_HEIGHT_FOG_STATES)return ~0u;
    return (intensity&7u)*8192+(64-(intensity&~7u))*64;
}
int fh_pica_height_fog_planes(float planes[2][4],const float inverse_view[3][4],
    float depth_scale,float depth_offset,float top,float bottom){
    if(!isfinite(depth_scale) || depth_scale==0 || !isfinite(depth_offset) ||
       !isfinite(top) || !isfinite(bottom) || top==bottom)return 0;
    float k=-1.f/(top-bottom);
    float p[2][4]={{0,0,-1.f/depth_scale,depth_offset},
        {k*inverse_view[1][0],k*inverse_view[1][1],k*inverse_view[1][2],
         k*inverse_view[1][3]+-top*k}};
    for(unsigned row=0;row<2;row++)for(unsigned column=0;column<4;column++)
        if(!isfinite(p[row][column]))return 0;
    for(unsigned row=0;row<2;row++)for(unsigned column=0;column<4;column++)
        planes[row][column]=p[row][column];
    return 1;
}
