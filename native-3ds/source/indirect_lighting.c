#include "fh_indirect_lighting.h"
#include "fh_fragment.h"
#include <math.h>
#include <string.h>

/* Opt-in until captured-image and integrated validation is complete. */
unsigned fh_indirect_lighting_enabled,fh_indirect_lighting_triangles;
unsigned fh_indirect_lighting_detail;
int fh_indirect_lighting_prepare(FhIndirectLighting *p,const FhDrawState *s){
    if(!s->indirect_count || s->texgens>8 || !fh_fragment_supported(s))return 0;
    memset(p,0,sizeof(*p));
    for(unsigned i=0;i<8;i++)p->scale[i][0]=p->scale[i][1]=1;
    for(unsigned i=0;i<16;i++)p->destination[i]=-1;
    for(unsigned i=0;i<3;i++){
        if(s->indirect_exponent[i]<-17 || s->indirect_exponent[i]>17)return 0;
        p->exponent[i]=ldexpf(1,s->indirect_exponent[i]);
        for(unsigned a=0;a<2;a++)for(unsigned b=0;b<3;b++)
            if(!isfinite(s->indirect_matrix[i][a][b]) || fabsf(s->indirect_matrix[i][a][b])>1)return 0;
    }
    for(unsigned i=0;i<s->indirect_count;i++){
        unsigned c=s->indirect_order[i][0],m=s->indirect_order[i][1];
        p->uv_mask|=1u<<c;p->scale[c][0]=s->textures[m].width;p->scale[c][1]=s->textures[m].height;
        for(unsigned j=0;j<2;j++){
            if(s->indirect_scale[i][j]>8)return 0;
            p->divisor[i][j]=(float)(1u<<s->indirect_scale[i][j]);
        }
    }
    unsigned reserved=0,changed=0;
    for(unsigned i=0;i<s->stages;i++){
        const FhTevStage *t=s->tev+i;unsigned c=t->coord,m=t->map&~256u;
        /* Repeating a wrapped offset across a triangle introduces a seam.
         * Bump alpha needs a separate raster channel, not a UV replacement. */
        if((t->indirect[4] && t->indirect[4]!=6) || (t->indirect[5] && t->indirect[5]!=6) ||
           ((t->channel==7 || t->channel==8) && t->indirect[8]))return 0;
        if(c<8){p->uv_mask|=1u<<c;if(m<8){
            if(!s->textures[m].width || !s->textures[m].height)return 0;
            p->scale[c][0]=s->textures[m].width;p->scale[c][1]=s->textures[m].height;
        }}
        unsigned selection=t->indirect[3],used=selection>=5;
        if(selection && selection<4)for(unsigned a=0;a<2;a++)for(unsigned b=0;b<3;b++)
            used|=s->indirect_matrix[selection-1][a][b]!=0;
        if(used && t->indirect[0]<s->indirect_count)p->indirect_mask|=1u<<t->indirect[0];
        unsigned indirect=selection || t->indirect[4] || t->indirect[5] || t->indirect[6];
        if(c<8 && t->map<8){
            if(indirect){p->destination[i]=-2;changed++;}
            else reserved|=1u<<c;
        }
    }
    if(!changed)return 0;
    for(unsigned i=0;i<8;i++)if(s->textures[i].width>1024 || s->textures[i].height>1024)return 0;
    for(unsigned i=0;i<s->stages;i++)if(p->destination[i]==-2){
        int slot=7;while(slot>=0 && (reserved&(1u<<slot)))slot--;
        if(slot<0)return 0;
        p->destination[i]=slot;reserved|=1u<<slot;
    }
    return 1;
}
int fh_indirect_lighting_plan(FhMaterial *plan,FhDrawState *draw,const FhDrawState *s){
    FhIndirectLighting p;if(!fh_indirect_lighting_prepare(&p,s))return 0;
    *draw=*s;draw->indirect_count=0;draw->texgens=8;
    for(unsigned i=0;i<s->stages;i++){
        memset(draw->tev[i].indirect,0,sizeof(draw->tev[i].indirect));
        if(p.destination[i]>=0)draw->tev[i].coord=(unsigned)p.destination[i];
    }
    /* This audited SFA family keeps raster alpha in an earlier GX register.
     * Its existing two-stage PICA translation handles that register lifetime;
     * the generic compiler cannot. Only replace its dependent coordinate. */
    if(fh_chained_material(s)){
        fh_chained_pipeline(plan,s);plan->pipeline.coord[0]=p.destination[1];return 1;
    }
    return fh_pipeline_compile(plan,draw);
}
int fh_indirect_lighting_vertex_prepared(FhVertex *out,const FhVertex *in,const FhDrawState *s,const FhIndirectLighting *p){
    float uv[8][2]={{0}},ind[4][3]={{0}},prior[2]={0};
    for(unsigned i=0;i<8;i++)if(p->uv_mask&(1u<<i)){
        float q=in->uv[i][2];if(!isfinite(q) || fabsf(q)<1e-20f)return 0;
        for(unsigned j=0;j<2;j++){uv[i][j]=in->uv[i][j]/q;if(!isfinite(uv[i][j]) || fabsf(uv[i][j])>1e12f)return 0;}
    }
    for(unsigned i=0;i<s->indirect_count;i++)if(p->indirect_mask&(1u<<i)){
        unsigned c=s->indirect_order[i][0],m=s->indirect_order[i][1];float sample[4];
        fh_texture_sample(sample,&s->textures[m],uv[c][0]*p->scale[c][0]/s->textures[m].width/p->divisor[i][0],
                          uv[c][1]*p->scale[c][1]/s->textures[m].height/p->divisor[i][1]);
        for(unsigned j=0;j<3;j++)ind[i][j]=roundf(sample[3-j]*255);
    }
    *out=*in;
    for(unsigned i=0;i<s->stages;i++){
        const FhTevStage *t=s->tev+i;unsigned c=t->coord;int base=c<8;
        float coord[2]={base?uv[c][0]:0,base?uv[c][1]:0};
        unsigned selection=t->indirect[3],indirect=selection || t->indirect[4] || t->indirect[5] || t->indirect[6];
        if(indirect){
            if(base)for(unsigned j=0;j<2;j++)coord[j]*=p->scale[c][j];
            for(unsigned j=0;j<2;j++)if(t->indirect[4+j])coord[j]=0;
            if(selection && t->indirect[0]<s->indirect_count){
                float delta[3];unsigned format=t->indirect[1],matrix=(selection-1)&3;
                for(unsigned j=0;j<3;j++)delta[j]=floorf(ind[t->indirect[0]][j]/(format?1u<<(format+2):1))+
                    (t->indirect[2]&(1u<<j)?format?1:-128:0);
                if(selection<4){
                    for(unsigned j=0;j<2;j++)for(unsigned k=0;k<3;k++)coord[j]+=delta[k]*s->indirect_matrix[matrix][j][k]*p->exponent[matrix];
                }else if(base){
                    float component=delta[selection<8?0:1];
                    for(unsigned j=0;j<2;j++)coord[j]+=uv[c][j]*p->scale[c][j]*component*p->exponent[matrix]/256.f;
                }
            }
            for(unsigned j=0;j<2;j++){if(t->indirect[6])coord[j]+=prior[j];prior[j]=coord[j];}
        }else if(base)for(unsigned j=0;j<2;j++)prior[j]=coord[j]*p->scale[c][j];
        if(p->destination[i]>=0){
            unsigned dest=(unsigned)p->destination[i];const FhTexture *tmap=&s->textures[t->map];
            out->uv[dest][0]=coord[0]/tmap->width;out->uv[dest][1]=coord[1]/tmap->height;out->uv[dest][2]=1;
            if(!isfinite(out->uv[dest][0]) || !isfinite(out->uv[dest][1]))return 0;
        }
    }
    return 1;
}
int fh_indirect_lighting_vertex(FhVertex *out,const FhVertex *in,const FhDrawState *s){
    FhIndirectLighting p;return fh_indirect_lighting_prepare(&p,s) && fh_indirect_lighting_vertex_prepared(out,in,s,&p);
}
static void midpoint(FhVertex *out,const FhVertex *a,const FhVertex *b){
    for(unsigned i=0;i<36;i++)((float*)out)[i]=.5f*(((const float*)a)[i]+((const float*)b)[i]);
    out->matrix_index=-1;
}
static int mesh(const FhVertex v[3],const FhDrawState *s,const FhIndirectLighting *p,unsigned level,FhTriangleSink sink,void *context){
    float edge=0;
    for(unsigned i=0;i<3;i++){
        const FhVertex *a=v+i,*b=v+(i+1)%3;
        float dx=(a->position[0]/a->position[3]-b->position[0]/b->position[3])*200;
        float dy=(a->position[1]/a->position[3]-b->position[1]/b->position[3])*120;
        edge=fmaxf(edge,dx*dx+dy*dy);
    }
    if(level<fh_indirect_lighting_detail && level<4 && edge>16*16){
        FhVertex m[3],child[3];for(unsigned i=0;i<3;i++)midpoint(m+i,v+i,v+(i+1)%3);
        for(unsigned part=0;part<4;part++){
            if(part<3){child[0]=v[part];child[1]=m[part];child[2]=m[(part+2)%3];}
            else memcpy(child,m,sizeof(child));
            if(!mesh(child,s,p,level+1,sink,context))return 0;
        }
    }else{
        FhVertex out[3];for(unsigned i=0;i<3;i++)if(!fh_indirect_lighting_vertex_prepared(out+i,v+i,s,p))return 0;
        sink(out,context);fh_indirect_lighting_triangles++;
    }
    return 1;
}
int fh_indirect_lighting_mesh_prepared(const FhVertex v[3],const FhDrawState *s,const FhIndirectLighting *p,FhTriangleSink sink,void *context){
    /* Same-sign projective denominators stay away from zero in children.
     * Reject before emitting anything; an incomplete triangle is never drawn. */
    for(unsigned i=0;i<3;i++){
        if(v[i].matrix_index>=0 || !(v[i].position[3]>0))return 0;
        for(unsigned c=0;c<36;c++)if(!isfinite(((const float*)(v+i))[c]) || fabsf(((const float*)(v+i))[c])>1e12f)return 0;
        for(unsigned c=0;c<8;c++)if((p->uv_mask&(1u<<c)) && (fabsf(v[i].uv[c][2])<1e-10f || v[i].uv[c][2]*v[0].uv[c][2]<=0))return 0;
        FhVertex check;if(!fh_indirect_lighting_vertex_prepared(&check,v+i,s,p))return 0;
    }
    return mesh(v,s,p,0,sink,context);
}
int fh_indirect_lighting_mesh(const FhVertex v[3],const FhDrawState *s,FhTriangleSink sink,void *context){
    FhIndirectLighting p;return fh_indirect_lighting_prepare(&p,s) && fh_indirect_lighting_mesh_prepared(v,s,&p,sink,context);
}
