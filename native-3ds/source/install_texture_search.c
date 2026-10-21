#include "fh_install_texture_search.h"
#include "fh_install_texture_hints.h"
#include "fh_install_texture_hint_data.h"
#include "fh_install_texture_padded.h"
#include "fh_install_worker.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static FhInstallTextureHints table;
static unsigned char*storage;
void fh_install_texture_search_stop(void){
 fh_install_worker_join();memset(&table,0,sizeof(table));free(storage);storage=NULL;
}
int fh_install_texture_search_start(const unsigned char source_sha[32]){
 if(storage||table.data||!source_sha)return 0;
 if((size_t)(fh_install_hints_compressed_end-fh_install_hints_compressed)!=FH_INSTALL_HINT_COMPRESSED_BYTES)return 0;
 storage=malloc(FH_INSTALL_HINT_RAW_BYTES);if(!storage)return 0;
 z_stream z={0};z.next_in=(Bytef*)fh_install_hints_compressed;z.avail_in=FH_INSTALL_HINT_COMPRESSED_BYTES;
 z.next_out=storage;z.avail_out=FH_INSTALL_HINT_RAW_BYTES;
 int ok=inflateInit(&z)==Z_OK;
 if(ok){ok=inflate(&z,Z_FINISH)==Z_STREAM_END&&!z.avail_in&&!z.avail_out;inflateEnd(&z);}
 if(ok)ok=fh_install_texture_hints_open(&table,storage,FH_INSTALL_HINT_RAW_BYTES,fh_install_hint_blob_sha,source_sha,fh_install_hint_codec_contract);
 if(!ok)fh_install_texture_search_stop();
 return ok;
}
static unsigned extent(unsigned n){unsigned p=8;while(p<n)p*=2;return p;}
static int encode(const void*raw,size_t bytes,unsigned w,unsigned h,unsigned gw,unsigned gh,void*out,size_t cap,
 FhInstallTextureResult*result,FhInstallTextureProgress progress,void*context,unsigned padded){
 if(!result)return 0;
 FhInstallTextureHint hint={0};int found=fh_install_texture_hints_find(&table,raw,bytes,w,h,gw,gh,14,&hint);
 if(found<0){memset(result,0,sizeof(*result));return 0;}
 int ok;
 if(padded)ok=fh_install_texture_padded_cmpr_modes(raw,bytes,w,h,out,cap,result,progress,context,hint.modes,found?(hint.blocks+3)/4:0);
 else ok=fh_install_texture_cmpr_modes(raw,bytes,w,h,gw,gh,out,cap,result,progress,context,hint.modes,found?(hint.blocks+3)/4:0);
 /* Never publish an accelerated result without matching every output byte's
  * fingerprint. Cancellation retains the preceding durable checkpoint. */
 if(ok==1&&found&&!fh_install_texture_hint_verify(&hint,out,result->bytes,result->format)){memset(result,0,sizeof(*result));return 0;}
 return ok;
}
int fh_install_texture_search_cmpr(const void*raw,size_t bytes,unsigned w,unsigned h,unsigned gw,unsigned gh,
 void*out,size_t cap,FhInstallTextureResult*result,FhInstallTextureProgress progress,void*context){
 return encode(raw,bytes,w,h,gw,gh,out,cap,result,progress,context,0);
}
int fh_install_texture_search_padded(const void*raw,size_t bytes,unsigned w,unsigned h,
 void*out,size_t cap,FhInstallTextureResult*result,FhInstallTextureProgress progress,void*context){
 if(!w||!h||w>1024||h>1024){if(result)memset(result,0,sizeof(*result));return 0;}
 return encode(raw,bytes,w,h,extent(w),extent(h),out,cap,result,progress,context,1);
}
