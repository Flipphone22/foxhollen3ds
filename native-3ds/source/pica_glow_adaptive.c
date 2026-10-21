#include "fh_pica_glow_adaptive.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static int finite_vertex(const FhGlowMeshVertex *v){
    return isfinite(v->u)&&isfinite(v->v)&&isfinite(v->dx)&&isfinite(v->dy)&&
        v->u>=0&&v->u<=1&&v->v>=0&&v->v<=1;
}
static int vertex(FhGlowMesh *m,float u,float v,FhGlowMeshSample sample,void *ctx){
    float field[2];
    if(m->vertices>=FH_GLOW_MESH_VERTICES||!sample(ctx,u,v,field)||
       !isfinite(field[0])||!isfinite(field[1]))return 0;
    m->vertex[m->vertices++]=(FhGlowMeshVertex){u,v,field[0],field[1]};return 1;
}
static int score(const FhGlowMesh *m,unsigned i,const FhGlowMeshVertex *training,
                 unsigned count,float *out){
    const FhGlowMeshVertex *a=m->vertex+m->index[i][0],*b=m->vertex+m->index[i][1],*c=m->vertex+m->index[i][2];
    float bx=b->u-a->u,by=b->v-a->v,cx=c->u-a->u,cy=c->v-a->v;
    float det=bx*cy-by*cx,maximum=0;
    if(!(det>0)||!isfinite(det))return 0;
    /* Vertices are dyadic midpoints of the unit square. Keep a deliberately
     * loose halo around the triangle: it exceeds the barycentric admission
     * tolerance and float roundoff, so nearby samples still take the exact
     * original calculation. Most distant samples avoid both VFP divides.
     * This is load-time work only; no resident storage or draw work added. */
    const float halo=1.f/512.f;
    float min_u=fminf(a->u,fminf(b->u,c->u))-halo;
    float max_u=fmaxf(a->u,fmaxf(b->u,c->u))+halo;
    float min_v=fminf(a->v,fminf(b->v,c->v))-halo;
    float max_v=fmaxf(a->v,fmaxf(b->v,c->v))+halo;
    for(unsigned n=0;n<count;n++){
        if(training[n].u<min_u||training[n].u>max_u||
           training[n].v<min_v||training[n].v>max_v)continue;
        float px=training[n].u-a->u,py=training[n].v-a->v;
        float u=(px*cy-py*cx)/det,v=(bx*py-by*px)/det;
        if(u< -1e-6f||v< -1e-6f||u+v>1.000001f)continue;
        float dx=(a->dx+(b->dx-a->dx)*u+(c->dx-a->dx)*v-training[n].dx)*.4f;
        float dy=(a->dy+(b->dy-a->dy)*u+(c->dy-a->dy)*v-training[n].dy)*(128.f/240.f);
        float e=dx*dx+dy*dy;
        if(!isfinite(e))return 0;
        if(e>maximum)maximum=e;
    }
    *out=maximum;return 1;
}
static int compile(FhGlowMesh *m,const FhGlowMeshVertex *training,unsigned count,
                   unsigned budget,FhGlowMeshSample sample,void *ctx,int shared){
    if(!m||!training||!sample||!count||count>FH_GLOW_MESH_TRAINING||
       budget<(shared?4u:2u)||budget>FH_GLOW_MESH_TRIANGLES)return 0;
    uintptr_t dst=(uintptr_t)m,src=(uintptr_t)training;size_t bytes=count*sizeof(*training);
    if(dst>UINTPTR_MAX-sizeof(*m)||src>UINTPTR_MAX-bytes||
       (dst<src+bytes&&src<dst+sizeof(*m)))return 0;
    for(unsigned i=0;i<count;i++)if(!finite_vertex(training+i))return 0;
    m->vertices=m->triangles=0;
    if(!vertex(m,0,0,sample,ctx)||!vertex(m,1,0,sample,ctx)||
       !vertex(m,1,1,sample,ctx)||!vertex(m,0,1,sample,ctx))goto failed;
    const uint16_t initial[2][3]={{0,1,2},{0,2,3}};
    memcpy(m->index,initial,sizeof initial);m->triangles=2;
    float scores[FH_GLOW_MESH_TRIANGLES];
    if(shared){
        if(!vertex(m,.5f,.5f,sample,ctx))goto failed;
        /* Exact topology/order after the ordinary first diagonal split.
         * Retain it even for constant fields: alpha/shape may not be planar. */
        const uint16_t crossed[4][3]={{2,4,1},{4,0,1},{0,4,3},{4,2,3}};
        memcpy(m->index,crossed,sizeof crossed);m->triangles=4;
    }
    for(unsigned i=0;i<m->triangles;i++)if(!score(m,i,training,count,scores+i))goto failed;
    while(m->triangles<budget){
        unsigned selected=0;
        for(unsigned i=1;i<m->triangles;i++)if(scores[i]>scores[selected])selected=i;
        if(scores[selected]<1e-20f)break;
        unsigned ea=0,eb=0;float longest=-1;
        for(unsigned e=0;e<3;e++){
            unsigned a=m->index[selected][e],b=m->index[selected][(e+1)%3];
            float x=m->vertex[a].u-m->vertex[b].u,y=m->vertex[a].v-m->vertex[b].v;
            float distance=x*x+y*y;
            if(distance>longest){longest=distance;ea=a;eb=b;}
        }
        uint16_t split[2][3];unsigned owners[2],owner_count=0;
        for(unsigned i=0;i<m->triangles;i++){
            unsigned has_a=0,has_b=0;
            for(unsigned j=0;j<3;j++){has_a|=m->index[i][j]==ea;has_b|=m->index[i][j]==eb;}
            if(has_a&&has_b){
                if(owner_count==2)goto failed;
                owners[owner_count]=i;memcpy(split[owner_count++],m->index[i],sizeof split[0]);
            }
        }
        if(!owner_count)goto failed;
        if(m->triangles+owner_count>budget)break;
        unsigned mid=m->vertices;
        if(!vertex(m,(m->vertex[ea].u+m->vertex[eb].u)*.5f,
                     (m->vertex[ea].v+m->vertex[eb].v)*.5f,sample,ctx))goto failed;
        unsigned kept=0;
        for(unsigned i=0;i<m->triangles;i++){
            if(i==owners[0]||(owner_count==2&&i==owners[1]))continue;
            if(kept!=i)memcpy(m->index[kept],m->index[i],sizeof m->index[0]);
            scores[kept++]=scores[i];
        }
        m->triangles=kept;
        for(unsigned o=0;o<owner_count;o++)for(unsigned e=0;e<3;e++){
            unsigned a=split[o][e],b=split[o][(e+1)%3],c=split[o][(e+2)%3];
            if(!((a==ea&&b==eb)||(a==eb&&b==ea)))continue;
            uint16_t children[2][3]={{a,mid,c},{mid,b,c}};
            for(unsigned j=0;j<2;j++){
                unsigned i=m->triangles++;
                memcpy(m->index[i],children[j],sizeof children[j]);
                if(!score(m,i,training,count,scores+i))goto failed;
            }
            break;
        }
    }
    return 1;
failed:
    m->vertices=m->triangles=0;return 0;
}
int fh_glow_mesh_compile(FhGlowMesh *m,const FhGlowMeshVertex *training,unsigned count,
                         unsigned budget,FhGlowMeshSample sample,void *ctx){
    return compile(m,training,count,budget,sample,ctx,0);
}
int fh_glow_mesh_compile_shared(FhGlowMesh *m,const FhGlowMeshVertex *training,unsigned count,
                                unsigned budget,FhGlowMeshSample sample,void *ctx){
    return compile(m,training,count,budget,sample,ctx,1);
}
