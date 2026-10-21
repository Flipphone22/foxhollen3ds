#include "fh_install_movie_decode.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
typedef struct{struct jpeg_error_mgr base;jmp_buf jump;unsigned warnings;char error[112];}Error;
typedef struct{struct jpeg_decompress_struct jpeg;Error error;unsigned char*stream;}Decoder;
static void fatal(j_common_ptr info){Error*e=(Error*)info->err;char message[JMSG_LENGTH_MAX];e->base.format_message(info,message);snprintf(e->error,sizeof(e->error),"%.111s",message);longjmp(e->jump,1);}
static void emit(j_common_ptr info,int level){if(level<0){Error*e=(Error*)info->err;char message[JMSG_LENGTH_MAX];e->warnings++;e->base.format_message(info,message);snprintf(e->error,sizeof(e->error),"%.111s",message);}}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
void fh_install_movie_image_close(FhInstallMovieImage*out){if(out){free(out->rgb);memset(out,0,sizeof(*out));}}
static int fail(FhInstallMovieImage*out,const char*why){snprintf(out->error,sizeof(out->error),"%.111s",why);return 0;}
/* THP removes JPEG entropy byte stuffing. Its EOI marker remains, followed by
 * zero padding to a four-byte boundary. Preserve EOI instead of stuffing it as
 * entropy (the older shim does that and suppresses libjpeg's warning). */
static unsigned rebuild(unsigned char*out,const unsigned char*in,unsigned bytes){
 if(bytes<4||(bytes&3)||in[0]!=255||in[1]!=0xd8)return 0;
 unsigned end=bytes;while(end>bytes-3&&in[end-1]==0)end--;
 if(end<4||in[end-2]!=255||in[end-1]!=0xd9)return 0;end-=2;
 unsigned at=2,scan=0;
 while(at<bytes){
  if(in[at++]!=255)return 0;while(at<bytes&&in[at]==255)at++;
  if(at==bytes)return 0;unsigned marker=in[at++];
  if(marker==0xd8||(marker>=0xd0&&marker<=0xd7))continue;
  if(marker==0||marker==0xd9||2>bytes-at)return 0;unsigned n=half(in+at);
  if(n<2||n>bytes-at)return 0;
  if(marker==0xda){scan=at+n;break;}at+=n;
 }
 if(!scan||scan>=end)return 0;memcpy(out,in,scan);unsigned written=scan;
 for(unsigned i=scan;i<end;i++){out[written++]=in[i];if(in[i]==255)out[written++]=0;}
 out[written++]=255;out[written++]=0xd9;return written;
}
int fh_install_movie_decode(FhInstallMovieImage*out,const void*source,unsigned bytes,FhInstallMovieDecodeProgress progress,void*context){
 if(!out)return 0;fh_install_movie_image_close(out);
 if(!source||bytes<4||bytes>FH_INSTALL_MOVIE_VIDEO_CAP)return fail(out,"Invalid PAL movie frame size.");
 Decoder*d=calloc(1,sizeof(*d));if(!d)return fail(out,"Not enough startup memory for JPEG decoder.");
 int result=0;d->stream=malloc(bytes*2+2);if(!d->stream){fail(out,"Not enough startup memory for JPEG reconstruction.");goto done;}
 unsigned count=rebuild(d->stream,source,bytes);if(!count){fail(out,"Invalid THP JPEG marker or scan extent.");goto done;}
 d->jpeg.err=jpeg_std_error(&d->error.base);d->error.base.error_exit=fatal;d->error.base.emit_message=emit;
 /* Decoder state is heap-owned: longjmp must not read changed automatic JPEG
  * structs with indeterminate values. No setjmp crosses a C++ frame. */
 if(setjmp(d->error.jump)){fail(out,d->error.error);goto done;}
 jpeg_create_decompress(&d->jpeg);d->jpeg.mem->max_memory_to_use=2*1024*1024;
 jpeg_mem_src(&d->jpeg,d->stream,count);
 if(jpeg_read_header(&d->jpeg,TRUE)!=JPEG_HEADER_OK||d->jpeg.image_width!=FH_INSTALL_MOVIE_WIDTH||d->jpeg.image_height!=FH_INSTALL_MOVIE_HEIGHT||
    d->jpeg.num_components!=3||d->jpeg.data_precision!=8||d->jpeg.progressive_mode||d->jpeg.arith_code){fail(out,"Unsupported PAL movie JPEG shape or coding.");goto done;}
 for(unsigned i=0;i<3;i++)if(d->jpeg.comp_info[i].h_samp_factor<1||d->jpeg.comp_info[i].h_samp_factor>2||d->jpeg.comp_info[i].v_samp_factor<1||d->jpeg.comp_info[i].v_samp_factor>2){fail(out,"Unsupported movie chroma sampling.");goto done;}
 d->jpeg.out_color_space=JCS_RGB;d->jpeg.dct_method=JDCT_ISLOW;
 if(!jpeg_start_decompress(&d->jpeg)||d->jpeg.output_width!=FH_INSTALL_MOVIE_WIDTH||d->jpeg.output_height!=FH_INSTALL_MOVIE_HEIGHT||d->jpeg.output_components!=3){fail(out,"Invalid decoded movie dimensions.");goto done;}
 out->rgb=malloc(FH_INSTALL_MOVIE_WIDTH*FH_INSTALL_MOVIE_HEIGHT*3);if(!out->rgb){fail(out,"Not enough startup memory for movie RGB.");goto done;}
 if(progress&&!progress(context,0,FH_INSTALL_MOVIE_HEIGHT)){result=-1;goto done;}
 while(d->jpeg.output_scanline<FH_INSTALL_MOVIE_HEIGHT){
  JSAMPROW row=out->rgb+d->jpeg.output_scanline*FH_INSTALL_MOVIE_WIDTH*3;
  if(jpeg_read_scanlines(&d->jpeg,&row,1)!=1){fail(out,"Movie decoder made no scanline progress.");goto done;}
  if(progress&&d->jpeg.output_scanline%16==0&&!progress(context,d->jpeg.output_scanline,FH_INSTALL_MOVIE_HEIGHT)){result=-1;goto done;}
 }
 if(!jpeg_finish_decompress(&d->jpeg)||d->error.warnings){fail(out,d->error.error[0]?d->error.error:"Incomplete or damaged movie JPEG.");goto done;}
 out->width=FH_INSTALL_MOVIE_WIDTH;out->height=FH_INSTALL_MOVIE_HEIGHT;out->bytes=out->width*out->height*3;result=1;
done:
 if(d->jpeg.mem)jpeg_destroy_decompress(&d->jpeg);free(d->stream);free(d);
 if(result!=1){free(out->rgb);out->rgb=NULL;out->bytes=out->width=out->height=0;}return result;
}
