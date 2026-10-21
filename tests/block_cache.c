#include <assert.h>
#include "fh_block_cache.h"
int main(int argc,char **argv){
    assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));unsigned raw_bytes=(unsigned)ftell(f);rewind(f);
    unsigned char *raw=malloc(raw_bytes);assert(raw && fread(raw,1,raw_bytes,f)==raw_bytes);fclose(f);
    FhBlockCache c={0};assert(fh_block_cache_open(&c,argv[2],8*1024*1024,raw_bytes));
    unsigned cost=c.storage_bytes;fh_block_cache_close(&c);
    assert(!fh_block_cache_open(&c,argv[2],cost-1,raw_bytes));assert(!c.storage);
    assert(!fh_block_cache_open(&c,argv[2],cost,raw_bytes+1));
    assert(fh_block_cache_open(&c,argv[2],cost,raw_bytes));
    unsigned char *out=malloc(raw_bytes+32);assert(out);memset(out,0xa7,raw_bytes+32);
    assert(fh_block_cache_read(&c,0,out+16,raw_bytes));assert(!memcmp(raw,out+16,raw_bytes));
    for(unsigned i=0;i<16;i++)assert(out[i]==0xa7 && out[raw_bytes+16+i]==0xa7);
    unsigned random=173;
    for(unsigned i=0;i<4000;i++){
        random=random*1664525+1013904223;unsigned at=random%raw_bytes;
        random=random*1664525+1013904223;unsigned n=random%100000;
        if(n>raw_bytes-at)n=raw_bytes-at;
        assert(fh_block_cache_read(&c,at,out,n));assert(!memcmp(raw+at,out,n));
    }
    assert(fh_block_cache_read(&c,raw_bytes,NULL,0));
    assert(!fh_block_cache_read(&c,raw_bytes,out,1));
    assert(!fh_block_cache_read(&c,UINT32_MAX,out,2));
    assert(!fh_block_cache_read(&c,0,out,UINT32_MAX));
    assert(!fh_block_cache_read(&c,0,NULL,1));
    fh_block_cache_close(&c);assert(!c.storage && !c.storage_bytes);
    free(raw);free(out);puts("block cache: 4000 random reads, complete bytes, guards and budget checks passed");
}
