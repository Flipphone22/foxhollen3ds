#include "fh_install_task.h"
#include "fh_install_fingerprint.h"
#include "fh_install_geometry_pack.h"
#include "fh_install_performance_pack.h"
#include "fh_install_texture_iso.h"
#include "fh_install_movie_pack.h"
#include "fh_install_cache.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    FhStartupProgress call;void*context;const char*cache_label;
    uint32_t fraction[6],verify_total,verify_done,verify_last;
    unsigned overall_seen;
    char verify_name[40];
} Progress;
static int overall(Progress*p,unsigned phase,unsigned base,unsigned span,unsigned done,unsigned total){
    if(phase>=6||done>total)return 0;
    unsigned f=base+(total?(unsigned)((uint64_t)span*done/total):0);
    if(f>1000000)return 0;
    /* Subtasks can revisit a scan callback after a pack callback when resuming.
     * Keep that phase's estimated completion monotonic, without changing work. */
    if(f<p->fraction[phase])f=p->fraction[phase];
    /* The movie reports each native tile, but overall completion advances
     * once per source frame. Do not repeat its metadata/timing/ETA traversal
     * 1024 times. The ordinary task callback still polls cancellation/UI on
     * every tile; preserve the first zero-progress event for each phase. */
    if((p->overall_seen&(1u<<phase))&&f==p->fraction[phase])return 1;
    p->overall_seen|=1u<<phase;p->fraction[phase]=f;
    char event[11]="@install/0";event[9]=(char)('0'+phase);
    return !p->call||p->call(p->context,event,f,1000000);
}
static int report(Progress*p,const char*name,unsigned done,unsigned total){
    if(!strcmp(name,"Verify source disc")&&!overall(p,0,0,1000000,done,total))return 0;
    return !p->call||p->call(p->context,name,done,total);
}
static int geometry(void*c,unsigned phase,unsigned done,unsigned total){
    if(!overall(c,1,phase==FH_INSTALL_GEOMETRY_SCAN?0:800000,phase==FH_INSTALL_GEOMETRY_SCAN?800000:150000,done,total))return 0;
    return report(c,phase==FH_INSTALL_GEOMETRY_SCAN?"Original geometry: scan records":"Original geometry: pack",done,total);
}
static int performance(void*c,unsigned phase,unsigned done,unsigned total){
    static const char*names[]={"Performance geometry: scan records","Performance geometry: reduce","Performance topology: pack","Performance geometry: pack"};
    if(phase>=4)return 0;
    if(phase!=1&&!overall(c,2,phase==0?0:phase==2?850000:880000,phase==0?850000:phase==2?30000:70000,done,total))return 0;
    return report(c,names[phase],done,total);
}
static int textures(void*c,unsigned phase,uint32_t done,uint32_t total){
    if(phase==FH_INSTALL_TEX_COLLECTION)return overall(c,3,0,980000,done,total);
    static const char*names[]={"Textures: full image","Textures: reduced image","Textures: scan","Textures: pack","Textures: exact image"};
    if(phase==FH_INSTALL_TEX_ASSEMBLE&&!overall(c,3,980000,20000,done,total))return 0;
    return phase<5&&report(c,names[phase],done,total);
}
static int movie(void*c,unsigned phase,unsigned frame,unsigned frames,unsigned done,unsigned total){
    static const char*names[]={"Movie: verify source","Movie: index","Movie: decode","Movie: resize","Movie: encode","Movie: compress","Movie: checkpoint","Movie: pack"};
    char label[40];
    if(phase>=8||frame>frames)return 0;
    if(phase<=1&&!overall(c,4,phase?30000:0,phase?10000:30000,done,total))return 0;
    if(phase>=2&&phase<=6&&!overall(c,4,40000,900000,frame,frames))return 0;
    if(phase==7&&!overall(c,4,940000,60000,done,total))return 0;
    /* Frame number is informational; bar retains this phase's actual units. */
    if(phase>=2&&phase<=6)snprintf(label,sizeof(label),"%s %u/%u",names[phase],frame<frames?frame+1:frames,frames);
    else snprintf(label,sizeof(label),"%s",names[phase]);
    return report(c,label,done,total);
}
static int cache(void*c,uint32_t done,uint32_t total){Progress*p=c;return overall(p,!strncmp(p->cache_label,"Original",8)?1:2,950000,50000,done,total)&&report(p,p->cache_label,done,total);}
static int verify(void*c,const char*name,uint32_t done,uint32_t total){
    Progress*p=c;
    if(strcmp(name,p->verify_name)){p->verify_done+=p->verify_last;p->verify_last=0;snprintf(p->verify_name,sizeof(p->verify_name),"%s",name);}
    p->verify_last=done;
    if(!overall(p,5,0,990000,p->verify_done+done,p->verify_total))return 0;
    char label[40];snprintf(label,sizeof(label),"Verify: %.31s",name);return report(c,label,done,total);
}
static int valid(const FhInstallTask*t){
    char path[FH_INSTALL_TX_PATH];
    if(!t||!t->disc_path||!*t->disc_path||!t->spec.disc_bytes||!fh_install_bundle_stage_root(t->root,0,path))return 0;
    for(unsigned i=0;i<5;i++){
        const unsigned char*p=i<4?t->spec.stage_recipe[i]:t->spec.source;unsigned bits=0;
        for(unsigned j=0;j<32;j++)bits|=p[j];if(!bits)return 0;
    }
    return 1;
}
static int disc_matches(const FhInstallTask*t,const FhInstallDisc*d){return d->bytes==t->spec.disc_bytes&&d->fst_crc==t->spec.fst_crc;}
unsigned fh_install_task_ready(void*context){
    FhInstallTask*t=context;if(!valid(t)||t->busy)return 0;
    memset(&t->selected,0,sizeof(t->selected));
    FhInstallDisc*d=calloc(1,sizeof(*d));if(!d)return 0;
    /* Runtime needs every FST file, not unused disc padding. disc_open
     * validates all extents against physical length and the PAL revision.
     * Keep the exact size/hash rule in prepare(), which creates new bundles.
     * This does not relabel or change an existing conversion's source hash. */
    unsigned ready=fh_install_disc_open(d,t->disc_path)&&
        d->bytes<=t->spec.disc_bytes&&d->fst_crc==t->spec.fst_crc&&
        fh_install_bundle_open(t->root,&t->spec,&t->selected);
    fh_install_disc_close(d);free(d);return ready;
}
static int empty_valid(const char*path,void*context){
    (void)context;unsigned char b[17],expected[16]="FHMLPK1";FILE*f=fopen(path,"rb");if(!f)return 0;
    size_t n=fread(b,1,sizeof(b),f);int ok=n==16&&!ferror(f)&&!memcmp(b,expected,16);if(fclose(f))ok=0;return ok;
}
static int empty_topology(FhInstallTransaction*t){
    if(t->state.complete&8)return 1;
    unsigned char b[16]="FHMLPK1";
    if(!fh_install_artifact_begin(t,3))return 0;
    if(!t->written&&!fh_install_artifact_write(t,b,sizeof(b)))return 0;
    return fh_install_artifact_finish(t,empty_valid,NULL);
}
int fh_install_task_prepare(void*context,FhStartupProgress callback,void*opaque,char*error,size_t capacity){
    FhInstallTask*t=context;FhInstallDisc*d=NULL;FhInstallFingerprint fingerprint={0};
    FhInstallTransaction tx={0};FhInstallGeneration stages[4]={{0}};Progress p={.call=callback,.context=opaque};
    int result=0;const char*why="Invalid installation recipe.";char directory[FH_INSTALL_TX_PATH];
    if(error&&capacity)error[0]=0;
    if(!valid(t)||t->busy){if(error&&capacity)snprintf(error,capacity,"%s",why);return 0;}
    t->busy=1;
    d=calloc(1,sizeof(*d));if(!d){why="Not enough startup memory for disc reader.";goto done;}
    if(!report(&p,"Open source disc",0,1)){result=-1;goto done;}
    if(!fh_install_disc_open(d,t->disc_path)){why=d->error;goto done;}
    if(!disc_matches(t,d)){why="Disc size or file table does not match this recipe.";goto done;}
    if(!fh_install_fingerprint_begin(&fingerprint,d)){why=fingerprint.error;goto done;}
    do{
        if(!report(&p,"Verify source disc",fingerprint.done,fingerprint.total)){result=-1;goto done;}
        result=fh_install_fingerprint_step(&fingerprint);
        if(result<0){result=0;why=fingerprint.error;goto done;}
    }while(!result);
    result=0;
    if(memcmp(fingerprint.digest,t->spec.source,32)){why="Disc fingerprint does not match this recipe.";goto done;}
    if(!report(&p,"Verify source disc",fingerprint.total,fingerprint.total)){result=-1;goto done;}
    fh_install_fingerprint_cancel(&fingerprint);
    if(mkdir(t->root,0777)&&errno!=EEXIST){why="Cannot create installation directory.";goto done;}
    for(unsigned stage=0;stage<4;stage++){
        static const char*names[]={"Original geometry: resume","Performance geometry: resume","Textures: resume","Movie: resume"};
        if(!report(&p,names[stage],0,1)){result=-1;goto done;}
        if(!fh_install_bundle_stage_root(t->root,stage,directory)||
           !fh_install_transaction_open(&tx,directory,t->spec.stage_recipe[stage],t->spec.source,fh_install_bundle_stage_mask(stage))){why=tx.error;goto done;}
        if(stage==FH_BUNDLE_ORIGINAL){
            result=fh_install_geometry_prepare(&tx,d,0,1,geometry,&p);
            if(result==1){p.cache_label="Original geometry: cache";result=fh_install_cache(&tx,1,2,FH_INSTALL_TASK_CACHE_LEVEL,cache,&p);}
            if(result==1)result=empty_topology(&tx);
        }else if(stage==FH_BUNDLE_PERFORMANCE){
            result=fh_install_performance_prepare(&tx,d,0,1,2,FH_INSTALL_TASK_RATIO,FH_INSTALL_TASK_ERROR,performance,&p);
            if(result==1){p.cache_label="Performance geometry: cache";result=fh_install_cache(&tx,2,3,FH_INSTALL_TASK_CACHE_LEVEL,cache,&p);}
        }else if(stage==FH_BUNDLE_TEXTURES){
            FhInstallTextureCoverage coverage={0};result=fh_install_texture_prepare_bases(&tx,d,0,1,FH_INSTALL_TASK_TEXTURE_EDGE,&coverage,textures,&p);
        }else result=fh_install_movie_prepare_rgb8(&tx,d,0,1,movie,&p);
        if(result!=1){why=tx.error;goto done;}
        result=0;
        if(tx.state.complete!=tx.state.required||tx.state.partial!=FH_INSTALL_NO_ARTIFACT||tx.failed){why="Converter returned an incomplete stage.";goto done;}
        stages[stage]=tx.state;fh_install_transaction_close(&tx);
        if(!overall(&p,stage+1,1000000,0,0,1)){result=-1;goto done;}
    }
    p.verify_total=stages[0].bytes[3]+stages[0].bytes[1]+stages[0].bytes[2]+
        stages[1].bytes[1]+stages[1].bytes[2]+stages[1].bytes[3]+stages[2].bytes[1]+stages[3].bytes[1];
    /* Release the source/FST before the complete-output verification pass. */
    fh_install_disc_close(d);free(d);d=NULL;
    result=fh_install_bundle_publish(t->root,&t->spec,stages,verify,&p,error,capacity);
    if(!result){why=error&&capacity&&*error?NULL:"Cannot publish completed installation.";goto done;}
    if(result==1&&!fh_install_bundle_open(t->root,&t->spec,&t->selected)){result=0;why="Published installation failed readiness verification.";}
done:
    /* Copy errors before destroying the object that owns their strings. */
    if(!result&&why&&error&&capacity)snprintf(error,capacity,"%s",*why?why:"Asset preparation failed.");
    fh_install_fingerprint_cancel(&fingerprint);fh_install_transaction_close(&tx);
    if(d){fh_install_disc_close(d);free(d);}t->busy=0;
    return result;
}
