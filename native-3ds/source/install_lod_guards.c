#include "fh_install_lod_guards.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static int fail(FhInstallLodGuards*g,const char*why){snprintf(g->error,sizeof(g->error),"%s",why);return 0;}
void fh_install_lod_guards_close(FhInstallLodGuards*g){if(g){free(g->positions);free(g->normals);free(g->morph);memset(g,0,sizeof(*g));}}
static int morphs(FhInstallLodGuards*g){
    const FhInstallVertexSource*s=g->source;if(s->world||!s->raw[0xf9])return 1;
    unsigned count=s->raw[0xf9],table=word(s->raw+0xdc);
    if(!table||table>s->bytes||count>(s->bytes-table)/4)return fail(g,"Invalid source morph table.");
    for(unsigned i=0;i<count;i++){
        unsigned at=word(s->raw+table+i*4),previous=~0u;if(!at)return fail(g,"Missing source morph stream.");
        unsigned ended=0;
        for(unsigned j=0;j<=s->positions_count;j++){
            if(at>s->bytes||2>s->bytes-at)return fail(g,"Truncated source morph stream.");
            unsigned flags=half(s->raw+at),index=flags&0x1fff;at+=2;
            if(index>=s->positions_count){ended=1;break;}
            if(previous!=~0u&&index<=previous)return fail(g,"Unordered source morph stream.");previous=index;
            for(unsigned bit=0x2000;bit<=0x8000;bit*=2)if(flags&bit){
                if(at>s->bytes||2>s->bytes-at)return fail(g,"Truncated source morph delta.");
                if(half(s->raw+at))g->morph[index]=1;at+=2;
            }
        }
        if(!ended)return fail(g,"Unterminated source morph stream.");
    }return 1;
}
int fh_install_lod_guards_begin(FhInstallLodGuards*g,const FhInstallVertexSource*s){
    if(!g)return 0;fh_install_lod_guards_close(g);
    if(!s||!s->raw)return fail(g,"Missing source geometry for reduction guards.");g->source=s;
    g->morph=calloc(s->positions_count?s->positions_count:1,1);
    if(!s->world){g->positions=calloc(s->positions_count?s->positions_count:1,sizeof(*g->positions));g->normals=calloc(s->normals_count?s->normals_count:1,sizeof(*g->normals));}
    int ok=g->morph&&(s->world||(g->positions&&g->normals));
    if(!ok)fail(g,"Not enough startup memory for deformation guards.");
    else if(!s->world&&(!fh_install_skin_stream(s,0,g->positions,s->positions_count)||!fh_install_skin_stream(s,1,g->normals,s->normals_count)))ok=fail(g,"Invalid source CPU blend definitions.");
    if(ok)ok=morphs(g);
    if(!ok){char error[sizeof(g->error)];memcpy(error,g->error,sizeof(error));fh_install_lod_guards_close(g);memcpy(g->error,error,sizeof(error));}return ok;
}
static int skin_equal(const FhInstallLodGuards*g,const FhModelRef*a,const FhModelRef*b){
    if(g->source->world)return 1;
    if(memcmp(g->positions+a->position,g->positions+b->position,sizeof(*g->positions)))return 0;
    FhInstallSkinWeights empty={0};
    const FhInstallSkinWeights*x=a->normal==65535?&empty:g->normals+a->normal,*y=b->normal==65535?&empty:g->normals+b->normal;
    return !memcmp(x,y,sizeof(*x));
}
static unsigned alpha(const FhInstallVertexSource*s,const FhModelRef*r){return r->color==65535?15:half(s->colors+r->color*2)&15;}
static unsigned position_hash(const unsigned char*p){unsigned h=2166136261u;for(unsigned i=0;i<6;i++){h^=p[i];h*=16777619u;}return h;}
int fh_install_lod_locks(FhInstallLodGuards*g,const FhInstallDrawSource*d,const void*packets,unsigned vertices,unsigned stride,
                        const unsigned*indices,unsigned count,const unsigned char*extra,unsigned char*locks,unsigned char*required){
    if(!g||!g->source||!d||!packets||!vertices||vertices>32768||!indices||!count||count%3||count>FH_INSTALL_SURFACE_TRIANGLES*3||!locks||!required)return 0;
    memset(locks,0,vertices);memset(required,0,count/3);const FhInstallVertexSource*s=g->source;const unsigned char*p=packets;
    if(!stride||stride>32||stride!=d->layout[0]+d->layout[1]+d->layout[2]+d->layout[3]+d->layout[4]*d->layout[5])return fail(g,"Invalid reduction packet layout.");
    FhModelRef*refs=malloc(vertices*sizeof(*refs));unsigned *group=malloc(vertices*sizeof(*group)),*hash=NULL;unsigned char*mixed=calloc(vertices,1);unsigned hash_count=1;
    while(hash_count<vertices*2)hash_count*=2;hash=calloc(hash_count,sizeof(*hash));int ok=0;
    if(!refs||!group||!mixed||!hash){fail(g,"Not enough memory for reduction seam guards.");goto done;}
    for(unsigned i=0;i<vertices;i++){
        if(!fh_install_vertex_reference(s,d,p+i*stride,stride,refs+i)){fail(g,"Invalid reduction vertex reference.");goto done;}
        const unsigned char*position=s->positions+refs[i].position*6;unsigned at=position_hash(position)&(hash_count-1);
        while(hash[at]&&memcmp(position,s->positions+refs[hash[at]-1].position*6,6))at=(at+1)&(hash_count-1);
        if(!hash[at])hash[at]=i+1;unsigned first=hash[at]-1;group[i]=first;
        if(memcmp(p+i*stride,p+first*stride,d->layout[0])||!skin_equal(g,refs+i,refs+first))mixed[first]=1;
    }
    for(unsigned i=0;i<vertices;i++)if(mixed[group[i]])locks[i]=1;
    for(unsigned i=0;i<count;i+=3){
        unsigned a=indices[i],b=indices[i+1],c=indices[i+2];if(a>=vertices||b>=vertices||c>=vertices){fail(g,"Invalid reduction triangle index.");goto done;}
        unsigned morph=g->morph[refs[a].position]||g->morph[refs[b].position]||g->morph[refs[c].position];
        unsigned skin=!skin_equal(g,refs+a,refs+b)||!skin_equal(g,refs+a,refs+c);
        unsigned matrix=memcmp(p+a*stride,p+b*stride,d->layout[0])||memcmp(p+a*stride,p+c*stride,d->layout[0]);
        unsigned gradient=alpha(s,refs+a)!=alpha(s,refs+b)||alpha(s,refs+a)!=alpha(s,refs+c);
        required[i/3]=morph||skin||(extra&&extra[i/3]);
        if(required[i/3]||matrix||gradient)locks[a]=locks[b]=locks[c]=1;
    }ok=1;
done:
    free(refs);free(group);free(mixed);free(hash);
    if(!ok){memset(locks,0,vertices);memset(required,0,count/3);}return ok;
}
typedef struct{unsigned v[3];} Face;
static Face face(const unsigned*p){
    unsigned first=0;
    for(unsigned i=1;i<3;i++)for(unsigned j=0;j<3;j++){
        if(p[(i+j)%3]<p[(first+j)%3]){first=i;break;}if(p[(i+j)%3]>p[(first+j)%3])break;
    }
    return (Face){{p[first],p[(first+1)%3],p[(first+2)%3]}};
}
static int compare(const void*a,const void*b){const Face*x=a,*y=b;for(unsigned i=0;i<3;i++)if(x->v[i]!=y->v[i])return x->v[i]<y->v[i]?-1:1;return 0;}
int fh_install_lod_preserved(const unsigned*source,unsigned count,const unsigned char*required,const unsigned*candidate,unsigned candidate_count,unsigned vertices){
    if(!source||!required||!candidate||!count||!candidate_count||count%3||candidate_count%3||count>FH_INSTALL_SURFACE_TRIANGLES*3||candidate_count>count||!vertices||vertices>32768)return 0;
    unsigned n=0;for(unsigned i=0;i<count;i++){if(source[i]>=vertices)return 0;if(i%3==0&&required[i/3])n++;}
    for(unsigned i=0;i<candidate_count;i++)if(candidate[i]>=vertices)return 0;if(!n)return 1;
    Face*need=malloc(n*sizeof(*need)),*have=malloc(candidate_count/3*sizeof(*have));int ok=0;if(!need||!have){ok=-1;goto done;}
    unsigned k=0;for(unsigned i=0;i<count;i+=3)if(required[i/3])need[k++]=face(source+i);
    for(unsigned i=0;i<candidate_count;i+=3)have[i/3]=face(candidate+i);
    qsort(need,n,sizeof(*need),compare);qsort(have,candidate_count/3,sizeof(*have),compare);
    k=0;for(unsigned i=0;i<n;i++){
        while(k<candidate_count/3&&compare(have+k,need+i)<0)k++;
        if(k==candidate_count/3||compare(have+k,need+i))goto done;k++;
    }ok=1;
done:free(need);free(have);return ok;
}
