#include "fh_install_actor_fog_groups.h"
#include <string.h>

static int slot(const FhInstallActorFogGroup*g,const FhPicaActorFogMatrix*key){
    for(unsigned i=0;i<g->fog_count;i++)if(!memcmp(g->fog+i,key,sizeof(*key)))return (int)(g->bone_count+i);
    return -1;
}
static float position(const unsigned char*p,float scale){
    unsigned u=(unsigned)p[0]<<8|p[1];return (u<32768?(int)u:(int)u-65536)*scale;
}
int fh_install_actor_fog_pack(const FhInstallVertexSource*s,
    const FhInstallSkinWeights*positions,const FhInstallSkinWeights*normals,
    const FhInstallActorFogGroup*g,FhModelCompactVertex*primary,
    FhPicaActorFogGpuVertex*companion,unsigned capacity){
    if(!s || !s->raw || s->world || !g || !primary || !companion ||
       !g->vertices || g->vertices>8192 || capacity<g->vertices ||
       !g->bone_count || !g->fog_count || g->bone_count>22 || g->fog_count>22 ||
       g->bone_count+g->fog_count>22 ||
       !s->positions || (g->vat!=3 && g->vat!=5 && g->vat!=6 && g->vat!=7))return 0;
    for(unsigned i=0;i<g->fog_count;i++){
        const FhPicaActorFogMatrix*k=g->fog+i;
        if(!fh_pica_actor_fog_matrix_valid(k,256,s->fixed_world))return 0;
        for(unsigned j=0;j<i;j++)if(!memcmp(k,g->fog+j,sizeof(*k)))return 0;
    }
    for(unsigned i=0;i<g->vertices;i++){
        FhModelCompactVertex v;
        if(slot(g,&g->ref[i].fog)<0 || g->ref[i].visible.position>=s->positions_count ||
           !fh_install_compact_actor(s,&g->ref[i].visible,g->two_uv,g->bones,
               g->bone_count,positions,normals,&v))return 0;
    }
    float scale=g->vat==7?1.f:g->vat==5?1.f/8:1.f/256;
    for(unsigned i=0;i<g->vertices;i++){
        fh_install_compact_actor(s,&g->ref[i].visible,g->two_uv,g->bones,
            g->bone_count,positions,normals,primary+i);
        const unsigned char*p=s->positions+g->ref[i].visible.position*6;
        unsigned row=(unsigned)slot(g,&g->ref[i].fog)*3;
        FhPicaActorFogGpuVertex v={.skin={row,row,4,0}};
        for(unsigned c=0;c<3;c++)v.position[c]=position(p+c*2,scale);
        companion[i]=v;
    }
    return 1;
}
