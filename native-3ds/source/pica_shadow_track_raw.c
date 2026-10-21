#include "fh_pica_shadow_track.h"
#include <stdlib.h>
#include <string.h>
static unsigned be16(const unsigned char*p){return (unsigned)p[0]*256+p[1];}
static unsigned be32(const unsigned char*p){return be16(p)*65536u+be16(p+2);}
static int16_t signed16(const unsigned char*p){unsigned n=be16(p);return (int16_t)(n<32768?(int)n:(int)n-65536);}
static int span(unsigned at,unsigned n,unsigned stride,unsigned bytes){return at<=bytes && n<=(bytes-at)/stride;}
int fh_pica_shadow_track_raw_scratch(const void*input,unsigned bytes,unsigned*scratch){
    if(!input || !scratch || bytes<0xa4 || bytes>4*1024*1024)return 0;
    const unsigned char*raw=input;
    unsigned nv=be16(raw+0x90),nt=be16(raw+0x98),ng=be16(raw+0x9a);
    unsigned vp=be32(raw+0x58),tp=be32(raw+0x4c),gp=be32(raw+0x50);
    /* The terminal group contains only a first-triangle sentinel. */
    if(!span(vp,nv,6,bytes) || !span(tp,nt,8,bytes) || !span(gp,ng,20,bytes) ||
       !span(gp+ng*20,1,2,bytes))return 0;
    unsigned vb=(nv*6+3)&~3u,tb=nt*8,gb=(ng+1)*sizeof(FhShadowTrackGroup);
    if(vb+tb+gb>512*1024)return 0;
    *scratch=vb+tb+gb;return 1;
}
int fh_pica_shadow_track_raw_source(const void*input,unsigned bytes,int(*compile)(const FhShadowTrackSource*,void*),
    void*context,unsigned*scratch,int16_t*y_offset){
    unsigned measured;
    if(!compile || !scratch || !fh_pica_shadow_track_raw_scratch(input,bytes,&measured))return 0;
    const unsigned char*raw=input;
    unsigned nv=be16(raw+0x90),nt=be16(raw+0x98),ng=be16(raw+0x9a);
    unsigned vp=be32(raw+0x58),tp=be32(raw+0x4c),gp=be32(raw+0x50);
    unsigned vb=(nv*6+3)&~3u,tb=nt*8;
    unsigned char*storage=malloc(measured);if(!storage)return 0;
    int16_t(*position)[3]=(void*)storage;
    uint16_t(*triangle)[4]=(void*)(storage+vb);
    FhShadowTrackGroup*group=(void*)(storage+vb+tb);
    for(unsigned i=0;i<nv;i++)for(unsigned k=0;k<3;k++)position[i][k]=signed16(raw+vp+i*6+k*2);
    for(unsigned i=0;i<nt;i++)for(unsigned k=0;k<4;k++)triangle[i][k]=(uint16_t)be16(raw+tp+i*8+k*2);
    for(unsigned i=0;i<ng;i++){
        const unsigned char*p=raw+gp+i*20;
        group[i]=(FhShadowTrackGroup){(uint16_t)be16(p),signed16(p+2),signed16(p+4),signed16(p+6),signed16(p+8),
            signed16(p+10),signed16(p+12),(uint16_t)be16(p+14),be32(p+16)};
    }
    group[ng]=(FhShadowTrackGroup){.first=(uint16_t)be16(raw+gp+ng*20)};
    int16_t offset=signed16(raw+0x8e);
    FhShadowTrackSource source={(const int16_t(*)[3])position,(const uint16_t(*)[4])triangle,group,nv,nt,ng,offset};
    int ok=compile(&source,context);
    if(ok){*scratch=measured;if(y_offset)*y_offset=offset;}
    free(storage);return ok;
}
typedef struct {FhShadowTrackVertex*v;unsigned vc;uint16_t*ix;unsigned ic;FhShadowTrackDraw*d;unsigned dc;FhShadowTrackIndexedSize result;} IndexedContext;
static int indexed(const FhShadowTrackSource*s,void*context){
    IndexedContext*c=context;return fh_pica_shadow_track_indexed_compile(s,c->v,c->vc,c->ix,c->ic,c->d,c->dc,&c->result);
}
int fh_pica_shadow_track_raw_compile(const void*input,unsigned bytes,FhShadowTrackVertex*vertices,unsigned vc,
    uint16_t*indices,unsigned ic,FhShadowTrackDraw*draws,unsigned dc,FhShadowTrackIndexedSize*size,int16_t*y_offset){
    if(!size || (vertices?(!indices || !draws):(indices!=NULL || draws!=NULL)))return 0;
    IndexedContext c={.v=vertices,.vc=vc,.ix=indices,.ic=ic,.d=draws,.dc=dc};unsigned scratch;
    if(!fh_pica_shadow_track_raw_source(input,bytes,indexed,&c,&scratch,y_offset))return 0;
    c.result.scratch_bytes+=scratch;*size=c.result;return 1;
}
