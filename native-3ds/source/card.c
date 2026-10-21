#include <dolphin/card.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fh_platform.h"
#include <zlib.h>

/* GCI persistence follows Melee's temporary/backup recovery scheme.
 * Unlike its BE8 engine, this bridge encodes the 64-byte header explicitly. */
enum { SECTOR=8192, CAPACITY=251*SECTOR, HEADER=64, RETAIN_LIMIT=128*1024 };
typedef struct { unsigned char *image; unsigned bytes; char disk_name[96]; } Save;
static Save saves[CARD_MAX_FILE];
static char directory[512]="sdmc:/3ds/foxhollow/saves";
static unsigned char game_id[4],maker_id[2];
static int mounted,initialized,loaded;
static unsigned used,allocated,cold_mounts,warm_mounts,pressure_releases;
static unsigned read16(const unsigned char *p) { return (unsigned)p[0]<<8|p[1]; }
static unsigned read32(const unsigned char *p) { return read16(p)<<16|read16(p+2); }
static void write16(unsigned char *p,unsigned value) { p[0]=value>>8;p[1]=value; }
static void write32(unsigned char *p,unsigned value) { write16(p,value>>16);write16(p+2,value); }
static int path(char *out,size_t bytes,const char *name,const char *suffix) {
    int n=snprintf(out,bytes,"%s/%s%s",directory,name,suffix);
    return n>=0 && (size_t)n<bytes;
}
static int exists(const char *name) { struct stat statbuf;return !stat(name,&statbuf); }
static void release(void) {
    for(unsigned i=0;i<CARD_MAX_FILE;i++) { free(saves[i].image);memset(&saves[i],0,sizeof(saves[i])); }
    used=allocated=0;mounted=loaded=0;
}
void fh_card_stats(FhCardStats *out) {
    *out=(FhCardStats){allocated,RETAIN_LIMIT,cold_mounts,warm_mounts,pressure_releases,(unsigned)mounted,(unsigned)loaded};
}
void fh_card_shutdown(void) {
    char text[192];snprintf(text,sizeof(text),"Native card inventory: bytes=%u retain_limit=%u cold_mounts=%u warm_mounts=%u releases=%u\n",
        allocated,RETAIN_LIMIT,cold_mounts,warm_mounts,pressure_releases);fh_platform_log(text);
    release();initialized=0;cold_mounts=warm_mounts=pressure_releases=0;
}
int fh_card_set_directory(const char *name) {
    if(initialized || !name || !*name || strlen(name)>=sizeof(directory)-100) return 0;
    strcpy(directory,name);return 1;
}
static int make_directory(void) {
    char copy[sizeof(directory)];strcpy(copy,directory);
    for(char *p=copy+1;*p;p++) if(*p=='/' && p[-1]!=':') {
        *p=0;if(mkdir(copy,0777) && errno!=EEXIST) return 0;*p='/';
    }
    return !mkdir(copy,0777) || errno==EEXIST;
}
static int persist(const Save *save) {
    char name[640],temporary[640],backup[640];
    if(!path(name,sizeof(name),save->disk_name,"") || !path(temporary,sizeof(temporary),save->disk_name,".tmp") || !path(backup,sizeof(backup),save->disk_name,".bak")) return 0;
    FILE *file=fopen(temporary,"wb");if(!file) return 0;
    int ok=fwrite(save->image,1,HEADER+save->bytes,file)==HEADER+save->bytes;
    if(fflush(file)) ok=0;
    if(fclose(file)) ok=0;
    if(!ok) { remove(temporary);return 0; }
    if(exists(name)) {
        if(exists(backup) && remove(backup)) { remove(temporary);return 0; }
        if(rename(name,backup)) { remove(temporary);return 0; }
    }
    if(rename(temporary,name)) {
        if(!exists(name) && exists(backup)) rename(backup,name);
        return 0;
    }
    return 1;
}
static int ready(int channel) { return channel!=0 || !mounted?CARD_RESULT_NOCARD:0; }
static int access_save(int channel,int number) {
    int result=ready(channel);if(result) return result;
    if(number<0 || number>=CARD_MAX_FILE || !saves[number].image) return CARD_RESULT_NOFILE;
    if(memcmp(saves[number].image,game_id,4) || memcmp(saves[number].image+4,maker_id,2)) return CARD_RESULT_NOPERM;
    return 0;
}
static int valid_name(const char *name) { return name && *name && strnlen(name,33)<=32; }
static int find(const char *name) {
    if(!valid_name(name)) return -1;
    for(unsigned i=0;i<CARD_MAX_FILE;i++) if(saves[i].image && !access_save(0,i) && !strncmp((char*)saves[i].image+8,name,32)) return i;
    return -1;
}
static void file_info(CARDFileInfo *info,int number) {
    memset(info,0,sizeof(*info));info->fileNo=number;info->length=saves[number].bytes;info->iBlock=5;
}
void CARDSetLoadType(CARDFileType type) {
    if(type!=CARD_GCIFOLDER) fh_platform_panic("Native saves use GCI storage; raw card images are unsupported");
}
void CARDSetCardImagePath(const char *name,s32 channel) {
    (void)name;(void)channel;fh_platform_panic("Raw memory-card image configured for native port");
}
void CARDInit(const char *game,const char *maker) {
    if(initialized) return;
    if(!game || !maker) fh_platform_panic("Missing save game identity");
    memcpy(game_id,game,4);memcpy(maker_id,maker,2);initialized=1;
}
s32 CARDProbeEx(s32 channel,s32 *size,s32 *sector) {
    if(channel!=0 || !initialized) return CARD_RESULT_NOCARD;
    if(size) *size=16;if(sector) *sector=SECTOR;return 0;
}
s32 CARDMount(s32 channel,void *area,CARDCallback callback) {
    (void)area;(void)callback;
    if(CARDProbeEx(channel,NULL,NULL)) return CARD_RESULT_NOCARD;
    if(mounted) return 0;
    /* Melee's virtual card retains its loaded inventory across SDK unmounts.
     * This process exclusively owns the virtual SD card; FTP/imports happen
     * between launches. Do not rescan directories/reload identical GCI bytes
     * for each game save. Writes still finish durably before reporting success.
     * A failed write or an oversized inventory forces the next cold reload. */
    if(loaded) { mounted=1;warm_mounts++;return 0; }
    cold_mounts++;
    if(!make_directory()) return CARD_RESULT_IOERROR;
    DIR *dir=opendir(directory);if(!dir) return CARD_RESULT_IOERROR;
    struct dirent *entry;char name[640],backup[640];
    while((entry=readdir(dir))) {
        size_t n=strlen(entry->d_name);
        if(n>8 && n<96 && !strcmp(entry->d_name+n-8,".gci.bak")) {
            char base[96];memcpy(base,entry->d_name,n-4);base[n-4]=0;
            path(name,sizeof(name),base,"");path(backup,sizeof(backup),entry->d_name,"");
            if(!exists(name) && rename(backup,name)) { closedir(dir);return CARD_RESULT_IOERROR; }
        }
    }
    closedir(dir);dir=opendir(directory);if(!dir) return CARD_RESULT_IOERROR;
    int slot=0,result=0;
    while((entry=readdir(dir))) {
        size_t n=strlen(entry->d_name);
        if(n<5 || strcmp(entry->d_name+n-4,".gci")) continue;
        if(n>=96 || slot==CARD_MAX_FILE) { result=CARD_RESULT_BROKEN;break; }
        path(name,sizeof(name),entry->d_name,"");
        FILE *file=fopen(name,"rb");unsigned char header[HEADER];
        if(!file) { result=CARD_RESULT_IOERROR;break; }
        if(fread(header,1,HEADER,file)!=HEADER) { fclose(file);result=CARD_RESULT_BROKEN;break; }
        unsigned bytes=read16(header+56)*SECTOR;
        if(!bytes || bytes>CAPACITY-used || header[0]==255) { fclose(file);result=CARD_RESULT_BROKEN;break; }
        Save *save=&saves[slot];save->image=malloc(HEADER+bytes);
        if(!save->image) { fclose(file);result=CARD_RESULT_IOERROR;break; }
        memcpy(save->image,header,HEADER);save->bytes=bytes;strcpy(save->disk_name,entry->d_name);
        if(fread(save->image+HEADER,1,bytes,file)!=bytes || fgetc(file)!=EOF || ferror(file)) result=CARD_RESULT_BROKEN;
        fclose(file);if(result) break;
        used+=bytes;allocated+=HEADER+bytes;slot++;
    }
    closedir(dir);
    if(result) { release();return result; }
    mounted=loaded=1;return 0;
}
s32 CARDUnmount(s32 channel) {
    if(channel) return CARD_RESULT_NOCARD;
    mounted=0;
    if(!loaded || allocated>RETAIN_LIMIT) { if(allocated)pressure_releases++;release(); }
    return 0;
}
s32 CARDCheck(s32 channel) { return ready(channel); }
s32 CARDGetSerialNo(s32 channel,u64 *serial) {
    int result=ready(channel);if(result) return result;
    if(!serial) return CARD_RESULT_FATAL_ERROR;
    *serial=0x46484e3344530001ULL;return 0;
}
s32 CARDOpen(s32 channel,const char *name,CARDFileInfo *info) {
    int result=ready(channel);if(result) return result;
    if(!info || !valid_name(name)) return CARD_RESULT_FATAL_ERROR;
    int number=find(name);if(number<0) return CARD_RESULT_NOFILE;
    file_info(info,number);return 0;
}
s32 CARDClose(CARDFileInfo *info) {
    if(!info) return CARD_RESULT_FATAL_ERROR;
    int result=access_save(info->chan,info->fileNo);if(!result) info->fileNo=-1;return result;
}
s32 CARDCreate(s32 channel,const char *name,u32 bytes,CARDFileInfo *info) {
    int result=ready(channel);if(result) return result;
    if(!valid_name(name)) return CARD_RESULT_NAMETOOLONG;
    if(!info || !bytes || bytes%SECTOR) return CARD_RESULT_FATAL_ERROR;
    if(find(name)>=0) return CARD_RESULT_EXIST;
    if(bytes>CAPACITY-used) return CARD_RESULT_INSSPACE;
    int number=0;while(number<CARD_MAX_FILE && saves[number].image) number++;
    if(number==CARD_MAX_FILE) return CARD_RESULT_NOENT;
    Save candidate={.image=calloc(1,HEADER+bytes),.bytes=bytes};
    if(!candidate.image) return CARD_RESULT_IOERROR;
    memcpy(candidate.image,game_id,4);memcpy(candidate.image+4,maker_id,2);
    memcpy(candidate.image+8,name,strlen(name));
    write32(candidate.image+44,0xffffffff);write32(candidate.image+60,0xffffffff);
    write16(candidate.image+54,5);write16(candidate.image+56,bytes/SECTOR);
    const char hex[]="0123456789abcdef";
    for(unsigned i=0;i<38;i++) { unsigned c=candidate.image[i<6?i:i+2];candidate.disk_name[2*i]=hex[c>>4];candidate.disk_name[2*i+1]=hex[c&15]; }
    memcpy(candidate.disk_name+76,".gci",5);
    if(!persist(&candidate)) { loaded=0;free(candidate.image);return CARD_RESULT_IOERROR; }
    saves[number]=candidate;used+=bytes;allocated+=HEADER+bytes;file_info(info,number);return 0;
}
static int transfer(const CARDFileInfo *info,const void *memory,s32 bytes,s32 offset) {
    if(!info || !memory || bytes<0 || offset<0) return CARD_RESULT_FATAL_ERROR;
    int result=access_save(info->chan,info->fileNo);if(result) return result;
    unsigned length=saves[info->fileNo].bytes;
    return (unsigned)offset>length || (unsigned)bytes>length-(unsigned)offset?CARD_RESULT_LIMIT:0;
}
s32 CARDRead(const CARDFileInfo *info,void *memory,s32 bytes,s32 offset) {
    int result=transfer(info,memory,bytes,offset);if(result) return result;
    memcpy(memory,saves[info->fileNo].image+HEADER+offset,bytes);return 0;
}
s32 CARDWrite(const CARDFileInfo *info,const void *memory,s32 bytes,s32 offset) {
    int result=transfer(info,memory,bytes,offset);if(result) return result;
    Save *save=&saves[info->fileNo],candidate=*save;
    candidate.image=malloc(HEADER+save->bytes);if(!candidate.image) return CARD_RESULT_IOERROR;
    memcpy(candidate.image,save->image,HEADER+save->bytes);
    memcpy(candidate.image+HEADER+offset,memory,bytes);
    if(!persist(&candidate)) { loaded=0;free(candidate.image);return CARD_RESULT_IOERROR; }
    free(save->image);*save=candidate;return 0;
}
s32 CARDGetStatus(s32 channel,s32 number,CARDStat *status) {
    int result=access_save(channel,number);if(result) return result;
    if(!status) return CARD_RESULT_FATAL_ERROR;
    const unsigned char *h=saves[number].image;memset(status,0,sizeof(*status));
    memcpy(status->gameName,h,4);memcpy(status->company,h+4,2);memcpy(status->fileName,h+8,32);
    status->length=saves[number].bytes;status->time=read32(h+40);status->bannerFormat=h[7];
    status->iconAddr=read32(h+44);status->iconFormat=read16(h+48);status->iconSpeed=read16(h+50);status->commentAddr=read32(h+60);
    unsigned offset=status->iconAddr;int tlut=0;
    status->offsetBanner=status->offsetBannerTlut=status->offsetIconTlut=0xffffffff;
    for(unsigned i=0;i<8;i++) status->offsetIcon[i]=0xffffffff;
    if(offset==0xffffffff) { status->bannerFormat=status->iconFormat=status->iconSpeed=0;return 0; }
    if((status->bannerFormat&3)==1) { status->offsetBanner=offset;offset+=3072;status->offsetBannerTlut=offset;offset+=512; }
    if((status->bannerFormat&3)==2) { status->offsetBanner=offset;offset+=6144; }
    for(unsigned i=0;i<8;i++) {
        unsigned format=(status->iconFormat>>(i*2))&3;
        if(format==1 || format==2) { status->offsetIcon[i]=offset;offset+=format==1?1024:2048;if(format==1) tlut=1; }
    }
    if(tlut) { status->offsetIconTlut=offset;offset+=512; }
    status->offsetData=offset;return 0;
}
s32 CARDSetStatus(s32 channel,s32 number,const CARDStat *status) {
    int result=access_save(channel,number);if(result) return result;
    if(!status) return CARD_RESULT_FATAL_ERROR;
    Save *save=&saves[number];unsigned char previous[HEADER];memcpy(previous,save->image,HEADER);
    save->image[7]=status->bannerFormat;write32(save->image+40,status->time);
    write32(save->image+44,status->iconAddr);write16(save->image+48,status->iconFormat);
    write16(save->image+50,status->iconSpeed);write32(save->image+60,status->commentAddr);
    if(!persist(save)) { loaded=0;memcpy(save->image,previous,HEADER);return CARD_RESULT_IOERROR; }
    return 0;
}
s32 CARDDelete(s32 channel,const char *name) {
    int result=ready(channel);if(result) return result;
    int number=find(name);if(number<0) return CARD_RESULT_NOFILE;
    Save *save=&saves[number];char live[640],kept[640],backup[640];
    path(live,sizeof(live),save->disk_name,"");path(kept,sizeof(kept),save->disk_name,".deleted");path(backup,sizeof(backup),save->disk_name,".bak");
    if(exists(kept) && remove(kept)) return CARD_RESULT_IOERROR;
    if(exists(backup) && remove(backup)) return CARD_RESULT_IOERROR;
    if(rename(live,kept)) return CARD_RESULT_IOERROR;
    used-=save->bytes;allocated-=HEADER+save->bytes;free(save->image);memset(save,0,sizeof(*save));return 0;
}
s32 CARDFormat(s32 channel) {
    int result=ready(channel);if(result) return result;
    for(unsigned i=0;i<CARD_MAX_FILE;i++) if(saves[i].image) {
        char name[33];memcpy(name,saves[i].image+8,32);name[32]=0;
        result=CARDDelete(channel,name);if(result) return result;
    }
    return 0;
}

/* Snapshot the exact retained card image used by this process. Called only
 * before first game input. No writes to the user's card and no mount changes. */
/* Before the game's first card mount, snapshot only the read-only on-disk
 * starting file (including the same missing-primary backup choice as mount).
 * This does not mount/recover/write the user's card or change cache state. */
static int snapshot_cold(const char *destination,unsigned *bytes,unsigned *crc){
    DIR *dir=opendir(directory);char chosen[640]={0};
    if(!dir&&errno!=ENOENT)return 0;
    if(dir){
        struct dirent *e;
        while((e=readdir(dir))){
            size_t n=strlen(e->d_name);int backup=n>8&&!strcmp(e->d_name+n-8,".gci.bak");
            if(!backup&&(n<5||strcmp(e->d_name+n-4,".gci")))continue;
            if(n>=96){closedir(dir);return 0;}
            char candidate[640];if(!path(candidate,sizeof(candidate),e->d_name,"")){closedir(dir);return 0;}
            if(backup){char primary[640];strcpy(primary,candidate);primary[strlen(primary)-4]=0;if(exists(primary))continue;}
            if(chosen[0]){closedir(dir);return 0;}strcpy(chosen,candidate);
        }
        closedir(dir);
    }
    unsigned char *image=NULL;unsigned length=0;int ok=1;
    if(chosen[0]){
        FILE*f=fopen(chosen,"rb");unsigned char header[64];if(!f)return 0;
        if(fread(header,1,64,f)!=64){fclose(f);return 0;}
        length=64+read16(header+56)*8192;
        if(length<8256||length>131136||memcmp(header,"GSA",3)||memcmp(header+4,"01",2)||
           (initialized&&(memcmp(header,game_id,4)||memcmp(header+4,maker_id,2)))){fclose(f);return 0;}
        image=malloc(length);if(!image){fclose(f);return 0;}memcpy(image,header,64);
        if(fread(image+64,1,length-64,f)!=length-64||fgetc(f)!=EOF||ferror(f))ok=0;
        if(fclose(f))ok=0;
    }
    if(ok){FILE*f=fopen(destination,"wb");if(!f)ok=0;
        else {if(length&&fwrite(image,1,length,f)!=length)ok=0;if(fflush(f))ok=0;if(fclose(f))ok=0;}}
    if(ok){*bytes=length;*crc=(unsigned)crc32(0,image,length);}free(image);return ok;
}
int fh_card_record_fixture(const char *destination,unsigned *bytes,unsigned *crc){
    if(!loaded)return snapshot_cold(destination,bytes,crc);
    if(!initialized)return 0;
    const Save *only=NULL;
    for(unsigned i=0;i<CARD_MAX_FILE;i++)if(saves[i].image){
        /* The current replay recipe represents one game card file. Reject
         * multiple/foreign files rather than silently choosing a wrong save. */
        if(only||memcmp(saves[i].image,game_id,4)||memcmp(saves[i].image+4,maker_id,2))return 0;
        only=saves+i;
    }
    unsigned length=only?HEADER+only->bytes:0;
    if(length>131136)return 0;
    FILE *f=fopen(destination,"wb");if(!f)return 0;
    int ok=!length||fwrite(only->image,1,length,f)==length;
    if(fflush(f))ok=0;if(fclose(f))ok=0;
    if(!ok)return 0;
    *bytes=length;*crc=(unsigned)crc32(0,only?only->image:NULL,length);return 1;
}
