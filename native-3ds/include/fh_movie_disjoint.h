#ifndef FH_MOVIE_DISJOINT_H
#define FH_MOVIE_DISJOINT_H
#include "fh_graphics.h"
#include <math.h>
#include <string.h>

/* Conservative separation in the actual rotated PICA projection. The error
 * margin follows the pinned Melee bounds approach: propagate model rounding
 * before projection, rather than estimating from a cancelled final value.
 * Identical source endpoints have identical GPU positions in both passes;
 * their common edges retain the rasterizer's ordinary edge ownership. */
typedef struct {float x,y,ex,ey;} FhMoviePoint;
static inline int fh_movie_project(FhMoviePoint *out,const FhVertex *v,
                                   const FhDrawState *s,const float p[4][4]){
    float q[4]={0},error[4]={0};
    for(unsigned i=0;i<4;i++)if(!isfinite(v->position[i]) || fabsf(v->position[i])>1e12f)return 0;
    if(s->raw_position && v->matrix_index>=0){
        if(!isfinite(v->matrix_index) || v->matrix_index>27 || v->matrix_index!=(float)(unsigned)v->matrix_index || (unsigned)v->matrix_index%3)return 0;
        unsigned m=(unsigned)v->matrix_index;
        float eye[4]={0,0,0,v->position[3]},e[4]={0};
        for(unsigned r=0;r<3;r++){
            float magnitude=1;
            for(unsigned c=0;c<4;c++){
                float a=s->model_palette[m+r][c];if(!isfinite(a) || fabsf(a)>1e12f)return 0;
                float term=a*v->position[c];eye[r]+=term;magnitude+=fabsf(term);
            }
            e[r]=.002f*magnitude;
        }
        for(unsigned r=0;r<4;r++)if(r!=2){
            float magnitude=1,propagated=0;
            for(unsigned c=0;c<4;c++){
                if(!isfinite(p[r][c]) || fabsf(p[r][c])>1e12f)return 0;
                float term=p[r][c]*eye[c];q[r]+=term;magnitude+=fabsf(term);
                propagated+=fabsf(p[r][c])*e[c];
            }
            error[r]=.002f*magnitude+propagated;
        }
    }else{
        if(!isfinite(v->matrix_index))return 0;
        float sx=s->viewport[2]/640.f,sy=s->viewport[3]/FH_EFB_HEIGHT;
        float ox=(2*s->viewport[0]+s->viewport[2]-640)/640.f;
        float oy=(FH_EFB_HEIGHT-2*s->viewport[1]-s->viewport[3])/FH_EFB_HEIGHT;
        float w=v->position[3];
        q[0]=v->position[1]*sy+w*oy;q[1]=-(v->position[0]*sx+w*ox);q[3]=w;
        error[0]=.002f*(1+fabsf(v->position[1]*sy)+fabsf(w*oy));
        error[1]=.002f*(1+fabsf(v->position[0]*sx)+fabsf(w*ox));
        error[3]=.002f*(1+fabsf(w));
    }
    if(!isfinite(q[3]) || !isfinite(error[3]) || q[3]<=error[3]+1e-8f)return 0;
    out->x=q[0]/q[3];out->y=q[1]/q[3];
    out->ex=(error[0]+fabsf(out->x)*error[3])/(q[3]-error[3]);
    out->ey=(error[1]+fabsf(out->y)*error[3])/(q[3]-error[3]);
    return isfinite(out->x) && isfinite(out->y) && isfinite(out->ex) && isfinite(out->ey) &&
           fabsf(out->x)<1e6f && fabsf(out->y)<1e6f && out->ex<1e6f && out->ey<1e6f;
}
static inline int fh_movie_side(const FhMoviePoint *a,const FhMoviePoint *b,const FhMoviePoint *c){
    float x=b->x-a->x,y=b->y-a->y,u=c->x-a->x,v=c->y-a->y;
    float ex=b->ex+a->ex,ey=b->ey+a->ey,eu=c->ex+a->ex,ev=c->ey+a->ey;
    float first=x*v,second=y*u,value=first-second;
    float margin=fabsf(x)*ev+fabsf(v)*ex+ex*ev+fabsf(y)*eu+fabsf(u)*ey+ey*eu+
                 .00001f*(1+fabsf(first)+fabsf(second));
    if(!isfinite(value) || !isfinite(margin))return 0;
    return value>margin?1:value<-margin?-1:0;
}
static inline int fh_movie_same(const FhVertex *a,const FhVertex *b,unsigned raw){
    return !memcmp(a->position,b->position,sizeof(a->position)) &&
           (!raw || !memcmp(&a->matrix_index,&b->matrix_index,sizeof(a->matrix_index)));
}
static inline int fh_movie_separates(const FhVertex *a,const FhMoviePoint *p,
                                   const FhVertex *b,const FhMoviePoint *q,unsigned raw){
    for(unsigned edge=0;edge<3;edge++){
        unsigned next=(edge+1)%3,third=(edge+2)%3;
        int inside=fh_movie_side(p+edge,p+next,p+third);if(!inside)continue;
        unsigned accepted=1,strict=0;
        for(unsigned j=0;j<3;j++){
            if(fh_movie_same(a+edge,b+j,raw) || fh_movie_same(a+next,b+j,raw))continue;
            if(fh_movie_side(p+edge,p+next,q+j)!=-inside){accepted=0;break;}
            strict=1;
        }
        if(accepted && strict)return 1;
    }
    return 0;
}
static inline unsigned fh_movie_groups(unsigned groups[128],const FhVertex *v,unsigned count,const FhDrawState *s){
    if(!v || count<6 || count>384 || count%3 || s->raw_position>1)return 0;
    FhMoviePoint points[384];unsigned char valid[128];float p[4][4];fh_vertex_pica_projection(p,s);
    for(unsigned i=0;i<count;i+=3){
        valid[i/3]=fh_movie_project(points+i,v+i,s,p) && fh_movie_project(points+i+1,v+i+1,s,p) &&
                   fh_movie_project(points+i+2,v+i+2,s,p) && fh_movie_side(points+i,points+i+1,points+i+2);
    }
    unsigned n=0;
    for(unsigned first=0;first<count;){
        unsigned end=first+3;
        if(valid[first/3])for(;end<count;end+=3){
            if(!valid[end/3])break;
            unsigned safe=1;
            for(unsigned j=first;j<end;j+=3)
                if(!fh_movie_separates(v+end,points+end,v+j,points+j,s->raw_position) &&
                   !fh_movie_separates(v+j,points+j,v+end,points+end,s->raw_position)){safe=0;break;}
            if(!safe)break;
        }
        groups[n++]=end-first;first=end;
    }
    return n;
}
static inline int fh_movie_disjoint(const FhVertex *v,unsigned count,const FhDrawState *s){
    unsigned groups[128];return fh_movie_groups(groups,v,count,s)==1;
}
#endif
