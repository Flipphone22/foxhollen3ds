#ifndef FH_PICA_SHADOW_QUERY_H
#define FH_PICA_SHADOW_QUERY_H
#include <stdint.h>
#include <math.h>
/* Original eight-point shadow broadphase policy, with the two angle lookups
 * shared across corners. Only six bounds are published, never render vertices.
 * Preserve corner order and the source's repeated float-to-int truncation. */
static inline int fh_pica_shadow_query_box(int32_t out[6],const float base[3],
    float scale,float lower,float s0,float c0,float s1,float c1){
    static const int8_t corners[8][3]={{-1,-1,-1},{-1,0,-1},{1,0,-1},{1,-1,-1},
        {-1,-1,1},{-1,0,1},{1,0,1},{1,-1,1}};
    int32_t result[6]={INT32_MAX,INT32_MAX,INT32_MAX,INT32_MIN,INT32_MIN,INT32_MIN};
    for(unsigned i=0;i<8;i++){
        float x=corners[i][0],y=lower*corners[i][1],z=corners[i][2];
        float a=y*c1-z*s1,b=z*c1+y*s1;
        float p[3]={x*c0+b*s0,a,b*c0-x*s0};
        for(unsigned k=0;k<3;k++){
            float value=scale*p[k]+base[k];
            if(!isfinite(value)||value<=-2147483520.f||value>=2147483520.f)return 0;
            if(value<result[k])result[k]=(int32_t)value;
            if(value>result[k+3])result[k+3]=(int32_t)value;
        }
    }
    for(unsigned k=0;k<6;k++)out[k]=result[k];
    return 1;
}
#endif
