#ifndef FH_INSTALL_PROFILE_H
#define FH_INSTALL_PROFILE_H
#include <stdint.h>
#include <string.h>

/* Inclusive wall spans between progress callbacks, not CPU utilization or
 * isolated SD timings. No writes while a converter owns its working files. */
enum {FH_INSTALL_PROFILE_PHASES=7};
typedef struct {
    uint64_t milliseconds[FH_INSTALL_PROFILE_PHASES], last;
    uint32_t callbacks;
    unsigned active, seen;
} FhInstallProfile;
static inline unsigned fh_install_profile_phase(const char*label){
    if(!strncmp(label,"Open source",11)||!strncmp(label,"Verify source",13))return 0;
    if(!strncmp(label,"Original geometry",17))return 1;
    if(!strncmp(label,"Performance",11))return 2;
    if(!strncmp(label,"Textures",8))return 3;
    if(!strncmp(label,"Movie",5))return 4;
    if(!strncmp(label,"Verify:",7))return 5;
    return 6;
}
static inline void fh_install_profile_begin(FhInstallProfile*p,uint64_t now){
    memset(p,0,sizeof(*p));p->active=6;p->seen=1u<<6;p->last=now;
}
static inline void fh_install_profile_finish(FhInstallProfile*p,uint64_t now){
    if(now>=p->last)p->milliseconds[p->active]+=now-p->last;
    p->last=now;
}
static inline void fh_install_profile_progress(FhInstallProfile*p,const char*label,uint64_t now){
    if(!strncmp(label,"@install/",9))return; /* Metadata, not a phase switch. */
    fh_install_profile_finish(p,now);p->active=fh_install_profile_phase(label);
    p->seen|=1u<<p->active;p->callbacks++;
}
#endif
