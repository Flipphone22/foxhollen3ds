#include "fh_install_actor_fog_width_catalog.h"
#include <string.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int fail(FhInstallActorFogWidthCatalog*s,const char*why){snprintf(s->error,sizeof(s->error),"%s",why);return 0;}
int fh_install_actor_fog_width_catalog_begin(FhInstallActorFogWidthCatalog*s,FhInstallDisc*d){
    if(!s)return 0;
    memset(s,0,sizeof(*s));
    if(!d||!d->file)return fail(s,"No source disc for actor fog texture dimensions.");
    for(unsigned i=1;i<d->entries;i++){
        FhInstallEntry entry;char path[FH_INSTALL_PATH_CAP];
        if(!fh_install_disc_entry(d,i,&entry)||!fh_install_disc_path(d,i,path,sizeof(path)))return fail(s,"Invalid source texture path.");
        if(entry.directory)continue;
        const char*name=strrchr(path,'/');
        if(!name||strcmp(name+1,"TEX1.tab"))continue;
        if(s->count==FH_INSTALL_FOG_TEXTURE_BANKS)return fail(s,"Too many source TEX1 banks.");
        if(fh_install_disc_find(d,path)!=(int)i)return fail(s,"Ambiguous source TEX1 table path.");
        memcpy(path+strlen(path)-3,"bin",3);int data=fh_install_disc_find(d,path);
        FhInstallActorFogWidths reader;
        if(data<0||!fh_install_actor_fog_width_begin(&reader,d,i,(unsigned)data))return fail(s,"Invalid or missing paired TEX1 data bank.");
        s->banks[s->count].table=i;s->banks[s->count].data=(unsigned)data;
        s->banks[s->count++].entries=reader.entries;
    }
    if(!s->count)return fail(s,"No source TEX1 banks.");
    s->disc=d;return 1;
}
int fh_install_actor_fog_width_catalog(void*context,uint32_t id,unsigned*out){
    FhInstallActorFogWidthCatalog*s=context;
    if(!s||!s->disc||!s->disc->file||!out||id>65535||s->error[0])return 0;
    unsigned index=id&0x7fff,width=0;s->requests++;
    for(unsigned i=0;i<64;i++)if(s->cached[i].index_plus_one==index+1){
        *out=s->cached[i].width;s->hits++;return 1;
    }
    for(unsigned i=0;i<s->count;i++){
        if(index>=s->banks[i].entries)continue;
        unsigned char entry[4];
        if(!fh_install_disc_read(s->disc,s->banks[i].table,index*4,entry,4))return fail(s,"Cannot read source TEX1 index.");
        unsigned value=word(entry);
        /* mergeTableFiles marks physically present A entries with bit30 and
         * leaves B entries with bit31. On disc, bit31 denotes presence; plain
         * nonzero words can be placeholders referring to another live bank. */
        if(value==0xffffffffu||!(value&0x80000000u))continue;
        FhInstallActorFogWidths reader;unsigned candidate;
        if(!fh_install_actor_fog_width_begin(&reader,s->disc,s->banks[i].table,s->banks[i].data)||
           !fh_install_actor_fog_width(&reader,index,&candidate))return fail(s,"Invalid present TEX1 image dimensions.");
        if(reader.workspace_peak>s->workspace_peak)s->workspace_peak=reader.workspace_peak;
        s->definitions++;
        if(width&&width!=candidate){s->conflicts++;return fail(s,"TEX1 dimensions depend on live area; contextual fog variants required.");}
        width=candidate;
    }
    if(!width)return fail(s,"No present TEX1 definition for actor fog texture.");
    unsigned slot=s->cursor++%64;s->cached[slot].index_plus_one=index+1;s->cached[slot].width=width;
    *out=width;return 1;
}
