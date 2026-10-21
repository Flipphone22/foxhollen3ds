#ifndef FH_INSTALL_BOOT_H
#define FH_INSTALL_BOOT_H
#include "fh_install_bundle.h"
#include "fh_startup_runner.h"
/* Before logging, DVD, game allocations or either renderer owns the screens.
 * Returns 1 only after menu Start and final readiness/binding succeeded. */
int fh_install_boot(const FhInstallBundleSpec*,const char*build,unsigned hardware_new,const FhStartupDiagnostic*,unsigned count);
#endif
