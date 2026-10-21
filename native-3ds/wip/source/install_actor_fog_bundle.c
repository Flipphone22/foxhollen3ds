#include "fh_install_actor_fog_bundle.h"
#include "fh_install_actor_fog_material.h"
#include <string.h>
#include <zlib.h>
static void put(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
int fh_install_actor_fog_bundle_write(const FhInstallActorFogSink*s,const void*raw,unsigned bytes,unsigned id,
    unsigned bump,FhActorFogTextureWidth width,void*context){
    if(!s||!s->append||!s->header||id>>31)return 0;
    FhPicaActorFogMaterial materials[64];FhPicaActorFogVariants plan;unsigned count;
    if(!fh_install_actor_fog_prepare(&plan,materials,&count,raw,bytes,bump,width,context))return 0;
    unsigned char h[FH_ACTOR_FOG_BUNDLE_HEADER]={0};
    if(!s->append(s->context,h,sizeof(h)))return 0;
    unsigned cursor=sizeof(h);
    for(unsigned v=1;v<=plan.count;v++){
        FhInstallActorFogAsset p={0};
        int ok=fh_install_actor_fog_variant_asset(&p,raw,bytes,id,&plan,v,materials,count);
        if(ok)ok=p.metadata_bytes>=128&&p.metadata_bytes<=FH_ACTOR_FOG_META_CAP&&
            p.gpu_bytes&&p.gpu_bytes<=FH_ACTOR_FOG_GPU_CAP&&
            p.metadata_bytes+p.gpu_bytes<=FH_ACTOR_FOG_BUNDLE_LIMIT-cursor;
        if(ok){
            unsigned char*d=h+196+(v-1)*16;
            put(d,cursor);put(d+4,p.metadata_bytes);put(d+8,p.gpu_bytes);
            put(d+12,(unsigned)crc32(crc32(0,p.metadata,p.metadata_bytes),p.gpu,p.gpu_bytes));
            ok=s->append(s->context,p.metadata,p.metadata_bytes)&&s->append(s->context,p.gpu,p.gpu_bytes);
            cursor+=p.metadata_bytes+p.gpu_bytes;
        }
        fh_install_actor_fog_asset_close(&p);if(!ok)return 0;
    }
    memcpy(h,"FHFGB001",8);put(h+8,id);put(h+12,bytes);put(h+16,(unsigned)crc32(0,raw,bytes));
    put(h+20,bump);put(h+24,cursor);memcpy(h+32,&plan,sizeof(plan));put(h+28,(unsigned)crc32(0,h,sizeof(h)));
    return s->header(s->context,h,sizeof(h));
}
static int append(void*context,const void*data,unsigned bytes){
    FILE*f=context;return fwrite(data,1,bytes,f)==bytes&&!ferror(f);
}
static int header(void*context,const void*data,unsigned bytes){
    FILE*f=context;return !ferror(f)&&!fseek(f,0,SEEK_SET)&&append(f,data,bytes)&&!fflush(f);
}
int fh_install_actor_fog_bundle(FILE*f,const void*raw,unsigned bytes,unsigned id,
    unsigned bump,FhActorFogTextureWidth width,void*context){
    if(!f||fseek(f,0,SEEK_END)||ftell(f)!=0)return 0;
    const FhInstallActorFogSink sink={append,header,f};
    return fh_install_actor_fog_bundle_write(&sink,raw,bytes,id,bump,width,context);
}
