#include "fh_install_reduce.h"
#include "fh_install_surface.h"
#include "fh_install_triangles.h"
#include "fh_install_simplify.h"
#include "fh_install_list_pack.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    FhInstallDrawSource context[256];unsigned contexts,list,context_error;
    const FhInstallVertexSource*source;FhInstallLodGuards*guards;FhInstallReduction*out;
    unsigned char*packets,*locks,*required;FhModelRef*refs;
    unsigned *indices,*hash,*parent,*ordered,*counts,*offsets,*mapping;
    float *positions,*attributes;unsigned vertices,vertex_limit,indices_count,indices_limit,stride,hash_count,channels;
} Mesh;
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static int signed_half(const unsigned char*p){unsigned n=half(p);return n<32768?(int)n:(int)n-65536;}
static int fail(FhInstallReduction*out,const char*why){snprintf(out->message,sizeof(out->message),"%s",why);return FH_REDUCE_ERROR;}
void fh_install_reduce_close(FhInstallReduction*out){if(out){free(out->data);memset(out,0,sizeof(*out));}}
static int context(void*ctx,const FhInstallDrawSource*d){
    Mesh*m=ctx;if(d->list!=m->list)return 1;
    if(m->contexts==256){m->context_error=1;return 0;}
    m->context[m->contexts++]=*d;return 1;
}
static unsigned hash_bytes(unsigned h,const void*data,unsigned bytes){const unsigned char*p=data;for(unsigned i=0;i<bytes;i++){h^=p[i];h*=16777619u;}return h;}
static int triangle(void*ctx,const unsigned char*a,const unsigned char*b,const unsigned char*c,unsigned stride){
    Mesh*m=ctx;const unsigned char*v[]={a,b,c};
    if(stride!=m->stride||m->indices_count>m->indices_limit-3)return 0;
    for(unsigned i=0;i<3;i++){
        unsigned at=hash_bytes(2166136261u,v[i],stride)&(m->hash_count-1);
        while(m->hash[at]&&memcmp(m->packets+(m->hash[at]-1)*stride,v[i],stride))at=(at+1)&(m->hash_count-1);
        if(!m->hash[at]){
            if(m->vertices==m->vertex_limit)return 0;
            memcpy(m->packets+m->vertices*stride,v[i],stride);m->hash[at]=++m->vertices;
        }
        m->indices[m->indices_count++]=m->hash[at]-1;
    }return 1;
}
static unsigned root(Mesh*m,unsigned i){while(m->parent[i]!=i){m->parent[i]=m->parent[m->parent[i]];i=m->parent[i];}return i;}
static void join(Mesh*m,unsigned a,unsigned b){a=root(m,a);b=root(m,b);if(a!=b){if(a>b){unsigned n=a;a=b;b=n;}m->parent[b]=a;}}
static int same_deformation(const Mesh*m,unsigned a,unsigned b){
    unsigned matrices=m->context[0].layout[0];const FhModelRef*x=m->refs+a,*y=m->refs+b;
    if(memcmp(m->packets+a*m->stride,m->packets+b*m->stride,matrices))return 0;
    if(m->source->world)return 1;FhInstallSkinWeights empty={0};
    if(memcmp(m->guards->positions+x->position,m->guards->positions+y->position,sizeof(empty)))return 0;
    return !memcmp(x->normal==65535?&empty:m->guards->normals+x->normal,y->normal==65535?&empty:m->guards->normals+y->normal,sizeof(empty));
}
static unsigned deformation_hash(const Mesh*m,unsigned i){
    const FhModelRef*r=m->refs+i;unsigned h=hash_bytes(2166136261u,m->source->positions+r->position*6,6);
    h=hash_bytes(h,m->packets+i*m->stride,m->context[0].layout[0]);
    if(!m->source->world){FhInstallSkinWeights empty={0};h=hash_bytes(h,m->guards->positions+r->position,sizeof(empty));h=hash_bytes(h,r->normal==65535?&empty:m->guards->normals+r->normal,sizeof(empty));}
    return h;
}
static int attributes(Mesh*m,unsigned vat){
    const FhInstallVertexSource*s=m->source;const FhInstallDrawSource*d=m->context;unsigned normals=d->layout[2]?3:0,colors=d->layout[3]?4:0;
    m->channels=normals+colors+d->layout[5]*2;
    m->refs=malloc(m->vertices*sizeof(*m->refs));m->positions=malloc(m->vertices*3*sizeof(float));m->attributes=malloc((m->vertices*m->channels+1)*sizeof(float));
    if(!m->refs||!m->positions||!m->attributes)return fail(m->out,"Not enough memory for simplification attributes.");
    for(unsigned i=0;i<m->vertices;i++){
        FhModelRef*r=m->refs+i;if(!fh_install_vertex_reference(s,d,m->packets+i*m->stride,m->stride,r))return fail(m->out,"Invalid simplification source reference.");
        for(unsigned k=0;k<3;k++)m->positions[i*3+k]=signed_half(s->positions+r->position*6+k*2);
        unsigned at=i*m->channels;
        if(normals)for(unsigned k=0;k<3;k++){unsigned n=s->normals[r->normal*s->normal_stride+k];m->attributes[at++]=(float)((n<128?(int)n:(int)n-256)/127.);}
        if(colors){unsigned c=half(s->colors+r->color*2);for(unsigned k=0;k<4;k++)m->attributes[at++]=(float)(((c>>(12-k*4))&15)/15.);}
        for(unsigned layer=0;layer<d->layout[5];layer++)for(unsigned k=0;k<2;k++)m->attributes[at++]=(float)(signed_half(s->uvs+(layer?r->material:r->uv)*4+k*2)/(vat==5?256.:1024.));
    }
    /* Same visual metric as the host recipe; scale disposable copies only. */
    double scales[11];for(unsigned k=0;k<normals+colors;k++)scales[k]=20;
    for(unsigned layer=0;layer<d->layout[5];layer++){
        unsigned at=normals+colors+layer*2;double area=0;
        for(unsigned i=0;i<m->indices_count;i+=3){
            const float*a=m->attributes+m->indices[i]*m->channels+at,*b=m->attributes+m->indices[i+1]*m->channels+at,*c=m->attributes+m->indices[i+2]*m->channels+at;
            area+=fabs(((double)b[0]-a[0])*((double)c[1]-a[1])-((double)b[1]-a[1])*((double)c[0]-a[0]))*.5;
        }
        double density=area/(m->indices_count/3),weight=density?fmin(100,fmax(10,1/sqrt(density))):100;scales[at]=scales[at+1]=s->world?1:weight/.05;
    }
    /* Original world recipe retains its unscaled .05 metrics. */
    if(!s->world)for(unsigned i=0;i<m->vertices*m->channels;i++)m->attributes[i]=(float)(m->attributes[i]*scales[i%m->channels]);
    return 1;
}
static int components(Mesh*m){
    unsigned n=m->vertices;
    m->parent=malloc(n*4);m->counts=calloc(n,4);m->offsets=malloc((n+1)*4);m->ordered=malloc(m->indices_count*4);m->mapping=malloc(n*4);
    if(!m->parent||!m->counts||!m->offsets||!m->ordered||!m->mapping)return fail(m->out,"Not enough memory for connected source features.");
    memset(m->hash,0,m->hash_count*4);
    for(unsigned i=0;i<n;i++)m->parent[i]=i;
    for(unsigned i=0;i<n;i++){
        unsigned at=deformation_hash(m,i)&(m->hash_count-1);
        while(m->hash[at]){
            unsigned other=m->hash[at]-1;
            if(!memcmp(m->source->positions+m->refs[i].position*6,m->source->positions+m->refs[other].position*6,6)&&same_deformation(m,i,other))break;
            at=(at+1)&(m->hash_count-1);
        }
        if(m->hash[at])join(m,i,m->hash[at]-1);else m->hash[at]=i+1;
    }
    for(unsigned i=0;i<m->indices_count;i+=3){join(m,m->indices[i],m->indices[i+1]);join(m,m->indices[i],m->indices[i+2]);}
    for(unsigned i=0;i<m->indices_count;i+=3)m->counts[root(m,m->indices[i])]+=3;
    unsigned offset=0;for(unsigned i=0;i<n;i++){m->offsets[i]=offset;offset+=m->counts[i];m->counts[i]=0;m->mapping[i]=~0u;}m->offsets[n]=offset;
    for(unsigned i=0;i<m->indices_count;i+=3){unsigned r=root(m,m->indices[i]),at=m->offsets[r]+m->counts[r];memcpy(m->ordered+at,m->indices+i,12);m->counts[r]+=3;}
    return 1;
}
static int simplify_components(Mesh*m,float ratio,float error,unsigned**result,unsigned*result_count){
    unsigned n=m->vertices;float*positions=malloc(n*12),*attrs=malloc((n*m->channels+1)*4);unsigned char*locks=malloc(n);
    unsigned *indices=malloc(m->indices_count*4),*reverse=malloc(n*4),*output=malloc(m->indices_count*4);int ok=0;
    if(!positions||!attrs||!locks||!indices||!reverse||!output){fail(m->out,"Not enough memory to simplify connected features.");goto done;}
    unsigned written=0;
    for(unsigned component=0;component<n;component++)if(m->counts[component]){
        unsigned count=m->counts[component],*source=m->ordered+m->offsets[component];m->out->components++;
        if(count<72){memcpy(output+written,source,count*4);written+=count;continue;}
        unsigned vertices=0;
        for(unsigned i=0;i<count;i++){
            unsigned id=source[i];if(m->mapping[id]==~0u){
                unsigned at=vertices++;m->mapping[id]=at;reverse[at]=id;memcpy(positions+at*3,m->positions+id*3,12);
                memcpy(attrs+at*m->channels,m->attributes+id*m->channels,m->channels*4);locks[at]=m->locks[id];
            }indices[i]=m->mapping[id];
        }
        FhInstallSimplification candidate={0};float weights[11];for(unsigned i=0;i<m->channels;i++)weights[i]=.05f;
        if(!fh_install_simplify(&candidate,indices,count,positions,vertices,attrs,m->channels,weights,locks,ratio,error,1,FH_INSTALL_SIMPLIFY_SCRATCH)){
            fail(m->out,candidate.message);fh_install_simplify_close(&candidate);goto done;
        }
        if(candidate.scratch_peak>m->out->simplify_scratch)m->out->simplify_scratch=candidate.scratch_peak;
        if(!candidate.count||candidate.count>=count){memcpy(output+written,source,count*4);written+=count;}
        else for(unsigned i=0;i<candidate.count;i++)output[written++]=reverse[candidate.indices[i]];
        fh_install_simplify_close(&candidate);for(unsigned i=0;i<vertices;i++)m->mapping[reverse[i]]=~0u;
    }
    *result=output;*result_count=written;output=NULL;ok=1;
done:free(positions);free(attrs);free(locks);free(indices);free(reverse);free(output);return ok;
}
static void close_mesh(Mesh*m){
    free(m->packets);free(m->indices);free(m->hash);free(m->refs);free(m->positions);free(m->attributes);free(m->locks);free(m->required);
    free(m->parent);free(m->ordered);free(m->counts);free(m->offsets);free(m->mapping);free(m);
}
int fh_install_reduce_list(FhInstallReduction*out,FhInstallLodGuards*g,unsigned list,float ratio,float error){
    if(!out)return FH_REDUCE_ERROR;fh_install_reduce_close(out);
    if(!g||!g->source||!g->morph||!isfinite(ratio)||ratio<=0||ratio>=1||!isfinite(error)||error<=0||error>.1f)return fail(out,"Invalid source reduction request.");
    const FhInstallVertexSource*s=g->source;Mesh*m=calloc(1,sizeof(*m));if(!m)return fail(out,"Not enough memory for source reduction contexts.");
    m->source=s;m->guards=g;m->out=out;m->list=list;int result=FH_REDUCE_ERROR;unsigned*candidate=NULL,candidate_count=0,draws;
    if(fh_install_descriptors(s->raw,s->bytes,s->world,context,m,&draws)!=1||m->context_error||!m->contexts){fail(out,"Missing or invalid source list contexts.");goto done;}
    out->contexts=m->contexts;
    for(unsigned i=1;i<m->contexts;i++)if(memcmp(m->context[0].layout,m->context[i].layout,sizeof(m->context[0].layout))){out->reason=FH_REDUCE_CONTEXT;result=FH_REDUCE_KEEP;goto done;}
    const unsigned*l=m->context[0].layout;unsigned table=word(s->raw+(s->world?0x68:0xd0));const unsigned char*entry=s->raw+table+list*28;
    const unsigned char*data=s->raw+word(entry);unsigned bytes=half(entry+4);m->stride=l[0]+l[1]+l[2]+l[3]+l[4]*l[5];FhInstallTriangles info;
    if(fh_install_triangles(data,bytes,m->stride,NULL,NULL,&info)!=1||info.triangles>FH_INSTALL_SURFACE_TRIANGLES){fail(out,"Invalid source triangle list for reduction.");goto done;}
    out->before=out->after=info.triangles;
    if(info.triangles<24){out->reason=FH_REDUCE_SMALL;result=FH_REDUCE_KEEP;goto done;}
    m->vertex_limit=bytes/m->stride;if(m->vertex_limit>32768)m->vertex_limit=32768;m->indices_limit=info.triangles*3;
    m->hash_count=1;while(m->hash_count<m->vertex_limit*2)m->hash_count*=2;
    m->packets=malloc(m->vertex_limit*m->stride);m->indices=malloc(m->indices_limit*4);m->hash=calloc(m->hash_count,4);
    if(!m->packets||!m->indices||!m->hash){fail(out,"Not enough memory for original reduction packets.");goto done;}
    if(fh_install_triangles(data,bytes,m->stride,triangle,m,&info)!=1||m->indices_count!=m->indices_limit){fail(out,"Cannot collect immutable source packets.");goto done;}
    m->locks=malloc(m->vertices);m->required=malloc(info.triangles);
    if(!m->locks||!m->required){fail(out,"Not enough memory for reduction locks.");goto done;}
    if(!fh_install_lod_locks(g,m->context,m->packets,m->vertices,m->stride,m->indices,m->indices_count,NULL,m->locks,m->required)){fail(out,g->error);goto done;}
    if(!attributes(m,info.vat)||!components(m))goto done;
    /* An aggressive candidate failing fidelity need not discard all potential
     * savings. Tighten the simplifier, never the acceptance tolerances. Source
     * parsing, immutable packets, components and metric arrays are reused. */
    float current_error=error;
retry:
    free(candidate);candidate=NULL;free(out->data);out->data=NULL;out->bytes=0;
    out->position_error=out->uv_error=out->color_error=0;out->components=0;out->attempts++;
    if(!simplify_components(m,ratio,current_error,&candidate,&candidate_count))goto done;
    if(candidate_count>=m->indices_count){out->reason=FH_REDUCE_NO_GAIN;result=FH_REDUCE_KEEP;goto done;}
    int protected=fh_install_lod_preserved(m->indices,m->indices_count,m->required,candidate,candidate_count,m->vertices);
    if(protected<0){fail(out,"Not enough memory to verify protected source faces.");goto done;}
    if(!protected){out->reason=FH_REDUCE_PROTECTED;result=FH_REDUCE_KEEP;goto refine;}
    out->data=malloc(bytes);if(!out->data){fail(out,"Not enough memory for reduced source list.");goto done;}
    if(!fh_install_list_pack(out->data,bytes,candidate,candidate_count,m->packets,m->vertices,m->stride,info.vat,&out->bytes)){out->reason=FH_REDUCE_CAPACITY;result=FH_REDUCE_KEEP;goto refine;}
    /* Release candidate indices before holding comparison surfaces; retain
     * parsed metric arrays for a tighter retry. The simplifier arena is gone. */
    free(candidate);candidate=NULL;
    FhInstallFidelityLimits limits={s->world?1:.5,1./128,1./32,.9396926208};
    for(unsigned i=0;i<m->contexts;i++){
        FhInstallSurfaceOwner original={0},reduced={0};FhInstallFidelity check;
        if(!fh_install_surface(&original,s,m->context+i,data,bytes)||!fh_install_surface(&reduced,s,m->context+i,out->data,out->bytes)){
            fail(out,original.error[0]?original.error:reduced.error);fh_install_surface_close(&original);fh_install_surface_close(&reduced);goto done;
        }
        int accepted=fh_install_fidelity(&original.surface,&reduced.surface,&limits,&check);
        fh_install_surface_close(&original);fh_install_surface_close(&reduced);
        out->position_error=fmax(out->position_error,check.position);out->uv_error=fmax(out->uv_error,check.uv);out->color_error=fmax(out->color_error,check.color);
        if(!accepted){
            if(check.reason==FH_INSTALL_FIDELITY_MEMORY||check.reason==FH_INSTALL_FIDELITY_INVALID){fail(out,"Native surface verification could not complete.");goto done;}
            out->reason=FH_REDUCE_SURFACE;out->fidelity_rejected++;result=FH_REDUCE_KEEP;goto refine;
        }
    }
    out->after=candidate_count/3;out->accepted_error=current_error;result=FH_REDUCE_CHANGED;goto done;
refine:
    if(out->attempts<5){current_error*=.25f;result=FH_REDUCE_ERROR;goto retry;}
done:
    free(candidate);close_mesh(m);if(result!=FH_REDUCE_CHANGED){free(out->data);out->data=NULL;out->bytes=0;}return result;
}
