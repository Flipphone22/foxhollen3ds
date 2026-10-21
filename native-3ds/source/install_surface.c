#include "fh_install_surface.h"
#include "fh_install_skin.h"
#include "fh_install_triangles.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    const FhInstallVertexSource*source;const FhInstallDrawSource*draw;
    FhInstallSkinWeights*skin;FhInstallSurfaceVertex*vertex;unsigned count,limit,vat;
} Builder;
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static int signed_half(const unsigned char*p){unsigned n=half(p);return n<32768?(int)n:(int)n-65536;}
static double real(const unsigned char*p){uint32_t n=(unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];float v;memcpy(&v,&n,4);return v;}
void fh_install_surface_close(FhInstallSurfaceOwner*s){if(s){free(s->allocation);memset(s,0,sizeof(*s));}}
static int error(FhInstallSurfaceOwner*s,const char*why){fh_install_surface_close(s);snprintf(s->error,sizeof(s->error),"%s",why);return 0;}
static int triangle(void*context,const unsigned char*a,const unsigned char*b,const unsigned char*c,unsigned stride){
    Builder*t=context;const FhInstallVertexSource*s=t->source;const unsigned char*packets[]={a,b,c};
    if(t->count==t->limit)return 0;
    for(unsigned i=0;i<3;i++){
        FhModelRef ref;if(!fh_install_vertex_reference(s,t->draw,packets[i],stride,&ref))return 0;
        FhInstallSurfaceVertex*v=t->vertex+t->count*3+i;double scale=t->vat==7?1:t->vat==5?1./8:1./256;
        for(unsigned k=0;k<3;k++)v->position[k]=signed_half(s->positions+ref.position*6+k*2)*scale;
        if(s->fixed_world&&ref.joint==s->joints_count){
            const FhInstallSkinWeights*w=t->skin+ref.position;
            if(w->present)for(unsigned k=0;k<3;k++)v->position[k]*=(w->weight[0]+w->weight[1])/128.;
        }else if(!s->rigid){
            FhModelBinding binding;if(!fh_install_vertex_binding(s,ref.joint,&binding))return 0;
            if(!binding.bind_space){
                if(binding.a>=s->joints_count)return 0;
                for(unsigned k=0;k<3;k++)v->position[k]+=real(s->joints+binding.a*28+16+k*4);
            }
        }
        for(unsigned k=0;k<3;k++)if(!isfinite(v->position[k])||fabs(v->position[k])>1e12)return 0;
        for(unsigned layer=0;layer<t->draw->layout[5];layer++){
            unsigned index=layer?ref.material:ref.uv;if(index>=s->uvs_count)return 0;
            for(unsigned k=0;k<2;k++)v->uv[layer*2+k]=signed_half(s->uvs+index*4+k*2)/(t->vat==5?256.:1024.);
        }
        unsigned color=ref.color==65535?65535:half(s->colors+ref.color*2);
        for(unsigned k=0;k<4;k++)v->rgba[k]=(double)((color>>(12-k*4))&15)/15;
    }
    t->count++;return 1;
}
int fh_install_surface(FhInstallSurfaceOwner*out,const FhInstallVertexSource*s,const FhInstallDrawSource*d,const void*list,unsigned bytes){
    if(!out)return 0;fh_install_surface_close(out);
    if(!s||!s->raw||!d||d->layout[5]>2)return error(out,"Invalid source surface context.");
    unsigned stride=d->layout[0]+d->layout[1]+d->layout[2]+d->layout[3]+d->layout[4]*d->layout[5];FhInstallTriangles info;
    if(fh_install_triangles(list,bytes,stride,NULL,NULL,&info)!=1||!info.triangles||info.triangles>FH_INSTALL_SURFACE_TRIANGLES)return error(out,"Invalid or oversized source surface list.");
    FhInstallSurfaceVertex*v=calloc(info.triangles*3,sizeof(*v));if(!v)return error(out,"Not enough startup memory for material surface.");out->allocation=v;
    FhInstallSkinWeights*skin=NULL;
    if(s->fixed_world&&s->positions_count){
        skin=calloc(s->positions_count,sizeof(*skin));
        if(!skin){error(out,"Not enough memory for source surface bindings.");return 0;}
        if(!fh_install_skin_stream(s,0,skin,s->positions_count)){free(skin);return error(out,"Invalid CPU-stream surface bindings.");}
    }
    Builder b={s,d,skin,v,0,info.triangles,info.vat};
    int ok=fh_install_triangles(list,bytes,stride,triangle,&b,&info)==1&&b.count==b.limit;free(skin);
    if(!ok)return error(out,"Cannot resolve original surface attributes or bindings.");
    out->surface=(FhInstallSurface){v,b.count,d->layout[5]*2};return 1;
}
