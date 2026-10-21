#include "fh_install_movie_rgb8.h"
#include <lz4.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

enum{W=256,H=192,RAW=W*W*3,SMALL=W*H*3};
typedef struct{FhInstallMovieFrameProgress call;void*context;} Progress;
static int report(Progress*p,unsigned phase,unsigned done,unsigned total){return !p->call||p->call(p->context,phase,done,total);}
static int resized(void*context,unsigned done,unsigned total){return report(context,FH_INSTALL_MOVIE_RESIZE,done,total);}
static void word(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static int fail(FhInstallMovieFrame*out,const char*why){snprintf(out->error,sizeof(out->error),"%s",why);return 0;}

int fh_install_movie_rgb8_frame(FhInstallMovieFrame*out,const FhInstallMovieImage*image,FhInstallMovieFrameProgress progress,void*context){
    if(!out)return 0;
    fh_install_movie_frame_close(out);
    if(!image||!image->rgb||image->width!=512||image->height!=384||image->bytes!=512*384*3)
        return fail(out,"Invalid original movie RGB frame.");
    unsigned char*small=malloc(SMALL),*raw=malloc(RAW),*packed=NULL;
    int result=0;Progress tracking={progress,context};
    if(!small||!raw){fail(out,"Not enough startup memory for RGB8 movie frame.");goto done;}
    result=fh_install_movie_half_rgb(small,SMALL,image->rgb,image->bytes,resized,&tracking);
    if(result!=1){if(!result)fail(out,"Movie half resize failed.");goto done;}
    if(!report(&tracking,FH_INSTALL_MOVIE_ENCODE,0,W*W/64)){result=-1;goto done;}
    unsigned at=0,tiles=0;
    for(unsigned by=0;by<W;by+=8)for(unsigned bx=0;bx<W;bx+=8){
        /* PICA RGB8: BGR components, Morton texel order within an 8x8 tile.
         * Preserve the existing bottom-row padding and source orientation. */
        for(unsigned i=0;i<64;i++){
            unsigned x=(i&1)|((i>>1)&2)|((i>>2)&4);
            unsigned y=((i>>1)&1)|((i>>2)&2)|((i>>3)&4),row=by+y;
            if(row>=H)row=H-1;
            const unsigned char*p=small+(row*W+bx+x)*3;
            raw[at++]=p[2];raw[at++]=p[1];raw[at++]=p[0];
        }
        /* Report every 16 tiles (1024 copied pixels), not every 8x8 tile.
         * The synchronous producer still owns UI/cancellation; there are no
         * file operations or worker waits inside this bounded copy interval.
         * Keep explicit zero/final progress and all source pixels unchanged. */
        ++tiles;
        if((tiles%16==0||tiles==W*W/64)&&
           !report(&tracking,FH_INSTALL_MOVIE_ENCODE,tiles,W*W/64)){result=-1;goto done;}
    }
    if(!report(&tracking,FH_INSTALL_MOVIE_COMPRESS,0,1)){result=-1;goto done;}
    result=0;
    unsigned capacity=LZ4_compressBound(RAW);packed=malloc(capacity);
    if(!packed){fail(out,"Not enough startup memory for RGB8 movie LZ4.");goto done;}
    int count=LZ4_compress_default((const char*)raw,(char*)packed,RAW,capacity);
    if(count<=0){fail(out,"Movie RGB8 compression failed.");goto done;}
    unsigned codec=(unsigned)count+4<RAW?2:0,payload=codec?(unsigned)count+4:RAW;
    out->data=malloc(32+payload);
    if(!out->data){fail(out,"Not enough startup memory for prepared RGB8 movie.");goto done;}
    memcpy(out->data,"FHRGB801",8);word(out->data+8,W);word(out->data+12,H);
    word(out->data+16,W);word(out->data+20,W);word(out->data+24,RAW);word(out->data+28,codec);
    memcpy(out->data+32,codec?packed:raw,codec?(unsigned)count:RAW);
    if(codec)word(out->data+32+count,(unsigned)crc32(0,raw,RAW));
    if(!report(&tracking,FH_INSTALL_MOVIE_COMPRESS,1,1)){result=-1;goto done;}
    out->bytes=32+payload;out->width=W;out->height=H;out->raw_bytes=RAW;out->codec=codec;
    out->components=SMALL; /* Exactly lossless against the unchanged resize. */
    result=1;
done:
    free(small);free(raw);free(packed);
    if(result!=1){
        free(out->data);out->data=NULL;out->bytes=out->width=out->height=out->raw_bytes=out->codec=0;
        out->components=out->squared_error=0;
    }
    return result;
}
