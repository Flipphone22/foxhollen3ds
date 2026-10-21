#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "fh_native_splash_geometry.h"
int main(void){
    float pos[16][3],uv[128][2],bands[8][3][4];unsigned char color[32];
    FhMinimalEffectVertex out[630];
    for(unsigned k=0;k<16;k++){pos[k][0]=k*.25f;pos[k][1]=k-8.f;pos[k][2]=3-k*.5f;}
    for(unsigned m=0;m<8;m++){
        color[m*4+3]=m*31;
        for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)bands[m][r][c]=(r+1)*.25f+(c+1)*.5f+m;
        for(unsigned k=0;k<16;k++){uv[m*16+k][0]=k/15.f;uv[m*16+k][1]=m/7.f;}
    }
    fh_native_splash_geometry(out,pos,uv,color,bands);
    unsigned at=0;
    for(unsigned k=0;k<15;k++){
        unsigned strip[16];
        for(unsigned m=0;m<8;m++){strip[2*m]=(7-m)*16+k;strip[2*m+1]=(7-m)*16+k+1;}
        for(unsigned t=2;t<16;t++){
            unsigned tri[3]={strip[t-2],strip[t-1],strip[t]};
            if(t&1){unsigned tmp=tri[0];tri[0]=tri[1];tri[1]=tmp;}
            for(unsigned j=0;j<3;j++){
                unsigned ix=tri[j],m=ix/16;const FhMinimalEffectVertex *v=out+at++;
                for(unsigned r=0;r<3;r++){
                    float expected=bands[m][r][3];
                    for(unsigned c=0;c<3;c++)expected+=bands[m][r][c]*pos[ix%16][c];
                    assert(fabsf(v->position[r]-expected)<.0001f);
                }
                assert(v->position[3]==1 && v->color[3]==color[m*4+3]/255.f);
                assert(v->uv[0]==uv[ix][0] && v->uv[1]==uv[ix][1]);
            }
        }
    }
    assert(at==630);puts("630 strip vertices: winding, original band transforms, UVs and lifetime alpha pass");
}
