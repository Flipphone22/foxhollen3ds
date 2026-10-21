#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_native_scene.h"
#include "fh_scene_snapshot.h"
typedef uint64_t u64;
#define FH_NATIVE_LINEAR_RESERVE 1024
#include "model_view_types.inc"
static unsigned fh_minimal_enabled=1,model_views_mode=1,model_view_gpu_bytes,model_view_peak;
static unsigned model_view_allocations,model_view_hits,model_view_fallbacks;
static unsigned model_bounds_bytes,model_bounds_peak,model_bounds_enabled=1;
unsigned fh_native_scene_cpu_bytes,fh_native_scene_gpu_bytes,fh_native_scene_admission[16];
static u64 scene_uploaded_bytes,scene_source_ticks,scene_source_bytes;
static unsigned frame=1,drains,flushes,allow_memory=1,allocation_fail;
static struct {const void *gpu;unsigned char bytes[128];} queued[64];
static unsigned queued_count;
static void verify_readers(void){
    for(unsigned i=0;i<queued_count;i++)assert(!memcmp(queued[i].gpu,queued[i].bytes,128));
}
static unsigned normal_reserve(unsigned n){return allow_memory && n<1024*1024;}
static unsigned linearSpaceFree(void){return allow_memory?16*1024*1024:0;}
static unsigned fh_minimal_linear_room(unsigned n){return allow_memory && n<1024*1024;}
static void *linearMemAlign(unsigned n,unsigned alignment){(void)alignment;return allocation_fail?NULL:malloc(n);}
static void linearFree(void *p){free(p);}
static void fh_gpu_cache_flush(const void *p,unsigned n){(void)p;(void)n;flushes++;}
static void GSPGPU_FlushDataCache(const void *p,unsigned n){fh_gpu_cache_flush(p,n);}
static u64 svcGetSystemTick(void){return 0;}
static int fh_model_resource_match(unsigned *version,const void *p,const void *s,size_t n){*version=0;return !memcmp(p,s,n);}
static unsigned fh_renderer_scene_upload_begin(unsigned used){
    verify_readers();if(used==frame){drains++;queued_count=0;}return frame;
}
static unsigned fh_renderer_scene_frame(void){return frame;}
static FhModelRef ref={0};
int fh_native_scene_group(const FhSceneAsset *a,unsigned i,FhSceneGroup *g){
    assert(!i);*g=(FhSceneGroup){.vertex=(const FhModelVertex*)a->gpu,.vertices=1,.refs=&ref,.format=3,.bones=2,.bone_ids=(const uint16_t*)(a->meta+48)};return 1;
}
unsigned fh_native_scene_compact_model(FhSceneAsset *a,const FhModelSource *s){(void)a;(void)s;abort();}
#include "platform_scene_model_views.h"
#include "model_view_functions.inc"
static unsigned char positions[12][9],normals[12][6],colors[4]={0x12,0x34},uvs[6]={0,1,0,2};
static FhModelSource source(unsigned i){
    /* Different low address bits exercise independent snapshot alignment. */
    return (FhModelSource){.positions=positions[i]+i%4,.normals=normals[i]+i%4,
        .colors=colors,.uvs=uvs,.position_count=1,.normal_count=1,.normal_stride=3,.color_count=1,.uv_count=1};
}
static void draw(SceneRecord *r,unsigned i){
    FhModelSource s=source(i);assert(fh_native_scene_prepare_model(&r->asset,&s));verify_readers();
    const FhModelCompactVertex *v=(const void*)r->asset.gpu;
    assert(v->position[0]==(int16_t)((s.positions[0]<<8)|s.positions[1]));
    assert(v->normal[0]==(int8_t)s.normals[0]);
    assert(v->color[0]==(s.colors[0]>>4) && v->uv[0]==(int16_t)((s.uvs[0]<<8)|s.uvs[1]));
    assert(v->skin[0]==3 && v->skin[2]==4 && v->tail[0]==37);
    const FhAnimatedBounds *bounds;FhSceneGroup group;
    fh_native_scene_group(&r->asset,0,&group);bounds=fh_native_scene_model_bounds(&r->asset,&group);
    if(bounds){
        float expected=v->position[0]/256.f+v->tail[0];
        assert(bounds->used==3 && bounds->bone[1].lo[0]<=expected && bounds->bone[1].hi[0]>=expected);
        assert(bounds->bone[1].hi[0]-bounds->bone[1].lo[0]<.02f);
    }
    assert(queued_count<64);queued[queued_count].gpu=r->asset.gpu;
    memcpy(queued[queued_count++].bytes,r->asset.gpu,128);fh_native_scene_used(&r->asset);
}
int main(void){
    SceneRecord *r=records;unsigned char gpu[128]={0},snapshot[64]={0},meta[64]={0};
    SceneSpan spans[4];SceneVersion versions[4]={0};unsigned cursor=0;
    for(unsigned i=0;i<12;i++){
        FhModelSource s=source(i);((unsigned char*)s.positions)[1]=i+1;((unsigned char*)s.normals)[0]=i+3;
    }
    FhModelSource a=source(0);const void *data[4]={a.positions,a.normals,a.colors,a.uvs};unsigned sizes[4]={6,3,2,4};
    for(unsigned i=0;i<4;i++){spans[i]=(SceneSpan){cursor,sizes[i]};memcpy(snapshot+cursor,data[i],sizes[i]);versions[i].data=data[i];cursor+=fh_scene_snapshot_space(sizes[i]);}
    FhModelCompactVertex *v=(void*)gpu;v->skin[0]=3;v->skin[2]=4;v->tail[0]=37;v->position[3]=1;
    a.format=3;assert(fh_native_model_refresh_compact(v,1,&ref,1,&a,15));
    *r=(SceneRecord){.owner=r,.gpu=gpu,.snapshots=snapshot,.spans=spans,.versions=versions,.span_count=4,.gpu_used_bytes=128,
        .asset={.gpu=gpu,.meta=meta,.groups=1,.gpu_bytes=128,.compact=1,.direct=1}};
    fh_native_scene_gpu_bytes=128;
    draw(r,0);draw(r,1);draw(r,2);draw(r,0);draw(r,1);
    assert(!drains && r->view_count==2 && model_view_hits==2);verify_readers();
    /* A real animation write must still drain the selected pending backing. */
    positions[1][2]++;draw(r,1);assert(drains==1);
    for(unsigned mask=1;mask<16;mask++){
        FhModelSource s=source(1);
        if(mask&1)((unsigned char*)s.positions)[1]++;
        if(mask&2)((unsigned char*)s.normals)[0]++;
        if(mask&4)colors[0]++;
        if(mask&8)uvs[1]++;
        draw(r,1);
    }
    /* Distinct pointers with identical bytes still need first publication. */
    a=source(1);FhModelSource b=source(3);memcpy((void*)b.positions,a.positions,6);memcpy((void*)b.normals,a.normals,3);
    unsigned old_flushes=flushes;draw(r,3);assert(flushes==old_flushes+1);
    allow_memory=0;unsigned old_drains=drains;draw(r,4);assert(drains==old_drains+1);allow_memory=1;
    allocation_fail=1;old_drains=drains;draw(r,5);assert(drains==old_drains+1);allocation_fail=0;
    unsigned actual_bytes=model_view_gpu_bytes;model_view_gpu_bytes=MODEL_VIEW_GPU_LIMIT;
    unsigned old_alloc=model_view_allocations;
    old_drains=drains;draw(r,6);assert(drains==old_drains+1 && model_view_allocations==old_alloc);
    model_view_gpu_bytes=actual_bytes;
    for(unsigned i=6;i<12;i++)draw(r,i);assert(r->view_count==7 && model_view_fallbacks>=4);
    /* Stale pointer identity never suppresses source-byte comparison. */
    positions[0][1]++;draw(r,0);assert(((FhModelCompactVertex*)r->asset.gpu)->position[0]==positions[0][1]);
    FhModelSource bad=source(0);bad.position_count=2;old_alloc=model_view_allocations;
    assert(!fh_native_scene_prepare_model(&r->asset,&bad) && model_view_allocations==old_alloc);
    verify_readers();queued_count=0;model_views_release(r);
    assert(!model_bounds_bytes && !model_view_gpu_bytes && !fh_native_scene_cpu_bytes && fh_native_scene_gpu_bytes==128 && r->asset.gpu==gpu);
    printf("PASS: isolated queued views, exact mutation/alias checks, all15 attribute masks, allocation/room/cap fallback, publication and release; drains=%u\n",drains);
}
