/* Startup-only native CMPR -> ETC1/ETC1A4 preparation.
 * Uses the pinned tex3ds ZLIB-licensed rg_etc1 codec, without Magick/PNG/tools.
 * GC palette semantics reuse the reviewed pinned Melee decoder adaptation. */
#include "fh_install_texture.h"
#include "fh_install_worker.h"
#include "../vendor/rg_etc1/rg_etc1.h"
#include "../vendor/melee/texture_decode.h"
#include <string.h>

static bool initialized;
void fh_install_texture_init(void){if(!initialized){rg_etc1::pack_etc1_block_init();initialized=true;}}
/* Startup-only, image-local memoization. A hash match is never sufficient:
 * all 64 input bytes must match before an encoded block can be reused. Keeping
 * this on the conversion stack avoids heap pressure and any live-game owner.
 * Quality/dithering are fixed for the entire image. Alpha coverage and the
 * visible-error calculation remain outside the cache. */
struct FhInstallEtcCache {
    struct Entry { uint32_t hash; unsigned int rgba[16]; unsigned char packed[8]; } entries[256];
    void clear(){for(unsigned i=0;i<256;i++)entries[i].hash=0;}
    static uint32_t key(const unsigned int*rgba){
        uint32_t h=2166136261u;
        for(unsigned i=0;i<16;i++)h=(h^rgba[i])*16777619u;
        h^=h>>16;
        return h?h:1;
    }
    void pack(unsigned char*packed,const unsigned int*rgba,rg_etc1::etc1_pack_params&parameters,unsigned mode){
        uint32_t h=key(rgba);Entry&e=entries[h&255];
        if(e.hash==h&&!memcmp(e.rgba,rgba,sizeof(e.rgba))){memcpy(packed,e.packed,8);return;}
        rg_etc1::pack_etc1_block_with_hint(packed,rgba,parameters,mode);
        memcpy(e.rgba,rgba,sizeof(e.rgba));memcpy(e.packed,packed,8);e.hash=h;
    }
};
static_assert(sizeof(FhInstallEtcCache)==19456,"Bounded startup ETC cache");
static bool extent(unsigned n){return n>=8&&n<=1024&&!(n&(n-1));}
static uint32_t pixel(const unsigned char*src,unsigned w,unsigned x,unsigned y){
    const unsigned char*p=src+((y/8)*(w/8)+x/8)*32+((y%8)/4*2+(x%8)/4)*8;
    uint32_t colors[4];mp_cmpr_palette(p,colors);return colors[(p[4+y%4]>>(6-2*(x%4)))&3];
}
/* Pillow BOX's separable half-up rounding for integer power-of-two reductions.
 * No full RGBA image is allocated, including for original 1024x1024 inputs. */
static uint32_t filtered(const unsigned char*src,unsigned w,unsigned x,unsigned y,unsigned sx,unsigned sy){
    if(sx==1&&sy==1)return pixel(src,w,x,y);
    unsigned sums[3]={0,0,0};
    for(unsigned j=0;j<sy;j++){
        unsigned row[3]={0,0,0};
        for(unsigned i=0;i<sx;i++){
            uint32_t c=pixel(src,w,x*sx+i,y*sy+j);
            for(unsigned k=0;k<3;k++)row[k]+=(c>>(24-k*8))&255;
        }
        for(unsigned k=0;k<3;k++)sums[k]+=(row[k]+sx/2)/sx;
    }
    return mp_rgba((sums[0]+sy/2)/sy,(sums[1]+sy/2)/sy,(sums[2]+sy/2)/sy,255);
}
struct TextureRows {
 const unsigned char*src;unsigned char*out;unsigned w,h,gw,gh,first,last;
 bool alpha;FhInstallTextureResult completed;unsigned done,cancelled;int status;
 FhInstallTextureProgress progress;void*context;TextureRows*peer;
 const unsigned char*modes;
};
static int texture_progress(TextureRows*j){
 unsigned done=__atomic_load_n(&j->done,__ATOMIC_RELAXED);
 if(j->peer)done+=__atomic_load_n(&j->peer->done,__ATOMIC_ACQUIRE);
 return !j->progress||j->progress(j->context,done,j->gw*j->gh/64);
}
static void texture_rows(void*context){
 TextureRows*j=static_cast<TextureRows*>(context);j->status=1;
 const unsigned char*src=j->src;unsigned char*out=j->out;
 unsigned w=j->w,h=j->h,gw=j->gw,gh=j->gh;bool alpha=j->alpha;
 FhInstallTextureResult&completed=j->completed;
    rg_etc1::etc1_pack_params parameters;parameters.m_quality=rg_etc1::cHighQuality;
    FhInstallEtcCache cache;cache.clear();
    unsigned sx=w/gw,sy=h/gh,done_tiles=0;
    for(unsigned by=j->first;by<j->last;by+=8){
        unsigned at=(by/8)*(gw/8)*(alpha?64:32);
        for(unsigned bx=0;bx<gw;bx+=8){
          if(__atomic_load_n(&j->cancelled,__ATOMIC_ACQUIRE)){j->status=-1;return;}
          for(unsigned dy=0;dy<8;dy+=4)for(unsigned dx=0;dx<8;dx+=4){
            unsigned int rgba[16],decoded[16];uint32_t original[16];unsigned char packed[8],coverage[8]={0};
            for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++){
                uint32_t color=filtered(src,w,bx+dx+x,by+dy+y,sx,sy);original[y*4+x]=color;
                unsigned char*p=(unsigned char*)(rgba+y*4+x);
                /* The existing PNG/Magick preparation composites fully
                 * transparent samples to black before ETC colour encoding.
                 * Preserve that encoder input; coverage is stored separately. */
                p[0]=(color&255)?(unsigned char)(color>>24):0;
                p[1]=(color&255)?(unsigned char)(color>>16):0;
                p[2]=(color&255)?(unsigned char)(color>>8):0;p[3]=255;
                coverage[2*x+y/2]|=(unsigned char)(((color&255)?15:0)<<((y&1)*4));
            }
            unsigned block=at/(alpha?16:8);
            unsigned mode=j->modes?(j->modes[block/4]>>(2*(block%4)))&3:4;
            cache.pack(packed,rgba,parameters,mode);
            if(!rg_etc1::unpack_etc1_block(packed,decoded)){j->status=0;return;}
            for(unsigned i=0;i<16;i++)if(original[i]&255){
                const unsigned char*p=(const unsigned char*)(decoded+i);
                for(unsigned k=0;k<3;k++){
                    int error=(int)((original[i]>>(24-k*8))&255)-p[k];completed.squared_error+=(unsigned)(error*error);
                }
                completed.visible_components+=3;
            }
            if(alpha){memcpy(out+at,coverage,8);at+=8;}
            for(unsigned i=0;i<8;i++)out[at++]=packed[7-i];
          }
          __atomic_store_n(&j->done,++done_tiles,__ATOMIC_RELEASE);
          if(j->progress&&!texture_progress(j)){j->status=-1;return;}
        }
    }
}
int fh_install_texture_cmpr_modes(const void*source,size_t source_bytes,unsigned w,unsigned h,unsigned gw,unsigned gh,
                           void*output,size_t capacity,FhInstallTextureResult*result,FhInstallTextureProgress progress,void*context,const unsigned char*modes,size_t mode_bytes){
    if(result)memset(result,0,sizeof(*result));
    if(!initialized||!source||!output||!result||!extent(w)||!extent(h)||!extent(gw)||!extent(gh)||gw>w||gh>h||source_bytes!=w*h/2)return 0;
    if(modes?mode_bytes!=gw*gh/64:mode_bytes!=0)return 0;
    const unsigned char*src=(const unsigned char*)source;unsigned char*out=(unsigned char*)output;
    bool alpha=false;
    for(unsigned at=0;at<source_bytes;at+=8){
        unsigned a=(src[at]<<8)|src[at+1],b=(src[at+2]<<8)|src[at+3];
        if(a<=b)for(unsigned j=0;j<4;j++)for(unsigned i=0;i<4;i++)
            if(((src[at+4+j]>>(6-2*i))&3)==3)alpha=true;
    }
    if(alpha&&(gw!=w||gh!=h))return 0;
    unsigned bytes=gw*gh/(alpha?1:2);
    if(capacity<bytes)return 0;
    /* Reject source/output overlap: encoder writes could corrupt later reads. */
    uintptr_t s=(uintptr_t)source,o=(uintptr_t)output;
    if(s>UINTPTR_MAX-source_bytes||o>UINTPTR_MAX-bytes||(s<o+bytes&&o<s+source_bytes))return 0;
    uintptr_t m=(uintptr_t)modes;
    if(modes&&(m>UINTPTR_MAX-mode_bytes||(m<o+bytes&&o<m+mode_bytes)))return 0;
    FhInstallTextureResult completed={bytes,alpha?13u:12u,gw,gh,0,0};
    unsigned total_tiles=gw*gh/64;
    if(progress&&!progress(context,0,total_tiles))return -1;
    TextureRows main={src,out,w,h,gw,gh,0,gh,alpha,completed,0,0,0,progress,context,nullptr,modes};
    TextureRows background=main;background.progress=nullptr;background.context=nullptr;
    /* Contiguous halves keep the image-local cache useful. Tiny images remain
     * serial; never pay a thread handoff for fewer than 128 native tiles. */
    if(total_tiles>=128&&gh>=16){
        unsigned split=((gh/8)/2)*8;
        background.first=split;
        if(fh_install_worker_submit(texture_rows,&background)){
            main.last=split;main.peer=&background;texture_rows(&main);
            if(main.status!=1)__atomic_store_n(&background.cancelled,1,__ATOMIC_RELEASE);
            while(!fh_install_worker_complete()){
                if(main.status==1&&!texture_progress(&main)){main.status=-1;__atomic_store_n(&background.cancelled,1,__ATOMIC_RELEASE);}
                fh_install_worker_wait_slice();
            }
            fh_install_worker_join();
            if(main.status!=1)return main.status;
            if(background.status!=1)return background.status;
            main.completed.squared_error+=background.completed.squared_error;
            main.completed.visible_components+=background.completed.visible_components;
            /* The final callback remains cancellable even when both halves
             * finish before the producer's final completion check. */
            if(!texture_progress(&main))return -1;
            *result=main.completed;return 1;
        }
    }
    texture_rows(&main);if(main.status!=1)return main.status;
    *result=main.completed;return 1;
}
int fh_install_texture_cmpr(const void*source,size_t bytes,unsigned w,unsigned h,unsigned gw,unsigned gh,
 void*output,size_t capacity,FhInstallTextureResult*result,FhInstallTextureProgress progress,void*context){
 return fh_install_texture_cmpr_modes(source,bytes,w,h,gw,gh,output,capacity,result,progress,context,nullptr,0);
}
