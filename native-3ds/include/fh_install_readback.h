#ifndef FH_INSTALL_READBACK_H
#define FH_INSTALL_READBACK_H
#include "fh_install_transaction.h"
/* Sequential semantic validation through the transaction's checksum owner.
 * The callback must consume the entire file. It cannot seek or borrow FILE*. */
typedef struct FhInstallReadback FhInstallReadback;
typedef int (*FhInstallReadbackValidate)(FhInstallReadback*,void*);
uint32_t fh_install_readback_size(const FhInstallReadback*);
int fh_install_readback_read(FhInstallReadback*,void*,uint32_t);
int fh_install_artifact_finish_readback(FhInstallTransaction*,FhInstallReadbackValidate,void*);
#endif
