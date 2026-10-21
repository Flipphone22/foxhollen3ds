#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <dolphin/vi.h>
#include "../native-3ds/source/gx_dispatch.c"
#include "state-batch-stubs.h"

static pthread_t reader;
static unsigned stopped,draws,mode,invalid_array,missing_dirty,mismatched_state,immutable_write,defer_reader;
static unsigned release_on_wait,wait_calls;
static unsigned wakes;
unsigned fh_retained_scope_validate=1;
unsigned fh_render_floor,fh_render_floor_setters;
unsigned fh_minimal_enabled;
void fh_minimal_projection(const float matrix[4][4]){(void)matrix;}
static unsigned char *source;
static FhGx expected[256];
static const void *reader_ranges[4];static unsigned reader_sizes[4],reader_active;
void fh_retained_reader_scope_begin(const void *const addresses[4],const unsigned bytes[4]){
    assert(!reader_active);reader_active=1;
    memcpy(reader_ranges,addresses,sizeof(reader_ranges));memcpy(reader_sizes,bytes,sizeof(reader_sizes));
}
void fh_retained_reader_scope_end(void){assert(reader_active);reader_active=0;}
void fh_platform_panic(const char *message){fprintf(stderr,"panic: %s\n",message);abort();}
void fh_gx_bounds_reset(void){}
void fh_geometry_reset(void){}
void fh_retained_reset(void){}
void fh_compiled_reset(void){}
void fh_renderer_dispatch_begin(void){assert(!fh_gxd_pending());}
void fh_platform_frame(void){assert(!fh_gxd_pending() && !deferred_end);}
void fh_dvd_pump(void){assert(!fh_gxd_pending() && !deferred_end);}
void fh_audio_pump(void){assert(!fh_gxd_pending() && !deferred_end);}
void fh_gx_frame_complete(void){assert(!fh_gxd_pending() && !deferred_end);}
void fh_renderer_black(int black){assert(black && !fh_gxd_pending() && !deferred_end);}
void *fh_renderer_cpu_cache_alloc(unsigned bytes){static unsigned char arena[256*1024] __attribute__((aligned(32)));assert(bytes==sizeof(arena));return arena;}
unsigned fh_renderer_source_exclusions(const void **addresses,size_t *bytes,unsigned capacity){(void)addresses;(void)bytes;(void)capacity;return 0;}
void fh_retained_scope_invalidate(const void *p,size_t n){(void)p;(void)n;}
void fh_renderer_invalidate(const void *p,size_t n){(void)p;(void)n;}
void fh_renderer_texture_hint(void) {}
size_t fh_texture_source_bytes(unsigned format,unsigned width,unsigned height){assert(format==GX_TF_C4 && width==8 && height==8);return 32;}
int fh_retained_scope_contains(const void *p,unsigned n){
    if(!reader_active)return 0;
    for(unsigned i=0;i<4;i++){
        uintptr_t a=(uintptr_t)p,b=(uintptr_t)reader_ranges[i];unsigned size=reader_sizes[i];
        if(b && a>=b && a-b<=size && n<=size-(a-b))return 1;
    }
    return 0;
}
int fh_retained_producer_scope_contains(const void *p,unsigned n){
    return (uintptr_t)p>=(uintptr_t)source && (uintptr_t)p-(uintptr_t)source<=256 && n<=256-((uintptr_t)p-(uintptr_t)source);
}
void fh_gxd_GXCallDisplayList(const void *p,u32 n){
    assert(n==64 && draws<256);
    if(mode==6 || mode==7){
        assert((p!=source+512)==(mode==6));
        assert((fh_gx.arrays[GX_VA_POS].data!=source)==(mode==6));
        for(unsigned i=0;i<64;i++)assert(((const unsigned char*)p)[i]==0x17);
        for(unsigned i=0;i<256;i++)assert(fh_gx.arrays[GX_VA_POS].data[i]==0x29);
        ++draws;return;
    }
    if(mode==5){
        assert(draws<2 && p==source+512 && fh_gx.arrays[GX_VA_POS].data==source+draws*256);
        assert(reader_ranges[0]==source+draws*256 && reader_sizes[0]==256);
        ++draws;return;
    }
    if(mode==4){
        assert(draws<2 && p!=source+512 && !memcmp(p,source+512,n));
        const unsigned char *pixels=fh_gx.draw.textures[0].pixels;
        assert(pixels!=source+2048 && pixels[0]==(draws?0x34:0x12));
        ++draws;return;
    }
    FhGx observed=fh_gx;
    if(p!=source+512){
        assert(mode==3 && !memcmp(p,source+512,n));
        for(unsigned i=GX_VA_POS;i<=GX_VA_TEX7;i++)if(observed.descriptors[i]>=GX_INDEX8){
            assert(!memcmp(observed.arrays[i].data,expected[draws].arrays[i].data,observed.arrays[i].bytes));
            observed.arrays[i].data=expected[draws].arrays[i].data;
        }
        for(unsigned i=0;i<8;i++)if(observed.draw.textures[i].pixels){
            FhTexture *t=&observed.draw.textures[i];const FhTexture *e=&expected[draws].draw.textures[i];
            assert(!memcmp(t->pixels,e->pixels,fh_texture_source_bytes(t->format,t->width,t->height)));
            if(t->palette && t->palette_count)assert(!memcmp(t->palette,e->palette,t->palette_count*2));
            t->pixels=e->pixels;t->palette=e->palette;
        }
    }
    if(!mode)expected[draws]=fh_gx;
    else if(memcmp(expected+draws,&observed,sizeof(observed))){
        unsigned char *a=(void*)(expected+draws),*b=(void*)&observed;
        for(unsigned i=0;i<sizeof(fh_gx);i++)if(a[i]!=b[i]){fprintf(stderr,"draw %u byte %u expected %u got %u\n",draws,i,a[i],b[i]);break;}
        abort();
    }
    /* Consumer-derived fields are not persistent GX register writes. A later
     * patch must preserve these until the next draw recalculates them. */
    fh_gx.draw.raw_position=draws+1;fh_gx.draw.model_palette[0][0]=(float)draws;
    ++draws;
}
static void *consume(void *unused){
    (void)unused;
    while(!__atomic_load_n(&stopped,__ATOMIC_ACQUIRE)){
        if(__atomic_load_n(&defer_reader,__ATOMIC_ACQUIRE)){sched_yield();continue;}
        if(!fh_gxd_consume_batch(64))sched_yield();
    }
    return NULL;
}
int fh_gxd_platform_start(void){assert(!pthread_create(&reader,NULL,consume,NULL));return 1;}
void fh_gxd_platform_wake(void){++wakes;}
void fh_gxd_platform_wait(void){
    ++wait_calls;
    if(release_on_wait)__atomic_store_n(&defer_reader,0,__ATOMIC_RELEASE);
    while(fh_gxd_pending())sched_yield();
}
static void scenario(unsigned selected){
    mode=selected;draws=0;fh_gx_state_batch_mode=selected==3?1:selected;
    fh_gx_owned_sources_mode=selected==3;fh_gx_reset();
    for(unsigned scope=0;scope<8;scope++){
        const void *ranges[4]={source};unsigned sizes[4]={256};
        assert(fh_gx_dispatch_begin(ranges,sizes));fh_gx_dispatch_lease_list(source+512,64);
        for(unsigned i=0;i<32;i++){
            unsigned step=scope*32+i;
            GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX16);
            GXSetArray(GX_VA_POS,source+(invalid_array?256:0),256,6,false);
            GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_S16,step%16);
            GXSetNumTexGens(2);GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_NRM,30,GX_FALSE,GX_PTIDENTITY);
            GXTlutObj palette;GXInitTlutObj(&palette,source+1024,GX_TL_RGB565,16);GXLoadTlut(&palette,3);
            GXTexObj texture;GXInitTexObjCI(&texture,source+2048,8,8,GX_TF_C4,GX_REPEAT,GX_CLAMP,GX_FALSE,3);
            /* Keep the fixture's serial independent of the three scenario
             * initializations. Real pixels and descriptors retain ownership. */
            ((FhTexture*)(void*)&texture)->serial=step+1;
            GXLoadTexObj(&texture,GX_TEXMAP0);memset(&texture,0xa5,sizeof(texture));memset(&palette,0xa5,sizeof(palette));
            GXSetViewport(0,0,640,480,0,1);GXSetScissor(step,2,620,470);
            GXSetNumTevStages(2);GXSetTevOp(GX_TEVSTAGE0,GX_MODULATE);
            GXSetTevOp(GX_TEVSTAGE1,GX_PASSCLR);
            for(unsigned repeat=0;repeat<12;repeat++){
                GXSetTevColor(GX_TEVREG0,(GXColor){step,repeat,99,255});
                GXSetTevColorIn(GX_TEVSTAGE0,GX_CC_ZERO,GX_CC_TEXC,GX_CC_RASC,GX_CC_ZERO);
                GXSetTevKColorSel(GX_TEVSTAGE0,GX_TEV_KCSEL_K0);
            }
            GXSetChanCtrl(GX_COLOR0A0,GX_TRUE,GX_SRC_REG,GX_SRC_VTX,3,GX_DF_CLAMP,GX_AF_SPEC);
            GXSetChanAmbColor(GX_COLOR0A0,(GXColor){13,step,29,127});
            GXLightObj light;memset(&light,0,sizeof(light));GXInitLightPos(&light,1,2,step);
            GXInitLightColor(&light,(GXColor){255,129,7,255});GXLoadLightObjImm(&light,GX_LIGHT0);
            float matrix[3][4]={{1,0,0,(float)step},{0,1,0,2},{0,0,1,3}};
            GXLoadTexMtxImm(matrix,GX_PTTEXMTX0,GX_MTX3x4);
            matrix[0][0]=-0.f;GXLoadTexMtxImm(matrix,GX_PTTEXMTX0,GX_MTX2x4);
            uint32_t *block=fh_gx_matrix_block_begin(2,3);assert(block);
            for(unsigned j=0;j<2;j++){
                uint32_t *row=fh_gx_matrix_block_row(block,j);
                fh_gx_matrix_block_position(row,matrix,j*3,30+j*3);
                fh_gx_matrix_block_normal(row,matrix);
            }
            fh_gx_matrix_block_end(block);memset(matrix,0xa5,sizeof(matrix));
            /* Getters fence previous writes. Following writes must snapshot
             * the completed state, not a stale producer copy. */
            if(i%7==0){u32 x,y,w,h;GXGetScissor(&x,&y,&w,&h);assert(x==step && y==2 && w==620 && h==470);}
            GXSetZMode(GX_TRUE,GX_LEQUAL,GX_FALSE);
            GXInvalidateVtxCache();GXSetCurrentMtx((step%10)*3);
            float projection[4][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
            projection[0][0]=(float)step;GXSetProjection(projection,GX_ORTHOGRAPHIC);
            GXLoadPosMtxImm(projection,6);GXLoadNrmMtxImm(projection,6);
            extern void fhLoadIdentityPosMtx(void),fhGXSetCullMode(GXCullMode);
            fhLoadIdentityPosMtx();fhGXSetCullMode((GXCullMode)(step%4));
            GXSetViewportJitter(1,2,640,480,0,1,step&1);
            GXSetColorUpdate(step&1);GXSetAlphaUpdate((step>>1)&1);
            GXSetAlphaCompare(GX_GEQUAL,step,GX_AOP_AND,GX_LESS,255-step);
            GXSetPixelFmt(GX_PF_RGB8_Z24,GX_ZC_LINEAR);GXSetZCompLoc(step&1);GXSetDither(step&1);
            GXSetFieldMode(GX_FALSE,GX_FALSE);
            GXSetFog(GX_FOG_LIN,step,step+100,1,1000,(GXColor){step,33,42,255});
            GXSetLineWidth(step,(GXTexOffset)(step%6));GXSetPointSize(step+1,(GXTexOffset)(step%6));
            GXEnableTexOffsets((GXTexCoordID)(step%8),step&1,(step>>1)&1);
            GXSetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_TEXA,GX_CA_RASA,GX_CA_ZERO);
            GXSetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
            GXSetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_SUB,GX_TB_ZERO,GX_CS_SCALE_2,GX_TRUE,GX_TEVREG1);
            GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);
            GXSetTevColorS10(GX_TEVREG1,(GXColorS10){-12,step,32,255});
            GXSetTevKColor(GX_KCOLOR0,(GXColor){step,17,93,127});
            GXSetTevKAlphaSel(GX_TEVSTAGE0,GX_TEV_KASEL_K0_A);
            GXSetTevSwapMode(GX_TEVSTAGE0,GX_TEV_SWAP1,GX_TEV_SWAP0);
            GXSetTevSwapModeTable(GX_TEV_SWAP1,GX_CH_RED,GX_CH_BLUE,GX_CH_GREEN,GX_CH_ALPHA);
            GXSetTevDirect(GX_TEVSTAGE1);GXSetNumIndStages(2);
            GXSetIndTexOrder(GX_INDTEXSTAGE0,GX_TEXCOORD0,GX_TEXMAP0);
            GXSetIndTexCoordScale(GX_INDTEXSTAGE0,(GXIndTexScale)(step%9),(GXIndTexScale)((step+1)%9));
            GXSetIndTexMtx(GX_ITM_0,projection,(s8)(step%17-8));
            GXSetTevIndirect(GX_TEVSTAGE0,GX_INDTEXSTAGE0,GX_ITF_8,GX_ITB_STU,GX_ITM_0,GX_ITW_OFF,GX_ITW_OFF,GX_FALSE,GX_FALSE,GX_ITBA_S);
            GXSetTevIndRepeat(GX_TEVSTAGE1);GXSetNumChans(2);
            GXSetChanMatColor(GX_COLOR0A0,(GXColor){255,step,32,127});
            if(mode==2 && missing_dirty)fh_gxp_state.post[63][0][0]=5.f;
            if(mode==2 && mismatched_state)fh_gxp_state.draw.depth_test^=1;
            if(mode==3 && immutable_write && i==1)source[0]^=1;
            GXCallDisplayList(source+512,64);
        }
        fh_gx_dispatch_end();assert(draws==(scope+1)*32);
        /* A direct setter between scopes must be visible in the next one. */
        GXSetBlendMode(GX_BM_BLEND,GX_BL_SRCALPHA,GX_BL_INVSRCALPHA,GX_LO_COPY);
    }
    assert(draws==256);
}
static void texture_write_scenario(void){
    mode=4;draws=0;fh_gx_reset();fh_gx_state_batch_mode=1;fh_gx_owned_sources_mode=1;
    __atomic_store_n(&defer_reader,1,__ATOMIC_RELEASE);
    const void *ranges[4]={source};unsigned sizes[4]={256};
    assert(fh_gx_dispatch_begin(ranges,sizes));fh_gx_dispatch_matrix_range(source+2048,32);
    fh_gx_dispatch_lease_list(source+512,64);
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX16);GXSetArray(GX_VA_POS,source,256,6,false);
    GXSetNumTevStages(1);GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);
    GXTlutObj palette;GXInitTlutObj(&palette,source+1024,GX_TL_RGB565,16);GXLoadTlut(&palette,3);
    GXTexObj texture;GXInitTexObjCI(&texture,source+2048,8,8,GX_TF_C4,GX_REPEAT,GX_CLAMP,GX_FALSE,3);
    GXLoadTexObj(&texture,GX_TEXMAP0);source[2048]=0x12;GXCallDisplayList(source+512,64);
    /* An ordered cache-write fast path is normally a matrix notification.
     * This alias case must still replace the texture resource version. Both
     * draws consume only after the original pixels have already changed. */
    source[2048]=0x34;assert(fh_gx_dispatch_cache_write(source+2048,1));
    GXCallDisplayList(source+512,64);
    __atomic_store_n(&defer_reader,0,__ATOMIC_RELEASE);fh_gx_dispatch_end();assert(draws==2);
}
static void delayed_scope_scenario(unsigned missing_begin,unsigned bad_fp){
    /* Exercise two ordered scope records before allowing the reader to run.
     * This tests packet ownership, not permission to remove production joins:
     * real caller source lifetimes still need that separate contract. */
    mode=5;draws=0;fh_gx_reset();fh_gx_state_batch_mode=1;fh_gx_owned_sources_mode=0;
    __atomic_store_n(&defer_reader,1,__ATOMIC_RELEASE);
    const void *ranges[4]={source};unsigned sizes[4]={256};
    assert(fh_gx_dispatch_begin(ranges,sizes));fh_gx_dispatch_lease_list(source+512,64);
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX16);GXSetArray(GX_VA_POS,source,256,6,false);
    GXCallDisplayList(source+512,64);
    (void)fh_gxd_reserve(1);fh_gxd_commit(OP_SCOPE_END);
    ranges[0]=source+256;
    if(!missing_begin)record_scope_begin(ranges,sizes,bad_fp?1:0x01000000u);
    GXSetArray(GX_VA_POS,source+256,256,6,false);GXCallDisplayList(source+512,64);
    /* Neither marker may retain these caller-owned descriptors. */
    ranges[0]=source+3072;sizes[0]=1;
    __atomic_store_n(&defer_reader,0,__ATOMIC_RELEASE);fh_gx_dispatch_end();
    assert(draws==2 && !reader_active && !reader_scope_open);
}
static void owned_async_scenario(unsigned action,unsigned force_borrow,unsigned missing_join){
    mode=force_borrow?7:6;draws=0;fh_gx_reset();fh_gx_state_batch_mode=1;fh_gx_owned_sources_mode=2;
    memset(source,0x29,256);memset(source+512,0x17,64);
    __atomic_store_n(&defer_reader,1,__ATOMIC_RELEASE);release_on_wait=1;
    const void *ranges[4]={source};unsigned sizes[4]={256};
    assert(fh_gx_dispatch_begin(ranges,sizes));fh_gx_dispatch_lease_list(source+512,64);
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_INDEX16);GXSetArray(GX_VA_POS,source,256,6,false);
    GXSetNumTevStages(1);GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR_NULL);
    GXSetNumIndStages(0);GXSetNumTexGens(0);
    if(force_borrow)fh_gx_owned_sources_mode=0;
    GXCallDisplayList(source+512,64);fh_gx_owned_sources_mode=2;
    unsigned waits_before=wait_calls,deferrals_before=fh_gx_owned_deferred_scopes;
    fh_gx_dispatch_end();
    if(force_borrow){
        assert(draws==1 && wait_calls==waits_before+1 && !deferred_end);
        assert(fh_gx_owned_deferred_scopes==deferrals_before);
    }else{
        assert(!draws && wait_calls==waits_before && deferred_end && fh_gxd_pending());
        assert(fh_gx_owned_deferred_scopes==deferrals_before+1 && !owned_model.count && !owned_textures.count);
        /* All original list/array bytes die before the reader is released. */
        memset(source,0xa7,256);memset(source+512,0xb3,64);
        if(action==0)fh_gx_dispatch_barrier();
        else if(action==1){
            if(missing_join)deferred_end=0; /* Negative control for the new join gate. */
            assert(fh_gx_dispatch_begin(ranges,sizes));fh_gx_dispatch_end();
        }else if(action==2)GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_COPY);
        else if(action==3)VIWaitForRetrace();
        else VISetBlack(GX_TRUE);
        assert(draws==1 && wait_calls>waits_before && !deferred_end && !fh_gxd_pending());
    }
    release_on_wait=0;
    for(unsigned i=0;i<FH_OWNED_SOURCE_SLOTS;i++)assert(!owned_pool.entry[i].pins);
}
int main(int argc,char **argv){
    uint32_t original[4]={1,2,3,4},state[4]={1,2,3,4};
    const uint32_t corrupt[]={1u<<16,99,5u|(1u<<16),77};
    assert(!fh_state_delta_apply(state,4,corrupt,4,0) && !memcmp(state,original,sizeof(state)));
    const uint32_t mismatched[]={1u|(1u<<16),99};
    assert(!fh_state_delta_apply(state,4,mismatched,2,1) && !memcmp(state,original,sizeof(state)));
    invalid_array=argc>1 && !strcmp(argv[1],"invalid-array");
    missing_dirty=argc>1 && !strcmp(argv[1],"missing-dirty");
    mismatched_state=argc>1 && !strcmp(argv[1],"mismatched-state");
    immutable_write=argc>1 && !strcmp(argv[1],"immutable-write");
    source=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(source!=MAP_FAILED && (uintptr_t)source<UINT32_MAX-4096);
    fh_gx_dispatch_enabled=2;fh_gx_dispatch_batch_enabled=1;fh_gx_matrix_block_enabled=1;
    if(invalid_array)scenario(1);
    scenario(0);scenario(2);scenario(1);
    assert(!owned_pool.storage && !fh_gxd_pending() && !fh_gxd_recording);
    unsigned idle_wakes=wakes;
    for(unsigned i=0;i<8000;i++)fh_gx_dispatch_barrier();
    assert(wakes==idle_wakes);
    scenario(3);
    texture_write_scenario();
    delayed_scope_scenario(argc>1 && !strcmp(argv[1],"missing-scope-begin"),argc>1 && !strcmp(argv[1],"bad-scope-fp"));
    for(unsigned action=0;action<5;action++)owned_async_scenario(action,0,argc>1 && !strcmp(argv[1],"missing-deferred-join"));
    owned_async_scenario(0,1,0);
    assert(fh_gx_owned_draws>0);
    assert(owned_pool.shares>0 && !owned_model.count && !owned_textures.count);
    for(unsigned i=0;i<FH_OWNED_SOURCE_SLOTS;i++)assert(!owned_pool.entry[i].pins);
    assert(fh_gx_state_batch_checks && fh_gx_state_batch_packets && fh_gx_state_batch_setters);
    __atomic_store_n(&stopped,1,__ATOMIC_RELEASE);assert(!pthread_join(reader,NULL));
    printf("1024 complete draw states match; 2 ordered texture-version draws; 2 delayed scope draws; 5 asynchronous owned exits and borrowed join; %u checked patches; %u setters; %zu producer bytes\n",
        fh_gx_state_batch_checks,fh_gx_state_batch_setters,sizeof(fh_gxp_state)+sizeof(state_previous)+sizeof(state_patch));
    munmap(source,4096);return 0;
}

