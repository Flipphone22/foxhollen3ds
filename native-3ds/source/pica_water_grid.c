#include "fh_pica_water_grid.h"
#include <string.h>
#include <math.h>
_Static_assert(sizeof(FhWaterGridVertex)==16,"Water grid full fetch stride");
int fh_water_grid_size(unsigned w,unsigned h,unsigned step,FhWaterGridSize *s){
    if(!s || w<4 || h<4 || w>64 || h>64 || (w&3) || (h&3) ||
       (step!=1 && step!=2 && step!=4 && step!=8) || w%step || h%step)return 0;
    unsigned x=w/step+2,y=h/step+2;
    *s=(FhWaterGridSize){x*y,6*(x-1)*(y-1),x*y*sizeof(FhWaterGridVertex),12*(x-1)*(y-1)};
    return 1;
}
static unsigned knot(unsigned i,unsigned count,unsigned step,unsigned dimension){
    if(!i)return 0;
    if(i==count-1)return 16384;
    return ((2*i-1)*step*8192)/dimension;
}
enum {CHANNEL_BLUE,CHANNEL_GREEN,CHANNEL_ALPHA};
static unsigned channel(const uint8_t *p,unsigned w,unsigned x,unsigned y,unsigned component){
    unsigned at=2*((y/4)*(w/4)*16+(x/4)*16+(y%4)*4+x%4);
    unsigned v=((unsigned)p[at]<<8)|p[at+1];
    if(component==CHANNEL_ALPHA){
        if(v&0x8000)return 255;
        v=(v>>12)&7;return (v<<5)|(v<<2)|(v>>1);
    }
    if(v&0x8000){v=(v>>(component==CHANNEL_GREEN?5:0))&31;return (v<<3)|(v>>2);}
    return ((v>>(component==CHANNEL_GREEN?4:0))&15)*17;
}
static int delta(const uint8_t *p,unsigned w,unsigned h,unsigned u,unsigned v,unsigned green){
    /* Texel-centre coordinates, Q14. Clamp before division so signed right
     * shift/negative remainder conventions do not affect the sampler. */
    int sx=(int)(u*w)-8192,sy=(int)(v*h)-8192;
    if(sx<0)sx=0;
    if(sy<0)sy=0;
    if(sx>(int)((w-1)*16384))sx=(int)((w-1)*16384);
    if(sy>(int)((h-1)*16384))sy=(int)((h-1)*16384);
    unsigned x=(unsigned)sx/16384,y=(unsigned)sy/16384;
    unsigned X=x+1<w?x+1:x,Y=y+1<h?y+1:y;
    unsigned fx=(unsigned)sx%16384,fy=(unsigned)sy%16384;
    uint64_t a=(uint64_t)channel(p,w,x,y,green)*(16384-fx)+channel(p,w,X,y,green)*fx;
    uint64_t b=(uint64_t)channel(p,w,x,Y,green)*(16384-fx)+channel(p,w,X,Y,green)*fx;
    uint64_t sum=a*(16384-fy)+b*fy;
    return (int)((sum+8388608)/16777216)-128*16;
}
static int compile_profile(const void *source,size_t bytes,unsigned w,unsigned h,unsigned step,
    FhWaterGridVertex *vertices,size_t vertex_bytes,uint16_t *indices,size_t index_bytes,
    unsigned component0,unsigned component1,unsigned uv_numerator){
    FhWaterGridSize s;
    if(!source || !vertices || !indices || !fh_water_grid_size(w,h,step,&s) ||
       bytes<(size_t)w*h*2 || vertex_bytes<s.vertex_bytes || index_bytes<s.index_bytes)return 0;
    unsigned nx=w/step+2,ny=h/step+2;
    memset(vertices,0,s.vertex_bytes);
    for(unsigned y=0;y<ny;y++)for(unsigned x=0;x<nx;x++){
        unsigned u=knot(x,nx,step,w),v=knot(y,ny,step,h);
        FhWaterGridVertex *o=&vertices[y*nx+x];
        o->uv_delta[0]=(int16_t)u;o->uv_delta[1]=(int16_t)v;
        unsigned su=u*uv_numerator/128,sv=v*uv_numerator/128;
        o->uv_delta[2]=(int16_t)delta(source,w,h,su,sv,component0);
        o->uv_delta[3]=(int16_t)delta(source,w,h,su,sv,component1);
    }
    unsigned n=0;
    for(unsigned y=0;y+1<ny;y++)for(unsigned x=0;x+1<nx;x++){
        unsigned a=y*nx+x,b=a+1,c=a+nx,d=c+1;
        indices[n++]=(uint16_t)b;indices[n++]=(uint16_t)c;indices[n++]=(uint16_t)a;
        indices[n++]=(uint16_t)b;indices[n++]=(uint16_t)d;indices[n++]=(uint16_t)c;
    }
    return 1;
}
int fh_water_grid_compile(const void *source,size_t bytes,unsigned w,unsigned h,unsigned step,
    FhWaterGridVertex *vertices,size_t vertex_bytes,uint16_t *indices,size_t index_bytes){
    return compile_profile(source,bytes,w,h,step,vertices,vertex_bytes,indices,index_bytes,
        CHANNEL_BLUE,CHANNEL_GREEN,128);
}
static unsigned texel_bound(unsigned u,unsigned size,unsigned upper){
    int q=(int)(u*size)-8192;
    if(q<=0)return 0;
    unsigned n=((unsigned)q+(upper?16383:0))/16384;
    return n<size?n:size-1;
}
static int compile_visible_profile(const void *source,size_t bytes,unsigned w,unsigned h,unsigned step,
    FhWaterGridVertex *v,size_t vb,uint16_t *ids,size_t ib,FhWaterGridSize *result,
    unsigned component0,unsigned component1,unsigned coverage,unsigned uv_numerator){
    FhWaterGridSize s;
    if(!result || !fh_water_grid_size(w,h,step,&s) ||
       !compile_profile(source,bytes,w,h,step,v,vb,ids,ib,component0,component1,uv_numerator))return 0;
    uint16_t remap[66*66];
    memset(remap,255,s.vertices*sizeof(*remap));
    unsigned nx=w/step+2,ny=h/step+2,n=0;
    for(unsigned y=0;y+1<ny;y++)for(unsigned x=0;x+1<nx;x++){
        unsigned a=y*nx+x,d=a+nx+1;
        unsigned x0=texel_bound((unsigned)v[a].uv_delta[0]*uv_numerator/128,w,0),x1=texel_bound((unsigned)v[d].uv_delta[0]*uv_numerator/128,w,1);
        unsigned y0=texel_bound((unsigned)v[a].uv_delta[1]*uv_numerator/128,h,0),y1=texel_bound((unsigned)v[d].uv_delta[1]*uv_numerator/128,h,1);
        unsigned visible=0;
        for(unsigned j=y0;j<=y1&&!visible;j++)for(unsigned i=x0;i<=x1;i++)
            if(channel(source,w,i,j,coverage)){visible=1;break;}
        if(!visible)continue;
        unsigned first=(y*(nx-1)+x)*6;
        for(unsigned i=0;i<6;i++){unsigned index=ids[first+i];ids[n++]=(uint16_t)index;remap[index]=0;}
    }
    unsigned count=0;
    for(unsigned i=0;i<s.vertices;i++)if(remap[i]!=UINT16_MAX){
        remap[i]=(uint16_t)count;v[count++]=v[i];
    }
    for(unsigned i=0;i<n;i++)ids[i]=remap[ids[i]];
    *result=(FhWaterGridSize){count,n,count*sizeof(*v),n*sizeof(*ids)};
    return 1;
}
int fh_water_grid_compile_visible(const void *source,size_t bytes,unsigned w,unsigned h,unsigned step,
    FhWaterGridVertex *v,size_t vb,uint16_t *ids,size_t ib,FhWaterGridSize *result){
    return compile_visible_profile(source,bytes,w,h,step,v,vb,ids,ib,result,
        CHANNEL_BLUE,CHANNEL_GREEN,CHANNEL_ALPHA,128);
}
int fh_water_grid_compile_ripple_visible(const void *source,size_t bytes,unsigned step,
    FhWaterGridVertex *v,size_t vb,uint16_t *ids,size_t ib,FhWaterGridSize *result){
    return compile_visible_profile(source,bytes,64,64,step,v,vb,ids,ib,result,
        CHANNEL_ALPHA,CHANNEL_BLUE,CHANNEL_GREEN,127);
}

int fh_water_grid_ripple_radial_size(unsigned segments,unsigned rings,FhWaterGridSize *s){
    if(!s || segments<24 || segments>64 || segments%8 || rings<12 || rings>32 || rings%4)return 0;
    unsigned angles=segments+4,vertices=1+(rings+1)*angles,indices=6*(rings+1)*angles;
    *s=(FhWaterGridSize){vertices,indices,vertices*sizeof(FhWaterGridVertex),indices*sizeof(uint16_t)};
    return 1;
}
static unsigned radial_vertex(const uint8_t *pixels,FhWaterGridVertex *v,unsigned *count,double u,double w){
    int x=(int)floor(u*16384+.5),y=(int)floor(w*16384+.5);
    if(x<0)x=0;
    if(x>16384)x=16384;
    if(y<0)y=0;
    if(y>16384)y=16384;
    /* Bounded load-time deduplication. Clipped rings must reuse earlier edge
     * vertices rather than create overlapping/zero-area triangles. */
    for(unsigned i=0;i<*count;i++)if(v[i].uv_delta[0]==x && v[i].uv_delta[1]==y)return i;
    unsigned index=(*count)++;FhWaterGridVertex *o=v+index;
    memset(o,0,sizeof(*o));o->uv_delta[0]=(int16_t)x;o->uv_delta[1]=(int16_t)y;
    unsigned su=(unsigned)x*127/128,sv=(unsigned)y*127/128;
    o->uv_delta[2]=(int16_t)delta(pixels,64,64,su,sv,CHANNEL_ALPHA);
    o->uv_delta[3]=(int16_t)delta(pixels,64,64,su,sv,CHANNEL_BLUE);
    return index;
}
static void radial_triangle(const FhWaterGridVertex *v,uint16_t *ids,unsigned *count,unsigned a,unsigned b,unsigned c){
    int64_t abx=v[b].uv_delta[0]-v[a].uv_delta[0],aby=v[b].uv_delta[1]-v[a].uv_delta[1];
    int64_t acx=v[c].uv_delta[0]-v[a].uv_delta[0],acy=v[c].uv_delta[1]-v[a].uv_delta[1];
    /* All rays are ordered; clipping and quantization can collapse a triangle.
     * Its surviving orientation must still be positive. */
    if(abx*acy-aby*acx<=0)return;
    ids[(*count)++]=(uint16_t)a;ids[(*count)++]=(uint16_t)b;ids[(*count)++]=(uint16_t)c;
}
int fh_water_grid_compile_ripple_radial(const void *source,size_t bytes,unsigned segments,unsigned rings,
    FhWaterGridVertex *v,size_t vb,uint16_t *ids,size_t ib,FhWaterGridSize *result){
    FhWaterGridSize capacity;
    if(!source || bytes<8192 || !v || !ids || !result ||
       !fh_water_grid_ripple_radial_size(segments,rings,&capacity) ||
       vb<capacity.vertex_bytes || ib<capacity.index_bytes)return 0;
    const double tau=6.283185307179586476925286766559,centre=32.5/63.5;
    double angles[68],dx[68],dy[68],boundary[68];uint16_t previous[68],current[68];
    unsigned n=segments,count=0,ni=0;
    for(unsigned i=0;i<segments;i++)angles[i]=tau*i/segments;
    for(unsigned y=0;y<2;y++)for(unsigned x=0;x<2;x++){
        double angle=atan2((double)y-centre,(double)x-centre);
        if(angle<0)angle+=tau;
        angles[n++]=angle;
    }
    for(unsigned i=1;i<n;i++){
        double angle=angles[i];unsigned j=i;
        while(j && angles[j-1]>angle){angles[j]=angles[j-1];j--;}
        angles[j]=angle;
    }
    unsigned unique=0;
    for(unsigned i=0;i<n;i++)if(!unique || angles[i]-angles[unique-1]>1e-10)angles[unique++]=angles[i];
    n=unique;
    unsigned middle=radial_vertex(source,v,&count,centre,centre);
    for(unsigned i=0;i<n;i++){
        dx[i]=cos(angles[i]);dy[i]=sin(angles[i]);double limit=2;
        if(dx[i]>1e-12)limit=fmin(limit,(1-centre)/dx[i]);
        if(dx[i]<-1e-12)limit=fmin(limit,-centre/dx[i]);
        if(dy[i]>1e-12)limit=fmin(limit,(1-centre)/dy[i]);
        if(dy[i]<-1e-12)limit=fmin(limit,-centre/dy[i]);
        boundary[i]=limit;previous[i]=(uint16_t)middle;
    }
    for(unsigned ring=1;ring<=rings+1;ring++){
        for(unsigned i=0;i<n;i++){
            double radius=ring==rings+1?boundary[i]:fmin(boundary[i],.54*ring/rings);
            current[i]=(uint16_t)radial_vertex(source,v,&count,centre+radius*dx[i],centre+radius*dy[i]);
        }
        for(unsigned i=0;i<n;i++){
            unsigned j=(i+1)%n;
            radial_triangle(v,ids,&ni,previous[i],current[i],previous[j]);
            radial_triangle(v,ids,&ni,current[i],current[j],previous[j]);
        }
        memcpy(previous,current,n*sizeof(*previous));
    }
    *result=(FhWaterGridSize){count,ni,count*sizeof(*v),ni*sizeof(*ids)};
    return 1;
}
