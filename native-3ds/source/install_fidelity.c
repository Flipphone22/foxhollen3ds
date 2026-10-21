#include "fh_install_fidelity.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {double low[3],high[3];unsigned first,count,left,right;} Node;
typedef struct {
    const FhInstallSurface*s;Node*nodes;unsigned *order,*hash,*next,used,hash_count;
    size_t bytes;
} Tree;
static const double weights[7][3]={{1,0,0},{0,1,0},{0,0,1},{.5,.5,0},{0,.5,.5},{.5,0,.5},{1./3,1./3,1./3}};
static double dot(const double*a,const double*b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static void difference(double*out,const double*a,const double*b){for(unsigned k=0;k<3;k++)out[k]=a[k]-b[k];}
static void normal(const FhInstallSurfaceVertex*v,double*out,double*length){
    double a[3],b[3];difference(a,v[1].position,v[0].position);difference(b,v[2].position,v[0].position);
    out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];
    *length=sqrt(dot(out,out));double divisor=fmax(*length,1e-30);for(unsigned k=0;k<3;k++)out[k]/=divisor;
}
static double closest(const double*p,const FhInstallSurfaceVertex*t,double*bary){
    double a[3],b[3],rel[3];difference(a,t[1].position,t[0].position);difference(b,t[2].position,t[0].position);difference(rel,p,t[0].position);
    double aa=dot(a,a),ab=dot(a,b),bb=dot(b,b),den=aa*bb-ab*ab,safe=fabs(den)>1e-16?den:1;
    double y=(bb*dot(rel,a)-ab*dot(rel,b))/safe,z=(aa*dot(rel,b)-ab*dot(rel,a))/safe;
    bary[0]=1-y-z;bary[1]=y;bary[2]=z;double best=INFINITY;
    if(fabs(den)>1e-16&&fmin(bary[0],fmin(y,z))>=-1e-10){
        double delta[3];for(unsigned k=0;k<3;k++)delta[k]=p[k]-(bary[0]*t[0].position[k]+y*t[1].position[k]+z*t[2].position[k]);best=dot(delta,delta);
    }
    for(unsigned i=0;i<3;i++){
        unsigned j=(i+1)%3;double edge[3],d[3];difference(edge,t[j].position,t[i].position);difference(d,p,t[i].position);
        double u=fmax(0,fmin(1,dot(d,edge)/fmax(dot(edge,edge),1e-30)));
        for(unsigned k=0;k<3;k++)d[k]=p[k]-(t[i].position[k]+u*edge[k]);double distance=dot(d,d);
        if(distance<best){best=distance;bary[0]=bary[1]=bary[2]=0;bary[i]=1-u;bary[j]=u;}
    }return best;
}
static double centroid(const Tree*t,unsigned id,unsigned axis){const FhInstallSurfaceVertex*v=t->s->vertex+id*3;return v[0].position[axis]+v[1].position[axis]+v[2].position[axis];}
static int less(const Tree*t,unsigned a,unsigned b,unsigned axis){double x=centroid(t,a,axis),y=centroid(t,b,axis);return x==y?a<b:x<y;}
static void swap(unsigned*a,unsigned*b){unsigned n=*a;*a=*b;*b=n;}
static void median(Tree*t,unsigned start,unsigned end,unsigned target,unsigned axis){
    while(end-start>1){
        unsigned pivot=t->order[start+(end-start)/2];swap(t->order+start+(end-start)/2,t->order+end-1);unsigned at=start;
        for(unsigned i=start;i+1<end;i++)if(less(t,t->order[i],pivot,axis)){swap(t->order+i,t->order+at);at++;}
        swap(t->order+at,t->order+end-1);if(at==target)return;if(at<target)start=at+1;else end=at;
    }
}
static unsigned build(Tree*t,unsigned first,unsigned count){
    unsigned id=t->used++;Node*n=t->nodes+id;n->first=first;n->count=count;
    for(unsigned k=0;k<3;k++){n->low[k]=INFINITY;n->high[k]=-INFINITY;}
    for(unsigned i=first;i<first+count;i++)for(unsigned v=0;v<3;v++)for(unsigned k=0;k<3;k++){
        double x=t->s->vertex[t->order[i]*3+v].position[k];n->low[k]=fmin(n->low[k],x);n->high[k]=fmax(n->high[k],x);
    }
    if(count>8){
        unsigned axis=0;for(unsigned k=1;k<3;k++)if(n->high[k]-n->low[k]>n->high[axis]-n->low[axis])axis=k;
        unsigned half=count/2;median(t,first,first+count,first+half,axis);n->count=0;
        n->left=build(t,first,half);n->right=build(t,first+half,count-half);
    }return id;
}
static int vertex_compare(const FhInstallSurfaceVertex*a,const FhInstallSurfaceVertex*b,unsigned channels){
    for(unsigned k=0;k<3;k++)if(a->position[k]!=b->position[k])return a->position[k]<b->position[k]?-1:1;
    for(unsigned k=0;k<channels;k++)if(a->uv[k]!=b->uv[k])return a->uv[k]<b->uv[k]?-1:1;
    for(unsigned k=0;k<4;k++)if(a->rgba[k]!=b->rgba[k])return a->rgba[k]<b->rgba[k]?-1:1;return 0;
}
static unsigned rotation(const FhInstallSurfaceVertex*v,unsigned channels){
    unsigned start=0;
    for(unsigned i=1;i<3;i++){
        for(unsigned j=0;j<3;j++){
            int c=vertex_compare(v+(i+j)%3,v+(start+j)%3,channels);if(c<0){start=i;break;}if(c>0)break;
        }
    }
    return start;
}
static unsigned hash_double(unsigned h,double x){
    uint64_t bits=0;if(x!=0)memcpy(&bits,&x,8);for(unsigned k=0;k<8;k++){h^=(unsigned)((bits>>(k*8))&255);h*=16777619u;}return h;
}
static unsigned face_hash(const FhInstallSurfaceVertex*v,unsigned channels){
    unsigned start=rotation(v,channels),h=2166136261u;
    for(unsigned i=0;i<3;i++){
        const FhInstallSurfaceVertex*p=v+(start+i)%3;
        for(unsigned k=0;k<3;k++)h=hash_double(h,p->position[k]);
        for(unsigned k=0;k<channels;k++)h=hash_double(h,p->uv[k]);for(unsigned k=0;k<4;k++)h=hash_double(h,p->rgba[k]);
    }return h;
}
static int equal(const FhInstallSurfaceVertex*a,const FhInstallSurfaceVertex*b,unsigned channels){
    for(unsigned start=0;start<3;start++){
        unsigned i=0;while(i<3&&!vertex_compare(a+i,b+(i+start)%3,channels))i++;if(i==3)return 1;
    }return 0;
}
static void close_tree(Tree*t){free(t->nodes);free(t->order);free(t->hash);free(t->next);memset(t,0,sizeof(*t));}
static int open_tree(Tree*t,const FhInstallSurface*s){
    t->s=s;t->hash_count=1;while(t->hash_count<s->triangles*2)t->hash_count*=2;
    /* Median halves reach <=8 faces within ceil(log2(N/8)) levels. */
    unsigned leaves=1;while(leaves*8<s->triangles)leaves*=2;unsigned nodes=leaves*2-1;
    t->bytes=(size_t)nodes*sizeof(Node)+(size_t)(s->triangles*2+t->hash_count)*sizeof(unsigned);
    t->nodes=calloc(nodes,sizeof(Node));t->order=malloc(s->triangles*sizeof(unsigned));t->next=calloc(s->triangles,sizeof(unsigned));t->hash=calloc(t->hash_count,sizeof(unsigned));
    if(!t->nodes||!t->order||!t->next||!t->hash){close_tree(t);return 0;}
    for(unsigned i=0;i<s->triangles;i++){
        t->order[i]=i;unsigned h=face_hash(s->vertex+i*3,s->uv_channels)&(t->hash_count-1);t->next[i]=t->hash[h];t->hash[h]=i+1;
    }
    build(t,0,s->triangles);return 1;
}
static double box_distance(const Node*n,const double*p){
    double sum=0;for(unsigned k=0;k<3;k++){double x=fmax(n->low[k]-p[k],fmax(p[k]-n->high[k],0));sum+=x*x;}return sum;
}
static void nearest(const Tree*t,unsigned id,const double*p,double*best,unsigned*face){
    const Node*n=t->nodes+id;if(box_distance(n,p)>*best+1e-8)return;
    if(n->count){for(unsigned i=n->first;i<n->first+n->count;i++){
        unsigned index=t->order[i];double bary[3],d=closest(p,t->s->vertex+index*3,bary);
        if(d<*best||(d==*best&&index<*face)){*best=d;*face=index;}
    }}else{
        unsigned a=n->left,b=n->right;if(box_distance(t->nodes+a,p)>box_distance(t->nodes+b,p))swap(&a,&b);
        nearest(t,a,p,best,face);nearest(t,b,p,best,face);
    }
}
typedef struct {double p[3],uv[4],color[4],normal[3],length,near,best,duv,dcolor;unsigned face;} Query;
static void attributes(const Tree*t,unsigned id,Query*q,const FhInstallFidelityLimits*limits){
    const Node*n=t->nodes+id;if(box_distance(n,q->p)>q->near+1e-8)return;
    if(!n->count){attributes(t,n->left,q,limits);attributes(t,n->right,q,limits);return;}
    for(unsigned i=n->first;i<n->first+n->count;i++){
        unsigned index=t->order[i];const FhInstallSurfaceVertex*v=t->s->vertex+index*3;double bary[3];
        if(closest(q->p,v,bary)>q->near+1e-8)continue;
        double norm[3],length;normal(v,norm,&length);
        if(q->length>=1e-12&&length>=1e-12&&dot(q->normal,norm)<limits->normal_cos)continue;
        double duv=0,dcolor=0;
        for(unsigned k=0;k<t->s->uv_channels;k++)duv=fmax(duv,fabs(bary[0]*v[0].uv[k]+bary[1]*v[1].uv[k]+bary[2]*v[2].uv[k]-q->uv[k]));
        for(unsigned k=0;k<4;k++)dcolor=fmax(dcolor,fabs(bary[0]*v[0].rgba[k]+bary[1]*v[1].rgba[k]+bary[2]*v[2].rgba[k]-q->color[k]));
        double score=fmax(duv/limits->uv,dcolor/limits->color);
        if(score<q->best||(score==q->best&&index<q->face)){q->best=score;q->face=index;q->duv=duv;q->dcolor=dcolor;}
    }
}
static int valid(const FhInstallSurface*s){
    if(!s||!s->vertex||!s->triangles||s->triangles>FH_INSTALL_SURFACE_TRIANGLES||s->uv_channels>4||s->uv_channels%2)return 0;
    for(unsigned i=0;i<s->triangles*3;i++){
        const FhInstallSurfaceVertex*v=s->vertex+i;
        for(unsigned k=0;k<3;k++)if(!isfinite(v->position[k])||fabs(v->position[k])>1e12)return 0;
        for(unsigned k=0;k<s->uv_channels;k++)if(!isfinite(v->uv[k])||fabs(v->uv[k])>1e12)return 0;
        for(unsigned k=0;k<4;k++)if(!isfinite(v->rgba[k])||v->rgba[k]<0||v->rgba[k]>1)return 0;
    }return 1;
}
static int direction(const FhInstallSurface*a,const FhInstallSurface*b,const FhInstallFidelityLimits*limits,FhInstallFidelity*out){
    Tree t={0};if(!open_tree(&t,b)){out->reason=FH_INSTALL_FIDELITY_MEMORY;return 0;}
    if(t.bytes>out->scratch_bytes)out->scratch_bytes=t.bytes;int ok=0;
    for(unsigned i=0;i<a->triangles;i++){
        const FhInstallSurfaceVertex*v=a->vertex+i*3;unsigned h=face_hash(v,a->uv_channels)&(t.hash_count-1),found=0;
        for(unsigned entry=t.hash[h];entry;entry=t.next[entry-1])if(equal(v,b->vertex+(entry-1)*3,a->uv_channels)){found=1;break;}
        if(found){out->exact_faces++;continue;}
        for(unsigned sample=0;sample<7;sample++){
            Query q={0};q.near=q.best=INFINITY;q.face=~0u;normal(v,q.normal,&q.length);
            for(unsigned j=0;j<3;j++){
                for(unsigned k=0;k<3;k++)q.p[k]+=weights[sample][j]*v[j].position[k];
                for(unsigned k=0;k<a->uv_channels;k++)q.uv[k]+=weights[sample][j]*v[j].uv[k];
                for(unsigned k=0;k<4;k++)q.color[k]+=weights[sample][j]*v[j].rgba[k];
            }
            unsigned face=~0u;nearest(&t,0,q.p,&q.near,&face);out->samples++;out->source_face=i;out->target_face=face;
            out->position=fmax(out->position,sqrt(q.near));
            if(q.near>(limits->position+1e-7)*(limits->position+1e-7)){out->reason=FH_INSTALL_FIDELITY_DISTANCE;goto done;}
            attributes(&t,0,&q,limits);out->target_face=q.face;
            if(q.face!=~0u){out->uv=fmax(out->uv,q.duv);out->color=fmax(out->color,q.dcolor);}
            if(q.best>1+1e-6){out->reason=FH_INSTALL_FIDELITY_ATTRIBUTES;goto done;}
        }
    }ok=1;
done:close_tree(&t);return ok;
}
int fh_install_fidelity(const FhInstallSurface*a,const FhInstallSurface*b,const FhInstallFidelityLimits*limits,FhInstallFidelity*out){
    if(!out)return 0;memset(out,0,sizeof(*out));out->reason=FH_INSTALL_FIDELITY_INVALID;
    if(!limits||!valid(a)||!valid(b)||a->uv_channels!=b->uv_channels||
       !isfinite(limits->position)||limits->position<=0||!isfinite(limits->uv)||limits->uv<=0||
       !isfinite(limits->color)||limits->color<=0||!isfinite(limits->normal_cos)||fabs(limits->normal_cos)>1)return 0;
    if(!direction(a,b,limits,out))return 0;out->direction=1;
    if(!direction(b,a,limits,out))return 0;out->reason=FH_INSTALL_FIDELITY_OK;out->accepted=1;return 1;
}
