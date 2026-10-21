#include "fh_install_movie_pack.h"
#include "fh_install_assembly.h"
#include "fh_install_readback.h"
#include "fh_install_movie_frame.h"
#include "fh_install_movie_rgb8.h"
#include "fh_prepared_movie.h"
#include <mbedtls/sha256.h>
#include <lz4.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
enum{PREFIX=148,HEADER=80,SPOOL_CAP=300*1024*1024,PACK_CAP=300*1024*1024,SCAN_CURSOR=0x4d565331,PACK_CURSOR=0x4d565032,STAGE=16384};
typedef struct{unsigned at,video,audio,vc,ac,source,source_crc;unsigned char sha[32];}Entry;
typedef struct{FhInstallMovieProgress function;void*context;unsigned frame,phase;}Progress;
typedef struct{FhInstallMovieSource*source;unsigned scale;Entry*entries;unsigned rgb8;}Validate;
static unsigned le(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static unsigned be(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void little(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static void big(unsigned char*p,unsigned n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static int fail(FhInstallTransaction*t,const char*why){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",why);return 0;}
static int report(Progress*p,unsigned phase,unsigned done,unsigned total){return !p->function||p->function(p->context,phase,p->frame,FH_INSTALL_MOVIE_FRAMES,done,total);}
static int source_progress(void*ctx,unsigned phase,unsigned done,unsigned total){return report(ctx,phase==FH_INSTALL_MOVIE_HASH?FH_INSTALL_MOVIE_PREP_HASH:FH_INSTALL_MOVIE_PREP_INDEX,done,total);}
static int decode_progress(void*ctx,unsigned done,unsigned total){return report(ctx,FH_INSTALL_MOVIE_PREP_DECODE,done,total);}
static int frame_progress(void*ctx,unsigned phase,unsigned done,unsigned total){return report(ctx,FH_INSTALL_MOVIE_PREP_RESIZE+phase,done,total);}
static void prefix(unsigned char*h,const FhInstallMovieSource*s,unsigned scale,unsigned rgb8){
 memset(h,0,PREFIX);memcpy(h,rgb8?"FHMOVS02":"FHMOVS01",8);little(h+8,scale);little(h+12,FH_INSTALL_MOVIE_FRAMES);little(h+16,s->disc_offset);little(h+20,s->bytes);
 memcpy(h+24,s->sha,32);memcpy(h+56,s->header,48);memcpy(h+104,s->info,40);little(h+144,(unsigned)crc32(0,h,144));
}
static int video_valid(const unsigned char*p,unsigned bytes,unsigned scale,unsigned rgb8){
 if(fh_movie_video_format(p,bytes)!=(rgb8?FH_TEXTURE_PICA_RGB8:FH_TEXTURE_PICA_ETC1))return 0;
 unsigned raw=fh_movie_video_bytes(p,bytes,512,384);if(!raw||le(p+8)!=512/scale||le(p+28)==1)return 0;
 if(!le(p+28))return 1;
 unsigned char*decoded=malloc(raw);if(!decoded)return 0;
 int ok=LZ4_decompress_safe((const char*)p+32,(char*)decoded,bytes-36,raw)==(int)raw&&(unsigned)crc32(0,decoded,raw)==le(p+bytes-4);free(decoded);return ok;
}
static int record(const unsigned char*h,Entry*e,unsigned ordinal,unsigned at,const FhInstallMovieSource*s){
 if(memcmp(h,"FHMVFR01",8)||le(h+8)!=ordinal||le(h+68)||le(h+72)||le(h+76)!=(unsigned)crc32(0,h,76))return 0;
 e->at=at;e->video=le(h+12);e->audio=le(h+16);e->vc=le(h+20);e->ac=le(h+24);e->source=le(h+28);e->source_crc=le(h+32);memcpy(e->sha,h+36,32);
 return e->video>=32&&e->video<=196640&&e->audio==s->index[ordinal].audio&&e->source==s->index[ordinal].video;
}
static int spool_read(FILE*f,FhInstallReadback*check,void*data,unsigned bytes){return check?fh_install_readback_read(check,data,bytes):fread(data,1,bytes,f)==bytes;}
static int read_spool(FILE*f,const Validate*v,Entry*entries,unsigned*count,FhInstallReadback*check){
 long length;
 if(check)length=fh_install_readback_size(check);
 else{if(fseek(f,0,SEEK_END))return 0;length=ftell(f);if(fseek(f,0,SEEK_SET))return 0;}
 if(length<PREFIX||length>SPOOL_CAP)return 0;
 unsigned char expected[PREFIX],actual[PREFIX];prefix(expected,v->source,v->scale,v->rgb8);if(!spool_read(f,check,actual,PREFIX)||memcmp(actual,expected,PREFIX))return 0;
 unsigned at=PREFIX,n=0;unsigned char h[HEADER];
 while(at<(unsigned)length){
  Entry e;if(n==FH_INSTALL_MOVIE_FRAMES||HEADER>(unsigned)length-at||!spool_read(f,check,h,HEADER)||!record(h,&e,n,at,v->source)||e.video+e.audio>(unsigned)length-at-HEADER)return 0;
  if(check){
   unsigned char*p=malloc(e.video+e.audio);if(!p)return 0;
   int ok=spool_read(f,check,p,e.video+e.audio)&&(unsigned)crc32(0,p,e.video)==e.vc&&(unsigned)crc32(0,p+e.video,e.audio)==e.ac&&video_valid(p,e.video,v->scale,v->rgb8);free(p);if(!ok)return 0;
  }else if(fseek(f,e.video+e.audio,SEEK_CUR))return 0;
  if(entries)entries[n]=e;n++;at+=HEADER+e.video+e.audio;
 }*count=n;return check||!ferror(f);
}
static int spool_validate(FhInstallReadback*check,void*ctx){unsigned n;return read_spool(NULL,ctx,NULL,&n,check)&&n==FH_INSTALL_MOVIE_FRAMES;}
static unsigned frame_bytes(const Entry*e){return (16+e->video+e->audio+31)&~31u;}
static void movie_header(unsigned char*h,const Validate*v,unsigned*total){
 FhInstallMovieSource*s=v->source;unsigned maximum=0,at=192;
 memset(h,0,192);memcpy(h,s->header,48);memcpy(h+48,s->info,40);
 for(unsigned i=0;i<FH_INSTALL_MOVIE_FRAMES;i++){unsigned bytes=frame_bytes(v->entries+i);if(bytes>maximum)maximum=bytes;if(i==FH_INSTALL_MOVIE_FRAMES-1)big(h+44,at);at+=bytes;}
 big(h+8,maximum);big(h+24,frame_bytes(v->entries));big(h+28,at-192);big(h+32,48);big(h+36,0);big(h+40,192);
 memcpy(h+128,v->rgb8?"FHRGB801":"FHPICA01",8);little(h+136,s->disc_offset);little(h+140,s->bytes);little(h+144,FH_INSTALL_MOVIE_FRAMES);little(h+148,FH_INSTALL_MOVIE_FRAMES);little(h+152,v->rgb8?1:12);little(h+156,2);memcpy(h+160,s->sha,32);*total=at;
}
static int pack_validate(FhInstallReadback*check,void*ctx){
 Validate*v=ctx;int ok=0;unsigned total;unsigned char expected[192],actual[192],*payload=NULL;movie_header(expected,v,&total);
 if(fh_install_readback_size(check)!=total||!fh_install_readback_read(check,actual,192)||memcmp(actual,expected,192)||
  !fh_movie_header_valid(actual,total,v->source->header,v->source->info,v->source->disc_offset,v->source->bytes))goto done;
 payload=malloc(be(actual+8));if(!payload)goto done;
 for(unsigned i=0;i<FH_INSTALL_MOVIE_FRAMES;i++){
  Entry*e=v->entries+i;unsigned bytes=frame_bytes(e);if(!fh_install_readback_read(check,payload,bytes)||be(payload)!=frame_bytes(v->entries+(i+1)%FH_INSTALL_MOVIE_FRAMES)||
   be(payload+4)!=frame_bytes(v->entries+(i+FH_INSTALL_MOVIE_FRAMES-1)%FH_INSTALL_MOVIE_FRAMES)||be(payload+8)!=e->video||be(payload+12)!=e->audio||
   (unsigned)crc32(0,payload+16,e->video)!=e->vc||(unsigned)crc32(0,payload+16+e->video,e->audio)!=e->ac||!video_valid(payload+16,e->video,v->scale,v->rgb8))goto done;
  for(unsigned j=16+e->video+e->audio;j<bytes;j++)if(payload[j])goto done;
 }ok=1;
done:free(payload);return ok;
}
static int copy(FhInstallTransaction*t,FILE*f,unsigned at,unsigned bytes){unsigned char b[STAGE];if(fseek(f,at,SEEK_SET))return fail(t,"Cannot seek movie spool.");while(bytes){unsigned n=bytes>sizeof(b)?sizeof(b):bytes;if(fread(b,1,n,f)!=n)return fail(t,"Cannot read movie spool.");if(!fh_install_artifact_write(t,b,n))return 0;bytes-=n;}return 1;}
static int prepare(FhInstallTransaction*t,FhInstallDisc*d,unsigned spool,unsigned output,unsigned scale,unsigned rgb8,FhInstallMovieProgress progress,void*context){
 if(!t||t->failed||t->output||!d||!d->file)return 0;
 if(spool>=FH_INSTALL_ARTIFACTS||output>=FH_INSTALL_ARTIFACTS||spool==output||(scale!=1&&scale!=2))return fail(t,"Invalid movie preparation recipe.");
 if(t->state.complete&(1u<<output))return 1;
 FhInstallMovieSource source={0};FhInstallMovieImage image={0};FhInstallMovieFrame frame={0};Progress tracking={progress,context,0,0};Entry*entries=NULL;FILE*input=NULL;int result=0;unsigned completed=0,encoding=0,metadata_ready=0;
 result=fh_install_movie_source_begin(&source,d,source_progress,&tracking);if(result!=1){if(!result)fail(t,source.error);goto done;}result=0;
 entries=calloc(FH_INSTALL_MOVIE_FRAMES,sizeof(*entries));if(!entries){fail(t,"Not enough startup memory for prepared movie index.");goto done;}
 Validate validation={&source,scale,entries,rgb8};
 if(!(t->state.complete&(1u<<spool))){
  if(!fh_install_artifact_begin(t,spool))goto done;
  if(!t->written){unsigned char h[PREFIX];prefix(h,&source,scale,rgb8);if(!fh_install_artifact_write(t,h,PREFIX)||!fh_install_artifact_checkpoint(t,0,SCAN_CURSOR))goto done;}
  unsigned count;
  if(!read_spool(t->output,&validation,entries,&count,0)||t->state.cursor[0]!=count||t->state.cursor[1]!=SCAN_CURSOR||fseek(t->output,t->written,SEEK_SET)){fail(t,"Invalid movie conversion checkpoint.");goto done;}
  completed=count;encoding=1;
  for(unsigned i=count;i<FH_INSTALL_MOVIE_FRAMES;i++){
   tracking.frame=i;if(!report(&tracking,FH_INSTALL_MOVIE_PREP_WRITE,i,FH_INSTALL_MOVIE_FRAMES)){result=-1;goto done;}
   if(!fh_install_movie_source_read(&source,i)){fail(t,source.error);goto done;}
   FhInstallMovieIndex*original=source.index+i;const unsigned char*video=source.frame+16,*audio=video+original->video;unsigned char sha[32];unsigned crc=(unsigned)crc32(0,video,original->video);
   if(mbedtls_sha256_ret(video,original->video,sha,0)){fail(t,"Cannot fingerprint original movie frame.");goto done;}
   unsigned reuse=i;for(unsigned j=0;j<i;j++)if(entries[j].source==original->video&&entries[j].source_crc==crc&&!memcmp(entries[j].sha,sha,32)){reuse=j;break;}
   if(reuse<i){
    Entry*e=entries+reuse;frame.data=malloc(e->video);if(!frame.data){fail(t,"Not enough startup memory to reuse a prepared movie frame.");goto done;}frame.bytes=e->video;
    if(!fh_install_artifact_read(t,e->at+HEADER,frame.data,e->video)||(unsigned)crc32(0,frame.data,e->video)!=e->vc||!video_valid(frame.data,e->video,scale,rgb8)){fail(t,"Reused movie frame failed validation.");goto done;}
   }else{
    result=fh_install_movie_decode(&image,video,original->video,decode_progress,&tracking);if(result!=1){if(!result)fail(t,image.error);goto done;}
    result=rgb8?fh_install_movie_rgb8_frame(&frame,&image,frame_progress,&tracking):fh_install_movie_frame(&frame,&image,scale,frame_progress,&tracking);if(result!=1){if(!result)fail(t,frame.error);goto done;}result=0;fh_install_movie_image_close(&image);
   }
   unsigned char h[HEADER]={0};memcpy(h,"FHMVFR01",8);little(h+8,i);little(h+12,frame.bytes);little(h+16,original->audio);little(h+20,(unsigned)crc32(0,frame.data,frame.bytes));little(h+24,(unsigned)crc32(0,audio,original->audio));little(h+28,original->video);little(h+32,crc);memcpy(h+36,sha,32);little(h+76,(unsigned)crc32(0,h,76));
   if(t->written>SPOOL_CAP||HEADER+frame.bytes+original->audio>SPOOL_CAP-t->written||!record(h,entries+i,i,t->written,&source)){fail(t,"Prepared movie exceeds bounded spool.");goto done;}
   if(!fh_install_artifact_write(t,h,HEADER)||!fh_install_artifact_write(t,frame.data,frame.bytes)||!fh_install_artifact_write(t,audio,original->audio))goto done;
   completed=i+1;
   /* Checkpoint at eight frames or the 1 MiB threshold. A normal
    * cancellation below commits every completed record, including when it
    * arrives during decode/resize/encode of the next frame. */
   if((completed==FH_INSTALL_MOVIE_FRAMES||completed-t->state.cursor[0]>=8||t->written-t->state.partial_bytes>=1024u*1024u)&&!fh_install_artifact_checkpoint(t,completed,SCAN_CURSOR))goto done;
   fh_install_movie_frame_close(&frame);
  }
  if(!report(&tracking,FH_INSTALL_MOVIE_PREP_WRITE,FH_INSTALL_MOVIE_FRAMES,FH_INSTALL_MOVIE_FRAMES)){result=-1;goto done;}
  if(!fh_install_artifact_finish_readback(t,spool_validate,&validation))goto done;
  metadata_ready=1;
 }
 {
  char path[FH_INSTALL_TX_PATH];unsigned count;
  /* Freshly finished spools already have the complete validated index in RAM.
   * A resumed completed spool reconstructs it; final movie validation remains. */
  if(!fh_install_artifact_path(t->root,&t->state,spool,path)||(input=fopen(path,"rb"))==NULL||(!metadata_ready&&(!read_spool(input,&validation,entries,&count,0)||count!=FH_INSTALL_MOVIE_FRAMES))){fail(t,"Cannot validate completed movie spool.");goto done;}
  unsigned char header[192];unsigned total;movie_header(header,&validation,&total);if(total>PACK_CAP){fail(t,"Prepared movie exceeds runtime size bound.");goto done;}
  if(!fh_install_artifact_begin(t,output))goto done;unsigned next=0;
  if(!t->written){if(!fh_install_artifact_write(t,header,192)||!fh_install_artifact_checkpoint(t,0,PACK_CURSOR))goto done;}
  else{
   next=t->state.cursor[0];unsigned expected=192;if(next>FH_INSTALL_MOVIE_FRAMES||t->state.cursor[1]!=PACK_CURSOR){fail(t,"Invalid movie assembly cursor.");goto done;}
   for(unsigned i=0;i<next;i++)expected+=frame_bytes(entries+i);unsigned char prior[192];
   if(t->written!=expected||!fh_install_artifact_read(t,0,prior,192)||memcmp(prior,header,192)){fail(t,"Movie assembly header or extent changed.");goto done;}
  }
  tracking.frame=next;if(!report(&tracking,FH_INSTALL_MOVIE_PREP_ASSEMBLE,next,FH_INSTALL_MOVIE_FRAMES)){result=-1;goto done;}
  for(unsigned i=next;i<FH_INSTALL_MOVIE_FRAMES;i++){
   Entry*e=entries+i;unsigned char h[16],zero[32]={0};big(h,frame_bytes(entries+(i+1)%FH_INSTALL_MOVIE_FRAMES));big(h+4,frame_bytes(entries+(i+FH_INSTALL_MOVIE_FRAMES-1)%FH_INSTALL_MOVIE_FRAMES));big(h+8,e->video);big(h+12,e->audio);
   unsigned padding=frame_bytes(e)-16-e->video-e->audio;
   if(!fh_install_artifact_write(t,h,16)||!copy(t,input,e->at+HEADER,e->video+e->audio)||!fh_install_artifact_write(t,zero,padding)||!fh_install_assembly_checkpoint(t,i+1,PACK_CURSOR,FH_INSTALL_MOVIE_FRAMES))goto done;
   tracking.frame=i+1;if(!report(&tracking,FH_INSTALL_MOVIE_PREP_ASSEMBLE,i+1,FH_INSTALL_MOVIE_FRAMES)){result=fh_install_artifact_checkpoint(t,i+1,PACK_CURSOR)?-1:0;goto done;}
  }
  result=fh_install_artifact_finish_readback(t,pack_validate,&validation);
 }
done:
 if(result==-1&&encoding&&t->output&&!t->failed&&t->state.partial==spool&&completed!=t->state.cursor[0]&&!fh_install_artifact_checkpoint(t,completed,SCAN_CURSOR))result=0;
 if(input&&fclose(input)&&result>0)result=fail(t,"Cannot close original movie spool.");free(entries);fh_install_movie_frame_close(&frame);fh_install_movie_image_close(&image);fh_install_movie_source_close(&source);if(result<=0)fh_install_transaction_close(t);return result;
}
int fh_install_movie_prepare(FhInstallTransaction*t,FhInstallDisc*d,unsigned spool,unsigned output,unsigned scale,FhInstallMovieProgress progress,void*context){return prepare(t,d,spool,output,scale,0,progress,context);}
int fh_install_movie_prepare_rgb8(FhInstallTransaction*t,FhInstallDisc*d,unsigned spool,unsigned output,FhInstallMovieProgress progress,void*context){return prepare(t,d,spool,output,2,1,progress,context);}
