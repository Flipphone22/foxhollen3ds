#include "fh_resource_tables.h"
#include <string.h>
enum { TABLES=7 };
typedef struct {const void *dst,*a,*b;} Entry;
static Entry entries[TABLES];
unsigned fh_resource_merge_hits,fh_resource_merge_misses;
void fh_resource_tables_invalidate(void){memset(entries,0,sizeof(entries));}
int fh_resource_merge_hit(const void *dst,const void *a,const void *b,int busy){
    if(busy)fh_resource_tables_invalidate();
    if(!busy && dst)for(unsigned i=0;i<TABLES;i++)
        if(entries[i].dst==dst && entries[i].a==a && entries[i].b==b){
            fh_resource_merge_hits++;return 1;
        }
    fh_resource_merge_misses++;return 0;
}
void fh_resource_merge_publish(const void *dst,const void *a,const void *b,int busy){
    if(!dst || busy)return;
    unsigned slot=TABLES;
    for(unsigned i=0;i<TABLES;i++){
        if(entries[i].dst==dst){slot=i;break;}
        if(!entries[i].dst && slot==TABLES)slot=i;
    }
    /* Unknown additional output tables get no cache entry, not an eviction
     * that could alter an output or extend any source's ownership. */
    if(slot<TABLES)entries[slot]=(Entry){dst,a,b};
}
