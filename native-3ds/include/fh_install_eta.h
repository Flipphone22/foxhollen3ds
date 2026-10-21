#ifndef FH_INSTALL_ETA_H
#define FH_INSTALL_ETA_H
#include <stdint.h>
#include <string.h>

/* UI estimates only, never converter/admission/publication policy. Rounded
 * New-mode Azahar phase times from setup-search-hints-fresh-complete-new-v1;
 * unseeded textures use the earlier completed two-attempt baseline (includes
 * partial recovery). The isolated hinted builder selects 1835 texture seconds.
 * These are NOT physical-console or Old-mode measurements. Keep learning each
 * phase independently; do not apply a universal emulator/hardware multiplier.
 * Keep the display explicitly approximate, especially on unmeasured hardware.
 */
enum {FH_INSTALL_ETA_PHASES=6,FH_INSTALL_ETA_SCALE=1000000,
      FH_INSTALL_ETA_TEXTURE_SECONDS=1835};
static const uint32_t fh_install_eta_prior_seconds[6]={323,203,560,FH_INSTALL_ETA_TEXTURE_SECONDS,1057,133};
typedef struct {
    uint32_t fraction[6],rate_ms[6],anchor_fraction;
    uint64_t started,last,anchor_time;
    unsigned active,seen,enabled,complete,sampled;
} FhInstallEta;
static inline void fh_install_eta_begin(FhInstallEta*e,uint64_t now){
    memset(e,0,sizeof(*e));
    for(unsigned i=0;i<6;i++)e->rate_ms[i]=fh_install_eta_prior_seconds[i]*1000;
    e->started=e->last=now;e->active=6;e->enabled=1;
}
static inline int fh_install_eta_update(FhInstallEta*e,unsigned phase,uint32_t done,uint32_t total,uint64_t now){
    if(!e->enabled||phase>=6||!total||done>total||now<e->last)return 0;
    e->last=now;
    uint32_t f=(uint32_t)((uint64_t)done*FH_INSTALL_ETA_SCALE/total);
    if(e->seen&&(phase<e->active||(phase==e->active&&f<e->fraction[phase])))return 0;
    for(unsigned i=0;i<phase;i++)e->fraction[i]=FH_INSTALL_ETA_SCALE;
    if(!(e->seen&(1u<<phase))){
        /* First callback may be a resumed prefix or an already finished stage.
         * Neither constitutes newly performed work for estimating throughput. */
        e->seen|=1u<<phase;e->active=phase;e->anchor_fraction=f;e->anchor_time=now;
    }else if(f>e->anchor_fraction&&now-e->anchor_time>=10000&&f-e->anchor_fraction>=2000){
        uint64_t rate=(now-e->anchor_time)*FH_INSTALL_ETA_SCALE/(f-e->anchor_fraction);
        if(rate>86400000)rate=86400000;
        e->rate_ms[phase]=(uint32_t)rate;e->sampled|=1u<<phase;
    }
    e->fraction[phase]=f;return 1;
}
static inline unsigned fh_install_eta_percent(const FhInstallEta*e){
    uint64_t done=0,total=0;
    for(unsigned i=0;i<6;i++){done+=(uint64_t)e->fraction[i]*fh_install_eta_prior_seconds[i];total+=fh_install_eta_prior_seconds[i];}
    unsigned p=(unsigned)(done*100/(total*FH_INSTALL_ETA_SCALE));
    return e->complete?100:p>99?99:p;
}
static inline uint64_t fh_install_eta_remaining(const FhInstallEta*e){
    uint64_t remaining=0;
    for(unsigned i=0;i<6;i++)remaining+=(uint64_t)e->rate_ms[i]*(FH_INSTALL_ETA_SCALE-e->fraction[i])/FH_INSTALL_ETA_SCALE;
    /* Lack of progress is not a countdown. A long stall increases the active
     * phase estimate; it never lets ETA expire while work is still pending. */
    if(e->active<6&&e->fraction[e->active]<FH_INSTALL_ETA_SCALE&&e->last>e->anchor_time){
        uint64_t spent=e->last-e->anchor_time;
        uint64_t budget=(uint64_t)e->rate_ms[e->active]*(FH_INSTALL_ETA_SCALE-e->anchor_fraction)/FH_INSTALL_ETA_SCALE;
        if(spent>budget)remaining+=spent-budget;
    }
    return remaining;
}
#endif
