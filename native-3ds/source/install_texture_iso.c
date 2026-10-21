#include "fh_install_texture_iso.h"
#include <stdlib.h>
#include <string.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int fail(FhInstallTextureScan*s,const char*text){snprintf(s->error,sizeof(s->error),"%s",text);return 0;}
void fh_install_texture_scan_close(FhInstallTextureScan*s){if(s){free(s->decoded);s->decoded=NULL;s->disc=NULL;}}
int fh_install_texture_scan_begin(FhInstallTextureScan*s,FhInstallDisc*d,unsigned file,unsigned offset){
    if(!s||!d||!d->file)return 0;fh_install_texture_scan_close(s);memset(s,0,sizeof(*s));s->disc=d;
    for(unsigned i=1;i<d->entries;i++){
        FhInstallEntry e;char path[FH_INSTALL_PATH_CAP];
        if(!fh_install_disc_entry(d,i,&e)||!fh_install_disc_path(d,i,path,sizeof(path)))return fail(s,"Cannot enumerate ISO texture paths.");
        if(e.directory)continue;char*name=strrchr(path,'/');name=name?name+1:path;size_t n=strlen(name),length=strlen(path);
        if(n<8||memcmp(name,"TEX",3)||strcmp(name+n-4,".bin"))continue;
        memcpy(path+length-3,"tab",3);int table=fh_install_disc_find(d,path);if(table<0)continue;
        FhInstallEntry te;if(!fh_install_disc_entry(d,(unsigned)table,&te)||te.directory||!te.bytes||te.bytes%4||te.bytes>FH_INSTALL_TEX_TABLE_BYTES)return fail(s,"Invalid texture table size.");
        if(s->banks_count==FH_INSTALL_TEX_BANKS)return fail(s,"Too many ISO texture banks.");
        s->banks[s->banks_count++]=(FhInstallTextureBank){i,(unsigned)table,e.bytes};
    }
    if(!s->banks_count)return fail(s,"No source texture banks were found.");
    if(file){
        while(s->bank<s->banks_count&&s->banks[s->bank].file<file)s->bank++;
        if(s->bank==s->banks_count||s->banks[s->bank].file!=file||offset>s->banks[s->bank].bytes)return fail(s,"Invalid ISO texture resume cursor.");
    }else if(offset)return fail(s,"Invalid initial ISO texture cursor.");
    s->resume_offset=offset;return 1;
}
static int add_offset(FhInstallTextureScan*s,unsigned offset){
    unsigned bytes=s->banks[s->bank].bytes;if(offset>bytes||16>bytes-offset)return fail(s,"Texture table points outside its bank.");
    unsigned lo=0,hi=s->count;
    while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(s->offsets[mid]<offset)lo=mid+1;else hi=mid;}
    if(lo<s->count&&s->offsets[lo]==offset)return 1;
    if(s->count==FH_INSTALL_TEX_OFFSETS)return fail(s,"Too many source records in a texture bank.");
    memmove(s->offsets+lo+1,s->offsets+lo,(s->count-lo)*sizeof(unsigned));s->offsets[lo]=offset;s->count++;return 1;
}
static int load_bank(FhInstallTextureScan*s){
    FhInstallTextureBank*b=&s->banks[s->bank];FhInstallEntry e;
    if(!fh_install_disc_entry(s->disc,b->table,&e))return fail(s,"Cannot read texture table metadata.");
    unsigned char*table=malloc(e.bytes);if(!table)return fail(s,"Not enough startup memory for texture table.");
    int ok=fh_install_disc_read(s->disc,b->table,0,table,e.bytes);s->count=s->index=0;
    for(unsigned at=0;ok&&at<e.bytes;at+=4){
        unsigned value=word(table+at);if(value==0xffffffffu)continue;
        unsigned frames=(value>>24)&63,offset=(value&0xffffff)*2;
        if(frames==1)ok=add_offset(s,offset);
        else if(frames>1){
            unsigned char indices[64*4];unsigned bytes=(frames+1)*4;
            if(offset>b->bytes||bytes>b->bytes-offset||!fh_install_disc_read(s->disc,b->file,offset,indices,bytes)){ok=fail(s,"Cannot read texture frame table.");break;}
            unsigned previous=0;
            for(unsigned j=0;ok&&j<=frames;j++){
                unsigned delta=word(indices+j*4);
                if(delta<bytes||(j&&delta<=previous)||delta>b->bytes-offset){ok=fail(s,"Invalid source texture frame extent.");break;}
                previous=delta;if(j<frames)ok=add_offset(s,offset+delta);
            }
        }
    }
    free(table);if(!ok){if(!s->error[0])fail(s,"Cannot read ISO texture table.");return 0;}
    while(s->index<s->count&&s->offsets[s->index]<s->resume_offset)s->index++;
    if(s->resume_offset){
        /* Resume can be exactly a record start or the preceding record end.
         * It must not silently skip an arbitrary interior source offset. */
        unsigned valid=s->index<s->count&&s->offsets[s->index]==s->resume_offset;
        if(!valid&&s->index){FhInstallRecord prior;if(fh_install_record_info(s->disc,b->file,s->offsets[s->index-1],&prior))valid=prior.offset+prior.header+prior.packed==s->resume_offset;}
        if(!valid)return fail(s,"Texture cursor is not a record boundary.");
    }
    s->resume_offset=0;s->loaded=1;return 1;
}
int fh_install_texture_scan_record(FhInstallTextureScan*s,FhInstallTextureRecord*out){
    if(!s||!s->disc||!out||s->error[0])return FH_INSTALL_TEX_ERROR;
    free(s->decoded);s->decoded=NULL;memset(out,0,sizeof(*out));
    while(s->bank<s->banks_count){
        if(!s->loaded&&!load_bank(s))return FH_INSTALL_TEX_ERROR;
        if(s->index<s->count)break;
        s->bank++;s->loaded=0;
    }
    if(s->bank==s->banks_count)return FH_INSTALL_TEX_DONE;
    FhInstallTextureBank*b=&s->banks[s->bank];unsigned offset=s->offsets[s->index++];FhInstallRecord record;
    if(!fh_install_record_info(s->disc,b->file,offset,&record)){fail(s,"Invalid original texture compression record.");return FH_INSTALL_TEX_ERROR;}
    unsigned end=offset+record.header+record.packed;
    if(s->index<s->count&&end>s->offsets[s->index]){fail(s,"Overlapping original texture records.");return FH_INSTALL_TEX_ERROR;}
    s->decoded=malloc(record.decoded);if(!s->decoded){fail(s,"Not enough startup memory for texture record.");return FH_INSTALL_TEX_ERROR;}
    if(!fh_install_record_decode(s->disc,b->file,&record,s->decoded,record.decoded)){fail(s,"Original texture decompression failed.");return FH_INSTALL_TEX_ERROR;}
    out->file=out->next_file=b->file;out->offset=offset;out->next_offset=end;s->decoded_records++;
    out->bytes=record.decoded;out->data=s->decoded;return FH_INSTALL_TEX_IMAGE;
}
int fh_install_texture_scan_next(FhInstallTextureScan*s,FhInstallTextureImage*out){
    if(!out)return FH_INSTALL_TEX_ERROR;
    memset(out,0,sizeof(*out));FhInstallTextureRecord record;
    int result=fh_install_texture_scan_record(s,&record);if(result!=FH_INSTALL_TEX_IMAGE)return result;
    out->file=record.file;out->offset=record.offset;out->next_file=record.next_file;out->next_offset=record.next_offset;
    const unsigned char*raw=s->decoded;
    /* TEXPRE also contains short stored preload wrappers (including nested
     * compressed payloads), not direct 96-byte texture headers. The existing
     * host recipe excludes these; count them explicitly before field reads. */
    if(record.bytes<96){s->skipped_records++;s->short_records++;return FH_INSTALL_TEX_SKIP;}
    unsigned w=(unsigned)raw[10]<<8|raw[11],h=(unsigned)raw[12]<<8|raw[13];
    if(raw[22]!=14||w<8||h<8||w>1024||h>1024||(w&(w-1))||(h&(h-1))){s->skipped_records++;return FH_INSTALL_TEX_SKIP;}
    unsigned bytes=w*h/2,mask=word(raw+80);
    if(bytes>record.bytes-96||(mask&&(mask<bytes||mask>record.bytes-96||bytes>record.bytes-96-mask))){fail(s,"Original texture colour or opacity extent is invalid.");return FH_INSTALL_TEX_ERROR;}
    out->width=w;out->height=h;out->bytes=bytes;out->pixels=raw+96;s->eligible_records++;return FH_INSTALL_TEX_IMAGE;
}
typedef struct {FhInstallTextureStageProgress function;void*context;} AssemblyProgress;
static int assemble_progress(void*context,uint32_t done,uint32_t total){AssemblyProgress*p=context;return !p->function||p->function(p->context,FH_INSTALL_TEX_ASSEMBLE,done,total);}
int fh_install_texture_prepare(FhInstallTransaction*t,FhInstallDisc*d,unsigned spool_id,unsigned output,unsigned edge,FhInstallTextureStageProgress progress,void*context){
    if(!t||t->failed||!d||!d->file||spool_id>=FH_INSTALL_ARTIFACTS||output>=FH_INSTALL_ARTIFACTS||spool_id==output)return 0;
    FhInstallTextureScan*scan=NULL;FhInstallTextureSpool spool={0};int result=0;
    if(!(t->state.complete&(1u<<spool_id))){
        if(!fh_install_texture_spool_begin(&spool,t,spool_id))goto done;
        scan=calloc(1,sizeof(*scan));if(!scan){snprintf(t->error,sizeof(t->error),"Not enough startup memory for texture scanner.");t->failed=1;goto done;}
        if(!fh_install_texture_scan_begin(scan,d,t->state.cursor[0],t->state.cursor[1]))goto scan_error;
        for(;;){
            if(progress&&!progress(context,FH_INSTALL_TEX_SCAN,scan->bank,scan->banks_count)){result=-1;goto done;}
            FhInstallTextureImage image;int next=fh_install_texture_scan_next(scan,&image);
            if(next==FH_INSTALL_TEX_ERROR)goto scan_error;
            if(next==FH_INSTALL_TEX_DONE)break;
            if(next==FH_INSTALL_TEX_SKIP)continue;
            result=fh_install_texture_spool_add(&spool,image.pixels,image.bytes,image.width,image.height,edge,image.next_file,image.next_offset,progress,context);
            if(result!=1)goto done;result=0;
        }
        if(progress&&!progress(context,FH_INSTALL_TEX_SCAN,scan->banks_count,scan->banks_count)){result=-1;goto done;}
        if(!fh_install_texture_spool_finish(&spool))goto done;
        fh_install_texture_scan_close(scan);free(scan);scan=NULL;
    }
    {AssemblyProgress tracking={progress,context};result=fh_install_texture_pack(t,spool_id,output,assemble_progress,&tracking);}
    goto done;
scan_error:
    snprintf(t->error,sizeof(t->error),"%s",scan->error);t->failed=1;
done:
    if(scan){fh_install_texture_scan_close(scan);free(scan);}fh_install_texture_spool_close(&spool);
    if(result<=0)fh_install_transaction_close(t);return result;
}
