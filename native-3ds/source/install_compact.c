#include "fh_install_compact.h"
#include <math.h>
#include <string.h>
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static int signed_half(const unsigned char*p){unsigned n=half(p);return n<32768?(int)n:(int)n-65536;}
static float real(const unsigned char*p){uint32_t n=(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];float f;memcpy(&f,&n,4);return f;}
static int valid(const FhInstallVertexSource*s,const FhModelRef*r,unsigned second){
    return s&&s->raw&&r&&second<=1&&r->position<s->positions_count&&
        (r->color==65535||r->color<s->colors_count)&&
        (r->normal==65535||r->normal<s->normals_count)&&
        (r->uv==65535||r->uv<s->uvs_count)&&(!second||r->material<s->uvs_count);
}
static void attributes(const FhInstallVertexSource*s,const FhModelRef*r,unsigned second,
                       int16_t pos[4],uint8_t color[4],int16_t uv[4]){
    for(unsigned k=0;k<3;k++)pos[k]=signed_half(s->positions+r->position*6+k*2);pos[3]=1;
    unsigned c=r->color==65535?65535:half(s->colors+r->color*2);
    for(unsigned k=0;k<4;k++)color[k]=(c>>(12-k*4))&15;
    for(unsigned k=0;k<2;k++){
        uv[k]=r->uv==65535?0:signed_half(s->uvs+r->uv*4+k*2);
        uv[k+2]=second?signed_half(s->uvs+r->material*4+k*2):uv[k];
    }
}
int fh_install_compact_world(const FhInstallVertexSource*s,const FhModelRef*r,unsigned second,FhWorldCompactVertex*out){
    if(!out||!valid(s,r,second)||!s->world||r->joint||r->normal!=65535)return 0;
    FhWorldCompactVertex v={0};int16_t uv[4];attributes(s,r,second,v.position,v.color,uv);
    memcpy(v.uv,uv,4);if(second)memcpy(v.second_uv,uv+2,4);*out=v;return 1;
}
static int sparse(const FhInstallSkinWeights*w,unsigned pair[4]){
    if(!w->present)return 1;
    if(!w->count||w->count>2||w->weight[0]+w->weight[1]!=128||!w->weight[0]||
       (w->count==1&&w->weight[1])||(w->count==2&&(!w->weight[1]||w->joint[0]>=w->joint[1])))return 0;
    pair[0]=w->joint[0];pair[1]=w->count==1?w->joint[0]:w->joint[1];
    pair[2]=w->weight[0];pair[3]=w->count==1?0:w->weight[1];return 1;
}
static int slot(const uint16_t*bones,unsigned count,unsigned bone){for(unsigned i=0;i<count;i++)if(bones[i]==bone)return (int)i*3;return -1;}
int fh_install_compact_weights(const FhInstallVertexSource*s,const FhModelRef*r,
    const FhInstallSkinWeights*positions,const FhInstallSkinWeights*normals,unsigned out[4]){
    if(!out||!valid(s,r,0)||s->world)return 0;
    FhModelBinding b;if(!fh_install_vertex_binding(s,r->joint,&b))return 0;
    unsigned pos[4]={b.a,b.b,b.weight*32,(4-b.weight)*32},nrm[4];memcpy(nrm,pos,sizeof(pos));
    if(s->fixed_world){
        if((s->positions_count&&!positions)||(s->normals_count&&!normals))return 0;
        if(r->joint==s->joints_count){
            if(!sparse(positions+r->position,pos))return 0;
            if(r->normal!=65535&&!sparse(normals+r->normal,nrm))return 0;
        }
        if(memcmp(pos,nrm,sizeof(pos)))return 0;
    }
    memcpy(out,pos,sizeof(pos));return 1;
}
int fh_install_compact_actor(const FhInstallVertexSource*s,const FhModelRef*r,unsigned second,
    const uint16_t*bones,unsigned count,const FhInstallSkinWeights*positions,const FhInstallSkinWeights*normals,FhModelCompactVertex*out){
    if(!out||!valid(s,r,second)||s->world||!bones||!count||count>22)return 0;
    unsigned total=s->rigid?1:s->joints_count+s->fixed_world;
    for(unsigned i=0;i<count;i++){if(bones[i]>=total)return 0;for(unsigned j=0;j<i;j++)if(bones[i]==bones[j])return 0;}
    FhModelBinding b;if(!fh_install_vertex_binding(s,r->joint,&b))return 0;
    FhModelCompactVertex v={0};attributes(s,r,second,v.position,v.color,v.uv);
    for(unsigned k=0;k<3;k++){
        if(r->normal!=65535){unsigned n=s->normals[r->normal*s->normal_stride+k];v.normal[k]=n<128?(int)n:(int)n-256;}
        v.tail[k]=b.bind_space||b.a>=s->joints_count?0:real(s->joints+b.a*28+16+k*4);
        if(!isfinite(v.tail[k]))return 0;
    }
    unsigned pos[4];if(!fh_install_compact_weights(s,r,positions,normals,pos))return 0;
    int a=slot(bones,count,pos[pos[2]?0:1]),d=slot(bones,count,pos[pos[3]?1:0]);
    if(a<0||d<0)return 0;
    v.skin[0]=a;v.skin[1]=d;v.skin[2]=s->fixed_world?pos[2]:b.weight;v.skin[3]=s->fixed_world?pos[3]:0;
    *out=v;return 1;
}
