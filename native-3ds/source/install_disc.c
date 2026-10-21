#include "fh_install_disc.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static uint32_t word(const unsigned char*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
void fh_install_disc_close(FhInstallDisc*d){
    if(!d)return;
    if(d->file)fclose(d->file);
    free(d->fst);d->file=NULL;d->fst=NULL;d->bytes=d->fst_bytes=d->entries=d->fst_crc=0;
}
static int fail(FhInstallDisc*d,const char*why){
    snprintf(d->error,sizeof(d->error),"%s",why);fh_install_disc_close(d);return 0;
}
static int read_at(FhInstallDisc*d,uint32_t at,void*out,uint32_t n){
    if(at>d->bytes || n>d->bytes-at || (!out&&n))return 0;
    if(fseek(d->file,(long)at,SEEK_SET))return 0;
    unsigned char*p=out;
    while(n){uint32_t take=n>FH_INSTALL_READ_CHUNK?FH_INSTALL_READ_CHUNK:n;
        if(fread(p,1,take,d->file)!=take)return 0;
        p+=take;n-=take;
    }
    return 1;
}
static const char* name(const FhInstallDisc*d,unsigned id){return (const char*)d->fst+d->entries*12+(word(d->fst+id*12)&0xffffff);}
int fh_install_disc_open(FhInstallDisc*d,const char*path){
    if(!d)return 0;
    fh_install_disc_close(d);d->error[0]=0;
    if(!path || !(d->file=fopen(path,"rb")))return fail(d,"Place the PAL revision 1 ISO at /3ds/foxhollow/disc.iso");
    long length;
    if(fseek(d->file,0,SEEK_END) || (length=ftell(d->file))<0x440 || length>INT32_MAX)
        return fail(d,"The disc image is incomplete or too large.");
    d->bytes=(uint32_t)length;unsigned char h[0x440];
    if(!read_at(d,0,h,sizeof(h)))return fail(d,"Cannot read the disc header.");
    if(memcmp(h,"GSAP01\0\1",8) || word(h+0x1c)!=0xc2339f3d)
        return fail(d,"This build needs Star Fox Adventures PAL revision 1 ISO.");
    uint32_t at=word(h+0x424),n=word(h+0x428);
    if(at<sizeof(h) || n<13 || n>FH_INSTALL_FST_CAP || at>d->bytes || n>d->bytes-at)
        return fail(d,"Invalid disc file table extent.");
    d->fst=malloc(n);d->fst_bytes=n;
    if(!d->fst || !read_at(d,at,d->fst,n))return fail(d,"Cannot load the disc file table.");
    d->entries=word(d->fst+8);
    if(word(d->fst)!=0x01000000 || word(d->fst+4)!=0 || !d->entries ||
       d->entries>FH_INSTALL_ENTRY_CAP || d->entries*12>=n)return fail(d,"Invalid disc root directory.");
    uint32_t stack[128]={0};unsigned depth=1;
    for(unsigned i=0;i<d->entries;i++){
        const unsigned char*e=d->fst+i*12;unsigned kind=e[0];uint32_t text=word(e)&0xffffff;
        if(kind>1 || text>=n-d->entries*12)return fail(d,"Invalid disc filename offset.");
        const char*p=name(d,i);const char*end=memchr(p,0,n-d->entries*12-text);
        if(!end)return fail(d,"Unterminated disc filename.");
        /* Root's name offset is not a path component. The PAL image's string
         * table begins with an ordinary filename rather than an empty string. */
        if(i){
            if(p==end || !strcmp(p,".") || !strcmp(p,".."))return fail(d,"Invalid disc filename.");
            for(const char*q=p;q<end;q++)if((unsigned char)*q<32 || *q=='/' || *q=='\\' || *q==':')
                return fail(d,"Invalid disc filename character.");
        }
        while(depth>1 && i>=word(d->fst+stack[depth-1]*12+8))depth--;
        unsigned parent=i?stack[depth-1]:0;d->parent[i]=parent;
        uint32_t offset=word(e+4),size=word(e+8);
        if(kind){
            if(size<=i || size>d->entries || (i&&(size>word(d->fst+parent*12+8) || offset!=parent)))
                return fail(d,"Invalid disc directory hierarchy.");
            if(i){if(depth==128)return fail(d,"Disc directories exceed the supported depth.");stack[depth++]=i;}
        }else if(offset>d->bytes || size>d->bytes-offset)return fail(d,"A disc file extends beyond the image.");
    }
    d->fst_crc=(uint32_t)crc32(0,d->fst,n);return 1;
}
int fh_install_disc_entry(const FhInstallDisc*d,unsigned id,FhInstallEntry*out){
    if(!d || !d->file || !out || id>=d->entries)return 0;
    const unsigned char*p=d->fst+id*12;
    out->offset=word(p+4);out->bytes=word(p+8);out->directory=p[0];return 1;
}
int fh_install_disc_path(const FhInstallDisc*d,unsigned id,char*out,size_t cap){
    if(!d || !d->file || !out || !cap || id>=d->entries)return 0;
    uint32_t chain[128];unsigned depth=0;size_t needed=1;
    for(unsigned i=id;i;i=d->parent[i]){
        if(depth==128)return 0;
        chain[depth++]=i;needed+=strlen(name(d,i))+1;
    }
    if(needed+(depth?0u:1u)>cap)return 0;
    char*p=out;*p++='/';
    while(depth){const char*s=name(d,chain[--depth]);size_t n=strlen(s);memcpy(p,s,n);p+=n;if(depth)*p++='/';}
    *p=0;return 1;
}
static unsigned lower(unsigned c){return c>='A'&&c<='Z'?c+32:c;}
static int equal(const char*a,const char*b){while(*a&&*b){if(lower((unsigned char)*a++)!=lower((unsigned char)*b++))return 0;}return *a==*b;}
int fh_install_disc_find(const FhInstallDisc*d,const char*path){
    if(!d || !d->file || !path || path[0]!='/')return -1;
    char candidate[FH_INSTALL_PATH_CAP];int found=-1;
    for(unsigned i=0;i<d->entries;i++)if(fh_install_disc_path(d,i,candidate,sizeof(candidate))&&equal(candidate,path)){
        if(found>=0)return -1;found=(int)i;
    }
    return found;
}
int fh_install_disc_read(FhInstallDisc*d,unsigned id,uint32_t offset,void*out,uint32_t bytes){
    FhInstallEntry e;if(!fh_install_disc_entry(d,id,&e)||e.directory||offset>e.bytes||bytes>e.bytes-offset)return 0;
    return read_at(d,e.offset+offset,out,bytes);
}
