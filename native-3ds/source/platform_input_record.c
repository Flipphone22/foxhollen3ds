#include "fh_replay.h"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include "fh_input_tape.h"
#include "fh_hardware.h"
#include "fh_gpu_cache.h"
#include "fh_graphics_options.h"
#if defined(FH_REPLAY_BUILD) && !defined(FH_UNIFIED_APP)
void fh_record_touch(unsigned held,int x,int y){(void)held;(void)x;(void)y;}
void fh_record_pad(const FhPadState*p){(void)p;}
unsigned fh_record_draw(uint16_t*f,unsigned initial){(void)f;(void)initial;return 0;}
unsigned fh_record_active(void){return 0;}
void fh_record_finish(void){}
void fh_record_frame(float delta){(void)delta;}
void fh_record_dvd_complete(const FhTapeIo*io){(void)io;}
void fh_record_movie_time(unsigned site,uint64_t time){(void)site;(void)time;}
#else
/* Private isolated replay cases; bounded RAM only between arm and stop.
 * The complete marker (scenario.fhr) is published last. */
#define CASES "sdmc:/3ds/foxhollow/replays/cases"
static FhTapeRing *tape;
static unsigned packets,record_graphics,record_original,record_textures,record_effects;
static double ordered_tick;
static FhTapeSample last_pad;
static unsigned have_pad;
static unsigned state,held,tap,seen_input,boot_checked,save_bytes,save_crc,paint_generation;
static char directory[160];
enum { WAIT,READY,RECORDING,SAVED,LOCKED,ERROR,ARMED };
#define ARM_FILE "sdmc:/3ds/foxhollow/record-next-boot.txt"
static void status(unsigned value){state=value;paint_generation++;}
static int active_pad(const FhPadState*p){return p->buttons||abs(p->x)>15||abs(p->y)>15||abs(p->cx)>15||abs(p->cy)>15||p->left>30||p->right>30;}
static void begin_record(void){
    mkdir("sdmc:/3ds/foxhollow/replays",0777);mkdir(CASES,0777);
    snprintf(directory,sizeof(directory),CASES "/record-%016llx",(unsigned long long)svcGetSystemTick());
    char path[192];snprintf(path,sizeof(path),"%s/fixture.gci",directory);
    if(mkdir(directory,0777)||!fh_card_record_fixture(path,&save_bytes,&save_crc)){status(ERROR);return;}
    snprintf(path,sizeof(path),"%s/recording.fht",directory);
    if(!fh_tape_stream_open(path,1)){status(ERROR);return;}
    tape=fh_tape_stream_ring();FhReplaySnapshot anchor;fh_replay_snapshot(&anchor);
    FhTapeSample first={.tick=0,.x=anchor.x,.y=anchor.y,.z=anchor.z,
        .ui=anchor.ui,.sequence=anchor.sequence,.player=anchor.player,.loading=anchor.loading,.flags=FH_TAPE_CHECK,.phase=FH_TAPE_ANCHOR};
    if(!fh_tape_push(tape,&first)){fh_tape_stream_close(0);tape=NULL;status(ERROR);return;}
    fh_tape_stream_notify();
    record_graphics=fh_graphics_options;record_effects=fh_graphics_effects;
    record_original=fh_graphics_original_active;
    record_textures=fh_graphics_textures_active;
    packets=1;ordered_tick=0;
    status(RECORDING);
}
void fh_record_finish(void){
    if(!tape)return;
    char path[192],temporary[192];int ok=state==RECORDING;
    double duration=ordered_tick;
    if(!fh_tape_stream_close(duration))ok=0;
    tape=NULL;
    snprintf(temporary,sizeof(temporary),"%s/scenario.tmp",directory);
    if(ok){
        FILE *f=fopen(temporary,"wb");if(!f)ok=0;
        else {
            if(fprintf(f,"FHREPLAY 1\nSAVE %u %08x\nLIMIT 3600\nGRAPHICS %u\nEFFECTS %u\nGEOMETRY %u\nTEXTURES %u\nPLAY_RECORDING\nEND\n",save_bytes,save_crc,record_graphics,record_effects,record_original,record_textures)<0||fflush(f))ok=0;
            if(fclose(f))ok=0;
        }
        snprintf(path,sizeof(path),"%s/scenario.fhr",directory);
        if(ok&&rename(temporary,path))ok=0;
    }
    char message[384];snprintf(message,sizeof(message),"Ordered input recording %s: %s packets=%u duration=%.9g ticks; captured simulation steps, checkpoints required\n",ok?"saved":"FAILED",directory,packets,duration);
    fh_platform_log(message);status(ok?SAVED:ERROR);
}
static void boot_record(void){
#ifdef FH_UNIFIED_APP
    if(fh_replay_active())return;
#endif
    if(boot_checked||!fh_hardware_is_new)return;
    boot_checked=1;status(READY);
    FILE*f=fopen(ARM_FILE,"rb");if(!f)return;
    int armed=fgetc(f)=='5';fclose(f);
    if(!armed){status(ERROR);return;}
    begin_record();
    if(state==RECORDING&&remove(ARM_FILE)){
        fh_tape_stream_invalidate();status(ERROR);fh_record_finish();
    }
}
static void append_sample(const FhTapeSample*s){
    if(ordered_tick>216000||__atomic_load_n(&tape->error,__ATOMIC_ACQUIRE)||!fh_tape_push(tape,s)){
        status(ERROR);fh_tape_stream_invalidate();
    }else {packets++;fh_tape_stream_notify();}
}
void fh_record_frame(float delta){
    if(!fh_hardware_is_new)return;
    boot_record();
    if(state!=RECORDING)return;
    ordered_tick+=(double)delta;
    have_pad=0;
    FhTapeSample s={.tick=ordered_tick,.phase=FH_TAPE_CLOCK,.delta=delta};append_sample(&s);
}
void fh_record_dvd_complete(const FhTapeIo*io){
    if(!fh_hardware_is_new)return;
    boot_record();
    if(state!=RECORDING)return;
    FhTapeSample s={.tick=ordered_tick,.phase=FH_TAPE_IO,.io=*io};append_sample(&s);
    have_pad=0; /* A completion separates otherwise identical controller reads. */
}
void fh_record_movie_time(unsigned site,uint64_t time){
    if(!fh_hardware_is_new)return;
    boot_record();if(state!=RECORDING)return;
    FhTapeSample s={.tick=ordered_tick,.phase=FH_TAPE_TIME,.time={site,(unsigned)time,(unsigned)(time>>32),0}};
    append_sample(&s);have_pad=0;
}
void fh_record_touch(unsigned now,int x,int y){
#ifdef FH_UNIFIED_APP
    if(fh_replay_active())return;
#endif
#if !defined(FH_REPLAY_BUILD) || defined(FH_UNIFIED_APP)
    if(fh_hardware_is_new&&now&&!held&&x>=16&&x<144&&y>=176&&y<224)tap=1;
#endif
    held=now;
}
void fh_record_pad(const FhPadState*p){
#ifdef FH_UNIFIED_APP
    if(fh_replay_active())return;
#endif
#if defined(FH_REPLAY_BUILD) && !defined(FH_UNIFIED_APP)
    (void)p;return; /* Playback must never record itself. */
#else
    if(!fh_hardware_is_new||state>=SAVED)return;
    boot_record();
    if(tap){
        tap=0;
        if(state==READY&&!seen_input&&!active_pad(p)){
            FILE*f=fopen(ARM_FILE,"wb");int ok=f!=NULL;
            if(f){if(fputs("5\n",f)<0||fflush(f))ok=0;if(fclose(f))ok=0;}
            status(ok?ARMED:ERROR);
            fh_platform_log(ok?"Recording armed for next launch; exit and relaunch to capture all simulation steps.\n":"Cannot arm next-launch recording.\n");
        }else if(state==RECORDING)fh_record_finish();
    }
    if(state==RECORDING){
        FhReplaySnapshot s;fh_replay_snapshot(&s);
        FhTapeSample sample={.tick=ordered_tick,.pad=*p,.x=s.x,.y=s.y,.z=s.z,
            .ui=s.ui,.sequence=s.sequence,.player=s.player,.loading=s.loading,.flags=FH_TAPE_CHECK,.phase=FH_TAPE_PAD};
        if(!have_pad||memcmp(&sample.pad,&last_pad.pad,sizeof(sample.pad))||
           sample.x!=last_pad.x||sample.y!=last_pad.y||sample.z!=last_pad.z||sample.ui!=last_pad.ui||
           sample.sequence!=last_pad.sequence||sample.player!=last_pad.player||sample.loading!=last_pad.loading){
            append_sample(&sample);last_pad=sample;have_pad=1;
        }
    }
    if(active_pad(p)){seen_input=1;if(state==WAIT||state==READY)status(LOCKED);}
#endif
}
unsigned fh_record_active(void){return state==RECORDING;}
static void pixel(uint16_t*f,int x,int y,uint16_t c){f[x*240+239-y]=c;}
static void label(uint16_t*f,const char*s){
    static const char letters[]="ACDEIKLOPRSTVW";
    static const unsigned char bits[][7]={
        {14,17,17,31,17,17,17},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
        {31,16,16,30,16,16,31},{31,4,4,4,4,4,31},{17,18,20,24,20,18,17},
        {16,16,16,16,16,16,31},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
        {30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
        {17,17,17,17,17,10,4},{17,17,17,21,21,21,10}};
    for(unsigned n=0;s[n];n++){
        const char*q=strchr(letters,s[n]);if(!q)continue;unsigned k=q-letters;
        for(int y=0;y<7;y++)for(int x=0;x<5;x++)if(bits[k][y]&(16>>x))
            for(int a=0;a<2;a++)for(int b=0;b<2;b++)pixel(f,28+n*12+x*2+a,193+y*2+b,0xffff);
    }
}
unsigned fh_record_draw(uint16_t*f,unsigned initial){
#ifdef FH_UNIFIED_APP
    if(fh_replay_active())return 0;
#endif
#if !defined(FH_REPLAY_BUILD) || defined(FH_UNIFIED_APP)
    if(!fh_hardware_is_new)return 0;
    static uint16_t*buffers[2];static unsigned generations[2];unsigned slot=0;
    if(buffers[0]&&buffers[0]!=f)slot=1;
    if(!initial&&buffers[slot]==f&&generations[slot]==paint_generation+1)return 0;
    buffers[slot]=f;generations[slot]=paint_generation+1;
    uint16_t color=state==RECORDING?0xb800:state==READY?0x0340:0x3186;
    for(int x=16;x<144;x++)for(int y=176;y<224;y++)pixel(f,x,y,color);
    const char*names[]={"WAIT","RECORD","STOP","SAVED","LOCKED","ERROR","RESTART"};label(f,names[state]);
    /* LCD framebuffer, not a PICA render target. Flush changed columns only. */
    fh_gpu_cache_flush(f+16*240,128*240*sizeof(*f));
    return 1;
#else
    (void)f;(void)initial;return 0;
#endif
}

#endif
