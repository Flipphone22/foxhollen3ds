#include "fh_install_effects.h"
#include "fh_install_actor_fog_iso.h"
#include "fh_install_fingerprint.h"
#include <stdlib.h>
#include <string.h>
unsigned fh_install_effects_ready(void*context){
    FhInstallEffectsTask*t=context;if(!t||t->busy)return 0;
    memset(&t->selected,0,sizeof(t->selected));
    return fh_install_task_ready(&t->base)&&fh_install_effects_open(t->base.root,&t->base.spec,&t->base.selected,&t->selected);
}
typedef struct {FhStartupProgress call;void*context;unsigned original;} Progress;
static int report(Progress*p,const char*name,unsigned done,unsigned total){return !p->call||p->call(p->context,name,done,total);}
static int fog_progress(void*context,unsigned done,unsigned total){
    Progress*p=context;return report(p,p->original?"@install/6":"@install/7",done,total)&&
        report(p,p->original?"Original actor fog: prepare":"Performance actor fog: prepare",done,total);
}
int fh_install_effects_prepare(void*context,FhStartupProgress progress,void*opaque,char*error,size_t capacity){
    FhInstallEffectsTask*t=context;FhInstallDisc*d=NULL;FhInstallFingerprint fingerprint={0};
    FhInstallTransaction tx={0};Progress p={progress,opaque,1};int result=0,verified=0;
    char directory[FH_INSTALL_TX_PATH],topology[FH_INSTALL_TX_PATH];unsigned char key[32];
    const char*why="Invalid actor fog preparation configuration.";
    if(error&&capacity)error[0]=0;
    if(!t||t->busy||t->base.busy){if(error&&capacity)snprintf(error,capacity,"%s",why);return 0;}
    t->busy=1;
    if(!report(&p,"@install/extra",0,1)){result=-1;goto done;}
    /* Reuse already committed base assets. Calling their Prepare unconditionally
     * would create new generations and repeat expensive conversion work. */
    if(!fh_install_task_ready(&t->base)){
        result=fh_install_task_prepare(&t->base,progress,opaque,error,capacity);
        if(result!=1){why=NULL;goto done;}verified=1;result=0;
    }
    if(!fh_install_effects_root(t->base.root,directory)||!fh_install_effects_key(&t->base.spec,&t->base.selected,key))goto done;
    if(fh_install_effects_open(t->base.root,&t->base.spec,&t->base.selected,&t->selected)){result=1;goto done;}
    d=calloc(1,sizeof(*d));if(!d){why="Not enough startup memory for actor fog disc reader.";goto done;}
    if(!fh_install_disc_open(d,t->base.disc_path)){why=d->error;goto done;}
    if(d->bytes!=t->base.spec.disc_bytes||d->fst_crc!=t->base.spec.fst_crc){why="New actor fog conversion needs the full original PAL revision 1 ISO.";goto done;}
    if(!verified){
        if(!fh_install_fingerprint_begin(&fingerprint,d)){why=fingerprint.error;goto done;}
        do{
            if(!report(&p,"@install/0",fingerprint.done,fingerprint.total)||
               !report(&p,"Verify source for actor fog",fingerprint.done,fingerprint.total)){result=-1;goto done;}
            result=fh_install_fingerprint_step(&fingerprint);
            if(result<0){result=0;why=fingerprint.error;goto done;}
        }while(!result);
        result=0;if(memcmp(fingerprint.digest,t->base.spec.source,32)){why="Disc fingerprint does not match actor fog recipe.";goto done;}
        fh_install_fingerprint_cancel(&fingerprint);
    }
    if(!report(&p,"@install/5",1,1)){result=-1;goto done;}
    if(!fh_install_transaction_open(&tx,directory,key,t->base.spec.source,3)){why=tx.error;goto done;}
    for(unsigned i=0;i<2;i++){
        p.original=!i;const char*patch=NULL;
        if(i){if(!fh_install_bundle_path(&t->base.selected,FH_BUNDLE_PERFORMANCE_MODEL,topology)){why="Cannot select committed performance topology.";goto done;}patch=topology;}
        result=fh_install_actor_fog_iso(&tx,d,i,patch,NULL,fog_progress,&p);
        if(result!=1){why=tx.error;goto done;}result=0;
        if(!report(&p,i?"@install/7":"@install/6",1,1)){result=-1;goto done;}
    }
    if(!report(&p,"Publish actor fog assets",0,1)){result=-1;goto done;}
    if(!fh_install_transaction_publish(&tx)){why=tx.error;goto done;}
    if(!fh_install_effects_open(t->base.root,&t->base.spec,&t->base.selected,&t->selected)){why="Published actor fog assets failed readiness validation.";goto done;}
    result=1;
done:
    if(!result&&why&&error&&capacity)snprintf(error,capacity,"%s",*why?why:"Actor fog preparation failed.");
    fh_install_fingerprint_cancel(&fingerprint);fh_install_transaction_close(&tx);
    if(d){fh_install_disc_close(d);free(d);}if(t)t->busy=0;return result;
}
