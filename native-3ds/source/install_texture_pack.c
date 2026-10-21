#include "fh_install_texture_search.h"
#include "fh_install_texture_padded.h"
#include "fh_texture_pack_padded.h"
/* Startup-only texture spool and FHTXPK2/typed FHTXPK3 publication. */
#include "fh_install_texture_pack.h"
#include "fh_install_assembly.h"
#include "fh_texture_pack.h"
#include "fh_install_texture_exact.h"
#include "fh_texture_pack_layout.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

enum {HEADER=96,STAGE=16384,FILE_LIMIT=128*1024*1024,PACK_CURSOR=0x54585032,MIP_LIMIT=16384};
typedef struct {unsigned at,w,h,source,crc,format,full,full_crc,gw,gh,small,small_crc,edge,source_format;} Entry;
typedef struct {Entry entry;unsigned reduced;} Output;
typedef struct {FhInstallTextureStageProgress function;void*context;unsigned phase;} Progress;
static int tile_progress(void*context,uint32_t done,uint32_t total){Progress*p=context;return !p->function||p->function(p->context,p->phase,done,total);}
static uint32_t get(const unsigned char*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(unsigned char*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(24-8*i));}
static int extent(unsigned n){return n>=8&&n<=1024&&!(n&(n-1));}
static int fail(FhInstallTransaction*t,const char*text){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",text);return 0;}
static int acceptable(const FhInstallTextureResult*r){return r->squared_error<=(uint64_t)196*r->visible_components;}
static unsigned record_bytes(const Entry*e){return HEADER+e->source+e->full+e->small;}
static int decode(const unsigned char*h,Entry*e,unsigned at){
    unsigned v2=!memcmp(h,"FHTXSPL2",8),v3=!memcmp(h,"FHTXSPL3",8),v4=!memcmp(h,"FHTXSPL4",8);
    if((!v2&&!v3&&!v4&&memcmp(h,"FHTXSPL1",8))||get(h+92)!=(uint32_t)crc32(0,h,92))return 0;
    memset(e,0,sizeof(*e));e->at=at;e->w=get(h+8);e->h=get(h+12);e->source=get(h+16);e->crc=get(h+20);
    e->format=get(h+24);e->full=get(h+28);e->full_crc=get(h+32);e->gw=get(h+36);e->gh=get(h+40);e->small=get(h+44);e->small_crc=get(h+48);
    e->edge=get(h+76);if(e->edge&&(!extent(e->edge)||e->edge>512))return 0;
    e->source_format=(v3||v4)?get(h+80):14;
    if(v4){
        uint64_t error=(uint64_t)get(h+60)<<32|get(h+56);
        unsigned components=get(h+52);
        if(e->source_format!=14||(extent(e->w)&&extent(e->h))||
           (e->format!=0&&e->format!=12&&e->format!=13)||
           !fh_pack_entry_valid_padded(e->w,e->h,e->source_format,e->source,e->format,
               fh_pack_extent(e->w),fh_pack_extent(e->h),e->full))return 0;
        if(components>3*e->w*e->h||components%3||error>(uint64_t)65025*components)return 0;
        if(e->format&&error>(uint64_t)196*components)return 0;
        if(!e->format&&error<=(uint64_t)196*components)return 0;
        if(e->format==12&&components!=3*e->w*e->h)return 0;
        /* Visible-only quality metric; no reduced image or reserved payload. */
        for(unsigned i=36;i<52;i++)if(h[i])return 0;
        for(unsigned i=64;i<76;i++)if(h[i])return 0;
        for(unsigned i=84;i<92;i++)if(h[i])return 0;
        return 1;
    }
    if(v3){
        unsigned needed=fh_pack_source_bytes(e->source_format,e->w,e->h);
        if(!needed||e->source!=needed||e->format!=fh_pack_exact_format(e->source_format)||
           !fh_pack_entry_valid(e->w,e->h,e->source_format,e->source,e->format,fh_pack_extent(e->w),fh_pack_extent(e->h),e->full))return 0;
        /* v3 stores exact images: no lossy metric, reduced image or hidden tail. */
        for(unsigned i=36;i<76;i++)if(h[i])return 0;
        for(unsigned i=84;i<92;i++)if(h[i])return 0;
        return 1;
    }
    if(!extent(e->w)||!extent(e->h)||e->source!=e->w*e->h/2||
       (e->format!=0&&e->format!=12&&e->format!=13))return 0;
    uint64_t error=(uint64_t)get(h+60)<<32|get(h+56),small_error=(uint64_t)get(h+72)<<32|get(h+68);
    unsigned components=get(h+52),small_components=get(h+64);
    if(components>3*e->w*e->h||components%3||small_components%3)return 0;
    if(e->full){
        unsigned expected=e->format?e->w*e->h/(e->format==12?2:1):e->w*e->h*4;
        if(e->full!=expected)return 0;
        if(e->format&&error>(uint64_t)196*components)return 0;
        if(!e->format&&(!v2||error<=(uint64_t)196*components))return 0;
        if(e->format==12&&components!=3*e->w*e->h)return 0;
    }else if(v2||e->format||e->full_crc||error<=(uint64_t)196*components)return 0;
    if(e->small){
        if(e->format!=12||!extent(e->gw)||!extent(e->gh)||e->gw>e->w||e->gh>e->h||
           !e->edge||e->gw!=(e->w>e->edge?e->edge:e->w)||e->gh!=(e->h>e->edge?e->edge:e->h)||
           (e->gw==e->w&&e->gh==e->h)||e->small!=e->gw*e->gh/2||small_components!=3*e->gw*e->gh||small_error>(uint64_t)196*small_components)return 0;
    }else if(e->gw||e->gh||e->small_crc)return 0;
    for(unsigned i=80;i<92;i++)if(h[i])return 0;
    return 1;
}

typedef struct {unsigned at,w,h,format,bytes,crc,parent_crc,parent_w,parent_h,level,source_format;} Mip;
typedef struct {unsigned crc,w,h,source_format,format,levels,first,offset,bytes,chain_crc;} Chain;
static int decode_mip(const unsigned char*h,Mip*m,unsigned at){
    if(memcmp(h,"FHTXSPL5",8)||get(h+92)!=(uint32_t)crc32(0,h,92))return 0;
    memset(m,0,sizeof(*m));m->at=at;m->w=get(h+8);m->h=get(h+12);m->format=get(h+24);m->bytes=get(h+28);m->crc=get(h+32);
    m->parent_crc=get(h+36);m->parent_w=get(h+40);m->parent_h=get(h+44);m->level=get(h+48);m->source_format=get(h+80);
    if(!m->level||m->level>9||!extent(m->parent_w)||!extent(m->parent_h)||m->w!=m->parent_w>>m->level||m->h!=m->parent_h>>m->level||
       m->w<8||m->h<8||!fh_pack_source_bytes(m->source_format,m->parent_w,m->parent_h)||
       !m->bytes||m->bytes!=fh_pack_native_bytes(m->format,m->w,m->h))return 0;
    for(unsigned i=16;i<24;i++)if(h[i])return 0;
    for(unsigned i=52;i<80;i++)if(h[i])return 0;
    for(unsigned i=84;i<92;i++)if(h[i])return 0;
    return 1;
}
static int mip_push(Mip**mips,unsigned*count,unsigned*capacity,const Mip*m){
    if(*count==MIP_LIMIT)return 0;
    if(mips){
        if(*count==*capacity){
            unsigned next=*capacity?*capacity*2:256;if(next>MIP_LIMIT)next=MIP_LIMIT;
            Mip*grown=realloc(*mips,next*sizeof(Mip));if(!grown)return 0;*mips=grown;*capacity=next;
        }
        (*mips)[*count]=*m;
    }
    (*count)++;return 1;
}
static int checksum(FILE*f,unsigned bytes,unsigned expected){
    unsigned char b[STAGE];unsigned crc=0;
    while(bytes){unsigned n=bytes>sizeof(b)?sizeof(b):bytes;if(fread(b,1,n,f)!=n)return 0;crc=(unsigned)crc32(crc,b,n);bytes-=n;}
    return crc==expected;
}
static int spool_read(FILE*f,Entry*entries,unsigned*count,unsigned verify,Mip**mips,unsigned*mip_count,unsigned*mip_capacity){
    unsigned at=0,n=0,outputs=0,edge=0,mn=0,mc=mip_capacity?*mip_capacity:0;unsigned char h[HEADER];
    if(fseek(f,0,SEEK_END))return 0;long length=ftell(f);if(length<0||length>FILE_LIMIT||fseek(f,0,SEEK_SET))return 0;
    while(at<(unsigned)length){
        if(HEADER>(unsigned)length-at||fread(h,1,HEADER,f)!=HEADER)return 0;
        if(!memcmp(h,"FHTXSPL5",8)){
            Mip m;if(!decode_mip(h,&m,at)||m.bytes>(unsigned)length-at-HEADER)return 0;
            if(verify){if(!checksum(f,m.bytes,m.crc))return 0;}
            else if(fseek(f,m.bytes,SEEK_CUR))return 0;
            if(!mip_push(mips,&mn,&mc,&m))return 0;
            at+=HEADER+m.bytes;continue;
        }
        Entry e;if(n==FH_INSTALL_TEXTURE_LIMIT||!decode(h,&e,at)||record_bytes(&e)>(unsigned)length-at)return 0;
        outputs+=(e.full!=0)+(e.small!=0);if(outputs>FH_INSTALL_TEXTURE_LIMIT)return 0;
        if(verify){if(!checksum(f,e.source,e.crc)||!checksum(f,e.full,e.full_crc)||!checksum(f,e.small,e.small_crc))return 0;}
        else if(fseek(f,e.source+e.full+e.small,SEEK_CUR))return 0;
        if(n&&e.edge!=edge)return 0;edge=e.edge;
        if(entries)entries[n]=e;n++;at+=record_bytes(&e);
    }
    *count=n;if(mip_count)*mip_count=mn;if(mip_capacity)*mip_capacity=mc;return !ferror(f);
}
static int spool_validate(const char*path,void*context){
    (void)context;FILE*f=fopen(path,"rb");if(!f)return 0;unsigned n;int ok=spool_read(f,NULL,&n,1,NULL,NULL,NULL);if(fclose(f))ok=0;return ok;
}
void fh_install_texture_spool_close(FhInstallTextureSpool*s){if(s){free(s->entries);free(s->mips);memset(s,0,sizeof(*s));}}
int fh_install_texture_spool_begin(FhInstallTextureSpool*s,FhInstallTransaction*t,unsigned artifact){
    if(!s||!t||s->entries||t->failed)return 0;
    if(!fh_install_artifact_begin(t,artifact))return 0;
    Entry*entries=calloc(FH_INSTALL_TEXTURE_LIMIT,sizeof(*entries));if(!entries)return fail(t,"Not enough startup memory for texture directory.");
    unsigned count=0,mip_count=0,mip_capacity=0;Mip*mips=NULL;
    if(!spool_read(t->output,entries,&count,0,&mips,&mip_count,&mip_capacity)||fseek(t->output,t->written,SEEK_SET)){free(entries);free(mips);return fail(t,"Invalid texture conversion checkpoint.");}
    s->entries=entries;s->transaction=t;s->count=count;s->mips=mips;s->mip_count=s->mip_admitted=mip_count;s->mip_capacity=mip_capacity;
    for(unsigned i=0;i<count;i++){s->admitted+=entries[i].full!=0;s->reduced+=entries[i].small!=0;s->rejected+=entries[i].full==0;s->uncompressed+=entries[i].full&&entries[i].format<12;}
    fh_install_texture_init();return 1;
}
static int same(FhInstallTransaction*t,const Entry*e,const void*source){
    unsigned char chunk[STAGE];
    for(unsigned at=0;at<e->source;){unsigned n=e->source-at;if(n>sizeof(chunk))n=sizeof(chunk);
        if(!fh_install_artifact_read(t,e->at+HEADER+at,chunk,n))return -1;
        if(memcmp(chunk,(const unsigned char*)source+at,n))return 0;at+=n;
    }
    return 1;
}
int fh_install_texture_spool_flush(FhInstallTextureSpool*s){
    if(!s||!s->entries||!s->transaction)return 0;
    if(!s->duplicate_pending)return !s->transaction->failed;
    if(!fh_install_artifact_checkpoint(s->transaction,s->duplicate_entry,s->duplicate_offset))return 0;
    s->duplicate_pending=0;return 1;
}
static int duplicate_cursor(FhInstallTextureSpool*s,uint32_t entry,uint32_t offset){
    s->duplicate_entry=entry;s->duplicate_offset=offset;s->duplicate_pending++;
    if(!s->duplicate_batch||s->duplicate_pending>=s->duplicate_batch||s->transaction->state.cursor[0]!=entry)
        return fh_install_texture_spool_flush(s);
    return 1;
}
/* Share the bounded pending cursor with duplicate records. The source
 * cursor remains replayable when colour/opacity are separate image records. */
static int payload_cursor(FhInstallTextureSpool*s,uint32_t entry,uint32_t offset){
    FhInstallTransaction*t=s->transaction;
    if(t->written<t->state.partial_bytes)return fail(t,"Texture checkpoint extent regressed.");
    s->duplicate_entry=entry;s->duplicate_offset=offset;s->duplicate_pending++;
    if(!s->payload_batch_bytes||!s->duplicate_batch||s->duplicate_pending>=s->duplicate_batch||
       t->written-t->state.partial_bytes>=s->payload_batch_bytes||t->state.cursor[0]!=entry)
        return fh_install_texture_spool_flush(s);
    return 1;
}
int fh_install_texture_spool_add(FhInstallTextureSpool*s,const void*source,unsigned bytes,unsigned w,unsigned h,unsigned edge,
                                uint32_t next_entry,uint32_t next_offset,FhInstallTextureStageProgress progress,void*context){
    if(!s||!s->entries||!s->transaction)return 0;FhInstallTransaction*t=s->transaction;
    if(t->failed||!t->output)return 0;
    if(!source||!extent(w)||!extent(h)||bytes!=w*h/2||(edge&&(!extent(edge)||edge>512)))return fail(t,"Invalid source texture or reduction recipe.");
    unsigned crc=(unsigned)crc32(0,source,bytes);Entry*entries=s->entries;
    if(s->count&&entries[0].edge!=edge)return fail(t,"Texture reduction recipe changed during conversion.");
    for(unsigned i=0;i<s->count;i++)if(entries[i].crc==crc&&entries[i].w==w&&entries[i].h==h&&entries[i].source_format==14){
        int equal=same(t,entries+i,source);if(equal<0)return 0;
        if(equal){if(!duplicate_cursor(s,next_entry,next_offset))return 0;s->duplicates++;return 1;}
    }
    if(s->count==FH_INSTALL_TEXTURE_LIMIT)return fail(t,"Too many unique source textures.");
    unsigned char*gpu=malloc(w*h);if(!gpu)return fail(t,"Not enough startup memory to encode texture.");
    unsigned char*small=NULL;FhInstallTextureResult full={0},reduced={0};
    Progress tracking={progress,context,FH_INSTALL_TEXTURE_FULL};
    int result=fh_install_texture_search_cmpr(source,bytes,w,h,w,h,gpu,w*h,&full,tile_progress,&tracking);
    if(result!=1)goto done;
    unsigned accept=acceptable(&full),gw=0,gh=0;
    unsigned exact=0,output_format=full.format,output_bytes=full.bytes;
    if(!accept){
        free(gpu);gpu=malloc(w*h*4);
        if(!gpu){result=fail(t,"Not enough startup memory for exact texture.");goto done;}
        FhInstallExactTexture original={source,NULL,w,h,14,0,0,bytes,0};
        tracking.phase=FH_INSTALL_TEXTURE_EXACT;
        result=fh_install_texture_exact(&original,gpu,w*h*4,&output_format,&output_bytes,tile_progress,&tracking);
        if(result!=1)goto done;exact=1;
    }
    if(accept&&full.format==12&&edge&&(w>edge||h>edge)){
        gw=w>edge?edge:w;gh=h>edge?edge:h;small=malloc(gw*gh/2);
        if(!small){result=fail(t,"Not enough startup memory for reduced texture.");goto done;}
        tracking.phase=FH_INSTALL_TEXTURE_REDUCED;
        result=fh_install_texture_search_cmpr(source,bytes,w,h,gw,gh,small,gw*gh/2,&reduced,tile_progress,&tracking);
        if(result!=1)goto done;
        if(!acceptable(&reduced)){gw=gh=0;}
    }
    unsigned outputs=s->admitted+s->reduced+1+(gw!=0);
    if(outputs>FH_INSTALL_TEXTURE_LIMIT){result=fail(t,"Prepared texture directory exceeds runtime capacity.");goto done;}
    unsigned char header[HEADER]={0};memcpy(header,"FHTXSPL2",8);
    put(header+8,w);put(header+12,h);put(header+16,bytes);put(header+20,crc);
    put(header+24,output_format);put(header+28,output_bytes);put(header+32,(unsigned)crc32(0,gpu,output_bytes));
    if(gw){put(header+36,gw);put(header+40,gh);put(header+44,reduced.bytes);put(header+48,(unsigned)crc32(0,small,reduced.bytes));}
    put(header+52,full.visible_components);put(header+56,(uint32_t)full.squared_error);put(header+60,(uint32_t)(full.squared_error>>32));
    put(header+64,reduced.visible_components);put(header+68,(uint32_t)reduced.squared_error);put(header+72,(uint32_t)(reduced.squared_error>>32));
    put(header+76,edge);
    put(header+92,(unsigned)crc32(0,header,92));Entry entry;
    if(!decode(header,&entry,t->written)||record_bytes(&entry)>FILE_LIMIT-t->written){result=fail(t,"Invalid or oversized texture staging record.");goto done;}
    result=fh_install_artifact_write(t,header,HEADER)&&fh_install_artifact_write(t,source,bytes)&&
           fh_install_artifact_write(t,gpu,entry.full)&&fh_install_artifact_write(t,small,entry.small)&&
           payload_cursor(s,next_entry,next_offset);
    if(result){entries[s->count++]=entry;s->admitted++;s->reduced+=gw!=0;s->uncompressed+=exact;}
done:
    free(gpu);free(small);
    if(!result&&!t->failed)fail(t,"Native texture conversion failed.");
    if(result==-1&&!fh_install_texture_spool_flush(s))result=0;
    if(result<=0)fh_install_transaction_close(t);
    return result;
}
int fh_install_texture_spool_add_typed(FhInstallTextureSpool*s,const void*source,unsigned bytes,unsigned w,unsigned h,unsigned source_format,unsigned edge,
                                      uint32_t next_entry,uint32_t next_offset,FhInstallTextureStageProgress progress,void*context){
    if(source_format==14&&extent(w)&&extent(h))return fh_install_texture_spool_add(s,source,bytes,w,h,edge,next_entry,next_offset,progress,context);
    if(!s||!s->entries||!s->transaction)return 0;FhInstallTransaction*t=s->transaction;
    if(t->failed||!t->output)return 0;
    unsigned needed=fh_pack_source_bytes(source_format,w,h);
    if(!source||!needed||bytes!=needed||(edge&&(!extent(edge)||edge>512)))return fail(t,"Invalid typed source texture.");
    Entry*entries=s->entries;unsigned crc=(unsigned)crc32(0,source,bytes);
    if(s->count&&entries[0].edge!=edge)return fail(t,"Texture reduction recipe changed during conversion.");
    for(unsigned i=0;i<s->count;i++)if(entries[i].crc==crc&&entries[i].w==w&&entries[i].h==h&&entries[i].source_format==source_format){
        int equal=same(t,entries+i,source);if(equal<0)return 0;
        if(equal){if(!duplicate_cursor(s,next_entry,next_offset))return 0;s->duplicates++;return 1;}
    }
    if(s->count==FH_INSTALL_TEXTURE_LIMIT||s->admitted+s->reduced==FH_INSTALL_TEXTURE_LIMIT)return fail(t,"Typed texture directory exceeds runtime capacity.");
    unsigned format=fh_pack_exact_format(source_format),n=fh_pack_native_bytes(format,fh_pack_extent(w),fh_pack_extent(h));
    void*gpu=malloc(n);if(!gpu)return fail(t,"Not enough startup memory for exact texture.");
    FhInstallExactTexture image={source,NULL,w,h,source_format,0,0,bytes,0};Progress tracking={progress,context,FH_INSTALL_TEXTURE_EXACT};
    unsigned produced=0,actual_format=0,padded=source_format==14;
    FhInstallTextureResult encoded={0};int result=1;
    if(padded){
        tracking.phase=FH_INSTALL_TEXTURE_FULL;
        result=fh_install_texture_search_padded(source,bytes,w,h,gpu,n,&encoded,tile_progress,&tracking);
        if(result!=1)goto done;
    }
    if(padded&&acceptable(&encoded)){
        format=encoded.format;n=encoded.bytes;
    }else{
        tracking.phase=FH_INSTALL_TEXTURE_EXACT;
        result=fh_install_texture_exact(&image,gpu,n,&actual_format,&produced,tile_progress,&tracking);
        if(result!=1)goto done;
        if(produced!=n||actual_format!=format){result=fail(t,"Exact texture layout changed unexpectedly.");goto done;}
    }
    unsigned char header[HEADER]={0};memcpy(header,padded?"FHTXSPL4":"FHTXSPL3",8);
    if(padded){put(header+52,encoded.visible_components);put(header+56,(uint32_t)encoded.squared_error);put(header+60,(uint32_t)(encoded.squared_error>>32));}
    put(header+8,w);put(header+12,h);put(header+16,bytes);put(header+20,crc);put(header+24,format);
    put(header+28,n);put(header+32,(unsigned)crc32(0,gpu,n));put(header+76,edge);put(header+80,source_format);
    put(header+92,(unsigned)crc32(0,header,92));Entry entry;
    if(!decode(header,&entry,t->written)||record_bytes(&entry)>FILE_LIMIT-t->written){result=fail(t,"Invalid or oversized typed texture record.");goto done;}
    result=fh_install_artifact_write(t,header,HEADER)&&fh_install_artifact_write(t,source,bytes)&&fh_install_artifact_write(t,gpu,n)&&payload_cursor(s,next_entry,next_offset);
    if(result){entries[s->count++]=entry;s->admitted++;s->uncompressed+=format<12;}
done:
    free(gpu);if(!result&&!t->failed)fail(t,"Exact texture preparation failed.");
    if(result==-1&&!fh_install_texture_spool_flush(s))result=0;
    if(result<=0)fh_install_transaction_close(t);return result;
}
int fh_install_texture_spool_advance(FhInstallTextureSpool*s,uint32_t next_entry,uint32_t next_offset){
    if(!s||!s->entries||!s->transaction||s->transaction->failed)return 0;
    return duplicate_cursor(s,next_entry,next_offset);
}
int fh_install_texture_spool_add_mip(FhInstallTextureSpool*s,const void*source,unsigned bytes,unsigned parent_w,unsigned parent_h,unsigned source_format,
                                    unsigned parent_crc,unsigned level,uint32_t next_entry,uint32_t next_offset,FhInstallTextureStageProgress progress,void*context){
    if(!s||!s->entries||!s->transaction)return 0;FhInstallTransaction*t=s->transaction;
    if(t->failed||!t->output)return 0;
    unsigned w=parent_w>>level,h=parent_h>>level;
    if(!source||!level||level>9||!extent(parent_w)||!extent(parent_h)||w<8||h<8||bytes!=fh_pack_source_bytes(source_format,w,h))
        return fail(t,"Invalid texture mip level.");
    Entry*entries=s->entries;const Entry*parent=NULL;
    for(unsigned i=0;i<s->count;i++)if(entries[i].crc==parent_crc&&entries[i].w==parent_w&&entries[i].h==parent_h&&entries[i].source_format==source_format&&entries[i].full){parent=entries+i;break;}
    if(!parent){s->mip_rejected++;return 2;}
    Mip*mips=s->mips;
    for(unsigned i=0;i<s->mip_count;i++)if(mips[i].parent_crc==parent_crc&&mips[i].parent_w==parent_w&&mips[i].parent_h==parent_h&&
        mips[i].source_format==source_format&&mips[i].level==level){
        if(mips[i].format!=parent->format){s->mip_rejected++;return 2;}
        if(!duplicate_cursor(s,next_entry,next_offset))return 0;s->mip_duplicates++;return 1;
    }
    if(s->mip_count==MIP_LIMIT){s->mip_rejected++;return 2;}
    unsigned format=parent->format,n=fh_pack_native_bytes(format,w,h);
    if(!n){s->mip_rejected++;return 2;}
    unsigned char*gpu=malloc(w*h*4);if(!gpu)return fail(t,"Not enough startup memory for texture mip level.");
    Progress tracking={progress,context,FH_INSTALL_TEXTURE_EXACT};int result=1;unsigned produced=0,actual=0;
    if(source_format==14&&(format==12||format==13)){
        FhInstallTextureResult encoded={0};tracking.phase=FH_INSTALL_TEXTURE_FULL;
        result=fh_install_texture_search_cmpr(source,bytes,w,h,w,h,gpu,w*h*4,&encoded,tile_progress,&tracking);
        if(result!=1)goto done;
        if(encoded.format!=format||encoded.bytes!=n||!acceptable(&encoded)){result=2;s->mip_rejected++;goto done;}
    }else{
        FhInstallExactTexture image={source,NULL,w,h,source_format,0,0,bytes,0};
        result=fh_install_texture_exact(&image,gpu,w*h*4,&actual,&produced,tile_progress,&tracking);
        if(result!=1)goto done;
        if(actual!=format||produced!=n){result=2;s->mip_rejected++;goto done;}
    }
    unsigned char header[HEADER]={0};memcpy(header,"FHTXSPL5",8);
    put(header+8,w);put(header+12,h);put(header+24,format);put(header+28,n);put(header+32,(unsigned)crc32(0,gpu,n));
    put(header+36,parent_crc);put(header+40,parent_w);put(header+44,parent_h);put(header+48,level);put(header+80,source_format);
    put(header+92,(unsigned)crc32(0,header,92));Mip m;
    if(!decode_mip(header,&m,t->written)||HEADER+n>FILE_LIMIT-t->written){result=fail(t,"Invalid or oversized texture mip record.");goto done;}
    unsigned capacity=s->mip_capacity,count=s->mip_count;Mip probe=m;
    if(!mip_push((Mip**)&s->mips,&count,&capacity,&probe)){result=fail(t,"Not enough startup memory for texture mip directory.");goto done;}
    s->mip_capacity=capacity;
    result=fh_install_artifact_write(t,header,HEADER)&&fh_install_artifact_write(t,gpu,n)&&payload_cursor(s,next_entry,next_offset);
    if(result){s->mip_count=count;s->mip_admitted++;}
done:
    free(gpu);if(!result&&!t->failed)fail(t,"Texture mip preparation failed.");
    if(result==-1&&!fh_install_texture_spool_flush(s))result=0;
    if(result<=0)fh_install_transaction_close(t);return result;
}
int fh_install_texture_spool_finish(FhInstallTextureSpool*s){
    if(!s||!s->transaction||!s->entries)return 0;
    int result=fh_install_texture_spool_flush(s)&&fh_install_artifact_finish(s->transaction,spool_validate,NULL);fh_install_texture_spool_close(s);return result;
}
static int compare(const void*a,const void*b){
    const Output*x=a,*y=b;
#define ORDER(k) do{if(x->entry.k!=y->entry.k)return x->entry.k<y->entry.k?-1:1;}while(0)
    ORDER(crc);ORDER(w);ORDER(h);ORDER(source_format);ORDER(at);
#undef ORDER
    return x->reduced==y->reduced?0:x->reduced?-1:1;
}
static int mip_key(const Mip*x,const Mip*y){
#define ORDER(k) do{if(x->k!=y->k)return x->k<y->k?-1:1;}while(0)
    ORDER(parent_crc);ORDER(parent_w);ORDER(parent_h);ORDER(source_format);
    return 0;
}
static int mip_compare(const void*a,const void*b){
    const Mip*x=a,*y=b;int k=mip_key(x,y);if(k)return k;
    ORDER(level);ORDER(at);
#undef ORDER
    return 0;
}
static int copy(FhInstallTransaction*t,FILE*f,unsigned at,unsigned bytes){
    unsigned char buffer[STAGE];if(fseek(f,at,SEEK_SET))return fail(t,"Cannot seek prepared texture staging file.");
    while(bytes){unsigned n=bytes>sizeof(buffer)?sizeof(buffer):bytes;
        if(fread(buffer,1,n,f)!=n)return fail(t,"Cannot read prepared texture staging file.");
        if(!fh_install_artifact_write(t,buffer,n))return 0;bytes-=n;
    }
    return 1;
}
static int pack_validate(const char*path,void*context){
    (void)context;FhTexturePack p={0};int ok=fh_texture_pack_open(&p,path,FH_INSTALL_TEXTURE_LIMIT*44);
    for(unsigned i=0;ok&&i<p.count;i++){
        const unsigned char*e=p.directory+i*p.stride;
        ok=!fseek(p.file,get(e+16),SEEK_SET)&&checksum(p.file,get(e+12),get(e))&&checksum(p.file,get(e+24),get(e+28));
    }
    for(unsigned i=0;ok&&i<p.mip_count;i++){
        const unsigned char*e=p.mips+i*FH_TEXTURE_MIP_STRIDE;
        ok=!fseek(p.file,get(e+24),SEEK_SET)&&checksum(p.file,get(e+28),get(e+32));
    }
    fh_texture_pack_close(&p);return ok;
}
int fh_install_texture_pack(FhInstallTransaction*t,unsigned spool,unsigned output,FhInstallTextureProgress progress,void*context){
    if(!t||t->failed||t->output)return 0;
    if(spool>=FH_INSTALL_ARTIFACTS||output>=FH_INSTALL_ARTIFACTS||spool==output||!(t->state.complete&(1u<<spool)))return fail(t,"Invalid texture-pack preparation recipe.");
    if(t->state.complete&(1u<<output))return 1;
    char filename[FH_INSTALL_TX_PATH];if(!fh_install_artifact_path(t->root,&t->state,spool,filename))return 0;
    FILE*f=fopen(filename,"rb");if(!f)return fail(t,"Cannot open completed texture staging file.");
    Entry*entries=malloc(FH_INSTALL_TEXTURE_LIMIT*sizeof(Entry));Output*ordered=NULL;unsigned char*directory=NULL,*mip_directory=NULL;unsigned source_count=0,count=0,mip_count=0,mip_capacity=0;int result=0;
    Mip*mips=NULL;Chain*chains=NULL;
    if(!entries){fail(t,"Not enough memory for texture assembly directory.");goto done;}
    if(!spool_read(f,entries,&source_count,1,&mips,&mip_count,&mip_capacity)){fail(t,"Completed texture staging file failed validation.");goto done;}
    for(unsigned i=0;i<source_count;i++)count+=(entries[i].full!=0)+(entries[i].small!=0);
    if(!count){fail(t,"No textures passed the conversion quality checks.");goto done;}
    unsigned stride=mip_count?44:40,padded_pack=0;
    for(unsigned i=0;i<source_count;i++){
        unsigned padded=!extent(entries[i].w)||!extent(entries[i].h);
        if(entries[i].source_format!=14||padded)stride=44;
        if(entries[i].source_format==14&&padded)padded_pack=1;
    }
    ordered=malloc(count*sizeof(Output));directory=calloc(1,16+count*stride);
    if(!ordered||!directory){fail(t,"Not enough memory to assemble texture pack.");goto done;}
    unsigned j=0;
    for(unsigned i=0;i<source_count;i++)if(entries[i].full){ordered[j++]=(Output){entries[i],0};if(entries[i].small)ordered[j++]=(Output){entries[i],1};}
    free(entries);entries=NULL;qsort(ordered,count,sizeof(Output),compare);
    memcpy(directory,padded_pack?"FHTXPK4":stride==44?"FHTXPK3":"FHTXPK2",8);put(directory+8,count);unsigned end=16+count*stride;
    for(unsigned i=0;i<count;i++){
        const Entry*e=&ordered[i].entry;unsigned small=ordered[i].reduced,gpu=small?e->small:e->full;unsigned char*d=directory+16+i*stride;
        if(e->source+gpu>FILE_LIMIT-end){fail(t,"Prepared texture pack exceeds runtime size limit.");goto done;}
        put(d,e->crc);put(d+4,e->w);put(d+8,e->h);put(d+12,e->source);put(d+16,end);put(d+20,e->format);
        put(d+24,gpu);put(d+28,small?e->small_crc:e->full_crc);put(d+32,small?e->gw:fh_pack_extent(e->w));put(d+36,small?e->gh:fh_pack_extent(e->h));
        if(stride==44)put(d+40,e->source_format);end+=e->source+gpu;
    }
    unsigned chain_count=0,mip_at=end,file_end=end;
    if(mip_count){
        qsort(mips,mip_count,sizeof(Mip),mip_compare);
        chains=malloc(mip_count*sizeof(Chain));if(!chains){fail(t,"Not enough memory for texture mip chains.");goto done;}
        for(unsigned i=0;i<mip_count;){
            unsigned g=i;while(g<mip_count&&!mip_key(mips+i,mips+g))g++;
            const Mip*k=mips+i;const Entry*parent=NULL;unsigned parents=0;
            for(unsigned j=0;j<count;j++){const Entry*e=&ordered[j].entry;
                if(e->crc==k->parent_crc&&e->w==k->parent_w&&e->h==k->parent_h&&e->source_format==k->source_format){parents++;if(!ordered[j].reduced)parent=e;}}
            unsigned levels=0,bytes=0,crc=0;
            if(parent&&parents==1+(parent->small!=0)&&extent(parent->w)&&extent(parent->h))
                for(unsigned level=1,j=i;j<g&&mips[j].level==level&&mips[j].format==parent->format;level++,j++){
                    crc=levels?(unsigned)crc32_combine(crc,mips[j].crc,mips[j].bytes):mips[j].crc;bytes+=mips[j].bytes;levels++;
                }
            if(levels&&chain_count<FH_TEXTURE_MIP_ENTRIES)chains[chain_count++]=(Chain){k->parent_crc,k->parent_w,k->parent_h,k->source_format,parent->format,levels,i,0,bytes,crc};
            i=g;
        }
    }
    if(chain_count){
        if(stride!=44){fail(t,"Texture mip chains require typed pack entries.");goto done;}
        memcpy(directory,"FHTXPK5",8);
        mip_directory=calloc(1,16+chain_count*36);if(!mip_directory){fail(t,"Not enough memory for texture mip directory.");goto done;}
        unsigned at=mip_at+16+chain_count*36;
        for(unsigned c=0;c<chain_count;c++){
            Chain*x=chains+c;unsigned char*d=mip_directory+16+c*36;
            if(x->bytes>FILE_LIMIT-at){fail(t,"Prepared texture pack exceeds runtime size limit.");goto done;}
            x->offset=at;at+=x->bytes;
            put(d,x->crc);put(d+4,x->w);put(d+8,x->h);put(d+12,x->source_format);put(d+16,x->format);put(d+20,x->levels);
            put(d+24,x->offset);put(d+28,x->bytes);put(d+32,x->chain_crc);
        }
        memcpy(mip_directory,"FHTXMIP1",8);put(mip_directory+8,chain_count);put(mip_directory+12,(unsigned)crc32(0,mip_directory+16,chain_count*36));
        file_end=at;
    }
    unsigned total=count+(chain_count?1+chain_count:0);
    put(directory+12,(unsigned)crc32(0,directory+16,count*stride));
    if(!fh_install_artifact_begin(t,output))goto done;
    unsigned next=0;
    if(!t->written){
        if(!fh_install_artifact_write(t,directory,16+count*stride)||!fh_install_artifact_checkpoint(t,0,PACK_CURSOR))goto done;
    }else{
        next=t->state.cursor[0];
        unsigned expected=next<count?get(directory+16+next*stride+16):next==count?mip_at:next-count-1<chain_count?chains[next-count-1].offset:file_end;
        if(t->state.cursor[1]!=PACK_CURSOR||next>total||expected!=t->written){fail(t,"Invalid texture-pack assembly cursor.");goto done;}
        unsigned char chunk[STAGE];
        for(unsigned at=0;at<16+count*stride;){unsigned n=16+count*stride-at;if(n>sizeof(chunk))n=sizeof(chunk);
            if(!fh_install_artifact_read(t,at,chunk,n)||memcmp(chunk,directory+at,n)){fail(t,"Texture directory changed during preparation.");goto done;}at+=n;
        }
    }
    if(progress&&!progress(context,next,total)){result=-1;goto done;}
    for(unsigned i=next;i<total;i++){
        if(i<count){
            const Entry*e=&ordered[i].entry;unsigned small=ordered[i].reduced;
            if(!copy(t,f,e->at+HEADER,e->source)||!copy(t,f,e->at+HEADER+e->source+(small?e->full:0),small?e->small:e->full))goto done;
        }else if(i==count){
            if(!fh_install_artifact_write(t,mip_directory,16+chain_count*36))goto done;
        }else{
            const Chain*x=chains+i-count-1;
            for(unsigned l=0;l<x->levels;l++)if(!copy(t,f,mips[x->first+l].at+HEADER,mips[x->first+l].bytes))goto done;
        }
        if(!fh_install_assembly_checkpoint(t,i+1,PACK_CURSOR,total))goto done;
        if(progress&&!progress(context,i+1,total)){result=fh_install_artifact_checkpoint(t,i+1,PACK_CURSOR)?-1:0;goto done;}
    }
    result=fh_install_artifact_finish(t,pack_validate,NULL);
done:
    free(entries);free(ordered);free(directory);free(mips);free(chains);free(mip_directory);
    if(fclose(f)&&result>0)result=fail(t,"Cannot close texture staging file.");
    if(result<=0)fh_install_transaction_close(t);return result;
}
