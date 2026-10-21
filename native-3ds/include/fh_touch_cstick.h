#ifndef FH_TOUCH_CSTICK_H
#define FH_TOUCH_CSTICK_H
#include "fh_pad_dpad_cstick.h"

enum { FH_TOUCH_CX=240, FH_TOUCH_CY=152, FH_TOUCH_RADIUS=64,
       FH_TOUCH_RANGE=48, FH_TOUCH_DEAD=8 };
typedef struct { unsigned held,active; } FhTouchCstick;
static inline int fh_touch_cstick_hit(int x,int y){
    if(x<0 || x>=320 || y<0 || y>=240)return 0;
    int dx=x-FH_TOUCH_CX,dy=y-FH_TOUCH_CY;
    return dx*dx+dy*dy<=FH_TOUCH_RADIUS*FH_TOUCH_RADIUS;
}
static inline int fh_touch_cstick_axis(int delta){
    int sign=delta<0?-1:1;
    if(delta<0)delta=-delta;
    if(delta<=FH_TOUCH_DEAD)return 0;
    if(delta>FH_TOUCH_RANGE)delta=FH_TOUCH_RANGE;
    return sign*(delta-FH_TOUCH_DEAD)*127/(FH_TOUCH_RANGE-FH_TOUCH_DEAD);
}
/* Called once per HID sample, independently of game/render FPS. Contacts must
 * start inside the pad; dragging out stays captured until release. Recompute
 * from the current contact, never accumulate displacement or retain axes. */
static inline void fh_touch_cstick_update(FhTouchCstick *touch,FhPadState *pad,
                                         int held,int x,int y){
    held=!!held;
    if(!held)touch->active=0;
    else if(!touch->held)touch->active=fh_touch_cstick_hit(x,y);
    touch->held=held;
    pad->cx=pad->cy=0;
    fh_pad_dpad_cstick(pad,held); /* Preserve existing physical D-pad shortcut. */
    if(touch->active){
        /* HID coordinates are screen pixels. Guard before signed arithmetic. */
        if(x<0)x=0;
        if(x>319)x=319;
        if(y<0)y=0;
        if(y>239)y=239;
        pad->cx=fh_touch_cstick_axis(x-FH_TOUCH_CX);
        pad->cy=fh_touch_cstick_axis(FH_TOUCH_CY-y); /* GC up is positive. */
    }
}

/* Bottom LCD RGB565 storage is rotated: columns of 240 pixels, bottom first.
 * Draw only during initialization into both existing LCD buffers. No new
 * allocation, per-frame rasterization, GPU commands or cache flushes. */
static inline void fh_touch_cstick_pixel(uint16_t *fb,int x,int y,uint16_t c){
    if(x>=0 && x<320 && y>=0 && y<240)fb[x*240+239-y]=c;
}
static inline void fh_touch_cstick_label(uint16_t *fb,int x,int y){
    static const uint8_t letters[5][7]={ /* ITEMS, 5x7 at scale 2 */
        {31,4,4,4,4,4,31},{31,4,4,4,4,4,4},
        {31,16,16,30,16,16,31},{17,27,21,21,17,17,17},
        {15,16,16,14,1,1,30}};
    for(int l=0;l<5;l++)for(int row=0;row<7;row++)for(int col=0;col<5;col++)
        if(letters[l][row]&(16>>col))for(int sy=0;sy<2;sy++)for(int sx=0;sx<2;sx++)
            fh_touch_cstick_pixel(fb,x+l*12+col*2+sx,y+row*2+sy,0xffff);
}
static inline void fh_touch_cstick_draw(uint16_t *fb){
    for(int y=-FH_TOUCH_RADIUS;y<=FH_TOUCH_RADIUS;y++)
        for(int x=-FH_TOUCH_RADIUS;x<=FH_TOUCH_RADIUS;x++){
            int r=x*x+y*y;
            if(r>FH_TOUCH_RADIUS*FH_TOUCH_RADIUS)continue;
            uint16_t c=r>=61*61?0xffe0:0x18c3;
            if(r<=FH_TOUCH_DEAD*FH_TOUCH_DEAD)c=0x7bef;
            /* Four yellow arrows identify the item-menu directions. */
            int ax=x<0?-x:x,ay=y<0?-y:y;
            if((ay>=38 && ay<=49 && ax<=49-ay) ||
               (ax>=38 && ax<=49 && ay<=49-ax))c=0xffe0;
            fh_touch_cstick_pixel(fb,FH_TOUCH_CX+x,FH_TOUCH_CY+y,c);
        }
    fh_touch_cstick_label(fb,FH_TOUCH_CX-29,60);
}
#endif
