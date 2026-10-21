/* Private captured ARM geometry admission diagnostics; no game boot. */
#include "fh_movie_disjoint.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f)return 3;
    unsigned char magic[8],bytes[3720];uint32_t header[4];FhVertex v[384];
    if(fread(magic,1,8,f)!=8 || fread(header,4,4,f)!=4 || memcmp(magic,"FHDRAW",6) ||
       header[1]>384 || header[1]%3 || header[2]!=3720 || header[3]!=148 ||
       fread(bytes,1,3720,f)!=3720 || fread(v,148,header[1],f)!=header[1])return 4;
    fclose(f);
    /* ARM capture has 32-bit pointers; copy only pointer-free transform fields
     * at their recorded ABI offsets into the host's wider draw-state layout. */
    FhDrawState state={0};
    memcpy(state.viewport,bytes+2932,24);memcpy(&state.raw_position,bytes+3172,4);
    memcpy(state.position_matrix,bytes+3176,64);memcpy(state.model_palette,bytes+3240,480);
    unsigned groups[128],n=fh_movie_groups(groups,v,header[1],&state);
    unsigned projected=0,nondegenerate=0,separated=0,adjacent=0;
    FhMoviePoint p[384];unsigned char valid[128]={0};float projection[4][4];fh_vertex_pica_projection(projection,&state);
    float max_error=0,min_area=1e30f,max_area=0;
    for(unsigned i=0;i<header[1];i+=3){
        unsigned good=1;
        for(unsigned j=0;j<3;j++){
            unsigned yes=fh_movie_project(p+i+j,v+i+j,&state,projection);projected+=yes;good&=yes;
            if(yes){float error=p[i+j].ex+p[i+j].ey;if(error>max_error)max_error=error;}
        }
        if(!good)continue;
        float area=fabsf((p[i+1].x-p[i].x)*(p[i+2].y-p[i].y)-(p[i+1].y-p[i].y)*(p[i+2].x-p[i].x));
        if(area<min_area)min_area=area;if(area>max_area)max_area=area;
        valid[i/3]=fh_movie_side(p+i,p+i+1,p+i+2)!=0;nondegenerate+=valid[i/3];
        for(unsigned j=0;j<i;j+=3)if(valid[i/3] && valid[j/3]){
            unsigned yes=fh_movie_separates(v+i,p+i,v+j,p+j,state.raw_position) || fh_movie_separates(v+j,p+j,v+i,p+i,state.raw_position);
            separated+=yes;if(j+3==i)adjacent+=yes;
        }
    }
    printf("{\"frame\":%u,\"vertices\":%u,\"raw_position\":%u,\"projected\":%u,\"nondegenerate\":%u,\"separated_pairs\":%u,\"adjacent_separated\":%u,\"max_error\":%.9g,\"min_area\":%.9g,\"max_area\":%.9g,\"groups\":[",header[0],header[1],state.raw_position,projected,nondegenerate,separated,adjacent,max_error,min_area,max_area);
    for(unsigned i=0;i<n;i++)printf("%s%u",i?",":"",groups[i]);puts("]}");return 0;
}
