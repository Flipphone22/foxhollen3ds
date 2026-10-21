#ifndef FH_INSTALL_PERFORMANCE_PACK_H
#define FH_INSTALL_PERFORMANCE_PACK_H
#include "fh_install_transaction.h"
#include "fh_install_model_reduce.h"
#include "fh_install_disc.h"
#include "fh_install_cursor.h"
typedef struct {
    FhInstallTransaction*transaction;void*entries;
    unsigned count,patches,packets,nonrendering,duplicates,before,after;
    FhInstallCursor cursor;
} FhInstallPerformanceSpool;
enum {FH_INSTALL_PERFORMANCE_TOPOLOGY,FH_INSTALL_PERFORMANCE_GEOMETRY};
typedef int (*FhInstallPerformanceProgress)(void*,unsigned done,unsigned total);
/* One completed original source per checkpoint: source SHA, admitted topology
 * and compiled native geometry. The geometry key uses the PATCHED source CRC.
 * Recipe key owns ratio/error and fidelity policy. No active-game publication. */
int fh_install_performance_spool_begin(FhInstallPerformanceSpool*,FhInstallTransaction*,unsigned artifact);
int fh_install_performance_spool_add(FhInstallPerformanceSpool*,const void*,unsigned bytes,unsigned id,unsigned next_file,unsigned next_index,float ratio,float error,FhInstallModelReduceProgress,void*);
int fh_install_performance_spool_finish(FhInstallPerformanceSpool*);
void fh_install_performance_spool_close(FhInstallPerformanceSpool*);
/* Completed spool -> existing FHMLPK1 or FHNSSET1. Packet-boundary resume.
 * Returns1 complete,0 error,-1 cancelled. Empty topology is valid; empty
 * geometry is not. The caller still owns whole-recipe publication. */
int fh_install_performance_pack(FhInstallTransaction*,unsigned spool,unsigned output,unsigned kind,FhInstallPerformanceProgress,void*);
enum {FH_INSTALL_PERF_SCAN,FH_INSTALL_PERF_REDUCE,FH_INSTALL_PERF_TOPOLOGY_PACK,FH_INSTALL_PERF_GEOMETRY_PACK};
typedef int (*FhInstallPerformanceStageProgress)(void*,unsigned phase,unsigned done,unsigned total);
/* ISO -> paired admitted topology/native geometry. Three recipe-owned artifact
 * IDs in one stage transaction. Scan counts index-table slots, reduction counts current
 * model lists, pack phases count output entries. No whole-game activation. */
int fh_install_performance_prepare(FhInstallTransaction*,FhInstallDisc*,unsigned spool,unsigned topology,unsigned geometry,float ratio,float error,FhInstallPerformanceStageProgress,void*);
#endif
