#include "fh_install_actor_fog_width.h"
#include "fh_install_record.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
typedef struct {unsigned live,peak;} Workspace;
typedef union {max_align_t alignment;unsigned bytes;} Allocation;
static voidpf allocate(voidpf context,uInt items,uInt size){
    Workspace*w=context;const unsigned limit=65536;
    if(w->live>limit-sizeof(Allocation)||
       (items&&size>(limit-w->live-sizeof(Allocation))/items))return NULL;
    unsigned bytes=items*size+sizeof(Allocation);Allocation*p=calloc(1,bytes);if(!p)return NULL;
    p->bytes=bytes;w->live+=bytes;if(w->live>w->peak)w->peak=w->live;return p+1;
}
static void release(voidpf context,voidpf memory){
    if(memory){Workspace*w=context;Allocation*p=(Allocation*)memory-1;w->live-=p->bytes;free(p);}
}
int fh_install_actor_fog_width_begin(FhInstallActorFogWidths*out,FhInstallDisc*d,unsigned table,unsigned data){
    FhInstallEntry t,f;
    if(!out||!d||!d->file||!fh_install_disc_entry(d,table,&t)||!fh_install_disc_entry(d,data,&f)||
       t.directory||f.directory||!t.bytes||t.bytes%4||t.bytes>65536||f.bytes<16)return 0;
    *out=(FhInstallActorFogWidths){.disc=d,.table=table,.file=data,.entries=t.bytes/4};return 1;
}
static int prefix(FhInstallActorFogWidths*s,const FhInstallRecord*r,unsigned char out[96]){
    if(r->decoded<96)return 0;
    if(!r->compressed)return fh_install_disc_read(s->disc,s->file,r->offset+r->header,out,96);
    unsigned char input[1024];Workspace work={0};
    z_stream z={.zalloc=allocate,.zfree=release,.opaque=&work};
    if(inflateInit(&z)!=Z_OK)return 0;
    z.next_out=out;z.avail_out=96;unsigned at=0;int ok=1;
    while(z.avail_out){
        if(!z.avail_in){
            unsigned n=r->packed-at;if(n>sizeof(input))n=sizeof(input);
            if(!n||!fh_install_disc_read(s->disc,s->file,r->offset+r->header+at,input,n)){ok=0;break;}
            at+=n;z.next_in=input;z.avail_in=n;
        }
        uLong consumed=z.total_in,produced=z.total_out;
        int result=inflate(&z,Z_NO_FLUSH);
        if((result!=Z_OK&&result!=Z_STREAM_END)||(result==Z_STREAM_END&&z.avail_out)||
           (z.total_in==consumed&&z.total_out==produced)){ok=0;break;}
    }
    if(z.total_out!=96)ok=0;
    inflateEnd(&z);if(work.peak>s->workspace_peak)s->workspace_peak=work.peak;
    return ok&&!work.live;
}
int fh_install_actor_fog_width(void*context,uint32_t id,unsigned*out){
    FhInstallActorFogWidths*s=context;
    if(!s||!s->disc||!s->disc->file||!s->entries||!out||id>65535)return 0;
    unsigned index=id&0x7fff;if(index>=s->entries)index=0;
    s->requests++;
    for(unsigned i=0;i<64;i++)if(s->cached[i].index_plus_one==index+1){
        *out=s->cached[i].width;s->hits++;return 1;
    }
    unsigned char table[4];FhInstallEntry data;
    if(!fh_install_disc_read(s->disc,s->table,index*4,table,4)||!fh_install_disc_entry(s->disc,s->file,&data))return 0;
    unsigned value=word(table),levels=(value>>24)&63,offset=(value&0xffffff)*2,end=data.bytes;
    if(!levels||value==0xffffffffu||offset>data.bytes)return 0;
    if(levels>1){
        unsigned char offsets[64*4];unsigned bytes=(levels+1)*4;
        if(!fh_install_disc_read(s->disc,s->file,offset,offsets,bytes))return 0;
        unsigned previous=0;
        for(unsigned i=0;i<=levels;i++){
            unsigned delta=word(offsets+i*4);
            if(delta<bytes||(i&&delta<=previous)||delta>data.bytes-offset)return 0;
            previous=delta;
        }
        end=offset+word(offsets+4);offset+=word(offsets);
    }
    FhInstallRecord record;unsigned char header[96];
    if(!fh_install_record_info(s->disc,s->file,offset,&record)||
       record.header>end-offset||record.packed>end-offset-record.header||!prefix(s,&record,header))return 0;
    unsigned width=(unsigned)header[10]<<8|header[11],height=(unsigned)header[12]<<8|header[13];
    if(!width||!height||width>1024||height>1024)return 0;
    unsigned slot=s->cursor++%64;s->cached[slot].index_plus_one=index+1;s->cached[slot].width=width;
    *out=width;return 1;
}
