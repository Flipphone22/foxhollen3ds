#include <assert.h>
#include "fh_block_cache.h"
int main(int argc,char **argv){
    assert(argc==3 || argc==4);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));unsigned size=ftell(f);rewind(f);
    unsigned char *raw=malloc(size),*out=malloc(size+32);assert(raw && out);
    assert(fread(raw,1,size,f)==size);fclose(f);
    FhBlockCache c={0};unsigned budget=32+((size+65535)/65536)*20+3*65536;
    assert(!fh_block_cache_open_window(&c,argv[2],budget-1,size));assert(!c.storage);
    if(argc==4)budget=(unsigned)strtoul(argv[3],NULL,10);
    assert(fh_block_cache_open_window(&c,argv[2],budget,size));assert(c.storage_bytes==budget && c.backing);
    memset(out,0xa7,size+32);
    assert(fh_block_cache_read(&c,0,out+16,size));assert(!memcmp(raw,out+16,size));
    for(unsigned i=0;i<16;i++)assert(out[i]==0xa7 && out[size+16+i]==0xa7);
    unsigned random=1249;
    for(unsigned i=0;i<4000;i++){
        random=random*1664525+1013904223;unsigned at=random%size;
        random=random*1664525+1013904223;unsigned n=random%100000;if(n>size-at)n=size-at;
        assert(fh_block_cache_read(&c,at,out,n));assert(!memcmp(out,raw+at,n));
    }
    assert(c.sd_reads && c.evictions);
    assert(fh_block_cache_read(&c,size,NULL,0));assert(!fh_block_cache_read(&c,size,out,1));
    assert(!fh_block_cache_read(&c,UINT32_MAX,out,1));
    printf("Window cache: 4000 random reads, full bytes, bounded allocation=%u; misses=%llu evictions=%llu\n",
        c.storage_bytes,(unsigned long long)c.sd_reads,(unsigned long long)c.evictions);
    /* Corrupted resident bytes cannot publish a decoded block. */
    assert(fh_block_cache_read(&c,0,out,1));c.decoded=UINT32_MAX;
    unsigned resident=c.resident[0];assert(resident);
    memset(c.storage+c.arena_start+resident-1,0,fh_block_word(c.storage+36));
    assert(!fh_block_cache_read(&c,0,out,65536));
    fh_block_cache_close(&c);assert(!c.storage && !c.backing && !c.storage_bytes);
    free(raw);free(out);return 0;
}
