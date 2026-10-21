#include "fh_install_movie_frame.h"
#include "fh_install_texture.h"
#include "fh_install_worker.h"
#include "../vendor/rg_etc1/rg_etc1.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <lz4.h>
#include <zlib.h>
static void word(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
extern "C" void fh_install_movie_frame_close(FhInstallMovieFrame*out){if(out){std::free(out->data);std::memset(out,0,sizeof(*out));}}
static int fail(FhInstallMovieFrame*out,const char*why){std::snprintf(out->error,sizeof(out->error),"%s",why);return 0;}
struct Resize{FhInstallMovieFrameProgress function;void*context;};
static int resize_progress(void*context,unsigned done,unsigned total){Resize*p=static_cast<Resize*>(context);return !p->function||p->function(p->context,FH_INSTALL_MOVIE_RESIZE,done,total);}
struct EncodeRows {
 const unsigned char*rgb;unsigned char*raw;unsigned w,h,first,last,stride;
 uint64_t squared_error,components;unsigned done,cancelled,base_done,reported;
 int result;FhInstallMovieFrameProgress progress;void*context;EncodeRows*peer;
};
static int encode_progress(EncodeRows*j){
 unsigned n=__atomic_load_n(&j->done,__ATOMIC_RELAXED)+j->base_done;
 if(j->peer)n+=__atomic_load_n(&j->peer->done,__ATOMIC_ACQUIRE);
 if(j->progress&&!j->progress(j->context,FH_INSTALL_MOVIE_ENCODE,n,j->w*j->w/64))return 0;
 j->reported=n;return 1;
}
static void encode_rows(void*context){
 EncodeRows*j=static_cast<EncodeRows*>(context);j->result=1;unsigned completed_tiles=0;
 rg_etc1::etc1_pack_params params;params.m_quality=rg_etc1::cHighQuality;
 for(unsigned by=j->first;by<j->last;by+=8*j->stride){
  unsigned at=(by/8)*(j->w/8)*32;
  for(unsigned bx=0;bx<j->w;bx+=8){
   if(__atomic_load_n(&j->cancelled,__ATOMIC_ACQUIRE)){j->result=-1;return;}
   for(unsigned dy=0;dy<8;dy+=4)for(unsigned dx=0;dx<8;dx+=4){
    /* Padding runs on main after both visible-row jobs have joined. */
    if(by+dy>=j->h&&(by>j->h||dy)){
     unsigned first=(j->h/8)*(j->w/8)*32+(bx/8)*32+(dx/4)*8;
     std::memcpy(j->raw+at,j->raw+first,8);at+=8;continue;
    }
    unsigned int pixels[16],decoded[16];unsigned char block[8];
    for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++){
     unsigned row=by+dy+y;if(row>=j->h)row=j->h-1;
     const unsigned char*p=j->rgb+(row*j->w+bx+dx+x)*3;
     unsigned char*q=reinterpret_cast<unsigned char*>(pixels+y*4+x);q[0]=p[0];q[1]=p[1];q[2]=p[2];q[3]=255;
    }
    rg_etc1::pack_etc1_block(block,pixels,params);
    if(!rg_etc1::unpack_etc1_block(block,decoded)){j->result=0;return;}
    if(by+dy<j->h)for(unsigned i=0;i<16;i++)for(unsigned c=0;c<3;c++){
     int error=reinterpret_cast<unsigned char*>(decoded+i)[c]-reinterpret_cast<unsigned char*>(pixels+i)[c];
     j->squared_error+=error*error;j->components++;
    }
    for(unsigned i=0;i<8;i++)j->raw[at++]=block[7-i];
   }
   __atomic_store_n(&j->done,++completed_tiles,__ATOMIC_RELEASE);
   if(j->progress&&!encode_progress(j)){j->result=-1;return;}
  }
 }
}
extern "C" int fh_install_movie_frame(FhInstallMovieFrame*out,const FhInstallMovieImage*image,unsigned scale,FhInstallMovieFrameProgress progress,void*context){
 if(!out)return 0;fh_install_movie_frame_close(out);
 if(!image||!image->rgb||image->width!=512||image->height!=384||image->bytes!=512*384*3||(scale!=1&&scale!=2))return fail(out,"Invalid original movie RGB frame.");
 unsigned w=512/scale,h=384/scale,raw_bytes=w*w/2;const unsigned char*rgb=image->rgb;
 unsigned char*small=nullptr,*raw=nullptr,*packed=nullptr;int result=0;
 if(scale==2){
  small=static_cast<unsigned char*>(std::malloc(w*h*3));if(!small){fail(out,"Not enough startup memory for half movie frame.");goto done;}
  {Resize tracking={progress,context};result=fh_install_movie_half_rgb(small,w*h*3,image->rgb,image->bytes,resize_progress,&tracking);}
  if(result!=1){if(!result)fail(out,"Movie half resize failed.");goto done;}result=0;rgb=small;
 }
 raw=static_cast<unsigned char*>(std::malloc(raw_bytes));if(!raw){fail(out,"Not enough startup memory for native movie texture.");goto done;}
 fh_install_texture_init();
 if(progress&&!progress(context,FH_INSTALL_MOVIE_ENCODE,0,w*w/64)){result=-1;goto done;}
 {
  EncodeRows background={rgb,raw,w,h,0,h,2,0,0,0,0,0,0,0,nullptr,nullptr,nullptr};
  EncodeRows main={rgb,raw,w,h,8,h,2,0,0,0,0,0,0,0,progress,context,&background};
  if(fh_install_worker_submit(encode_rows,&background)){
   /* Alternate native tile rows balance sky/detail regions across the two
    * cores. The worker owns disjoint output rows and never calls the UI. */
   encode_rows(&main);
   if(main.result!=1)__atomic_store_n(&background.cancelled,1,__ATOMIC_RELEASE);
   while(!fh_install_worker_complete()){
    if(main.result==1&&!encode_progress(&main)){main.result=-1;__atomic_store_n(&background.cancelled,1,__ATOMIC_RELEASE);}
    fh_install_worker_wait_slice();
   }
   fh_install_worker_join(); /* No borrowed input/output can retire before this. */
   result=main.result!=1?main.result:background.result;
   if(result!=1){if(!result)fail(out,"Invalid native movie ETC1 block.");goto done;}
   out->squared_error=main.squared_error+background.squared_error;out->components=main.components+background.components;
   EncodeRows padding={rgb,raw,w,h,h,w,1,0,0,0,0,h*w/64,0,0,progress,context,nullptr};
   encode_rows(&padding);result=padding.result;
  }else{
   main.first=0;main.last=w;main.stride=1;main.peer=nullptr;encode_rows(&main);result=main.result;
   out->squared_error=main.squared_error;out->components=main.components;
  }
  if(result!=1){if(!result)fail(out,"Invalid native movie ETC1 block.");goto done;}result=0;
 }
 if(progress&&!progress(context,FH_INSTALL_MOVIE_COMPRESS,0,1)){result=-1;goto done;}
 {
  unsigned capacity=LZ4_compressBound(raw_bytes);packed=static_cast<unsigned char*>(std::malloc(capacity));if(!packed){fail(out,"Not enough startup memory for movie LZ4.");goto done;}
  int count=LZ4_compress_default(reinterpret_cast<const char*>(raw),reinterpret_cast<char*>(packed),raw_bytes,capacity);
  if(count<=0){fail(out,"Movie texture compression failed.");goto done;}
  unsigned codec=unsigned(count)+4<raw_bytes?2:0,payload=codec?unsigned(count)+4:raw_bytes;
  out->data=static_cast<unsigned char*>(std::malloc(32+payload));if(!out->data){fail(out,"Not enough startup memory for prepared movie frame.");goto done;}
  std::memcpy(out->data,"FHPICA01",8);word(out->data+8,w);word(out->data+12,h);word(out->data+16,w);word(out->data+20,w);word(out->data+24,raw_bytes);word(out->data+28,codec);
  std::memcpy(out->data+32,codec?packed:raw,codec?unsigned(count):raw_bytes);
  if(codec)word(out->data+32+count,static_cast<unsigned>(crc32(0,raw,raw_bytes)));
  if(progress&&!progress(context,FH_INSTALL_MOVIE_COMPRESS,1,1)){result=-1;goto done;}
  out->bytes=32+payload;out->width=w;out->height=h;out->raw_bytes=raw_bytes;out->codec=codec;result=1;
 }
done:
 std::free(small);std::free(raw);std::free(packed);
 if(result!=1){std::free(out->data);out->data=nullptr;out->bytes=out->width=out->height=out->raw_bytes=out->codec=0;out->squared_error=out->components=0;}return result;
}
