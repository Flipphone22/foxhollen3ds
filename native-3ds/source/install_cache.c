/* Bounded native replacement for prepare_block_cache.py. */
#include "fh_install_cache.h"
#include "fh_block_cache.h"
#include <lz4.h>
#include <lz4hc.h>

enum {BLOCK=65536,MAX_SOURCE=64*1024*1024,SCRATCH_LIMIT=512*1024,VERIFY_BUDGET=256*1024};
static void put32(unsigned char*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static int fail(FhInstallTransaction*t,const char*s){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",s);return 0;}
typedef struct {uint32_t bytes,crc;} Expected;
static int validate(const char*path,void*context){
    Expected*e=context;FhBlockCache cache={0};
    int ok=fh_block_cache_open_window(&cache,path,VERIFY_BUDGET,e->bytes);
    if(ok)ok=fh_block_word(cache.storage+24)==e->crc;
    fh_block_cache_close(&cache);return ok;
}
int fh_install_cache(FhInstallTransaction*t,unsigned input,unsigned output,int level,FhInstallCacheProgress progress,void*context){
    if(!t||t->failed||t->output)return 0;
    if(input>=FH_INSTALL_ARTIFACTS||output>=FH_INSTALL_ARTIFACTS||input==output||
       !(t->state.complete&(1u<<input))||!(t->state.required&(1u<<output))||
       (level!=0&&(level<3||level>12)))return fail(t,"Invalid block-cache preparation recipe.");
    if(t->state.complete&(1u<<output))return 1;
    Expected expected={t->state.bytes[input],t->state.crc[input]};
    if(!expected.bytes||expected.bytes>MAX_SOURCE)return fail(t,"Block-cache source size outside supported bounds.");
    char filename[FH_INSTALL_TX_PATH];
    if(!fh_install_artifact_path(t->root,&t->state,input,filename))return fail(t,"Block-cache source path is too long.");
    FILE*source=fopen(filename,"rb");if(!source)return fail(t,"Cannot open prepared block-cache source.");
    unsigned blocks=(expected.bytes+BLOCK-1)/BLOCK,dir_bytes=blocks*16,base=32+dir_bytes;
    unsigned encoded_bytes=(unsigned)LZ4_compressBound(BLOCK);
    unsigned state_bytes=(unsigned)(level?LZ4_sizeofStateHC():LZ4_sizeofState());
    size_t total=BLOCK+encoded_bytes+state_bytes+base;
    void*state=NULL;unsigned char*raw=NULL,*encoded=NULL,*directory=NULL;int result=0;
    if(total>SCRATCH_LIMIT){fail(t,"LZ4 preparation exceeds startup scratch budget.");goto done;}
    if(!fh_install_artifact_begin(t,output))goto done;
    /* Only finish publishes a nonempty checkpoint in this stage. A resumed
     * nonempty artifact is complete and is validated again before retirement. */
    if(t->written){result=fh_install_artifact_finish(t,validate,&expected);goto done;}
    if(progress&&!progress(context,0,expected.bytes)){result=-1;goto done;}
    state=malloc(state_bytes);raw=malloc(BLOCK);encoded=malloc(encoded_bytes);directory=calloc(1,base);
    if(!state||!raw||!encoded||!directory){fail(t,"Not enough startup memory to prepare block cache.");goto done;}
    if(!fh_install_artifact_write(t,directory,base))goto done;
    uint32_t whole=0,at=0;
    for(unsigned i=0;i<blocks;i++){
        unsigned bytes=expected.bytes-at;if(bytes>BLOCK)bytes=BLOCK;
        if(fread(raw,1,bytes,source)!=bytes){fail(t,"Cannot read block-cache source.");goto done;}
        int n=level?LZ4_compress_HC_extStateHC(state,(const char*)raw,(char*)encoded,bytes,encoded_bytes,level):
                    LZ4_compress_fast_extState(state,(const char*)raw,(char*)encoded,bytes,encoded_bytes,1);
        if(n<=0){fail(t,"Native LZ4 compression failed.");goto done;}
        unsigned mode=(unsigned)n<bytes,size=mode?(unsigned)n:bytes;unsigned char*entry=directory+32+i*16;
        put32(entry,t->written);put32(entry+4,size);put32(entry+8,(uint32_t)crc32(0,raw,bytes));put32(entry+12,mode);
        if(!fh_install_artifact_write(t,mode?encoded:raw,size))goto done;
        whole=(uint32_t)crc32(whole,raw,bytes);at+=bytes;
        if(progress&&!progress(context,at,expected.bytes)){result=-1;goto done;}
    }
    if(fgetc(source)!=EOF||ferror(source)||whole!=expected.crc){fail(t,"Block-cache source changed or failed its checksum.");goto done;}
    memcpy(directory,"FHBCACH1",8);put32(directory+8,expected.bytes);put32(directory+12,blocks);
    put32(directory+16,t->written);put32(directory+20,(uint32_t)crc32(0,directory+32,dir_bytes));put32(directory+24,whole);
    if(!fh_install_artifact_patch(t,0,directory,base))goto done;
    /* Validation owns a separate bounded cache; do not stack both allocations. */
    free(state);state=NULL;free(raw);raw=NULL;free(encoded);encoded=NULL;free(directory);directory=NULL;
    result=fh_install_artifact_finish(t,validate,&expected);
done:
    free(state);free(raw);free(encoded);free(directory);
    if(fclose(source)&&result>0)result=fail(t,"Cannot close block-cache source.");
    if(result<=0)fh_install_transaction_close(t);
    return result;
}
