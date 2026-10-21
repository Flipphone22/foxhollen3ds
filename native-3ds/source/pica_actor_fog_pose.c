#include "fh_pica_actor_fog.h"
#include <math.h>
#include <string.h>

extern void C_MTXConcat(const float a[3][4],const float b[3][4],float out[3][4]);
static int finite_matrix(const float *m){
    for(unsigned i=0;i<12;i++)if(!isfinite(m[i]))return 0;
    return 1;
}
int fh_pica_actor_fog_blend_pose(float out[3][4],const float a[12],const float b[12],
    const float tail_a[3],const float tail_b[3],unsigned quarter_weight){
    if(!out || !a || !b || !tail_a || !tail_b || quarter_weight>4 ||
       !finite_matrix(a) || !finite_matrix(b))return 0;
    for(unsigned i=0;i<3;i++)if(!isfinite(tail_a[i]) || !isfinite(tail_b[i]))return 0;
    float translation[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}},ma[3][4],mb[3][4];
    for(unsigned i=0;i<3;i++)translation[i][3]=-tail_a[i];
    C_MTXConcat((const float(*)[4])a,translation,ma);
    for(unsigned i=0;i<3;i++)translation[i][3]=-tail_b[i];
    C_MTXConcat((const float(*)[4])b,translation,mb);
    float weight=quarter_weight*.25f,inverse=1.f-weight;
    for(unsigned row=0;row<3;row++)for(unsigned column=0;column<4;column++)
        translation[row][column]=ma[row][column]*weight+mb[row][column]*inverse;
    if(!finite_matrix(&translation[0][0]))return 0;
    memcpy(out,translation,sizeof(translation));return 1;
}
int fh_pica_actor_fog_pose(float out[3][4],unsigned kind,const float pose[12],
    const float view[3][4],const float adjustment[3][4],unsigned cached_normal){
    if(!out || !pose || !view || cached_normal>1 ||
       (kind!=FH_ACTOR_FOG_POSITION && kind!=FH_ACTOR_FOG_NORMAL) ||
       !finite_matrix(pose) || !finite_matrix(&view[0][0]) ||
       (kind==FH_ACTOR_FOG_NORMAL && (!adjustment || !finite_matrix(&adjustment[0][0]))))return 0;
    float mid[3][4],result[3][4];
    C_MTXConcat(view,(const float(*)[4])pose,mid);
    if(kind==FH_ACTOR_FOG_NORMAL){
        if(!cached_normal)for(unsigned row=0;row<3;row++)mid[row][3]=0.f;
        C_MTXConcat(mid,adjustment,result);
        if(cached_normal)for(unsigned row=0;row<3;row++)result[row][3]=0.f;
    }else memcpy(result,mid,sizeof(result));
    if(!finite_matrix(&result[0][0]))return 0;
    memcpy(out,result,sizeof(result));return 1;
}
