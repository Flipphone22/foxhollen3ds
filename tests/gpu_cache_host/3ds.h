#include <stdint.h>
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;
typedef int32_t Result;
#define CUR_PROCESS_HANDLE 0
#define GX_CMDLIST_FLUSH 1
#define R_FAILED(r) ((r)<0)
u64 svcGetSystemTick(void);
Result svcFlushProcessDataCache(unsigned,u32,unsigned);
Result GSPGPU_FlushDataCache(const void*,unsigned);
