#include "gx_internal.h"
#include "fh_gpu_lighting.h"
#include "fh_gpu_texgen.h"
#include "fh_geometry_cache.h"
#include "fh_retained_geometry.h"
#include "fh_geometry_sets.h"
#include "fh_gx_source_ranges.h"
#include "fh_compiled_geometry.h"
#include "fh_vertex_copy.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <dolphin/vi.h>
volatile unsigned test_checks,test_failure;
/* Functional ARM fixture has no OS timer or performance claim. Production
 * attribution remains disabled; link its timer dependency explicitly. */
unsigned fh_render_attribution_active;
uint64_t svcGetSystemTick(void){return 0;}
/* This instruction fixture has no asynchronous dispatcher or deferred owner. */
void fh_gx_dispatch_join_deferred(void){}
__attribute__((noinline,noreturn)) void test_pass(void) { for(;;) __asm__ volatile("nop"); }
__attribute__((noinline,noreturn)) void test_fail(unsigned line) { test_failure=line;for(;;) __asm__ volatile("nop"); }
#define CHECK(v) do { if(!(v)) test_fail(__LINE__);++test_checks; } while(0)
void fh_platform_panic(const char *message) { (void)message;test_fail(9999); }
static FhVertex captured[16384];static unsigned vertex_count,draw_calls;
static unsigned gpu_transform_available;
unsigned fh_gpu_sphere_disabled=1;
unsigned fh_light_lod_enabled,fh_light_lod_draws,fh_light_lod_source_lights,fh_light_lod_palette_draws;
extern unsigned fh_texgen_cache_disabled,fh_texgen_cache_hits,fh_texgen_cache_misses;
int fh_renderer_transform_available(void){return gpu_transform_available;}
static unsigned accept_points,point_calls;
int fh_renderer_points_available(const FhDrawState *s){(void)s;return accept_points;}
void fh_renderer_draw_points(const FhVertex *v,unsigned n,const FhDrawState *s,unsigned w,unsigned m,float t){
    CHECK(accept_points && n<=384 && vertex_count+n<=16384 && s->raw_position && s->cull==0);
    CHECK(w==18 && m==1 && t==.5f);memcpy(captured+vertex_count,v,n*sizeof(*v));vertex_count+=n;point_calls++;
}
static unsigned cpu_cull_available;
int fh_renderer_cpu_cull_available(const FhDrawState *state){(void)state;return cpu_cull_available;}
static unsigned gpu_cull_available;
int fh_renderer_gpu_cull_available(const FhDrawState *state){(void)state;return gpu_cull_available;}
static unsigned accept_lighting;
volatile unsigned lighting_benchmark;
volatile unsigned lighting_cache_benchmark,lighting_cache_signature;
static FhGpuLighting lighting_prepared;
static unsigned accept_texgen,omit_texgens;
static unsigned material_texgen_inputs=255;
static unsigned accept_compiled;
static unsigned accept_compiled_fur;
int fh_renderer_compiled_fur_available(const FhDrawState *s){return accept_compiled_fur && s->texgens==5;}
int fh_renderer_fur_texgen_available(const FhDrawState *s){return fh_renderer_compiled_fur_available(s);}
static unsigned compiled_post_seen;
static float compiled_post_seen_matrix[3][4];
int fh_renderer_draw_compiled(const FhCompiledMesh *mesh,const FhDrawState *state,FhCompiledDecode decode,FhCompiledDecodeBatch batch){
    (void)state;if(!accept_compiled)return 0;
    compiled_post_seen=mesh->fur_post!=NULL;
    if(mesh->fur_post)memcpy(compiled_post_seen_matrix,mesh->fur_post,48);
    static FhVertex unique[8192];
    CHECK(mesh->unique<=8192 && mesh->count<=16384);
    if(batch){
        unsigned before=vertex_count;uint64_t probe=batch(NULL,mesh);CHECK(vertex_count==before);
        uint64_t decoded=batch(unique,mesh);CHECK(!probe || probe==decoded);
    }else for(unsigned i=0;i<mesh->unique;i++)decode(unique+i,mesh->packets+mesh->offsets[i]);
    for(unsigned i=0;i<mesh->count;i++){CHECK(mesh->indices[i]<mesh->unique);captured[vertex_count++]=unique[mesh->indices[i]];}
    draw_calls++;return 1;
}
static float texgen_prepared[3][4];
void *fh_renderer_retained_alloc(unsigned bytes){
    static unsigned char storage[4*1024*1024] __attribute__((aligned(32)));
    return bytes==sizeof(storage)?storage:NULL;
}
void *fh_renderer_cpu_cache_alloc(unsigned bytes){
    static unsigned char bounds[1024*1024] __attribute__((aligned(32)));
    return bytes==sizeof(bounds)?bounds:NULL;
}
int fh_renderer_replay_retained(uint64_t id,unsigned count,const FhDrawState *state){(void)id;(void)count;(void)state;return 0;}
int fh_renderer_replay_retained_points(uint64_t id,unsigned count,const FhDrawState *state,unsigned width,unsigned offsets,float t){(void)id;(void)count;(void)state;(void)width;(void)offsets;(void)t;return 0;}
void fh_renderer_draw_retained_points(uint64_t id,const FhVertex *v,unsigned count,const FhDrawState *state,unsigned width,unsigned offsets,float t){(void)id;fh_renderer_draw_points(v,count,state,width,offsets,t);}
void fh_renderer_draw_retained(uint64_t id,const FhVertex *v,unsigned count,const FhDrawState *state,const uint32_t *ids){(void)id;fh_renderer_draw_identified(v,count,state,ids);}
int fh_renderer_prepare_texgen(const FhDrawState *s,const float post[3][4]){(void)s;if(accept_texgen)memcpy(texgen_prepared,post,sizeof(texgen_prepared));return accept_texgen;}
int fh_renderer_texgens_unused(const FhDrawState *s){(void)s;return omit_texgens;}
unsigned fh_renderer_texgen_inputs(const FhDrawState *s){(void)s;return material_texgen_inputs;}
int fh_renderer_prepare_uvgen(const FhDrawState *s,const float matrix[3][4]){(void)s;if(accept_texgen)memcpy(texgen_prepared,matrix,sizeof(texgen_prepared));return accept_texgen;}
int fh_renderer_prepare_lighting(const FhDrawState *s,const FhGpuLighting *l){(void)s;if(accept_lighting)lighting_prepared=*l;return accept_lighting;}
unsigned fh_renderer_material_scope_begin(const FhDrawState *s){(void)s;return 0;}
void fh_renderer_material_scope_end(unsigned *entered){(void)entered;}
void fh_renderer_material_scope_invalidate(void){}
const FhGpuLighting *fh_renderer_prepared_lighting(void){return &lighting_prepared;}
static unsigned event;
void fh_renderer_sync(void) {CHECK(event==1);event=2;}
static void breakpoint_callback(void) {CHECK(event==2);event=3;}
void fh_renderer_present(void) {CHECK(event==3 && GXReadDrawSync()==123);event=4;}
void fh_renderer_clear(uint32_t color,uint32_t depth) {(void)color;(void)depth;CHECK(event==4);event=5;}
void VIWaitForRetrace(void) {CHECK(event==5);event=6;}
void fh_renderer_draw(const FhVertex *vertices,unsigned count,const FhDrawState *state) {
    CHECK(vertex_count+count<=16384 && state==&fh_gx.draw);draw_calls++;
    memcpy(captured+vertex_count,vertices,count*sizeof(*vertices));vertex_count+=count;
}
void fh_renderer_draw_identified(const FhVertex *vertices,unsigned count,const FhDrawState *state,const uint32_t *identities) {
    unsigned slots[1024]={0};
    for(unsigned i=0;i<count;i++)if(identities[i]) {
        unsigned slot=identities[i]&1023;
        while(slots[slot] && identities[slots[slot]-1]!=identities[i])slot=(slot+1)&1023;
        if(slots[slot])CHECK(!memcmp(vertices+i,vertices+slots[slot]-1,sizeof(*vertices)));
        else slots[slot]=i+1;
    }
    fh_renderer_draw(vertices,count,state);
}
void fh_renderer_invalidate(const void *address,size_t bytes) { (void)address;(void)bytes; }
void fh_renderer_texture_hint(void) {}
static int near(float a,float b) { float diff=a-b;return diff>-.0001f && diff<.0001f; }
/* Pre-optimization scalar lighting oracle: independent channel/part loops. */
static void reference_lighting(float output[2][4],const float color[2][4],const float position[3],const float normal[3]) {
    for(unsigned channel=0;channel<2;channel++)for(unsigned part=0;part<2;part++){
        const FhChannel *state=&fh_gx.channel[channel+part*2];
        const float *material=state->material_source==GX_SRC_VTX?color[channel]:fh_gx.material[channel];
        const float *ambient=state->ambient_source==GX_SRC_VTX?color[channel]:fh_gx.ambient[channel];
        float light[4];memcpy(light,ambient,sizeof(light));
        if(state->enabled)for(unsigned i=0;i<8;i++)if(state->mask&(1u<<i)){
            const FhLight *source=&fh_gx.lights[i];float delta[3];
            for(unsigned j=0;j<3;j++)delta[j]=source->position[j]-position[j];
            float distance=sqrtf(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
            if(distance>0)for(unsigned j=0;j<3;j++)delta[j]/=distance;
            float dot=normal[0]*delta[0]+normal[1]*delta[1]+normal[2]*delta[2];
            float diffuse=state->diffuse==GX_DF_NONE?1:dot;
            if(state->diffuse==GX_DF_CLAMP && diffuse<0)diffuse=0;
            float attenuation=1;
            if(state->attenuation!=GX_AF_NONE){
                const float *v=state->attenuation==GX_AF_SPEC?normal:delta;
                float cosine=v[0]*source->direction[0]+v[1]*source->direction[1]+v[2]*source->direction[2];
                if(cosine<0 || (state->attenuation==GX_AF_SPEC && dot<0))cosine=0;
                float d=state->attenuation==GX_AF_SPEC?cosine:distance;
                float numerator=source->a[0]+source->a[1]*cosine+source->a[2]*cosine*cosine;
                float denominator=source->k[0]+source->k[1]*d+source->k[2]*d*d;
                attenuation=numerator>0 && denominator>0?numerator/denominator:0;
            }
            for(unsigned j=0;j<4;j++)light[j]+=source->color[j]*diffuse*attenuation;
        }
        for(unsigned j=part?3:0;j<(part?4:3);j++)output[channel][j]=material[j]*(state->enabled?(light[j]<0?0:light[j]>1?1:light[j]):1);
    }
}
static void use_both_raster_channels(void) {
    GXSetNumTevStages(2);
    GXSetTevOrder(GX_TEVSTAGE1,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR1A1);
    GXSetTevColorIn(GX_TEVSTAGE1,GX_CC_RASC,GX_CC_CPREV,GX_CC_HALF,GX_CC_ZERO);
    GXSetTevAlphaIn(GX_TEVSTAGE1,GX_CA_RASA,GX_CA_APREV,GX_CA_KONST,GX_CA_ZERO);
}
static void lighting_regression(void){
    fh_gx_reset();gpu_transform_available=0;use_both_raster_channels();
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    for(unsigned sample=0;sample<54;sample++){
        for(unsigned i=0;i<8;i++){
            FhLight *l=&fh_gx.lights[i];
            l->position[0]=i==0?2:i-3.f;l->position[1]=i==0?3:2.f*i;l->position[2]=i==0?-4:(i&1)?-10:10;
            l->direction[0]=.2f;l->direction[1]=-.4f;l->direction[2]=.8f;
            l->a[0]=sample%5==0?-1:.25f;l->a[1]=.3f;l->a[2]=.4f;
            l->k[0]=sample%7==0?0:1;l->k[1]=.1f;l->k[2]=.01f;
            for(unsigned c=0;c<4;c++)l->color[c]=((i+c+sample)%9)/32.f;
        }
        for(unsigned ch=0;ch<4;ch++){
            unsigned shared=sample<27?0:ch;
            fh_gx.channel[ch]=(FhChannel){sample%9!=0 || ch==0,ch&1,(ch>>1)&1,(sample*17+shared*31)&255,(sample+shared)%3,(sample/3+shared)%3};
            fh_gx.ambient[ch&1][ch/2]=.1f;
        }
        for(unsigned ch=0;ch<2;ch++)for(unsigned c=0;c<4;c++){
            fh_gx.ambient[ch][c]=(ch+c)/32.f;fh_gx.material[ch][c]=(ch+c+1)/8.f;
        }
        float expected[2][4],colors[2][4]={{1,1,1,1},{1,1,1,1}},position[]={2,3,-4},normal[]={0,0,1};
        reference_lighting(expected,colors,position,normal);
        vertex_count=0;GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned v=0;v<3;v++){GXPosition3f32(2,3,-4);GXNormal3f32(0,0,1);}GXEnd();
        for(unsigned c=0;c<8;c++)CHECK(((float*)captured[0].color)[c]==((float*)expected)[c]);
        /* A new primitive may change only TEV/texture state, as fur shells do.
         * All channels, masks and scalar edge cases above keep their oracle. */
        GXSetTevKColor(GX_KCOLOR0,(GXColor){sample,2,3,4});
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned v=0;v<3;v++){GXPosition3f32(2,3,-4);GXNormal3f32(0,0,1);}GXEnd();
        for(unsigned v=3;v<6;v++)for(unsigned c=0;c<8;c++)CHECK(((float*)captured[v].color)[c]==((float*)expected)[c]);
    }
}
static void lighting_cache_regression(void){
    fh_gx_reset();gpu_transform_available=0;vertex_count=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_NRM,GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    GXSetChanCtrl(GX_COLOR0A0,GX_TRUE,GX_SRC_REG,GX_SRC_REG,63,GX_DF_CLAMP,GX_AF_SPOT);
    GXSetChanMatColor(GX_COLOR0A0,(GXColor){140,190,90,210});
    GXSetChanAmbColor(GX_COLOR0A0,(GXColor){20,15,10,5});
    float positions[33][3],normals[33][3];u8 list[3+33*2]={GX_TRIANGLES,0,33};
    for(unsigned i=0;i<33;i++) {
        positions[i][0]=(i%6)*.75f;positions[i][1]=(i/6)*.5f;positions[i][2]=-4;
        normals[i][0]=normals[i][1]=0;normals[i][2]=1;list[3+i*2]=list[4+i*2]=i;
    }
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);GXSetArray(GX_VA_NRM,normals,sizeof(normals),12,true);
    for(unsigned i=0;i<8;i++) {
        FhLight *l=&fh_gx.lights[i];memset(l,0,sizeof(*l));
        l->position[0]=i-3.f;l->position[1]=2.f*i;l->position[2]=10;
        l->a[0]=1;l->k[0]=1;l->k[1]=.0033333f;l->k[2]=.000066667f;
        for(unsigned c=0;c<4;c++)l->color[c]=(i+c+1)/32.f;
    }
    unsigned signature=2166136261u;
    for(unsigned shell=0;shell<24;shell++) {
        /* First sixteen draws share lighting, like repeated fur shells.
         * Then change each value dependency while retaining source addresses. */
        if(shell==16)positions[0][0]+=.125f;
        if(shell==17)normals[1][2]=-1;
        if(shell==18)fh_gx.ambient[0][0]+=.1f;
        if(shell==19)fh_gx.material[0][3]*=.5f;
        if(shell==20)fh_gx.lights[5].position[0]+=10;
        if(shell==21)fh_gx.lights[5].color[1]*=.5f;
        if(shell==22)fh_gx.channel[0].mask=0xa5;
        if(shell==23)fh_gx.channel[0].attenuation=GX_AF_NONE;
        GXSetTevKColor(GX_KCOLOR0,(GXColor){shell,2,3,4});vertex_count=0;
        lighting_cache_benchmark=shell<16;GXCallDisplayList(list,sizeof(list));lighting_cache_benchmark=0;
        CHECK(vertex_count==33);
        for(unsigned v=0;v<33;v++) {
            float expected[2][4],colors[2][4]={{1,1,1,1},{1,1,1,1}};
            reference_lighting(expected,colors,positions[v],normals[v]);
            for(unsigned c=0;c<4;c++) {
                CHECK(captured[v].color[0][c]==expected[0][c]);
                unsigned word;memcpy(&word,&captured[v].color[0][c],4);signature=(signature^word)*16777619u;
            }
        }
    }
    lighting_cache_signature=signature;
}
static void two_channel_cache_regression(void){
    extern unsigned fh_cpu_light_hits,fh_cpu_light_misses;
    unsigned hits=fh_cpu_light_hits,misses=fh_cpu_light_misses;
    fh_gx_reset();use_both_raster_channels();gpu_transform_available=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR1,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR1,GX_CLR_RGBA,GX_RGBA8,0);
    for(unsigned state=0;state<48;state++){
        unsigned variant=state<8?0:state<40?state-7:0; /* Hits, state eviction, old-state reuse. */
        for(unsigned c=0;c<4;c++){
            GXSetChanCtrl(c,GX_TRUE,variant&1,(variant>>1)&1,(variant&4)?0xa5:0x3f,c<2?GX_DF_NONE:GX_DF_CLAMP,c<2?GX_AF_SPEC:GX_AF_SPOT);
            for(unsigned k=0;k<4;k++){fh_gx.material[c&1][k]=.2f+(variant%5+k)*.05f;fh_gx.ambient[c&1][k]=.01f*(variant%7+k);}
        }
        for(unsigned l=0;l<8;l++){
            FhLight *v=fh_gx.lights+l;memset(v,0,sizeof(*v));
            v->position[0]=l-3.f+.03125f*variant;v->position[1]=l;v->position[2]=10;
            v->direction[0]=.01f*variant;v->direction[1]=.1f;v->direction[2]=.8f;
            v->a[0]=.1f;v->a[1]=.02f*variant;v->a[2]=.5f;
            v->k[0]=1;v->k[1]=.03f;v->k[2]=.002f*(variant+1);
            for(unsigned k=0;k<4;k++)v->color[k]=(l+k+1)/32.f;
        }
        for(unsigned pass=0;pass<3;pass++){
            float positions[3][3],normals[3][3]={{0,0,1},{0,1,0},{1,0,0}};
            const float input[2][4]={{51/255.f,102/255.f,153/255.f,204/255.f},{204/255.f,153/255.f,102/255.f,51/255.f}};
            vertex_count=0;GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
            for(unsigned v=0;v<3;v++){
                positions[v][0]=v+.125f*(variant%3);positions[v][1]=1;positions[v][2]=-2;
                GXPosition3f32(positions[v][0],1,-2);GXNormal3f32(normals[v][0],normals[v][1],normals[v][2]);
                GXColor4u8(51,102,153,204);GXColor4u8(204,153,102,51);
            }
            GXEnd();CHECK(vertex_count==3);
            for(unsigned v=0;v<3;v++){
                float expected[2][4];reference_lighting(expected,(float(*)[4])input,positions[v],normals[v]);
                for(unsigned c=0;c<8;c++)CHECK(((float*)captured[v].color)[c]==((float*)expected)[c]);
            }
        }
    }
    CHECK(fh_cpu_light_hits>hits+200 && fh_cpu_light_misses>misses+80);
}
static void flat_lighting_regression(void){
    fh_gx_reset();use_both_raster_channels();
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
    /* Independent register/vertex material and ambient, enabled/disabled,
     * empty/multiple light masks, both transform paths, and changing state. */
    for(unsigned sample=0;sample<32;sample++){
        gpu_transform_available=sample>>4;
        GXSetChanCtrl(GX_COLOR0A0,(sample>>2)&1,sample&1,(sample>>1)&1,(sample&8)?0x85:0,GX_DF_NONE,GX_AF_NONE);
        GXSetChanMatColor(GX_COLOR0A0,(GXColor){30+sample,80,180,150});
        GXSetChanAmbColor(GX_COLOR0A0,(GXColor){20,40+sample,60,80});
        for(unsigned light=0;light<8;light++)for(unsigned c=0;c<4;c++)fh_gx.lights[light].color[c]=(light+c)/16.f;
        vertex_count=0;GXBegin(GX_TRIANGLES,GX_VTXFMT0,96);
        for(unsigned v=0;v<96;v++){GXPosition3f32(2,3,-4);GXNormal3f32(0,0,2);GXColor4u8(20+v,70,100,200-v);}
        GXEnd();
        for(unsigned v=0;v<96;v++){
            float colors[2][4]={{(20+v)/255.f,70/255.f,100/255.f,(200-v)/255.f},{1,1,1,1}},expected[2][4];
            const float position[]={2,3,-4},normal[]={0,0,1};reference_lighting(expected,colors,position,normal);
            for(unsigned c=0;c<8;c++)CHECK(((float*)captured[v].color)[c]==((float*)expected)[c]);
            CHECK(near(captured[v].position[0],2)&&near(captured[v].position[1],3)&&near(captured[v].position[2],-4));
        }
    }
    /* Specular attenuation's diffuse field is forced off by the GX API. */
    for(unsigned channel=0;channel<6;channel++)for(unsigned diffuse=0;diffuse<3;diffuse++){
        GXSetChanCtrl(channel,GX_TRUE,GX_SRC_REG,GX_SRC_REG,0xff,diffuse,GX_AF_SPEC);
        CHECK(fh_gx.channel[channel<4?channel:channel&1].diffuse==GX_DF_NONE);
        if(channel>=4)CHECK(fh_gx.channel[2+(channel&1)].diffuse==GX_DF_NONE);
    }
}
/* Enabled lighting channels that no TEV stage/texgen consumes must not
 * constrain position transforms. Verify all consumed colors against full
 * scalar lighting, including color-driven texgens and alpha-only TEV inputs. */
static void raster_dependency_regression(void) {
    for(unsigned sample=0;sample<24;sample++) {
        fh_gx_reset();gpu_transform_available=0;vertex_count=0;
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
        unsigned mask=sample&3,texgen=sample/4;
        GXSetNumTevStages(2);
        for(unsigned ch=0;ch<2;ch++) {
            GXSetTevOrder(ch,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,ch?GX_COLOR1A1:GX_COLOR0A0);
            GXSetTevColorIn(ch,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,(mask&(1u<<ch)) && texgen%2==0?GX_CC_RASC:GX_CC_ZERO);
            GXSetTevAlphaIn(ch,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,(mask&(1u<<ch)) && texgen%2?GX_CA_RASA:GX_CA_ZERO);
            GXSetChanCtrl(ch?GX_COLOR1A1:GX_COLOR0A0,GX_TRUE,GX_SRC_REG,GX_SRC_REG,7,GX_DF_CLAMP,GX_AF_NONE);
            GXSetChanMatColor(ch?GX_COLOR1A1:GX_COLOR0A0,(GXColor){180,140,90,210});
            GXSetChanAmbColor(ch?GX_COLOR1A1:GX_COLOR0A0,(GXColor){20,30,40,50});
        }
        for(unsigned i=0;i<3;i++) {
            GXLightObj light;GXInitLightPos(&light,i-1.f,2+i,8-i);
            GXInitLightColor(&light,(GXColor){40,30+i,20,35});GXLoadLightObjImm(&light,1u<<i);
        }
        if(texgen>=2) {
            unsigned ch=(texgen-2)&1;mask|=1u<<ch;
            GXSetNumTexGens(1);GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_SRTG,ch?GX_TG_COLOR1:GX_TG_COLOR0,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
        }
        float colors[2][4]={{1,1,1,1},{1,1,1,1}},expected[2][4];
        const float pos[]={2,3,-4},normal[]={0,0,1};reference_lighting(expected,colors,pos,normal);
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,24);
        for(unsigned v=0;v<24;v++){GXPosition3f32(2,3,-4);GXNormal3f32(0,0,1);}GXEnd();
        CHECK(vertex_count==24);
        for(unsigned v=0;v<24;v++) {
            for(unsigned ch=0;ch<2;ch++)if(mask&(1u<<ch))for(unsigned c=0;c<4;c++)CHECK(captured[v].color[ch][c]==expected[ch][c]);
            if(texgen>=2)for(unsigned c=0;c<2;c++)CHECK(captured[v].uv[0][c]==expected[(texgen-2)&1][c]);
        }
    }
}
/* Lighting retains its eye-space inputs even when PICA handles projection. */
static void lit_projection_regression(void) {
    FhVertex reference[6];
    const float pos[3][4]={{2,0,0,7},{0,3,0,-4},{0,0,2,-8}};
    const float nrm[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    const float projection[4][4]={{.2f,0,0,0},{0,.3f,0,0},{0,0,.25f,0},{0,0,0,1}};
    for(unsigned gpu=0;gpu<2;gpu++) {
        fh_gx_reset();gpu_transform_available=gpu;vertex_count=0;use_both_raster_channels();
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_PNMTXIDX,GX_DIRECT);GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
        for(unsigned m=0;m<2;m++){GXLoadPosMtxImm((MtxPtr)pos,m?30:3);GXLoadNrmMtxImm((MtxPtr)nrm,m?30:3);}
        memcpy(fh_gx.projection,projection,sizeof(projection));
        for(unsigned ch=0;ch<2;ch++) {
            GXSetChanCtrl(ch?GX_COLOR1A1:GX_COLOR0A0,GX_TRUE,GX_SRC_REG,GX_SRC_REG,3,GX_DF_CLAMP,GX_AF_SPOT);
            GXSetChanMatColor(ch?GX_COLOR1A1:GX_COLOR0A0,(GXColor){180,100,220,200});
            GXSetChanAmbColor(ch?GX_COLOR1A1:GX_COLOR0A0,(GXColor){10,20,30,40});
        }
        for(unsigned i=0;i<2;i++) {
            GXLightObj light;GXInitLightPos(&light,2+i*3,1,5);
            GXInitLightDir(&light,0,0,-1);GXInitLightColor(&light,(GXColor){60,40,20,100});
            GXInitLightAttn(&light,1,.2f,.1f,1,.01f,.001f);GXLoadLightObjImm(&light,1u<<i);
        }
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,6);
        for(unsigned v=0;v<6;v++) {
            u8 matrix=v<3?3:30;fh_gx_fifo_write(&matrix,1);GXPosition3f32((v%3)-1.f,.5f,-2);GXNormal3f32(0,0,1);
        }
        GXEnd();CHECK(vertex_count==6);
        if(!gpu){memcpy(reference,captured,sizeof(reference));continue;}
        CHECK(fh_gx.draw.raw_position);
        for(unsigned v=0;v<6;v++) {
            CHECK(!memcmp(captured[v].color,reference[v].color,sizeof(reference[v].color)));
            CHECK(captured[v].matrix_index==(v<3?3:-1));
            if(v<3) {
                float eye[4]={0,0,0,1},clip[4]={0};
                for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)eye[r]+=fh_gx.draw.model_palette[3+r][c]*captured[v].position[c];
                for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)clip[r]+=fh_gx.draw.position_matrix[r][c]*eye[c];
                for(unsigned c=0;c<4;c++)CHECK(near(clip[c],reference[v].position[c]));
            } else CHECK(!memcmp(captured[v].position,reference[v].position,sizeof(reference[v].position)));
        }
    }
}
static void deferred_lighting_regression(void) {
    FhVertex reference[3];
    for(unsigned test=0;test<12;test++)for(unsigned mode=0;mode<2;mode++) {
        fh_gx_reset();gpu_transform_available=1;accept_lighting=mode;vertex_count=0;
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC);
        GXSetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_RASA);
        GXSetChanCtrl(GX_COLOR0,GX_TRUE,GX_SRC_REG,test==1?GX_SRC_REG:GX_SRC_VTX,255,GX_DF_CLAMP,GX_AF_SPOT);
        GXSetChanCtrl(GX_ALPHA0,GX_FALSE,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
        GXSetChanMatColor(GX_COLOR0A0,(GXColor){200,120,80,170});GXSetChanAmbColor(GX_COLOR0A0,(GXColor){10,20,30,40});
        for(unsigned i=0;i<8;i++) {
            GXLightObj light;GXInitLightPos(&light,2+i*3,1,5);GXInitLightDir(&light,0,0,-1);
            GXInitLightColor(&light,(GXColor){10,20,30,100});GXInitLightAttn(&light,1,.2f,.1f,1,.01f,.001f);GXLoadLightObjImm(&light,1u<<i);
        }
        if(test==2)GXSetVtxDesc(GX_VA_PNMTXIDX,GX_DIRECT);
        if(test==3)fh_gx.channel[2].enabled=1;
        if(test==4)fh_gx.channel[0].ambient_source=GX_SRC_VTX;
        if(test==5){GXSetNumTexGens(1);GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_SRTG,GX_TG_COLOR0,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);}
        if(test==6)fh_gx.channel[0].attenuation=GX_AF_SPEC;
        if(test==7)use_both_raster_channels();
        if(test==8)fh_gx.lights[0].position[0]=1e12f;
        if(test==9)fh_gx.position[0][0][0]=1000;
        lighting_benchmark=test<2?1:0;
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned i=0;i<3;i++) {
            if(test==2){u8 matrix=0;fh_gx_fifo_write(&matrix,1);}
            GXPosition3f32(test==10?1e6f:i*.25f,.5f,-2);GXNormal3f32(0,0,test==11?1e-10f:1);GXColor4u8(180,100,220,128);
        }
        GXEnd();CHECK(vertex_count==3);
        if(!mode){memcpy(reference,captured,sizeof(reference));continue;}
        unsigned expected=test<=2 || test>=10;CHECK(((fh_gx.draw.raw_position&2)!=0)==expected);
        for(unsigned i=0;i<3;i++) {
            if(test<=2) {
                CHECK(captured[i].matrix_index==0 && (lighting_prepared.mask&255)==255);
                CHECK(!!(lighting_prepared.mask&FH_LIGHT_MODEL_NORMAL)==(test==2));
                CHECK(captured[i].uv[7][0]==0 && captured[i].uv[7][1]==0 && captured[i].uv[7][2]==1);
                float material[2][4]={{180/255.f,100/255.f,220/255.f,128/255.f},{0}},out[2][4]={{0}};
                float eye[3]={i*.25f,.5f,-2},normal[3]={0,0,1};reference_lighting(out,material,eye,normal);
                for(unsigned c=0;c<4;c++)CHECK(near(reference[i].color[0][c],out[0][c]));
                for(unsigned c=0;c<3;c++)CHECK(near(captured[i].color[0][c],test==1?1.f:material[0][c]));
                  CHECK(captured[i].color[0][3]==reference[i].color[0][3]);
                  FhVertex resolved=captured[i];fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
                  for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[0][c],reference[i].color[0][c]));
            } else {
                CHECK(!memcmp(captured[i].color,reference[i].color,sizeof(reference[i].color)));
                if(test>=10)CHECK(captured[i].matrix_index==-1);
            }
        }
    }
    accept_lighting=0;lighting_benchmark=0;
}
static void palette_lighting_regression(void){
    FhVertex reference[3];
    for(unsigned trial=0;trial<25;trial++)for(unsigned mode=0;mode<2;mode++){
        fh_gx_reset();gpu_transform_available=1;accept_lighting=mode;accept_texgen=mode;vertex_count=0;
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_PNMTXIDX,GX_DIRECT);GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC);
        GXSetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_RASA);
        GXSetChanCtrl(GX_COLOR0,GX_TRUE,GX_SRC_REG,GX_SRC_REG,1,GX_DF_CLAMP,GX_AF_SPOT);
        GXSetChanCtrl(GX_ALPHA0,GX_FALSE,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
        GXSetChanMatColor(GX_COLOR0A0,(GXColor){200,120,80,170});GXSetChanAmbColor(GX_COLOR0A0,(GXColor){10,20,30,40});
        GXLightObj light;GXInitLightPos(&light,2,1,5);GXInitLightDir(&light,0,0,-1);
        GXInitLightColor(&light,(GXColor){60,80,100,255});GXInitLightAttn(&light,1,.2f,.1f,1,.01f,.001f);GXLoadLightObjImm(&light,1);
        if(trial>=19){
            GXSetChanCtrl(GX_COLOR0,GX_FALSE,GX_SRC_VTX,trial==20?GX_SRC_VTX:GX_SRC_REG,255,GX_DF_NONE,GX_AF_SPEC);
            /* Disabled channels must ignore stale masks, light and ambient
             * data while preserving register/vertex RGB and CPU alpha. */
            for(unsigned i=0;i<8;i++)fh_gx.lights[i].position[0]=NAN;
            fh_gx.ambient[0][0]=NAN;
            if(trial==20){GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);GXSetVtxAttrFmt(0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);}
            if(trial==22)GXSetChanCtrl(GX_ALPHA0,GX_TRUE,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
        }
        for(unsigned i=0;i<11;i++){
            float model[3][4]={{1,0,0,i*.25f},{0,1,0,.5f},{0,0,1,-2}};
            float normal[3][4]={{1,0,i*.1f,0},{0,.5f,0,0},{0,0,1+i*.25f,0}};
            if(trial==1)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<3;c++)normal[r][c]=0;
            if(trial==2)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<3;c++)normal[r][c]*=1e-10f;
            if(trial==3)normal[0][0]=32;
            if(trial==5)model[0][0]=1000;
            if(trial==6)normal[0][0]=INFINITY;
            if(trial==7)model[0][0]=NAN;
            if(trial>=8){
                float angle=i*.1f;model[0][0]=cosf(angle);model[0][1]=-sinf(angle);model[1][0]=sinf(angle);model[1][1]=cosf(angle);
                memcpy(normal,model,sizeof(normal));for(unsigned r=0;r<3;r++)normal[r][3]=0;
                if(trial==9)normal[0][0]+=.01f; /* Distinct normal matrix stays on old route. */
                if(trial==11){model[0][1]=1;memcpy(normal,model,sizeof(normal));for(unsigned r=0;r<3;r++)normal[r][3]=0;}
                if(trial==18 || trial==21){normal[0][0]=1;normal[0][1]=0;normal[1][0]=0;normal[1][1]=1;}
            }
            GXLoadPosMtxImm(model,i*3);GXLoadNrmMtxImm(normal,i*3);
            if(trial>=12)GXLoadTexMtxImm(normal,GX_TEXMTX0+i*3,GX_MTX3x4);
        }
        if(trial>=12){
            GXSetNumTexGens(2);GXSetVtxDesc(GX_VA_TEX0MTXIDX,GX_DIRECT);
            GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_NRM,GX_TEXMTX0,trial==14 || trial==24,GX_PTTEXMTX0);
            GXSetTexCoordGen2(1,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
            float post[3][4]={{-.5f,0,0,.5f},{0,-.5f,0,.5f},{0,0,0,1}};
            if(trial==16)post[2][3]=0;
            GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);
            if(trial==13)fh_gx.texture[10][0][0]+=.1f;
        }
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned i=0;i<3;i++){
            u8 matrix=(trial==4?10:i*4)*3;fh_gx_fifo_write(&matrix,1);
            if(trial>=12){u8 texture=matrix+GX_TEXMTX0+(trial==15 && i==1?3:0);fh_gx_fifo_write(&texture,1);}
            GXPosition3f32(i*.125f,0,0);GXNormal3f32(0,0,trial==10 || trial==17 || trial==23?1e-7f:1);
            if(trial==20)GXColor4u8(30+i*50,140-i*10,60,190);
        }
        GXEnd();CHECK(vertex_count==3);
        if(!mode){memcpy(reference,captured,sizeof(reference));continue;}
        if(trial>=19){
            unsigned supported=trial!=22 && trial!=24;
            CHECK(fh_gx.draw.raw_position==(supported?7u:1u));
            if(supported){CHECK(!(lighting_prepared.mask&255));CHECK(lighting_prepared.ambient[0]==1);}
            if(trial==21)CHECK(lighting_prepared.mask&FH_LIGHT_NORMAL_PALETTE);
            for(unsigned i=0;i<3;i++){
                CHECK(captured[i].matrix_index==(trial==23?-1.f:(float)(i*12)));
                FhVertex resolved=captured[i];fh_gpu_texgen_resolve(&resolved,&fh_gx.draw,texgen_prepared);fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
                for(unsigned c=0;c<3;c++)CHECK(near(resolved.uv[0][c],reference[i].uv[0][c]));
                for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[0][c],reference[i].color[0][c]));
            }
            continue;
        }
        unsigned supported=trial<3 || trial==4 || trial>=8;CHECK(!!(fh_gx.draw.raw_position&2)==supported);
        if(trial>=8)CHECK(!!(lighting_prepared.mask&FH_LIGHT_MODEL_NORMAL)==(trial==8 || trial==10 || (trial>=12 && trial<18)));
        if(trial==18)CHECK(lighting_prepared.mask&FH_LIGHT_NORMAL_PALETTE);
        if(trial>=12)CHECK(!!(fh_gx.draw.raw_position&4)==(trial==12 || trial==15 || trial==17 || trial==18));
        for(unsigned i=0;i<3;i++){
            if(trial>=8){
                CHECK(captured[i].matrix_index==(trial==10 || trial==17?-1.f:(float)(i*12)));
                FhVertex resolved=captured[i];fh_gpu_texgen_resolve(&resolved,&fh_gx.draw,texgen_prepared);fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
                if(trial>=12)for(unsigned c=0;c<3;c++)CHECK(near(resolved.uv[0][c],reference[i].uv[0][c]));
                for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[0][c],reference[i].color[0][c]));
                if(trial==8)CHECK(captured[i].uv[7][0]==0 && captured[i].uv[7][1]==0 && captured[i].uv[7][2]==1);
                continue;
            }
            if(trial<2){
                CHECK(captured[i].matrix_index==(float)(i*12));
                for(unsigned r=0;r<3;r++)for(unsigned c=0;c<3;c++)CHECK(lighting_prepared.normal[r][c]==(float)(r==c));
                float n[3],eye[3];for(unsigned r=0;r<3;r++){
                    n[r]=fh_gx.normal[i*4][r][2];CHECK(near((lighting_prepared.mask&FH_LIGHT_NORMAL_PALETTE)?(float)(r==2):n[r],captured[i].uv[7][r]));
                    eye[r]=fh_gx.position[i*4][r][0]*i*.125f+fh_gx.position[i*4][r][3];
                }
                float length=sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(length>1e-20f)for(unsigned c=0;c<3;c++)n[c]/=length;
                float material[2][4]={{0}},color[2][4]={{0}};reference_lighting(color,material,eye,n);
                for(unsigned c=0;c<4;c++)CHECK(near(reference[i].color[0][c],color[0][c]));
                  for(unsigned c=0;c<3;c++)CHECK(captured[i].color[0][c]==1.f);
                  FhVertex resolved=captured[i];fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
                  for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[0][c],reference[i].color[0][c]));
            }else CHECK(!memcmp(reference[i].color,captured[i].color,sizeof(reference[i].color)));
            if(trial==2 || trial==4)CHECK(captured[i].matrix_index==-1);
        }
    }
    accept_lighting=accept_texgen=0;
}
static void batching_primitive(unsigned i,unsigned mixed){
    const unsigned kinds[]={GX_TRIANGLES,GX_TRIANGLESTRIP,GX_TRIANGLEFAN,GX_QUADS,GX_LINES,GX_POINTS};
    const unsigned counts[]={3,5,5,4,2,1};unsigned n=mixed?i%6:0;
    GXBegin(kinds[n],i&1?GX_VTXFMT1:GX_VTXFMT0,counts[n]);
    for(unsigned v=0;v<counts[n];v++){
        if(i&1)GXPosition3s16(v*24,v%2*64,-128);
        else GXPosition3f32(v*.09375f,v%2*.25f,-.5f);
        GXColor4u8(i,v*32,80,255);
    }
    GXEnd();
}
static void component_regression(void){
    for(unsigned type=GX_U8;type<=GX_F32;type++)for(unsigned le=0;le<2;le++){
        fh_gx_reset();gpu_transform_available=1;vertex_count=0;
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,type,2);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,type,2);
        GXSetNumTexGens(1);GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
        u8 bytes[16]={0};unsigned width=type<2?1:type<4?2:4;
        const u32 float_bits[]={0xc0200000,0x40800000,0x40c00000};
        for(unsigned c=0;c<3;c++){
            u32 bits=type==GX_F32?float_bits[c]:(c+1)*8;
            if((type==GX_S8 || type==GX_S16) && !c)bits=(u32)-8;
            for(unsigned b=0;b<width;b++)bytes[1+c*width+b]=bits>>((le?b:width-1-b)*8);
        }
        GXSetArray(GX_VA_POS,bytes+1,3*width,3*width,le);
        GXSetArray(GX_VA_TEX0,bytes+1,2*width,2*width,le);
        const u8 list[]={0x90,0,3,0,0,0,0,0,0};GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3);
        float x=type==GX_F32?-2.5f:(type==GX_S8 || type==GX_S16)?-2.f:2.f;
        for(unsigned i=0;i<3;i++){
            CHECK(captured[i].position[0]==x && captured[i].position[1]==4 && captured[i].position[2]==6);
            CHECK(captured[i].uv[0][0]==x && captured[i].uv[0][1]==4 && captured[i].uv[0][2]==1);
        }
    }
}
static void batching_regression(void){
    static FhVertex expected_vertices[1024];static unsigned char list[8192];
    for(unsigned gpu=0;gpu<2;gpu++)for(unsigned mixed=0;mixed<2;mixed++){
        fh_gx_reset();gpu_transform_available=gpu;GXSetViewport(0,0,640,480,0,1);
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        GXSetVtxAttrFmt(GX_VTXFMT1,GX_VA_POS,GX_POS_XYZ,GX_S16,8);
        for(unsigned f=0;f<2;f++)GXSetVtxAttrFmt(f,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
        GXSetChanCtrl(GX_COLOR0A0,GX_FALSE,GX_SRC_REG,GX_SRC_VTX,0,GX_DF_NONE,GX_AF_NONE);
        unsigned primitives=mixed?24:160;
        vertex_count=draw_calls=0;
        for(unsigned i=0;i<primitives;i++)batching_primitive(i,mixed);
        unsigned expected_count=vertex_count,original_calls=draw_calls;
        memcpy(expected_vertices,captured,vertex_count*sizeof(*captured));
        GXBeginDisplayList(list,sizeof(list));
        for(unsigned i=0;i<primitives;i++)batching_primitive(i,mixed);
        unsigned bytes=GXEndDisplayList();vertex_count=draw_calls=0;
        GXCallDisplayList(list,bytes);
        CHECK(vertex_count==expected_count && !memcmp(captured,expected_vertices,vertex_count*sizeof(*captured)));
        CHECK(draw_calls<original_calls);
        if(!mixed)CHECK(draw_calls==2 && vertex_count==480);
        else CHECK(draw_calls==12); /* triangle group, line, point per block */
        /* A subsequent direct primitive must not inherit deferred submission. */
        vertex_count=draw_calls=0;batching_primitive(0,0);CHECK(vertex_count==3 && draw_calls==1);
    }
}
static void texgen_position_matrix_regression(void){
    for(unsigned gpu=0;gpu<2;gpu++)for(unsigned indexed=0;indexed<2;indexed++)
    for(unsigned kind=0;kind<4;kind++)for(unsigned st=0;st<2;st++) {
        fh_gx_reset();gpu_transform_available=gpu;vertex_count=0;GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_S16,0);
        if(indexed)GXSetVtxDesc(GX_VA_TEX0MTXIDX,GX_DIRECT);
        unsigned index=(unsigned[]){GX_PNMTX0,GX_PNMTX9,GX_TEXMTX0,GX_IDENTITY}[kind];
        float position[3][4]={{2,0,0,10},{0,3,0,20},{0,0,4,30}};
        float texture[3][4]={{5,0,0,40},{0,6,0,50},{0,0,7,60}};
        float post[3][4]={{.5f,0,0,1},{0,.25f,0,2},{0,0,2,3}};
        GXLoadPosMtxImm(position,GX_PNMTX0);position[0][3]=13;GXLoadPosMtxImm(position,GX_PNMTX9);
        GXLoadTexMtxImm(texture,GX_TEXMTX0,GX_MTX3x4);GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);
        GXSetNumTexGens(1);GXSetTexCoordGen2(GX_TEXCOORD0,st?GX_TG_MTX2x4:GX_TG_MTX3x4,
            GX_TG_POS,indexed?GX_TEXMTX0:index,GX_FALSE,GX_PTTEXMTX0);
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned i=0;i<3;i++){if(indexed){u8 b=index;fh_gx_fifo_write(&b,1);}GXPosition3s16(1,2,3);}GXEnd();
        const float expected[4][3]={{12,26,42},{15,26,42},{45,62,81},{1,2,3}};
        for(unsigned i=0;i<3;i++){
            CHECK(captured[i].uv[0][0]==expected[kind][0]*.5f+1);
            CHECK(captured[i].uv[0][1]==expected[kind][1]*.25f+2);
            CHECK(captured[i].uv[0][2]==(st?1:expected[kind][2])*2+3);
        }
    }
    /* A texture-matrix load addressing PNMTX0 aliases that XF slot. */
    float alias[3][4]={{2,0,0,4},{0,2,0,5},{0,0,2,6}};
    GXLoadTexMtxImm(alias,GX_PNMTX0,GX_MTX3x4);
    CHECK(!memcmp(fh_gx.position[0],alias,sizeof(alias)));
    float rows[2][4]={{7,0,0,8},{0,9,0,10}};
    GXLoadTexMtxImm(rows,GX_PNMTX0,GX_MTX2x4);
    CHECK(!memcmp(fh_gx.position[0],rows,sizeof(rows)) && !memcmp(fh_gx.position[0][2],alias[2],sizeof(alias[2])));
    GXLoadTexMtxImm(alias,GX_TEXMTX0,GX_MTX3x4);GXLoadTexMtxImm(rows,GX_TEXMTX0,GX_MTX2x4);
    CHECK(!memcmp(fh_gx.texture[GX_TEXMTX0/3],rows,sizeof(rows)) && !memcmp(fh_gx.texture[GX_TEXMTX0/3][2],alias[2],sizeof(alias[2])));
}
static void texgen_reuse_regression(void){
    for(unsigned pass=0;pass<16;pass++) {
        fh_gx_reset();vertex_count=0;GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
        float matrix[3][4]={{2,0,0,1+pass},{0,3,0,2},{0,0,4,3}};
        float second[3][4]={{5,0,0,6},{0,7,0,8},{0,0,9,10}};
        float post[3][4]={{.5f,0,0,1},{0,.25f,0,2},{0,0,2,3}};
        GXLoadTexMtxImm(matrix,GX_TEXMTX0,GX_MTX3x4);GXLoadTexMtxImm(second,GX_TEXMTX1,GX_MTX3x4);
        GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);GXSetNumTexGens(8);
        for(unsigned t=0;t<8;t++) {
            if(pass&4)GXSetVtxDesc(GX_VA_TEX0MTXIDX+t,GX_DIRECT);
            GXSetTexCoordGen2(t,pass&1?GX_TG_MTX2x4:GX_TG_MTX3x4,GX_TG_POS,GX_TEXMTX0,!!(pass&2),pass&8?GX_PTTEXMTX0:GX_PTIDENTITY);
        }
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned i=0;i<3;i++) {
            if(pass&4)for(unsigned t=0;t<8;t++){u8 m=(i+t)%3==0?GX_IDENTITY:(i+t)%3==1?GX_TEXMTX0:GX_TEXMTX1;fh_gx_fifo_write(&m,1);}
            GXPosition3f32(i+1,i+2,i+3);
        }
        GXEnd();CHECK(vertex_count==3);
        for(unsigned i=0;i<3;i++)for(unsigned t=0;t<8;t++) {
            unsigned which=pass&4?(i+t)%3:1;float result[3];
            for(unsigned c=0;c<3;c++)result[c]=which?((which==1?matrix:second)[c][c]*(i+c+1)+(which==1?matrix:second)[c][3]):i+c+1;
            if(pass&1)result[2]=1;
            if(pass&2){float length=sqrtf(result[0]*result[0]+result[1]*result[1]+result[2]*result[2]);for(unsigned c=0;c<3;c++)result[c]/=length;}
            if(pass&8)for(unsigned c=0;c<3;c++)result[c]=post[c][c]*result[c]+post[c][3];
            for(unsigned c=0;c<3;c++)CHECK(near(captured[i].uv[t][c],result[c]));
        }
    }
    /* Identical self/forward sources are not common expressions: slot zero
     * observes the initially empty slot one, whereas slot two sees its value. */
    fh_gx_reset();vertex_count=0;GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetNumTexGens(4);
    GXSetTexCoordGen2(0,GX_TG_MTX3x4,GX_TG_TEXCOORD1,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXSetTexCoordGen2(1,GX_TG_MTX3x4,GX_TG_POS,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXSetTexCoordGen2(2,GX_TG_MTX3x4,GX_TG_TEXCOORD1,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXSetTexCoordGen2(3,GX_TG_MTX3x4,GX_TG_TEXCOORD1,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);for(unsigned i=0;i<3;i++)GXPosition3f32(2,3,4);GXEnd();
    for(unsigned i=0;i<3;i++)for(unsigned c=0;c<3;c++){
        CHECK(captured[i].uv[0][c]==0);CHECK(captured[i].uv[2][c]==c+2);CHECK(captured[i].uv[3][c]==c+2);
    }
}
static void texgen_cache_regression(void){
    unsigned cache_hits_before=fh_texgen_cache_hits;
    fh_gx_reset();gpu_transform_available=1;GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);GXSetNumTexGens(1);
    for(unsigned sample=0;sample<96;sample++)for(unsigned repeat=0;repeat<2;repeat++) {
        float post[3][4]={{1,0,0,sample/16.f},{0,2,0,-sample/32.f},{0,0,3,.25f}};
        GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);
        GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,GX_TRUE,GX_PTTEXMTX0);
        vertex_count=0;GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned i=0;i<3;i++){GXPosition3f32(i,0,-1);GXTexCoord2f32(i*.25f+sample/128.f,-i*.5f);}GXEnd();
        CHECK(vertex_count==3);
        for(unsigned i=0;i<3;i++) {
            float v[3]={i*.25f+sample/128.f,-i*.5f,1};
            float length=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);for(unsigned c=0;c<3;c++)v[c]/=length;
            for(unsigned c=0;c<3;c++) {
                float expected=post[c][0]*v[0]+post[c][1]*v[1]+post[c][2]*v[2]+post[c][3];
                CHECK(captured[i].uv[0][c]==expected);
            }
        }
    }
    CHECK(fh_texgen_cache_hits>=cache_hits_before+280);
}
static void geometry_cache_regression(void){
    fh_retained_disabled=1;
    fh_gx_reset();gpu_transform_available=1;GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GXSetNumTexGens(1);GXSetTexCoordGen2(0,GX_TG_MTX3x4,GX_TG_POS,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXSetChanCtrl(GX_COLOR0A0,GX_FALSE,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
    float positions[3][3]={{1,2,3},{4,5,6},{7,8,9}};
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);
    u8 list[]={GX_TRIANGLES,0,6,0,1,2,0,1,2};FhVertex expected_vertices[6];
    unsigned hits,misses,vertices,used,h;
    vertex_count=0;GXCallDisplayList(list,sizeof(list));memcpy(expected_vertices,captured,sizeof(expected_vertices));
    fh_geometry_counters(&h,&misses,&vertices,&used);
    vertex_count=0;GXCallDisplayList(list,sizeof(list));fh_geometry_counters(&hits,&misses,&vertices,&used);
    CHECK(hits==h+1 && vertex_count==6 && !memcmp(expected_vertices,captured,sizeof(expected_vertices)));
    positions[0][0]=17;vertex_count=0;GXCallDisplayList(list,sizeof(list));
    CHECK(captured[0].position[0]==17 && captured[0].uv[0][0]==17);
    GXSetChanMatColor(GX_COLOR0A0,(GXColor){128,64,32,16});vertex_count=0;GXCallDisplayList(list,sizeof(list));
    CHECK(captured[0].color[0][0]==128/255.f && captured[0].color[0][3]==16/255.f);
    fh_geometry_counters(&h,&misses,&vertices,&used);
    GXSetTevKColor(GX_KCOLOR0,(GXColor){9,8,7,6});vertex_count=0;GXCallDisplayList(list,sizeof(list));
    fh_geometry_counters(&hits,&misses,&vertices,&used);CHECK(hits==h+1);
    /* Mutable list bytes and source arrays are independently validated. */
    list[3]=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(captured[0].position[0]==4);
    for(unsigned i=0;i<80;i++) {
        positions[1][0]=20+i;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==6 && captured[0].position[0]==20+i);
        fh_geometry_counters(&hits,&misses,&vertices,&used);CHECK(used<=1024*1024);
    }
    /* After sustained misses, every bypass must still decode the same output.
     * Periodic probes must recover when the workload becomes reusable. */
    memcpy(expected_vertices,captured,sizeof(expected_vertices));
    fh_geometry_counters(&h,&misses,&vertices,&used);
    unsigned attempts=0;
    do {
        vertex_count=0;GXCallDisplayList(list,sizeof(list));
        fh_geometry_counters(&hits,&misses,&vertices,&used);
        CHECK(vertex_count==6 && !memcmp(expected_vertices,captured,sizeof(expected_vertices)));
    }while(hits==h && ++attempts<258);
    CHECK(hits==h+1);
    /* Retain original batch boundaries across multiple 384-vertex chunks. */
    u8 large[963]={GX_TRIANGLES,3,192};for(unsigned i=0;i<960;i++)large[3+i]=i%3;
    static FhVertex large_expected[960];vertex_count=0;GXCallDisplayList(large,sizeof(large));
    CHECK(vertex_count==960);memcpy(large_expected,captured,sizeof(large_expected));
    fh_geometry_counters(&h,&misses,&vertices,&used);vertex_count=0;GXCallDisplayList(large,sizeof(large));
    fh_geometry_counters(&hits,&misses,&vertices,&used);
    CHECK(hits==h+1 && vertex_count==960 && !memcmp(large_expected,captured,sizeof(large_expected)));
    /* CPU geometric lighting is cached only with all of its inputs owned.
     * Compare each changed light/normal/ambient against the scalar oracle. */
    GXSetChanCtrl(GX_COLOR0A0,GX_TRUE,GX_SRC_REG,GX_SRC_REG,1,GX_DF_CLAMP,GX_AF_SPOT);
    GXSetNumTexGens(0);GXSetVtxDesc(GX_VA_NRM,GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    float normal[3]={0,0,1};GXSetArray(GX_VA_NRM,normal,sizeof(normal),12,true);
    u8 lit[]={GX_TRIANGLES,0,3,0,0,1,0,2,0};
    float colors[2][4]={{1,1,1,1},{1,1,1,1}},reference[2][4];
    for(unsigned mode=0;mode<12;mode++) {
        FhLight *l=&fh_gx.lights[0];memset(l,0,sizeof(*l));
        l->position[0]=mode;l->position[2]=100;l->direction[2]=1;
        l->a[0]=1;l->k[0]=1;l->color[0]=.5f;l->color[1]=.25f;l->color[2]=.75f;l->color[3]=1;
        fh_gx.ambient[0][0]=mode*.01f;
        normal[0]=(mode&1)?1:0;normal[2]=(mode&1)?0:1;
        for(unsigned repeat=0;repeat<2;repeat++) {
            vertex_count=0;GXCallDisplayList(lit,sizeof(lit));CHECK(vertex_count==3);
            for(unsigned i=0;i<3;i++) {
                reference_lighting(reference,colors,positions[i],normal);
                CHECK(!memcmp(captured[i].color[0],reference[0],sizeof(reference[0])));
            }
        }
    }
}
static void retained_geometry_regression(void) {
    for(unsigned mode=0;mode<4;mode++){
        fh_gx_reset();gpu_transform_available=1;accept_lighting=mode>=2;accept_texgen=mode==3;
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_PNMTXIDX,GX_DIRECT);
        GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_NRM,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
        if(mode==3)GXSetVtxDesc(GX_VA_TEX0MTXIDX,GX_DIRECT);
        GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
        float pos[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},normals[3][3]={{0,0,1},{0,0,1},{0,0,1}},uv[3][2]={{0,0},{1,0},{0,1}};
        GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_NRM,normals,sizeof(normals),12,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
        GXSetNumTexGens(mode==3?2:1);GXSetTexCoordGen2(0,GX_TG_MTX2x4,mode==3?GX_TG_NRM:GX_TG_TEX0,mode==3?GX_TEXMTX0:GX_IDENTITY,0,mode==3?GX_PTTEXMTX0:GX_PTIDENTITY);
        GXSetTexCoordGen2(1,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
        GXSetChanCtrl(GX_COLOR0,mode!=0,GX_SRC_REG,GX_SRC_REG,1,GX_DF_CLAMP,GX_AF_SPOT);
        GXSetChanCtrl(GX_ALPHA0,0,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
        GXLightObj light;GXInitLightPos(&light,2,1,5);GXInitLightDir(&light,0,0,-1);GXInitLightColor(&light,(GXColor){80,120,160,255});GXInitLightAttn(&light,1,.1f,.1f,1,.01f,.01f);GXLoadLightObjImm(&light,1);
        u8 list[33]={GX_TRIANGLES,0,6};unsigned size=3;
        for(unsigned i=0;i<6;i++){unsigned n=i%3;list[size++]=n*3;if(mode==3)list[size++]=GX_TEXMTX0+n*3;list[size++]=n;list[size++]=n;list[size++]=n;}
        FhVertex expected[6];unsigned h,m,v,used,before;
        for(unsigned change=0;change<20;change++){
            for(unsigned bone=0;bone<3;bone++){
                float a=.01f*(change+bone),model[3][4]={{cosf(a),-sinf(a),0,change*.01f},{sinf(a),cosf(a),0,0},{0,0,1,0}},normal[3][4];
                memcpy(normal,model,sizeof(normal));normal[0][3]=0;
                GXLoadPosMtxImm(model,bone*3);GXLoadNrmMtxImm(normal,bone*3);GXLoadTexMtxImm(normal,GX_TEXMTX0+bone*3,GX_MTX3x4);
            }
            float post[3][4]={{-.5f,0,0,.5f+change*.001f},{0,-.5f,0,.5f},{0,0,0,1}};GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);
            fh_gx.lights[0].position[0]=2+change*.1f;fh_gx.projection[0][0]=1+change*.01f;
            if(change==4)pos[1][0]+=.125f;
            if(change==6)uv[2][0]=.375f;
            if(change==8)normals[0][1]=.125f;
            if(change==10)GXSetChanMatColor(GX_COLOR0A0,(GXColor){128,220,90,170});
            if(change==12)list[size-1]=1;
            if(change==14)fh_gx.formats[0][GX_VA_TEX0].fraction=1;
            fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,size);CHECK(vertex_count==6);memcpy(expected,captured,sizeof(expected));
            fh_retained_disabled=0;vertex_count=0;GXCallDisplayList(list,size);CHECK(vertex_count==6 && !memcmp(expected,captured,sizeof(expected)));
            fh_retained_stats(&before,&m,&v,&used);vertex_count=0;GXCallDisplayList(list,size);
            fh_retained_stats(&h,&m,&v,&used);CHECK(vertex_count==6 && !memcmp(expected,captured,sizeof(expected)));CHECK(used<=4*1024*1024);
            CHECK(mode==1?h==before:h==before+1);
        }
        if(mode==0)for(unsigned change=0;change<14;change++){
            /* Disabled/unused channel state is not a baked dependency. RGB
             * and alpha use independent light masks; every live component
             * must still invalidate and match uncached vertex evaluation. */
            if(change==1){fh_gx.ambient[0][0]=.2f;fh_gx.lights[7].color[0]=.9f;fh_gx.material[1][0]=.1f;}
            if(change==2)GXSetChanCtrl(GX_ALPHA0,0,GX_SRC_VTX,GX_SRC_REG,255,GX_DF_SIGN,GX_AF_SPEC);
            if(change==3)GXSetChanCtrl(GX_COLOR0,1,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
            if(change==4){fh_gx.lights[7].color[0]=.3f;GXSetChanCtrl(GX_COLOR0,1,GX_SRC_REG,GX_SRC_REG,0,GX_DF_CLAMP,GX_AF_SPOT);}
            if(change==5)fh_gx.ambient[0][0]=.3f;
            if(change==6){fh_gx.ambient[0][3]=.25f;fh_gx.lights[7].color[3]=.1f;}
            if(change==7)GXSetChanCtrl(GX_COLOR0,1,GX_SRC_REG,GX_SRC_REG,1,GX_DF_NONE,GX_AF_NONE);
            if(change==8)fh_gx.lights[1].color[0]=.6f;
            if(change==9)fh_gx.lights[0].color[3]=.4f;
            if(change==10)fh_gx.lights[0].color[0]=.05f;
            if(change==11)GXSetChanCtrl(GX_ALPHA0,1,GX_SRC_REG,GX_SRC_REG,2,GX_DF_NONE,GX_AF_NONE);
            if(change==12)fh_gx.lights[1].color[0]=.05f;
            if(change==13)fh_gx.lights[1].color[3]=.15f;
            fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,size);CHECK(vertex_count==6);memcpy(expected,captured,sizeof(expected));
            fh_retained_disabled=0;fh_retained_stats(&before,&m,&v,&used);vertex_count=0;GXCallDisplayList(list,size);fh_retained_stats(&h,&m,&v,&used);
            CHECK(vertex_count==6 && !memcmp(expected,captured,sizeof(expected)));
            if(change)CHECK(h==before+((change==3 || change==5 || change==7 || change==10 || change==11 || change==13)?0:1));
        }
        if(mode==3)for(unsigned change=0;change<8;change++){
            /* Alternate enabled/mask/ambient live uniforms on one immutable
             * mesh; compare resolved cached pixels' vertex inputs with scalar
             * GX and require reuse until the actual material source changes. */
            GXSetChanCtrl(GX_COLOR0,change&1,GX_SRC_REG,change>=6?GX_SRC_VTX:GX_SRC_REG,change&2?3:1,GX_DF_CLAMP,GX_AF_SPOT);
            GXSetChanAmbColor(GX_COLOR0,(GXColor){20+change,40,60,255});
            accept_lighting=accept_texgen=0;fh_retained_disabled=1;vertex_count=0;
            GXCallDisplayList(list,size);memcpy(expected,captured,sizeof(expected));
            accept_lighting=accept_texgen=1;fh_retained_disabled=0;
            fh_retained_stats(&before,&m,&v,&used);vertex_count=0;GXCallDisplayList(list,size);
            fh_retained_stats(&h,&m,&v,&used);CHECK(vertex_count==6 && fh_gx.draw.raw_position==7);
            CHECK(h==before+(change==6?0:1));
            for(unsigned i=0;i<6;i++){
                FhVertex resolved=captured[i];fh_gpu_texgen_resolve(&resolved,&fh_gx.draw,texgen_prepared);fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
                for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[0][c],expected[i].color[0][c]));
                for(unsigned c=0;c<3;c++)CHECK(near(resolved.uv[0][c],expected[i].uv[0][c]));
            }
        }
    }
    fh_retained_disabled=0;accept_lighting=accept_texgen=0;
}
static void retained_texgens_regression(void) {
    /* Compare every generated coordinate with uncached GX decoding. Late
     * texture matrices, post matrices and source arrays must invalidate just
     * as the first two do; the arena remains the same bounded allocation. */
    for(unsigned count=3;count<=8;count++) {
        fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=0;
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
        GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
        float pos[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},uv[3][2]={{.1f,.2f},{.9f,.3f},{.4f,.8f}};
        GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
        GXSetNumTexGens(count);GXSetChanCtrl(GX_COLOR0A0,0,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
        for(unsigned i=0;i<count;i++) {
            float m[3][4]={{1,0,.1f*i,0},{0,1,0,.05f*i},{0,0,1,0}};
            float post[3][4]={{1,0,0,.03f*i},{0,1,0,0},{0,0,1,0}};
            GXLoadTexMtxImm(m,GX_TEXMTX0+3*i,GX_MTX3x4);GXLoadTexMtxImm(post,GX_PTTEXMTX0+3*i,GX_MTX3x4);
            GXSetTexCoordGen2(i,GX_TG_MTX3x4,GX_TG_TEX0,GX_TEXMTX0+3*i,0,GX_PTTEXMTX0+3*i);
        }
        u8 list[]={GX_TRIANGLES,0,6,0,0,1,1,2,2,0,0,1,1,2,2};FhVertex expected[6];
        for(unsigned change=0;change<4*count;change++) {
            unsigned stage=change%count;
            if(change<count)fh_gx.texture[(GX_TEXMTX0+stage*3)/3][0][3]+=.01f;
            else if(change<2*count)fh_gx.post[stage][1][3]+=.02f;
            else if(change<3*count)fh_gx.texgens[stage].normalize^=1;
            else uv[stage%3][stage%2]+=.015f;
            fh_retained_disabled=fh_texgen_cache_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(vertex_count==6);memcpy(expected,captured,sizeof(expected));
            fh_retained_disabled=fh_texgen_cache_disabled=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(vertex_count==6 && !memcmp(expected,captured,sizeof(expected)));
            unsigned h,m,v,used,before;fh_retained_stats(&before,&m,&v,&used);
            vertex_count=0;GXCallDisplayList(list,sizeof(list));fh_retained_stats(&h,&m,&v,&used);
            CHECK(vertex_count==6 && !memcmp(expected,captured,sizeof(expected)));CHECK(h==before+1 && used<=4*1024*1024);
        }
    }
}
static void large_retained_regression(void) {
    fh_retained_max_vertices=16384;fh_retained_max_list_bytes=256*1024;
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    float positions[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}};
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);GXSetNumTexGens(0);
    static u8 list[16386];list[0]=GX_TRIANGLES;list[1]=63;list[2]=255;
    for(unsigned i=0;i<16383;i++)list[3+i]=i%3;
    CHECK(fh_geometry_vertex_count(list,sizeof(list))==16383);
    list[1]=5001>>8;list[2]=5001&255;
    unsigned h,m,v,used;
    for(unsigned change=0;change<3;change++){
        if(change==1)positions[1][0]+=.125f;
        if(change==2)list[4095]=2;
        vertex_count=0;GXCallDisplayList(list,5004);CHECK(vertex_count==5001);
        fh_retained_stats(&h,&m,&v,&used);unsigned before=h;
        /* Geometry starts after the reserved 1 MiB source-snapshot region. */
        if(!change)CHECK(used>=1024*1024 && used-1024*1024<40000);
        vertex_count=0;GXCallDisplayList(list,5004);CHECK(vertex_count==5001);
        fh_retained_stats(&h,&m,&v,&used);CHECK(h==before+1 && used<=4*1024*1024);
        for(unsigned i=0;i<vertex_count;i++){
            CHECK(!memcmp(captured[i].position,positions[list[i+3]],12));
            CHECK(captured[i].position[3]==1 && captured[i].matrix_index==0);
        }
    }
    /* Force arena wrap with distinct lists and large owned source arrays.
     * Reusing those addresses must either replay verified output or decode it
     * afresh; it must never return another entry's overwritten geometry. */
    fh_gx_reset();GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetNumTexGens(0);
    static float large_array[51200];static u8 lists[40][32] __attribute__((aligned(32)));
    memcpy(large_array,positions,sizeof(positions));GXSetArray(GX_VA_POS,large_array,sizeof(large_array),12,true);
    for(unsigned round=0;round<2;round++)for(unsigned i=0;i<40;i++){
        lists[i][0]=GX_TRIANGLES;lists[i][1]=0;lists[i][2]=3;
        for(unsigned n=0;n<3;n++)lists[i][n+3]=(n+i)%3;
        vertex_count=0;GXCallDisplayList(lists[i],6);CHECK(vertex_count==3);
        for(unsigned n=0;n<3;n++)CHECK(!memcmp(captured[n].position,positions[(n+i)%3],12));
    }
    fh_retained_stats(&h,&m,&v,&used);unsigned before=h;
    vertex_count=0;GXCallDisplayList(lists[39],6);fh_retained_stats(&h,&m,&v,&used);
    CHECK(h==before+1 && vertex_count==3 && used<=4*1024*1024);
    fh_retained_max_vertices=4096;fh_retained_max_list_bytes=65536;
}
static void point_submission_regression(void){
    fh_gx_reset();gpu_transform_available=accept_points=1;GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_S16,0);GXSetPointSize(18,GX_TO_HALF);GXEnableTexOffsets(0,0,1);GXSetCullMode(GX_CULL_ALL);
    float matrix[3][4]={{1,0,0,5},{0,1,0,2},{0,0,1,-8}};GXLoadPosMtxImm(matrix,0);
    static u8 list[3+770*6]={GX_POINTS,3,2};
    for(unsigned i=0;i<770;i++){list[3+i*6]=0;list[4+i*6]=i%100;list[7+i*6]=255;list[8+i*6]=255;}
    vertex_count=point_calls=0;GXCallDisplayList(list,sizeof(list));
    CHECK(vertex_count==770 && point_calls==3 && fh_gx.draw.cull==GX_CULL_ALL);
    for(unsigned i=0;i<770;i++)CHECK(captured[i].matrix_index==0 && captured[i].position[0]==i%100 && captured[i].position[2]==-1);
    unsigned decoded_before,reused;fh_gx_vertex_counters(&decoded_before,&reused);
    matrix[0][3]=10;GXLoadPosMtxImm(matrix,0);vertex_count=point_calls=0;GXCallDisplayList(list,sizeof(list));
    unsigned decoded;fh_gx_vertex_counters(&decoded,&reused);
    CHECK(vertex_count==770 && point_calls==3 && decoded==decoded_before && fh_gx.draw.cull==GX_CULL_ALL);
    accept_points=0;
}
static void early_visibility_regression(void) {
    fh_gx_reset();gpu_transform_available=1;GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_PNMTXIDX,GX_DIRECT);GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    float positions[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}};
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);
    for(unsigned large=0;large<2;large++){
    unsigned vertices=large?96:6;u8 list[3+96*2]={GX_TRIANGLES,0,vertices};
    for(unsigned i=0;i<vertices;i++){list[3+i*2]=(i%3)*3;list[4+i*2]=i%3;}
    FhVertex expected[96];unsigned rejected=0;
    for(unsigned trial=0;trial<96;trial++) {
        for(unsigned m=0;m<3;m++) {
            float matrix[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
            if(trial%8<6)matrix[(trial%8)/2][3]=(trial&1)?4:-4;
            if(trial&8)matrix[0][3]+=.25f*m;
            if(trial%8==6 && m==2)matrix[0][3]=10; /* Crossing a plane. */
            GXLoadPosMtxImm(matrix,m*3);
        }
        if(trial>=64)positions[0][0]=trial&1?INFINITY:NAN;
        else positions[0][0]=trial&16?0.0f:-.2f;
        cpu_cull_available=0;vertex_count=0;GXCallDisplayList(list,3+vertices*2);
        CHECK(vertex_count==vertices);memcpy(expected,captured,vertices*sizeof(FhVertex));
        unsigned outside=63;
        for(unsigned i=0;i<vertices;i++) {
            const float *p=expected[i].position;unsigned matrix=expected[i].matrix_index;
            float eye[4]={0,0,0,p[3]},q[4];
            /* Independent copy of the former renderer's ordered transform. */
            for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)eye[r]+=fh_gx.draw.model_palette[matrix+r][c]*p[c];
            for(unsigned r=0;r<4;r++){float value=0;for(unsigned c=0;c<4;c++)value+=fh_gx.draw.position_matrix[r][c]*eye[c];q[r]=value;}
            outside&=(q[0]<-q[3]?1u:0)|(q[0]>q[3]?2u:0)|(q[1]<-q[3]?4u:0)|(q[1]>q[3]?8u:0)|(q[2]<-q[3]?16u:0)|(q[2]>0?32u:0);
        }
        cpu_cull_available=1;vertex_count=0;GXCallDisplayList(list,3+vertices*2);
        if(!vertex_count){CHECK(outside!=0 && trial<64);rejected++;}
        else CHECK(vertex_count==vertices && !memcmp(expected,captured,vertices*sizeof(FhVertex)));
    }
    CHECK(rejected>=40);cpu_cull_available=0;
    }
}
/* Independent double-precision half-space oracle. */
static unsigned bounds_rng=0x312421;
static float bounds_random(void){bounds_rng=bounds_rng*1664525u+1013904223u;return ((int)(bounds_rng>>8)-8388608)/1048576.f;}
static void gpu_visibility_regression(void){
    FhDrawState state={0};unsigned rejected=0;
    for(unsigned trial=0;trial<1024;trial++){
        for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)state.model_palette[r][c]=bounds_random();
        for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)state.position_matrix[r][c]=bounds_random();
        state.viewport[0]=bounds_random()*40;state.viewport[1]=bounds_random()*20;
        state.viewport[2]=320+bounds_random()*20;state.viewport[3]=264+bounds_random()*20;
        state.viewport[4]=0;state.viewport[5]=1;
        float p[4]={bounds_random()*100,bounds_random()*100,bounds_random()*100,1},projection[4][4];
        fh_vertex_pica_projection(projection,&state);
        unsigned mask=fh_vertex_conservative_outside(p,0,&state,projection);
        double eye[4]={0,0,0,1};
        for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)eye[r]+=(double)state.model_palette[r][c]*p[c];
        for(unsigned view=0;view<2;view++){
            const float (*m)[4]=view?projection:state.position_matrix;double q[4]={0};
            for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)q[r]+=(double)m[r][c]*eye[c];
            for(unsigned plane=0;plane<4;plane++)if(mask&(1u<<(view*4+plane)))CHECK(q[3]+(plane&1?-1:1)*q[plane/2]<0);
        }
        if((mask&15) && (mask&240))rejected++;
        if(trial%16==0){p[0]=NAN;CHECK(!fh_vertex_conservative_outside(p,0,&state,projection));}
    }
    CHECK(rejected>100);
    fh_gx_reset();gpu_transform_available=1;gpu_cull_available=1;cpu_cull_available=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    float positions[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}};
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);
    u8 list[51]={GX_TRIANGLES,0,48};for(unsigned i=0;i<48;i++)list[3+i]=i%3;
    for(unsigned trial=0;trial<8;trial++){
        float model[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
        if(trial<4)model[trial/2][3]=trial&1?10:-10;
        if(trial==5)positions[0][0]=20;else if(trial==6)positions[0][0]=NAN;else if(trial==7)positions[0][0]=INFINITY;
        GXLoadPosMtxImm(model,0);vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==(trial<4?0:48));
    }
    extern unsigned fh_gx_bounds_hits,fh_gx_bounds_disabled;
    /* Compare cached bounds against the ordinary visibility path while source
     * storage is reused, indices change, and the live camera crosses a plane. */
    unsigned hits_before=fh_gx_bounds_hits;
    positions[0][0]=-.2f;
    for(unsigned trial=0;trial<160;trial++){
        float model[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
        model[0][3]=(trial%5<3)?8:0;
        if(trial%11==0)for(unsigned i=0;i<3;i++)positions[i][0]=(trial&1)?-8:0;
        if(trial%7==0)list[3]=(list[3]+1)%3;
        GXLoadPosMtxImm(model,0);
        fh_gx_bounds_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        unsigned expected_count=vertex_count;FhVertex expected[48];memcpy(expected,captured,sizeof(expected));
        fh_gx_bounds_disabled=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==expected_count);
        if(vertex_count)CHECK(!memcmp(expected,captured,vertex_count*sizeof(*captured)));
    }
    CHECK(fh_gx_bounds_hits>hits_before+30);
    /* The source pool is shared with decoded geometry. Evict its snapshots
     * independently of the bounds arena, then test the surviving boxes against
     * the uncached oracle. Overlapping bindings are distinct owned snapshots. */
    static unsigned char bound_pressure[512*1024];
    for(unsigned round=0;round<3;round++){
        for(unsigned i=0;i<5;i++)CHECK(fh_retained_source_acquire(bound_pressure+i*65536,256*1024)!=0);
        if(round==1)fh_retained_reset();
        for(unsigned i=0;i<3;i++)positions[i][0]=round==2?100:0;
        fh_gx_bounds_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        unsigned expected_count=vertex_count;FhVertex expected[48];memcpy(expected,captured,sizeof(expected));
        fh_gx_bounds_disabled=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==expected_count);
        if(vertex_count)CHECK(!memcmp(expected,captured,vertex_count*sizeof(*captured)));
    }
    gpu_cull_available=0;
}
static void emboss_regression(void){
    fh_gx_reset();GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    float model[3][4]={{1,0,0,10},{0,1,0,20},{0,0,1,30}},normal[3][4]={{2,0,0,0},{0,3,0,0},{0,0,1,0}};
    float post[3][4]={{1,0,0,0},{0,1,0,0},{0,0,2,0}};
    GXLoadPosMtxImm(model,0);GXLoadNrmMtxImm(normal,0);GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);
    GXSetNumTexGens(2);GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_MTX3x4,GX_TG_TEX0,GX_IDENTITY,GX_FALSE,GX_PTTEXMTX0);
    GXSetTexCoordGen2(GX_TEXCOORD1,GX_TG_BUMP7,GX_TG_TEXCOORD0,GX_IDENTITY,GX_TRUE,GX_PTTEXMTX0);
    for(unsigned gpu=0;gpu<2;gpu++)for(unsigned zero=0;zero<2;zero++){
        gpu_transform_available=gpu;vertex_count=0;
        fh_gx.lights[7].position[0]=12+(zero?0:3);fh_gx.lights[7].position[1]=23+(zero?0:4);fh_gx.lights[7].position[2]=26;
        GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned i=0;i<3;i++){GXPosition3f32(2,3,-4);GXTexCoord2f32(.25f,.5f);}GXEnd();
        CHECK(fh_gx.draw.raw_position==gpu);
        CHECK(near(captured[0].uv[1][0],zero?.25f:1.45f));
        CHECK(near(captured[0].uv[1][1],zero?.5f:2.9f));
        CHECK(captured[0].uv[1][2]==2);
    }
}
static void raw_uv_regression(void){
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    float pos[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},uv[3][2]={{0,0},{1,0},{0,1}};
    GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
    GXSetNumTexGens(1);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_TEX0,GX_TEXMTX0,0,GX_PTIDENTITY);
    const u8 list[]={GX_TRIANGLES,0,3,0,0,1,1,2,2};FhVertex reference[3];
    unsigned hits_before,m,v,used;fh_retained_stats(&hits_before,&m,&v,&used);
    for(unsigned trial=0;trial<40;trial++){
        float matrix[3][4]={{1+trial*.015625f,-.25f,0,trial*.03125f},{.125f,.75f,0,-trial*.015625f},{0,0,0,1}};
        if(trial==20)uv[0][0]=.5f;
        GXLoadTexMtxImm(matrix,GX_TEXMTX0,GX_MTX2x4);
        fh_retained_disabled=1;accept_texgen=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3 && fh_gx.draw.raw_position==1);memcpy(reference,captured,sizeof(reference));
        fh_retained_disabled=0;accept_texgen=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3 && fh_gx.draw.raw_position==21);
        for(unsigned i=0;i<3;i++){
            FhVertex resolved=captured[i];fh_gpu_texgen_resolve(&resolved,&fh_gx.draw,texgen_prepared);
            CHECK(!memcmp(resolved.position,reference[i].position,16));
            CHECK(!memcmp(resolved.color,reference[i].color,32));
            for(unsigned c=0;c<3;c++)CHECK(near(resolved.uv[0][c],reference[i].uv[0][c]));
        }
    }
    unsigned h;fh_retained_stats(&h,&m,&v,&used);CHECK(h>=hits_before+36);
    accept_texgen=0;omit_texgens=1;fh_retained_disabled=0;
    fh_retained_stats(&hits_before,&m,&v,&used);
    for(unsigned trial=0;trial<12;trial++){
        float matrix[3][4]={{1,0,0,trial},{0,1,0,trial},{0,0,0,1}};
        GXLoadTexMtxImm(matrix,GX_TEXMTX0,GX_MTX2x4);vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3 && fh_gx.draw.texgens==1 && fh_gx_active_texgens==0 && fh_gx.draw.raw_position==1);
        for(unsigned i=0;i<3;i++)for(unsigned c=0;c<3;c++)CHECK(captured[i].uv[0][c]==0);
    }
    fh_retained_stats(&h,&m,&v,&used);CHECK(h>=hits_before+10);omit_texgens=0;
}
static void post_uv_regression(void){
    extern unsigned fh_post_uvgen_enabled;
    for(unsigned mode=0;mode<4;mode++){
        fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=omit_texgens=0;
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
        GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
        float pos[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},uv[3][2]={{0,0},{1,0},{0,1}};
        GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
        GXSetNumTexGens(1);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_TEX0,mode==3?GX_TEXMTX0:GX_IDENTITY,0,GX_PTTEXMTX0);
        float primary[3][4]={{.5f,0,0,.1f},{0,.75f,0,.2f},{0,0,0,1}};
        GXLoadTexMtxImm(primary,GX_TEXMTX0,GX_MTX2x4);
        const u8 list[]={GX_TRIANGLES,0,3,0,0,1,1,2,2};FhVertex reference[3];
        unsigned before,m,v,used;fh_retained_stats(&before,&m,&v,&used);
        for(unsigned trial=0;trial<40;trial++){
            float post[3][4]={{1+trial*.015625f,-.25f,.0625f,trial*.03125f},{.125f,.75f,0,-trial*.015625f},{0,0,0,1}};
            if(mode==1){post[2][2]=1;post[2][3]=0;}
            if(mode==2)post[2][0]=.25f;
            if(trial==20)uv[0][0]=.5f;
            GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);
            fh_post_uvgen_enabled=0;fh_retained_disabled=1;accept_texgen=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));
            CHECK(vertex_count==3 && fh_gx.draw.raw_position==1);memcpy(reference,captured,sizeof(reference));
            fh_post_uvgen_enabled=1;fh_retained_disabled=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));
            CHECK(vertex_count==3 && fh_gx.draw.raw_position==(mode<2?21:1));
            for(unsigned i=0;i<3;i++){
                FhVertex resolved=captured[i];fh_gpu_texgen_resolve(&resolved,&fh_gx.draw,texgen_prepared);
                CHECK(!memcmp(resolved.position,reference[i].position,16));
                CHECK(!memcmp(resolved.color,reference[i].color,32));
                CHECK(!memcmp(resolved.uv[0],reference[i].uv[0],12));
            }
        }
        unsigned h;fh_retained_stats(&h,&m,&v,&used);if(mode<2)CHECK(h>=before+36);
    }
    fh_post_uvgen_enabled=0;accept_texgen=0;fh_retained_disabled=0;
}
static void untextured_flat_regression(void){
    for(unsigned textured=0;textured<3;textured++){
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=0;omit_texgens=!textured;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_CLR0,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
    GXSetVtxDesc(GX_VA_NRM,GX_INDEX8);GXSetVtxAttrFmt(0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    float pos[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},uv[3][2]={{0,0},{1,0},{0,1}};
    u8 colors[3][4]={{80,90,100,200},{120,180,40,240},{20,255,100,140}};
    float normals[3][3]={{0,0,1e-10f},{1e30f,0,0},{NAN,0,0}};
    GXSetArray(GX_VA_NRM,normals,sizeof(normals),12,true);
    GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_CLR0,colors,sizeof(colors),4,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
    GXSetNumTexGens(1);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_TEX0,textured==1?GX_IDENTITY:GX_TEXMTX0,0,GX_PTIDENTITY);
    const u8 list[]={GX_TRIANGLES,0,3,0,0,0,0,1,1,1,1,2,2,2,2};FhVertex reference[3];
    GXSetTevOrder(0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
    GXSetTevColorIn(0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC);GXSetTevAlphaIn(0,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_RASA);
    GXSetChanCtrl(GX_ALPHA0,textured,GX_SRC_REG,textured?GX_SRC_VTX:GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
    unsigned start,m,v,used;fh_retained_stats(&start,&m,&v,&used);
    for(unsigned trial=0;trial<32;trial++){
        normals[0][0]=trial*.03125f;
        if(!textured || trial==20)uv[0][0]=trial*.00390625f;
        if((!textured && trial<16) || trial==20)colors[0][0]=80+trial;
        GXSetChanCtrl(GX_COLOR0,1,GX_SRC_REG,trial/16,3,GX_DF_NONE,GX_AF_NONE);
        GXSetChanAmbColor(GX_COLOR0,(GXColor){trial*3,40,20,255});
        GXSetChanMatColor(GX_COLOR0A0,(GXColor){220-trial,100+trial,70,80+trial*4});
        GXSetChanAmbColor(GX_ALPHA0,(GXColor){0,0,0,40+trial*3});
        for(unsigned c=0;c<3;c++){fh_gx.lights[0].color[c]=.1f+trial*.005f;fh_gx.lights[1].color[c]=.05f;}
        float matrix[3][4]={{1,0,0,trial},{0,1,0,-trial},{0,0,0,1}};GXLoadTexMtxImm(matrix,GX_TEXMTX0,GX_MTX2x4);
        accept_lighting=accept_texgen=0;fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));memcpy(reference,captured,sizeof(reference));
        accept_lighting=1;accept_texgen=textured==2;fh_retained_disabled=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3 && fh_gx.draw.raw_position==(textured==2?55:35) && fh_gx_active_texgens==!!textured);
        CHECK(lighting_prepared.mask&FH_LIGHT_FLAT);
        for(unsigned i=0;i<3;i++){
            FhVertex resolved=captured[i];fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
            for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[0][c],reference[i].color[0][c]));
            if(textured){fh_gpu_texgen_resolve(&resolved,&fh_gx.draw,texgen_prepared);for(unsigned c=0;c<3;c++)CHECK(near(resolved.uv[0][c],reference[i].uv[0][c]));}
        }
    }
    unsigned h;fh_retained_stats(&h,&m,&v,&used);CHECK(h>=start+26);
    if(textured==2)for(unsigned edge=0;edge<2;edge++){
        /* A CPU clip-space fallback must keep final UVs; vertex ambient
         * alpha must retain its per-vertex clamp instead of a live factor. */
        if(!edge)pos[0][0]=1e6f;
        else {pos[0][0]=-.2f;GXSetChanCtrl(GX_ALPHA0,1,GX_SRC_VTX,GX_SRC_VTX,1,GX_DF_NONE,GX_AF_NONE);fh_gx.lights[0].color[3]=.3f;}
        accept_lighting=accept_texgen=0;fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));memcpy(reference,captured,sizeof(reference));
        accept_lighting=accept_texgen=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3);
        CHECK(edge?!(fh_gx.draw.raw_position&32):captured[0].matrix_index<0);
        for(unsigned i=0;i<3;i++){
            FhVertex resolved=captured[i];fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);fh_gpu_texgen_resolve(&resolved,&fh_gx.draw,texgen_prepared);
            for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[0][c],reference[i].color[0][c]));
            for(unsigned c=0;c<3;c++)CHECK(near(resolved.uv[0][c],reference[i].uv[0][c]));
        }
    }
    accept_lighting=accept_texgen=omit_texgens=0;fh_retained_disabled=0;
    }
}
static void flat_position_texgen_regression(void){
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=omit_texgens=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_CLR0,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    float pos[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},uv[3][2]={{0,0},{1,0},{0,1}};
    u8 colors[3][4]={{80,90,100,200},{120,180,40,240},{20,255,100,140}};
    GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_CLR0,colors,sizeof(colors),4,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
    GXSetNumTexGens(3);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
    GXSetTexCoordGen2(1,GX_TG_MTX3x4,GX_TG_POS,0,0,GX_PTTEXMTX0);
    GXSetTexCoordGen2(2,GX_TG_MTX3x4,GX_TG_POS,0,0,GX_PTTEXMTX0+3);
    GXSetTevOrder(0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);
    GXSetTevColorIn(0,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_RASC);GXSetTevAlphaIn(0,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_RASA);
    GXSetChanCtrl(GX_COLOR0,1,GX_SRC_REG,GX_SRC_VTX,0,GX_DF_NONE,GX_AF_NONE);
    GXSetChanCtrl(GX_ALPHA0,0,GX_SRC_REG,GX_SRC_VTX,0,GX_DF_NONE,GX_AF_NONE);
    const u8 list[]={GX_TRIANGLES,0,3,0,0,0,1,1,1,2,2,2};FhVertex reference[3];
    unsigned start,h,m,v,used;fh_retained_stats(&start,&m,&v,&used);
    for(unsigned trial=0;trial<32;trial++){
        float model[3][4]={{1,0,0,trial>=8?.125f:0},{0,1,0,0},{0,0,1,0}};
        float post[3][4]={{.5f,0,0,.5f},{0,.5f,0,.5f},{0,0,trial>=16?.5f:1,1}};
        GXLoadPosMtxImm(model,0);GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);GXLoadTexMtxImm(post,GX_PTTEXMTX0+3,GX_MTX3x4);
        GXSetChanAmbColor(GX_COLOR0,(GXColor){40+trial*3,180-trial,80,255});
        if(trial==24)pos[0][0]+=.125f;
        accept_lighting=0;fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));memcpy(reference,captured,sizeof(reference));
        accept_lighting=1;fh_retained_disabled=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3 && fh_gx.draw.raw_position==35 && (lighting_prepared.mask&FH_LIGHT_FLAT));
        for(unsigned i=0;i<3;i++){
            FhVertex resolved=captured[i];fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
            for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[0][c],reference[i].color[0][c]));
            CHECK(!memcmp(resolved.uv,reference[i].uv,3*sizeof(resolved.uv[0])));
        }
    }
    fh_retained_stats(&h,&m,&v,&used);CHECK(h>=start+28);
    accept_lighting=0;fh_retained_disabled=0;
}
static void source_ranges_regression(void){
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=0;
    fh_retained_disabled=0;fh_retained_ranges_enabled=1;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetNumTexGens(0);
    float positions[64][3];
    for(unsigned i=0;i<64;i++){positions[i][0]=-.2f+i*.001f;positions[i][1]=.1f;positions[i][2]=-.5f;}
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);
    u8 list[]={GX_TRIANGLES,0,3,20,21,22};FhGxSourceRange range[12]={0};
    CHECK(fh_gx_source_ranges(list,sizeof(list),0,1,range));
    CHECK(range[0].offset==240 && range[0].bytes==36);
    unsigned h,m,v,used,before;uint64_t acquired=fh_retained_range_used_bytes;
    vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(vertex_count==3);
    CHECK(fh_retained_range_used_bytes==acquired+36);
    fh_retained_stats(&before,&m,&v,&used);
    positions[0][0]=.9f;positions[63][2]=-.9f;
    vertex_count=0;GXCallDisplayList(list,sizeof(list));
    fh_retained_stats(&h,&m,&v,&used);CHECK(h==before+1 && vertex_count==3);
    /* Force source-token retirement even though this mesh owns only36B. */
    static unsigned char churn[200006];
    for(unsigned bank=0;bank<6;bank++)CHECK(fh_retained_source_acquire(churn+bank,200000)!=0);
    before=h;vertex_count=0;GXCallDisplayList(list,sizeof(list));
    fh_retained_stats(&h,&m,&v,&used);CHECK(h==before && vertex_count==3);
    for(unsigned trial=0;trial<8;trial++){
        if(trial==1)positions[21][0]=.3f;
        if(trial==2)list[3]=23;
        if(trial==3)positions[23][1]=.25f;
        if(trial==4){list[3]=50;positions[50][0]=.5f;}
        if(trial==5)GXSetArray(GX_VA_POS,positions,sizeof(positions),0,true);
        if(trial==6)GXSetArray(GX_VA_POS,positions+1,sizeof(positions)-12,12,true);
        FhVertex fresh[3];fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3);memcpy(fresh,captured,sizeof(fresh));fh_retained_disabled=0;
        vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(vertex_count==3 && !memcmp(fresh,captured,sizeof(fresh)));
    }
    /* Independent NBT3 indices address separate vector slices in each element. */
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_NRM,GX_INDEX16);GXSetVtxAttrFmt(0,GX_VA_NRM,GX_NRM_NBT3,GX_S16,0);
    unsigned char normals[512]={0};GXSetArray(GX_VA_NRM,normals,sizeof(normals),18,false);
    const u8 nbt[]={GX_TRIANGLES,0,1,0,3,0,1,0,7};
    CHECK(fh_gx_source_ranges(nbt,sizeof(nbt),0,2,range));
    CHECK(range[1].offset==24 && range[1].bytes==120); /* [1*18+6,7*18+12+6) */
    CHECK(!fh_gx_source_ranges(nbt,sizeof(nbt)-1,0,2,range));
    GXSetArray(GX_VA_NRM,normals,143,18,false);CHECK(!fh_gx_source_ranges(nbt,sizeof(nbt),0,2,range));
    GXSetArray(GX_VA_NRM,normals,sizeof(normals),0xffffffffu,false);CHECK(!fh_gx_source_ranges(nbt,sizeof(nbt),0,2,range));
    /* Exercise all three independently indexed normal vectors through decoding. */
    fh_gx_reset();GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_NRM,GX_INDEX16);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_NRM,GX_NRM_NBT3,GX_F32,0);
    float vectors[8][9];for(unsigned i=0;i<72;i++)((float*)vectors)[i]=.01f*(i+1);
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);GXSetArray(GX_VA_NRM,vectors,sizeof(vectors),36,true);
    GXSetNumTexGens(3);
    for(unsigned i=0;i<3;i++)GXSetTexCoordGen2(i,GX_TG_MTX3x4,GX_TG_NRM+i,GX_IDENTITY,0,GX_PTIDENTITY);
    const u8 combined[]={GX_TRIANGLES,0,3,0,0,3,0,1,0,7,1,0,3,0,1,0,7,2,0,3,0,1,0,7};
    for(unsigned trial=0;trial<9;trial++){
        unsigned slice=trial%3,index=slice==0?3:slice==1?1:7;vectors[index][slice*3]+=.0625f;
        FhVertex fresh[3];fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(combined,sizeof(combined));
        CHECK(vertex_count==3);memcpy(fresh,captured,sizeof(fresh));fh_retained_disabled=0;
        for(unsigned repeat=0;repeat<2;repeat++){
            vertex_count=0;GXCallDisplayList(combined,sizeof(combined));CHECK(vertex_count==3 && !memcmp(fresh,captured,sizeof(fresh)));
        }
    }
    /* Large BE indices and trailing NOP bytes; no widened source access. */
    static unsigned char wide[65538];GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX16);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_U8,0);GXSetArray(GX_VA_POS,wide,sizeof(wide),1,false);
    const u8 last[]={GX_POINTS,0,1,255,255,0,0};
    CHECK(fh_gx_source_ranges(last,sizeof(last),0,1,range));CHECK(range[0].offset==65535 && range[0].bytes==3);
    GXSetArray(GX_VA_POS,wide,sizeof(wide)-1,1,false);CHECK(!fh_gx_source_ranges(last,sizeof(last),0,1,range));
    CHECK(!fh_gx_source_ranges(last,sizeof(last),1,1,range));
    const u8 command[]={0x61,0,0,0,0};CHECK(!fh_gx_source_ranges(command,sizeof(command),0,1,range));
    CHECK(fh_retained_range_scans>0 && fh_retained_range_used_bytes<fh_retained_range_full_bytes);
}
static void retained_sets_regression(void){
    static u8 storage[32768] __attribute__((aligned(32)));
    u8 *lists[5];unsigned found=0;
    unsigned bucket=fh_geometry_primary_set((uintptr_t)storage,127);
    const u8 triangle[]={GX_TRIANGLES,0,3,0,1,2};
    for(unsigned i=0;i+sizeof(triangle)<=sizeof(storage) && found<5;i+=8){
        uintptr_t address=(uintptr_t)(storage+i);
        if(fh_geometry_primary_set(address,127)!=bucket)continue;
        CHECK(fh_geometry_secondary_set(address,127)!=bucket);
        lists[found++]=storage+i;memcpy(storage+i,triangle,sizeof(triangle));
    }
    CHECK(found==5);
    for(unsigned mode=0;mode<2;mode++){
        fh_gx_reset();fh_retained_two_sets=mode;fh_retained_disabled=0;
        gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=0;
        GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
        GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetNumTexGens(0);
        float source[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}};
        GXSetArray(GX_VA_POS,source,sizeof(source),12,true);
        unsigned h,m,v,used,start_h,start_m,secondary=fh_retained_secondary_hits;
        fh_retained_stats(&start_h,&start_m,&v,&used);
        for(unsigned pass=0;pass<4;pass++)for(unsigned i=0;i<5;i++){
            vertex_count=0;GXCallDisplayList(lists[i],sizeof(triangle));
            CHECK(vertex_count==3);
            for(unsigned j=0;j<3;j++)CHECK(!memcmp(captured[j].position,source[j],12));
        }
        fh_retained_stats(&h,&m,&v,&used);
        CHECK(h-start_h==(mode?15:0));CHECK(m-start_m==(mode?5:20));
        CHECK(mode?fh_retained_secondary_hits>secondary:fh_retained_secondary_hits==secondary);
        /* Both owned list bytes and source bytes must still invalidate reuse. */
        source[0][0]=.7f;lists[4][3]=2;
        for(unsigned i=0;i<5;i++){
            vertex_count=0;GXCallDisplayList(lists[i],sizeof(triangle));
            CHECK(vertex_count==3);
            CHECK(captured[0].position[0]==source[i==4?2:0][0]);
        }
        lists[4][3]=0;
    }
    fh_retained_two_sets=0;
}
static void retained_alias_regression(void){
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=0;fh_retained_disabled=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetNumTexGens(0);
    float source[2][3][3]={{{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}}};memcpy(source[1],source[0],sizeof(source[0]));
    const u8 list[]={GX_TRIANGLES,0,3,0,1,2};FhVertex reference[3];unsigned h,m,v,used,start;
    fh_retained_stats(&start,&m,&v,&used);
    for(unsigned trial=0;trial<12;trial++){
        unsigned slot=trial&1;
        if(trial==6)source[slot][0][0]=.25f;
        if(trial==8)memcpy(source[1],source[0],sizeof(source[0]));
        GXSetArray(GX_VA_POS,source[slot],sizeof(source[slot]),12,true);
        fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));memcpy(reference,captured,sizeof(reference));
        fh_retained_disabled=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==3 && !memcmp(reference,captured,sizeof(reference)));
    }
    fh_retained_stats(&h,&m,&v,&used);CHECK(h>=start+10);
}
static void retained_scope_regression(void){
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=0;fh_retained_disabled=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetNumTexGens(0);
    float source[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},other[3][3];memcpy(other,source,sizeof(source));
    const void *ranges[4]={source,0,0,0};unsigned sizes[4]={sizeof(source),0,0,0};
    const u8 list[]={GX_TRIANGLES,0,3,0,1,2};
    fh_retained_scope_validate=0;fh_retained_scope_disabled=0;
    GXSetArray(GX_VA_POS,source,sizeof(source),12,true);
    vertex_count=0;GXCallDisplayList(list,sizeof(list));
    fh_retained_scope_begin(ranges,sizes);
    unsigned start=fh_retained_scope_reuses;
    for(unsigned i=0;i<4;i++){vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(vertex_count==3 && captured[0].position[0]==source[0][0]);}
    CHECK(fh_retained_scope_reuses>=start+3);
    source[0][0]=.7f;fh_gx_cache_range(source,sizeof(source),1);
    vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(captured[0].position[0]==.7f);
    GXSetArray(GX_VA_POS,other,sizeof(other),12,true);
    for(unsigned i=0;i<3;i++){other[0][0]=.1f*i;vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(captured[0].position[0]==other[0][0]);}
    fh_retained_scope_end();source[0][0]=-.8f;GXSetArray(GX_VA_POS,source,sizeof(source),12,true);
    vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(captured[0].position[0]==-.8f);
    fh_retained_scope_validate=1;fh_retained_scope_begin(ranges,sizes);
    for(unsigned i=0;i<2;i++){vertex_count=0;GXCallDisplayList(list,sizeof(list));}
    unsigned violations=fh_retained_scope_violations;source[0][0]=.9f;
    vertex_count=0;GXCallDisplayList(list,sizeof(list));
    CHECK(fh_retained_scope_violations==violations+1 && captured[0].position[0]==.9f);
    fh_retained_scope_end();fh_retained_scope_validate=0;
    start=fh_retained_scope_reuses;fh_retained_scope_disabled=1;fh_retained_scope_begin(ranges,sizes);
    for(unsigned i=0;i<3;i++){source[0][0]=-.1f*i;vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(captured[0].position[0]==source[0][0]);}
    CHECK(fh_retained_scope_reuses==start);fh_retained_scope_end();fh_retained_scope_disabled=0;
}
static void changing_retained_regression(void){
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    float pos[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},uv[3][2]={{0,0},{1,0},{0,1}};
    GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
    GXSetNumTexGens(1);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_TEX0,GX_TEXMTX0,0,GX_PTIDENTITY);
    static u8 list[3+384*2]={GX_TRIANGLES,1,128};static FhVertex expected_output[384];
    for(unsigned i=0;i<384;i++)list[3+i*2]=list[4+i*2]=i%3;
    unsigned compiled_start=fh_compiled_hits,stable_hits=0;
    for(unsigned revision=0;revision<40;revision++){
        float matrix[3][4]={{1,0,0,revision*.001f},{0,1,0,0},{0,0,1,0}};
        pos[0][0]=-.2f+revision*.001f;uv[1][1]=revision*.002f;GXLoadTexMtxImm(matrix,GX_TEXMTX0,GX_MTX2x4);
        fh_retained_disabled=1;accept_compiled=0;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==384);memcpy(expected_output,captured,sizeof(expected_output));
        fh_retained_disabled=0;accept_compiled=revision!=5;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==384 && !memcmp(expected_output,captured,sizeof(expected_output)));
        /* A renderer refusal still records and renders full current geometry. */
        if(revision==5)CHECK(!fh_retained_prefer_compiled);
    }
    CHECK(fh_compiled_hits>compiled_start+25);
    /* Once the mutable stream settles, bounded readmission must recover
     * immutable hits rather than staying permanently on the indexed route. */
    for(unsigned repeat=0;repeat<36;repeat++){
        unsigned h,m,v,used,before;fh_retained_stats(&before,&m,&v,&used);
        vertex_count=0;GXCallDisplayList(list,sizeof(list));fh_retained_stats(&h,&m,&v,&used);
        CHECK(vertex_count==384 && !memcmp(expected_output,captured,sizeof(expected_output)));
        stable_hits+=h-before;
    }
    CHECK(stable_hits>0);accept_compiled=0;fh_retained_disabled=0;
}
static void compiled_matrix_mask_regression(void){
    unsigned char list[]={GX_TRIANGLES,0,6,0,11,3,22,0,33,27,44,9,55,6,66};
    const FhCompiledMesh *mesh=fh_compiled_mesh(list,sizeof(list),2,0);
    CHECK(mesh && mesh->matrix_mask==(1u|2u|512u|8u|4u));
    unsigned hits=fh_compiled_hits;
    CHECK(fh_compiled_mesh(list,sizeof(list),2,0)->matrix_mask==mesh->matrix_mask && fh_compiled_hits>hits);
    list[9]=18;mesh=fh_compiled_mesh(list,sizeof(list),2,0);
    CHECK(mesh && mesh->matrix_mask==(1u|2u|64u|8u|4u));
    list[4]=255;CHECK(fh_compiled_mesh(list,sizeof(list),2,0)->matrix_mask==mesh->matrix_mask);
    list[3]=1;CHECK(fh_compiled_mesh(list,sizeof(list),2,0)->matrix_mask==0x3ff);
    list[3]=30;CHECK(fh_compiled_mesh(list,sizeof(list),2,0)->matrix_mask==0x3ff);
    CHECK(!fh_compiled_mesh(list,sizeof(list)-1,2,0));
    list[3]=0;list[0]=GX_TRIANGLESTRIP;mesh=fh_compiled_mesh(list,sizeof(list),2,0);
    CHECK(mesh && mesh->matrix_mask==(1u|2u|64u|8u|4u));
    list[0]=GX_TRIANGLEFAN;CHECK(fh_compiled_mesh(list,sizeof(list),2,0)->matrix_mask==mesh->matrix_mask);
    /* Both unused tail packets of a quad remain in the conservative union. */
    list[0]=GX_QUADS;mesh=fh_compiled_mesh(list,sizeof(list),2,0);
    CHECK(mesh && mesh->count==6 && mesh->matrix_mask==(1u|2u|64u|8u|4u));
}
static void compiled_mesh_regression(void){
    compiled_matrix_mask_regression();
    fh_gx_reset();gpu_transform_available=accept_lighting=accept_texgen=1;
    fh_retained_disabled=1;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_PNMTXIDX,GX_DIRECT);GXSetVtxDesc(GX_VA_TEX0MTXIDX,GX_DIRECT);
    GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_NRM,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    float pos[3][3]={{-.2f,-.3f,-.5f},{.4f,.1f,-.5f},{0,.5f,-.5f}},normals[3][3]={{0,0,1},{0,0,1},{0,0,1}},uv[3][2]={{0,0},{1,0},{0,1}},alternate[3][3];
    GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_NRM,normals,sizeof(normals),12,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
    GXSetNumTexGens(2);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_NRM,GX_TEXMTX0,0,GX_PTTEXMTX0);
    GXSetTexCoordGen2(1,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
    GXSetChanCtrl(GX_COLOR0,1,GX_SRC_REG,GX_SRC_REG,1,GX_DF_CLAMP,GX_AF_SPOT);
    GXSetChanCtrl(GX_ALPHA0,0,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
    GXLightObj lamp;GXInitLightPos(&lamp,2,1,5);GXInitLightDir(&lamp,0,0,-1);GXInitLightColor(&lamp,(GXColor){80,120,160,255});GXInitLightAttn(&lamp,1,.1f,.1f,1,.01f,.01f);GXLoadLightObjImm(&lamp,1);
    float post[3][4]={{-.5f,0,0,.5f},{0,-.5f,0,.5f},{0,0,0,1}};GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);
    static unsigned char list[3+4098*5];static FhVertex expected_output[16384];
    const unsigned kinds[]={GX_TRIANGLES,GX_TRIANGLESTRIP,GX_TRIANGLEFAN,GX_QUADS};
    for(unsigned trial=0;trial<8;trial++){
        accept_lighting=accept_texgen=trial<4;
        unsigned inputs=trial>=4?192:trial==0?4098:trial==3?2732:1368;unsigned bytes=3+inputs*5;
        list[0]=kinds[trial%4];list[1]=inputs>>8;list[2]=inputs&255;
        for(unsigned i=0;i<inputs;i++){unsigned n=i%3,at=3+i*5;list[at]=n*3;list[at+1]=GX_TEXMTX0+n*3;list[at+2]=list[at+3]=list[at+4]=n;}
        for(unsigned repeat=0;repeat<2;repeat++){
            pos[0][0]+=.05f;uv[1][0]-=.025f;normals[2][0]+=.125f;
            memcpy(alternate,pos,sizeof(pos));GXSetArray(GX_VA_POS,repeat?alternate:pos,sizeof(pos),12,true);
            unsigned before=fh_compiled_hits;
            accept_compiled=0;vertex_count=0;GXCallDisplayList(list,bytes);unsigned count=vertex_count;
            CHECK(fh_gx.draw.raw_position==(trial<4?7:1) && count>=(trial<4?4096:96));memcpy(expected_output,captured,count*sizeof(*captured));
            accept_compiled=1;vertex_count=0;GXCallDisplayList(list,bytes);
            CHECK(vertex_count==count && !memcmp(expected_output,captured,count*sizeof(*captured)));
            CHECK(fh_compiled_hits>before);
        }
    }
    /* Repeated five-generator shells: palettes/post0 remain live while the
     * indexed source bytes, colors and other generators remain verified. */
    accept_lighting=accept_texgen=accept_compiled=accept_compiled_fur=1;
    GXSetNumTexGens(5);
    for(unsigned i=1;i<5;i++)GXSetTexCoordGen2(i,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,i==4,GX_PTIDENTITY);
    list[0]=GX_TRIANGLES;list[1]=0;list[2]=96;
    extern unsigned fh_compiled_decode_hits,fh_compiled_decode_vertices;
    unsigned retained_before=fh_compiled_decode_hits;
    for(unsigned shell=0;shell<24;shell++){
        if(shell==3)alternate[0][1]+=.125f;
        if(shell==5)normals[1][0]+=.125f;
        if(shell==7)uv[1][0]+=.25f;
        if(shell==9)list[5]=2;
        if(shell==11)GXSetChanMatColor(GX_COLOR0A0,(GXColor){32,64,128,192});
        if(shell==13)GXSetTexCoordGen2(4,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
        /* Animated matrices are not baked into retained attributes. */
        for(unsigned m=0;m<10;m++)fh_gx.position[m][0][3]+=.01f;
        post[0][3]+=.01f;GXLoadTexMtxImm(post,GX_PTTEXMTX0,GX_MTX3x4);
        fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,3+96*5);
        CHECK(vertex_count==96 && fh_gx.draw.raw_position==7);memcpy(expected_output,captured,96*sizeof(*captured));
        fh_retained_disabled=0;
        for(unsigned repeat=0;repeat<2;repeat++){
            vertex_count=0;GXCallDisplayList(list,3+96*5);
            CHECK(vertex_count==96 && !memcmp(expected_output,captured,96*sizeof(*captured)));
        }
    }
    CHECK(fh_compiled_decode_hits>=retained_before+30 && fh_compiled_decode_vertices>=90);
    /* Disabled lighting must not force all five surface coordinates and the
     * mesh back onto ARM. Compare resolved output with scalar decoding while
     * normals, live palettes, material color and source UVs change. A stale
     * disabled-channel mask must not activate lights. */
    GXSetChanCtrl(GX_COLOR0,0,GX_SRC_REG,GX_SRC_REG,255,GX_DF_NONE,GX_AF_NONE);
    retained_before=fh_compiled_decode_hits;
    for(unsigned frame=0;frame<12;frame++){
        if(frame==4)normals[1][0]+=.0625f;
        if(frame==6)uv[1][1]+=.125f;
        for(unsigned m=0;m<3;m++){
            fh_gx.normal[m][0][1]=frame*.015625f;
            memcpy(fh_gx.texture[10+m],fh_gx.normal[m],48);
        }
        GXSetChanMatColor(GX_COLOR0A0,(GXColor){32+frame*8,64,128,192});
        accept_lighting=accept_texgen=0;fh_retained_disabled=1;vertex_count=0;
        GXCallDisplayList(list,3+96*5);CHECK(vertex_count==96);
        memcpy(expected_output,captured,96*sizeof(*captured));
        accept_lighting=accept_texgen=1;fh_retained_disabled=0;
        for(unsigned repeat=0;repeat<2;repeat++){
            vertex_count=0;GXCallDisplayList(list,3+96*5);
            CHECK(vertex_count==96 && fh_gx.draw.raw_position==7);
            CHECK(!(lighting_prepared.mask&255));
            for(unsigned i=0;i<96;i++){
                fh_gpu_texgen_resolve(captured+i,&fh_gx.draw,texgen_prepared);
                fh_gpu_lighting_resolve(captured+i,&fh_gx.draw,&lighting_prepared);
                for(unsigned c=0;c<4;c++)CHECK(fabsf(captured[i].color[0][c]-expected_output[i].color[0][c])<1e-6f);
                for(unsigned g=0;g<5;g++)for(unsigned c=0;c<3;c++)
                    CHECK(fabsf(captured[i].uv[g][c]-expected_output[i].uv[g][c])<1e-6f);
            }
        }
    }
    CHECK(fh_compiled_decode_hits>=retained_before+12);
    fh_gx.texture[10][0][0]+=.125f;vertex_count=0;
    GXCallDisplayList(list,3+96*5);CHECK(!(fh_gx.draw.raw_position&4));
    memcpy(fh_gx.texture[10],fh_gx.normal[0],48);
    GXSetChanCtrl(GX_COLOR0,1,GX_SRC_REG,GX_SRC_REG,1,GX_DF_CLAMP,GX_AF_SPOT);
    /* Scroll only the dependent-sample post transform. Compare with the
     * ordinary decoder with retention and the template feature disabled;
     * source writes and unsupported generator dependencies must stay live. */
    extern unsigned fh_fur_post_mode,fh_fur_post_checks;
    unsigned post_checks_before=fh_fur_post_checks;
    retained_before=fh_compiled_decode_hits;
    for(unsigned trial=0;trial<32;trial++){
        float scroll[3][4]={{1,.125f,0,trial*.03125f},{-.25f,1,0,-trial*.0625f},{0,0,1,0}};
        GXLoadTexMtxImm(scroll,GX_PTTEXMTX0+3,GX_MTX3x4);
        GXSetTexCoordGen2(1,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,trial==16,
            trial==26?GX_PTIDENTITY:GX_PTTEXMTX0+3);
        GXSetTexCoordGen2(2,GX_TG_MTX2x4,trial==22?GX_TG_TEXCOORD1:GX_TG_TEX0,
            GX_IDENTITY,0,trial==20?GX_PTTEXMTX0+3:GX_PTIDENTITY);
        if(trial==4)alternate[0][0]+=.0625f;
        if(trial==8)uv[1][1]+=.125f;
        if(trial==12)normals[2][1]+=.0625f;
        if(trial==24)list[7]=1;
        fh_fur_post_mode=0;fh_retained_disabled=1;vertex_count=0;GXCallDisplayList(list,3+96*5);
        CHECK(vertex_count==96);memcpy(expected_output,captured,96*sizeof(*captured));
        fh_fur_post_mode=2;fh_retained_disabled=0;vertex_count=0;compiled_post_seen=0;
        GXCallDisplayList(list,3+96*5);
        CHECK(vertex_count==96 && !memcmp(expected_output,captured,96*sizeof(*captured)));
        if(trial==16 || trial==20 || trial==22 || trial==26)CHECK(!compiled_post_seen);
        else CHECK(compiled_post_seen && !memcmp(compiled_post_seen_matrix,scroll,48));
    }
    CHECK(fh_fur_post_checks>post_checks_before && fh_compiled_decode_hits>=retained_before+15);
    fh_fur_post_mode=0;
    GXSetTexCoordGen2(1,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
    GXSetTexCoordGen2(2,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
    /* Unused poisoned matrices cannot affect vertices which only consume
     * slots0..2. Compare expanded scalar lighting/texgen to resolved GPU data,
     * then prove that a used poison still retains the original fallback. */
    extern unsigned fh_palette_scope_mode;
    unsigned saved_scope=fh_palette_scope_mode;
    float saved_normal[3][4],saved_model[3][4],saved_texture[3][4];
    memcpy(saved_normal,fh_gx.normal[9],48);memcpy(saved_model,fh_gx.position[9],48);memcpy(saved_texture,fh_gx.texture[19],48);
    uint32_t nan_bits=0x7fc00123u;
    for(unsigned poison=0;poison<3;poison++){
        memcpy(fh_gx.normal[9],saved_normal,48);memcpy(fh_gx.position[9],saved_model,48);memcpy(fh_gx.texture[19],saved_texture,48);
        float *target=poison==0?&fh_gx.normal[9][0][0]:poison==1?&fh_gx.position[9][0][0]:&fh_gx.texture[19][0][0];
        memcpy(target,&nan_bits,4);
        fh_retained_disabled=1;fh_palette_scope_mode=0;accept_lighting=accept_texgen=0;vertex_count=0;
        GXCallDisplayList(list,3+96*5);CHECK(vertex_count==96);memcpy(expected_output,captured,96*sizeof(*captured));
        fh_palette_scope_mode=2;accept_lighting=accept_texgen=1;vertex_count=0;
        GXCallDisplayList(list,3+96*5);CHECK(vertex_count==96 && fh_gx.draw.raw_position==7);
        for(unsigned i=0;i<96;i++){
            FhVertex resolved=captured[i];fh_gpu_texgen_resolve(&resolved,&fh_gx.draw,texgen_prepared);fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
            for(unsigned c=0;c<4;c++)CHECK(near(resolved.position[c],expected_output[i].position[c]));
            for(unsigned ch=0;ch<2;ch++)for(unsigned c=0;c<4;c++)CHECK(near(resolved.color[ch][c],expected_output[i].color[ch][c]));
            for(unsigned u=0;u<5;u++)for(unsigned c=0;c<3;c++)CHECK(near(resolved.uv[u][c],expected_output[i].uv[u][c]));
        }
    }
    memcpy(fh_gx.normal[9],saved_normal,48);memcpy(fh_gx.position[9],saved_model,48);memcpy(fh_gx.texture[19],saved_texture,48);
    float used=fh_gx.normal[2][0][0];memcpy(&fh_gx.normal[2][0][0],&nan_bits,4);vertex_count=0;
    GXCallDisplayList(list,3+96*5);CHECK(!(fh_gx.draw.raw_position&2));fh_gx.normal[2][0][0]=used;
    fh_palette_scope_mode=saved_scope;

    /* Material lowering consumes 0/2/4. An animated unused generator must
     * neither perform coordinate work nor invalidate the resident packet.
     * Reintroducing a generated-coordinate dependency must restore it. */
    unsigned input_hits=fh_compiled_decode_hits;
    for(unsigned frame=0;frame<12;frame++){
        float scroll[3][4]={{1,0,0,frame*.125f},{0,1,0,-frame*.0625f},{0,0,1,0}};
        GXLoadTexMtxImm(scroll,GX_PTTEXMTX0+3,GX_MTX3x4);
        GXSetTexCoordGen2(1,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTTEXMTX0+3);
        GXSetTexCoordGen2(4,GX_TG_MTX2x4,frame>=8?GX_TG_TEXCOORD1:GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
        material_texgen_inputs=255;fh_retained_disabled=1;vertex_count=0;
        GXCallDisplayList(list,3+96*5);memcpy(expected_output,captured,96*sizeof(*captured));
        FhDrawState reference_state=fh_gx.draw;FhGpuLighting reference_light=lighting_prepared;
        float reference_post[3][4];memcpy(reference_post,texgen_prepared,48);
        for(unsigned i=0;i<96;i++){
            fh_gpu_texgen_resolve(expected_output+i,&reference_state,reference_post);
            fh_gpu_lighting_resolve(expected_output+i,&reference_state,&reference_light);
        }
        material_texgen_inputs=21;fh_retained_disabled=0;vertex_count=0;
        GXCallDisplayList(list,3+96*5);CHECK(vertex_count==96);
        CHECK(fh_gx_texgen_inputs==(frame>=8?23:21));
        for(unsigned i=0;i<96;i++){
            fh_gpu_texgen_resolve(captured+i,&fh_gx.draw,texgen_prepared);
            fh_gpu_lighting_resolve(captured+i,&fh_gx.draw,&lighting_prepared);
            for(unsigned g=0;g<5;g+=2)for(unsigned c=0;c<3;c++)
                CHECK(fabsf(captured[i].uv[g][c]-expected_output[i].uv[g][c])<1e-6f);
        }
    }
    CHECK(fh_compiled_decode_hits>=input_hits+6);
    material_texgen_inputs=255;
    GXSetTexCoordGen2(1,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
    GXSetTexCoordGen2(4,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);

    /* Indexed ordinary scenery: preserve the independent expanded decoder
     * oracle across changing arrays, UV transforms, list bytes and palettes. */
    fh_gx_reset();gpu_transform_available=accept_lighting=accept_texgen=1;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    GXSetNumTexGens(1);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
    GXSetChanCtrl(GX_COLOR0A0,0,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
    GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_TEX0,uv,sizeof(uv),8,true);
    list[0]=GX_TRIANGLES;list[1]=1;list[2]=128;
    for(unsigned i=0;i<384;i++)list[3+2*i]=list[4+2*i]=i%3;
    retained_before=fh_compiled_decode_hits;
    for(unsigned trial=0;trial<16;trial++){
        if(trial==2)pos[0][1]+=.125f;
        if(trial==4)uv[1][1]+=.25f;
        if(trial==6)list[3]=2;
        if(trial==8){memcpy(alternate,pos,sizeof(pos));GXSetArray(GX_VA_POS,alternate,sizeof(alternate),12,true);}
        if(trial==10)alternate[2][0]-=.125f;
        GXSetChanMatColor(GX_COLOR0A0,(GXColor){trial*8,100,200,255-trial*3});
        fh_gx.position[0][0][3]=trial*.01f;
        fh_retained_disabled=1;accept_compiled=0;vertex_count=0;GXCallDisplayList(list,3+384*2);
        CHECK(vertex_count==384 && fh_gx.draw.raw_position==35);memcpy(expected_output,captured,384*sizeof(*captured));
        fh_retained_disabled=0;accept_compiled=1;
        for(unsigned repeat=0;repeat<2;repeat++){
            vertex_count=0;GXCallDisplayList(list,3+384*2);
            CHECK(vertex_count==384 && !memcmp(expected_output,captured,384*sizeof(*captured)));
        }
    }
    CHECK(fh_compiled_decode_hits>=retained_before+24);
    accept_compiled=accept_compiled_fur=0;fh_retained_disabled=0;
}
static void large_compiled_regression(void){
    fh_gx_reset();gpu_transform_available=accept_lighting=accept_texgen=1;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX16);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX16);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    GXSetNumTexGens(1);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY);
    GXSetChanCtrl(GX_COLOR0A0,0,GX_SRC_REG,GX_SRC_REG,0,GX_DF_NONE,GX_AF_NONE);
    static FhVertex expected_output[2049];
    extern unsigned fh_compiled_decode_hits;unsigned retained_before;
    /* Scenery exceeding the former 1024-unique limit must retain all attributes,
     * including tail mutations. The over-limit case still decodes normally. */
    static float large_pos[2049][3],large_uv[2049][2];
    static unsigned char large_list[3+2049*4];
    const unsigned sizes[]={1023,1024,1025,1083,2047,2048,2049};
    GXSetVtxDesc(GX_VA_POS,GX_INDEX16);GXSetVtxDesc(GX_VA_TEX0,GX_INDEX16);
    for(unsigned i=0;i<2049;i++){
        large_pos[i][0]=i*.0001f;large_pos[i][1]=.1f;large_pos[i][2]=-.5f;
        large_uv[i][0]=i*.0002f;large_uv[i][1]=.2f;
    }
    GXSetArray(GX_VA_POS,large_pos,sizeof(large_pos),12,true);GXSetArray(GX_VA_TEX0,large_uv,sizeof(large_uv),8,true);
    for(unsigned shape=0;shape<sizeof(sizes)/sizeof(*sizes);shape++){
        unsigned unique=sizes[shape],count=(unique+2)/3*3,bytes=3+count*4;
        large_list[0]=GX_TRIANGLES;large_list[1]=count>>8;large_list[2]=count;
        for(unsigned i=0;i<count;i++){
            unsigned at=3+i*4,index=i%unique;
            large_list[at]=large_list[at+2]=index>>8;large_list[at+1]=large_list[at+3]=index;
        }
        for(unsigned revision=0;revision<3;revision++){
            large_pos[unique-1][1]+=.125f;large_uv[unique-1][0]+=.25f;
            fh_retained_disabled=1;accept_compiled=0;vertex_count=0;GXCallDisplayList(large_list,bytes);
            CHECK(vertex_count==count);memcpy(expected_output,captured,count*sizeof(*captured));
            fh_retained_disabled=0;accept_compiled=1;retained_before=fh_compiled_decode_hits;
            for(unsigned repeat=0;repeat<2;repeat++){
                vertex_count=0;GXCallDisplayList(large_list,bytes);
                CHECK(vertex_count==count && !memcmp(expected_output,captured,count*sizeof(*captured)));
            }
            CHECK(fh_compiled_decode_hits-retained_before==(unique<=FH_COMPILED_RETAINED_UNIQUE?1:0));
        }
    }
    accept_compiled=accept_compiled_fur=0;fh_retained_disabled=0;
}
static void shared_sources_regression(void){
    fh_gx_reset();gpu_transform_available=1;accept_lighting=accept_texgen=accept_compiled=0;
    gpu_cull_available=cpu_cull_available=0;fh_retained_disabled=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetNumTexGens(0);
    /* Thirty-two lists share a large mutable array. Duplicating this source
     * alone exceeds the entire four-MiB cache before decoded vertices. */
    static float positions[16384][3];static u8 lists[32][112];
    for(unsigned i=0;i<16384;i++)for(unsigned c=0;c<3;c++)positions[i][c]=(float)(i+c)*.125f;
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);
    for(unsigned l=0;l<32;l++){
        lists[l][0]=GX_TRIANGLES;lists[l][1]=0;lists[l][2]=96;
        for(unsigned i=0;i<96;i++)lists[l][3+i]=(i+l)%192;
        vertex_count=0;GXCallDisplayList(lists[l],sizeof(lists[l]));CHECK(vertex_count==96);
    }
    unsigned before,h,m,v,used;fh_retained_stats(&before,&m,&v,&used);
    for(unsigned l=0;l<32;l++){
        vertex_count=0;GXCallDisplayList(lists[l],sizeof(lists[l]));CHECK(vertex_count==96);
        for(unsigned i=0;i<96;i++)for(unsigned c=0;c<3;c++)CHECK(captured[i].position[c]==positions[(i+l)%192][c]);
    }
    fh_retained_stats(&h,&m,&v,&used);CHECK(h>=before+28);CHECK(used<=4*1024*1024);
    /* Both referenced and unreferenced bytes change. Shared immutable copies
     * must not let one owner's refresh make another owner's output stale. */
    for(unsigned revision=0;revision<8;revision++){
        positions[0][0]+=1;positions[16383][2]+=2;
        for(unsigned l=0;l<2;l++){
            vertex_count=0;GXCallDisplayList(lists[l],sizeof(lists[l]));CHECK(vertex_count==96);
            for(unsigned i=0;i<96;i++)for(unsigned c=0;c<3;c++)CHECK(captured[i].position[c]==positions[(i+l)%192][c]);
        }
    }
    /* Distinct large bindings force source-ring wrap independently of the
     * per-binding recycling policy. Old geometry must not alias new bytes. */
    static float churn[6][16384][3];
    for(unsigned bank=0;bank<6;bank++){
        for(unsigned i=0;i<192;i++)for(unsigned c=0;c<3;c++)churn[bank][i][c]=(float)(bank*100+i+c);
        GXSetArray(GX_VA_POS,churn[bank],sizeof(churn[bank]),12,true);
        vertex_count=0;GXCallDisplayList(lists[0],sizeof(lists[0]));CHECK(vertex_count==96);
        for(unsigned i=0;i<96;i++)for(unsigned c=0;c<3;c++)CHECK(captured[i].position[c]==churn[bank][i][c]);
    }
    GXSetArray(GX_VA_POS,positions,sizeof(positions),12,true);
    vertex_count=0;GXCallDisplayList(lists[0],sizeof(lists[0]));CHECK(captured[0].position[0]==positions[0][0]);
}
static void sphere_deferred_regression(void){
    fh_gx_reset();gpu_transform_available=accept_lighting=accept_texgen=accept_compiled=1;
    fh_retained_disabled=0;
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);GXSetVtxDesc(GX_VA_NRM,GX_INDEX8);
    GXSetVtxAttrFmt(0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    float pos[3][3]={{-.3f,-.2f,-2},{.3f,-.2f,-2},{0,.4f,-2}},normal[3][3]={{0,0,1},{0,1,1},{0,0,-1}};
    GXSetArray(GX_VA_POS,pos,sizeof(pos),12,true);GXSetArray(GX_VA_NRM,normal,sizeof(normal),12,true);
    GXSetNumTexGens(1);GXSetTexCoordGen2(0,GX_TG_MTX2x4,GX_TG_NRM,GX_TEXMTX0,0,GX_PTIDENTITY);
    float tex[3][4]={{.5f,0,0,.5f},{0,-.5f,0,.5f},{0,0,0,1}};GXLoadTexMtxImm(tex,GX_TEXMTX0,GX_MTX2x4);
    GXSetNumTevStages(2);
    for(unsigned c=0;c<2;c++){
        fh_gx.draw.tev[c]=(FhTevStage){.color={15,10,11,c?0:8},.alpha={7,7,7,7},.color_clamp=1,.alpha_clamp=1,.channel=4+c};
        GXSetChanCtrl(c,1,GX_SRC_REG,GX_SRC_REG,3u<<(c*3),GX_DF_NONE,GX_AF_SPEC);
        GXSetChanCtrl(c+2,1,GX_SRC_REG,GX_SRC_REG,4u<<(c*3),GX_DF_CLAMP,GX_AF_SPOT);
        GXSetChanAmbColor(c+4,(GXColor){10,20,30,40});GXSetChanMatColor(c+4,(GXColor){200,150,100,220});
    }
    for(unsigned i=0;i<6;i++){
        GXLightObj l;GXInitLightPos(&l,i*100,200,1000);GXInitLightDir(&l,0,0,-1);
        GXInitLightAttn(&l,.1f,.2f,.5f,1,i%3==2?0:-.1f,i%3==2?0:.3f);
        GXInitLightColor(&l,(GXColor){50+i*10,100,150,180});GXLoadLightObjImm(&l,1u<<i);
    }
    static u8 list[3+384*2]={GX_TRIANGLES,1,128};static FhVertex expected_output[384];
    for(unsigned i=0;i<384;i++)list[3+i*2]=list[4+i*2]=i%3;
    extern unsigned fh_compiled_decode_hits;unsigned before=fh_compiled_decode_hits;
    for(unsigned trial=0;trial<4;trial++){
        fh_gx.lights[0].position[0]+=50;fh_gx.material[1][0]-=.1f;
        if(trial==2)normal[1][0]+=.25f;
        fh_gpu_sphere_disabled=1;vertex_count=0;GXCallDisplayList(list,sizeof(list));
        CHECK(vertex_count==384 && fh_gx.draw.raw_position==1);memcpy(expected_output,captured,sizeof(expected_output));
        fh_gpu_sphere_disabled=0;
        for(unsigned repeat=0;repeat<2;repeat++){
            vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(vertex_count==384 && fh_gx.draw.raw_position==67);
            CHECK(lighting_prepared.mask&FH_LIGHT_SPHERE);
            for(unsigned i=0;i<384;i++){
                FhVertex resolved=captured[i];fh_gpu_lighting_resolve(&resolved,&fh_gx.draw,&lighting_prepared);
                for(unsigned c=0;c<8;c++)CHECK(fabsf(((float*)resolved.color)[c]-((float*)expected_output[i].color)[c])<2e-6f);
                CHECK(!memcmp(resolved.position,expected_output[i].position,16) && !memcmp(resolved.uv[0],expected_output[i].uv[0],12));
            }
        }
    }
    CHECK(fh_compiled_decode_hits>=before+6);
    fh_gx.lights[2].k[1]=.1f;vertex_count=0;GXCallDisplayList(list,sizeof(list));CHECK(fh_gx.draw.raw_position==1);
    fh_gpu_sphere_disabled=1;accept_lighting=accept_texgen=accept_compiled=0;
}
void test_main(void) {
#ifdef FH_TEST_RETAINED_KEY
    fh_retained_key_mode=2;
#endif
#ifdef FH_TEST_SOURCE_RANGES
    source_ranges_regression();retained_alias_regression();retained_scope_regression();
    retained_texgens_regression();large_retained_regression();shared_sources_regression();
    changing_retained_regression();test_pass();
#endif
#ifdef FH_TEST_RETAINED_SETS
    retained_sets_regression();fh_retained_two_sets=1;
    retained_alias_regression();retained_scope_regression();retained_texgens_regression();
    shared_sources_regression();test_pass();
#endif
#ifdef FH_TEST_SPHERE_LIGHTING
    sphere_deferred_regression();test_pass();
#endif
#ifdef FH_TEST_LARGE_COMPILED
    large_compiled_regression();test_pass();
#endif
#ifdef FH_TEST_SHARED_SOURCES
    shared_sources_regression();test_pass();
#endif
#ifdef FH_TEST_RAW_UV
    raw_uv_regression();post_uv_regression();untextured_flat_regression();flat_position_texgen_regression();retained_alias_regression();retained_scope_regression();test_pass();
#endif
#ifdef FH_TEST_CHANGING_GEOMETRY
    changing_retained_regression();test_pass();
#endif
#ifdef FH_TEST_RETAINED_TEXGENS
    texgen_cache_regression();retained_texgens_regression();test_pass();
#endif
#ifdef FH_TEST_COMPILED_GEOMETRY
    compiled_mesh_regression();test_pass();
#endif
#ifdef FH_TEST_LARGE_GEOMETRY
    large_retained_regression();test_pass();
#endif
    /* Every word alignment modulo a cache line, byte-exact payload and guards.
     * General registers live across the call as part of this compiled oracle. */
    unsigned char copy_source[180] __attribute__((aligned(32)));
    unsigned char copy_target[180] __attribute__((aligned(32)));
    for(unsigned a=0;a<8;a++)for(unsigned b=0;b<8;b++) {
        for(unsigned i=0;i<180;i++){copy_source[i]=(i*37+a*13+b)&255;copy_target[i]=0xa5;}
        fh_vertex_copy((FhVertex*)(copy_target+4*b),(const FhVertex*)(copy_source+4*a));
        for(unsigned i=0;i<180;i++)CHECK(copy_target[i]==(i>=4*b && i<4*b+148?copy_source[i-4*b+4*a]:0xa5));
    }
    fh_gx_reset();GXClearVtxDesc();
    GXSetBreakPtCallback(breakpoint_callback);GXEnableBreakPt((void*)1);event=1;
    GXSetDrawSync(123);CHECK(event==1 && GXReadDrawSync()==0);
    GXCopyDisp((void*)2,GX_TRUE);CHECK(event==6);GXDisableBreakPt();
    GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_TEX0,GX_TEX_ST,GX_F32,0);
    GXSetNumTexGens(1);GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXSetChanCtrl(GX_COLOR0A0,GX_FALSE,GX_SRC_REG,GX_SRC_VTX,0,GX_DF_NONE,GX_AF_NONE);
    GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
    for(unsigned i=0;i<3;i++) { GXPosition3f32(i,i+1,-2);GXColor4u8(20+i,40,60,255);GXTexCoord2f32(i*.5f,.75f); }
    GXEnd();CHECK(vertex_count==3);
    for(unsigned i=0;i<3;i++) {
        CHECK(near(captured[i].position[0],i) && near(captured[i].position[1],i+1) && near(captured[i].position[2],-2) && near(captured[i].position[3],1));
        CHECK(near(captured[i].color[0][0],(20+i)/255.f) && near(captured[i].color[0][3],1));
        CHECK(near(captured[i].uv[0][0],i*.5f) && near(captured[i].uv[0][1],.75f));
    }
    unsigned char list[256] __attribute__((aligned(32)));
    GXBeginDisplayList(list,sizeof(list));GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
    for(unsigned i=0;i<3;i++) { GXPosition3f32(i,i+1,-2);GXColor4u8(20+i,40,60,255);GXTexCoord2f32(i*.5f,.75f); }
    unsigned bytes=GXEndDisplayList();CHECK(bytes==96 && list[0]==0x90 && list[1]==0 && list[2]==3);
    GXCallDisplayList(list,bytes);CHECK(vertex_count==6);
    CHECK(!memcmp(captured,captured+3,3*sizeof(*captured)));
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT1,GX_VA_POS,GX_POS_XYZ,GX_S16,1);
    const unsigned char positions[]={0,2,0,4,0,6,0xff,0xfe,0xff,0xfc,0xff,0xfa,0,8,0,10,0,12};
    GXSetArray(GX_VA_POS,positions,sizeof(positions),6,false);
    const unsigned char indexed[]={0x91,0,3,0,2,0,0,0,1};
    GXCallDisplayList(indexed,sizeof(indexed));CHECK(vertex_count==9);
    CHECK(near(captured[6].position[0],4) && near(captured[7].position[0],1) && near(captured[8].position[0],-1));
    const s16 native_positions[]={2,4,6,-2,-4,-6,8,10,12};
    GXSetArray(GX_VA_POS,native_positions,sizeof(native_positions),6,true);
    GXCallDisplayList(indexed,sizeof(indexed));CHECK(vertex_count==12);
    CHECK(!memcmp(captured+6,captured+9,3*sizeof(*captured)));
    float matrix[3][4]={{1,0,0,10},{0,1,0,20},{0,0,1,30}};
    GXLoadPosMtxImm(matrix,3);GXSetCurrentMtx(3);GXCallDisplayList(indexed,sizeof(indexed));
    CHECK(near(captured[12].position[0],14) && near(captured[12].position[1],25) && near(captured[12].position[2],36));
    GXTexObj texture;unsigned char pixels[64]={0};
    GXInitTexObj(&texture,pixels,4,4,GX_TF_RGBA8,GX_CLAMP,GX_CLAMP,GX_FALSE);
    GXInitTexObjUserData(&texture,(void*)0x1234);GXLoadTexObj(&texture,GX_TEXMAP0);
    CHECK(GXGetTexObjWidth(&texture)==4 && GXGetTexObjHeight(&texture)==4 && GXGetTexObjFmt(&texture)==GX_TF_RGBA8 && GXGetTexObjUserData(&texture)==(void*)0x1234);
    CHECK(fh_gx.draw.textures[0].pixels==pixels && GXGetTexBufferSize(4,4,GX_TF_RGBA8,GX_FALSE,0)==64);
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX16);GXSetVtxDesc(GX_VA_NBT,GX_INDEX8);GXSetVtxDesc(GX_VA_NRM,GX_NONE);
    GXSetVtxAttrFmt(GX_VTXFMT3,GX_VA_POS,GX_POS_XYZ,GX_S16,1);
    GXSetVtxAttrFmt(GX_VTXFMT3,GX_VA_NBT,GX_NRM_NBT,GX_S8,0);
    const s8 nbt[]={0,0,64,0,64,0,64,0,0};GXSetArray(GX_VA_NBT,nbt,sizeof(nbt),9,true);
    const u8 nbt_list[]={0x93,0,3,0,2,0,0,0,0,0,1,0};
    GXCallDisplayList(nbt_list,sizeof(nbt_list));CHECK(vertex_count==18);
    CHECK(near(captured[15].position[0],14) && near(captured[17].position[0],9));
    GXSetVtxDesc(GX_VA_NBT,GX_NONE);CHECK(fh_gx.descriptors[GX_VA_NRM]==GX_NONE);
    fh_gx_reset();GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);
    GXSetPointSize(12,GX_TO_ONE);GXSetCullMode(GX_CULL_ALL);
    GXBegin(GX_POINTS,GX_VTXFMT0,1);GXPosition3f32(0,0,-.5f);GXEnd();
    CHECK(vertex_count==24 && fh_gx.draw.cull==GX_CULL_ALL);
    CHECK(near((captured[19].position[0]-captured[18].position[0])*320,2));
    CHECK(near((captured[18].position[1]-captured[20].position[1])*240,2));
    GXSetLineWidth(18,GX_TO_ZERO);GXBegin(GX_LINES,GX_VTXFMT0,2);
    GXPosition3f32(0,0,-.5f);GXPosition3f32(.5f,0,-.5f);GXEnd();
    CHECK(vertex_count==30 && near((captured[24].position[1]-captured[25].position[1])*240,3));
    GXBegin(GX_POINTS,GX_VTXFMT0,1);GXPosition3f32(0,0,-2);GXEnd();CHECK(vertex_count==30);
    fh_gx_reset();GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_S16,0);
    s16 shared[]={1,2,3,4,5,6,7,8,9};GXSetArray(GX_VA_POS,shared,sizeof(shared),6,true);
    const u8 repeated[]={0x90,0,6,0,1,2,0,1,2};unsigned decoded,reused,before;
    fh_gx_vertex_counters(&decoded,&before);GXCallDisplayList(repeated,sizeof(repeated));fh_gx_vertex_counters(&decoded,&reused);
    CHECK(reused-before==3 && !memcmp(captured+30,captured+33,3*sizeof(*captured)));
    shared[0]=99;GXCallDisplayList(repeated,sizeof(repeated));CHECK(near(captured[36].position[0],99));
    GXLoadPosMtxImm(matrix,0);GXCallDisplayList(repeated,sizeof(repeated));CHECK(near(captured[42].position[0],109));
    fh_gx_reset();GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    GXSetNumTexGens(1);GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_MTX3x4,GX_TG_NRM,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXSetChanMatColor(GX_COLOR0A0,(GXColor){128,255,64,200});GXSetChanAmbColor(GX_COLOR0A0,(GXColor){64,32,16,128});
    GXLightObj lamp;GXInitLightPos(&lamp,0,0,10);GXInitLightColor(&lamp,(GXColor){255,128,64,128});GXLoadLightObjImm(&lamp,GX_LIGHT0);
    for(unsigned mode=0;mode<3;mode++){
        GXSetChanCtrl(GX_COLOR0A0,mode!=0,GX_SRC_REG,GX_SRC_REG,mode==1?GX_LIGHT0:0,GX_DF_CLAMP,GX_AF_NONE);
        unsigned start=vertex_count;GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
        for(unsigned i=0;i<3;i++){GXPosition3f32(0,0,-2);GXNormal3f32(0,0,2);}GXEnd();
        const FhVertex *v=&captured[start];CHECK(near(v->uv[0][2],2));
        float expected_red=128/255.f*(mode==2?64/255.f:1);
        float expected_alpha=200/255.f*(mode==2?128/255.f:1);
        CHECK(near(v->color[0][0],expected_red) && near(v->color[0][3],expected_alpha));
    }
    gpu_transform_available=1;GXLoadPosMtxImm(matrix,0);
    unsigned start=vertex_count;GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
    for(unsigned i=0;i<3;i++){GXPosition3f32(0,0,-2);GXNormal3f32(0,0,2);}GXEnd();
    CHECK(fh_gx.draw.raw_position && near(captured[start].position[0],0) && near(fh_gx.draw.model_palette[0][3],10));
    GXSetChanCtrl(GX_COLOR0A0,GX_TRUE,GX_SRC_REG,GX_SRC_REG,GX_LIGHT0,GX_DF_CLAMP,GX_AF_NONE);
    start=vertex_count;GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
    for(unsigned i=0;i<3;i++){GXPosition3f32(0,0,-2);GXNormal3f32(0,0,2);}GXEnd();
    CHECK(fh_gx.draw.raw_position && near(captured[start].position[0],0));
    vertex_count=0;fh_gx_reset();GXClearVtxDesc();GXSetVtxDesc(GX_VA_PNMTXIDX,GX_DIRECT);GXSetVtxDesc(GX_VA_POS,GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_S16,0);
    s16 mixed_positions[]={0,0,-2,1,0,-2,2,0,-2};GXSetArray(GX_VA_POS,mixed_positions,sizeof(mixed_positions),6,true);
    matrix[0][3]=10;GXLoadPosMtxImm(matrix,0);matrix[0][3]=20;GXLoadPosMtxImm(matrix,3);matrix[0][3]=30;GXLoadPosMtxImm(matrix,30);
    const u8 mixed_indices[]={0x90,0,3,0,0,3,1,30,2};GXCallDisplayList(mixed_indices,sizeof(mixed_indices));
    CHECK(vertex_count==3 && fh_gx.draw.raw_position);
    CHECK(captured[0].matrix_index==0 && captured[1].matrix_index==3 && captured[2].matrix_index==-1);
    CHECK(near(captured[0].position[0],0) && near(captured[1].position[0],1) && near(captured[2].position[0],32));
    CHECK(near(fh_gx.draw.model_palette[0][3],10) && near(fh_gx.draw.model_palette[3][3],20));
    /* Fixed wire-format oracle, including signed limits, signed zero and an
     * unaligned destination. Recording does not interpret descriptor state. */
    u8 wire[34];memset(wire,0xa5,sizeof(wire));
    GXBeginDisplayList(wire+1,32);GXBegin(GX_TRIANGLES,GX_VTXFMT0,1);
    GXPosition3f32(-0.0f,-2.5f,.0625f);GXPosition3s16(-32768,32767,-1);
    GXNormal3s8(-128,127,-1);GXColor4u8(1,2,128,255);GXTexCoord2s16(-256,256);
    const u8 expected_wire[]={0x90,0,1,0x80,0,0,0,0xc0,0x20,0,0,0x3d,0x80,0,0,
        0x80,0,0x7f,0xff,0xff,0xff,0x80,0x7f,0xff,1,2,0x80,0xff,0xff,0,1,0};
    CHECK(GXEndDisplayList()==sizeof(expected_wire));
    CHECK(!memcmp(wire+1,expected_wire,sizeof(expected_wire)) && wire[0]==0xa5 && wire[33]==0xa5);
    lighting_regression();lighting_cache_regression();two_channel_cache_regression();flat_lighting_regression();raster_dependency_regression();lit_projection_regression();deferred_lighting_regression();palette_lighting_regression();emboss_regression();batching_regression();component_regression();texgen_position_matrix_regression();texgen_reuse_regression();texgen_cache_regression();geometry_cache_regression();retained_geometry_regression();retained_texgens_regression();early_visibility_regression();gpu_visibility_regression();point_submission_regression();test_pass();
}
