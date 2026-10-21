#include "fh_world_bounds.h"
enum { WORLD_NEAR_BYTES=192*1024, WORLD_NEAR_ATTRS=13, WORLD_NEAR_VERTICES=8192, WORLD_NEAR_PLANES=5, WORLD_NEAR_POLY=9 };
#define WORLD_NEAR_GUARD 4.0f
#define WORLD_NEAR_MARGIN 64.0f
typedef struct {float c[4],a[WORLD_NEAR_ATTRS];} WorldNearVertex;
static unsigned char *world_near_arena[2];
static unsigned world_near_used,world_near_frame=~0u;
static unsigned world_near_draws,world_near_kept,world_near_clipped,world_near_dropped,world_near_fallbacks,world_near_large,world_near_full;
static float world_near_plane[4],world_near_margin;
static unsigned char world_near_code[WORLD_NEAR_VERTICES];
extern unsigned fh_minimal_frames;
static void world_near_prepare(const float view[3][4],const float projection[4][4]){
    for(unsigned c=0;c<4;c++){
        float z=projection[2][0]*view[0][c]+projection[2][1]*view[1][c]+projection[2][2]*view[2][c]+(c==3?projection[2][3]:0.f);
        float w=projection[3][0]*view[0][c]+projection[3][1]*view[1][c]+projection[3][2]*view[2][c]+(c==3?projection[3][3]:0.f);
        world_near_plane[c]=z+w;
    }
    float slope=projection[2][2]+projection[3][2];
    world_near_margin=(slope<0?-slope:slope)*WORLD_NEAR_MARGIN;
}
static int world_near_straddles(const FhWorldBounds *b){
    float lo=world_near_plane[3],hi=world_near_plane[3];
    for(unsigned k=0;k<3;k++){
        float a=world_near_plane[k]*b->lo[k],c=world_near_plane[k]*b->hi[k];
        lo+=a<c?a:c;hi+=a<c?c:a;
    }
    return lo<world_near_margin && hi>0;
}
static void world_near_decode(WorldNearVertex *o,const unsigned char *v,unsigned compact,
    const float view[3][4],const float projection[4][4]){
    float *a=o->a;
    if(compact){
        int16_t s[8];memcpy(s,v,8);memcpy(s+4,v+12,8);
        for(unsigned k=0;k<4;k++){a[k]=s[k];a[4+k]=v[8+k];a[8+k]=s[4+k];}
        a[12]=0;
    }else memcpy(a,v,sizeof(float)*WORLD_NEAR_ATTRS);
    float e[4];
    for(unsigned r=0;r<3;r++)e[r]=view[r][0]*a[0]+view[r][1]*a[1]+view[r][2]*a[2]+view[r][3]*a[3];
    e[3]=1;
    for(unsigned r=0;r<4;r++)o->c[r]=projection[r][0]*e[0]+projection[r][1]*e[1]+projection[r][2]*e[2]+projection[r][3]*e[3];
}
static float world_near_distance(const float *c,unsigned plane){
    switch(plane){
    case 0:return c[2]+c[3];
    case 1:return WORLD_NEAR_GUARD*c[3]-c[0];
    case 2:return WORLD_NEAR_GUARD*c[3]+c[0];
    case 3:return WORLD_NEAR_GUARD*c[3]-c[1];
    default:return WORLD_NEAR_GUARD*c[3]+c[1];
    }
}
static unsigned world_near_outcode(const float *c){
    unsigned code=0;
    for(unsigned k=0;k<WORLD_NEAR_PLANES;k++)if(world_near_distance(c,k)<0)code|=1u<<k;
    return code;
}
static void world_near_lerp(WorldNearVertex *o,const WorldNearVertex *a,const WorldNearVertex *b,float t){
    for(unsigned k=0;k<4;k++)o->c[k]=a->c[k]+(b->c[k]-a->c[k])*t;
    for(unsigned k=0;k<WORLD_NEAR_ATTRS;k++)o->a[k]=a->a[k]+(b->a[k]-a->a[k])*t;
}
static unsigned world_near_clip(WorldNearVertex *poly,unsigned n,unsigned code,WorldNearVertex *scratch){
    WorldNearVertex *in=poly,*out=scratch;
    for(unsigned plane=0;plane<WORLD_NEAR_PLANES && n>=3;plane++){
        if(!(code&(1u<<plane)))continue;
        unsigned m=0;
        for(unsigned i=0;i<n;i++){
            const WorldNearVertex *a=in+i,*b=in+(i+1)%n;
            float da=world_near_distance(a->c,plane),db=world_near_distance(b->c,plane);
            if(da>=0)out[m++]=*a;
            if((da>=0)!=(db>=0) && m<WORLD_NEAR_POLY)world_near_lerp(out+m++,a,b,da/(da-db));
        }
        WorldNearVertex *t=in;in=out;out=t;n=m;
    }
    if(in!=poly)memcpy(poly,in,sizeof(*poly)*n);
    return n;
}
static unsigned world_near_draw(const FhPicaWorldDraw *g,unsigned compact,unsigned textured,
    const float view[3][4],float scale,const float projection[4][4]){
    if(!g->vertices || g->vertices>WORLD_NEAR_VERTICES || g->indices%3){world_near_fallbacks++;world_near_large++;return 0;}
    if(world_near_frame!=fh_minimal_frames){world_near_frame=fh_minimal_frames;world_near_used=0;}
    unsigned slot=world_near_frame&1;
    if(!world_near_arena[slot]){
        world_near_arena[slot]=linearMemAlign(WORLD_NEAR_BYTES,128);
        if(!world_near_arena[slot]){world_near_fallbacks++;return 0;}
    }
    float scaled[3][4];
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)scaled[r][c]=view[r][c]*(c<3?scale:1.f);
    unsigned stride=compact?20:64;
    const unsigned char *base=g->vertex;
    for(unsigned i=0;i<g->vertices;i++){
        const unsigned char *v=base+i*stride;float p[4];
        if(compact){int16_t s[4];memcpy(s,v,8);for(unsigned k=0;k<4;k++)p[k]=s[k];}
        else memcpy(p,v,16);
        float e[3];
        for(unsigned r=0;r<3;r++)e[r]=scaled[r][0]*p[0]+scaled[r][1]*p[1]+scaled[r][2]*p[2]+scaled[r][3]*p[3];
        float c[4];
        for(unsigned r=0;r<4;r++)c[r]=projection[r][0]*e[0]+projection[r][1]*e[1]+projection[r][2]*e[2]+projection[r][3];
        world_near_code[i]=(unsigned char)world_near_outcode(c);
    }
    unsigned index_bytes=((g->indices*2u)+127u)&~127u;
    if(world_near_used+index_bytes>WORLD_NEAR_BYTES){world_near_fallbacks++;world_near_full++;return 0;}
    uint16_t *kept=(uint16_t*)(world_near_arena[slot]+world_near_used);
    unsigned char *vertex_base=world_near_arena[slot]+world_near_used+index_bytes;
    unsigned vertex_room=(WORLD_NEAR_BYTES-world_near_used-index_bytes)/64;
    unsigned kept_count=0,emitted=0,clipped=0,dropped=0;
    float *emit=(float*)vertex_base;
    for(unsigned i=0;i<g->indices;i+=3){
        uint16_t idx[3]={g->index[i],g->index[i+1],g->index[i+2]};
        if(idx[0]>=g->vertices || idx[1]>=g->vertices || idx[2]>=g->vertices){world_near_fallbacks++;return 0;}
        unsigned c0=world_near_code[idx[0]],c1=world_near_code[idx[1]],c2=world_near_code[idx[2]];
        if(!(c0|c1|c2)){kept[kept_count++]=idx[0];kept[kept_count++]=idx[1];kept[kept_count++]=idx[2];continue;}
        if(c0&c1&c2){dropped++;continue;}
        WorldNearVertex out[WORLD_NEAR_POLY],scratch[WORLD_NEAR_POLY];
        for(unsigned k=0;k<3;k++)world_near_decode(out+k,base+idx[k]*stride,compact,scaled,projection);
        unsigned n=world_near_clip(out,3,c0|c1|c2,scratch);
        if(n<3){dropped++;continue;}
        if(emitted+(n-2)*3>vertex_room){world_near_fallbacks++;world_near_full++;return 0;}
        for(unsigned t=1;t+1<n;t++){
            const WorldNearVertex *v[3]={out,out+t,out+t+1};
            for(unsigned k=0;k<3;k++){
                float *o=emit+emitted*16;memcpy(o,v[k]->a,sizeof(float)*WORLD_NEAR_ATTRS);o[13]=o[14]=o[15]=0;emitted++;
            }
        }
        clipped++;
    }
    world_near_draws++;world_near_kept+=kept_count/3;world_near_clipped+=clipped;world_near_dropped+=dropped;
    world_near_used+=index_bytes+((emitted*64u+127u)&~127u);
    if(kept_count){
        GSPGPU_FlushDataCache(kept,kept_count*2);
        C3D_DrawElements(GPU_TRIANGLES,kept_count,C3D_UNSIGNED_SHORT,kept);
    }
    if(emitted){
        GSPGPU_FlushDataCache(emit,emitted*64);
        fh_pica_world_layout_bind(0,textured);
        fh_pica_world_buffers(0,textured,emit);
        C3D_DrawArrays(GPU_TRIANGLES,0,emitted);
    }
    return 1;
}
