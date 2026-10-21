#ifndef FH_LAST_FRAME_H
#define FH_LAST_FRAME_H
#include <stddef.h>
enum { FH_LAST_FRAME_WIDTH=400, FH_LAST_FRAME_HEIGHT=240,
       FH_LAST_FRAME_BYTES=400*240*3, FH_LAST_FRAME_SOURCE_BYTES=400*240*4 };
/* libctru's portrait RGBA8 LCD storage is ABGR, bottom-to-top within each
 * column. Read explicitly on the CPU before any filesystem IPC, following
 * pinned Melee capture_frame: accelerated emulator surfaces need materializing.
 * This is a completed LCD transfer buffer, never raw PICA depth/colour VRAM. */
static inline int fh_last_frame_copy(unsigned char *out,size_t bytes,
    const volatile unsigned char *lcd,unsigned physical_width,unsigned physical_height){
    if(!out || !lcd || bytes<FH_LAST_FRAME_BYTES ||
       physical_width!=FH_LAST_FRAME_HEIGHT || physical_height!=FH_LAST_FRAME_WIDTH)return 0;
    for(unsigned y=0;y<FH_LAST_FRAME_HEIGHT;y++)for(unsigned x=0;x<FH_LAST_FRAME_WIDTH;x++){
        size_t input=((size_t)x*FH_LAST_FRAME_HEIGHT+FH_LAST_FRAME_HEIGHT-1-y)*4;
        size_t output=((size_t)y*FH_LAST_FRAME_WIDTH+x)*3;
        out[output]=lcd[input+3];out[output+1]=lcd[input+2];out[output+2]=lcd[input+1];
    }
    return 1;
}
#endif
