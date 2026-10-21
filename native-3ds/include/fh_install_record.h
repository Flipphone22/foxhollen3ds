#ifndef FH_INSTALL_RECORD_H
#define FH_INSTALL_RECORD_H
#include "fh_install_disc.h"
enum {FH_INSTALL_RECORD_CAP=4*1024*1024,FH_INSTALL_PACKED_CAP=FH_INSTALL_RECORD_CAP+65536};
typedef struct {uint32_t offset,decoded,packed,header;unsigned compressed;} FhInstallRecord;
int fh_install_record_info(FhInstallDisc*,unsigned file,uint32_t offset,FhInstallRecord*);
/* Read/decode one original ZLB/DIRn record; never load a whole model/texture
 * bank. Caller owns output. Peak transient packed scratch <=4 MiB+64 KiB.
 * Only complete source streams succeed; no silent truncation or fallback. */
int fh_install_record_decode(FhInstallDisc*,unsigned file,const FhInstallRecord*,void*,uint32_t capacity);
#endif
