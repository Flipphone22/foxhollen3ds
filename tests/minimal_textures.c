#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "fh_graphics.h"
#include "fh_pica_texture.h"
#include "fh_texture_pack.h"
#include "fh_texture_dependencies.h"
#include "fh_native_model.h"
#include "fh_native_ui.h"
#include "fh_memory_budget.h"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#include "../native-3ds/vendor/melee/scissor.h"
#include <math.h>
#include <zlib.h>
typedef struct {float position[4],color[4],uv[2];} FhMinimalEffectVertex;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef struct C3D_RenderTarget C3D_RenderTarget;
typedef unsigned GPU_TEXCOLOR;
typedef unsigned GPU_TEXTURE_WRAP_PARAM;
enum { FH_MINIMAL_SKY_TINT=2 };
enum { GPU_SCISSOR_DISABLE=0,GPU_SCISSOR_NORMAL };
static unsigned last_scissor[5];
static void C3D_SetScissor(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e){unsigned v[5]={a,b,c,d,e};memcpy(last_scissor,v,sizeof(v));}
enum {GPU_CLAMP_TO_EDGE,GPU_REPEAT,GPU_MIRRORED_REPEAT,GPU_LINEAR,GPU_NEAREST};
typedef struct {void *data;unsigned width,height,size;} C3D_Tex;
static unsigned queued,joins,allocated,fail_allocate;
static unsigned upload_writes,upload_drains,begins;
unsigned fh_minimal_frames;
static unsigned initialized=1,frame_draws,frame_indices,fh_minimal_draws,fh_minimal_indices,fh_minimal_rejected;
typedef int shaderProgram_s;
static shaderProgram_s program,ui_program,world_program;
static float projection[4][4];
static void flush_ui(void);
static void host_reset_ui(void);
u32 __ctru_heap_size=68*1024*1024,__stacksize__=512*1024;
struct host_malloc_info {unsigned uordblks;};
static struct host_malloc_info mallinfo(void){return (struct host_malloc_info){allocated};}
unsigned fh_native_scene_cache_release(void){return 0;}
static unsigned linear_budget=24*1024*1024;
static unsigned linearSpaceFree(void){assert(allocated<=linear_budget);return linear_budget-allocated;}
static unsigned host_cache_bytes;
static void *linearMemAlign(unsigned bytes,unsigned alignment){
    assert(!host_cache_bytes);void *p=aligned_alloc(alignment,(bytes+alignment-1)&~(alignment-1));
    if(p){host_cache_bytes=bytes;allocated+=bytes;}return p;
}
static void linearFree(void *p){if(p){allocated-=host_cache_bytes;host_cache_bytes=0;}free(p);}
static u64 svcGetSystemTick(void){static u64 ticks;return ++ticks;}
static void fh_minimal_sync(void){flush_ui();queued=0;joins++;host_reset_ui();}
static int C3D_TexInit(C3D_Tex *t,unsigned w,unsigned h,unsigned fmt){
    if(fail_allocate){fail_allocate=0;return 0;}
    unsigned bits=fmt==10||fmt==12?4:fmt==7||fmt==9||fmt==13?8:fmt==2||fmt==3||fmt==5?16:32;
    t->data=malloc(w*h*bits/8);if(!t->data)return 0;
    t->width=w;t->height=h;t->size=w*h*bits/8;allocated+=t->size;return 1;
}
static void C3D_TexDelete(C3D_Tex *t){assert(!queued);allocated-=t->size;free(t->data);t->data=NULL;}
static void C3D_TexFlush(C3D_Tex *t){assert(t->data);}
static void C3D_TexSetWrap(C3D_Tex *t,unsigned s,unsigned v){(void)t;(void)s;(void)v;}
static void C3D_TexSetFilter(C3D_Tex *t,unsigned s,unsigned v){(void)t;(void)s;(void)v;}
static int fh_memory_compare(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
typedef struct {uint64_t permutation;unsigned count,fixed;} C3D_AttrInfo;
typedef int C3D_BufInfo;
typedef int C3D_TexEnv;
enum {GPU_SHORT,GPU_UNSIGNED_BYTE,GPU_BYTE,GPU_FLOAT,GPU_VERTEX_SHADER,GPU_PRIMARY_COLOR,GPU_TEXTURE0,GPU_TEXTURE1,GPU_PREVIOUS,GPU_MODULATE,GPU_REPLACE,
    GPU_INTERPOLATE,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA,
    C3D_RGB,C3D_Alpha,C3D_Both,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,GPU_CULL_NONE,
    GPU_ALWAYS,GPU_GEQUAL,GPU_GREATER,GPU_WRITE_ALL,GPU_WRITE_COLOR,GPU_TRIANGLES,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO};
#include <stdbool.h>
static void begin(void){begins++;}
static jmp_buf expected_panic;
static unsigned expect_scope_panic;
static unsigned fh_minimal_enabled=1;
void fh_minimal_texture_written(const void*,size_t);
static void fh_platform_panic(const char *why){
    if((expect_scope_panic==1 && !strcmp(why,"Texture changed inside audited render scope")) ||
       (expect_scope_panic==2 && !strcmp(why,"File texture changed without publication")))longjmp(expected_panic,1);
    fprintf(stderr,"%s\n",why);abort();
}
static void fh_platform_log(const char *message){(void)message;}
static void fh_native_fog_apply(unsigned enabled){assert(!enabled);}
static void C3D_BindProgram(void *p){(void)p;}
static C3D_AttrInfo *C3D_GetAttrInfo(void){static C3D_AttrInfo a;return &a;}
static void AttrInfo_Init(C3D_AttrInfo *a){memset(a,0,sizeof(*a));}
static void AttrInfo_AddLoader(C3D_AttrInfo *a,unsigned b,unsigned c,unsigned d){
    (void)c;(void)d;assert(a->count<12);a->permutation|=(uint64_t)b<<(4*a->count++);
}
static int AttrInfo_AddFixed(C3D_AttrInfo *a,unsigned b){
    assert(b<12 && a->count<12);unsigned id=a->count++;
    a->permutation|=(uint64_t)b<<(4*id);a->fixed|=1u<<id;return id;
}
static void C3D_BoolUnifSet(unsigned type,unsigned id,bool value){(void)type;assert(id>=0x68 && id<0x78);(void)value;}
static unsigned fixed_written;
static void C3D_FixedAttribSet(unsigned a,float b,float c,float d,float e){
    assert(a<12 && isfinite(b) && isfinite(c) && isfinite(d) && isfinite(e));fixed_written|=1u<<a;
}
static C3D_BufInfo *C3D_GetBufInfo(void){static C3D_BufInfo a;return &a;}
static void BufInfo_Init(C3D_BufInfo *a){(void)a;}
static const unsigned char *bound_vertices;
static unsigned bound_stride,bound_count,bound_permutation;
static int BufInfo_Add(C3D_BufInfo *a,const void *b,unsigned c,unsigned d,unsigned e){
    (void)a;bound_vertices=b;bound_stride=c;bound_count=d;bound_permutation=e;return 0;
}
static void GSPGPU_FlushDataCache(const void *a,unsigned b){assert(a && b);}
static void fh_gpu_cache_flush(const void *a,unsigned b){assert(a && b);}
static void C3D_FVUnifSet(unsigned a,unsigned b,float c,float d,float e,float f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}
static C3D_TexEnv *C3D_GetTexEnv(unsigned a){static C3D_TexEnv e;(void)a;return &e;}
static void C3D_TexEnvInit(C3D_TexEnv *a){(void)a;}
static void C3D_TexEnvBufUpdate(unsigned a,unsigned b){(void)a;assert(!b);}
static void C3D_TexEnvSrc(C3D_TexEnv *a,unsigned b,unsigned c,unsigned d,unsigned e){(void)a;(void)b;(void)c;(void)d;(void)e;}
static void C3D_TexEnvFunc(C3D_TexEnv *a,unsigned b,unsigned c){(void)a;(void)b;(void)c;}
static void C3D_TexEnvOpRgb(C3D_TexEnv *a,unsigned b,unsigned c,unsigned d){(void)a;(void)b;(void)c;(void)d;}
static void C3D_TexEnvOpAlpha(C3D_TexEnv *a,unsigned b,unsigned c,unsigned d){(void)a;(void)b;(void)c;(void)d;}
static void C3D_TexBind(unsigned a,C3D_Tex *b){(void)a;(void)b;}
static void C3D_CullFace(unsigned a){(void)a;}
static void C3D_DepthTest(bool a,unsigned b,unsigned c){(void)a;(void)b;(void)c;}
static float depth_scale=-1,depth_offset;
static void C3D_DepthMap(bool a,float b,float c){assert(a);depth_scale=b;depth_offset=c;}
static void C3D_AlphaTest(bool a,unsigned b,unsigned c){(void)a;(void)b;(void)c;}
static void C3D_AlphaBlend(unsigned a,unsigned b,unsigned c,unsigned d,unsigned e,unsigned f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}
static unsigned drawn_vertices,expect_projected_ui;
static void C3D_DrawArrays(unsigned a,unsigned b,unsigned c){(void)a;(void)b;
    unsigned effect=C3D_GetAttrInfo()->fixed==8;
    assert(c%(effect?3:6)==0);
    assert(C3D_GetAttrInfo()->permutation==(effect?0x3210:0x210));
    assert(bound_stride==40 && bound_count==3 && bound_permutation==0x210);
    for(unsigned i=0;i<c;i++)for(unsigned j=0;j<4;j++){
        float value=((const float*)(bound_vertices+i*bound_stride))[j];
        assert(isfinite(value) && (effect || expect_projected_ui || fabsf(value)<=1));
    }
    drawn_vertices+=c;queued=1;
}
static void GPUCMD_GetBuffer(void *a,u32 *capacity,u32 *offset){(void)a;*capacity=65536;*offset=0;}
#include "../native-3ds/vendor/melee/texture_index.h"
#define FH_TEXTURE_INDEX_VERIFY 1
#include "minimal_texture_impl.inc"
static void host_reset_ui(void){ui_used=0;}
int main(void){
    { /* A failed ownership lookup must not suppress dynamic pixel validation. */
        unsigned char owner[128]={0};
        FhTexture image={.pixels=owner+32,.width=8,.height=8,.format=0};
        texture_asset_mode=1;
        assert(base_texture(&image));
        unsigned searches=texture_asset_searches,negative=texture_asset_negative_hits;
        u64 compared=base_compare_bytes;
        for(unsigned i=0;i<100;i++)assert(base_texture(&image));
        assert(texture_asset_searches==searches && texture_asset_negative_hits==negative+100);
        assert(base_compare_bytes==compared+100*32);
        unsigned uploads=base_uploads;
        owner[36]^=0x7f; /* Deliberately unpublished: still detected by equality. */
        assert(base_texture(&image) && base_uploads==uploads+1);
        fh_minimal_texture_asset(owner,&image);
        assert(base_texture(&image));
        unsigned hits=texture_asset_hits;
        assert(base_texture(&image) && texture_asset_hits==hits+1);
        fh_minimal_texture_asset_forget(owner,sizeof(owner));
        assert(base_texture(&image));
        searches=texture_asset_searches;
        assert(base_texture(&image) && texture_asset_searches==searches);
        fh_minimal_texture_asset(owner,&image); /* Same-address registration after a cached miss. */
        assert(base_texture(&image));hits=texture_asset_hits;
        assert(base_texture(&image) && texture_asset_hits==hits+1);
        fh_minimal_texture_asset_forget(owner,sizeof(owner));
        for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
    }
    { /* One file allocation can own independent colour and opacity planes. */
        unsigned char owner[128]={0};
        FhTexture images[2]={{.pixels=owner+32,.width=8,.height=8,.format=14},
                             {.pixels=owner+64,.width=8,.height=8,.format=0}};
        texture_asset_mode=2;
        for(unsigned i=0;i<2;i++)fh_minimal_texture_asset(owner,images+i);
        for(unsigned i=0;i<2;i++)assert(base_texture(images+i));
        unsigned hits=texture_asset_hits;u64 compared=base_compare_bytes;
        for(unsigned i=0;i<2;i++)assert(base_texture(images+i));
        assert(texture_asset_hits==hits+2 && base_compare_bytes==compared);
        owner[65]^=3;fh_minimal_texture_written(owner+65,1);
        unsigned uploads=base_uploads;
        assert(base_texture(images) && base_uploads==uploads);
        assert(base_texture(images+1) && base_uploads==uploads+1);
        fh_minimal_texture_asset_forget(owner,sizeof(owner));
        for(unsigned i=0;i<BASE_TEXTURES;i++)if(base_textures[i].gpu.data){
            assert(!texture_asset_current(base_textures+i));retire_base(base_textures+i);
        }
    }
    const unsigned file_formats[]={0,1,2,3,4,5,6,14};
    for(unsigned f=0;f<sizeof(file_formats)/sizeof(file_formats[0]);f++){
        unsigned char owned[320]={0};FhTexture file={.pixels=owned+32,.width=8,.height=8,.format=file_formats[f]};
        texture_asset_mode=3;unsigned registered=texture_asset_registered;
        fh_minimal_texture_asset(owned,&file);
        assert(texture_asset_registered==registered+(file.format==14));
        texture_asset_mode=2;
        fh_minimal_texture_asset(owned,&file);texture_asset_mode=2;
        C3D_Tex *gpu=base_texture(&file);assert(gpu);
        unsigned hits=texture_asset_hits,checked=texture_asset_checks;u64 compared=base_compare_bytes;
        assert(base_texture(&file)==gpu && texture_asset_hits==hits+1 && texture_asset_checks==checked+1 && base_compare_bytes==compared);
        owned[40]^=7;expect_scope_panic=2;
        if(!setjmp(expected_panic)){base_texture(&file);assert(!"Unpublished file mutation was accepted");}
        expect_scope_panic=0;fh_minimal_texture_written(owned+40,1);
        queued=1;unsigned uploads=base_uploads,before=joins;assert(base_texture(&file));
        assert(base_uploads==uploads+1 && joins>before);
        texture_asset_mode=1;
        /* Lifetime end and same-address reuse must invalidate scoped reuse. */
        fh_minimal_texture_scope_begin();assert(base_texture(&file));
        fh_minimal_texture_asset_forget(owned,sizeof(owned));owned[42]^=3;uploads=base_uploads;
        assert(base_texture(&file) && base_uploads==uploads+1);
        fh_minimal_texture_scope_end();owned[43]^=5;uploads=base_uploads;
        assert(base_texture(&file) && base_uploads==uploads+1); /* unregistered: exact compare */
        fh_minimal_texture_scope_begin();fh_minimal_texture_asset(owned,&file);owned[44]^=9;
        /* A fresh registration forces initial validation even within a scope. */
        uploads=base_uploads;assert(base_texture(&file) && base_uploads==uploads+1);
        fh_minimal_texture_scope_end();
        BaseTexture *entry=NULL;for(unsigned i=0;i<BASE_TEXTURES;i++)if(base_textures[i].source.pixels==file.pixels)entry=base_textures+i;
        assert(entry && texture_asset_current(entry));
        unsigned char *many=calloc(2*TEXTURE_ASSET_SLOTS+1,320);assert(many);
        for(unsigned i=0;i<2*TEXTURE_ASSET_SLOTS+1;i++){
            FhTexture other=file;other.pixels=many+i*320+32;fh_minimal_texture_asset(many+i*320,&other);
        }
        assert(!texture_asset_current(entry)); /* registry capacity cannot reuse old token */
        fh_minimal_texture_asset_forget(many,(2*TEXTURE_ASSET_SLOTS+1)*320);free(many);
        texture_asset_serial=UINT32_MAX;fh_minimal_texture_asset(owned,&file);
        assert(texture_asset_exhausted && !texture_asset_current(entry));
        fh_minimal_texture_asset_forget(owned,sizeof(owned));fh_minimal_sync();
        for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
        memset(texture_assets,0,sizeof(texture_assets));texture_asset_serial=texture_asset_exhausted=0;
    }
    {
        unsigned char owned[256]={0},dynamic[64]={0};
        FhTexture file={.pixels=owned+32,.width=8,.height=8,.format=1};
        FhTexture loose=file;loose.pixels=dynamic;
        texture_asset_mode=5;fh_minimal_texture_asset(owned,&file);
        assert(base_texture(&file) && base_texture(&loose));
        unsigned hits=texture_asset_hits;u64 compared=base_compare_bytes;
        fh_minimal_texture_hint();
        assert(base_texture(&file) && texture_asset_hits==hits+1 && base_compare_bytes==compared);
        assert(base_texture(&loose) && base_compare_bytes==compared+64);
        owned[32]^=1;fh_minimal_texture_written(owned+32,1);
        unsigned uploads=base_uploads;
        assert(base_texture(&file) && base_uploads==uploads+1);
        fh_minimal_texture_hint();owned[32]^=1;expect_scope_panic=2;
        if(!setjmp(expected_panic)){base_texture(&file);assert(!"Unpublished file write escaped hint verification");}
        expect_scope_panic=0;owned[32]^=1;
        /* Unknown CPU writes must invalidate even a file-owned texture. */
        owned[33]^=7;fh_minimal_texture_written(NULL,0);uploads=base_uploads;
        assert(base_texture(&file) && base_uploads==uploads+1);
        fh_minimal_texture_asset_forget(owned,sizeof(owned));
        fh_minimal_texture_hint();hits=texture_asset_hits;
        assert(base_texture(&file) && texture_asset_hits==hits);
        for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
        texture_asset_mode=1;
    }
    {
        /* Same 2.5 MiB source working set, only 160 KiB of compressed GPU
         * data. New 3DS keeps exact snapshots through repeated publication;
         * Old 3DS must evict under its smaller unchanged memory budget. */
        enum { COUNT=20,SOURCE=512*512/2,GPU_BYTES=128*128/2 };
        unsigned char *sources=calloc(COUNT,SOURCE),gpu_bytes[GPU_BYTES];assert(sources);
        memset(gpu_bytes,0x35,sizeof(gpu_bytes));
        texture_pack.file=tmpfile();texture_pack.directory=calloc(1,40);
        texture_pack.file_bytes=SOURCE+GPU_BYTES;
        assert(texture_pack.file && texture_pack.directory);
        texture_pack.count=1;texture_pack.stride=texture_pack.directory_bytes=40;
        unsigned words[10]={crc32(0,sources,SOURCE),512,512,SOURCE,0,12,GPU_BYTES,
                            crc32(0,gpu_bytes,GPU_BYTES),128,128};
        for(unsigned i=0;i<10;i++)for(unsigned b=0;b<4;b++)
            texture_pack.directory[4*i+b]=words[i]>>(24-8*b);
        assert(fwrite(sources,1,SOURCE,texture_pack.file)==SOURCE);
        assert(fwrite(gpu_bytes,1,GPU_BYTES,texture_pack.file)==GPU_BYTES);
        for(unsigned mode=1;mode<=2;mode++){
            texture_asset_mode=mode;
            unsigned evicted=base_budget_evictions,uploads=base_uploads;
            for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<COUNT;i++){
                FhTexture s={.pixels=sources+i*SOURCE,.width=512,.height=512,.format=14};
                if(!pass)fh_minimal_texture_asset(sources+i*SOURCE,&s);
                fh_minimal_texture_written(s.pixels,SOURCE);
                C3D_Tex *t=surface_texture(&s);assert(t && t->size==GPU_BYTES);
                assert(!memcmp(t->data,gpu_bytes,GPU_BYTES));queued=1;
            }
#ifdef FH_OLD_3DS
            assert(base_budget_evictions>evicted && base_uploads>uploads+COUNT);
#else
            assert(base_uploads==uploads+COUNT && base_budget_evictions==evicted);
            assert(base_cpu_bytes==COUNT*SOURCE && base_gpu_bytes==COUNT*GPU_BYTES);
#endif
            fh_minimal_texture_asset_forget(sources,COUNT*SOURCE);
            for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
        }
        texture_asset_mode=1;fh_texture_pack_close(&texture_pack);free(sources);
    }
    {
    /* A room's small textures fit the byte caps. Visiting them twice in one
     * frame must neither upload twice nor discard the most recent material. */
    unsigned char *small=malloc((BASE_TEXTURES+202)*64);assert(small);
    memset(small,0x69,(BASE_TEXTURES+202)*64);
    unsigned uploads_before=base_uploads;
    for(unsigned i=0;i<200;i++){
        FhTexture s={.pixels=small+i*64,.width=8,.height=8,.format=1};assert(base_texture(&s));
    }
    for(unsigned i=0;i<200;i++){
        FhTexture s={.pixels=small+i*64,.width=8,.height=8,.format=1};assert(base_texture(&s));
    }
    assert(base_uploads==uploads_before+200 && base_live==200);
    for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
    for(unsigned i=0;i<BASE_TEXTURES;i++){
        FhTexture s={.pixels=small+i*64,.width=8,.height=8,.format=1};assert(base_texture(&s));
    }
    FhTexture oldest={.pixels=small,.width=8,.height=8,.format=1};
    FhTexture newest=oldest;newest.pixels=small+(BASE_TEXTURES-1)*64;
    FhTexture extra=oldest;extra.pixels=small+BASE_TEXTURES*64;
    uploads_before=base_uploads;queued=1;assert(base_texture(&extra) && !queued);
    assert(base_texture(&newest) && base_uploads==uploads_before+1);
    assert(base_texture(&oldest) && base_uploads==uploads_before+2);
    for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
    assert(!base_live);free(small);
    }
    /* Cold model binding cannot inherit fixed registers from earlier GPU work.
     * This would reject the former unlit/startup binding (only slot5 written). */
    FhModelVertex model_vertex={0};
    for(unsigned compact=0;compact<2;compact++){
        fixed_written=0;bind_vertices(&model_vertex,compact,7);
        C3D_AttrInfo *a=C3D_GetAttrInfo();
        assert(a->count==12 && a->permutation==UINT64_C(0xba9876543210));
        assert(a->fixed==(compact?0xfc0u:0xfe0u) && (fixed_written&a->fixed)==a->fixed);
    }
    unsigned before_joins=joins;
    fh_minimal_frames=7;
    assert(fh_minimal_prepare_write(7)==8 && joins==before_joins && begins==1);
    assert(fh_minimal_prepare_write(8)==8 && joins==before_joins+1 && begins==2);
    assert(upload_writes==2 && upload_drains==1);
    unsigned char bytes[4][513],palettes[4][35];
    for(unsigned a=0;a<4;a++){
        memset(bytes[a],0x12,sizeof(bytes[a]));memset(palettes[a],0x33,sizeof(palettes[a]));
        FhTexture s={.pixels=bytes[a]+a,.width=8,.height=8,.format=1};
        C3D_Tex *gpu=base_texture(&s);assert(gpu);unsigned uploads=base_uploads;
        unsigned char old[64];memcpy(old,gpu->data,64);queued=1;
        assert(base_texture(&s)==gpu && base_uploads==uploads && queued);
        /* An unannounced write still cannot reuse stale pixels. */
        bytes[a][a+63]^=0x77;gpu=base_texture(&s);
        assert(gpu && base_uploads==uploads+1 && !queued && memcmp(old,gpu->data,64));
        queued=1;fh_minimal_texture_written(s.pixels,64);assert(queued);
        uploads=base_uploads;assert(base_texture(&s) && queued && base_uploads==uploads);
        s.format=8;s.palette=palettes[a]+a;s.palette_count=16;s.palette_format=1;
        gpu=base_texture(&s);assert(gpu);memcpy(old,gpu->data,64);queued=1;
        palettes[a][a+2]^=0x55;gpu=base_texture(&s);
        assert(gpu && !queued && memcmp(old,gpu->data,64));
    }
    /* Audited world reuse: exact image/palette compare at each entrance;
     * explicit writes break reuse immediately, unknown writes at boundaries
     * remain detected. Verification mode compares every elided source. */
    {
        FhTexture scoped={.pixels=bytes[0],.width=8,.height=8,.format=8,
            .palette=palettes[0],.palette_count=16,.palette_format=1};
        fh_minimal_texture_scope_begin();assert(base_texture(&scoped));
        u64 compared=base_compare_bytes,hits=texture_scope_hits;
        assert(base_texture(&scoped) && base_compare_bytes==compared && texture_scope_hits==hits+1);
        texture_scope_mode=2;
        u64 verified=texture_scope_checks;
        assert(base_texture(&scoped) && texture_scope_checks==verified+1);
        expect_scope_panic=1;
        if(!setjmp(expected_panic)){
            bytes[0][0]^=1;base_texture(&scoped);
            assert(!"Verification must reject an unannounced in-scope write");
        }
        bytes[0][0]^=1;expect_scope_panic=0;
        unsigned uploads=base_uploads;
        palettes[0][2]^=0x7f;fh_minimal_texture_written(palettes[0]+2,1);
        assert(base_texture(&scoped) && base_uploads==uploads+1);
        fh_minimal_texture_scope_end();bytes[0][0]^=0x3f;
        uploads=base_uploads;
        assert(base_texture(&scoped) && base_uploads==uploads+1);
        fh_minimal_texture_scope_begin();assert(base_texture(&scoped));
        fh_minimal_texture_scope_end();palettes[0][0]^=0x66;
        fh_minimal_texture_scope_begin();uploads=base_uploads;
        assert(base_texture(&scoped) && base_uploads==uploads+1);
        /* Serial wrap must not resurrect an old validation token. */
        texture_scope_serial=UINT32_MAX;
        palettes[0][0]^=0x66;uploads=base_uploads;
        fh_minimal_texture_scope_begin();
        assert(base_texture(&scoped) && base_uploads==uploads+1);
        fh_minimal_texture_scope_end();texture_scope_mode=0;
        fh_minimal_texture_scope_begin();compared=base_compare_bytes;
        assert(base_texture(&scoped) && base_compare_bytes>compared && !texture_scope);
        fh_minimal_texture_scope_end();texture_scope_mode=1;
    }
    /* Reduced world and full-resolution HUD cannot alias cache identity.
     * Exact-source mutation must retire either quality domain behind a fence. */
    unsigned char pack[512]={0},cmpr[128]={0};
    memcpy(pack,"FHTXPK2",8);
    #define PUT32(at,x) do{unsigned v=(x);pack[at]=v>>24;pack[(at)+1]=v>>16;pack[(at)+2]=v>>8;pack[(at)+3]=v;}while(0)
    PUT32(8,2);unsigned pack_end=96;
    for(unsigned i=0;i<2;i++){
        unsigned at=16+40*i,side=i?16:8,n=side*side/2;
        PUT32(at,crc32(0,cmpr,sizeof(cmpr)));PUT32(at+4,16);PUT32(at+8,16);PUT32(at+12,128);
        PUT32(at+16,pack_end);PUT32(at+20,12);PUT32(at+24,n);PUT32(at+32,side);PUT32(at+36,side);
        memset(pack+pack_end+128,0x61+i,n);PUT32(at+28,crc32(0,pack+pack_end+128,n));pack_end+=128+n;
    }
    PUT32(12,crc32(0,pack+16,80));
    const char *pack_path="surface-test.bin";FILE *pack_file=fopen(pack_path,"wb");assert(pack_file);
    assert(fwrite(pack,1,pack_end,pack_file)==pack_end && !fclose(pack_file));
    assert(fh_texture_pack_open(&texture_pack,pack_path,80));
    /* Pressure discards the full cache, retains only a bounded file window,
     * and never waits on GPU work because neither cache is GPU-owned. */
    unsigned saved_budget=linear_budget;
    texture_resident_allocation=1024*1024;
    texture_resident_storage=linearMemAlign(texture_resident_allocation,128);assert(texture_resident_storage);
    assert(fh_texture_pack_resident_load(&texture_pack,texture_resident_storage,texture_resident_allocation));
    linear_budget=allocated+FH_NATIVE_LINEAR_RESERVE;
    unsigned cache_joins=joins;queued=1;
    fh_minimal_texture_cache_trim(FH_NATIVE_LINEAR_RESERVE+8192);
    assert(!texture_resident_storage && texture_window_storage && texture_pack.window);
    assert(texture_window_allocation<=256*1024 && linearSpaceFree()>=FH_NATIVE_LINEAR_RESERVE+8192);
    assert(joins==cache_joins && queued);queued=0;linear_budget=saved_budget;
    FhTexture reduced={.pixels=cmpr,.width=16,.height=16,.format=14};
    C3D_Tex *small=surface_texture(&reduced);assert(small && small->width==8 && small->size==32);
    base_pin=small;C3D_Tex *full=base_texture(&reduced);base_pin=NULL;
    assert(full && full!=small && full->width==16 && full->size==128);
    assert(base_uv_scale(16,8)==1 && base_uv_scale(16,16)==1 && base_uv_scale(12,16)==.75f);
    assert(surface_texture(&reduced)==small && base_texture(&reduced)==full);
    queued=1;cmpr[50]^=1;small=surface_texture(&reduced);
    assert(small && small->width==16 && !queued); /* Unknown content falls back, never stale prepared data. */
    queued=1;full=base_texture(&reduced);assert(full && !queued);
    cache_joins=joins;queued=1;
    fh_minimal_texture_cache_trim(linearSpaceFree()+1);
    assert(!texture_window_storage && !texture_pack.window && !host_cache_bytes && joins==cache_joins && queued);
    fh_texture_pack_close(&texture_pack);assert(!remove(pack_path));
    #undef PUT32
    /* Wrap/filter changes do not alter storage identity. */
    FhTexture s={.pixels=bytes[0],.width=8,.height=8,.format=1};
    assert(base_texture(&s));unsigned uploads=base_uploads;
    s.wrap_s=1;s.wrap_t=2;s.min_filter=s.mag_filter=1;
    assert(base_texture(&s) && base_uploads==uploads);
    /* A CMPR working set that exceeded the old RGBA8 cap now fits without
     * losing texels or changing masks. Second sweep must reuse every image. */
    for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
    {
#ifdef FH_OLD_3DS
        enum {COUNT=40};
#else
        enum {COUNT=80};
#endif
        unsigned char *images=malloc(COUNT*8192);assert(images);memset(images,0x42,COUNT*8192);
        unsigned before_uploads=base_uploads,before_evictions=base_budget_evictions;
        for(unsigned sweep=0;sweep<2;sweep++)for(unsigned i=0;i<COUNT;i++){
            FhTexture cmpr_image={.pixels=images+i*8192,.width=128,.height=128,.format=14};
            C3D_Tex *gpu=surface_texture(&cmpr_image);assert(gpu && gpu->size==32768);queued=1;
        }
        assert(base_uploads==before_uploads+COUNT && base_budget_evictions==before_evictions);
        assert(base_gpu_bytes==COUNT*32768 && base_gpu_bytes*2>BASE_GPU_LIMIT);
        for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
        free(images);
    }
    /* Entry and byte pressure both evict only after a GPU ownership fence. */
    unsigned char *large=malloc(150*8192);memset(large,0x42,150*8192);
    for(unsigned i=0;i<150;i++){
        s=(FhTexture){.pixels=large+i*8192,.width=128,.height=128,.format=14};
        queued=1;fh_minimal_frames++;
        assert(base_texture(&s));assert(base_gpu_bytes<=BASE_GPU_LIMIT && base_cpu_bytes<=BASE_CPU_LIMIT);
    }
    assert(base_retired>50 && joins>50);
    s.pixels=bytes[0];s.width=s.height=8;s.format=1;
    fail_allocate=1;assert(!base_texture(&s));assert(base_texture(&s));
    s.format=255;assert(!base_texture(&s));s.width=2048;assert(!base_texture(&s));
    ui_vertices=malloc(UI_VERTICES*sizeof(*ui_vertices));assert(ui_vertices);
    const float ortho[4][4]={{1.f/320,0,0,-1},{0,-1.f/240,0,1},{0,0,1,0},{0,0,0,1}};
    const float rect[4]={0,0,2560,1920},uv[4]={0,0,1,1};
    const unsigned scissor[4]={160,120,320,240};
    s=(FhTexture){.pixels=bytes[0],.width=8,.height=8,.format=1};
    fh_minimal_quad(&s,ortho,rect,uv,0xffffffff,1,scissor);
    assert(ui_count==6 && fabsf(ui_vertices[0].position[0]-.5f)<.00001f &&
        fabsf(ui_vertices[0].uv[0]-.25f)<.00001f && fabsf(ui_vertices[2].uv[1]-.75f)<.00001f);
    for(unsigned i=0;i<600;i++)fh_minimal_quad(&s,ortho,rect,uv,0xffffffff,1,scissor);
    flush_ui();assert(drawn_vertices==601*6 && ui_draws==3);
    unsigned before=drawn_vertices;
    fh_minimal_quad(&s,ortho,rect,uv,0,1,scissor);flush_ui();assert(drawn_vertices==before);
    fh_minimal_quad(&s,ortho,rect,uv,0xffffffff,1,scissor);
    fh_minimal_texture_written(bytes[0],64);assert(ui_count==0 && drawn_vertices==before+6);
    FhTexture mask={.pixels=bytes[1],.width=8,.height=8,.format=0};
    fh_minimal_image(&s,&mask,ortho,rect,uv,0xffffffff,0,1,scissor);
    assert(ui_count==6 && ui_mask && ui_texture && ui_mask!=ui_texture && ui_additive);
    unsigned masked_draws=ui_draws;
    fh_minimal_texture_written(mask.pixels,32);assert(!ui_count && ui_draws==masked_draws+1);
    /* Zero sky tint is not zero opacity: draw both endpoint blends. */
    before=drawn_vertices;
    fh_minimal_image(&s,NULL,ortho,rect,uv,0x3377bb00,0,FH_MINIMAL_SKY_TINT,scissor);
    assert(ui_count==6 && ui_additive==FH_MINIMAL_SKY_TINT);flush_ui();
    fh_minimal_image(&s,NULL,ortho,rect,uv,0x3377bbff,0,FH_MINIMAL_SKY_TINT,scissor);
    flush_ui();assert(drawn_vertices==before+12);
    /* Original particle order, state boundaries and clip-plane preservation.
     * A near-plane crossing quad stays intact for GPU homogeneous clipping. */
    FhMinimalEffectVertex quad[4]={
        {{-4,-2,-1,1},{1,0,0,.3f},{0,0}},{{4,-2,-1,1},{0,1,0,.4f},{1,0}},
        {{4,2,-10,1},{0,0,1,.5f},{1,1}},{{-4,2,-10,1},{1,1,1,.6f},{0,1}}};
    float view[3][4]={{1,0,0,3},{0,1,0,0},{0,0,1,-9}};
    projection[0][1]=1;projection[1][0]=-1;projection[2][2]=-0.01f;projection[2][3]=-2.5f;projection[3][2]=-1;
    before=drawn_vertices;
    fh_minimal_effect(&s,quad,view,1,-.075f,1,scissor);
    unsigned first=ui_first;
    assert(ui_effect && ui_count==6 && !memcmp(ui_vertices+first,quad,sizeof(*quad)));
    assert(!memcmp(ui_vertices+first+5,quad+3,sizeof(*quad)));
    assert(!memcmp(effect_projection,projection,sizeof(projection)));
    fh_minimal_effect(&s,quad,view,1,-.075f,1,scissor);assert(ui_count==12);
    unsigned batches=effect_draws;
    fh_minimal_effect(&s,quad,view,2,0,1,scissor);assert(effect_draws==batches+1 && ui_count==6);
    fh_minimal_quad(&s,ortho,rect,uv,0xffffffff,0,scissor);
    assert(!ui_effect && effect_draws==batches+2 && ui_count==6);
    assert(depth_scale==-1 && depth_offset==0);flush_ui();
    assert(drawn_vertices==before+24);
    for(unsigned i=0;i<600;i++)fh_minimal_effect(&s,quad,view,2,0,1,scissor);
    flush_ui();assert(drawn_vertices==before+3624);
    fh_minimal_effect(&s,quad,view,0,0,1,scissor);
    fh_minimal_texture_written(s.pixels,64);assert(!ui_count);
    unsigned rejected=fh_minimal_rejected;
    fh_minimal_effect(&s,quad,view,4,0,1,scissor);
    assert(fh_minimal_rejected==rejected+1 && !ui_count);
    /* Precipitation triangles share the same ordered stream with billboards.
     * Three vertices must remain three (no synthetic duplicate triangle),
     * including negative UVs and the original local-to-view transform. */
    before=drawn_vertices;unsigned triangles=effect_triangles;
    quad[0].uv[0]=-.375f;quad[2].uv[1]=2;
    fh_minimal_effect_triangle(&s,quad,view,2,0,1,scissor);
    first=ui_first;
    assert(ui_count==3 && !memcmp(ui_vertices+first,quad,3*sizeof(*quad)));
    fh_minimal_effect(&s,quad,view,2,0,1,scissor);
    assert(ui_count==9 && effect_triangles==triangles+3);
    assert(!memcmp(effect_view,view,sizeof(view)));flush_ui();
    for(unsigned i=0;i<1100;i++)fh_minimal_effect_triangle(&s,quad,view,2,0,1,scissor);
    flush_ui();assert(drawn_vertices==before+3309 && effect_triangles==triangles+1103);
    /* Perspective menu text/icons retain homogeneous coordinates and batch
     * scissors, with the same source ownership as existing HUD quads. */
    expect_projected_ui=1;
    unsigned partial[4]={40,60,300,220},clip[4];
    assert(mp_scissor_rect(partial[0],partial[1],partial[2],partial[3],clip));
    unsigned projected=ui_projected_quads;
    float projected_position[4][4];
    assert(fh_native_ui_project(projected_position,projection,view,rect,24));
    fh_minimal_projected_image(&s,NULL,view,rect,24,uv,0xaabbcc80,1,0,partial);
    assert(ui_scissored && !ui_effect && ui_count==6);
    assert(!memcmp(ui_vertices[ui_first].position,projected_position[0],16));
    assert(!memcmp(ui_vertices[ui_first+5].position,projected_position[3],16));
    assert(ui_vertices[ui_first].position[3]!=1 && ui_vertices[ui_first].color[3]==128/255.f);
    fh_minimal_projected_image(&s,NULL,view,rect,24,uv,0xffffffff,1,0,partial);
    assert(ui_count==12 && ui_projected_quads==projected+2);flush_ui();
    assert(last_scissor[0]==GPU_SCISSOR_NORMAL && !memcmp(last_scissor+1,clip,sizeof(clip)));
    fh_minimal_projected_image(&s,NULL,view,rect,24,uv,0xffffffff,1,0,partial);
    fh_minimal_quad(&s,ortho,rect,uv,0xffffffff,0,scissor);
    assert(!ui_scissored && !ui_effect && ui_count==6);flush_ui();
    assert(last_scissor[0]==GPU_SCISSOR_DISABLE);
    rejected=fh_minimal_rejected;float invalid[4]={NAN,0,100,100};
    fh_minimal_projected_image(&s,NULL,view,invalid,24,uv,0xffffffff,1,0,partial);
    assert(fh_minimal_rejected==rejected+1 && !ui_count);
    expect_projected_ui=0;
    /* Both images are alive across cache pressure, even before submission. */
    base_pin=base_texture(&s);assert(base_pin);void *pinned_data=base_pin->data;
    for(unsigned i=0;i<150;i++){
        FhTexture pressure={.pixels=large+i*8192,.width=128,.height=128,.format=14};
        fh_minimal_frames++;assert(base_texture(&pressure));assert(base_pin->data==pinned_data);
    }
    /* Scene admission may reclaim texture storage, but never its pinned
     * material sources, and every retirement drains queued GPU readers. */
    texture_resident_storage=malloc(16384);assert(texture_resident_storage);
    texture_resident_allocation=host_cache_bytes=16384;allocated+=host_cache_bytes;
    texture_pack.resident=texture_resident_storage;
    texture_pack.resident_offsets=malloc(8);assert(texture_pack.resident_offsets);
    texture_pack.resident_bytes=16384;
    linear_budget=allocated+FH_NATIVE_LINEAR_RESERVE;queued=1;before_joins=joins;
    assert(fh_minimal_linear_room(8192));
    assert(!texture_resident_allocation && !texture_pack.resident && !host_cache_bytes);
    assert(joins==before_joins && queued && base_pin->data==pinned_data);
    assert(!fh_minimal_texture_cache_release());
    linear_budget=allocated+FH_NATIVE_LINEAR_RESERVE;
    queued=1;before_joins=joins;
    assert(fh_minimal_linear_room(8192));
    assert(joins>before_joins && base_pin->data==pinned_data);
    assert(!fh_minimal_linear_room(UINT32_MAX));
    assert(!fh_minimal_linear_room(linear_budget));
    assert(base_pin->data==pinned_data);
    linear_budget=24*1024*1024;base_pin=NULL;
    fh_minimal_sync();free(ui_vertices);
    queued=1;
    for(unsigned i=0;i<BASE_TEXTURES;i++)retire_base(base_textures+i);
    assert(!allocated && !base_cpu_bytes && !base_gpu_bytes);free(large);
    printf("native base textures: alignment/mutation/palette/eviction/failure checks passed, %u joins\n",joins);
}
