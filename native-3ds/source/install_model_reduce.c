#include "fh_install_model_reduce.h"
#include "fh_model_lod.h"
#include "fh_install_triangles.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
enum {PATCH_CAP=1024*1024};
typedef struct{unsigned char used[256],stride[256];unsigned count;} Lists;
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static void put(unsigned char*p,unsigned n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static int collect(void*context,const FhInstallDrawSource*d){Lists*l=context;if(d->list>=256)return 0;if(!l->used[d->list]){l->used[d->list]=1;l->stride[d->list]=d->layout[0]+d->layout[1]+d->layout[2]+d->layout[3]+d->layout[4]*d->layout[5];l->count++;}return 1;}
void fh_install_model_reduce_close(FhInstallModelReduction*out){if(out){free(out->patch);free(out->model);memset(out,0,sizeof(*out));}}
static int fail(FhInstallModelReduction*out,const char*why){snprintf(out->error,sizeof(out->error),"%s",why);return 0;}
int fh_install_model_reduce(FhInstallModelReduction*out,const void*source,unsigned bytes,unsigned id,float ratio,float error,FhInstallModelReduceProgress progress,void*context){
    if(!out)return 0;fh_install_model_reduce_close(out);FhInstallVertexSource s;FhInstallLodGuards guards={0};Lists lists={0};unsigned draws;
    if(!isfinite(ratio)||ratio<=0||ratio>=1||!isfinite(error)||error<=0||error>.1f||!fh_install_vertex_source(&s,source,bytes,id>>31)||
       fh_install_descriptors(source,bytes,id>>31,collect,&lists,&draws)!=1)return fail(out,"Invalid model reduction source or recipe.");
    if(!lists.count)return 1;
    const unsigned char*raw=source;unsigned header=s.world?164:252,table=word(raw+(s.world?0x68:0xd0));
    unsigned visible=raw[s.world?0xa1:0xf5],all=visible+(s.world?0:raw[0xf6]);
    if(table<header||table>bytes||all>(bytes-table)/28)return fail(out,"Invalid visible/shadow list table.");
    unsigned capacity=24;
    for(unsigned i=0;i<visible;i++)if(lists.used[i]){
        unsigned entry=table+i*28,at=word(raw+entry),size=half(raw+entry+4);
        if(at<header||at>bytes||size>bytes-at||(at<table+all*28&&table<at+size))return fail(out,"Source display list overlaps model metadata.");
        capacity+=8+size;
    }
    if(capacity>PATCH_CAP)capacity=PATCH_CAP;
    out->patch=malloc(capacity);if(!out->patch)return fail(out,"Not enough startup memory for model topology patch.");
    memcpy(out->patch,"FHMLD01",8);put(out->patch+8,id);put(out->patch+12,bytes);put(out->patch+16,(unsigned)crc32(0,raw,bytes));put(out->patch+20,0);
    unsigned at=24;int result=0;
    if(!fh_install_lod_guards_begin(&guards,&s)){fail(out,guards.error);goto done;}
    if(progress&&!progress(context,0,lists.count)){result=-1;goto done;}
    for(unsigned i=0;i<visible;i++)if(lists.used[i]){
        unsigned entry=table+i*28,offset=word(raw+entry),length=half(raw+entry+4),alias=0;
        FhInstallTriangles original;
        if(fh_install_triangles(raw+offset,length,lists.stride[i],NULL,NULL,&original)!=1){fail(out,"Cannot count original model topology.");goto done;}
        out->before+=original.triangles;out->after+=original.triangles;
        for(unsigned j=0;j<all;j++)if(j!=i){unsigned other=word(raw+table+j*28),n=half(raw+table+j*28+4);if(n&&(uint64_t)offset<(uint64_t)other+n&&(uint64_t)other<(uint64_t)offset+length){alias=1;break;}}
        if(alias)out->alias_kept++;
        else{
            FhInstallReduction candidate={0};int status=fh_install_reduce_list(&candidate,&guards,i,ratio,error);
            if(status==FH_REDUCE_ERROR){fail(out,candidate.message);fh_install_reduce_close(&candidate);goto done;}
            if(candidate.simplify_scratch>out->simplify_scratch)out->simplify_scratch=candidate.simplify_scratch;
            if(status==FH_REDUCE_CHANGED){
                if(candidate.before!=original.triangles){fail(out,"Reduction source triangle count changed.");fh_install_reduce_close(&candidate);goto done;}
                out->after-=candidate.before-candidate.after;
                if(candidate.bytes>capacity-at||8>capacity-at-candidate.bytes){fail(out,"Reduced model topology exceeds its bounded patch.");fh_install_reduce_close(&candidate);goto done;}
                put(out->patch+at,i);put(out->patch+at+4,candidate.bytes);memcpy(out->patch+at+8,candidate.data,candidate.bytes);at+=8+candidate.bytes;out->changed++;
                out->position_error=fmax(out->position_error,candidate.position_error);out->uv_error=fmax(out->uv_error,candidate.uv_error);out->color_error=fmax(out->color_error,candidate.color_error);
            }else if(candidate.reason<6)out->kept[candidate.reason]++;
            fh_install_reduce_close(&candidate);
        }
        out->lists++;if(progress&&!progress(context,out->lists,lists.count)){result=-1;goto done;}
    }
    if(out->lists!=lists.count){fail(out,"Reduction list coverage changed.");goto done;}
    if(out->changed){
        put(out->patch+20,out->changed);out->model=malloc(bytes);
        if(!out->model){fail(out,"Not enough memory to validate model topology publication.");goto done;}
        memcpy(out->model,raw,bytes);
        if(!fh_model_lod_patch(out->model,bytes,id,out->patch,at)){fail(out,"Native topology patch rejected by runtime validation.");goto done;}
        out->patch_bytes=at;out->model_bytes=bytes;
    }else{free(out->patch);out->patch=NULL;}
    result=1;
done:
    fh_install_lod_guards_close(&guards);
    if(result!=1){free(out->patch);free(out->model);out->patch=out->model=NULL;out->patch_bytes=out->model_bytes=0;}return result;
}
