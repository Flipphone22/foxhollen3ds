#include "fh_pica_shadow_points.h"
#include <string.h>
_Static_assert(sizeof(FhShadowPoint)==32,"one compact receiver record per triangle");

static unsigned surface(unsigned flags){
    if(!(flags&4) || ((flags&8)?(flags&1):(flags&2)))return 0;
    return ((flags&8)?20:2)|8|((flags&32)?64:0);
}
static int cell_range(unsigned byte,int16_t *lo,int16_t *hi){
    if(!byte)return 0;
    unsigned first=0,last=0;
    while(!(byte&(1u<<first)))first++;
    unsigned run=byte>>first;
    if(run&(run+1))return 0;
    last=first;
    while(run>>=1)last++;
    *lo=(int16_t)(first*80);*hi=(int16_t)((last+1)*80);return 1;
}
static void positions(int16_t out[9],const FhShadowTrackLiveRef*r,const int16_t(*p)[3]){
    for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++){
        int x=p[r->source[j]][k];out[j*3+k]=(int16_t)(x>=0?x/8:-((-x+7)/8));
    }
}
int fh_pica_shadow_points_compile(const FhShadowTrackSource*s,FhShadowPoint*v,unsigned vc,
    FhShadowTrackDraw*d,unsigned dc,FhShadowTrackLiveRef*refs,unsigned rc,FhShadowPointSize*out){
    if(!s || !out || !s->position || !s->triangle || !s->group || s->vertices>65535 ||
       s->triangles>65535 || s->groups>65535 ||
       (v?(!d || !refs):(d!=NULL || refs!=NULL)))return 0;
    if(s->group[0].first || s->group[s->groups].first!=s->triangles)return 0;
    for(unsigned g=0;g<s->groups;g++){
        const FhShadowTrackGroup*p=s->group+g;
        if(p->first>s->group[g+1].first || s->group[g+1].first>s->triangles ||
           p->min_x>p->max_x || p->min_y>p->max_y || p->min_z>p->max_z)return 0;
    }
    for(unsigned t=0;t<s->triangles;t++)for(unsigned k=0;k<3;k++)if(s->triangle[t][k]>=s->vertices)return 0;
    FhShadowPointSize size={0};
    for(unsigned pass=0;pass<(v?2u:1u);pass++){
        unsigned count=0,draws=0;
        for(unsigned g=0;g<s->groups;g++){
            const FhShadowTrackGroup*p=s->group+g;unsigned type=surface(p->flags);
            if(!type || p->min_x>640 || p->max_x<0 || p->min_z>640 || p->max_z<0)continue;
            unsigned first=count;
            FhShadowTrackDraw group={.first=first,.source_group=g,.flags=type};
            for(unsigned t=p->first;t<s->group[g+1].first;t++){
                unsigned mask=s->triangle[t][3];if(!(mask&255) || !(mask>>8))continue;
                FhShadowPoint point={0};
                if(!cell_range(mask&255,point.lo,point.hi) ||
                   !cell_range(mask>>8,point.lo+2,point.hi+2))return 0;
                if(point.lo[0]<p->min_x)point.lo[0]=p->min_x;
                if(point.hi[0]>p->max_x)point.hi[0]=p->max_x;
                if(point.lo[2]<p->min_z)point.lo[2]=p->min_z;
                if(point.hi[2]>p->max_z)point.hi[2]=p->max_z;
                point.lo[1]=p->min_y;point.hi[1]=p->max_y;
                if(pass){
                    FhShadowTrackLiveRef ref={{s->triangle[t][0],s->triangle[t][1],s->triangle[t][2]}};
                    int16_t xyz[9];positions(xyz,&ref,s->position);memcpy(&point,xyz,sizeof(xyz));
                    if(count==first)for(unsigned k=0;k<3;k++)group.lo[k]=group.hi[k]=xyz[k];
                    for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++){
                        if(xyz[j*3+k]<group.lo[k])group.lo[k]=xyz[j*3+k];
                        if(xyz[j*3+k]>group.hi[k])group.hi[k]=xyz[j*3+k];
                    }
                    refs[count]=ref;v[count]=point;
                }
                count++;
            }
            if(count!=first){if(pass){group.count=count-first;d[draws]=group;}draws++;}
        }
        if(!pass){
            size=(FhShadowPointSize){count,draws,count*sizeof(FhShadowPoint),count*sizeof(FhShadowTrackLiveRef),draws*sizeof(FhShadowTrackDraw)};
            if(v && (vc<count || dc<draws || rc<count))return 0;
        }
    }
    *out=size;return 1;
}
unsigned fh_pica_shadow_points_update(FhShadowPoint*v,const FhShadowTrackLiveRef*r,unsigned count,
    const int16_t(*p)[3],unsigned*first,unsigned*last){
    unsigned changed=0;*first=*last=0;
    for(unsigned i=0;i<count;i++){
        int16_t xyz[9];positions(xyz,r+i,p);
        if(!memcmp(v+i,xyz,sizeof(xyz)))continue;
        if(!changed)*first=i*sizeof(*v);
        memcpy(v+i,xyz,sizeof(xyz));changed++;*last=(i+1)*sizeof(*v);
    }
    return changed;
}
