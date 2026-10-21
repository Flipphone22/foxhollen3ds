#ifndef FH_INSTALL_GEOMETRY_ISO_H
#define FH_INSTALL_GEOMETRY_ISO_H
#include "fh_install_record.h"
enum {FH_INSTALL_GEO_BANKS=256,FH_INSTALL_GEO_TABLE_CAP=65536};
typedef struct {unsigned file,table,world;} FhInstallGeometryBank;
typedef struct {
    FhInstallDisc*disc;FhInstallGeometryBank banks[FH_INSTALL_GEO_BANKS];
    unsigned banks_count,bank,index,entries,loaded;
    /* Index-table slots, including duplicates/unused slots; not time estimates.
     * Computed from tables already inspected at begin, with no extra reads. */
    unsigned total_entries,scanned_entries;
    unsigned char *table,*decoded;
    unsigned records,skipped,unsupported;
    char error[112];
} FhInstallGeometryScan;
typedef struct {unsigned id,file,offset,bytes,next_file,next_index;const unsigned char*data;} FhInstallGeometrySource;
enum {FH_INSTALL_GEO_ERROR=-1,FH_INSTALL_GEO_DONE=0,FH_INSTALL_GEO_SOURCE=1,FH_INSTALL_GEO_SKIP=2};
/* Owns table/record storage, exclusively borrows the ISO cursor; no extraction.
 * Resume cursor is FST bank entry + next table index, not an arbitrary byte.
 * Returned source bytes remain valid until next/close. Zero initialize. */
int fh_install_geometry_scan_begin(FhInstallGeometryScan*,FhInstallDisc*,unsigned file,unsigned index);
int fh_install_geometry_scan_next(FhInstallGeometryScan*,FhInstallGeometrySource*);
void fh_install_geometry_scan_close(FhInstallGeometryScan*);
#endif
