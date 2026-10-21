/* Called only on requested clean exit, after renderer retirement and before
 * gfxExit. No per-frame readback, image allocation or filesystem work. */
#include "fh_last_frame.h"
static unsigned last_frame_requested;
int fh_platform_capture_requested(void){return last_frame_requested!=0;}
static void last_frame_capture(unsigned retrace){
    if(!last_frame_requested)return;
    u16 width=0,height=0;
    const u8 *lcd=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&width,&height);
    if(!lcd || width!=240 || height!=400 || gfxGetScreenFormat(GFX_TOP)!=GSP_RGBA8_OES){
        fh_platform_log("Last frame: unsupported LCD layout; no capture\n");return;
    }
    unsigned char *rgb=malloc(FH_LAST_FRAME_BYTES);
    if(!rgb){fh_platform_log("Last frame: no shutdown scratch memory\n");return;}
    /* GSP still exists; the renderer has joined all GPU/transfer work. Never
     * clean/flush this GPU-written buffer before reading it. */
    Result result=GSPGPU_InvalidateDataCache((void*)lcd,FH_LAST_FRAME_SOURCE_BYTES);
    if(R_FAILED(result) || !fh_last_frame_copy(rgb,FH_LAST_FRAME_BYTES,lcd,width,height)){
        free(rgb);fh_platform_log("Last frame: LCD cache invalidation failed\n");return;
    }
    unsigned long long stamp=(unsigned long long)osGetTime();
    const char *path="sdmc:/3ds/foxhollow/last-frame-top.ppm";
    FILE *file=fopen(path,"wb");unsigned ok=0;
    if(file){
        /* gfxGetFramebuffer exposes the drawing buffer: after double-buffer
         * swap this can be one presented frame older than the visible screen.
         * Record that limitation instead of claiming an exact scanout capture. */
        ok=fprintf(file,"P6\n# retrace=%u stamp=%llu source=completed-LCD-back-buffer\n400 240\n255\n",retrace,stamp)>0;
        if(ok)ok=fwrite(rgb,1,FH_LAST_FRAME_BYTES,file)==FH_LAST_FRAME_BYTES;
        if(fclose(file))ok=0;
        if(!ok)remove(path); /* Never leave a partial image labelled complete. */
    }
    free(rgb);
    char message[160];
    snprintf(message,sizeof(message),"Last frame: %s retrace=%u stamp=%llu (LCD back buffer; may be one frame older)\n",
        ok?"last-frame-top.ppm saved":"capture write failed",retrace,stamp);
    fh_platform_log(message);
}
