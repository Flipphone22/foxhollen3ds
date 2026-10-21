#include "fh_pica_actor_fog_plan.h"
int fh_pica_actor_fog_plan_world_pose(float out[3][4],const FhPicaActorFogPosePlan*p,
    const float world[3][4],const float view[3][4],const float adjustment[3][4]){
    if(!p || !world || p->a || p->b || p->weight!=4 || p->blend ||
       p->reserved[0] || p->reserved[1] || p->reserved[2] ||
       (p->kind!=FH_ACTOR_FOG_WORLD_POSITION && p->kind!=FH_ACTOR_FOG_WORLD_NORMAL))return 0;
    unsigned kind=p->kind==FH_ACTOR_FOG_WORLD_POSITION?FH_ACTOR_FOG_POSITION:FH_ACTOR_FOG_NORMAL;
    return fh_pica_actor_fog_pose(out,kind,&world[0][0],view,adjustment,0);
}
int fh_pica_actor_fog_plan_pose(float out[3][4],const FhPicaActorFogPosePlan*p,
    const float*poses,unsigned pose_count,const float view[3][4],
    const float adjustment[3][4],unsigned cached_normal){
    if(!out || !p || !poses || !pose_count || pose_count>256 || p->a>=pose_count || p->b>=pose_count ||
       p->weight>4 || p->blend>1 || p->reserved[0] || p->reserved[1] || p->reserved[2] ||
       (!p->blend && (p->a!=p->b || p->weight!=4)))return 0;
    const float*pose=poses+p->a*16;float blended[3][4];
    if(p->blend){
        if(!fh_pica_actor_fog_blend_pose(blended,pose,poses+p->b*16,p->tail_a,p->tail_b,p->weight))return 0;
        pose=&blended[0][0];
    }
    return fh_pica_actor_fog_pose(out,p->kind,pose,view,adjustment,cached_normal);
}
