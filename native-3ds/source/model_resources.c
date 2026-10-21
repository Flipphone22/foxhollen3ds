#include "fh_model_resources.h"
#include "fh_gx_dispatch.h"
#include "fh_memory_bytes.h"
#include "fh_platform.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

enum { OWNERS=128,RANGES=1024,PAGE_WORDS=256 };
typedef struct {uintptr_t address;size_t bytes;unsigned version,escaped;} Owner;
typedef struct {uintptr_t address;size_t bytes;unsigned owner;} Range;
static Owner owners[OWNERS];
static Range ranges[RANGES];
/* Read by the optional compiler write barrier. Exact range checks still run
 * before any reader fence or generation change; hashed page hits can alias. */
uint32_t fh_model_write_pages[PAGE_WORDS];
#define pages fh_model_write_pages
static unsigned range_count,serial,exhausted;
unsigned fh_model_resources_mode;
unsigned fh_model_resources_registered,fh_model_resources_rejected,fh_model_resources_escaped,fh_model_resources_writes;
unsigned fh_model_resources_hits,fh_model_resources_checks,fh_model_resources_violations;
uint64_t fh_model_resources_reused_bytes;
const unsigned fh_model_resources_storage_bytes=sizeof(owners)+sizeof(ranges)+sizeof(pages);
_Static_assert(sizeof(owners)+sizeof(ranges)+sizeof(pages)<=32*1024,"Model resource directory budget");
static int overlaps(uintptr_t a,size_t n,uintptr_t b,size_t m){return n && m && (a>=b?a-b<m:b-a<n);}
static int contains(uintptr_t a,size_t n,uintptr_t b,size_t m){return b>=a && b-a<=n && m<=n-(b-a);}
static void fence(void){
#ifdef FH_GX_DISPATCH_BUILD
    fh_gx_dispatch_barrier();
#endif
}
static unsigned next_version(void){
    if(exhausted)return 0;
    if(serial==UINT_MAX){
        /* Never recycle a generation while an old cache token can exist. */
        memset(owners,0,sizeof(owners));range_count=0;memset(pages,0,sizeof(pages));exhausted=1;return 0;
    }
    return ++serial;
}
static int range_order(const void *a,const void *b){
    uintptr_t x=((const Range*)a)->address,y=((const Range*)b)->address;
    return (x>y)-(x<y);
}
static void rebuild_pages(void){
    memset(pages,0,sizeof(pages));
    for(unsigned i=0;i<range_count;i++){
        uintptr_t first=ranges[i].address>>12,last=(ranges[i].address+ranges[i].bytes-1)>>12;
        for(uintptr_t page=first;page<=last;page++){
            unsigned bit=page&(PAGE_WORDS*32-1);pages[bit>>5]|=1u<<(bit&31);
        }
    }
}
static void remove_owner(unsigned owner){
    unsigned out=0;
    for(unsigned i=0;i<range_count;i++)if(ranges[i].owner!=owner)ranges[out++]=ranges[i];
    range_count=out;memset(owners+owner,0,sizeof(owners[0]));
}
void fh_model_resources_register(const void *pointer,size_t bytes,const FhModelResourceRange *input,unsigned count){
    if(!fh_model_resources_mode || exhausted)return;
    fence();uintptr_t address=(uintptr_t)pointer;
    if(!address || !bytes || bytes>UINTPTR_MAX-address || !input || !count || count>260){fh_model_resources_rejected++;return;}
    for(unsigned i=0;i<OWNERS;i++)if(owners[i].escaped &&
        overlaps(address,bytes,owners[i].address,owners[i].bytes)){
        /* Re-layout inside a live allocation does not retire escaped aliases.
         * Only release of the containing allocation can restore ownership. */
        fh_model_resources_rejected++;return;
    }
    unsigned slot=OWNERS;
    for(unsigned i=0;i<OWNERS;i++)if(owners[i].address==address){
        /* An escaped pointer can write again later. Only actual release may
         * end this revocation; another registration cannot restore trust. */
        if(owners[i].escaped){fh_model_resources_rejected++;return;}
        remove_owner(i);slot=i;break;
    }
    for(unsigned i=0;i<OWNERS;i++)if(owners[i].address && overlaps(address,bytes,owners[i].address,owners[i].bytes))remove_owner(i);
    if(slot==OWNERS)for(unsigned i=0;i<OWNERS;i++)if(!owners[i].address){slot=i;break;}
    if(slot==OWNERS || count>RANGES-range_count){rebuild_pages();fh_model_resources_rejected++;return;}
    unsigned first=range_count;
    for(unsigned i=0;i<count;i++){
        uintptr_t source=(uintptr_t)input[i].data;size_t size=input[i].bytes;
        if(!size)continue;
        if(!source || !contains(address,bytes,source,size)){
            range_count=first;rebuild_pages();fh_model_resources_rejected++;return;
        }
        ranges[range_count++]=(Range){source,size,slot};
    }
    if(range_count==first){fh_model_resources_rejected++;rebuild_pages();return;}
    unsigned version=next_version();if(!version)return;
    owners[slot]=(Owner){address,bytes,version,0};
    qsort(ranges,range_count,sizeof(ranges[0]),range_order);
    /* Merge overlapping/adjacent ranges only inside one allocation owner.
     * No query may bridge a gap that has not been declared geometry. */
    unsigned out=0;
    for(unsigned i=0;i<range_count;i++){
        Range r=ranges[i];
        if(out && ranges[out-1].owner==r.owner && r.address-ranges[out-1].address<=ranges[out-1].bytes){
            size_t end=r.address-ranges[out-1].address+r.bytes;
            if(end>ranges[out-1].bytes)ranges[out-1].bytes=end;
        }else ranges[out++]=r;
    }
    range_count=out;rebuild_pages();fh_model_resources_registered++;
}
void fh_model_resources_escape(const void *pointer){
    if(!fh_model_resources_mode || !pointer)return;
    fence();uintptr_t address=(uintptr_t)pointer;
    for(unsigned i=0;i<OWNERS;i++)if(owners[i].address==address){
        if(!owners[i].escaped){owners[i].escaped=1;fh_model_resources_escaped++;}return;
    }
}
void fh_model_resources_forget(const void *pointer){
    if(!fh_model_resources_mode || !pointer)return;
    fence();uintptr_t address=(uintptr_t)pointer;
    for(unsigned i=0;i<OWNERS;i++)if(owners[i].address==address){remove_owner(i);rebuild_pages();return;}
}
void fh_model_resources_forget_allocation(const void *pointer,size_t bytes){
    if(!fh_model_resources_mode || !pointer || !bytes)return;
    fence();uintptr_t address=(uintptr_t)pointer;
    for(unsigned i=0;i<OWNERS;i++)if(owners[i].address &&
        overlaps(address,bytes,owners[i].address,owners[i].bytes))remove_owner(i);
    rebuild_pages();
}
static unsigned upper_bound(uintptr_t address);
void fh_model_resources_write_begin(const void *pointer,size_t bytes){
    if(!fh_model_resources_mode || !pointer || !bytes)return;
    uintptr_t address=(uintptr_t)pointer;
    if(bytes<=UINTPTR_MAX-address){
        unsigned at=upper_bound(address),hit=0;if(at)at--;
        /* Compiler barriers are frequent. Only genuine declared geometry
         * writes require joining the reader; unrelated writes stay local. */
        for(;at<range_count && ranges[at].address<address+bytes;at++)
            if(overlaps(address,bytes,ranges[at].address,ranges[at].bytes)){hit=1;break;}
        if(!hit)return;
    }
    fence();fh_model_resources_written(pointer,bytes);
}
/* First range whose start is greater than address. */
static unsigned upper_bound(uintptr_t address){
    unsigned low=0,high=range_count;
    while(low<high){unsigned middle=low+(high-low)/2;if(ranges[middle].address<=address)low=middle+1;else high=middle;}
    return low;
}
unsigned fh_model_resource_version(const void *pointer,size_t bytes){
    if(!fh_model_resources_mode || !pointer || !bytes || !range_count)return 0;
    uintptr_t address=(uintptr_t)pointer;unsigned bit=(address>>12)&(PAGE_WORDS*32-1);
    if(!(pages[bit>>5]&(1u<<(bit&31))))return 0;
    unsigned at=upper_bound(address);if(!at)return 0;
    Range *r=ranges+at-1;Owner *o=owners+r->owner;
    return !o->escaped && contains(r->address,r->bytes,address,bytes)?o->version:0;
}
void fh_model_resources_written(const void *pointer,size_t bytes){
    if(!fh_model_resources_mode || !range_count)return;
    if(!pointer){
        unsigned version=next_version();if(!version)return;
        for(unsigned i=0;i<OWNERS;i++)if(owners[i].address)owners[i].version=version;
        fh_model_resources_writes++;return;
    }
    uintptr_t address=(uintptr_t)pointer;
    if(!bytes)return;
    if(bytes>UINTPTR_MAX-address){fh_model_resources_written(NULL,0);return;}
    uintptr_t first=address>>12,last=(address+bytes-1)>>12;
    if(last-first<PAGE_WORDS*32){
        unsigned possible=0;
        for(uintptr_t page=first;page<=last;page++){unsigned bit=page&(PAGE_WORDS*32-1);if(pages[bit>>5]&(1u<<(bit&31))){possible=1;break;}}
        if(!possible)return;
    }
    unsigned at=upper_bound(address),version=0;if(at)at--;
    for(;at<range_count && ranges[at].address<address+bytes;at++){
        Range *r=ranges+at;
        if(!overlaps(address,bytes,r->address,r->bytes))continue;
        if(!version){version=next_version();if(!version)return;fh_model_resources_writes++;}
        owners[r->owner].version=version;
    }
}
int fh_model_resource_match(unsigned *observed,const void *data,const void *snapshot,size_t bytes){
    unsigned version=fh_model_resource_version(data,bytes);
    if(version && *observed==version){
        if(fh_model_resources_mode==2){
            fh_model_resources_checks++;
            if(fh_memory_compare(data,snapshot,bytes)){
                fh_model_resources_violations++;fh_platform_panic("Unreported model geometry write");
            }
        }
        fh_model_resources_hits++;fh_model_resources_reused_bytes+=bytes;return 1;
    }
    if(fh_memory_compare(data,snapshot,bytes)){*observed=0;return 0;}
    *observed=version;return 1;
}
