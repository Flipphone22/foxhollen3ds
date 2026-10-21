#ifndef FH_INSTALL_GEOMETRY_HINTS_H
#define FH_INSTALL_GEOMETRY_HINTS_H
#include "fh_install_model_reduce.h"
/* Optional startup prototype: hashes identify source models for which the
 * checked reducer retains the original. No vertices/topology/material bytes.
 * Not connected to the application until full output equivalence is proved. */
typedef struct {
    const unsigned char*data;
    unsigned bytes,count,ratio_bits,error_bits;
} FhInstallGeometryHints;
int fh_install_geometry_hints_open(FhInstallGeometryHints*,const void*,unsigned,
    const unsigned char blob_sha[32],const unsigned char disc_sha[32],const unsigned char contract_sha[32]);
int fh_install_geometry_hints_find(const FhInstallGeometryHints*,unsigned id,unsigned bytes,
    const unsigned char source_sha[32],float ratio,float error);
/* The caller must supply the SHA256 it just computed from this exact source,
 * as the performance spool owner already does before reduction/deduplication.
 * Missing hints use the existing reducer. The table storage stays borrowed. */
int fh_install_model_reduce_hinted(const FhInstallGeometryHints*,FhInstallModelReduction*,
    const void*,unsigned bytes,unsigned id,const unsigned char source_sha[32],float ratio,float error,
    FhInstallModelReduceProgress,void*);
#endif
