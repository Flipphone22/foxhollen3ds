/* Included by the image owner. Colour-only shutdown diagnostic: never reads
 * raw depth, never runs during gameplay. All descriptors remain alive until
 * the parent's queue join completes. See pinned Melee blend_verify.h. */
#include <string.h>
extern void fh_minimal_sync(void);
static uint32_t shadow_capture_meta[FH_SHADOW_IMAGE_SLOTS][8];
void fh_pica_shadow_image_describe(unsigned asset,unsigned frame,unsigned alpha,unsigned receivers){
    uint32_t *m=shadow_capture_meta[slot];
    m[1]=frame;m[2]=asset;m[3]=alpha;m[6]=receivers;
}
static uint32_t shadow_capture_crc(const unsigned char *p,unsigned bytes){
    uint32_t c=~0u;
    for(unsigned i=0;i<bytes;i++){
        c^=p[i];for(unsigned j=0;j<8;j++)c=(c>>1)^(0xedb88320u&-(c&1));
    }
    return ~c;
}
void fh_pica_shadow_image_capture_exit(void){
    if(!fh_platform_capture_requested() || !initialized || active || !updates)return;
    enum {BYTES=FH_SHADOW_IMAGE_SIZE*FH_SHADOW_IMAGE_SIZE*3};
    unsigned char *pixels=linearAlloc(BYTES);
    if(!pixels){fh_platform_log("Shadow masks: no shutdown scratch memory\n");return;}
    const char *path="sdmc:/3ds/foxhollow/shadow-masks.bin";
    const char *temporary="sdmc:/3ds/foxhollow/shadow-masks.tmp";
    uint64_t stamp=osGetTime();
    uint32_t header[12]={0x4d534846,1,FH_SHADOW_IMAGE_SIZE,FH_SHADOW_IMAGE_SIZE,
        FH_SHADOW_IMAGE_SLOTS,(uint32_t)stamp,(uint32_t)(stamp>>32),32,BYTES,slot,updates,0};
    FILE *file=fopen(temporary,"wb");unsigned ok=0;
    if(file){
        ok=fwrite(header,1,sizeof(header),file)==sizeof(header);
        for(unsigned i=0;ok && i<FH_SHADOW_IMAGE_SLOTS;i++){
            /* Publish reused allocator cache lines BEFORE GPU writes. Never
             * clean the destination after transfer: invalidate only. */
            if(R_FAILED(GSPGPU_FlushDataCache(pixels,BYTES))){ok=0;break;}
            if(shadow_capture_meta[i][0]){
                C3D_SyncDisplayTransfer(targets[i].colorBuf,
                    GX_BUFFER_DIM(FH_SHADOW_IMAGE_SIZE,FH_SHADOW_IMAGE_SIZE),(u32*)pixels,
                    GX_BUFFER_DIM(FH_SHADOW_IMAGE_SIZE,FH_SHADOW_IMAGE_SIZE),
                    GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB565)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8));
                /* In-frame SyncDisplayTransfer queues, it does NOT wait. */
                fh_minimal_sync();
                if(R_FAILED(GSPGPU_InvalidateDataCache(pixels,BYTES))){ok=0;break;}
            }else memset(pixels,0,BYTES); /* An unused slot is explicitly invalid. */
            uint32_t meta[8];memcpy(meta,shadow_capture_meta[i],sizeof(meta));
            meta[7]=shadow_capture_crc(pixels,BYTES);
            ok=fwrite(meta,1,sizeof(meta),file)==sizeof(meta) && fwrite(pixels,1,BYTES,file)==BYTES;
        }
        if(fclose(file))ok=0;
        if(ok){remove(path);if(rename(temporary,path))ok=0;}
        if(!ok)remove(temporary);
    }
    linearFree(pixels);
    char line[192];snprintf(line,sizeof(line),"Shadow masks: %s stamp=%llu updates=%u latest_slot=%u scratch_bytes=%u\n",
        ok?"shadow-masks.bin saved":"capture failed",(unsigned long long)stamp,updates,slot,BYTES);
    fh_platform_log(line);
}
