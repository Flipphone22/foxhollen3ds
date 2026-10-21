/* LCD-only menu: no recurring GPU work, allocation or SD writes. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "fh_graphics_options.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_perf_display.h"
#include "fh_hardware.h"

unsigned fh_graphics_options=FH_GRAPHICS_DEFAULT;
unsigned fh_graphics_effects,fh_effects_unavailable_mask,fh_effects_unavailable_seen;
unsigned fh_graphics_unavailable_mask,fh_graphics_unavailable_seen;
static unsigned quarantine_options,quarantine_effects;
static int quarantine_loaded;
static void quarantine_load(void){
    if(quarantine_loaded)return;
    quarantine_loaded=1;
    FILE *f=fopen("sdmc:/3ds/foxhollow/gpu-quarantine-2.txt","rb");if(!f)return;
    unsigned options=0,effects=0;
    if(fscanf(f,"%u %u",&options,&effects)==2){
        quarantine_options=options&(FH_GRAPHICS_BLUR_FILTER|FH_GRAPHICS_HEAT|FH_GRAPHICS_MOTION_BLUR);
        quarantine_effects=effects&FH_EFFECT_DISTORTION;
    }
    fclose(f);
}
void fh_graphics_quarantine_reload(void){quarantine_loaded=0;quarantine_options=quarantine_effects=0;quarantine_load();}
void fh_graphics_gpu_quarantine(void){
    quarantine_load();
    unsigned options=fh_graphics_options&(FH_GRAPHICS_BLUR_FILTER|FH_GRAPHICS_HEAT);
    unsigned effects=fh_graphics_effects&FH_EFFECT_DISTORTION;
    if(!(options&~quarantine_options) && !(effects&~quarantine_effects))return;
    quarantine_options|=options;quarantine_effects|=effects;
    FILE *f=fopen("sdmc:/3ds/foxhollow/gpu-quarantine-2.txt","wb");
    if(f){fprintf(f,"%u %u\n",quarantine_options,quarantine_effects);fclose(f);}
    char line[128];snprintf(line,sizeof line,"GPU stall quarantine: options=%u effects=%u\n",quarantine_options,quarantine_effects);fh_platform_log(line);
}
unsigned fh_graphics_fps_visible;
unsigned fh_graphics_original_requested,fh_graphics_original_active,fh_graphics_original_available;
unsigned fh_graphics_textures_requested,fh_graphics_textures_active;
unsigned fh_graphics_perf_visible,fh_graphics_gpu_visible;
static unsigned opened,held,locked,page,generation;
static unsigned fps_generation,fps_tenths,fps_frames;
static uint64_t fps_start;
enum {OPTION_FLAG,OPTION_FPS,OPTION_PERF,OPTION_GPU,OPTION_GEOMETRY,OPTION_TEXTURES,OPTION_EFFECT};
static const struct {const char *label;unsigned bit,inverted,kind;} options[]={
    {"PARTICLES",FH_GRAPHICS_PARTICLES,0,OPTION_FLAG},
    {"LIGHTING",FH_GRAPHICS_NO_LIGHTING,1,OPTION_FLAG},
    {"ENV LIGHT",FH_GRAPHICS_NO_ENVIRONMENT,1,OPTION_FLAG},
    {"FPS DISPLAY",0,0,OPTION_FPS},
    {"PERF STATS",0,0,OPTION_PERF},
    {"GPU COUNTERS",0,0,OPTION_GPU},
    {"RENDERING",FH_GRAPHICS_NO_RENDER,1,OPTION_FLAG},
    {"WORLD",FH_GRAPHICS_NO_WORLD,1,OPTION_FLAG},
    {"ACTORS",FH_GRAPHICS_NO_ACTORS,1,OPTION_FLAG},
    {"WORLD TEX",FH_GRAPHICS_NO_WORLD_TEX,1,OPTION_FLAG},
    {"ACTOR TEX",FH_GRAPHICS_NO_ACTOR_TEX,1,OPTION_FLAG},
    {"HUD",FH_GRAPHICS_NO_HUD,1,OPTION_FLAG},
    {"HOLOGRAM",FH_GRAPHICS_NO_HOLOGRAM,1,OPTION_FLAG},
    {"SKY",FH_GRAPHICS_NO_SKY,1,OPTION_FLAG},
    {"HUD TEX",FH_GRAPHICS_NO_HUD_TEX,1,OPTION_FLAG},
    {"PARTICLE TEX",FH_GRAPHICS_NO_PARTICLE_TEX,1,OPTION_FLAG},
    {"MOVIES",FH_GRAPHICS_NO_MOVIES,1,OPTION_FLAG},
    {"GROUND SHADOW",FH_GRAPHICS_NO_GROUND_SHADOWS,1,OPTION_FLAG},
    {"DISTANCE FOG",FH_GRAPHICS_NO_DISTANCE_FOG,1,OPTION_FLAG},
    {"SHADOW SHAPE",FH_GRAPHICS_SILHOUETTE_SHADOWS,0,OPTION_FLAG},
    {"SPLASHES",FH_GRAPHICS_SPLASHES,0,OPTION_FLAG},
    {"ORIGINAL GEO",0,0,OPTION_GEOMETRY},
    {"ORIGINAL TEX",0,0,OPTION_TEXTURES},
    {"MORPHS",FH_GRAPHICS_NO_MORPHS,1,OPTION_FLAG},
    {"WATER DROPS",FH_GRAPHICS_WATER_DROPS,0,OPTION_FLAG},
    {"FUR",FH_GRAPHICS_FUR,0,OPTION_FLAG},
    {"SPECULAR",FH_GRAPHICS_SPECULAR,0,OPTION_FLAG},
    {"SOURCE FUR",FH_GRAPHICS_SOURCE_FUR,0,OPTION_FLAG},
    {"LIGHT MAPS",FH_GRAPHICS_DIFFUSE_MAPS,0,OPTION_FLAG},
    {"HEIGHT FOG",FH_GRAPHICS_HEIGHT_FOG,0,OPTION_FLAG},
    {"FUR GROOM",FH_GRAPHICS_FUR_GROOM,0,OPTION_FLAG},
    {"SPLASH REFL",FH_GRAPHICS_SPLASH_REFLECTION,0,OPTION_FLAG},
    {"MOTION BLUR",FH_GRAPHICS_MOTION_BLUR,0,OPTION_FLAG},
    {"BLUR FILTER",FH_GRAPHICS_BLUR_FILTER,0,OPTION_FLAG},
    {"HEAT HAZE",FH_GRAPHICS_HEAT,0,OPTION_FLAG},
    {"WAKES",FH_GRAPHICS_WAKES,0,OPTION_FLAG},
    {"RIPPLES",FH_EFFECT_RIPPLES,0,OPTION_EFFECT},
    {"RIPPLE FAST",FH_EFFECT_RIPPLE_FAST,0,OPTION_EFFECT},
    {"PART REFL",FH_EFFECT_PARTICLE_REFLECTION,0,OPTION_EFFECT},
    {"SCREEN COLOR",FH_EFFECT_SCREEN_COLOR,0,OPTION_EFFECT},
    {"SPIRIT VISION",FH_EFFECT_SPIRIT_VISION,0,OPTION_EFFECT},
    {"DISTORTION EXP",FH_EFFECT_DISTORTION,0,OPTION_EFFECT},
    {"WEATHER",FH_GRAPHICS_WEATHER,0,OPTION_FLAG},
};
enum { COUNT=sizeof(options)/sizeof(options[0]), ROWS=3 };
void fh_graphics_options_reject(unsigned mask){
    mask&=fh_graphics_options&(FH_GRAPHICS_MOTION_BLUR|FH_GRAPHICS_BLUR_FILTER|FH_GRAPHICS_HEAT|FH_GRAPHICS_WAKES|FH_GRAPHICS_SPLASH_REFLECTION);
    if(!(mask&~fh_graphics_unavailable_mask))return;
    fh_graphics_unavailable_mask|=mask;fh_graphics_unavailable_seen|=mask;generation++;
    char line[128];snprintf(line,sizeof(line),"Graphics memory unavailable: mask=%u requested=%u\n",mask,fh_graphics_options);fh_platform_log(line);
}
void fh_graphics_effects_set(unsigned mask){
    if(mask&~FH_EFFECT_VALID)fh_platform_panic("Unknown extended graphics effects");
    quarantine_load();mask&=~quarantine_effects;
    if(fh_graphics_effects==mask)return;
    fh_effects_unavailable_mask&=mask;fh_graphics_effects=mask;generation++;
    char line[96];snprintf(line,sizeof line,"Graphics effects: mask=%u\n",mask);fh_platform_log(line);
}
void fh_graphics_effects_reject(unsigned mask){
    mask&=fh_graphics_effects&FH_EFFECT_VALID;
    if(!(mask&~fh_effects_unavailable_mask))return;
    fh_effects_unavailable_mask|=mask;fh_effects_unavailable_seen|=mask;generation++;
    char line[96];snprintf(line,sizeof line,"Graphics effects unavailable: mask=%u\n",mask);fh_platform_log(line);
}
void fh_graphics_options_set(unsigned mask){
    if(mask&~FH_GRAPHICS_VALID)fh_platform_panic("Unknown graphics options");
    quarantine_load();mask&=~quarantine_options;
    if(fh_graphics_options==mask)return;
    fh_graphics_unavailable_mask&=mask;
    fh_graphics_options=mask;generation++;
    char line[128];snprintf(line,sizeof(line),"Graphics options: mask=%u particles=%u lighting=%u environment=%u\n",mask,!!(mask&FH_GRAPHICS_PARTICLES),!(mask&FH_GRAPHICS_NO_LIGHTING),!(mask&(FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ENVIRONMENT)));fh_platform_log(line);
}
void fh_graphics_options_frame(uint64_t ticks,uint64_t frequency){
    if(!fh_graphics_fps_visible)return;
    if(!fps_start){fps_start=ticks;fps_frames=0;return;}
    fps_frames++;
    uint64_t elapsed=ticks-fps_start;
    if(elapsed<frequency/2 || !elapsed)return;
    uint64_t value=((uint64_t)fps_frames*frequency*10+elapsed/2)/elapsed;
    unsigned next=value>9999?9999:(unsigned)value;
    if(next!=fps_tenths){fps_tenths=next;fps_generation++;}
    fps_start=ticks;fps_frames=0;
}
void fh_graphics_options_touch(unsigned now,int x,int y,unsigned lock){
    if(locked!=!!lock){locked=!!lock;generation++;}
    if(now&&!held && x>=16 && x<160){
        if(y>=16 && y<48){opened=!opened;generation++;}
        else if(opened && y>=56 && y<140){
            unsigned i=page*ROWS+(y-56)/28;
            if(i<COUNT && options[i].kind==OPTION_FPS){
                fh_graphics_fps_visible^=1;fps_start=0;fps_frames=fps_tenths=0;
                generation++;fps_generation++;
            }else if(i<COUNT && options[i].kind==OPTION_PERF){
                fh_graphics_perf_visible^=1;generation++;
            }else if(i<COUNT && options[i].kind==OPTION_GPU && !locked){
                fh_graphics_gpu_visible^=1;generation++;
            }else if(i<COUNT && !locked){
                if(options[i].kind==OPTION_GEOMETRY){
                    if(fh_graphics_original_available){fh_graphics_original_requested^=1;generation++;}
                }else if(options[i].kind==OPTION_TEXTURES){
                    fh_graphics_textures_requested^=1;generation++;
                }else if(options[i].kind==OPTION_EFFECT)fh_graphics_effects_set(fh_graphics_effects^options[i].bit);
                else fh_graphics_options_set(fh_graphics_options^options[i].bit);
            }
        }else if(opened && y>=144 && y<168 && COUNT>ROWS){
            unsigned pages=(COUNT+ROWS-1)/ROWS;
            page=(page+pages+(x<88?-1:1))%pages;generation++;
        }
    }
    held=now;
}
static void pixel(uint16_t*f,int x,int y,uint16_t c){f[x*240+239-y]=c;}
static void box(uint16_t*f,int x,int y,int w,int h,uint16_t c){
    for(int i=x;i<x+w;i++)for(int j=y;j<y+h;j++)pixel(f,i,j,c);
}
static void label(uint16_t*f,int x,int y,const char*s,unsigned scale){
    static const char letters[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ<>0123456789.-/";
    static const uint8_t bits[][7]={
        {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
        {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
        {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
        {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
        {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
        {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
        {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
        {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{2,4,8,16,8,4,2},{8,4,2,1,2,4,8},
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14},{0,0,0,0,0,12,12},{0,0,0,31,0,0,0},{1,2,2,4,8,8,16}};
    for(unsigned n=0;s[n];n++){
        const char*q=strchr(letters,s[n]);if(!q)continue;unsigned k=q-letters;
        for(unsigned r=0;r<7;r++)for(unsigned c=0;c<5;c++)if(bits[k][r]&(16>>c))
            for(unsigned a=0;a<scale;a++)for(unsigned b=0;b<scale;b++){
                unsigned px=x+(n*6+c)*scale+a,py=y+r*scale+b;
                if(px<(x<160?160u:312u)&&py<240)pixel(f,px,py,0xffff);
            }
    }
}
unsigned fh_graphics_options_draw(uint16_t*f,unsigned initial){
    static uint16_t*buffers[2];static unsigned generations[2],fps_generations[2];unsigned slot=0;
    static unsigned perf_generations[2],gpu_generations[2];
    if(buffers[0]&&buffers[0]!=f)slot=1;
    unsigned changed=0,left_dirty=0,right_dirty=0,menu_dirty=initial||buffers[slot]!=f||generations[slot]!=generation+1;
    unsigned fps_dirty=initial||buffers[slot]!=f||fps_generations[slot]!=fps_generation+1;
    unsigned perf_dirty=initial||buffers[slot]!=f||perf_generations[slot]!=fh_perf_display.generation+1;
    unsigned gpu_dirty=menu_dirty||initial||buffers[slot]!=f||gpu_generations[slot]!=fh_perf_display.gpu_generation+1;
    if(!menu_dirty&&!fps_dirty&&!perf_dirty&&!gpu_dirty)return 0;
    buffers[slot]=f;
    if(menu_dirty){
    generations[slot]=generation+1;
    box(f,16,16,144,152,0);
    box(f,16,16,144,32,opened?0x320e:0x18c6);label(f,24,25,opened?"CLOSE":(fh_graphics_unavailable_mask|fh_effects_unavailable_mask)?"GFX MEMORY":"GRAPHICS",2);
    if(opened){
        for(unsigned r=0;r<ROWS && page*ROWS+r<COUNT;r++){
            unsigned i=page*ROWS+r,on=options[i].bit?!!(fh_graphics_options&options[i].bit)^options[i].inverted:fh_graphics_fps_visible,y=56+28*r;
            if(options[i].kind==OPTION_GEOMETRY)on=fh_graphics_original_requested;
            if(options[i].kind==OPTION_TEXTURES)on=fh_graphics_textures_requested;
            if(options[i].kind==OPTION_EFFECT)on=!!(fh_graphics_effects&options[i].bit);
            if(options[i].kind==OPTION_PERF)on=fh_graphics_perf_visible;
            if(options[i].kind==OPTION_GPU)on=fh_graphics_gpu_visible;
            unsigned unavailable=!!(options[i].bit&(options[i].kind==OPTION_EFFECT?fh_effects_unavailable_mask:options[i].kind==OPTION_FLAG?fh_graphics_unavailable_mask:0));
            box(f,16,y,144,26,unavailable?0x6000:locked&&options[i].kind!=OPTION_FPS&&options[i].kind!=OPTION_PERF?0x3186:on?0x0340:0x6000);
            label(f,22,y+9,options[i].label,1);label(f,116,y+9,options[i].kind==OPTION_GEOMETRY&&!fh_graphics_original_available?"N-A":unavailable?"MEM":on?"ON":"OFF",1);
        }
        if(page*ROWS+ROWS>=COUNT){
            /* The last page can have three controls; keep the status inside
             * the header rather than painting over its third touch row. */
            label(f,22,40,fh_graphics_unavailable_mask?"OPTION NEEDS MEMORY":(fh_graphics_original_requested!=fh_graphics_original_active || fh_graphics_textures_requested!=fh_graphics_textures_active)?"RESTART REQUIRED":"SELECT SAVES OPTIONS",1);
        }
        if(COUNT>ROWS){label(f,28,150,"< PREV",1);label(f,108,150,"NEXT >",1);}
    }
    /* Exact changed LCD column span; shared caller swaps once for all widgets. */
    left_dirty=1;changed=1;
    }
    if(perf_dirty||menu_dirty){
        perf_generations[slot]=fh_perf_display.generation+1;
        /* Old hardware has no recording button here. Reuse its existing LCD
         * column flush; no new framebuffer, GPU command or SD write. */
        if(!fh_hardware_is_new){
            box(f,16,166,144,74,0);
            if(!opened&&fh_graphics_perf_visible){
                FhPerfDisplay*p=&fh_perf_display;char line[32];
                if(p->cpu_valid){
                    snprintf(line,sizeof(line),"MAIN %u.%01u MS AVG",p->cpu_total_us/1000,p->cpu_total_us%1000/100);label(f,20,167,line,1);
                    static const char*names[16]={"SIM","PREP","DRAW","TEX","AUD","LOAD","FX","UI","GPUW","WRKW","VBL","OTHER","SET","PRES","IN","MET"};
                    for(unsigned i=0;i<8;i++){
                        unsigned a=p->cpu_us[i*2],b=p->cpu_us[i*2+1];
                        /* Values >=100ms still fit in each 11-character cell. */
                        snprintf(line,sizeof(line),"%s %u.%01u",names[i*2],a/1000,a%1000/100);label(f,20,176+i*8,line,1);
                        snprintf(line,sizeof(line),"%s %u.%01u",names[i*2+1],b/1000,b%1000/100);label(f,92,176+i*8,line,1);
                    }
                }else label(f,20,167,"MAIN TIMING WAITING",1);
            }
            left_dirty=1;changed=1;
        }
        if(!opened&&fh_graphics_perf_visible){
            char line[32];FhPerfDisplay*p=&fh_perf_display;
            box(f,16,56,144,110,0);
            snprintf(line,sizeof(line),"HEAP %u/%u K",p->heap_used/1024,p->heap_limit/1024);label(f,20,58,line,1);
            snprintf(line,sizeof(line),"LINEAR %u/%u K",p->linear_used/1024,p->linear_limit/1024);label(f,20,70,line,1);
            snprintf(line,sizeof(line),"VRAM %u/6144 K",p->vram_used/1024);label(f,20,82,line,1);
            if(p->audio_valid)snprintf(line,sizeof(line),"AUDIO %u/%u MS",p->audio_ms[0],p->audio_ms[1]);else snprintf(line,sizeof(line),"AUDIO SAMPLE BUSY");
            label(f,20,94,line,1);
            snprintf(line,sizeof(line),"UNDERRUN %u/%u",p->audio_underruns[0],p->audio_underruns[1]);label(f,20,106,line,1);
            snprintf(line,sizeof(line),"READ %u K/S",p->read_kib_s);label(f,20,118,line,1);
            snprintf(line,sizeof(line),"READ IPC %u MS",p->read_ms);label(f,20,130,line,1);
            snprintf(line,sizeof(line),"FS IPC %u MS",p->fs_ms);label(f,20,142,line,1);
            snprintf(line,sizeof(line),"READ MAX %u MS",p->read_max_ms);label(f,20,154,line,1);
            left_dirty=1;changed=1;
        }
    }
    if(gpu_dirty){
        gpu_generations[slot]=fh_perf_display.gpu_generation+1;
        box(f,176,52,136,52,0);
        if(fh_graphics_gpu_visible){
            char line[32];FhPerfDisplay*p=&fh_perf_display;
            if(p->gpu_timing_valid){
                snprintf(line,sizeof(line),"FRAME %u.%02u MS",p->frame_us/1000,p->frame_us%1000/10);label(f,180,54,line,1);
                snprintf(line,sizeof(line),"GPU Q %u.%02u MS",p->gpu_queue_us/1000,p->gpu_queue_us%1000/10);label(f,180,62,line,1);
            }else label(f,180,54,"GPU TIMING WAITING",1);
            if(!p->gpu_valid)label(f,180,78,p->gpu_failed?"COUNTERS UNSUPPORTED":"COUNTERS WAITING",1);
            if(p->gpu_valid)for(unsigned i=0;i<4;i++){
                snprintf(line,sizeof(line),"%u %04u  %u %04u",i*2,p->gpu_words[i]&65535,i*2+1,p->gpu_words[i]>>16);
                label(f,180,72+i*8,line,1);
            }
        }
        right_dirty=1;changed=1;
    }
    if(fps_dirty){
        fps_generations[slot]=fps_generation+1;
        box(f,176,16,136,32,0);
        if(fh_graphics_fps_visible){
            char text[16];
            if(fps_tenths)snprintf(text,sizeof(text),"FPS %u.%u",fps_tenths/10,fps_tenths%10);
            else snprintf(text,sizeof(text),"FPS --");
            label(f,180,25,text,2);
        }
        /* One cache call, at most twice per second per LCD buffer; no GPU
         * rendering or SD traffic. Hiding FPS clears each buffer once. */
        right_dirty=1;changed=1;
    }
    /* Publish only after every widget has written. Both spans retain the
     * established column bounds; the caller swaps only after these flushes. */
    if(left_dirty)fh_gpu_cache_flush(f+16*240,144*240*sizeof(*f));
    if(right_dirty)fh_gpu_cache_flush(f+176*240,136*240*sizeof(*f));
    return changed;
}
