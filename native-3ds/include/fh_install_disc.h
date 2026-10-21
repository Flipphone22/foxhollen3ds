#ifndef FH_INSTALL_DISC_H
#define FH_INSTALL_DISC_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#define FH_INSTALL_DISC_PATH "sdmc:/3ds/foxhollow/disc.iso"
/* The installer owns this reader before game/renderer/async DVD startup.
 * No borrowed FILE, shared cursor, extraction paths or GPU allocations. */
enum {FH_INSTALL_FST_CAP=2*1024*1024,FH_INSTALL_ENTRY_CAP=8192,
      FH_INSTALL_READ_CHUNK=64*1024,FH_INSTALL_PATH_CAP=1024};
typedef struct {
    FILE *file;
    unsigned char *fst;
    uint32_t bytes,fst_bytes,entries,fst_crc;
    uint32_t parent[FH_INSTALL_ENTRY_CAP];
    char error[112];
} FhInstallDisc;
typedef struct {uint32_t offset,bytes;unsigned directory;} FhInstallEntry;
/* Zero-initialize before first open; close retains a diagnostic error. */
int fh_install_disc_open(FhInstallDisc*,const char*);
void fh_install_disc_close(FhInstallDisc*);
int fh_install_disc_entry(const FhInstallDisc*,unsigned,FhInstallEntry*);
int fh_install_disc_path(const FhInstallDisc*,unsigned,char*,size_t);
int fh_install_disc_find(const FhInstallDisc*,const char*);
int fh_install_disc_read(FhInstallDisc*,unsigned,uint32_t,void*,uint32_t);
#endif
