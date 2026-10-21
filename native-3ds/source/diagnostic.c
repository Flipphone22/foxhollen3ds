#include <3ds.h>
#include <citro3d.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "diagnostic_shader.h"

u32 __ctru_heap_size = 8 * 1024 * 1024;
u32 __ctru_linear_heap_size = 8 * 1024 * 1024;

enum { WIDTH = 400, HEIGHT = 240, FRAME_COUNT = 240, MAX_VERTICES = 96 };
static const char *directory = "sdmc:/3ds/foxhollow-foundation";
typedef struct { float position[3], uv[2], color[4]; } Vertex;
typedef struct { unsigned frame, buttons; int x, y; } Input;
_Static_assert(sizeof(Vertex) == 36, "Vertex layout");
_Static_assert(sizeof(void*) == 4, "ARM32 target");
static Vertex *vertices;
static unsigned vertex_count, frames, input_count, applied_inputs;
static Input inputs[32];
static FILE *logfile;
static unsigned min_linear, min_vram, max_heap;
static s64 max_commit, peak_commit;
static bool cpu_pending;

static void log_event(const char *message) {
    if (logfile) { fprintf(logfile, "%u %s\n", frames, message); fflush(logfile); }
    svcOutputDebugString(message, strlen(message));
}

static FILE *open_file(const char *name, const char *mode) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", directory, name);
    return fopen(path, mode);
}

static void on_apt(APT_HookType type, void *unused) {
    (void)unused;
    if (type == APTHOOK_ONRESTORE || type == APTHOOK_ONWAKEUP) cpu_pending = true;
}

static void memory_sample(void) {
    struct mallinfo heap = mallinfo();
    unsigned linear = linearSpaceFree(), vram = vramSpaceFree();
    if ((unsigned)heap.uordblks > max_heap) max_heap = heap.uordblks;
    if (linear < min_linear) min_linear = linear;
    if (vram < min_vram) min_vram = vram;
    Handle limit;
    ResourceLimitType type = RESLIMIT_COMMIT;
    if (R_SUCCEEDED(svcGetResourceLimit(&limit, CUR_PROCESS_HANDLE))) {
        s64 current = 0;
        svcGetResourceLimitLimitValues(&max_commit, limit, &type, 1);
        svcGetResourceLimitCurrentValues(&current, limit, &type, 1);
        if (current > peak_commit) peak_commit = current;
        svcCloseHandle(limit);
    }
}

static void quad(float x, float y, float w, float h, float z,
                 float r, float g, float b, float a) {
    const unsigned corners[6] = {0,1,2,0,2,3};
    const float coords[4][2] = {{x,y},{x+w,y},{x+w,y+h},{x,y+h}};
    const float uv[4][2] = {{0,0},{1,0},{1,1},{0,1}};
    for (unsigned i=0; i<6; ++i) {
        Vertex *v = &vertices[vertex_count++];
        unsigned c = corners[i];
        *v = (Vertex){{coords[c][0],coords[c][1],z}, {uv[c][0],uv[c][1]}, {r,g,b,a}};
    }
}

static unsigned morton(unsigned x, unsigned y) {
    unsigned value=0;
    for (unsigned bit=0;bit<3;++bit) value |= ((x>>bit)&1)<<(2*bit) | ((y>>bit)&1)<<(2*bit+1);
    return value;
}

static void set_texture(bool enabled) {
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, enabled ? GPU_TEXTURE0 : GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, 0);
    C3D_TexEnvFunc(env, C3D_Both, enabled ? GPU_MODULATE : GPU_REPLACE);
}

static bool sd_check(void) {
    unsigned char data[256];
    FILE *source=open_file("fixture.bin", "rb");
    if (!source) return false;
    size_t size=fread(data,1,sizeof(data),source);
    int extra=fgetc(source);
    fclose(source);
    if (size!=sizeof(data) || extra!=EOF) return false;
    FILE *dest=open_file("roundtrip.bin", "wb");
    if (!dest) return false;
    bool ok=fwrite(data,1,size,dest)==size;
    if (fclose(dest)) ok=false;
    return ok;
}

static bool capture(C3D_RenderTarget *target, u32 *readback, unsigned number) {
    C3D_FrameSync();
    C3D_SyncDisplayTransfer(target->frameBuf.colorBuf, GX_BUFFER_DIM(240,400), readback,
                           GX_BUFFER_DIM(240,400), 0);
    if (R_FAILED(GSPGPU_InvalidateDataCache(readback, WIDTH*HEIGHT*4))) return false;
    u8 *copy=malloc(WIDTH*HEIGHT*4);
    if (!copy) return false;
    for (unsigned i=0;i<WIDTH*HEIGHT*4;++i) copy[i]=((volatile u8*)readback)[i];
    char name[64];
    snprintf(name,sizeof(name),"frame-%03u.rgba",number);
    FILE *file=open_file(name,"wb");
    if (!file) { free(copy); return false; }
    bool ok=fwrite(copy,1,WIDTH*HEIGHT*4,file)==WIDTH*HEIGHT*4;
    if (fclose(file)) ok=false;
    memory_sample();
    free(copy);
    return ok;
}

int main(void) {
    mkdir("sdmc:/3ds",0777);
    mkdir(directory,0777);
    logfile=open_file("diagnostic.log","wb");
    log_event("startup");
    bool is_new=false;
    Result model_query=APT_CheckNew3DS(&is_new);
    if (is_new) osSetSpeedupEnable(true);
    aptHookCookie hook;
    aptHook(&hook,on_apt,NULL);
    min_linear=~0u; min_vram=~0u;
    bool sd_ok=sd_check();
    FILE *script=open_file("input.txt","r");
    if (script) {
        while (input_count<32 && fscanf(script,"%u %u %d %d",&inputs[input_count].frame,
               &inputs[input_count].buttons,&inputs[input_count].x,&inputs[input_count].y)==4) ++input_count;
        fclose(script);
    }
    gfxInit(GSP_RGBA8_OES,GSP_RGB565_OES,false);
    consoleInit(GFX_BOTTOM,NULL);
    printf("Foxhollow foundation\nNative New 3DS diagnostic\n\nTexturing / depth / blending\nScripted input / SD roundtrip\n\n240 frames, automatic exit\n");
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) { log_event("C3D_Init failed"); return 1; }
    C3D_RenderTarget *target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    vertices=linearAlloc(MAX_VERTICES*sizeof(Vertex));
    u32 *readback=linearMemAlign(WIDTH*HEIGHT*4,0x1000);
    C3D_Tex texture;
    if (!target || !vertices || !readback || !C3D_TexInit(&texture,8,8,GPU_RGBA8)) {
        log_event("allocation failed"); return 1;
    }
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8));
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
        u8 *pixel=(u8*)texture.data+4*morton(x,y);
        bool bright=((x/2)^(y/2))&1;
        pixel[0]=255; pixel[1]=bright?32:200; pixel[2]=bright?220:64; pixel[3]=bright?240:32;
    }
    C3D_TexFlush(&texture);
    C3D_TexSetFilter(&texture,GPU_NEAREST,GPU_NEAREST);
    C3D_TexSetWrap(&texture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    C3D_TexBind(0,&texture);
    DVLB_s *shader=DVLB_ParseFile((u32*)diagnostic_shader,sizeof(diagnostic_shader));
    if (!shader) { log_event("shader parse failed"); return 1; }
    shaderProgram_s program;
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program,&shader->DVLE[0]);
    C3D_BindProgram(&program);
    C3D_AttrInfo *attributes=C3D_GetAttrInfo();
    AttrInfo_Init(attributes);
    AttrInfo_AddLoader(attributes,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(attributes,1,GPU_FLOAT,2);
    AttrInfo_AddLoader(attributes,2,GPU_FLOAT,4);
    C3D_BufInfo *buffers=C3D_GetBufInfo();
    BufInfo_Init(buffers);
    if (BufInfo_Add(buffers,vertices,sizeof(Vertex),3,0x210)<0) return 1;
    C3D_Mtx projection;
    Mtx_OrthoTilt(&projection,0,400,240,0,0,1,true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,shaderInstanceGetUniformLocation(program.vertexShader,"projection"),&projection);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    for (int i=0;i<6;++i) C3D_TexEnvInit(C3D_GetTexEnv(i));
    Input current={0};
    unsigned captures=0, buttons_seen=0;
    bool capture_ok=true;
    FILE *timings=open_file("timings.csv","w");
    if (!timings) return 1;
    fputs("frame,wall_ms,cpu_submit_ms,gpu_ms,buttons,x,y\n",timings);
    memory_sample();
    log_event("graphics ready");
    u64 previous=svcGetSystemTick();
    while (frames<FRAME_COUNT && aptMainLoop()) {
        if (cpu_pending) { if(is_new) osSetSpeedupEnable(true); cpu_pending=false; }
        hidScanInput();
        if (hidKeysDown()&KEY_START) break;
        if (applied_inputs<input_count && inputs[applied_inputs].frame==frames) {
            current=inputs[applied_inputs++];
            buttons_seen |= current.buttons;
            log_event("script input applied");
        }
        if (!C3D_FrameBegin(C3D_FRAME_SYNCDRAW)) break;
        vertex_count=0;
        quad(20,25,100,100,0.5f,1,1,1,1);
        quad(145,25,85,100,0.25f,0,1,0,1);
        quad(180,50,85,50,0.75f,1,0,0,1);
        quad(285,25,95,100,0.5f,0,0,1,1);
        quad(305,45,55,60,0.5f,1,0,0,0.5f);
        quad(192+current.x,175-current.y,16,16,0.5f,1,1,1,1);
        if (R_FAILED(GSPGPU_FlushDataCache(vertices,vertex_count*sizeof(Vertex)))) break;
        C3D_RenderTargetClear(target,C3D_CLEAR_ALL,0x101820ff,0);
        C3D_FrameDrawOn(target);
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        set_texture(true);
        C3D_DrawArrays(GPU_TRIANGLES,0,6);
        set_texture(false);
        C3D_DrawArrays(GPU_TRIANGLES,6,12);
        C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
        C3D_DrawArrays(GPU_TRIANGLES,18,6);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
        C3D_DrawArrays(GPU_TRIANGLES,24,6);
        C3D_DrawArrays(GPU_TRIANGLES,30,6);
        C3D_FrameEnd(0);
        ++frames;
        u64 now=svcGetSystemTick();
        double wall=(double)(now-previous)*1000.0/SYSCLOCK_ARM11;
        previous=now;
        fprintf(timings,"%u,%.6f,%.6f,%.6f,%u,%d,%d\n",frames,wall,C3D_GetProcessingTime(),C3D_GetDrawingTime(),current.buttons,current.x,current.y);
        if (frames%30==0) { memory_sample(); log_event("heartbeat"); fflush(timings); }
        if (frames==30 || frames==90 || frames==150 || frames==240) {
            capture_ok=capture(target,readback,frames)&&capture_ok;
            ++captures;
        }
    }
    fclose(timings);
    C3D_FrameSync();
    memory_sample();
    bool ok=frames==FRAME_COUNT && sd_ok && capture_ok && applied_inputs==input_count && input_count>0;
    FILE *result=open_file("result.json","w");
    if(result) {
        fprintf(result,"{\"schema\":1,\"pass\":%s,\"frames\":%u,\"new3ds\":%s,\"model_query_result\":%ld,\"cpu_fast_requested\":%s,\"cpu_clock_verified\":false,\"sd_roundtrip\":%s,\"script_events\":%u,\"applied_events\":%u,\"buttons_seen\":%u,\"captures\":%u,\"capture_ok\":%s,\"heap_reserved\":%lu,\"linear_reserved\":%lu,\"heap_peak_used\":%u,\"linear_min_free\":%u,\"vram_min_free\":%u,\"process_commit_limit\":%lld,\"process_commit_peak\":%lld,\"max_vertices\":%u,\"vertices_per_frame\":%u}\n",
                ok?"true":"false",frames,is_new?"true":"false",(long)model_query,is_new?"true":"false",sd_ok?"true":"false",input_count,applied_inputs,buttons_seen,captures,capture_ok?"true":"false",(unsigned long)__ctru_heap_size,(unsigned long)__ctru_linear_heap_size,max_heap,min_linear,min_vram,(long long)max_commit,(long long)peak_commit,MAX_VERTICES,vertex_count);
        fclose(result);
    } else ok=false;
    log_event(ok?"complete":"failed");
    C3D_RenderTargetDelete(target);
    C3D_TexDelete(&texture);
    linearFree(readback); linearFree(vertices);
    shaderProgramFree(&program); DVLB_Free(shader);
    C3D_Fini(); gfxExit(); aptUnhook(&hook);
    if(logfile) fclose(logfile);
    return ok?0:1;
}
