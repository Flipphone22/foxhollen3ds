#include "fh_block_cache.h"
#include <stdint.h>
typedef uint64_t u64;
static u64 svcGetSystemTick(void){static u64 t;return ++t;}
