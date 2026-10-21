#include "fh_pica_shadow_track.h"
#include <string.h>
_Static_assert(sizeof(FhShadowTrackLiveVertex)==20,"live receiver attributes");
_Static_assert(sizeof(FhShadowTrackLiveRef)==6,"live receiver source bindings");
static unsigned surface(unsigned flags){
    if(!(flags&4) || ((flags&8)?(flags&1):(flags&2)))return 0;
    return ((flags&8)?20:2)|8|((flags&32)?64:0);
}
static void quantize(int16_t out[3],const int16_t in[3],int16_t y){
    for(unsigned k=0;k<3;k++){
        int v=in[k],q=v>=0?v/8:-((-v+7)/8);if(k==1)q+=y;
        unsigned bits=(unsigned)q&65535;out[k]=(int16_t)(bits<32768?(int)bits:(int)bits-65536);
    }
}
static FhShadowTrackLiveVertex positions(const FhShadowTrackLiveRef*r,const int16_t(*p)[3],int16_t y){
    FhShadowTrackLiveVertex v={0};quantize(v.a,p[r->source[0]],y);
    quantize(v.b,p[r->source[1]],y);quantize(v.c,p[r->source[2]],y);return v;
}
int fh_pica_shadow_track_live_compile(const FhShadowTrackSource*s,FhShadowTrackLiveVertex*v,unsigned vc,
    uint16_t*ix,unsigned ic,FhShadowTrackDraw*d,unsigned dc,FhShadowTrackLiveRef*refs,unsigned rc,FhShadowTrackLiveSize*out){
    if(!s || !out || !s->position || !s->triangle || !s->group || s->vertices>65535 ||
       s->triangles>65535 || s->groups>65535 ||
       (v?(!ix || !d || !refs):(ix!=NULL || d!=NULL || refs!=NULL)))return 0;
    if(s->group[0].first || s->group[s->groups].first!=s->triangles)return 0;
    for(unsigned g=0;g<s->groups;g++){
        const FhShadowTrackGroup*p=s->group+g;
        if(p->first>s->group[g+1].first || s->group[g+1].first>s->triangles ||
           p->min_x>p->max_x || p->min_y>p->max_y || p->min_z>p->max_z)return 0;
    }
    for(unsigned t=0;t<s->triangles;t++)for(unsigned k=0;k<3;k++)if(s->triangle[t][k]>=s->vertices)return 0;
    FhShadowTrackLiveSize size={0};
    for(unsigned pass=0;pass<(v?2u:1u);pass++){
        unsigned count=0,draws=0;
        for(unsigned g=0;g<s->groups;g++){
            const FhShadowTrackGroup*p=s->group+g;unsigned type=surface(p->flags);
            if(!type || p->min_x>640 || p->max_x<0 || p->min_z>640 || p->max_z<0)continue;
            unsigned first=count;
            FhShadowTrackDraw group={.first=first*3,.source_group=g,.flags=type};
            for(unsigned t=p->first;t<s->group[g+1].first;t++){
                if(!(s->triangle[t][3]&255) || !(s->triangle[t][3]&65280))continue;
                if(count==21845)return 0;
                if(pass){
                    FhShadowTrackLiveRef ref={{s->triangle[t][0],s->triangle[t][1],s->triangle[t][2]}};
                    FhShadowTrackLiveVertex value=positions(&ref,s->position,s->y_offset);
                    const int16_t*point[3]={value.a,value.b,value.c};
                    if(count==first)for(unsigned k=0;k<3;k++)group.lo[k]=group.hi[k]=value.a[k];
                    for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++){
                        if(point[j][k]<group.lo[k])group.lo[k]=point[j][k];
                        if(point[j][k]>group.hi[k])group.hi[k]=point[j][k];
                    }
                    refs[count]=ref;
                    for(unsigned corner=0;corner<3;corner++){value.selector=(int16_t)corner;v[count*3+corner]=value;ix[count*3+corner]=(uint16_t)(count*3+corner);}
                }
                count++;
            }
            if(count!=first){
                if(pass){group.count=(count-first)*3;d[draws]=group;}
                draws++;
            }
        }
        if(!pass){
            size=(FhShadowTrackLiveSize){{count*3,draws,count*3*20+((count*6+15)&~15u),draws*sizeof(FhShadowTrackDraw)+count*sizeof(FhShadowTrackLiveRef)},count,0};
            if(v && (vc<count*3 || ic<count*3 || dc<draws || rc<count))return 0;
        }
    }
    *out=size;return 1;
}
unsigned fh_pica_shadow_track_live_positions(FhShadowTrackLiveVertex*v,const FhShadowTrackLiveRef*r,unsigned count,
    const int16_t(*p)[3],int16_t y,unsigned*first,unsigned*last,FhShadowTrackRoutes*routes){
    unsigned changed=0;*first=*last=0;
    for(unsigned i=0;i<count;i++){
        FhShadowTrackLiveVertex value=positions(r+i,p,y);
        if(routes){
            const int16_t*point[3]={value.a,value.b,value.c};
            /* One conservative live bound for both surface selections; an
             * alternate subset can only be smaller. No stale load-time bounds. */
            for(unsigned route=0;route<2;route++)if(routes->route[route].count){
                FhShadowTrackRoute*b=routes->route+route;
                if(!i)for(unsigned k=0;k<3;k++)b->lo[k]=b->hi[k]=value.a[k];
                for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++){
                    if(point[j][k]<b->lo[k])b->lo[k]=point[j][k];
                    if(point[j][k]>b->hi[k])b->hi[k]=point[j][k];
                }
            }
        }
        if(!memcmp(v[i*3].a,value.a,6) && !memcmp(v[i*3].b,value.b,6) && !memcmp(v[i*3].c,value.c,6))continue;
        if(!changed)*first=i*3*sizeof(*v);
        for(unsigned corner=0;corner<3;corner++){
            /* Leave the selector and every index untouched. */
            memcpy(v[i*3+corner].a,value.a,6);memcpy(v[i*3+corner].b,value.b,6);memcpy(v[i*3+corner].c,value.c,6);
        }
        changed++;*last=(i+1)*3*sizeof(*v);
    }
    return changed;
}
typedef struct {FhShadowTrackLiveVertex*v;unsigned vc;uint16_t*ix;unsigned ic;FhShadowTrackDraw*d;unsigned dc;
    FhShadowTrackLiveRef*refs;unsigned rc;FhShadowTrackLiveSize result;} LiveContext;
static int live_compile(const FhShadowTrackSource*s,void*context){
    LiveContext*c=context;return fh_pica_shadow_track_live_compile(s,c->v,c->vc,c->ix,c->ic,c->d,c->dc,c->refs,c->rc,&c->result);
}
int fh_pica_shadow_track_raw_live_compile(const void*input,unsigned bytes,FhShadowTrackLiveVertex*v,unsigned vc,
    uint16_t*ix,unsigned ic,FhShadowTrackDraw*d,unsigned dc,FhShadowTrackLiveRef*refs,unsigned rc,FhShadowTrackLiveSize*size,int16_t*y){
    if(!size || (v?(!ix || !d || !refs):(ix!=NULL || d!=NULL || refs!=NULL)))return 0;
    LiveContext c={.v=v,.vc=vc,.ix=ix,.ic=ic,.d=d,.dc=dc,.refs=refs,.rc=rc};unsigned scratch;
    if(!fh_pica_shadow_track_raw_source(input,bytes,live_compile,&c,&scratch,y))return 0;
    c.result.scratch_bytes=scratch;*size=c.result;return 1;
}
