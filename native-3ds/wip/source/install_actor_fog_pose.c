#include "fh_install_actor_fog_pose.h"
#include <math.h>
#include <string.h>
static float scalar(const unsigned char*p){
    uint32_t u=(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
    float f;memcpy(&f,&u,4);return f;
}
static int recipe(FhPicaActorFogPosePlan*out,const FhInstallVertexSource*s,
    const FhPicaActorFogMatrix*k){
    unsigned n=s->joints_count;
    if(!fh_pica_actor_fog_matrix_valid(k,n?n+s->blends_count:1,s->fixed_world))return 0;
    FhPicaActorFogPosePlan p={.kind=k->kind,.weight=4};
    if(k->kind==FH_ACTOR_FOG_WORLD_POSITION || k->kind==FH_ACTOR_FOG_WORLD_NORMAL){*out=p;return 1;}
    if(!n){if(k->joint || s->blends_count)return 0;}
    else if(k->joint<n)p.a=p.b=k->joint;
    else{
        unsigned extra=k->joint-n;
        if(extra>=s->blends_count || !s->blends || !s->joints)return 0;
        const unsigned char*b=s->blends+extra*4;
        if(b[0]>=n || b[1]>=n || b[2]>4)return 0;
        p.a=b[0];p.b=b[1];p.weight=b[2];p.blend=1;
        for(unsigned i=0;i<3;i++){
            p.tail_a[i]=scalar(s->joints+p.a*28+16+i*4);
            p.tail_b[i]=scalar(s->joints+p.b*28+16+i*4);
            if(!isfinite(p.tail_a[i]) || !isfinite(p.tail_b[i]))return 0;
        }
    }
    *out=p;return 1;
}
int fh_install_actor_fog_pose_plan(FhPicaActorFogPosePlan*out,unsigned capacity,
    const FhInstallVertexSource*s,const FhPicaActorFogMatrix*keys,unsigned count){
    if(!out || !s || !s->raw || s->world || !keys || !count || count>22 || capacity<count ||
       s->joints_count>255 || s->blends_count>255 || s->joints_count+s->blends_count>256)return 0;
    FhPicaActorFogPosePlan p;
    for(unsigned i=0;i<count;i++)if(!recipe(&p,s,keys+i))return 0;
    for(unsigned i=0;i<count;i++){recipe(&p,s,keys+i);out[i]=p;}
    return 1;
}
