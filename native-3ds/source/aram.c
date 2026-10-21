#include <dolphin/ar.h>
#include <string.h>
#include "fh_platform.h"
enum { ARAM_BYTES=16*1024*1024, ARAM_BASE=0x4000, PAGE_BYTES=65536, PAGES=ARAM_BYTES/PAGE_BYTES };
static unsigned char *storage;
static unsigned char *pages[PAGES];
static int initialized;
static u32 next_address=ARAM_BASE;
static ARQCallback dma_callback;
u32 ARInit(u32 *stack,u32 count) {
    (void)stack;(void)count;
    initialized=1;
    return ARAM_BASE;
}
void *ARGetStorageAddress(void) {
    if(!storage) {
        storage=fh_platform_alloc(ARAM_BYTES,32);
        if(!storage) fh_platform_panic("Contiguous ARAM view allocation failed");
        memset(storage,0,ARAM_BYTES);
        for(unsigned i=0;i<PAGES;i++) if(pages[i]) { memcpy(storage+i*PAGE_BYTES,pages[i],PAGE_BYTES);fh_platform_free(pages[i]);pages[i]=NULL; }
    }
    initialized=1;return storage;
}
u32 ARGetBaseAddress(void) { return ARAM_BASE; }
u32 ARGetSize(void) { return ARAM_BYTES; }
u32 ARGetInternalSize(void) { return ARAM_BYTES; }
BOOL ARCheckInit(void) { return initialized; }
u32 ARAlloc(u32 bytes) {
    if (bytes>ARAM_BYTES-next_address || bytes>UINT32_MAX-31) return 0;
    u32 start=next_address; next_address+=(bytes+31)&~31u; return start;
}
static void transfer(u32 type,void *memory,u32 aram_address,u32 bytes) {
    if (!initialized || aram_address>ARAM_BYTES || bytes>ARAM_BYTES-aram_address || (!memory && bytes))
        fh_platform_panic("ARAM transfer exceeds reserved memory");
    if(type>1) fh_platform_panic("Invalid ARAM transfer direction");
    unsigned char *main=memory;
    while(bytes) {
        unsigned page=aram_address/PAGE_BYTES,offset=aram_address%PAGE_BYTES,count=PAGE_BYTES-offset;
        if(count>bytes) count=bytes;
        unsigned char *data=storage?storage+page*PAGE_BYTES:pages[page];
        if(!data && type==ARAM_DIR_MRAM_TO_ARAM) {
            data=pages[page]=fh_platform_alloc(PAGE_BYTES,32);
            if(!data) fh_platform_panic("ARAM page allocation failed");
            memset(data,0,PAGE_BYTES);
        }
        if(type==ARAM_DIR_MRAM_TO_ARAM) memcpy(data+offset,main,count);
        else if(data) memcpy(main,data+offset,count);
        else memset(main,0,count);
        aram_address+=count;main+=count;bytes-=count;
    }
}
void ARStartDMA(u32 type,u32 main_address,u32 aram_address,u32 bytes) {
    transfer(type,(void*)(uintptr_t)main_address,aram_address,bytes);
    if(dma_callback) dma_callback(0);
}
ARQCallback ARRegisterDMACallback(ARQCallback callback) { ARQCallback old=dma_callback;dma_callback=callback;return old; }
u32 ARGetDMAStatus(void) { return 0; }
void ARQInit(void) { if(!initialized) ARInit(NULL,0); }
void ARQPostRequest(ARQRequest *request,u32 owner,u32 type,u32 priority,uintptr_t source,uintptr_t destination,u32 bytes,ARQCallback callback) {
    uintptr_t aram=type==ARQ_TYPE_MRAM_TO_ARAM?destination:source;
    void *main=(void*)(type==ARQ_TYPE_MRAM_TO_ARAM?source:destination);
    if (!request || aram>ARAM_BYTES || bytes>ARAM_BYTES-aram)
        fh_platform_panic("Invalid ARQ transfer");
    request->owner=owner;request->type=type;request->priority=priority;
    request->source=source;request->dest=destination;request->length=bytes;request->callback=callback;
    transfer(type,main,(u32)aram,bytes);
    if (callback) callback((uintptr_t)request);
}
