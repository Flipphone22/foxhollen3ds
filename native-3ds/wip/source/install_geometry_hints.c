#include "fh_install_geometry_hints.h"
#include "fh_install_triangles.h"
#include <mbedtls/sha256.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

enum {HEADER=96,ENTRY=40,MAX_RECORDS=4096};
static unsigned word(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static unsigned big(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static unsigned bits(float f){unsigned n;memcpy(&n,&f,4);return n;}
static int order(const unsigned char*a,const unsigned char*b){
    int n=memcmp(a,b,32);if(n)return n;
    for(unsigned at=32;at<40;at+=4){unsigned x=word(a+at),y=word(b+at);if(x!=y)return x<y?-1:1;}
    return 0;
}
int fh_install_geometry_hints_open(FhInstallGeometryHints*out,const void*data,unsigned bytes,
    const unsigned char blob_sha[32],const unsigned char disc_sha[32],const unsigned char contract_sha[32]){
    if(out)memset(out,0,sizeof(*out));
    if(!out||!data||!blob_sha||!disc_sha||!contract_sha||bytes<HEADER||bytes>HEADER+MAX_RECORDS*ENTRY)return 0;
    const unsigned char*p=data;unsigned count=word(p+12);unsigned char sha[32];float ratio,error;
    unsigned rb=word(p+20),eb=word(p+24);memcpy(&ratio,&rb,4);memcpy(&error,&eb,4);
    if(memcmp(p,"FHKEEP01",8)||word(p+8)!=1||!count||count>MAX_RECORDS||bytes!=HEADER+count*ENTRY||word(p+16)!=bytes||word(p+92)||
       !isfinite(ratio)||ratio<=0||ratio>=1||!isfinite(error)||error<=0||error>.1f||memcmp(p+28,disc_sha,32)||memcmp(p+60,contract_sha,32)||
       mbedtls_sha256_ret(p,bytes,sha,0)||memcmp(sha,blob_sha,32))return 0;
    for(unsigned i=0;i<count;i++){
        const unsigned char*e=p+HEADER+i*ENTRY;unsigned id=word(e+32),n=word(e+36);
        if(n<(id>>31?164u:252u)||n>4*1024*1024||(i&&order(e-ENTRY,e)>=0))return 0;
    }
    out->data=p;out->bytes=bytes;out->count=count;out->ratio_bits=rb;out->error_bits=eb;return 1;
}
int fh_install_geometry_hints_find(const FhInstallGeometryHints*t,unsigned id,unsigned bytes,
    const unsigned char source_sha[32],float ratio,float error){
    if(!source_sha||!isfinite(ratio)||!isfinite(error))return 0;
    if(!t||!t->data||!t->count||bits(ratio)!=t->ratio_bits||bits(error)!=t->error_bits)return 0;
    unsigned char key[ENTRY];memcpy(key,source_sha,32);
    for(unsigned i=0;i<4;i++){key[32+i]=id>>(i*8);key[36+i]=bytes>>(i*8);}
    unsigned lo=0,hi=t->count;
    while(lo<hi){unsigned mid=lo+(hi-lo)/2;const unsigned char*e=t->data+HEADER+mid*ENTRY;if(order(e,key)<0)lo=mid+1;else hi=mid;}
    return lo<t->count&&!order(t->data+HEADER+lo*ENTRY,key);
}
typedef struct {unsigned char used[256],stride[256];unsigned count;} Lists;
static int collect(void*context,const FhInstallDrawSource*d){
    Lists*l=context;if(d->list>=256)return 0;
    if(!l->used[d->list]){l->used[d->list]=1;l->stride[d->list]=d->layout[0]+d->layout[1]+d->layout[2]+d->layout[3]+d->layout[4]*d->layout[5];l->count++;}
    return 1;
}
static int fail(FhInstallModelReduction*out,const char*text){snprintf(out->error,sizeof(out->error),"%s",text);return 0;}
int fh_install_model_reduce_hinted(const FhInstallGeometryHints*t,FhInstallModelReduction*out,
    const void*source,unsigned bytes,unsigned id,const unsigned char source_sha[32],float ratio,float error,
    FhInstallModelReduceProgress progress,void*context){
    if(!fh_install_geometry_hints_find(t,id,bytes,source_sha,ratio,error))return fh_install_model_reduce(out,source,bytes,id,ratio,error,progress,context);
    if(!out)return 0;fh_install_model_reduce_close(out);FhInstallVertexSource s;Lists lists={0};unsigned draws;
    if(!fh_install_vertex_source(&s,source,bytes,id>>31)||fh_install_descriptors(source,bytes,id>>31,collect,&lists,&draws)!=1)return fail(out,"Invalid hinted original geometry.");
    if(!lists.count)return 1;
    const unsigned char*raw=source;unsigned header=s.world?164:252,table=big(raw+(s.world?0x68:0xd0));
    unsigned visible=raw[s.world?0xa1:0xf5],all=visible+(s.world?0:raw[0xf6]);
    if(table<header||table>bytes||all>(bytes-table)/28)return fail(out,"Invalid hinted visible/shadow list table.");
    /* Recompute these fields from the ISO. The hint contains no triangle
     * counts or aliases and does not produce geometry or a topology patch. */
    for(unsigned i=0;i<visible;i++)if(lists.used[i]){
        unsigned entry=table+i*28,at=big(raw+entry),n=half(raw+entry+4);
        if(at<header||at>bytes||n>bytes-at||(at<table+all*28&&table<at+n))return fail(out,"Hinted source list overlaps metadata.");
    }
    if(progress&&!progress(context,0,lists.count))return -1;
    for(unsigned i=0;i<visible;i++)if(lists.used[i]){
        unsigned entry=table+i*28,offset=big(raw+entry),n=half(raw+entry+4);FhInstallTriangles triangles;
        if(fh_install_triangles(raw+offset,n,lists.stride[i],NULL,NULL,&triangles)!=1)return fail(out,"Cannot count hinted original topology.");
        out->before+=triangles.triangles;out->after+=triangles.triangles;
        for(unsigned j=0;j<all;j++)if(j!=i){unsigned other=big(raw+table+j*28),length=half(raw+table+j*28+4);if(length&&(uint64_t)offset<(uint64_t)other+length&&(uint64_t)other<(uint64_t)offset+n){out->alias_kept++;break;}}
        out->lists++;if(progress&&!progress(context,out->lists,lists.count))return -1;
    }
    if(out->lists!=lists.count)return fail(out,"Hinted original list coverage changed.");
    /* No search occurred: keep-reason, scratch and error metrics remain zero. */
    return 1;
}
