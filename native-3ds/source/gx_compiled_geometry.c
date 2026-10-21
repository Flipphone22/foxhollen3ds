#include "gx_internal.h"
#include "fh_compiled_geometry.h"
#include "fh_memory_bytes.h"
#include "fh_source_identity.h"
#include "fh_model_resources.h"
#include <string.h>
/* Compile immutable packet topology, not values read from mutable GX arrays.
 * Every use verifies the complete owned display list. Attribute decoding reads
 * the current arrays synchronously, preserving morphs and buffer swaps. */
enum { CAPACITY=512*1024,ENTRIES=256,WAYS=4,HASH_SLOTS=8192 };
typedef struct {
    const void *list;unsigned bytes,stride,format,offset,size,stamp,valid,model_version;
    FhCompiledMesh mesh;
} Entry;
static Entry entries[ENTRIES];
static uint32_t hash_slots[HASH_SLOTS];
static unsigned cursor,clock;
unsigned fh_compiled_disabled,fh_compiled_hits,fh_compiled_misses,fh_compiled_unique,fh_compiled_indices;
static unsigned be16(const unsigned char *p){return (unsigned)p[0]<<8|p[1];}
void fh_compiled_reset(void){memset(entries,0,sizeof(entries));cursor=0;}
const FhCompiledMesh *fh_compiled_mesh(const void *list,unsigned bytes,unsigned stride,unsigned format){
    if(fh_compiled_disabled || !list || !bytes || bytes>65536 || !stride || stride>256)return NULL;
    unsigned char *arena=fh_geometry_compiled_storage();
    const void *identity=fh_source_identity(list,bytes);
    uintptr_t hash=(uintptr_t)identity;Entry *set=entries+(((hash>>4)^(hash>>13))&(ENTRIES/WAYS-1))*WAYS,*entry=NULL;
    ++clock;
    for(unsigned i=0;i<WAYS;i++){
        Entry *e=set+i;
        if(e->valid && e->list==identity && e->bytes==bytes && e->stride==stride && e->format==format &&
           (fh_model_resources_mode?fh_model_resource_match(&e->model_version,list,arena+e->offset,bytes):!fh_memory_compare(arena+e->offset,list,bytes))){
            e->stamp=clock;fh_compiled_hits++;return &e->mesh;
        }
        if(!entry || !e->valid || (entry->valid && e->stamp<entry->stamp))entry=e;
    }
    const unsigned char *p=list;unsigned at=0,inputs=0,expanded=0;
    while(at<bytes){
        unsigned op=p[at++];if(!op)continue;unsigned kind=op&0xf8;
        if((op&7)!=format || (kind!=GX_TRIANGLES && kind!=GX_TRIANGLESTRIP && kind!=GX_TRIANGLEFAN && kind!=GX_QUADS) || bytes-at<2)return NULL;
        unsigned n=be16(p+at);at+=2;if(n>(bytes-at)/stride)return NULL;
        inputs+=n;expanded+=kind==GX_TRIANGLES?n/3*3:kind==GX_QUADS?n/4*6:n>2?(n-2)*3:0;
        if(inputs>16384 || expanded>16384)return NULL;at+=n*stride;
    }
    if(!inputs || !expanded)return NULL;
    unsigned payload=(bytes+3)&~3u,required=(payload+inputs*4+expanded*2+3)&~3u;
    if(required>CAPACITY)return NULL;
    unsigned offset,ring=entry->size<required;
    if(entry->size>=required)offset=entry->offset;
    else {if(required>CAPACITY-cursor)cursor=0;offset=cursor;cursor+=required;}
    for(unsigned i=0;i<ENTRIES;i++)if(entries[i].size && entries[i].offset<offset+required && offset<entries[i].offset+entries[i].size)memset(entries+i,0,sizeof(*entries));
    fh_copy_bytes(arena+offset,list,bytes);
    p=arena+offset;uint32_t *offsets=(uint32_t*)(p+payload);uint16_t *indices=(uint16_t*)(offsets+inputs);
    unsigned unique=0,count=0,matrix_mask=0;at=0;memset(hash_slots,0,sizeof(hash_slots));
    while(at<bytes){
        unsigned op=p[at++];if(!op)continue;unsigned kind=op&0xf8,n=be16(p+at);at+=2;
        uint16_t previous[3]={0},first=0;
        for(unsigned i=0;i<n;i++){
            uint32_t h=2166136261u;for(unsigned j=0;j<stride;j++)h=(h^p[at+j])*16777619u;
            unsigned slot=h&(HASH_SLOTS-1),probes=0,index;
            while(hash_slots[slot] && fh_memory_compare(p+offsets[hash_slots[slot]-1],p+at,stride)){
                slot=(slot+1)&(HASH_SLOTS-1);if(++probes==HASH_SLOTS)return NULL;
            }
            if(hash_slots[slot])index=hash_slots[slot]-1;
            else {
                if(unique==HASH_SLOTS)return NULL;index=unique;offsets[unique++]=at;hash_slots[slot]=index+1;
                unsigned matrix=p[at];
                matrix_mask|=matrix<=27 && matrix%3==0?1u<<(matrix/3):0x3ffu;
            }
            if(kind==GX_TRIANGLES){previous[i%3]=index;if(i%3==2)for(unsigned j=0;j<3;j++)indices[count++]=previous[j];}
            else if(kind==GX_QUADS){if(i%4==3){indices[count++]=previous[0];indices[count++]=previous[1];indices[count++]=previous[2];indices[count++]=previous[0];indices[count++]=previous[2];indices[count++]=index;}else previous[i%4]=index;}
            else if(kind==GX_TRIANGLESTRIP){if(i>=2){indices[count++]=previous[0];indices[count++]=previous[1];indices[count++]=index;}previous[i&1]=index;}
            else {if(!i)first=index;else if(i>=2){indices[count++]=first;indices[count++]=previous[0];indices[count++]=index;}previous[0]=index;}
            at+=stride;
        }
    }
    if(count!=expanded)return NULL;
    /* The packet offsets contain one entry per distinct vertex, not per
     * expanded/input corner. Release that pessimistic reservation after the
     * build; the complete original list remains owned and byte-checked. */
    memmove(offsets+unique,indices,count*sizeof(*indices));indices=(uint16_t*)(offsets+unique);
    unsigned actual=(payload+unique*4+count*2+3)&~3u;
    if(ring)cursor=offset+actual;
    *entry=(Entry){.list=identity,.bytes=bytes,.stride=stride,.format=format,.offset=offset,.size=actual,.stamp=clock,.valid=1,
        .mesh={.packets=p,.offsets=offsets,.indices=indices,.unique=unique,.count=count,.matrix_mask=matrix_mask?matrix_mask:0x3ffu}};
    fh_compiled_misses++;return &entry->mesh;
}
