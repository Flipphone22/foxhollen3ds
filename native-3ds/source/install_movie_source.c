#include "fh_install_movie_source.h"
#include "fh_install_movie_decode.h"
#include <mbedtls/sha256.h>
#include <stdlib.h>
#include <string.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int fail(FhInstallMovieSource*s,const char*why){snprintf(s->error,sizeof(s->error),"%s",why);return 0;}
void fh_install_movie_source_close(FhInstallMovieSource*s){if(s){free(s->index);free(s->frame);s->index=NULL;s->frame=NULL;s->disc=NULL;}}
int fh_install_movie_source_begin(FhInstallMovieSource*s,FhInstallDisc*d,FhInstallMovieSourceProgress progress,void*context){
 if(!s||!d||!d->file)return 0;fh_install_movie_source_close(s);memset(s,0,sizeof(*s));s->disc=d;
 int file=fh_install_disc_find(d,"/starfox.thp");FhInstallEntry entry;
 if(file<0||!fh_install_disc_entry(d,file,&entry)||entry.directory||entry.bytes!=FH_INSTALL_MOVIE_SOURCE_BYTES){fh_install_movie_source_close(s);return fail(s,"Original PAL movie is missing or has the wrong extent.");}
 s->file=file;s->disc_offset=entry.offset;s->bytes=entry.bytes;
 if(!fh_install_disc_read(d,file,0,s->header,48)||memcmp(s->header,"THP\0\0\1\0\0",8)||word(s->header+20)!=FH_INSTALL_MOVIE_FRAMES||
    word(s->header+32)!=48||word(s->header+36)||word(s->header+40)!=88||word(s->header+28)!=s->bytes-88||
    !word(s->header+8)||word(s->header+8)>FH_INSTALL_MOVIE_VIDEO_CAP||!fh_install_disc_read(d,file,48,s->info,40)||
    word(s->info)!=2||s->info[4]!=0||s->info[5]!=1||word(s->info+20)!=512||word(s->info+24)!=384||word(s->info+28)!=2||word(s->info+32)!=32000){
  fh_install_movie_source_close(s);return fail(s,"Unsupported original PAL movie header.");
 }
 unsigned char*buffer=malloc(FH_INSTALL_READ_CHUNK);mbedtls_sha256_context hash;mbedtls_sha256_init(&hash);int result=0;
 if(!buffer||mbedtls_sha256_starts_ret(&hash,0)){fail(s,"Not enough startup memory for movie fingerprint.");goto done;}
 if(progress&&!progress(context,FH_INSTALL_MOVIE_HASH,0,s->bytes)){result=-1;goto done;}
 for(unsigned at=0;at<s->bytes;){unsigned n=s->bytes-at;if(n>FH_INSTALL_READ_CHUNK)n=FH_INSTALL_READ_CHUNK;
  if(!fh_install_disc_read(d,file,at,buffer,n)||mbedtls_sha256_update_ret(&hash,buffer,n)){fail(s,"Cannot fingerprint original movie.");goto done;}at+=n;
  if(progress&&!progress(context,FH_INSTALL_MOVIE_HASH,at,s->bytes)){result=-1;goto done;}
 }
 {
  static const unsigned char expected[32]={0x19,0xd0,0x8f,0xf1,0xab,0x4c,0x18,0x58,0x93,0xdf,0x57,0xd8,0x9e,0x48,0x8a,0xa0,0xb5,0xc4,0xe6,0x56,0xbe,0xbb,0x5d,0x70,0x31,0xde,0x65,0x22,0x8a,0x21,0x66,0xe6};
  if(mbedtls_sha256_finish_ret(&hash,s->sha)||memcmp(s->sha,expected,32)){fail(s,"Original PAL movie fingerprint does not match.");goto done;}
 }
 s->index=calloc(FH_INSTALL_MOVIE_FRAMES,sizeof(*s->index));if(!s->index){fail(s,"Not enough startup memory for movie index.");goto done;}
 if(progress&&!progress(context,FH_INSTALL_MOVIE_INDEX,0,FH_INSTALL_MOVIE_FRAMES)){result=-1;goto done;}
 {
  unsigned at=word(s->header+40),bytes=word(s->header+24),previous=0,first_previous=0,maximum=0,window=~0u,window_bytes=0;
  for(unsigned i=0;i<FH_INSTALL_MOVIE_FRAMES;i++){
   if(bytes<16||(bytes&31)||bytes>word(s->header+8)||at>s->bytes||bytes>s->bytes-at){fail(s,"Invalid original movie frame extent.");goto done;}
   unsigned char prefix[16];
   for(unsigned n=0;n<16;){
    unsigned offset=at+n;
    if(window==~0u||offset<window||offset-window>=window_bytes){window=offset;window_bytes=s->bytes-offset;if(window_bytes>FH_INSTALL_READ_CHUNK)window_bytes=FH_INSTALL_READ_CHUNK;
     if(!fh_install_disc_read(d,file,window,buffer,window_bytes)){fail(s,"Cannot read original movie index.");goto done;}
    }
    unsigned take=window_bytes-(offset-window);if(take>16-n)take=16-n;memcpy(prefix+n,buffer+offset-window,take);n+=take;
   }
   unsigned next=word(prefix),prev=word(prefix+4),video=word(prefix+8),audio=word(prefix+12);
   if((i&&prev!=previous)||video<4||video>bytes-16||audio>bytes-16-video||video&3||!audio){fail(s,"Invalid original movie component sizes or links.");goto done;}
   if(!i)first_previous=prev;s->index[i]=(FhInstallMovieIndex){at,bytes,video,audio};if(bytes>maximum)maximum=bytes;
   previous=bytes;at+=bytes;bytes=next;
   if(progress&&!progress(context,FH_INSTALL_MOVIE_INDEX,i+1,FH_INSTALL_MOVIE_FRAMES)){result=-1;goto done;}
  }
  if(at!=s->bytes||bytes!=s->index[0].bytes||previous!=first_previous||maximum!=word(s->header+8)||s->index[FH_INSTALL_MOVIE_FRAMES-1].offset!=word(s->header+44)){fail(s,"Original movie chain does not cover the complete stream.");goto done;}
 }
 result=1;
done:mbedtls_sha256_free(&hash);free(buffer);if(result!=1)fh_install_movie_source_close(s);return result;
}
int fh_install_movie_source_read(FhInstallMovieSource*s,unsigned frame){
 if(!s||!s->disc||!s->index||frame>=FH_INSTALL_MOVIE_FRAMES)return 0;
 if(!s->frame){s->frame=malloc(word(s->header+8));if(!s->frame)return fail(s,"Not enough startup memory for original movie frame.");}
 FhInstallMovieIndex*i=s->index+frame;
 if(!fh_install_disc_read(s->disc,s->file,i->offset,s->frame,i->bytes)||word(s->frame+8)!=i->video||word(s->frame+12)!=i->audio||
    word(s->frame)!=s->index[(frame+1)%FH_INSTALL_MOVIE_FRAMES].bytes||word(s->frame+4)!=s->index[(frame+FH_INSTALL_MOVIE_FRAMES-1)%FH_INSTALL_MOVIE_FRAMES].bytes)return fail(s,"Original movie frame changed after indexing.");
 return 1;
}
