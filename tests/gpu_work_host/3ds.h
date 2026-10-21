#include <stdint.h>
#include <assert.h>
typedef uint64_t u64;
typedef uint32_t u32;
typedef int32_t Result;
typedef int LightLock;
typedef enum {GSPGPU_EVENT_PSC0,GSPGPU_EVENT_PSC1,GSPGPU_EVENT_VBlank0,
    GSPGPU_EVENT_VBlank1,GSPGPU_EVENT_PPF,GSPGPU_EVENT_P3D,GSPGPU_EVENT_DMA} GSPGPU_Event;
#define R_FAILED(r) ((r)<0)
static inline void LightLock_Lock(LightLock *l){assert(*l==1);*l=0;}
static inline void LightLock_Unlock(LightLock *l){assert(*l==0);*l=1;}
u64 svcGetSystemTick(void);
