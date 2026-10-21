#ifndef FH_MEMORY_BUDGET_H
#define FH_MEMORY_BUDGET_H
/* Explicit reserve in each target's linear pool; both exceed 15%. */
#include "fh_hardware.h"
#define FH_NATIVE_LINEAR_RESERVE ((fh_hardware_large_memory?4u:1u)*1024u*1024u)
#endif
