#include "fh_install_bundle.h"
#include <mbedtls/sha256.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

enum {MANIFEST=500,CHUNK=16384};
static const char*stage_names[4]={"original","performance","textures","movie"};
static const unsigned masks[4]={15,15,3,3};
static const unsigned stage_ids[8]={0,0,0,1,1,1,2,3},artifact_ids[8]={3,1,2,1,2,3,1,1};
/* Movie storage follows the prepared reader's 300 MiB file bound. Validation
 * remains streamed through CHUNK bytes; this is not a resident RAM allocation. */
static const unsigned caps[8]={16,64*1024*1024,80*1024*1024,32*1024*1024,64*1024*1024,80*1024*1024,128*1024*1024,300*1024*1024};
static const char*labels[8]={"Checking original topology","Checking original geometry","Checking original cache","Checking performance topology","Checking performance geometry","Checking performance cache","Checking textures","Checking movie"};
static const char*magic[8]={"FHMLPK1","FHNSSET1","FHBCACH1","FHMLPK1","FHNSSET1","FHBCACH1","FHTXPK3","THP\0\0\1\0\0"};
static unsigned get(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static void put(unsigned char*p,unsigned n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
static int valid_root(const char*root){return root&&*root&&strlen(root)<220&&!strstr(root,"..");}
int fh_install_bundle_stage_root(const char*root,unsigned stage,char*out){
    if(!out||!valid_root(root)||stage>=4)return 0;
    int n=snprintf(out,FH_INSTALL_TX_PATH,"%s/%s",root,stage_names[stage]);return n>0&&n<240;
}
unsigned fh_install_bundle_stage_mask(unsigned stage){return stage<4?masks[stage]:0;}
static int bundle_root(const char*root,char*out){if(!valid_root(root))return 0;int n=snprintf(out,FH_INSTALL_TX_PATH,"%s/bundle",root);return n>0&&n<240;}
static int recipe(const FhInstallBundleSpec*s,unsigned char*out){
    unsigned char bytes[136];if(!s||!s->disc_bytes)return 0;
    memcpy(bytes,"FHBNDL01",8);memcpy(bytes+8,s->stage_recipe,128);return !mbedtls_sha256_ret(bytes,sizeof(bytes),out,0);
}
static int stages_valid(const FhInstallBundleSpec*s,const FhInstallGeneration*g){
    for(unsigned i=0;i<4;i++)if(!g[i].generation||g[i].required!=masks[i]||g[i].complete!=masks[i]||
       g[i].partial!=FH_INSTALL_NO_ARTIFACT||memcmp(g[i].recipe,s->stage_recipe[i],32)||memcmp(g[i].source,s->source,32))return 0;
    for(unsigned i=0;i<8;i++){
        unsigned n=g[stage_ids[i]].bytes[artifact_ids[i]];
        if(n<16||n>caps[i]||(i==0&&n!=16)||(i==7&&n<192))return 0;
    }
    return 1;
}
int fh_install_bundle_path(const FhInstallBundle*b,unsigned asset,char*out){
    char root[FH_INSTALL_TX_PATH];if(!b||!out||asset>=8||!fh_install_bundle_stage_root(b->root,stage_ids[asset],root))return 0;
    return fh_install_artifact_path(root,b->stages+stage_ids[asset],artifact_ids[asset],out);
}
static int sizes_valid(const FhInstallBundle*b){
    for(unsigned i=0;i<8;i++){
        char path[FH_INSTALL_TX_PATH];struct stat st;
        if(!fh_install_bundle_path(b,i,path)||stat(path,&st)||!S_ISREG(st.st_mode)||st.st_size!=(long)b->stages[stage_ids[i]].bytes[artifact_ids[i]])return 0;
    }
    return 1;
}
static void encode(unsigned char*out,const FhInstallBundleSpec*s,const FhInstallGeneration*g,const unsigned char*key){
    memset(out,0,MANIFEST);memcpy(out,"FHBNDL01",8);memcpy(out+8,key,32);memcpy(out+40,s->source,32);put(out+72,s->disc_bytes);put(out+76,s->fst_crc);
    for(unsigned i=0;i<4;i++){
        unsigned char*p=out+80+i*104;put(p,g[i].generation);put(p+4,g[i].required);memcpy(p+8,g[i].recipe,32);
        for(unsigned j=0;j<8;j++){put(p+40+j*8,g[i].bytes[j]);put(p+44+j*8,g[i].crc[j]);}
    }
    put(out+496,(unsigned)crc32(0,out,496));
}
static int decode(const unsigned char*data,const FhInstallBundleSpec*s,const unsigned char*key,FhInstallBundle*out){
    if(memcmp(data,"FHBNDL01",8)||memcmp(data+8,key,32)||memcmp(data+40,s->source,32)||get(data+72)!=s->disc_bytes||get(data+76)!=s->fst_crc||get(data+496)!=(unsigned)crc32(0,data,496))return 0;
    for(unsigned i=0;i<4;i++){
        const unsigned char*p=data+80+i*104;FhInstallGeneration*g=out->stages+i;memset(g,0,sizeof(*g));
        g->generation=get(p);g->required=g->complete=get(p+4);g->partial=FH_INSTALL_NO_ARTIFACT;memcpy(g->recipe,p+8,32);memcpy(g->source,s->source,32);
        for(unsigned j=0;j<8;j++){g->bytes[j]=get(p+40+j*8);g->crc[j]=get(p+44+j*8);}
    }
    return stages_valid(s,out->stages);
}
static int read_exact(const char*path,void*out,unsigned n){
    FILE*f=fopen(path,"rb");if(!f)return 0;int ok=fread(out,1,n,f)==n&&fgetc(f)==EOF&&!ferror(f);if(fclose(f))ok=0;return ok;
}
/* Read both existing transaction commit slots so a bad newer bundle manifest
 * or missing child can fall back without scanning generation directories. */
int fh_install_bundle_open(const char*root,const FhInstallBundleSpec*s,FhInstallBundle*out){
    unsigned char key[32];char directory[FH_INSTALL_TX_PATH];if(!out)return 0;memset(out,0,sizeof(*out));
    if(!bundle_root(root,directory)||!recipe(s,key))return 0;
    for(unsigned slot=0;slot<2;slot++){
        char path[FH_INSTALL_TX_PATH];unsigned char record[192],manifest[MANIFEST];
        int n=snprintf(path,sizeof(path),"%s/current-%u.bin",directory,slot);if(n<0||n>=(int)sizeof(path)||!read_exact(path,record,sizeof(record)))continue;
        if(memcmp(record,"FHIPREP1",8)||get(record+8)!=1||get(record+188)!=(unsigned)crc32(0,record,188)||
           !get(record+12)||!get(record+16)||get(record+20)!=1||get(record+24)!=1||get(record+28)!=FH_INSTALL_NO_ARTIFACT||
           get(record+32)||get(record+36)||get(record+40)||get(record+44)||memcmp(record+48,key,32)||memcmp(record+80,s->source,32)||get(record+112)!=MANIFEST)continue;
        FhInstallGeneration generation={0};generation.generation=get(record+12);
        if(!fh_install_artifact_path(directory,&generation,0,path)||!read_exact(path,manifest,sizeof(manifest))||get(record+116)!=(unsigned)crc32(0,manifest,sizeof(manifest)))continue;
        FhInstallBundle candidate={0};strcpy(candidate.root,root);candidate.generation=generation.generation;candidate.serial=get(record+16);
        if(decode(manifest,s,key,&candidate)&&sizes_valid(&candidate)&&candidate.serial>out->serial)*out=candidate;
    }
    return out->serial!=0;
}
static int file_check(const FhInstallBundle*b,unsigned i,FhInstallBundleProgress progress,void*context){
    char path[FH_INSTALL_TX_PATH];if(!fh_install_bundle_path(b,i,path))return 0;
    unsigned total=b->stages[stage_ids[i]].bytes[artifact_ids[i]],expected=b->stages[stage_ids[i]].crc[artifact_ids[i]];
    FILE*f=fopen(path,"rb");if(!f)return 0;unsigned char bytes[CHUNK];unsigned at=0,crc=0;int result=1;
    while(at<total){
        if(progress&&!progress(context,labels[i],at,total)){result=-1;break;}
        unsigned n=total-at;if(n>sizeof(bytes))n=sizeof(bytes);
        if(fread(bytes,1,n,f)!=n){result=0;break;}
        if(!at){
            /* Both typed formats are produced by checked texture stages. v4
             * adds padded CMPR; do not accept arbitrary future headers. */
            int header_ok=!memcmp(bytes,magic[i],8)||(i==6&&(!memcmp(bytes,"FHTXPK4",8)||!memcmp(bytes,"FHTXPK5",8)));
            if(!header_ok||(i==0&&memcmp(bytes+8,"\0\0\0\0\0\0\0\0",8))){result=0;break;}
            if(i==2||i==5){unsigned scene=i==2?1:2;const FhInstallGeneration*g=b->stages+stage_ids[i];
                if(n<32||get(bytes+8)!=g->bytes[scene]||get(bytes+24)!=g->crc[scene]){result=0;break;}}
            if(i==7&&(n<192||(memcmp(bytes+128,"FHPICA01",8)&&memcmp(bytes+128,"FHRGB801",8)))){result=0;break;}
        }
        crc=(unsigned)crc32(crc,bytes,n);at+=n;
    }
    if(result==1&&(crc!=expected||fgetc(f)!=EOF||ferror(f)))result=0;
    if(fclose(f)&&result==1)result=0;
    if(result==1&&progress&&!progress(context,labels[i],total,total))result=-1;
    return result;
}
typedef struct {const FhInstallBundleSpec*spec;const unsigned char*key;} Validation;
static int manifest_valid(const char*path,void*context){
    Validation*v=context;unsigned char data[MANIFEST];FhInstallBundle b={0};
    return read_exact(path,data,sizeof(data))&&decode(data,v->spec,v->key,&b);
}
int fh_install_bundle_publish(const char*root,const FhInstallBundleSpec*s,const FhInstallGeneration stages[4],FhInstallBundleProgress progress,void*context,char*error,size_t capacity){
    FhInstallBundle b={0};FhInstallTransaction t={0};unsigned char key[32],manifest[MANIFEST];char directory[FH_INSTALL_TX_PATH];int result=0;
    if(error&&capacity)error[0]=0;
    if(!stages||!bundle_root(root,directory)||!recipe(s,key)||!stages_valid(s,stages))goto done;
    strcpy(b.root,root);memcpy(b.stages,stages,sizeof(b.stages));if(!sizes_valid(&b))goto done;
    for(unsigned i=0;i<8;i++){result=file_check(&b,i,progress,context);if(result!=1)goto done;}
    result=0;encode(manifest,s,stages,key);Validation validation={s,key};
    /* This tiny metadata artifact restarts after interruption; large completed
     * child stages are retained and are never re-encoded here. */
    if(!fh_install_transaction_restart(&t,directory,key,s->source,1)||!fh_install_artifact_begin(&t,0)||
       !fh_install_artifact_write(&t,manifest,sizeof(manifest))||!fh_install_artifact_finish(&t,manifest_valid,&validation)||
       !sizes_valid(&b)||!fh_install_transaction_publish(&t))goto done;
    result=1;
done:
    if(!result&&error&&capacity)snprintf(error,capacity,"%s",t.error[0]?t.error:"Prepared bundle is incomplete or failed validation.");
    fh_install_transaction_close(&t);return result;
}
