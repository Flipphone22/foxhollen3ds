/* Included by the native material owner after its pack declaration.
 * Reuse the verified staged page reader, never expose this storage to PICA.
 * No reallocation during draws, no worker and no GPU-retirement requirement. */
#include "fh_pica_texture_window.h"
static void *texture_window;
static unsigned texture_window_bytes,texture_window_peak,texture_window_releases;
unsigned fh_pica_texture_window_release(void){
    unsigned bytes=texture_window_bytes;
    if(!bytes)return 0;
    fh_texture_pack_window_drop(&pack); /* detach before freeing caller storage */
    free(texture_window);texture_window=NULL;texture_window_bytes=0;
    texture_window_releases++;return bytes;
}
static void texture_window_init(void){
    extern u32 __ctru_heap_size,__stacksize__;
    unsigned bytes=fh_hardware_large_memory?256u*1024u:64u*1024u;
    if(!pack.file || texture_window ||
       ((u64)(unsigned)mallinfo().uordblks+__stacksize__+bytes+64)*100>=(u64)__ctru_heap_size*85)return;
    void *storage=malloc(bytes);
    if(!storage)return;
    if(!fh_texture_pack_window(&pack,storage,bytes)){free(storage);return;}
    texture_window=storage;texture_window_bytes=bytes;
    if(bytes>texture_window_peak)texture_window_peak=bytes;
}
static void texture_window_report(void){
    char line[256];
    snprintf(line,sizeof(line),"PICA texture window: bytes=%u peak=%u releases=%u hits=%llu misses=%llu returned=%llu read_calls=%llu file_bytes=%llu\n",
        texture_window_bytes,texture_window_peak,texture_window_releases,
        (unsigned long long)pack.window_hits,(unsigned long long)pack.window_misses,
        (unsigned long long)pack.window_bytes,(unsigned long long)pack.read_calls,
        (unsigned long long)pack.read_bytes);
    fh_platform_log(line);
}
