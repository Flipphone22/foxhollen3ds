#include "fh_graphics_options.h"
#include "fh_touch_cstick.h"
#include "fh_replay.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#undef assert
#define assert(c) do{if(!(c)){fprintf(stderr,"check failed line %d: %s\n",__LINE__,#c);exit(3);}}while(0)
#endif
/* Same translation unit permits checking numeric FPS and repaint generations,
 * rather than accepting a merely nonblank LCD result. */
#include "../native-3ds/source/platform_graphics_options.c"
#include "../native-3ds/source/graphics_presets.c"
static unsigned flushes;
FhPerfDisplay fh_perf_display;
/* Existing layout checks exercise the New recording area. Old CPU/Items
 * bounds are covered by test_cpu_panel_lcd.py. */
unsigned fh_hardware_is_new=1,fh_hardware_large_memory;
static char recipe_path[1024];
void fh_platform_panic(const char*s){fprintf(stderr,"%s\n",s);abort();}
void fh_platform_log(const char*s){fputs(s,stdout);}
void fh_gpu_cache_flush(const void*p,unsigned bytes){assert(p && (bytes==144*240*2 || bytes==136*240*2));flushes++;}
static void tap(int x,int y,unsigned locked){fh_graphics_options_touch(0,0,0,locked);fh_graphics_options_touch(1,x,y,locked);}
static int recipe(const char *body,unsigned expected){
    FILE*f=fopen(recipe_path,"w+b");assert(f);fputs("FHREPLAY 1\nSAVE 0 0\nLIMIT 60\n",f);fputs(body,f);rewind(f);
    FhReplayPlan plan;char error[128];int ok=fh_replay_parse(f,&plan,error,sizeof(error));fclose(f);
    if(ok)assert(plan.graphics_options==expected);return ok;
}
int main(int argc,char**argv){
    assert(argc==2);snprintf(recipe_path,sizeof(recipe_path),"%s.scenario",argv[1]);
    assert(fh_graphics_options==FH_GRAPHICS_DEFAULT);
    /* Exercise all existing all-on recipes without changing their meaning. */
    fh_graphics_options_set(FH_GRAPHICS_ALL);
    uint16_t *a=malloc(320*240*2),*b=malloc(320*240*2);assert(a&&b);
    for(unsigned i=0;i<320*240;i++)a[i]=b[i]=0x1234;
    assert(fh_graphics_options_draw(a,1));assert(fh_graphics_options_draw(b,1));
    assert(!fh_graphics_options_draw(a,0));unsigned old_flush=flushes;
    for(unsigned i=0;i<10000;i++)assert(!fh_graphics_options_draw(i&1?a:b,0));assert(flushes==old_flush);
    tap(32,30,0); /* Open. */
    tap(32,65,0);assert(fh_graphics_options==0);
    for(unsigned i=0;i<100;i++)fh_graphics_options_touch(1,32,65,0);assert(fh_graphics_options==0);
    tap(32,65,1);assert(fh_graphics_options==0); /* Recording/replay lock. */
    tap(240,152,0);fh_graphics_options_touch(1,32,65,0);assert(fh_graphics_options==0); /* Drag-in is not a tap. */
    tap(32,65,0);assert(fh_graphics_options==FH_GRAPHICS_ALL);
    assert(fh_graphics_options_draw(a,0));assert(fh_graphics_options_draw(b,0));
    /* Graphics never writes Items or the recording button, including clearing. */
    for(unsigned x=0;x<320;x++)for(unsigned y=0;y<240;y++)
        if(!((x>=16&&x<160&&y>=16&&y<168)||(x>=176&&x<312&&((y>=16&&y<48)||(y>=52&&y<104)))))assert(a[x*240+239-y]==0x1234);
    tap(32,93,0);assert(fh_graphics_options==(FH_GRAPHICS_ALL|FH_GRAPHICS_NO_LIGHTING));
    tap(32,93,0);tap(32,121,0);assert(fh_graphics_options==(FH_GRAPHICS_ALL|FH_GRAPHICS_NO_ENVIRONMENT));
    tap(32,121,0);assert(fh_graphics_options==FH_GRAPHICS_ALL);
    tap(120,153,0); /* Second page: FPS is independent of replay/record lock. */
    assert(!fh_graphics_fps_visible);tap(32,65,1);assert(fh_graphics_fps_visible);
    assert(fh_graphics_options_draw(a,0));assert(fh_graphics_options_draw(b,0));
    fh_graphics_options_frame(1000,60000);
    for(unsigned i=1;i<=30;i++)fh_graphics_options_frame(1000+i*1000,60000);
    assert(fps_tenths==600);
    assert(fh_graphics_options_draw(a,0));assert(fh_graphics_options_draw(b,0));
    assert(!fh_graphics_options_draw(a,0));old_flush=flushes;
    for(unsigned i=31;i<=60;i++){fh_graphics_options_frame(1000+i*1000,60000);assert(!fh_graphics_options_draw(i&1?a:b,0));}
    assert(flushes==old_flush); /* Unchanged 60 FPS value does not repaint. */
    for(unsigned i=1;i<=15;i++)fh_graphics_options_frame(61000+i*2000,60000);
    assert(fps_tenths==300);
    assert(fh_graphics_options_draw(a,0));assert(fh_graphics_options_draw(b,0));
    tap(32,65,1);assert(!fh_graphics_fps_visible);
    assert(fh_graphics_options_draw(a,0));assert(fh_graphics_options_draw(b,0));old_flush=flushes;
    for(unsigned i=0;i<10000;i++){fh_graphics_options_frame(200000+i*1000,60000);assert(!fh_graphics_options_draw(i&1?a:b,0));}
    assert(flushes==old_flush); /* Off means no periodic LCD traffic. */
    tap(32,65,0); /* Save a readable, enabled 30 FPS capture. */
    fh_graphics_options_frame(1000,60000);
    for(unsigned i=1;i<=15;i++)fh_graphics_options_frame(1000+i*2000,60000);
    assert(fps_tenths==300);assert(fh_graphics_options_draw(a,0));
    FhTouchCstick stick={0};FhPadState pad={0};fh_touch_cstick_update(&stick,&pad,1,32,65);assert(!pad.cx&&!pad.cy);
    fh_touch_cstick_update(&stick,&pad,0,0,0);fh_touch_cstick_update(&stick,&pad,1,288,152);assert(pad.cx==127);
    assert(recipe("PLAY_RECORDING\nEND\n",FH_GRAPHICS_ALL));
    assert(recipe("GRAPHICS 0\nPLAY_RECORDING\nEND\n",0));
    assert(recipe("GRAPHICS 1\nPLAY_RECORDING\nEND\n",1));
    assert(recipe("GRAPHICS 7\nPLAY_RECORDING\nEND\n",7));
    assert(recipe("GRAPHICS 16383\nPLAY_RECORDING\nEND\n",16383));
    assert(recipe("GRAPHICS 16384\nPLAY_RECORDING\nEND\n",16384));
    assert(recipe("GRAPHICS 32768\nPLAY_RECORDING\nEND\n",32768));
    assert(recipe("GRAPHICS 65536\nPLAY_RECORDING\nEND\n",65536));
    assert(recipe("GRAPHICS 131072\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_SPLASHES));
    assert(recipe("GRAPHICS 262144\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_NO_MORPHS));
    assert(recipe("GRAPHICS 524288\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_WATER_DROPS));
    assert(recipe("GRAPHICS 1048576\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_FUR));
    assert(recipe("GRAPHICS 2097152\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_SPECULAR));
    assert(recipe("GRAPHICS 4194304\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_SOURCE_FUR));
    assert(recipe("GRAPHICS 8388608\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_DIFFUSE_MAPS));
    assert(recipe("GRAPHICS 16777216\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_HEIGHT_FOG));
    if(FH_GRAPHICS_VALID&33554432)assert(recipe("GRAPHICS 33554432\nPLAY_RECORDING\nEND\n",33554432));
    else assert(!recipe("GRAPHICS 33554432\nPLAY_RECORDING\nEND\n",0));
    assert(recipe("GRAPHICS 67108864\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_FUR_GROOM));
    assert(recipe("GRAPHICS 134217728\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_SPLASH_REFLECTION));
    assert(recipe("GRAPHICS 268435456\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_MOTION_BLUR));
    assert(recipe("GRAPHICS 536870912\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_BLUR_FILTER));
    assert(recipe("GRAPHICS 1073741824\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_HEAT));
    assert(recipe("GRAPHICS 2147483648\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_WAKES));
    assert(recipe("GRAPHICS 4294967295\nPLAY_RECORDING\nEND\n",0xffffffffu));
    assert(!recipe("GRAPHICS 4294967296\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("GRAPHICS -1\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("GRAPHICS 999999999999999999999999999999999\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("GRAPHICS 0\nGRAPHICS 1\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("PLAY_RECORDING\nGRAPHICS 0\nEND\n",0));
    /* Every option on every page is reversible and locked during recording. */
    for(unsigned i=0;i<COUNT;i++)if(options[i].kind==OPTION_FLAG){
        page=i/ROWS;unsigned prior=fh_graphics_options,y=65+28*(i%ROWS);
        tap(32,y,0);assert(fh_graphics_options==(prior^options[i].bit));
        tap(32,y,1);assert(fh_graphics_options==(prior^options[i].bit));
        tap(32,y,0);assert(fh_graphics_options==prior);
    }
    page=0;tap(32,153,0);assert(page==(COUNT+ROWS-1)/ROWS-1);
    page=7; /* Original geometry/textures keep their existing page. */
    tap(32,65,0);assert(!fh_graphics_original_requested); /* Missing original package. */
    fh_graphics_original_available=1;tap(32,65,0);assert(fh_graphics_original_requested&&!fh_graphics_original_active);
    tap(32,65,1);assert(fh_graphics_original_requested&&!fh_graphics_original_active);
    tap(32,65,0);assert(!fh_graphics_original_requested);
    tap(32,93,0);assert(fh_graphics_textures_requested&&!fh_graphics_original_requested&&!fh_graphics_textures_active);
    tap(32,93,1);assert(fh_graphics_textures_requested);
    tap(32,93,0);assert(!fh_graphics_textures_requested);
    assert(recipe("GEOMETRY 1\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_ALL));
    assert(!recipe("GEOMETRY 2\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("GEOMETRY 0\nGEOMETRY 1\nPLAY_RECORDING\nEND\n",0));
    assert(recipe("GEOMETRY 0\nTEXTURES 1\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_ALL));
    assert(!recipe("TEXTURES 2\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("TEXTURES 0\nTEXTURES 1\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("PLAY_RECORDING\nTEXTURES 1\nEND\n",0));
    while(page)tap(120,153,0);assert(page==0);
    page=1;tap(32,93,1);assert(fh_graphics_perf_visible); /* Diagnostic only. */
    tap(32,121,0);assert(fh_graphics_gpu_visible);
    tap(32,121,1);assert(fh_graphics_gpu_visible); /* Hardware profiling locks. */
    tap(32,30,0); /* Close menu to show stats. */
    fh_perf_display.heap_used=40000000;fh_perf_display.heap_limit=50331648;
    fh_perf_display.linear_used=5000000;fh_perf_display.linear_limit=10485760;
    fh_perf_display.audio_valid=1;fh_perf_display.audio_ms[0]=40;
    fh_perf_display.audio_underruns[0]=0xffffffffu;
    fh_perf_display.gpu_valid=1;for(unsigned i=0;i<4;i++)fh_perf_display.gpu_words[i]=512u|(512u<<16);
    fh_perf_display.generation++;fh_perf_display.gpu_generation++;
    assert(fh_graphics_options_draw(a,0));assert(fh_graphics_options_draw(b,0));
    old_flush=flushes;for(unsigned i=0;i<10000;i++)assert(!fh_graphics_options_draw(i&1?a:b,0));assert(flushes==old_flush);
    /* Stats never overwrite the item pad or recorder controls, even large counters. */
    for(unsigned x=0;x<320;x++)for(unsigned y=0;y<240;y++)
        if(!((x>=16&&x<160&&y>=16&&y<168)||(x>=176&&x<312&&((y>=16&&y<48)||(y>=52&&y<104)))))assert(a[x*240+239-y]==0x1234);
    for(unsigned i=0;i<FH_PRESET_CUSTOM;i++){
        fh_graphics_options_set(FH_GRAPHICS_VALID);
        assert(fh_graphics_preset_apply(i));
        assert(!(fh_graphics_options&(FH_GRAPHICS_SPLASH_REFLECTION|FH_GRAPHICS_MOTION_BLUR|FH_GRAPHICS_BLUR_FILTER|FH_GRAPHICS_HEAT|FH_GRAPHICS_WAKES)));
    }
    fh_graphics_options_set(fh_graphics_options|FH_GRAPHICS_SPLASH_REFLECTION);
    assert(fh_graphics_preset_apply(FH_PRESET_CUSTOM));
    assert(fh_graphics_options&FH_GRAPHICS_SPLASH_REFLECTION);
    fh_graphics_options_set(FH_GRAPHICS_MOTION_BLUR|FH_GRAPHICS_BLUR_FILTER);
    unsigned requested=fh_graphics_options;
    fh_graphics_options_reject(FH_GRAPHICS_BLUR_FILTER);
    assert(fh_graphics_options==requested&&fh_graphics_unavailable_mask==FH_GRAPHICS_BLUR_FILTER);
    assert(fh_graphics_unavailable_seen==FH_GRAPHICS_BLUR_FILTER);
    opened=1;page=(COUNT-1)/ROWS;generation++;
    assert(fh_graphics_options_draw(a,0));assert(fh_graphics_options_draw(b,0));
    old_flush=flushes;
    for(unsigned i=0;i<10000;i++){fh_graphics_options_reject(FH_GRAPHICS_BLUR_FILTER);assert(!fh_graphics_options_draw(i&1?a:b,0));}
    assert(flushes==old_flush);
    if(argc==2){FILE*f=fopen(argv[1],"wb");assert(f);fwrite(a,2,320*240,f);fclose(f);}
    fh_graphics_options_set(FH_GRAPHICS_MOTION_BLUR);
    assert(!fh_graphics_unavailable_mask&&fh_graphics_unavailable_seen==FH_GRAPHICS_BLUR_FILTER);
    fh_graphics_options_set(requested);assert(!fh_graphics_unavailable_mask);
    unsigned wake_row=0;while(wake_row<COUNT&&options[wake_row].bit!=FH_GRAPHICS_WAKES)wake_row++;
    assert(wake_row<COUNT);fh_graphics_options_set(FH_GRAPHICS_DEFAULT);opened=1;page=wake_row/ROWS;
    int wake_y=65+(wake_row%ROWS)*28;
    tap(32,wake_y,0);assert(fh_graphics_options&FH_GRAPHICS_WAKES);
    tap(32,wake_y,1);assert(fh_graphics_options&FH_GRAPHICS_WAKES);
    tap(32,wake_y,0);assert(!(fh_graphics_options&FH_GRAPHICS_WAKES));
    unsigned ripple_row=0;while(ripple_row<COUNT&&options[ripple_row].kind!=OPTION_EFFECT)ripple_row++;
    assert(ripple_row<COUNT&&!fh_graphics_effects);page=ripple_row/ROWS;
    unsigned old_mask=fh_graphics_options;int ripple_y=65+(ripple_row%ROWS)*28;
    tap(32,ripple_y,0);assert(fh_graphics_effects==FH_EFFECT_RIPPLES&&fh_graphics_options==old_mask);
    tap(32,ripple_y,1);assert(fh_graphics_effects==FH_EFFECT_RIPPLES);
    fh_graphics_effects_reject(FH_EFFECT_RIPPLES);assert(fh_effects_unavailable_mask==1&&fh_effects_unavailable_seen==1);
    assert(fh_graphics_options_draw(a,0));assert(fh_graphics_options_draw(b,0));old_flush=flushes;
    for(unsigned i=0;i<10000;i++){fh_graphics_effects_reject(1);assert(!fh_graphics_options_draw(i&1?a:b,0));}
    assert(flushes==old_flush);tap(32,ripple_y,0);assert(!fh_graphics_effects&&!fh_effects_unavailable_mask&&fh_effects_unavailable_seen==1);
    for(unsigned i=0;i<FH_PRESET_CUSTOM;i++){fh_graphics_effects_set(1);assert(fh_graphics_preset_apply(i)&&!fh_graphics_effects);}
    fh_graphics_effects_set(1);assert(fh_graphics_preset_apply(FH_PRESET_CUSTOM)&&fh_graphics_effects==1);
    FILE*ef=tmpfile();assert(ef);fputs("FHREPLAY 1\nSAVE 0 0\nLIMIT 60\nEFFECTS 1\nPLAY_RECORDING\nEND\n",ef);rewind(ef);
    FhReplayPlan ep;char error[128];assert(fh_replay_parse(ef,&ep,error,sizeof error)&&ep.graphics_effects==1);fclose(ef);
    assert(recipe("EFFECTS 0\nPLAY_RECORDING\nEND\n",FH_GRAPHICS_ALL));
    assert(!recipe("EFFECTS 2\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("EFFECTS -1\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("EFFECTS 4294967296\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("EFFECTS 1\nEFFECTS 0\nPLAY_RECORDING\nEND\n",0));
    assert(!recipe("PLAY_RECORDING\nEFFECTS 1\nEND\n",0));
    assert(!recipe("EFFECTS 1x\nPLAY_RECORDING\nEND\n",0));
    free(a);free(b);puts("Graphics UI: bounds, tap ownership, lock, idle no-flush, independent extended effects and replay settings passed");
}
