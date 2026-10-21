#include <dolphin/os.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "fh_platform.h"
#include "fh_game_memory.h"

#include "fh_hardware.h"
#define ARENA_BYTES ((fh_hardware_large_memory?26u:19u)*1024u*1024u)
enum { HEAPS=8, ALIGNMENT=32 };
unsigned fh_game_region_reserve(void) {
    return fh_hardware_large_memory?0xd80000:0x780000;
}
unsigned fh_game_region_size(unsigned region) {
    static const unsigned small[]={0,0x180000,0x7ff40,0x57ff40};
    static const unsigned large[]={0,0x380000,0x13ff40,0x8bff40};
    const unsigned* sizes=fh_hardware_large_memory?large:small;
    if(region<1 || region>3)fh_platform_panic("Invalid game region budget");
    return sizes[region];
}
typedef struct Block {
    size_t bytes;
    struct Block *next;
    uint32_t magic;
    int32_t owner;
    uint32_t reserved[2];
} __attribute__((aligned(32))) Block;
_Static_assert(sizeof(Block)==32,"Heap header must occupy one cache line");
typedef struct { uintptr_t begin,end; Block *free; int active; } Heap;
static Heap heaps[HEAPS];
static unsigned heap_limit;
static uint8_t *arena,*arena_low,*arena_high;
static size_t used,peak;
uintptr_t OSBaseAddress;
volatile OSHeapHandle __OSCurrHeap=-1;

static uintptr_t align_up(uintptr_t value, size_t alignment) {
    if (!alignment || (alignment&(alignment-1)) || value>UINTPTR_MAX-(alignment-1))
        fh_platform_panic("Invalid arena alignment");
    return (value+alignment-1)&~(uintptr_t)(alignment-1);
}
static Heap *heap_at(int handle) {
    if (handle<0 || (unsigned)handle>=heap_limit || !heaps[handle].active)
        fh_platform_panic("Invalid OS heap handle");
    return &heaps[handle];
}
void OSInit(void) {
    if (arena) return;
    arena=fh_platform_alloc(ARENA_BYTES,ALIGNMENT);
    if (!arena) fh_platform_panic("Game arena allocation failed");
    memset(arena,0,0x4000);
    OSBaseAddress=(uintptr_t)arena;
    *(uint32_t*)(arena+0xf8)=162000000;
    *(uint32_t*)(arena+0xfc)=486000000;
    *(uint32_t*)(arena+0x28)=ARENA_BYTES;
    *(uint32_t*)(arena+0xf0)=ARENA_BYTES;
    arena_low=arena+0x4000;
    arena_high=arena+ARENA_BYTES;
}
void *OSGetArenaLo(void) { OSInit(); return arena_low; }
void *OSGetArenaHi(void) { OSInit(); return arena_high; }
void OSSetArenaLo(void *value) {
    OSInit();
    if ((uintptr_t)value<(uintptr_t)arena || (uintptr_t)value>(uintptr_t)arena_high)
        fh_platform_panic("OSSetArenaLo outside arena");
    arena_low=value;
}
void OSSetArenaHi(void *value) {
    OSInit();
    if ((uintptr_t)value<(uintptr_t)arena_low || (uintptr_t)value>(uintptr_t)arena+ARENA_BYTES)
        fh_platform_panic("OSSetArenaHi outside arena");
    arena_high=value;
}
void *OSAllocFromArenaLo(u32 bytes,u32 alignment) {
    OSInit();
    uintptr_t start=align_up((uintptr_t)arena_low,alignment);
    if (start>(uintptr_t)arena_high || bytes>(uintptr_t)arena_high-start) return NULL;
    uintptr_t end=align_up(start+bytes,alignment);
    if (end>(uintptr_t)arena_high) return NULL;
    arena_low=(uint8_t*)end;
    return (void*)start;
}
void *OSAllocFromArenaHi(u32 bytes,u32 alignment) {
    OSInit();
    align_up(0,alignment);
    if (bytes>(size_t)(arena_high-arena_low)) return NULL;
    uintptr_t start=((uintptr_t)arena_high-bytes)&~(uintptr_t)(alignment-1);
    if (start<(uintptr_t)arena_low) return NULL;
    arena_high=(uint8_t*)start;
    return arena_high;
}
u32 OSGetPhysicalMemSize(void) { return ARENA_BYTES; }
u32 OSGetConsoleSimulatedMemSize(void) { return ARENA_BYTES; }
u32 OSGetConsoleType(void) { return 0x10000002; }
void *OSInitAlloc(void *start,void *end,int count) {
    if (count<1 || count>HEAPS || (uintptr_t)start>=(uintptr_t)end || used)
        fh_platform_panic("Invalid OS heap initialization");
    memset(heaps,0,sizeof(heaps));
    heap_limit=count; __OSCurrHeap=-1;
    return (void*)align_up((uintptr_t)start,ALIGNMENT);
}
OSHeapHandle OSCreateHeap(void *start,void *end) {
    uintptr_t begin=align_up((uintptr_t)start,ALIGNMENT);
    uintptr_t finish=(uintptr_t)end&~(uintptr_t)(ALIGNMENT-1);
    if (finish<=begin || finish-begin<64) return -1;
    if (begin<(uintptr_t)arena || finish>(uintptr_t)arena+ARENA_BYTES) return -1;
    for (unsigned i=0;i<heap_limit;++i)
        if (heaps[i].active && begin<heaps[i].end && finish>heaps[i].begin) return -1;
    for (unsigned i=0;i<heap_limit;++i) if (!heaps[i].active) {
        heaps[i]=(Heap){begin,finish,(Block*)begin,1};
        *heaps[i].free=(Block){.bytes=finish-begin,.owner=(int)i};
        return (int)i;
    }
    return -1;
}
OSHeapHandle OSSetCurrentHeap(OSHeapHandle handle) {
    heap_at(handle);
    int old=__OSCurrHeap; __OSCurrHeap=handle; return old;
}
void *OSAllocFromHeap(OSHeapHandle handle,u32 bytes) {
    Heap *heap=heap_at(handle);
    if (!bytes || bytes>UINT32_MAX-63) return NULL;
    size_t need=align_up((size_t)bytes+sizeof(Block),ALIGNMENT);
    for (Block **link=&heap->free;*link;link=&(*link)->next) {
        Block *block=*link;
        if (block->bytes<need) continue;
        if (block->bytes-need>=64) {
            Block *tail=(Block*)((uint8_t*)block+need);
            *tail=(Block){.bytes=block->bytes-need,.next=block->next,.owner=handle};
            *link=tail; block->bytes=need;
        } else *link=block->next;
        block->magic=0x46484850; block->owner=handle; block->next=NULL;
        used+=block->bytes; if (used>peak) peak=used;
        return block+1;
    }
    return NULL;
}
void OSFreeToHeap(OSHeapHandle handle,void *memory) {
    if (!memory) return;
    Heap *heap=heap_at(handle);
    uintptr_t value=(uintptr_t)memory;
    if (value<heap->begin+sizeof(Block) || value>=heap->end || value%ALIGNMENT)
        fh_platform_panic("Invalid OS heap free address");
    Block *block=(Block*)memory-1;
    if (block->magic!=0x46484850 || block->owner!=handle || block->bytes>heap->end-(uintptr_t)block)
        fh_platform_panic("Invalid OS heap free header");
    used-=block->bytes; block->magic=0;
    Block **link=&heap->free;
    while (*link && (uintptr_t)*link<(uintptr_t)block) link=&(*link)->next;
    block->next=*link; *link=block;
    for (Block *current=heap->free;current && current->next;) {
        if ((uintptr_t)current+current->bytes==(uintptr_t)current->next) {
            current->bytes+=current->next->bytes; current->next=current->next->next;
        } else current=current->next;
    }
}
s32 OSCheckHeap(OSHeapHandle handle) {
    Heap *heap=heap_at(handle); size_t available=0;
    for (Block *block=heap->free;block;block=block->next) available+=block->bytes-sizeof(Block);
    return (s32)available;
}
u32 OSReferentSize(void *memory) {
    Block *block=(Block*)memory-1;
    if (block->magic!=0x46484850) fh_platform_panic("Invalid OS allocation query");
    return block->bytes-sizeof(Block);
}
void OSDestroyHeap(OSHeapHandle handle) {
    Heap *heap=heap_at(handle);
    if (!heap->free || heap->free->bytes!=heap->end-heap->begin || heap->free->next)
        fh_platform_panic("Destroying nonempty OS heap");
    memset(heap,0,sizeof(*heap));
}
size_t fh_memory_peak(void) { return peak; }
size_t fh_memory_used(void) { return used; }
void fh_memory_shutdown(void) {
    fh_platform_free(arena); arena=arena_low=arena_high=NULL;
    OSBaseAddress=0; used=peak=heap_limit=0; __OSCurrHeap=-1;
    memset(heaps,0,sizeof(heaps));
}
