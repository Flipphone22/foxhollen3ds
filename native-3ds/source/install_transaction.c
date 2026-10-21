/* Startup-only transaction owner. No SD work is added to gameplay. */
#include "fh_install_transaction.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
enum {RECORD_BYTES=192,CHUNK=16384};
static uint32_t word(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(unsigned char*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static int error(FhInstallTransaction*t,const char*s){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",s);return 0;}
static int path(char*out,const char*root,unsigned gen,const char*name){
    int n=gen?snprintf(out,FH_INSTALL_TX_PATH,"%s/g%08x/%s",root,gen,name):snprintf(out,FH_INSTALL_TX_PATH,"%s/%s",root,name);
    return n>0&&n<FH_INSTALL_TX_PATH;
}
int fh_install_artifact_path(const char*root,const FhInstallGeneration*g,unsigned id,char out[FH_INSTALL_TX_PATH]){
    char name[24];if(!root||!g||!g->generation||id>=FH_INSTALL_ARTIFACTS)return 0;
    snprintf(name,sizeof(name),"asset-%u.bin",id);return path(out,root,g->generation,name);
}
static int part_path(FhInstallTransaction*t,unsigned id,char*out){char name[24];snprintf(name,sizeof(name),"asset-%u.part",id);return path(out,t->root,t->state.generation,name);}
static void encode(unsigned char*b,const FhInstallGeneration*g,unsigned committed){
    memset(b,0,RECORD_BYTES);memcpy(b,"FHIPREP1",8);put(b+8,committed);
    const uint32_t v[]={g->generation,g->serial,g->required,g->complete,g->partial,g->partial_bytes,g->partial_crc,g->cursor[0],g->cursor[1]};
    for(unsigned i=0;i<9;i++)put(b+12+4*i,v[i]);
    memcpy(b+48,g->recipe,32);memcpy(b+80,g->source,32);
    for(unsigned i=0;i<FH_INSTALL_ARTIFACTS;i++){put(b+112+8*i,g->bytes[i]);put(b+116+8*i,g->crc[i]);}
    put(b+188,(uint32_t)crc32(0,b,188));
}
static int decode(FhInstallGeneration*g,const unsigned char*b,unsigned committed){
    if(memcmp(b,"FHIPREP1",8)||word(b+8)!=committed||word(b+188)!=(uint32_t)crc32(0,b,188))return 0;
    memset(g,0,sizeof(*g));g->generation=word(b+12);g->serial=word(b+16);g->required=word(b+20);g->complete=word(b+24);
    g->partial=word(b+28);g->partial_bytes=word(b+32);g->partial_crc=word(b+36);g->cursor[0]=word(b+40);g->cursor[1]=word(b+44);
    memcpy(g->recipe,b+48,32);memcpy(g->source,b+80,32);
    for(unsigned i=0;i<FH_INSTALL_ARTIFACTS;i++){g->bytes[i]=word(b+112+8*i);g->crc[i]=word(b+116+8*i);if(g->bytes[i]>INT32_MAX)return 0;}
    if(!g->generation||!g->serial||!g->required||(g->required&~255u)||(g->complete&~g->required)||g->partial>FH_INSTALL_NO_ARTIFACT||g->partial_bytes>INT32_MAX)return 0;
    if(g->partial<FH_INSTALL_ARTIFACTS && (!(g->required&(1u<<g->partial))||(g->complete&(1u<<g->partial))))return 0;
    if(g->partial==FH_INSTALL_NO_ARTIFACT && (g->partial_bytes||g->partial_crc))return 0;
    if(committed && (g->complete!=g->required||g->partial!=FH_INSTALL_NO_ARTIFACT))return 0;
    return 1;
}
static int read_record(const char*filename,FhInstallGeneration*g,unsigned committed){
    unsigned char b[RECORD_BYTES+1];FILE*f=fopen(filename,"rb");if(!f)return 0;
    size_t n=fread(b,1,sizeof(b),f);int ok=!ferror(f);if(fclose(f))ok=0;
    return ok&&n==RECORD_BYTES&&decode(g,b,committed);
}
static int sync_file(FILE*f){return !fflush(f)&&!fsync(fileno(f));}
static int write_record(const char*filename,const FhInstallGeneration*g,unsigned committed){
    unsigned char b[RECORD_BYTES],readback[RECORD_BYTES+1];encode(b,g,committed);
    FILE*f=fopen(filename,"wb");if(!f)return 0;
    int ok=fwrite(b,1,sizeof(b),f)==sizeof(b)&&sync_file(f);if(fclose(f))ok=0;
    if(!ok)return 0;f=fopen(filename,"rb");if(!f)return 0;
    size_t n=fread(readback,1,sizeof(readback),f);ok=!ferror(f);if(fclose(f))ok=0;
    return ok&&n==sizeof(b)&&!memcmp(b,readback,sizeof(b));
}
static int scan_file(const char*filename,uint32_t bytes,uint32_t expected,unsigned exact){
    FILE*f=fopen(filename,"rb");if(!f)return 0;
    unsigned char b[CHUNK];uint32_t left=bytes,crc=0;int ok=1;
    while(left){unsigned n=left>sizeof(b)?sizeof(b):left;if(fread(b,1,n,f)!=n){ok=0;break;}crc=(uint32_t)crc32(crc,b,n);left-=n;}
    if(exact&&fgetc(f)!=EOF)ok=0;if(ferror(f)||crc!=expected)ok=0;if(fclose(f))ok=0;return ok;
}
static int files_valid(const char*root,const FhInstallGeneration*g,unsigned full){
    char filename[FH_INSTALL_TX_PATH];
    for(unsigned i=0;i<FH_INSTALL_ARTIFACTS;i++)if(g->complete&(1u<<i)){
        if(!fh_install_artifact_path(root,g,i,filename))return 0;
        struct stat st;if(stat(filename,&st)||!S_ISREG(st.st_mode)||st.st_size!=g->bytes[i])return 0;
        if(full&&!scan_file(filename,g->bytes[i],g->crc[i],1))return 0;
    }
    return 1;
}
static int match(const FhInstallGeneration*g,const unsigned char*r,const unsigned char*s,unsigned required){return g->required==required&&!memcmp(g->recipe,r,32)&&!memcmp(g->source,s,32);}
static uint32_t generation_id(const char *name){
    if(strlen(name)!=9||name[0]!='g')return 0;
    uint32_t id=0;
    for(unsigned i=1;i<9;i++){
        unsigned digit=(unsigned char)name[i];
        if(digit>='0'&&digit<='9')digit-='0';
        else if(digit>='a'&&digit<='f')digit=digit-'a'+10;
        else return 0;
        id=(id<<4)|digit;
    }
    return id;
}
static int pair(const char*root,unsigned gen,unsigned committed,FhInstallGeneration*out,unsigned*slot){
    char filename[FH_INSTALL_TX_PATH],name[24];FhInstallGeneration g;int found=0;
    for(unsigned i=0;i<2;i++){
        snprintf(name,sizeof(name),"%s-%u.bin",committed?"current":"state",i);
        if(!path(filename,root,gen,name)||!read_record(filename,&g,committed)||(gen&&g.generation!=gen))continue;
        if(!found||g.serial>out->serial){*out=g;*slot=i;found=1;}
    }
    return found;
}
int fh_install_active(const char*root,const unsigned char*recipe,const unsigned char*source,unsigned required,FhInstallGeneration*out){
    if(!root||!recipe||!source||!out)return 0;
    char filename[FH_INSTALL_TX_PATH],name[24];FhInstallGeneration g;int found=0;
    for(unsigned i=0;i<2;i++){
        snprintf(name,sizeof(name),"current-%u.bin",i);
        if(!path(filename,root,0,name)||!read_record(filename,&g,1)||!match(&g,recipe,source,required)||!files_valid(root,&g,0))continue;
        if(!found||g.serial>out->serial){*out=g;found=1;}
    }
    return found;
}
void fh_install_transaction_close(FhInstallTransaction*t){if(t&&t->output){fclose(t->output);t->output=NULL;}}
static int checkpoint(FhInstallTransaction*t,FhInstallGeneration*g){
    if(t->failed)return 0;
    if(t->state.serial==UINT32_MAX)return error(t,"Asset checkpoint sequence exhausted.");
    g->serial=t->state.serial+1;char name[24],filename[FH_INSTALL_TX_PATH];unsigned slot=t->slot^1;
    snprintf(name,sizeof(name),"state-%u.bin",slot);
    if(!path(filename,t->root,g->generation,name)||!write_record(filename,g,0))return error(t,"Cannot save asset conversion checkpoint.");
    t->state=*g;t->slot=slot;return 1;
}
static int transaction_open(FhInstallTransaction*t,const char*root,const unsigned char*recipe,const unsigned char*source,unsigned required,unsigned fresh){
    if(!t||!root||!recipe||!source)return 0;
    /* Restart callers may pass the owner's current root/key fields. Retain
     * those before closing and clearing the previous task state. */
    char root_copy[sizeof(t->root)];unsigned char recipe_copy[32],source_copy[32];
    if(!*root||strlen(root)>=sizeof(root_copy)||strstr(root,"..")||!required||(required&~255u))return error(t,"Invalid generated-asset root or recipe.");
    strcpy(root_copy,root);memcpy(recipe_copy,recipe,32);memcpy(source_copy,source,32);
    root=root_copy;recipe=recipe_copy;source=source_copy;
    fh_install_transaction_close(t);memset(t,0,sizeof(*t));
    strcpy(t->root,root);if(mkdir(root,0777)&&errno!=EEXIST)return error(t,"Cannot create generated-asset directory.");
    DIR*dir=opendir(root);if(!dir)return error(t,"Cannot open generated-asset directory.");
    struct dirent*entry;uint32_t highest=0,protected[2]={0};unsigned seen=0,found=0;
    for(unsigned i=0;i<2;i++){
        char name[24],filename[FH_INSTALL_TX_PATH];FhInstallGeneration active;
        snprintf(name,sizeof(name),"current-%u.bin",i);
        if(path(filename,root,0,name)&&read_record(filename,&active,1))protected[i]=active.generation;
    }
    while((entry=readdir(dir))){
        unsigned id=generation_id(entry->d_name);if(!id)continue;
        if(++seen>4096){closedir(dir);return error(t,"Too many generated-asset generations.");}
        if(id>highest)highest=id;
        if(fresh||id==protected[0]||id==protected[1])continue;
        FhInstallGeneration g;unsigned slot;
        if(pair(root,id,0,&g,&slot)&&match(&g,recipe,source,required)&&(!found||id>t->state.generation)){
            t->state=g;t->slot=slot;found=1;
        }
    }
    closedir(dir);
    if(found){if(!files_valid(root,&t->state,1))return error(t,"Completed conversion data failed its checksum.");return 1;}
    if(highest==UINT32_MAX)return error(t,"Generated-asset generation sequence exhausted.");
    FhInstallGeneration g;memset(&g,0,sizeof(g));g.generation=highest+1;g.required=required;g.partial=FH_INSTALL_NO_ARTIFACT;
    memcpy(g.recipe,recipe,32);memcpy(g.source,source,32);
    char dirname[FH_INSTALL_TX_PATH];if(!path(dirname,root,g.generation,"")||mkdir(dirname,0777))return error(t,"Cannot create a new asset generation.");
    return checkpoint(t,&g);
}
int fh_install_transaction_open(FhInstallTransaction*t,const char*root,const unsigned char*recipe,const unsigned char*source,unsigned required){return transaction_open(t,root,recipe,source,required,0);}
int fh_install_transaction_restart(FhInstallTransaction*t,const char*root,const unsigned char*recipe,const unsigned char*source,unsigned required){return transaction_open(t,root,recipe,source,required,1);}
int fh_install_artifact_begin(FhInstallTransaction*t,unsigned id){
    if(!t||t->failed||t->output||id>=FH_INSTALL_ARTIFACTS||!(t->state.required&(1u<<id))||(t->state.complete&(1u<<id)))return 0;
    if(t->state.partial!=FH_INSTALL_NO_ARTIFACT&&t->state.partial!=id)return error(t,"Another asset is still being converted.");
    char filename[FH_INSTALL_TX_PATH];if(!part_path(t,id,filename))return error(t,"Asset path is too long.");
    if(t->state.partial==id){
        struct stat st;if(stat(filename,&st)){
            char final[FH_INSTALL_TX_PATH];
            if(errno!=ENOENT||!fh_install_artifact_path(t->root,&t->state,id,final)||rename(final,filename))return error(t,"Missing conversion checkpoint data.");
        }
        if(!scan_file(filename,t->state.partial_bytes,t->state.partial_crc,0))return error(t,"Partial conversion data failed its checksum.");
        t->output=fopen(filename,"r+b");
        if(!t->output||ftruncate(fileno(t->output),t->state.partial_bytes)||fseek(t->output,t->state.partial_bytes,SEEK_SET))return error(t,"Cannot resume conversion output.");
        t->written=t->state.partial_bytes;t->crc=t->state.partial_crc;t->checksum_dirty=0;return 1;
    }
    t->output=fopen(filename,"w+b");if(!t->output)return error(t,"Cannot create conversion output.");
    t->written=t->crc=t->checksum_dirty=0;FhInstallGeneration g=t->state;g.partial=id;g.partial_bytes=g.partial_crc=g.cursor[0]=g.cursor[1]=0;
    if(!sync_file(t->output))return error(t,"Cannot flush new conversion output.");
    return checkpoint(t,&g);
}
int fh_install_artifact_write(FhInstallTransaction*t,const void*bytes,uint32_t n){
    if(!t||t->failed||!t->output||(!bytes&&n))return 0;
    if(!n)return 1;
    if(n>INT32_MAX-t->written)return error(t,"Converted asset exceeds the supported file size.");
    if(fwrite(bytes,1,n,t->output)!=n)return error(t,"Cannot write conversion output; check SD space.");
    t->crc=(uint32_t)crc32(t->crc,bytes,n);t->written+=n;return 1;
}
int fh_install_artifact_checkpoint(FhInstallTransaction*t,uint32_t entry,uint32_t offset){
    if(!t||t->failed||!t->output)return 0;
    if(!sync_file(t->output))return error(t,"Cannot flush conversion output.");
    if(t->checksum_dirty){
        unsigned char buffer[CHUNK];uint32_t left=t->written,crc=0;
        if(fseek(t->output,0,SEEK_SET))return error(t,"Cannot read finalized asset header.");
        while(left){
            unsigned n=left>sizeof(buffer)?sizeof(buffer):left;
            if(fread(buffer,1,n,t->output)!=n)return error(t,"Cannot verify finalized asset header.");
            crc=(uint32_t)crc32(crc,buffer,n);left-=n;
        }
        if(fseek(t->output,t->written,SEEK_SET))return error(t,"Cannot restore conversion output position.");
        t->crc=crc;t->checksum_dirty=0;
    }
    FhInstallGeneration g=t->state;g.partial_bytes=t->written;g.partial_crc=t->crc;g.cursor[0]=entry;g.cursor[1]=offset;return checkpoint(t,&g);
}
int fh_install_artifact_patch(FhInstallTransaction*t,uint32_t offset,const void*data,uint32_t bytes){
    if(!t||t->failed||!t->output||(!data&&bytes))return 0;
    if(offset>t->written||bytes>t->written-offset||t->state.partial_bytes)return error(t,"Cannot overwrite checkpointed conversion data.");
    if(!bytes)return 1;
    if(fseek(t->output,offset,SEEK_SET)||fwrite(data,1,bytes,t->output)!=bytes||fseek(t->output,t->written,SEEK_SET))return error(t,"Cannot finalize converted asset header.");
    t->checksum_dirty=1;return 1;
}
int fh_install_artifact_read(FhInstallTransaction*t,uint32_t offset,void*data,uint32_t bytes){
    if(!t||t->failed||!t->output||(!data&&bytes))return 0;
    if(offset>t->written||bytes>t->written-offset)return error(t,"Read outside owned conversion output.");
    if(fseek(t->output,offset,SEEK_SET)||fread(data,1,bytes,t->output)!=bytes||fseek(t->output,t->written,SEEK_SET))return error(t,"Cannot read conversion staging data.");
    return 1;
}
int fh_install_artifact_finish(FhInstallTransaction*t,FhInstallValidate validate,void*context){
    if(!t||t->failed||!t->output||!validate)return 0;
    if(!fh_install_artifact_checkpoint(t,t->state.cursor[0],t->state.cursor[1]))return 0;
    int closed=fclose(t->output);t->output=NULL;if(closed)return error(t,"Cannot close converted asset.");
    unsigned id=t->state.partial;char partial[FH_INSTALL_TX_PATH],final[FH_INSTALL_TX_PATH];
    if(!part_path(t,id,partial)||!fh_install_artifact_path(t->root,&t->state,id,final))return error(t,"Asset path is too long.");
    if(!scan_file(partial,t->written,t->crc,1)||!validate(partial,context))return error(t,"Converted asset failed validation.");
    struct stat st;if(!stat(final,&st)||errno!=ENOENT||rename(partial,final))return error(t,"Cannot finalize converted asset.");
    FhInstallGeneration g=t->state;g.bytes[id]=t->written;g.crc[id]=t->crc;g.complete|=1u<<id;
    g.partial=FH_INSTALL_NO_ARTIFACT;g.partial_bytes=g.partial_crc=g.cursor[0]=g.cursor[1]=0;return checkpoint(t,&g);
}
int fh_install_transaction_publish(FhInstallTransaction*t){
    if(!t||t->failed||t->output||t->state.partial!=FH_INSTALL_NO_ARTIFACT||t->state.complete!=t->state.required)return 0;
    if(!files_valid(t->root,&t->state,1))return error(t,"Generated assets failed final readback.");
    FhInstallGeneration current,g=t->state;unsigned slot=1;uint32_t serial=0,usable_serial=0;
    for(unsigned i=0;i<2;i++){
        char name[24],filename[FH_INSTALL_TX_PATH];snprintf(name,sizeof(name),"current-%u.bin",i);
        if(!path(filename,t->root,0,name)||!read_record(filename,&current,1))continue;
        if(current.serial>serial)serial=current.serial;
        if(files_valid(t->root,&current,0)&&current.serial>usable_serial){usable_serial=current.serial;slot=i;}
    }
    if(serial==UINT32_MAX)return error(t,"Active asset sequence exhausted.");g.serial=serial+1;
    char name[24],filename[FH_INSTALL_TX_PATH];snprintf(name,sizeof(name),"current-%u.bin",slot^1);
    if(!path(filename,t->root,0,name)||!write_record(filename,&g,1))return error(t,"Cannot activate the new asset generation.");
    return 1;
}

/* Optional sequential validator. Existing transaction entry points above are
 * unchanged; this path combines their whole-file CRC and semantic readback. */
#include "fh_install_readback.h"
struct FhInstallReadback {FILE*file;uint32_t size,read,crc;unsigned failed;};
uint32_t fh_install_readback_size(const FhInstallReadback*r){return r?r->size:0;}
int fh_install_readback_read(FhInstallReadback*r,void*data,uint32_t bytes){
    if(!r||r->failed)return 0;
    if((!data&&bytes)||r->read>r->size||bytes>r->size-r->read){r->failed=1;return 0;}
    if(!bytes)return 1;
    if(fread(data,1,bytes,r->file)!=bytes){r->failed=1;return 0;}
    r->crc=(uint32_t)crc32(r->crc,data,bytes);r->read+=bytes;return 1;
}
static int readback_valid(FILE*f,uint32_t bytes,uint32_t crc,FhInstallReadbackValidate validate,void*context){
    FhInstallReadback r={f,bytes,0,0,0};
    return validate(&r,context)&&!r.failed&&r.read==bytes&&r.crc==crc&&fgetc(f)==EOF&&!ferror(f);
}
int fh_install_artifact_finish_readback(FhInstallTransaction*t,FhInstallReadbackValidate validate,void*context){
    if(!t||t->failed||!t->output||!validate)return 0;
    if(!fh_install_artifact_checkpoint(t,t->state.cursor[0],t->state.cursor[1]))return 0;
    int closed=fclose(t->output);t->output=NULL;if(closed)return error(t,"Cannot close converted asset.");
    unsigned id=t->state.partial;char partial[FH_INSTALL_TX_PATH],final[FH_INSTALL_TX_PATH];
    if(!part_path(t,id,partial)||!fh_install_artifact_path(t->root,&t->state,id,final))return error(t,"Asset path is too long.");
    FILE*f=fopen(partial,"rb");if(!f)return error(t,"Cannot open converted asset for validation.");
    int valid=readback_valid(f,t->written,t->crc,validate,context);if(fclose(f))valid=0;
    if(!valid)return error(t,"Converted asset failed validation.");
    struct stat st;if(!stat(final,&st)||errno!=ENOENT||rename(partial,final))return error(t,"Cannot finalize converted asset.");
    FhInstallGeneration g=t->state;g.bytes[id]=t->written;g.crc[id]=t->crc;g.complete|=1u<<id;
    g.partial=FH_INSTALL_NO_ARTIFACT;g.partial_bytes=g.partial_crc=g.cursor[0]=g.cursor[1]=0;return checkpoint(t,&g);
}
