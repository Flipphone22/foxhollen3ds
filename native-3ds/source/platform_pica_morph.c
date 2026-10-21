/* Only original fixed-point morph results cross this boundary. GPU skinning,
 * topology, materials and other resident attributes retain their ownership. */
#include <3ds.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_morph.h"
#include "fh_gpu_cache.h"
#include "fh_minimal_graphics.h"
#include "fh_scene_storage.h"
#include "fh_memory_budget.h"
#include "fh_platform.h"
#include "fh_graphics_options.h"
enum {CAPACITY=64,SLOTS=3,STORAGE_LANES=SLOTS+4,CPU_LIMIT=256*1024};
/* New's measured 24 MiB linear reservation can admit larger transition sets.
 * This is an allocation ceiling, not a reservation. Every allocation must
 * still retain the global linear reserve AND 15% of the actual linear heap.
 * Smaller grants keep the existing ceiling, regardless of hardware label. */
static unsigned morph_linear_limit(void){
    extern u32 __ctru_linear_heap_size;
    return __ctru_linear_heap_size>=24u*1024*1024?2u*1024*1024:1024u*1024;
}
#define LINEAR_LIMIT (morph_linear_limit())
/* Asset geometry is immutable for the lifetime of its morph owners. The exact
 * source mask keys its packed group layout; never share animated positions. */
typedef struct {
    const FhSceneAsset *asset;void *meta,*gpu;
    unsigned sources,mask_bytes,bytes,refs;
} MorphAttributes;
static MorphAttributes attributes[CAPACITY];
typedef struct {
    MorphAttributes *attributes;
    const void *owner,*file,*poses[2];const FhSceneAsset *asset;
    FhPicaMorphSize size;FhPicaMorphGroup *groups;FhPicaMorphWrite *writes;
    unsigned *offsets;void *meta,*gpu;unsigned cpu_bytes,slot,used[SLOTS],active,pending;
} Morph;
static Morph records[CAPACITY];
static unsigned high,linear_bytes,cpu_bytes,linear_peak,cpu_peak,attached,updated,selected,vertices;
static u64 update_ticks,flushed;
static unsigned morph_meta_extent(unsigned bytes){return (bytes+127u)&~127u;}
static int morph_storage_room(unsigned bytes){
    extern u32 __ctru_linear_heap_size;
    unsigned available=linearSpaceFree();
    return bytes<=available && available-bytes>=FH_NATIVE_LINEAR_RESERVE &&
        (u64)(available-bytes)*100>=(u64)__ctru_linear_heap_size*15;
}
static int morph_storage_allocate(unsigned normal,unsigned gpu,void **meta,void **vertices){
    *meta=*vertices=NULL;
    if(normal>CPU_LIMIT || gpu>LINEAR_LIMIT)return 0;
    unsigned offset=morph_meta_extent(normal),bytes=offset+gpu;
    if(!bytes || !morph_storage_room(bytes+128))return 0;
    void *p=linearMemAlign(bytes,128);if(!p)return 0;
    if(!morph_storage_room(0)){linearFree(p);return 0;}
    *meta=p;*vertices=(char*)p+offset;return 1;
}
static void publish_positions(Morph *m){
    void *dest=(char*)m->gpu+m->slot*m->size.bytes;
    for(unsigned j=0;j<m->size.groups;j++){
        FhPicaMorphGroup *g=m->groups+j;
        fh_gpu_cache_flush((char*)dest+g->offset,g->vertices*8);flushed+=g->vertices*8;
    }
    m->pending=0;
}
unsigned fh_pica_morph_declined;
void fh_pica_morph_attach(const void *owner,const void *file,const void *p0,const void *p1,const uint8_t *mask,unsigned sources){
    const FhSceneAsset *a=fh_native_scene_find(file);if(!a)return; /* animation-only resource */
    FhPicaMorphSize s;
    if(!fh_pica_morph_compile(a,mask,sources,&s,NULL,0,NULL,0))fh_platform_panic("Native morph plan admission");
    if(!s.groups)return;
    unsigned id=high;
    for(unsigned i=0;i<high;i++){
        if(records[i].owner==owner)fh_platform_panic("Native morph duplicate owner");
        if(!records[i].owner && id==high)id=i;
    }
    if(id==CAPACITY){fh_pica_morph_declined++;return;}
    MorphAttributes *shared=NULL,*empty=NULL;
    unsigned mask_bytes=(sources+7u)/8u;
    for(unsigned i=0;i<CAPACITY;i++){
        MorphAttributes *p=attributes+i;
        if(!p->refs){if(!empty)empty=p;continue;}
        if(p->asset==a && p->sources==sources && p->bytes==s.bytes*4 &&
           !memcmp(p->meta,mask,mask_bytes)){shared=p;break;}
    }
    if(!shared && !empty){fh_pica_morph_declined++;return;}
    unsigned normal=s.groups*sizeof(FhPicaMorphGroup)+s.writes*sizeof(FhPicaMorphWrite)+a->groups*sizeof(unsigned);
    extern u32 __ctru_heap_size,__stacksize__;
    unsigned extra_cpu=shared?0:mask_bytes;
    unsigned lanes=SLOTS+(shared?0:4);
    if(normal>CPU_LIMIT-cpu_bytes || extra_cpu>CPU_LIMIT-cpu_bytes-normal ||
       s.bytes>LINEAR_LIMIT/lanes || s.bytes*lanes>LINEAR_LIMIT-linear_bytes ||
       !morph_storage_room(morph_meta_extent(normal)+s.bytes*SLOTS+128+
           (shared?0:morph_meta_extent(mask_bytes)+s.bytes*4+128))){
        char message[240];snprintf(message,sizeof message,
            "Native morph budget: asset=%u request=%u cpu=%u gpu_request=%u gpu_used=%u heap=%u stack=%u grant=%u linear_free=%u morph_limit=%u\n",
            a->id,normal+extra_cpu,cpu_bytes,s.bytes*lanes,linear_bytes,
            (unsigned)mallinfo().uordblks,(unsigned)__stacksize__,(unsigned)__ctru_heap_size,(unsigned)linearSpaceFree(),LINEAR_LIMIT);
        fh_platform_log(message);fh_pica_morph_declined++;return;
    }
    Morph m={.owner=owner,.file=file,.poses={p0,p1},.asset=a,.size=s,.cpu_bytes=normal};
    if(!morph_storage_allocate(normal,s.bytes*SLOTS,&m.meta,&m.gpu)){fh_pica_morph_declined++;return;}
    m.groups=m.meta;m.writes=(FhPicaMorphWrite*)(m.groups+s.groups);m.offsets=(unsigned*)(m.writes+s.writes);
    if(!fh_pica_morph_compile(a,mask,sources,&s,m.groups,s.groups,m.writes,s.writes))fh_platform_panic("Native morph plan publication");
    for(unsigned i=0;i<a->groups;i++)m.offsets[i]=~0u;
    for(unsigned i=0;i<s.groups;i++)m.offsets[m.groups[i].group]=m.groups[i].offset;
    if(!shared){
        void *meta,*gpu;
        if(!morph_storage_allocate(mask_bytes,s.bytes*4,&meta,&gpu)){
            linearFree(m.meta);fh_pica_morph_declined++;return;
        }
        memcpy(meta,mask,mask_bytes);memset(gpu,0,s.bytes*4);
        fh_pica_morph_pack_attributes(gpu,a,m.groups,s.groups);
        fh_gpu_cache_flush(gpu,s.bytes*4);
        shared=empty;
        *shared=(MorphAttributes){.asset=a,.meta=meta,.gpu=gpu,.sources=sources,
            .mask_bytes=mask_bytes,.bytes=s.bytes*4};
        linear_bytes+=shared->bytes;cpu_bytes+=mask_bytes;
    }
    shared->refs++;m.attributes=shared;
    memset(m.gpu,0,s.bytes*SLOTS);
    for(unsigned i=0;i<SLOTS;i++)fh_pica_morph_initialize((char*)m.gpu+i*s.bytes,a,m.groups,s.groups);
    fh_gpu_cache_flush(m.gpu,s.bytes*SLOTS);
    records[id]=m;if(id==high)high++;attached++;
    linear_bytes+=s.bytes*SLOTS;cpu_bytes+=normal;
    if(linear_bytes>linear_peak)linear_peak=linear_bytes;
    if(cpu_bytes>cpu_peak)cpu_peak=cpu_bytes;
}
void fh_pica_morph_changed(const void *owner,const unsigned char *positions){
    for(unsigned i=0;i<high;i++)if(records[i].owner==owner){
        Morph *m=records+i;
        /* The source draws base vertices when no channel produced a stream.
         * Do not keep displaying the last expression after its reset. */
        m->active=positions!=NULL;if(!positions)return;
        u64 start=svcGetSystemTick();unsigned slot=m->pending?m->slot:(m->slot+1)%SLOTS;
        /* Retire only the slot about to be overwritten; same-frame multiple
         * source updates never overwrite a pending draw or another instance. */
        if(!m->pending)fh_minimal_prepare_write(m->used[slot]);
        void *dest=(char*)m->gpu+slot*m->size.bytes;
        fh_pica_morph_apply(dest,positions,m->groups,m->size.groups,m->writes);
        m->slot=slot;m->used[slot]=0;m->pending=1;
        /* Preserve the latest expression before collision skinning overwrites
         * the source buffer. While hidden, reuse this CPU-owned slot: no new
         * allocation, repeated GPU retirement, or cache publication. */
        if(!(fh_graphics_options&(FH_GRAPHICS_NO_MORPHS|FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS)))publish_positions(m);
        updated++;vertices+=m->size.writes;
        update_ticks+=svcGetSystemTick()-start;return;
    }
}
const void *fh_pica_morph_view(const FhSceneAsset *a,const float *poses){
    if(fh_graphics_options&(FH_GRAPHICS_NO_MORPHS|FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS))return NULL;
    for(unsigned i=0;i<high;i++){
        Morph *m=records+i;
        if(m->owner && m->active && m->asset==a && (m->poses[0]==poses || m->poses[1]==poses)){
            if(m->pending)publish_positions(m);
            return m;
        }
    }
    return NULL;
}
const void *fh_pica_morph_positions(const void *view,unsigned group){
    Morph *m=(Morph*)view;if(!m || m->offsets[group]==~0u)return NULL;
    m->used[m->slot]=fh_minimal_frames+1;selected++;
    return (char*)m->gpu+m->slot*m->size.bytes+m->offsets[group];
}
const void *fh_pica_morph_attributes(const void *view,unsigned group){
    const Morph *m=view;if(!m || m->offsets[group]==~0u)return NULL;
    return (char*)m->attributes->gpu+m->offsets[group]*4;
}
static void release(Morph *m){
    linear_bytes-=m->size.bytes*SLOTS;cpu_bytes-=m->cpu_bytes;
    MorphAttributes *shared=m->attributes;
    /* Caller has retired all readers before releasing either allocation. */
    if(!--shared->refs){
        linear_bytes-=shared->bytes;cpu_bytes-=shared->mask_bytes;
        linearFree(shared->meta);memset(shared,0,sizeof(*shared));
    }
    linearFree(m->meta);memset(m,0,sizeof(*m));
}
void fh_pica_morph_forget(const void *allocation,unsigned bytes){
    uintptr_t base=(uintptr_t)allocation;unsigned joined=0;
    for(unsigned i=0;i<high;i++){
        Morph *m=records+i;uintptr_t owner=(uintptr_t)m->owner,file=(uintptr_t)m->file;
        if(m->owner && ((owner>=base && owner-base<bytes)||(file>=base && file-base<bytes))){
            if(!joined){fh_renderer_scene_storage_join();joined=1;}
            release(m);
        }
    }
    while(high && !records[high-1].owner)high--;
}
void fh_pica_morph_shutdown(void){
    /* Host already retired all commands. Never reopen its frame on shutdown. */
    for(unsigned i=0;i<high;i++)if(records[i].owner)release(records+i);
    high=0;
    char line[256];snprintf(line,sizeof(line),"PICA morph: instances=%u updates=%u groups=%u positions=%u linear_peak=%u cpu_peak=%u flush_bytes=%llu update_ticks=%llu\n",
        attached,updated,selected,vertices,linear_peak,cpu_peak,(unsigned long long)flushed,(unsigned long long)update_ticks);fh_platform_log(line);
}
