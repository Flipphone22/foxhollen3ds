#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "fh_uvgen_matrix.h"
static uint32_t seed=1;
static float sample(void){seed=seed*1664525u+1013904223u;return ((int)(seed%8193)-4096)/256.f;}
int main(void){
    unsigned checks=0;
    for(unsigned trial=0;trial<20000;trial++){
        float post[3][4]={{0}},matrix[3][4],out[3][4],uv[3]={sample(),sample(),1};
        for(unsigned r=0;r<2;r++)for(unsigned c=0;c<4;c++)post[r][c]=sample();
        post[2][trial%2?2:3]=1;memcpy(matrix,post,sizeof(matrix));matrix[2][0]=NAN;
        assert(fh_uvgen_matrix(out,NULL,post));
        for(unsigned r=0;r<3;r++){
            float original=post[r][0]*uv[0]+post[r][1]*uv[1]+post[r][2]*uv[2]+post[r][3];
            float resolved=out[r][0]*uv[0]+out[r][1]*uv[1]+out[r][2]*uv[2]+out[r][3];
            assert(!memcmp(&original,&resolved,4));checks++;
        }
        assert(fh_uvgen_matrix(out,matrix,NULL)); /* 2x4 ignores third row. */
        assert(!fh_uvgen_matrix(out,matrix,post));assert(!fh_uvgen_matrix(out,NULL,NULL));
        for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++){
            float keep=post[r][c];post[r][c]=NAN;assert(!fh_uvgen_matrix(out,NULL,post));
            post[r][c]=INFINITY;assert(!fh_uvgen_matrix(out,NULL,post));post[r][c]=keep;
        }
        post[2][trial%4]+=1;assert(!fh_uvgen_matrix(out,NULL,post));
    }
    printf("PASS %u exact post-only coordinate components, 20000 changing matrices; projective/nonfinite/composed states rejected\n",checks);
}
