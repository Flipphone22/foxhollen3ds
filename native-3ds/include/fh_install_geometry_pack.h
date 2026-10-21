#ifndef FH_INSTALL_GEOMETRY_PACK_H
#define FH_INSTALL_GEOMETRY_PACK_H
#include "fh_install_transaction.h"
#include "fh_install_disc.h"
#include "fh_install_cursor.h"

enum {FH_INSTALL_GEOMETRY_LIMIT=4096};
typedef struct {
    FhInstallTransaction*transaction;void*entries;
    unsigned count,packets,nonrendering,duplicates;
    FhInstallCursor cursor;
} FhInstallGeometrySpool;
typedef int (*FhInstallGeometryProgress)(void*,unsigned done,unsigned total);
/* Startup only. Source is original (or separately validated reduced) geometry.
 * Recipe owns which variant this artifact contains. SHA256 disambiguates CRC
 * collisions; no completed source needs recompiling after interruption. */
int fh_install_geometry_spool_begin(FhInstallGeometrySpool*,FhInstallTransaction*,unsigned artifact);
int fh_install_geometry_spool_add(FhInstallGeometrySpool*,const void*,unsigned bytes,unsigned id,unsigned next_file,unsigned next_index);
int fh_install_geometry_spool_finish(FhInstallGeometrySpool*);
void fh_install_geometry_spool_close(FhInstallGeometrySpool*);
/* Completed spool -> existing FHNSSET1, resume at packet boundaries. Returns
 * 1 completed,0 error,-1 cancelled. Does not activate an incomplete recipe. */
int fh_install_geometry_pack(FhInstallTransaction*,unsigned spool,unsigned output,FhInstallGeometryProgress,void*);
enum {FH_INSTALL_GEOMETRY_SCAN,FH_INSTALL_GEOMETRY_ASSEMBLE};
typedef int (*FhInstallGeometryStageProgress)(void*,unsigned phase,unsigned done,unsigned total);
/* Original geometry directly from ISO. Scan progress counts index-table slots;
 * assembly progress counts packets. Callback may cancel between bounded units.
 * Does not generate reduced models or publish the whole game generation. */
int fh_install_geometry_prepare(FhInstallTransaction*,FhInstallDisc*,unsigned spool,unsigned output,FhInstallGeometryStageProgress,void*);
#endif
