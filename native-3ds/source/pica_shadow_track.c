#include "fh_pica_shadow_track.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(FhShadowTrackGroup)==20,"original collision group layout");
_Static_assert(sizeof(FhShadowTrackVertex)==20,"compact receiver vertex layout");

/* trackBuildBlockTriangles query0x81, before spatial/normal selection. */
static unsigned group_type(unsigned flags){
    if(!(flags&4) || ((flags&8)?(flags&1):(flags&2)))return 0;
    return ((flags&8)?(4|16):2)|8|((flags&32)?64:0);
}
static int triangle(const FhShadowTrackSource*s,unsigned n,FhShadowTrackVertex out[3]){
    if(!(s->triangle[n][3]&255) || !(s->triangle[n][3]&65280))return 0;
    for(unsigned i=0;i<3;i++){
        const int16_t*p=s->position[s->triangle[n][i]];
        for(unsigned k=0;k<3;k++){
            /* Source ARM/PPC arithmetic shift rounds negative values down.
             * Express it without relying on a host's signed-shift convention. */
            int x=p[k],q=x>=0?x/8:-((-x+7)/8);
            if(k==1)q+=s->y_offset;
            /* Source stores the resulting coordinates in signed16 TrackTriangle.
             * Explicit modulo also avoids implementation-defined narrowing. */
            unsigned bits=(unsigned)q&65535;
            out[i].position[k]=(int16_t)(bits<32768?(int)bits:(int)bits-65536);
        }
        out[i].position[3]=1;
    }
    /* Original static block broadphase clamps horizontal queries to0..640. */
    for(unsigned k=0;k<3;k+=2){
        int lo=out[0].position[k],hi=lo;
        for(unsigned i=1;i<3;i++){int q=out[i].position[k];if(q<lo)lo=q;if(q>hi)hi=q;}
        if(lo>640 || hi<0)return 0;
    }
    float a[3],b[3],normal[3];
    for(unsigned k=0;k<3;k++){a[k]=(float)out[0].position[k]-out[1].position[k];b[k]=(float)out[1].position[k]-out[2].position[k];}
    for(unsigned k=0;k<3;k++)normal[k]=a[(k+1)%3]*b[(k+2)%3]-a[(k+2)%3]*b[(k+1)%3];
    float mag=sqrtf(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
    if(!(mag>0))return 0;
    float scale=1.f/mag;
    for(unsigned i=0;i<3;i++)for(unsigned k=0;k<3;k++)out[i].normal[k]=normal[k]*scale;
    return 1;
}
int fh_pica_shadow_track_compile(const FhShadowTrackSource*s,FhShadowTrackVertex*vertices,unsigned vc,
                                 FhShadowTrackDraw*draws,unsigned dc,FhShadowTrackSize*size){
    if(!s || !size || !s->position || !s->triangle || !s->group ||
       s->vertices>65535 || s->triangles>65535 || s->groups>65535 ||
       ((vertices==NULL)!=(draws==NULL)))return 0;
    if(s->group[0].first!=0 || s->group[s->groups].first!=s->triangles)return 0;
    for(unsigned g=0;g<s->groups;g++){
        const FhShadowTrackGroup*p=s->group+g;
        if(p->first>s->group[g+1].first || s->group[g+1].first>s->triangles ||
           p->min_x>p->max_x || p->min_y>p->max_y || p->min_z>p->max_z)return 0;
    }
    for(unsigned t=0;t<s->triangles;t++)for(unsigned k=0;k<3;k++)if(s->triangle[t][k]>=s->vertices)return 0;
    FhShadowTrackSize measured={0};
    /* First pass guarantees atomic rejection of a too-small destination. */
    for(unsigned pass=0;pass<(vertices?2u:1u);pass++){
        unsigned corners=0,count=0;
        for(unsigned g=0;g<s->groups;g++){
            const FhShadowTrackGroup*p=s->group+g;unsigned type=group_type(p->flags);
            if(!type || p->min_x>640 || p->max_x<0 || p->min_z>640 || p->max_z<0)continue;
            FhShadowTrackDraw d={.first=corners,.source_group=g,.flags=type};
            for(unsigned t=p->first;t<s->group[g+1].first;t++){
                FhShadowTrackVertex v[3];if(!triangle(s,t,v))continue;
                if(!d.count)for(unsigned k=0;k<3;k++)d.lo[k]=d.hi[k]=v[0].position[k];
                for(unsigned i=0;i<3;i++)for(unsigned k=0;k<3;k++){
                    int16_t q=v[i].position[k];if(q<d.lo[k])d.lo[k]=q;if(q>d.hi[k])d.hi[k]=q;
                }
                if(pass)memcpy(vertices+corners,v,sizeof(v));
                corners+=3;d.count+=3;
            }
            if(d.count){if(pass)draws[count]=d;count++;}
        }
        if(!pass){
            measured=(FhShadowTrackSize){corners,count,corners*sizeof(FhShadowTrackVertex),count*sizeof(FhShadowTrackDraw)};
            if(vertices && (vc<corners || dc<count))return 0;
        }
    }
    *size=measured;return 1;
}
int fh_pica_shadow_track_indexed_compile(const FhShadowTrackSource*s,FhShadowTrackVertex*vertices,unsigned vc,
    uint16_t*indices,unsigned ic,FhShadowTrackDraw*draws,unsigned dc,FhShadowTrackIndexedSize*size){
    if(!size || (vertices?(!indices || !draws):(indices!=NULL || draws!=NULL)))return 0;
    FhShadowTrackSize plain;
    if(!fh_pica_shadow_track_compile(s,NULL,0,NULL,0,&plain) || plain.corners>65535)return 0;
    if(vertices && (ic<plain.corners || dc<plain.draws))return 0;
    unsigned slots=1;while(slots<plain.corners*2)slots*=2;
    unsigned ib=(plain.corners*2+3)&~3u;
    unsigned scratch_bytes=plain.gpu_bytes+plain.cpu_bytes+ib+slots*4;
    if(scratch_bytes>1024*1024)return 0;
    void*scratch=malloc(scratch_bytes);if(!scratch)return 0;
    FhShadowTrackVertex*v=scratch;
    FhShadowTrackDraw*d=(void*)((unsigned char*)scratch+plain.gpu_bytes);
    uint16_t*ix=(void*)((unsigned char*)d+plain.cpu_bytes);
    uint32_t*table=(void*)((unsigned char*)ix+ib);
    if(!fh_pica_shadow_track_compile(s,v,plain.corners,d,plain.draws,&plain)){free(scratch);return 0;}
    memset(table,0xff,slots*4);unsigned unique=0;
    for(unsigned corner=0;corner<plain.corners;corner++){
        FhShadowTrackVertex value=v[corner];uint32_t hash=2166136261u;
        for(unsigned b=0;b<sizeof(value);b++){hash^=((const unsigned char*)&value)[b];hash*=16777619u;}
        unsigned slot=hash&(slots-1),index;
        while((index=table[slot])!=UINT32_MAX && memcmp(v+index,&value,sizeof(value)))slot=(slot+1)&(slots-1);
        if(index==UINT32_MAX){index=unique++;v[index]=value;table[slot]=index;}
        ix[corner]=(uint16_t)index;
    }
    if(vertices && vc<unique){free(scratch);return 0;}
    if(vertices){
        memcpy(vertices,v,unique*sizeof(*v));memcpy(indices,ix,plain.corners*2);memcpy(draws,d,plain.cpu_bytes);
    }
    plain.gpu_bytes=((unique*sizeof(*v)+15)&~15u)+((plain.corners*2+15)&~15u);
    *size=(FhShadowTrackIndexedSize){plain,unique,scratch_bytes};free(scratch);return 1;
}
