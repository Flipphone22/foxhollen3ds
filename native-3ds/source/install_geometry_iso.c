#include "fh_install_geometry_iso.h"
#include <stdlib.h>
#include <string.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int fail(FhInstallGeometryScan*s,const char*why){snprintf(s->error,sizeof(s->error),"%s",why);return 0;}
void fh_install_geometry_scan_close(FhInstallGeometryScan*s){if(s){free(s->table);free(s->decoded);s->table=s->decoded=NULL;s->disc=NULL;}}
int fh_install_geometry_scan_begin(FhInstallGeometryScan*s,FhInstallDisc*d,unsigned file,unsigned index){
    if(!s||!d||!d->file)return 0;fh_install_geometry_scan_close(s);memset(s,0,sizeof(*s));s->disc=d;
    for(unsigned i=1;i<d->entries;i++){
        FhInstallEntry e;char path[FH_INSTALL_PATH_CAP];if(!fh_install_disc_entry(d,i,&e)||!fh_install_disc_path(d,i,path,sizeof(path)))return fail(s,"Cannot enumerate source geometry paths.");
        if(e.directory)continue;
        char*name=strrchr(path,'/');name=name?name+1:path;size_t n=strlen(name),length=strlen(path);unsigned world=0;
        if(!strcmp(name,"MODELS.bin"))memcpy(path+length-3,"tab",3);
        else if(n>=11&&!memcmp(name,"mod",3)&&!strcmp(name+n-8,".zlb.bin")){world=1;memcpy(path+length-8,".tab",5);}
        else continue;
        int table=fh_install_disc_find(d,path);if(table<0){if(world)continue;return fail(s,"A source model bank has no index table.");}
        FhInstallEntry te;
        if(!fh_install_disc_entry(d,table,&te)||te.directory||te.bytes<(world?8u:4u)||te.bytes%4||te.bytes>FH_INSTALL_GEO_TABLE_CAP)return fail(s,"Invalid source geometry table extent.");
        if(s->banks_count==FH_INSTALL_GEO_BANKS)return fail(s,"Too many source geometry banks.");
        s->banks[s->banks_count++]=(FhInstallGeometryBank){i,(unsigned)table,world};
        unsigned entries=te.bytes/4-world;
        s->total_entries+=entries;
        if(i<file)s->scanned_entries+=entries;
    }
    if(!s->banks_count)return fail(s,"No indexed source geometry banks found.");
    if(file){
        while(s->bank<s->banks_count&&s->banks[s->bank].file<file)s->bank++;
        if(s->bank==s->banks_count||s->banks[s->bank].file!=file)return fail(s,"Unknown source geometry resume bank.");
        FhInstallEntry te;fh_install_disc_entry(d,s->banks[s->bank].table,&te);
        if(index>te.bytes/4-s->banks[s->bank].world)return fail(s,"Source geometry resume index exceeds its table.");
    }else if(index)return fail(s,"Invalid initial geometry cursor.");
    s->index=index;s->scanned_entries+=index;return 1;
}
static int load(FhInstallGeometryScan*s){
    FhInstallEntry e;if(!fh_install_disc_entry(s->disc,s->banks[s->bank].table,&e))return fail(s,"Cannot inspect source geometry table.");
    s->table=malloc(e.bytes);if(!s->table)return fail(s,"Cannot allocate source geometry table.");
    if(!fh_install_disc_read(s->disc,s->banks[s->bank].table,0,s->table,e.bytes))return fail(s,"Cannot read source geometry table.");
    s->entries=e.bytes/4-s->banks[s->bank].world;s->loaded=1;return 1;
}
int fh_install_geometry_scan_next(FhInstallGeometryScan*s,FhInstallGeometrySource*out){
    if(!s||!s->disc||!out||s->error[0])return FH_INSTALL_GEO_ERROR;
    free(s->decoded);s->decoded=NULL;memset(out,0,sizeof(*out));
    while(s->bank<s->banks_count){
        if(!s->loaded&&!load(s))return FH_INSTALL_GEO_ERROR;
        if(s->index<s->entries)break;
        free(s->table);s->table=NULL;s->bank++;s->index=s->entries=s->loaded=0;
    }
    if(s->bank==s->banks_count)return FH_INSTALL_GEO_DONE;
    const FhInstallGeometryBank*b=s->banks+s->bank;unsigned id=s->index++,entry=word(s->table+id*4);
    s->scanned_entries++;
    out->id=id|(b->world?0x80000000u:0);out->file=out->next_file=b->file;out->next_index=s->index;
    if(entry==0xffffffffu||(b->world?(!entry&&!word(s->table+(id+1)*4)):(!entry||!(entry&0x30000000)))){s->skipped++;return FH_INSTALL_GEO_SKIP;}
    unsigned offset=entry&(b->world?0xffffffu:0x0fffffffu);unsigned char marker[4];
    if(!fh_install_disc_read(s->disc,b->file,offset,marker,4)){fail(s,"Source geometry offset exceeds its bank.");return FH_INSTALL_GEO_ERROR;}
    if(!b->world){
        if(word(marker)!=0xfacefeed){s->unsupported++;return FH_INSTALL_GEO_SKIP;}
        offset+=0x24;
        if(!fh_install_disc_read(s->disc,b->file,offset,marker,4)){fail(s,"Source model wrapper is truncated.");return FH_INSTALL_GEO_ERROR;}
    }
    if(memcmp(marker,"ZLB\0",4)){s->unsupported++;return FH_INSTALL_GEO_SKIP;}
    FhInstallRecord record;
    if(!fh_install_record_info(s->disc,b->file,offset,&record)){fail(s,"Invalid source geometry compression record.");return FH_INSTALL_GEO_ERROR;}
    if(record.decoded<(b->world?164u:256u)||record.decoded>(b->world?0x32000u:FH_INSTALL_RECORD_CAP)){s->unsupported++;return FH_INSTALL_GEO_SKIP;}
    s->decoded=malloc(record.decoded);if(!s->decoded){fail(s,"Cannot allocate source geometry record.");return FH_INSTALL_GEO_ERROR;}
    if(!fh_install_record_decode(s->disc,b->file,&record,s->decoded,record.decoded)){fail(s,"Source geometry decompression failed.");return FH_INSTALL_GEO_ERROR;}
    out->offset=offset;out->bytes=record.decoded;out->data=s->decoded;s->records++;return FH_INSTALL_GEO_SOURCE;
}
