#ifndef FH_BLOCK_CACHE_H
#define FH_BLOCK_CACHE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "fh_lz4_block.h"
#include "fh_asset_staging.h"
/* Optional immutable, CPU-only prepared-file cache. Independent 64 KiB LZ4
 * blocks allow bounded random reads without retaining the uncompressed file.
 * Opening verifies every decoded block. Full residency performs all SD reads
 * then; windowed residency retains a private file for measured cache misses.
 * No worker, GPU storage or game source pointer is involved. */
enum { FH_BLOCK_CACHE_CHUNK=65536 };
typedef struct {
    unsigned char *storage,*scratch;
    unsigned storage_bytes,file_bytes,raw_bytes,blocks,decoded;
    uint64_t reads,bytes,decodes;
    /* Large complete packages use a bounded compressed-byte working set.
     * Full-resident packages retain the original no-filesystem read path. */
    FILE *backing;
    unsigned *resident,arena_start,arena_bytes,arena_at;
    unsigned char *input;
    uint64_t sd_reads,sd_bytes,evictions;
} FhBlockCache;
static inline unsigned fh_block_word(const unsigned char *p){
    return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;
}
static inline void fh_block_cache_close(FhBlockCache *c){
    if(c->backing)fclose(c->backing);
    free(c->storage);memset(c,0,sizeof(*c));
}
static inline int fh_block_cache_decode(FhBlockCache *c,unsigned block){
    if(block>=c->blocks)return 0;
    if(c->decoded==block)return 1;
    const unsigned char *e=c->storage+32+block*16;
    unsigned at=fh_block_word(e),size=fh_block_word(e+4);
    unsigned n=c->raw_bytes-block*FH_BLOCK_CACHE_CHUNK;
    if(n>FH_BLOCK_CACHE_CHUNK)n=FH_BLOCK_CACHE_CHUNK;
    const unsigned char *encoded;
    if(c->backing){
        if(!c->resident[block]){
            if(fseek(c->backing,at,SEEK_SET) || fread(c->input,1,size,c->backing)!=size)return 0;
            c->sd_reads++;c->sd_bytes+=size;
            if(size>c->arena_bytes)return 0;
            if(size>c->arena_bytes-c->arena_at)c->arena_at=0;
            unsigned begin=c->arena_at,end=begin+size;
            for(unsigned i=0;i<c->blocks;i++)if(c->resident[i]){
                unsigned old=c->resident[i]-1,length=fh_block_word(c->storage+32+i*16+4);
                if(old<end && begin<old+length){c->resident[i]=0;c->evictions++;}
            }
            memcpy(c->storage+c->arena_start+begin,c->input,size);
            c->resident[block]=begin+1;c->arena_at=end;
        }
        encoded=c->storage+c->arena_start+c->resident[block]-1;
    }else encoded=c->storage+at;
    if(fh_block_word(e+12)){
        if(!fh_lz4_block(c->scratch,n,encoded,size))return 0;
    }else memcpy(c->scratch,encoded,n);
    if(c->backing && (unsigned)crc32(0,c->scratch,n)!=fh_block_word(e+8))return 0;
    c->decoded=block;c->decodes++;return 1;
}
static inline int fh_block_cache_read(FhBlockCache *c,unsigned at,void *out,unsigned bytes){
    if(!c->storage || (!out && bytes) || at>c->raw_bytes || bytes>c->raw_bytes-at)return 0;
    unsigned char *target=out;unsigned remaining=bytes;
    while(remaining){
        unsigned block=at/FH_BLOCK_CACHE_CHUNK,offset=at%FH_BLOCK_CACHE_CHUNK;
        unsigned n=FH_BLOCK_CACHE_CHUNK-offset;if(n>remaining)n=remaining;
        if(!fh_block_cache_decode(c,block))return 0;
        memcpy(target,c->scratch+offset,n);target+=n;at+=n;remaining-=n;
    }
    c->reads++;c->bytes+=bytes;return 1;
}
static inline int fh_block_cache_open(FhBlockCache *c,const char *path,unsigned budget,unsigned expected_bytes){
    fh_block_cache_close(c);
    if(budget<FH_BLOCK_CACHE_CHUNK+48 || !expected_bytes || expected_bytes>64*1024*1024)return 0;
    FILE *f=fh_asset_open_staged(path);if(!f)return 0;
    long length;int ok=0;unsigned char header[32];
    if(fseek(f,0,SEEK_END) || (length=ftell(f))<48 || (unsigned long)length>budget-FH_BLOCK_CACHE_CHUNK ||
       fseek(f,0,SEEK_SET) || fread(header,1,32,f)!=32 || memcmp(header,"FHBCACH1",8))goto done;
    unsigned n=(unsigned)length,raw=fh_block_word(header+8),blocks=fh_block_word(header+12);
    if(raw!=expected_bytes || blocks!=(raw+FH_BLOCK_CACHE_CHUNK-1)/FH_BLOCK_CACHE_CHUNK ||
       fh_block_word(header+16)!=n || fh_block_word(header+28) || 32+blocks*16>n)goto done;
    c->storage=malloc(n+FH_BLOCK_CACHE_CHUNK);if(!c->storage)goto done;
    c->storage_bytes=n+FH_BLOCK_CACHE_CHUNK;c->file_bytes=n;c->raw_bytes=raw;c->blocks=blocks;
    c->scratch=c->storage+n;c->decoded=UINT32_MAX;memcpy(c->storage,header,32);
    if(fread(c->storage+32,1,n-32,f)!=n-32 ||
       (unsigned)crc32(0,c->storage+32,blocks*16)!=fh_block_word(header+20))goto done;
    unsigned end=32+blocks*16,whole=0;
    for(unsigned i=0;i<blocks;i++){
        const unsigned char *e=c->storage+32+i*16;
        unsigned at=fh_block_word(e),size=fh_block_word(e+4),mode=fh_block_word(e+12);
        unsigned count=raw-i*FH_BLOCK_CACHE_CHUNK;if(count>FH_BLOCK_CACHE_CHUNK)count=FH_BLOCK_CACHE_CHUNK;
        if(at!=end || at>n || !size || size>n-at || size>count || mode>1 || (!mode && size!=count))goto done;
        end=at+size;
        if(!fh_block_cache_decode(c,i) || (unsigned)crc32(0,c->scratch,count)!=fh_block_word(e+8))goto done;
        whole=crc32(whole,c->scratch,count);
    }
    if(end!=n || whole!=fh_block_word(header+24))goto done;
    c->decoded=UINT32_MAX;c->decodes=0;ok=1;
done:
    if(fclose(f))ok=0;
    if(!ok)fh_block_cache_close(c);
    return ok;
}
/* Same immutable container, same decoder/CRC, bounded normal RAM. Validate the
 * entire file at startup and retain its first compressed blocks that fit.
 * Later misses replace overlapping compressed ranges in a FIFO byte arena;
 * no allocation, game callback, GPU access or async ownership is introduced.
 * A miss remains filesystem work and must never be reported as resident I/O. */
static inline int fh_block_cache_open_window(FhBlockCache *c,const char *path,unsigned budget,unsigned expected_bytes){
    fh_block_cache_close(c);
    if(!expected_bytes || expected_bytes>64*1024*1024)return 0;
    FILE *f=fh_asset_open_staged(path);if(!f)return 0;
    long length;unsigned char header[32];int ok=0;
    if(fseek(f,0,SEEK_END) || (length=ftell(f))<48 || (unsigned long)length>65*1024*1024 ||
       fseek(f,0,SEEK_SET) || fread(header,1,32,f)!=32 || memcmp(header,"FHBCACH1",8))goto done;
    unsigned raw=fh_block_word(header+8),blocks=fh_block_word(header+12),n=(unsigned)length;
    if(raw!=expected_bytes || blocks!=(raw+FH_BLOCK_CACHE_CHUNK-1)/FH_BLOCK_CACHE_CHUNK ||
       fh_block_word(header+16)!=n || fh_block_word(header+28) || 32+blocks*16>n ||
       budget<32+blocks*20+3*FH_BLOCK_CACHE_CHUNK)goto done;
    c->storage=malloc(budget);if(!c->storage)goto done;
    c->storage_bytes=budget;c->file_bytes=n;c->raw_bytes=raw;c->blocks=blocks;
    c->resident=(unsigned*)(c->storage+32+blocks*16);memset(c->resident,0,blocks*4);
    c->arena_start=32+blocks*20;c->arena_bytes=budget-c->arena_start-2*FH_BLOCK_CACHE_CHUNK;
    c->input=c->storage+budget-2*FH_BLOCK_CACHE_CHUNK;c->scratch=c->input+FH_BLOCK_CACHE_CHUNK;
    memcpy(c->storage,header,32);
    if(fread(c->storage+32,16,blocks,f)!=blocks ||
       (unsigned)crc32(0,c->storage+32,blocks*16)!=fh_block_word(header+20))goto done;
    unsigned end=32+blocks*16,whole=0,prefix=1;
    for(unsigned i=0;i<blocks;i++){
        const unsigned char *e=c->storage+32+i*16;
        unsigned at=fh_block_word(e),size=fh_block_word(e+4),mode=fh_block_word(e+12);
        unsigned count=raw-i*FH_BLOCK_CACHE_CHUNK;if(count>FH_BLOCK_CACHE_CHUNK)count=FH_BLOCK_CACHE_CHUNK;
        if(at!=end || at>n || !size || size>n-at || size>count || mode>1 || (!mode && size!=count) ||
           fread(c->input,1,size,f)!=size)goto done;
        if(mode){if(!fh_lz4_block(c->scratch,count,c->input,size))goto done;}
        else memcpy(c->scratch,c->input,count);
        if((unsigned)crc32(0,c->scratch,count)!=fh_block_word(e+8))goto done;
        whole=crc32(whole,c->scratch,count);end=at+size;
        if(size>c->arena_bytes-c->arena_at)prefix=0;
        if(prefix){memcpy(c->storage+c->arena_start+c->arena_at,c->input,size);c->resident[i]=c->arena_at+1;c->arena_at+=size;}
    }
    if(end!=n || whole!=fh_block_word(header+24))goto done;
    c->backing=f;c->decoded=UINT32_MAX;ok=1;
done:
    if(!ok){fclose(f);fh_block_cache_close(c);}
    return ok;
}
#endif
