#ifndef FH_INSTALL_CURSOR_H
#define FH_INSTALL_CURSOR_H
#include "fh_install_transaction.h"
/* Startup geometry transactions may batch complete records. Flush before bank
 * changes and on orderly cancellation. Unexpected interruption repeats at most
 * batch-1 records or less than payload_bytes plus one record. Zero payload_bytes
 * keeps new records eager; zero batch keeps all cursor updates eager. */
typedef struct {unsigned batch,pending,file,index,payload_bytes;} FhInstallCursor;
static inline int fh_install_cursor_flush(FhInstallTransaction*t,FhInstallCursor*c){
    if(!t||t->failed)return 0;
    if(!c->pending)return 1;
    if(!fh_install_artifact_checkpoint(t,c->file,c->index))return 0;
    c->pending=0;return 1;
}
static inline int fh_install_cursor_boundary(FhInstallTransaction*t,FhInstallCursor*c,unsigned file){
    return !c->pending||file==c->file||fh_install_cursor_flush(t,c);
}
static inline int fh_install_cursor_record(FhInstallTransaction*t,FhInstallCursor*c,unsigned file,unsigned index){
    if(!t||t->failed||!t->output||t->written<t->state.partial_bytes)return 0;
    c->file=file;c->index=index;c->pending++;
    return c->payload_bytes&&c->batch&&c->pending<c->batch&&t->written-t->state.partial_bytes<c->payload_bytes?1:fh_install_cursor_flush(t,c);
}
static inline int fh_install_cursor_duplicate(FhInstallTransaction*t,FhInstallCursor*c,unsigned file,unsigned index){
    if(!fh_install_cursor_boundary(t,c,file))return 0;
    c->file=file;c->index=index;c->pending++;
    return c->batch&&c->pending<c->batch?1:fh_install_cursor_flush(t,c);
}
#endif
