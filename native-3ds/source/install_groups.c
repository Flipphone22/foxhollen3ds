#include "fh_install_groups.h"
#include "fh_install_triangles.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
enum {SLOTS=16384};
typedef struct {FhInstallGroup g;uint16_t lookup[SLOTS];} Work;
typedef struct {
    const FhInstallVertexSource*s;const FhInstallSkinWeights*positions,*normals;
    FhInstallGroupEmit emit;void*context;FhInstallGroupResult*result;
    Work piece,batch;FhInstallDrawSource draw;unsigned vat,flags;int status;
} Compiler;
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static int fail(Compiler*c,const char*why){snprintf(c->result->error,sizeof(c->result->error),"%s",why);c->status=0;return 0;}
static void reset(Work*w){/* Array entries are overwritten before their counted prefix is consumed. */memset(&w->g,0,offsetof(FhInstallGroup,ref));memset(w->lookup,255,sizeof(w->lookup));}
static unsigned bucket(const FhModelRef*r){const unsigned char*p=(const unsigned char*)r;unsigned h=2166136261u;for(unsigned i=0;i<sizeof(*r);i++)h=(h^p[i])*16777619u;return h&(SLOTS-1);}
static unsigned find(const Work*w,const FhModelRef*r){unsigned slot=bucket(r);while(w->lookup[slot]!=65535){if(!memcmp(w->g.ref+w->lookup[slot],r,sizeof(*r)))return w->lookup[slot];slot=(slot+1)&(SLOTS-1);}return 65535;}
static unsigned add(Work*w,const FhModelRef*r){unsigned slot=bucket(r);while(w->lookup[slot]!=65535){if(!memcmp(w->g.ref+w->lookup[slot],r,sizeof(*r)))return w->lookup[slot];slot=(slot+1)&(SLOTS-1);}unsigned id=w->g.vertices++;w->g.ref[id]=*r;w->lookup[slot]=id;return id;}
static unsigned union_bones(const uint16_t*a,unsigned na,const uint16_t*b,unsigned nb,uint16_t*out){
    unsigned n=na;memcpy(out,a,na*2);for(unsigned i=0;i<nb;i++){unsigned j=0;while(j<n&&out[j]!=b[i])j++;if(j==n)out[n++]=b[i];}return n;
}
static int publish(Compiler*c){
    FhInstallGroup*g=&c->batch.g;if(!g->indices)return 1;
    if(c->result->groups==256)return fail(c,"Native geometry exceeds256 groups.");
    for(unsigned i=1;i<g->bone_count;i++){uint16_t b=g->bones[i];unsigned j=i;while(j&&g->bones[j-1]>b){g->bones[j]=g->bones[j-1];j--;}g->bones[j]=b;}
    if(c->emit&&!c->emit(c->context,g)){c->status=-1;return 0;}
    c->result->groups++;c->result->vertices+=g->vertices;c->result->indices+=g->indices;reset(&c->batch);return 1;
}
static int finish_piece(Compiler*c){
    FhInstallGroup*p=&c->piece.g,*b=&c->batch.g;if(!p->indices)return 1;
    uint16_t merged[44];unsigned nb=union_bones(b->bones,b->bone_count,p->bones,p->bone_count,merged);
    if(b->indices&&(b->vat!=c->vat||b->flags!=c->flags||b->shader!=c->draw.shader||b->stream!=c->draw.stream||b->two_uv!=(c->draw.layout[5]==2)||
       b->vertices+p->vertices>8192||b->indices+p->indices>16384||nb>22||b->sections==256)){
        if(!publish(c))return 0;b=&c->batch.g;nb=union_bones(b->bones,0,p->bones,p->bone_count,merged);
    }
    if(c->result->draws==256)return fail(c,"Native geometry exceeds256 draw sections.");
    b->vat=c->vat;b->flags=c->flags;b->shader=c->draw.shader;b->stream=c->draw.stream;b->two_uv=c->draw.layout[5]==2;
    memcpy(b->bones,merged,nb*2);b->bone_count=nb;
    b->section[b->sections++]=(FhInstallSection){c->draw.list,c->draw.shader,c->draw.stream,c->draw.bit_offset,b->indices,p->indices,c->result->draws++};
    for(unsigned i=0;i<p->indices;i++)b->index[b->indices++]=add(&c->batch,p->ref+p->index[i]);
    reset(&c->piece);return 1;
}
static int triangle(void*context,const unsigned char*a,const unsigned char*b,const unsigned char*d,unsigned stride){
    Compiler*c=context;const unsigned char*corners[]={a,b,d};FhModelRef ref[3];uint16_t needed[6];unsigned nn=0;
    for(unsigned i=0;i<3;i++){
        if(!fh_install_vertex_reference(c->s,&c->draw,corners[i],stride,ref+i))return fail(c,"Invalid source vertex reference.");
        unsigned weights[4]={0,0,128,0};if(!c->s->world&&!fh_install_compact_weights(c->s,ref+i,c->positions,c->normals,weights))return fail(c,"Unrepresentable source skin weights or normals.");
        for(unsigned k=0;k<2;k++)if(weights[k+2]){unsigned j=0;while(j<nn&&needed[j]!=weights[k])j++;if(j==nn)needed[nn++]=weights[k];}
    }
    Work*w=&c->piece;uint16_t merged[28];unsigned nb=union_bones(w->g.bones,w->g.bone_count,needed,nn,merged),unique=0;
    for(unsigned i=0;i<3;i++)if(find(w,ref+i)==65535){unsigned j=0;while(j<i&&memcmp(ref+j,ref+i,sizeof(*ref)))j++;if(j==i)unique++;}
    unsigned limit=c->s->world?32766:16383;
    if(w->g.indices&&(w->g.indices+3>limit||w->g.vertices+unique>8192||nb>22)){
        if(c->s->world)return fail(c,"A world source list exceeds a native group; cannot split visibility bits.");
        if(!finish_piece(c))return 0;w=&c->piece;nb=union_bones(w->g.bones,0,needed,nn,merged);
    }
    if(nb>22)return fail(c,"Native triangle palette capacity exceeded.");
    memcpy(w->g.bones,merged,nb*2);w->g.bone_count=nb;
    for(unsigned i=0;i<3;i++){
        if(w->g.vertices==8192&&find(w,ref+i)==65535)return fail(c,"Native triangle vertex capacity exceeded.");
        w->g.index[w->g.indices++]=add(w,ref+i);
    }
    return 1;
}
static int draw(void*context,const FhInstallDrawSource*d){
    Compiler*c=context;c->draw=*d;const unsigned char*p=c->s->raw;
    const unsigned char*entry=p+word(p+(c->s->world?0x68:0xd0))+d->list*28;
    const unsigned char*shader=p+word(p+(c->s->world?0x64:0x38))+d->shader*68;
    c->flags=word(shader+0x3c);unsigned stride=d->layout[0]+d->layout[1]+d->layout[2]+d->layout[3]+d->layout[4]*d->layout[5];
    const unsigned char*data=p+word(entry);unsigned bytes=half(entry+4);FhInstallTriangles summary;
    if(fh_install_triangles(data,bytes,stride,NULL,NULL,&summary)!=1||!summary.triangles||
       (summary.vat!=3&&summary.vat!=5&&summary.vat!=6&&summary.vat!=7))return fail(c,"Invalid or empty source primitive list.");
    c->vat=summary.vat;c->result->source_draws++;
    if(fh_install_triangles(data,bytes,stride,triangle,c,&summary)!=1)return 0;
    return finish_piece(c);
}
int fh_install_groups(const FhInstallVertexSource*s,const FhInstallSkinWeights*positions,
    const FhInstallSkinWeights*normals,FhInstallGroupEmit emit,void*context,FhInstallGroupResult*result){
    if(!result)return 0;memset(result,0,sizeof(*result));if(!s||!s->raw)return 0;
    Compiler*c=calloc(1,sizeof(*c));if(!c){snprintf(result->error,sizeof(result->error),"Not enough startup memory for group scratch.");return 0;}
    c->s=s;c->positions=positions;c->normals=normals;c->emit=emit;c->context=context;c->result=result;c->status=1;reset(&c->piece);reset(&c->batch);
    unsigned count=0;int status=fh_install_descriptors(s->raw,s->bytes,s->world,draw,c,&count);
    if(status==0)fail(c,"Invalid source geometry description.");
    if(status==1&&c->status==1)publish(c);
    int answer=c->status;free(c);return answer;
}
