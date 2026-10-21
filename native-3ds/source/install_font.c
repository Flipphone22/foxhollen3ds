#include "fh_install_font.h"
#include "fh_texture_pack_layout.h"
#include <stdlib.h>
#include <string.h>

static unsigned u16(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static unsigned u32(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int span(unsigned at,unsigned n,unsigned bytes){return at<=bytes&&n<=bytes-at;}
int fh_install_font_layout(const void*data,unsigned bytes,FhInstallFontLayout*out){
    if(!out)return 0;memset(out,0,sizeof(*out));
    if(!data||bytes<4||bytes>FH_INSTALL_FONT_BYTES)return 0;
    const unsigned char*p=data;unsigned glyphs=u32(p);FhInstallFontLayout layout={0};
    if(!glyphs){*out=layout;return 1;}
    if(glyphs>(bytes-4)/16)return 0;
    /* The source table header overlaps the unused last four glyph bytes. */
    unsigned at=glyphs*16;if(!span(at,8,bytes))return 0;
    unsigned entries=u16(p+at+4),strings_offset=u16(p+at+6);at+=8;
    if(entries>(bytes-at)/12)return 0;at+=entries*12;
    if(!span(at,4,bytes))return 0;unsigned strings=u32(p+at);at+=4;
    if(strings>(bytes-at)/4)return 0;at+=strings*4;
    if(!span(at,strings_offset,bytes))return 0;at+=strings_offset;
    if(!span(at,4,bytes))return 0;unsigned skip=u32(p+at);
    if(!span(at,skip,bytes)||!span(at+skip,4,bytes))return 0;at+=skip+4;
    for(;;){
        if(!span(at,8,bytes))return 0;
        unsigned kind=u16(p+at),bits=u16(p+at+2),w=u16(p+at+4),h=u16(p+at+6);
        if(!w&&!h){layout.end=at;*out=layout;return 1;}
        if(layout.count==FH_INSTALL_FONT_IMAGES||!w||!h||w>1024||h>1024)return 0;
        unsigned fmt;if(kind==1&&bits==16)fmt=5;else if(kind==2&&bits==4)fmt=0;else return 0;
        unsigned size=w*h*bits/8;
        /* Do not synthesize unread tile padding: the source copies exactly
         * area*bpp bytes. Reject a layout that needs any additional bytes. */
        if(size!=fh_pack_source_bytes(fmt,w,h)||!span(at+8,size,bytes))return 0;
        layout.images[layout.count++]=(FhInstallFontImage){at+8,size,w,h,fmt};at+=8+size;
    }
}
static int fail(FhInstallTransaction*t,const char*message){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",message);return 0;}
static int font_path(const char*path){
    if(*path=='/')path++;size_t n=strlen(path);
    return n>13&&!memcmp(path,"gametext/",9)&&!strcmp(path+n-4,".bin");
}
int fh_install_font_prepare(FhInstallTextureSpool*s,FhInstallDisc*d,unsigned edge,FhInstallTextureStageProgress progress,void*context){
    if(!s||!s->transaction||!s->entries||!d||!d->file)return 0;
    FhInstallTransaction*t=s->transaction;unsigned resume=t->state.cursor[0]&FH_INSTALL_FONT_CURSOR;
    unsigned first=resume?(t->state.cursor[0]&~FH_INSTALL_FONT_CURSOR):1,offset=resume?t->state.cursor[1]:0;
    if(!first||first>=d->entries)return fail(t,"Invalid font preparation cursor.");
    for(unsigned i=first;i<d->entries;i++){
        FhInstallEntry e;char path[FH_INSTALL_PATH_CAP];
        if(!fh_install_disc_entry(d,i,&e)||!fh_install_disc_path(d,i,path,sizeof(path)))return fail(t,"Cannot enumerate game-text atlases.");
        if(e.directory||!font_path(path)){if(resume&&i==first)return fail(t,"Font cursor names another source.");continue;}
        if(progress&&!progress(context,2,i,d->entries))return -1;
        if(e.bytes<4||e.bytes>FH_INSTALL_FONT_BYTES)return fail(t,"Game-text file exceeds startup budget.");
        unsigned char*data=malloc(e.bytes);if(!data)return fail(t,"Not enough startup memory for game text.");
        FhInstallFontLayout layout;int ok=fh_install_disc_read(d,i,0,data,e.bytes)&&fh_install_font_layout(data,e.bytes,&layout);
        if(!ok){free(data);return fail(t,"Invalid game-text atlas layout.");}
        unsigned start=0;
        if(resume&&i==first){
            while(start<layout.count&&layout.images[start].offset-8<offset)start++;
            if(offset!=(start<layout.count?layout.images[start].offset-8:layout.end)){free(data);return fail(t,"Font cursor is not an image boundary.");}
        }
        for(unsigned j=start;j<layout.count;j++){
            const FhInstallFontImage*image=layout.images+j;
            int result=fh_install_texture_spool_add_typed(s,data+image->offset,image->bytes,image->width,image->height,image->format,edge,
                FH_INSTALL_FONT_CURSOR|i,image->offset+image->bytes,progress,context);
            if(result!=1){free(data);return result;}
        }
        free(data);
    }
    return 1;
}
