#ifndef FH_INSTALL_ASSEMBLY_H
#define FH_INSTALL_ASSEMBLY_H
#include "fh_install_transaction.h"
/* Assembly copies already durable, validated spool records. Commit each MiB
 * or the last record, not every tiny copy. Unexpected interruption may repeat
 * less than one MiB plus one record of copying; conversion is never repeated.
 * Callers force a checkpoint on orderly cancellation. No write buffering or
 * weakened final validation is introduced. Never use this for new encodes. */
static inline int fh_install_assembly_checkpoint(FhInstallTransaction*t,
    uint32_t next,uint32_t tag,uint32_t total){
    if(!t||t->failed||!t->output||next>total||t->written<t->state.partial_bytes)return 0;
    if(next==total||t->written-t->state.partial_bytes>=1024u*1024u)
        return fh_install_artifact_checkpoint(t,next,tag);
    return 1;
}
#endif
