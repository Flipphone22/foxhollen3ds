#include "fh_light_lod.h"
#include <assert.h>
#include <stdio.h>
static unsigned checks;
#define CHECK(x) do{assert(x);checks++;}while(0)
int main(void){
    FhGpuLighting original={0};original.mask=255|FH_LIGHT_MODEL_NORMAL|FH_LIGHT_LIVE_MATERIAL;
    FhDrawState s={0};s.raw_position=3;for(unsigned c=0;c<3;c++)s.model_palette[c][c]=1;
    for(unsigned c=0;c<3;c++){original.ambient[c]=.03f;original.normal[c][3]=.7f;}
    for(unsigned i=0;i<8;i++){
        original.lights[i][0][2]=10;original.lights[i][3][0]=original.lights[i][4][0]=1;
        for(unsigned c=0;c<3;c++)original.lights[i][2][c]=(.02f+.01f*i)*(c+1);
    }
    float center[3]={0};FhGpuLighting reduced=original;
    CHECK(fh_light_lod_reduce(&reduced,center));CHECK((reduced.mask&255)==3);
    CHECK(!memcmp(reduced.lights[0],original.lights[7],sizeof(original.lights[0])));
    for(unsigned r=0;r<5;r++)for(unsigned c=0;c<3;c++)CHECK(reduced.lights[1][r][c]==original.lights[6][r][c]);
    for(unsigned c=0;c<3;c++){
        float dropped=0;for(unsigned i=0;i<6;i++)dropped+=original.lights[i][2][c];
        CHECK(fabsf(reduced.lights[1][c][3]-.25f*dropped)<1e-6f);
        CHECK(fabsf(reduced.normal[c][2]-.5f*dropped)<1e-6f);
        CHECK(reduced.normal[c][0]==0 && reduced.normal[c][1]==0);
        CHECK(reduced.normal[c][3]==original.normal[c][3]);
    }
    for(unsigned test=0;test<1001;test++){
        float z=-1+test*.002f;
        FhVertex v={0};v.position[3]=1;v.color[0][0]=v.color[0][1]=v.color[0][2]=1;v.color[0][3]=.37f;
        v.uv[7][0]=sqrtf(fmaxf(0,1-z*z));v.uv[7][2]=z;
        fh_gpu_lighting_resolve(&v,&s,&reduced);
        for(unsigned c=0;c<3;c++){
            float dropped=0;for(unsigned i=0;i<6;i++)dropped+=original.lights[i][2][c];
            float expected=.03f+(original.lights[7][2][c]+original.lights[6][2][c])*fmaxf(0,z)+dropped*fmaxf(0,.25f+.5f*z);
            expected=.7f*fminf(1,expected);
            CHECK(fabsf(v.color[0][c]-expected)<2e-6f);
        }
        CHECK(v.color[0][3]==.37f);
    }
    /* Rejection is transactional, including malformed input and unsupported families. */
    for(unsigned test=0;test<7;test++){
        FhGpuLighting bad=original;
        if(test==0)bad.mask=3|FH_LIGHT_MODEL_NORMAL;
        if(test==1)bad.mask|=FH_LIGHT_SPHERE;
        if(test==2)bad.mask|=FH_LIGHT_FLAT;
        if(test==3)bad.mask|=FH_LIGHT_DIFFUSE_FILL;
        if(test==4)bad.lights[0][2][0]=NAN;
        if(test==5)bad.lights[0][2][0]=-1;
        if(test==6)bad.lights[0][4][0]=1e-30f;
        FhGpuLighting before=bad;CHECK(!fh_light_lod_reduce(&bad,center));CHECK(!memcmp(&bad,&before,sizeof(bad)));
    }
    printf("light_lod checks=%u; directional-fill semantics, not original-image equivalence\n",checks);return 0;
}
