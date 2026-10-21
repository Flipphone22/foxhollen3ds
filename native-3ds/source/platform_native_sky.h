/* Shared renderer ownership, raster lowering and explicit GPU completion. */
#include "fh_native_sky.h"
#include "native_sky_shader.h"
#include "fh_surface_detail.h"
enum { SKY_GROUPS=76,SKY_POINTS=50,SKY_LIST_BYTES=544 };
static struct {
    unsigned active,emit,next,valid,draws,uploads;
    void *vertices;
    unsigned char *source;
    unsigned sizes[SKY_GROUPS];
    FhDrawState state;
    float view[3][4],colors[SKY_GROUPS][4];
    DVLB_s *binary;shaderProgram_s program;
} native_sky;
static unsigned sky_half(const unsigned char *p){return (unsigned)p[0]<<8|p[1];}
static int sky_list_valid(const unsigned char *p,unsigned bytes){
    if(!p || bytes<3+SKY_POINTS*10 || bytes>SKY_LIST_BYTES || p[0]!=0xb8 || sky_half(p+1)!=SKY_POINTS)return 0;
    for(unsigned i=0;i<SKY_POINTS;i++)if(sky_half(p+3+i*10+6) || sky_half(p+3+i*10+8))return 0;
    for(unsigned i=3+SKY_POINTS*10;i<bytes;i++)if(p[i])return 0;
    return 1;
}
int fh_renderer_sky_begin(const void *const *lists,const uint16_t *sizes,unsigned count,
    const FhDrawState *state,const float view[3][4]){
    static unsigned reported;
    if(!reported++){extern u32 __ctru_heap_size,__stacksize__;char line[192];snprintf(line,sizeof(line),"Native sky gate fog=%u indirect=%u cull=%u color=%u normal=%u stack=%u budget=%u cpu=%u linear_free=%u\n",(unsigned)state->fog_type,(unsigned)state->indirect_count,(unsigned)state->cull,(unsigned)state->color_write,(unsigned)mallinfo().uordblks,(unsigned)__stacksize__,(unsigned)__ctru_heap_size,fh_native_scene_cpu_bytes,(unsigned)linearSpaceFree());fh_platform_log(line);}
    if(!initialized || (!fh_native_scene_enabled && !fh_surface_detail) || native_sky.active || count!=SKY_GROUPS || !lists || !sizes ||
       state->fog_type || state->indirect_count || state->cull || !state->color_write)return 0;
    unsigned changed=!native_sky.valid;
    for(unsigned i=0;i<SKY_GROUPS;i++){
        if(!sky_list_valid(lists[i],sizes[i]))return 0;
        if(!changed && (native_sky.sizes[i]!=sizes[i] ||
            fh_memory_compare(native_sky.source+i*SKY_LIST_BYTES,lists[i],sizes[i])))changed=1;
    }
    if(!native_sky.vertices){
        extern u32 __ctru_heap_size,__stacksize__;
        unsigned cpu=SKY_GROUPS*SKY_LIST_BYTES,gpu=SKY_GROUPS*SKY_POINTS*16;
        if(fh_native_scene_cpu_bytes+cpu>384*1024 || fh_native_scene_gpu_bytes+gpu>2*1024*1024 ||
           ((u64)(unsigned)mallinfo().uordblks+__stacksize__+cpu+4096)*100>(u64)__ctru_heap_size*85 ||
           linearSpaceFree()<gpu+LINEAR_RESERVE)return 0;
        native_sky.source=malloc(cpu);native_sky.vertices=linearMemAlign(gpu,128);
        if(!native_sky.source || !native_sky.vertices){free(native_sky.source);linearFree(native_sky.vertices);native_sky.source=NULL;native_sky.vertices=NULL;return 0;}
        fh_native_scene_cpu_bytes+=cpu;fh_native_scene_gpu_bytes+=gpu;
    }
    if(changed){
        begin();fh_renderer_sync(); /* Old GPU readers complete before rewrite. */
        float points[SKY_POINTS][4];
        for(unsigned i=0;i<SKY_GROUPS;i++){
            const unsigned char *p=lists[i];
            for(unsigned j=0;j<SKY_POINTS;j++){
                for(unsigned c=0;c<3;c++)points[j][c]=(int16_t)sky_half(p+3+j*10+c*2);
                points[j][3]=i;
            }
            memcpy((unsigned char*)native_sky.vertices+i*sizeof(points),points,sizeof(points));
            memcpy(native_sky.source+i*SKY_LIST_BYTES,p,sizes[i]);native_sky.sizes[i]=sizes[i];
        }
        GSPGPU_FlushDataCache(native_sky.vertices,SKY_GROUPS*SKY_POINTS*16);
        native_sky.valid=1;native_sky.uploads++;
    }
    if(!native_sky.binary){
        native_sky.binary=DVLB_ParseFile((u32*)native_sky_shader,sizeof(native_sky_shader));
        if(!native_sky.binary || native_sky.binary->numDVLE!=2)fh_platform_panic("Native sky shader parse failed");
        shaderProgramInit(&native_sky.program);shaderProgramSetVsh(&native_sky.program,&native_sky.binary->DVLE[0]);
        shaderProgramSetGsh(&native_sky.program,&native_sky.binary->DVLE[1],6);
    }
    native_sky.state=*state;memcpy(native_sky.view,view,sizeof(native_sky.view));native_sky.next=0;native_sky.active=1;return 1;
}
static void native_sky_emit(void){
    flush_draw();flush_stream_buffers();C3D_BindProgram(&native_sky.program);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddLoader(a,0,GPU_FLOAT,4);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,native_sky.vertices,16,1,0)<0)fh_platform_panic("Native sky vertex binding failed");
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,native_sky.view[r][0],native_sky.view[r][1],native_sky.view[r][2],native_sky.view[r][3]);
    float projection[4][4];fh_vertex_pica_projection(projection,&native_sky.state);
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    for(unsigned r=0;r<SKY_GROUPS;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,7+r,native_sky.colors[r][0],native_sky.colors[r][1],native_sky.colors[r][2],native_sky.colors[r][3]);
    C3D_FVUnifSet(GPU_GEOMETRY_SHADER,0,12.f/(6*FH_EFB_HEIGHT),12.f/(6*640),0,0);
    C3D_DrawArrays(GPU_GEOMETRY_PRIM,0,SKY_GROUPS*SKY_POINTS);
    draws++;total_vertices+=SKY_GROUPS*SKY_POINTS;native_sky.draws++;
    bind_geometry();
}
void fh_renderer_sky_color(unsigned group,unsigned r,unsigned g,unsigned b,unsigned a){
    if(!native_sky.active || group!=native_sky.next || group>=SKY_GROUPS || (r|g|b|a)>255)fh_platform_panic("Native sky group order");
    native_sky.colors[group][0]=r/255.f;native_sky.colors[group][1]=g/255.f;
    native_sky.colors[group][2]=b/255.f;native_sky.colors[group][3]=a/255.f;
    if(++native_sky.next!=SKY_GROUPS)return;
    /* The original per-group GX setters are retained and may be queued to
     * the shared owner. Retire them before this direct renderer submission. */
#ifdef FH_GX_DISPATCH_BUILD
    extern void fh_gx_dispatch_barrier(void);fh_gx_dispatch_barrier();
#endif
    FhMaterial plan={0};plan.map=plan.coord=-1;plan.scale[0]=plan.scale[1]=1;
    for(unsigned c=0;c<4;c++)plan.color[c][1+c]=1;
    service_audio();begin();
    native_sky.emit=1;render_direct(NULL,SKY_GROUPS*SKY_POINTS,&native_sky.state,&plan);
    native_sky.emit=native_sky.active=0;
}
static void native_sky_shutdown(void){
    if(native_sky.vertices){
        char line[128];snprintf(line,sizeof(line),"Native sky batches=%u lists_replaced=%u uploads=%u cpu=%u gpu=%u\n",
            native_sky.draws,native_sky.draws*SKY_GROUPS,native_sky.uploads,SKY_GROUPS*SKY_LIST_BYTES,SKY_GROUPS*SKY_POINTS*16);fh_platform_log(line);
        fh_native_scene_cpu_bytes-=SKY_GROUPS*SKY_LIST_BYTES;fh_native_scene_gpu_bytes-=SKY_GROUPS*SKY_POINTS*16;
        linearFree(native_sky.vertices);free(native_sky.source);
    }
    if(native_sky.binary){shaderProgramFree(&native_sky.program);DVLB_Free(native_sky.binary);}
    memset(&native_sky,0,sizeof(native_sky));
}
