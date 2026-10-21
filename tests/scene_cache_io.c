/* Included after the production scene stream adapter by test_block_cache.py. */
#include <assert.h>
int main(int argc,char **argv){
    (void)cache_load_ticks; /* initialize() is outside this adapter fixture. */
    assert(argc==3);file=fopen(argv[1],"rb");assert(file);
    assert(!fseek(file,0,SEEK_END));unsigned size=(unsigned)ftell(file);
    unsigned char *raw=malloc(size),*out=malloc(size);assert(raw && out);
    rewind(file);assert(fread(raw,1,size,file)==size);
    assert(fh_block_cache_open(&scene_cache,argv[2],8*1024*1024,size));
    assert(!scene_seek(65500));assert(scene_read(out,96)==96);
    assert(!memcmp(out,raw+65500,96));assert(scene_sd_reads==0);
    /* Pressure release between packet header and payload must restore the
     * physical FILE cursor, even across a compressed block boundary. */
    unsigned allocation=scene_cache.storage_bytes;
    assert(fh_native_scene_cache_release()==allocation);
    assert(fh_native_scene_cache_release()==0);
    assert(scene_read(out,8192)==8192);assert(!memcmp(out,raw+65596,8192));
    assert(scene_sd_reads==1 && scene_sd_bytes==8192 && cache_releases==1);
    assert(!scene_seek(size-17));assert(scene_read(out,32)==17);
    assert(!memcmp(out,raw+size-17,17));
    /* Absent cache is the normal Old 3DS/memory-pressure fallback. */
    assert(!scene_seek(123));assert(scene_read(out,90000)==90000);
    assert(!memcmp(out,raw+123,90000));
    assert(fh_block_cache_open(&scene_cache,argv[2],8*1024*1024,size));
    /* Invalid compressed payload after startup verification: original bytes
     * overwrite any partial output. No cached cursor may leak into fread. */
    memset(scene_cache.storage+32+scene_cache.blocks*16,0,
           scene_cache.file_bytes-32-scene_cache.blocks*16);
    assert(!scene_seek(65500));assert(scene_read(out,90000)==90000);
    assert(!memcmp(out,raw+65500,90000));assert(!scene_cache.storage);
    unsigned budget=32+((size+65535)/65536)*20+3*65536;
    assert(fh_block_cache_open_window(&scene_cache,argv[2],budget,size));
    assert(!scene_seek(0));assert(scene_read(out,size)==size);
    assert(!memcmp(out,raw,size));assert(scene_cache.sd_reads);
    assert(!scene_seek(65500));assert(scene_read(out,96)==96);
    assert(fh_native_scene_cache_release()==budget);
    assert(scene_read(out,8192)==8192);assert(!memcmp(out,raw+65596,8192));
    assert(fh_block_cache_open_window(&scene_cache,argv[2],budget,size));
    assert(!scene_seek(0));assert(scene_read(out,1)==1);
    unsigned resident=scene_cache.resident[0];assert(resident);
    memset(scene_cache.storage+scene_cache.arena_start+resident-1,0,fh_block_word(scene_cache.storage+36));
    scene_cache.decoded=UINT32_MAX;
    assert(!scene_seek(0));assert(scene_read(out,90000)==90000);
    assert(!memcmp(out,raw,90000));assert(!scene_cache.storage);
    fclose(file);free(raw);free(out);
    puts("Scene stream: cached reads, mid-packet pressure release, short read, uncached seek, corrupt-cache fallback passed");
    return 0;
}
