#ifndef FH_GPU_INPUT_SNAPSHOT_H
#define FH_GPU_INPUT_SNAPSHOT_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* CPU-visible input evidence only. Never invalidate GPU caches or read VRAM.
 * Concurrent GPU/audio writes mean this is not an atomic device snapshot.
 * Streaming through ordinary heap scratch avoids filesystem IPC on linear RAM.
 * Header/footer are explicit little-endian, independent of C struct padding. */
enum { FH_GPU_SNAPSHOT_CHUNK=65536, FH_GPU_SNAPSHOT_MAX=24*1024*1024 };
static void fh_snapshot_u32(unsigned char *p,uint32_t v){
    for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));
}
static int fh_gpu_input_snapshot(FILE *f,const unsigned char *source,
    uint32_t virt,uint32_t phys,uint32_t bytes,uint32_t cmdvirt,
    uint32_t cmdphys,uint32_t cmdbytes,unsigned char *scratch){
    if(!f || !source || !scratch || !bytes || bytes>FH_GPU_SNAPSHOT_MAX ||
       !virt || !phys || bytes>UINT32_MAX-virt || bytes>UINT32_MAX-phys ||
       cmdvirt<virt || cmdvirt-virt>bytes || cmdbytes>bytes-(cmdvirt-virt) ||
       cmdphys!=phys+(cmdvirt-virt))return 0;
    unsigned char h[64]={0};memcpy(h,"FHGPUM1",7);
    fh_snapshot_u32(h+8,1);fh_snapshot_u32(h+12,sizeof(h));
    fh_snapshot_u32(h+16,virt);fh_snapshot_u32(h+20,phys);
    fh_snapshot_u32(h+24,bytes);fh_snapshot_u32(h+28,cmdvirt);
    fh_snapshot_u32(h+32,cmdphys);fh_snapshot_u32(h+36,cmdbytes);
    fh_snapshot_u32(h+40,1); /* bit0: CPU view, potentially stale GPU outputs */
    if(fwrite(h,1,sizeof(h),f)!=sizeof(h) || ferror(f))return 0;
    uint32_t hash=2166136261u;
    for(uint32_t off=0;off<bytes;){
        uint32_t n=bytes-off;if(n>FH_GPU_SNAPSHOT_CHUNK)n=FH_GPU_SNAPSHOT_CHUNK;
        memcpy(scratch,source+off,n);
        for(uint32_t i=0;i<n;i++){hash^=scratch[i];hash*=16777619u;}
        if(fwrite(scratch,1,n,f)!=n || ferror(f))return 0;
        off+=n;
    }
    unsigned char end[16]={0};memcpy(end,"FHGPEND",7);
    fh_snapshot_u32(end+8,bytes);fh_snapshot_u32(end+12,hash);
    return fwrite(end,1,sizeof(end),f)==sizeof(end) && !ferror(f);
}
#endif
