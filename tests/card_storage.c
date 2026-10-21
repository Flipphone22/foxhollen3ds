#include <dolphin/card.h>
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fh_platform.h"
#include "fh_input_tape.h"
#include <zlib.h>
void fh_platform_panic(const char *message) { fprintf(stderr,"PANIC %s\n",message);abort(); }
void fh_platform_log(const char *message) { fputs(message,stdout); }
static unsigned filesystem_calls;
FILE *__real_fopen(const char*,const char*);
FILE *__wrap_fopen(const char *p,const char *m){filesystem_calls++;return __real_fopen(p,m);}
DIR *__real_opendir(const char*);
DIR *__wrap_opendir(const char *p){filesystem_calls++;return __real_opendir(p);}
int __real_mkdir(const char*,mode_t);
int __wrap_mkdir(const char *p,mode_t m){filesystem_calls++;return __real_mkdir(p,m);}
struct dirent *__real_readdir(DIR*);
struct dirent *__wrap_readdir(DIR *d){filesystem_calls++;return __real_readdir(d);}
int __real_closedir(DIR*);
int __wrap_closedir(DIR *d){filesystem_calls++;return __real_closedir(d);}
int main(int argc,char **argv) {
    assert(argc==2);assert(fh_card_set_directory(argv[1]));
    CARDInit("GSAE","01");
    int size,sector;assert(CARDProbeEx(0,&size,&sector)==0 && size==16 && sector==8192);
    assert(CARDProbeEx(1,NULL,NULL)==CARD_RESULT_NOCARD);
    assert(CARDMount(0,NULL,NULL)==0);
    char fixture[512];snprintf(fixture,sizeof(fixture),"%s/record-fixture.bin",argv[1]);
    unsigned recorded_bytes=99,recorded_crc=99;
    assert(fh_card_record_fixture(fixture,&recorded_bytes,&recorded_crc));
    assert(recorded_bytes==0&&recorded_crc==0);assert(!unlink(fixture));

    CARDFileInfo info;assert(CARDOpen(0,"Star Fox Adventures",&info)==CARD_RESULT_NOFILE);
    assert(CARDCreate(0,"Star Fox Adventures",16384,&info)==0);
    assert(CARDCreate(0,"Star Fox Adventures",16384,&info)==CARD_RESULT_EXIST);
    unsigned char data[8192],readback[8192];
    for(unsigned i=0;i<sizeof(data);i++) data[i]=(i*137+17)&255;
    assert(CARDWrite(&info,data,8192,8192)==0);
    assert(fh_card_record_fixture(fixture,&recorded_bytes,&recorded_crc));
    assert(recorded_bytes==16448);
    FILE *recorded=fopen(fixture,"rb");assert(recorded);unsigned char image[16448];
    assert(fread(image,1,sizeof(image),recorded)==sizeof(image)&&fgetc(recorded)==EOF);fclose(recorded);
    assert(recorded_crc==(unsigned)crc32(0,image,sizeof(image)));
    assert(!memcmp(image+64+8192,data,8192));assert(!unlink(fixture));

    assert(CARDRead(&info,readback,8192,8192)==0 && !memcmp(data,readback,8192));
    assert(CARDWrite(&info,data,8192,8193)==CARD_RESULT_LIMIT);
    assert(CARDRead(&info,readback,-1,0)==CARD_RESULT_FATAL_ERROR);
    assert(CARDRead(&info,readback,1,-1)==CARD_RESULT_FATAL_ERROR);
    CARDStat status;assert(CARDGetStatus(0,info.fileNo,&status)==0);
    assert(status.length==16384 && !memcmp(status.gameName,"GSAE",4));
    status.bannerFormat=2;status.iconAddr=0;status.iconFormat=1;status.iconSpeed=1;status.commentAddr=8000;status.time=12345;
    assert(CARDSetStatus(0,info.fileNo,&status)==0);
    assert(CARDClose(&info)==0);
    assert(CARDRead(&info,readback,1,0)==CARD_RESULT_NOFILE);
    unsigned before=filesystem_calls;
    assert(CARDUnmount(0)==0);assert(CARDMount(0,NULL,NULL)==0);
    assert(filesystem_calls==before);
    FhCardStats stats;fh_card_stats(&stats);
    assert(stats.cold_mounts==1 && stats.warm_mounts==1 && stats.bytes==16448);
    assert(CARDOpen(0,"Star Fox Adventures",&info)==0);
    assert(CARDRead(&info,readback,8192,8192)==0 && !memcmp(data,readback,8192));
    assert(CARDGetStatus(0,info.fileNo,&status)==0);
    assert(status.time==12345 && status.commentAddr==8000 && status.offsetBanner==0 && status.offsetIcon[0]==6144 && status.offsetIconTlut==7168 && status.offsetData==7680);
    before=filesystem_calls;
    for(unsigned i=0;i<100;i++){
        assert(CARDUnmount(0)==0 && CARDMount(0,NULL,NULL)==0);
        assert(CARDOpen(0,"Star Fox Adventures",&info)==0);
        assert(CARDRead(&info,readback,8192,8192)==0 && !memcmp(data,readback,8192));
    }
    assert(filesystem_calls==before);
    DIR *dir=opendir(argv[1]);assert(dir);struct dirent *entry;char base[1024]={0};
    while((entry=readdir(dir))) { size_t n=strlen(entry->d_name);if(n>4 && !strcmp(entry->d_name+n-4,".gci")) snprintf(base,sizeof(base),"%s/%s",argv[1],entry->d_name); }
    closedir(dir);assert(*base);
    FILE *file=fopen(base,"rb");assert(file);unsigned char header[64];assert(fread(header,1,64,file)==64);fclose(file);
    assert(!memcmp(header,"GSAE01",6) && header[56]==0 && header[57]==2 && header[40]==0 && header[42]==0x30 && header[43]==0x39);
    char temporary[1040],backup[1040];snprintf(temporary,sizeof(temporary),"%s.tmp",base);snprintf(backup,sizeof(backup),"%s.bak",base);
    assert(!mkdir(temporary,0700));memset(readback,0xcc,sizeof(readback));
    assert(CARDWrite(&info,readback,8192,8192)==CARD_RESULT_IOERROR);
    assert(CARDRead(&info,readback,8192,8192)==0 && !memcmp(data,readback,8192));
    assert(!rmdir(temporary));assert(CARDUnmount(0)==0);
    fh_card_stats(&stats);assert(!stats.loaded && !stats.bytes);
    assert(!remove(backup));assert(!rename(base,backup));
    assert(CARDMount(0,NULL,NULL)==0 && !access(base,F_OK));
    assert(CARDOpen(0,"Star Fox Adventures",&info)==0);
    assert(CARDRead(&info,readback,8192,8192)==0 && !memcmp(data,readback,8192));
    assert(CARDDelete(0,"Star Fox Adventures")==0);
    assert(CARDUnmount(0)==0 && CARDMount(0,NULL,NULL)==0);
    assert(CARDOpen(0,"Star Fox Adventures",&info)==CARD_RESULT_NOFILE);
    assert(CARDCreate(0,"../escaped/name",8192,&info)==0);
    assert(CARDOpen(0,"../escaped/name",&info)==0);
    assert(CARDFormat(0)==0);
    assert(CARDOpen(0,"../escaped/name",&info)==CARD_RESULT_NOFILE);
    /* Retention is bounded even for other imported virtual card files. */
    assert(CARDCreate(0,"large",17*8192,&info)==0);
    assert(CARDUnmount(0)==0);fh_card_stats(&stats);assert(!stats.bytes && !stats.loaded);
    assert(CARDMount(0,NULL,NULL)==0 && CARDOpen(0,"large",&info)==0);
    assert(CARDFormat(0)==0);
    fh_card_shutdown();CARDInit("GSAE","01");
    assert(CARDMount(0,NULL,NULL)==0);fh_card_stats(&stats);
    assert(stats.cold_mounts==1 && stats.warm_mounts==0 && !stats.bytes);
    fh_card_shutdown();
    puts("PASS: GCI bytes, bounds, metadata, cold remount, failed write rollback, backup recovery, deletion and format");
    return 0;
}
