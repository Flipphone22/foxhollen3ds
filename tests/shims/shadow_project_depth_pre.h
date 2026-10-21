#include <assert.h>
#include <stdint.h>
typedef struct {int field_rendering,fbWidth,xfbHeight;} GXRenderModeObj;
static GXRenderModeObj source_mode={0,640,480},*gRenderModeObj=&source_mode;
static unsigned gViewportJitterField;
static float source_near,source_far;
static void GXSetViewport(float x,float y,float w,float h,float n,float f){
    assert(x==0&&y==0&&w==640&&h==480);source_near=n;source_far=f;
}
static void GXSetViewportJitter(float x,float y,float w,float h,float n,float f,unsigned field){
    assert(field==gViewportJitterField);GXSetViewport(x,y,w,h,n,f);
}
