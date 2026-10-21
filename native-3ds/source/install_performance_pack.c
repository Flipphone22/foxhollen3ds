#include "fh_install_performance_pack.h"
#include "fh_install_assembly.h"
#include "fh_install_scene.h"
#include <mbedtls/sha256.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
enum{HEADER=96,LIMIT=4096,STAGE=16384,SPOOL_LIMIT=160*1024*1024,
     PATCH_LIMIT=1024*1024,SCENE_LIMIT=2*1024*1024+256*1024,CURSOR=0x52535031};
typedef struct{unsigned at,id,source,original,target,patch,patch_crc,scene,scene_crc,before,after,changed,aliases;unsigned char sha[32];} Entry;
static unsigned le(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static unsigned be(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void little(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static void big(unsigned char*p,unsigned n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static int fail(FhInstallTransaction*t,const char*why){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",why);return 0;}
static int decode(const unsigned char*h,Entry*e,unsigned at){
 if(memcmp(h,"FHRDSPL1",8)||le(h+88)||le(h+92)!=(unsigned)crc32(0,h,92))return 0;
 *e=(Entry){0};e->at=at;e->id=le(h+8);e->source=le(h+12);e->original=le(h+16);e->target=le(h+20);
 e->patch=le(h+24);e->patch_crc=le(h+28);e->scene=le(h+32);e->scene_crc=le(h+36);memcpy(e->sha,h+40,32);
 e->before=le(h+72);e->after=le(h+76);e->changed=le(h+80);e->aliases=le(h+84);
 return e->source>=(e->id>>31?164u:252u)&&e->source<=4*1024*1024&&e->before>=e->after&&e->changed<=255&&e->aliases<=255&&
  ((!e->patch&&!e->patch_crc&&!e->changed&&e->original==e->target&&e->before==e->after)||(e->patch>=24&&e->patch<=PATCH_LIMIT&&e->changed&&e->before>e->after))&&
  ((!e->scene&&!e->scene_crc&&!e->patch)||(e->scene>=128&&e->scene<=SCENE_LIMIT));
}
static int patch(FILE*f,const Entry*e){
 if(!e->patch)return 1;unsigned char*p=malloc(e->patch);if(!p)return 0;
 int ok=fread(p,1,e->patch,f)==e->patch&&(unsigned)crc32(0,p,e->patch)==e->patch_crc&&
  !memcmp(p,"FHMLD01",8)&&be(p+8)==e->id&&be(p+12)==e->source&&be(p+16)==e->original&&be(p+20)==e->changed;
 unsigned at=24,previous=0;
 for(unsigned i=0;ok&&i<e->changed;i++){
  if(at>e->patch||8>e->patch-at){ok=0;break;}unsigned list=be(p+at),n=be(p+at+4);at+=8;
  if(list>=255||(i&&list<=previous)||!n||n>65535||(n&31)||n>e->patch-at){ok=0;break;}previous=list;at+=n;
 }
 ok=ok&&at==e->patch;free(p);return ok;
}
static int scene(FILE*f,const Entry*e){
 if(!e->scene)return 1;void*allocation=malloc(e->scene+15);if(!allocation)return 0;
 unsigned char*p=(void*)(((uintptr_t)allocation+15)&~(uintptr_t)15);FhSceneAsset asset;
 int ok=fread(p,1,e->scene,f)==e->scene&&(unsigned)crc32(0,p,e->scene)==e->scene_crc;unsigned meta=ok?le(p+68):0;
 ok=ok&&meta>=128&&meta<=e->scene&&fh_native_scene_open(&asset,p,meta,p+meta,e->scene-meta,e->id,e->source,e->target);free(allocation);return ok;
}
static int read_spool(FILE*f,Entry*entries,unsigned*count,unsigned verify){
 if(fseek(f,0,SEEK_END))return 0;long length=ftell(f);if(length<0||length>SPOOL_LIMIT||fseek(f,0,SEEK_SET))return 0;
 unsigned at=0,n=0;unsigned char h[HEADER];
 while(at<(unsigned)length){
  Entry e;if(n==LIMIT||HEADER>(unsigned)length-at||fread(h,1,HEADER,f)!=HEADER||!decode(h,&e,at)||e.patch+e.scene>(unsigned)length-at-HEADER)return 0;
  if(verify){if(!patch(f,&e)||!scene(f,&e))return 0;}else if(fseek(f,e.patch+e.scene,SEEK_CUR))return 0;
  if(entries){for(unsigned i=0;i<n;i++)if(entries[i].id==e.id&&entries[i].source==e.source&&(entries[i].original==e.original||entries[i].target==e.target))return 0;entries[n]=e;}
  n++;at+=HEADER+e.patch+e.scene;
 }*count=n;return !ferror(f);
}
static int spool_validate(const char*path,void*context){(void)context;FILE*f=fopen(path,"rb");if(!f)return 0;unsigned n;int ok=read_spool(f,NULL,&n,1);if(fclose(f))ok=0;return ok;}
void fh_install_performance_spool_close(FhInstallPerformanceSpool*s){if(s){free(s->entries);memset(s,0,sizeof(*s));}}
int fh_install_performance_spool_begin(FhInstallPerformanceSpool*s,FhInstallTransaction*t,unsigned artifact){
 if(!s||s->entries||!t||t->failed||!fh_install_artifact_begin(t,artifact))return 0;
 Entry*entries=calloc(LIMIT,sizeof(*entries));if(!entries)return fail(t,"Not enough memory for reduction directory.");unsigned n;
 if(!read_spool(t->output,entries,&n,0)||fseek(t->output,t->written,SEEK_SET)){free(entries);return fail(t,"Invalid reduction checkpoint.");}
 s->entries=entries;s->transaction=t;s->count=n;
 for(unsigned i=0;i<n;i++){s->patches+=entries[i].patch!=0;s->packets+=entries[i].scene!=0;s->nonrendering+=entries[i].scene==0;s->before+=entries[i].before;s->after+=entries[i].after;}return 1;
}
int fh_install_performance_spool_add(FhInstallPerformanceSpool*s,const void*raw,unsigned bytes,unsigned id,unsigned next_file,unsigned next_index,float ratio,float error,FhInstallModelReduceProgress progress,void*context){
 if(!s||!s->entries||!s->transaction)return 0;FhInstallTransaction*t=s->transaction;if(t->failed||!t->output)return 0;
 if(!raw||bytes<(id>>31?164u:252u)||bytes>4*1024*1024)return fail(t,"Invalid reduction source size.");
 unsigned crc=(unsigned)crc32(0,raw,bytes);unsigned char sha[32];if(mbedtls_sha256_ret(raw,bytes,sha,0))return fail(t,"Cannot fingerprint reduction source.");Entry*entries=s->entries;
 if(!fh_install_cursor_boundary(t,&s->cursor,next_file))return 0;
 for(unsigned i=0;i<s->count;i++)if(entries[i].id==id&&entries[i].source==bytes&&entries[i].original==crc){
  if(memcmp(sha,entries[i].sha,32))return fail(t,"Ambiguous original reduction identity (CRC collision).");
  if(!fh_install_cursor_duplicate(t,&s->cursor,next_file,next_index))return 0;s->duplicates++;return 1;
 }
 if(s->count==LIMIT)return fail(t,"Too many source reduction records.");
 FhInstallModelReduction reduction={0};FhInstallScene compiled={0};int result=fh_install_model_reduce(&reduction,raw,bytes,id,ratio,error,progress,context);
 if(result!=1){if(!result)fail(t,reduction.error);goto done;}
 const void*prepared=reduction.model?reduction.model:raw;unsigned target=(unsigned)crc32(0,prepared,bytes);result=0;
 for(unsigned i=0;i<s->count;i++)if(entries[i].id==id&&entries[i].source==bytes&&entries[i].target==target){fail(t,"Ambiguous reduced geometry identity.");goto done;}
 if(!fh_install_scene(&compiled,prepared,bytes,id)){fail(t,compiled.error);goto done;}
 unsigned char h[HEADER]={0};memcpy(h,"FHRDSPL1",8);little(h+8,id);little(h+12,bytes);little(h+16,crc);little(h+20,target);
 little(h+24,reduction.patch_bytes);little(h+28,(unsigned)crc32(0,reduction.patch,reduction.patch_bytes));little(h+32,compiled.bytes);little(h+36,(unsigned)crc32(0,compiled.data,compiled.bytes));
 memcpy(h+40,sha,32);little(h+72,reduction.before);little(h+76,reduction.after);little(h+80,reduction.changed);little(h+84,reduction.alias_kept);little(h+92,(unsigned)crc32(0,h,92));Entry e;
 if(!decode(h,&e,t->written)||t->written>SPOOL_LIMIT||HEADER+e.patch+e.scene>SPOOL_LIMIT-t->written){fail(t,"Reduced geometry spool exceeds bounded storage.");goto done;}
 result=fh_install_artifact_write(t,h,HEADER)&&fh_install_artifact_write(t,reduction.patch,reduction.patch_bytes)&&fh_install_artifact_write(t,compiled.data,compiled.bytes)&&fh_install_cursor_record(t,&s->cursor,next_file,next_index);
 if(result){entries[s->count++]=e;s->patches+=e.patch!=0;s->packets+=e.scene!=0;s->nonrendering+=e.scene==0;s->before+=e.before;s->after+=e.after;}
done:fh_install_scene_close(&compiled);fh_install_model_reduce_close(&reduction);return result;
}
int fh_install_performance_spool_finish(FhInstallPerformanceSpool*s){if(!s||!s->entries||!s->transaction)return 0;int ok=fh_install_cursor_flush(s->transaction,&s->cursor)&&fh_install_artifact_finish(s->transaction,spool_validate,NULL);fh_install_performance_spool_close(s);return ok;}
static int order(const Entry*x,const Entry*y,unsigned topology){
 if(x->id!=y->id)return x->id<y->id?-1:1;if(x->source!=y->source)return x->source<y->source?-1:1;
 unsigned a=topology?x->original:x->target,b=topology?y->original:y->target;return a==b?0:a<b?-1:1;
}
static int order_topology(const void*x,const void*y){return order(x,y,1);}
static int order_geometry(const void*x,const void*y){return order(x,y,0);}
static int validate_pack(const char*path,void*context){
 unsigned topology=*(unsigned*)context,stride=topology?24:32;unsigned(*word)(const unsigned char*)=topology?be:le;
 FILE*f=fopen(path,"rb");if(!f)return 0;int ok=0;unsigned char h[16],*directory=NULL;Entry previous={0};
 if(fseek(f,0,SEEK_END))goto done;long length=ftell(f);
 if(length<16||length>(topology?32:64)*1024*1024||fseek(f,0,SEEK_SET)||fread(h,1,16,f)!=16||memcmp(h,topology?"FHMLPK1":"FHNSSET1",8))goto done;
 unsigned n=word(h+8);if(n>LIMIT||(!n&&!topology)||n*stride>(unsigned)length-16)goto done;
 directory=malloc(n*stride+1);if(!directory||fread(directory,stride,n,f)!=n||(unsigned)crc32(0,directory,n*stride)!=word(h+12))goto done;
 unsigned at=16+n*stride;
 for(unsigned i=0;i<n;i++){
  const unsigned char*d=directory+i*stride;Entry e={0};e.id=word(d);e.source=word(d+4);e.original=e.target=word(d+8);unsigned bytes=word(d+16),crc=word(d+20);
  if(word(d+12)!=at||at>(unsigned)length||bytes>(unsigned)length-at||(i&&order(&previous,&e,topology)>=0))goto done;
  if(topology){
   if(bytes<24||bytes>PATCH_LIMIT)goto done;unsigned char prefix[24];if(fread(prefix,1,24,f)!=24||fseek(f,-24,SEEK_CUR))goto done;
   e.patch=bytes;e.patch_crc=crc;e.changed=be(prefix+20);if(!e.changed||e.changed>255||!patch(f,&e))goto done;
  }else{if(word(d+24)||word(d+28)||bytes<128||bytes>SCENE_LIMIT)goto done;e.scene=bytes;e.scene_crc=crc;if(!scene(f,&e))goto done;}
  unsigned pad=topology?0:(0u-bytes)&3u;for(unsigned j=0;j<pad;j++)if(fgetc(f)!=0)goto done;at+=bytes+pad;previous=e;
 }ok=at==(unsigned)length&&!ferror(f);
done:free(directory);if(fclose(f))ok=0;return ok;
}
static int copy(FhInstallTransaction*t,FILE*f,unsigned at,unsigned bytes){
 unsigned char b[STAGE];if(fseek(f,at,SEEK_SET))return fail(t,"Cannot seek reduced geometry spool.");
 while(bytes){unsigned n=bytes>sizeof(b)?sizeof(b):bytes;if(fread(b,1,n,f)!=n)return fail(t,"Cannot read reduced geometry spool.");if(!fh_install_artifact_write(t,b,n))return 0;bytes-=n;}return 1;
}
int fh_install_performance_pack(FhInstallTransaction*t,unsigned spool,unsigned output,unsigned kind,FhInstallPerformanceProgress progress,void*context){
 if(!t||t->failed||t->output)return 0;
 if(kind>FH_INSTALL_PERFORMANCE_GEOMETRY||spool>=FH_INSTALL_ARTIFACTS||output>=FH_INSTALL_ARTIFACTS||spool==output||!(t->state.complete&(1u<<spool)))return fail(t,"Invalid performance pack recipe.");
 if(t->state.complete&(1u<<output))return 1;unsigned topology=kind==FH_INSTALL_PERFORMANCE_TOPOLOGY,stride=topology?24:32,limit=(topology?32:64)*1024*1024;
 unsigned(*word)(const unsigned char*)=topology?be:le;void(*put)(unsigned char*,unsigned)=topology?big:little;
 char path[FH_INSTALL_TX_PATH];if(!fh_install_artifact_path(t->root,&t->state,spool,path))return 0;FILE*f=fopen(path,"rb");if(!f)return fail(t,"Cannot open reduction spool.");
 Entry*e=malloc(LIMIT*sizeof(*e));unsigned char*directory=NULL;unsigned count=0;int result=0;
 if(!e){fail(t,"Not enough memory for performance assembly.");goto done;}
 if(!read_spool(f,e,&count,1)){fail(t,"Reduced spool failed validation.");goto done;}
 unsigned n=0;for(unsigned i=0;i<count;i++)if(topology?e[i].patch:e[i].scene)e[n++]=e[i];
 if(!n&&!topology){fail(t,"No native geometry in performance collection.");goto done;}
 qsort(e,n,sizeof(*e),topology?order_topology:order_geometry);unsigned directory_bytes=16+n*stride,end=directory_bytes;
 directory=calloc(1,directory_bytes);if(!directory){fail(t,"Not enough memory for performance pack directory.");goto done;}
 memcpy(directory,topology?"FHMLPK1":"FHNSSET1",8);put(directory+8,n);
 for(unsigned i=0;i<n;i++){
  unsigned bytes=topology?e[i].patch:e[i].scene,padding=topology?0:(0u-bytes)&3u;unsigned char*d=directory+16+i*stride;
  if(bytes+padding>limit-end){fail(t,"Performance pack exceeds runtime storage bound.");goto done;}
  put(d,e[i].id);put(d+4,e[i].source);put(d+8,topology?e[i].original:e[i].target);put(d+12,end);put(d+16,bytes);put(d+20,topology?e[i].patch_crc:e[i].scene_crc);end+=bytes+padding;
 }
 put(directory+12,(unsigned)crc32(0,directory+16,n*stride));if(!fh_install_artifact_begin(t,output))goto done;unsigned next=0;
 if(!t->written){if(!fh_install_artifact_write(t,directory,directory_bytes)||!fh_install_artifact_checkpoint(t,0,CURSOR+kind))goto done;}
 else{
  next=t->state.cursor[0];unsigned expected=next<n?word(directory+16+next*stride+12):end;
  if(next>n||t->state.cursor[1]!=CURSOR+kind||t->written!=expected){fail(t,"Invalid performance assembly cursor.");goto done;}
  unsigned char b[STAGE];for(unsigned at=0;at<directory_bytes;){unsigned bytes=directory_bytes-at;if(bytes>sizeof(b))bytes=sizeof(b);
   if(!fh_install_artifact_read(t,at,b,bytes)||memcmp(b,directory+at,bytes)){fail(t,"Performance directory changed during resume.");goto done;}at+=bytes;}
 }
 if(progress&&!progress(context,next,n)){result=-1;goto done;}
 for(unsigned i=next;i<n;i++){
  unsigned bytes=topology?e[i].patch:e[i].scene,padding=topology?0:(0u-bytes)&3u;const unsigned char zero[4]={0};
  if(!copy(t,f,e[i].at+HEADER+(topology?0:e[i].patch),bytes)||!fh_install_artifact_write(t,zero,padding)||!fh_install_assembly_checkpoint(t,i+1,CURSOR+kind,n))goto done;
  if(progress&&!progress(context,i+1,n)){result=fh_install_artifact_checkpoint(t,i+1,CURSOR+kind)?-1:0;goto done;}
 }
 result=fh_install_artifact_finish(t,validate_pack,&topology);
done:free(e);free(directory);if(fclose(f)&&result>0)result=fail(t,"Cannot close performance spool.");if(result<=0)fh_install_transaction_close(t);return result;
}
