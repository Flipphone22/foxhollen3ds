#include <dolphin/pad.h>
#include <string.h>
#include "fh_platform.h"

static PADKeyButtonBinding key_buttons[4][PAD_BUTTON_COUNT];
static PADKeyAxisBinding key_axes[4][PAD_BUTTON_COUNT];
BOOL PADInit(void) { return 1; }
BOOL PADReset(u32 mask) { (void)mask;return 1; }
BOOL PADRecalibrate(u32 mask) { return PADReset(mask); }
u32 PADRead(PADStatus *status) {
    FhPadState native;
    fh_platform_pad(&native);
    memset(status,0,4*sizeof(*status));
    memcpy(status,&native,sizeof(native));
    for(unsigned i=1;i<4;i++) status[i].err=PAD_ERR_NO_CONTROLLER;
    return 0;
}
static void clamp_stick(s8 *px,s8 *py,int maximum,int diagonal) {
    int x=*px,y=*py,sx=x<0?-1:1,sy=y<0?-1:1;
    x*=sx;y*=sy;
    x=x<=15?0:(x-15)*maximum/112;
    y=y<=15?0:(y-15)*maximum/112;
    int d=x>=y?diagonal*x+(maximum-diagonal)*y:diagonal*y+(maximum-diagonal)*x;
    if(d>diagonal*maximum) { x=diagonal*maximum*x/d;y=diagonal*maximum*y/d; }
    *px=x*sx;*py=y*sy;
}
static u8 clamp_trigger(u8 value) { return value<=30?0:(value>180?180:value)-30; }
void PADClamp(PADStatus *status) {
    for(unsigned i=0;i<4;i++) if(!status[i].err) {
        clamp_stick(&status[i].stickX,&status[i].stickY,72,40);
        clamp_stick(&status[i].substickX,&status[i].substickY,59,31);
        status[i].triggerLeft=clamp_trigger(status[i].triggerLeft);
        status[i].triggerRight=clamp_trigger(status[i].triggerRight);
    }
}
void PADControlMotor(u32 port,u32 command) { (void)port;(void)command; }
void PADSetKeyboardActive(u32 port,BOOL active) { (void)port;(void)active; }
BOOL PADSetKeyButtonBindings(u32 port,PADKeyButtonBinding *bindings) {
    if(port>=4 || !bindings) return 0;
    memcpy(key_buttons[port],bindings,sizeof(key_buttons[port]));return 1;
}
BOOL PADSetKeyAxisBindings(u32 port,PADKeyAxisBinding *bindings) {
    if(port>=4 || !bindings) return 0;
    memcpy(key_axes[port],bindings,sizeof(key_axes[port]));return 1;
}
PADKeyButtonBinding *PADGetKeyButtonBindings(u32 port,u32 *count) {
    if(count) *count=port<4?PAD_BUTTON_COUNT:0;
    return port<4?key_buttons[port]:NULL;
}
