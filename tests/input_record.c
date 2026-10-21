#include <3ds.h>
#include "fh_input_tape.h"
#include "fh_graphics_options.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
unsigned fh_hardware_is_new=1,fh_hardware_large_memory=1;
unsigned fh_graphics_options=0; /* Deliberately record a non-default visual choice. */
unsigned fh_graphics_effects=FH_EFFECT_RIPPLES;
unsigned fh_graphics_original_active=0,fh_graphics_textures_active=1;
static double clock_value;static FhReplaySnapshot state;
static unsigned copies;
void fh_platform_log(const char*s){fputs(s,stdout);}
void fh_gpu_cache_flush(const void*p,unsigned n){(void)p;(void)n;}
double fh_input_clock(void){return clock_value;}
void fh_replay_snapshot(FhReplaySnapshot*s){*s=state;}
int fh_card_record_fixture(const char*p,unsigned*b,unsigned*c){copies++;FILE*f=fopen(p,"wb");assert(f);fclose(f);*b=*c=0;return 1;}
int main(int argc,char**argv){
    (void)argc;mkdir("sdmc:",0777);mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow",0777);
    FhPadState pad={0};
    if(argv[1][0]=='r')fh_record_frame(0.75f);
    fh_record_pad(&pad);state.title=1;clock_value=1;fh_record_pad(&pad);
    if(argv[1][0]=='l'){pad.buttons=0x100;clock_value=2;fh_record_pad(&pad);pad.buttons=0;fh_record_touch(1,32,190);clock_value=3;fh_record_pad(&pad);assert(copies==0);puts("PASS: first gameplay input permanently locks recording");return 0;}
    if(argv[1][0]=='a'){
        fh_record_touch(1,32,190);clock_value=2;fh_record_pad(&pad);assert(!copies&&!fh_record_active());
        FILE*f=fopen("sdmc:/3ds/foxhollow/record-next-boot.txt","rb");assert(f&&fgetc(f)=='5');fclose(f);
        puts("PASS: recording arms next launch without losing pre-arm simulation steps");return 0;
    }
    assert(copies==1);
    assert(fh_record_active());
    /* The published recipe must use the starting options, not mutable globals. */
    fh_graphics_effects=0;
    for(unsigned i=3;i<500;i++){clock_value=i;fh_record_frame(i%5?1.25f:0);FhTapeIo io={42,i*32,32,32};fh_record_dvd_complete(&io);fh_record_movie_time(1,(1ULL<<35)+i);pad.x=(int)(i%127);pad.buttons=(i/60)%2?0x100:0;fh_record_pad(&pad);if(i%100==0)svcSleepThread(3000000);}
    fh_record_touch(1,32,190);clock_value=500;fh_record_pad(&pad);assert(copies==1);
    assert(!fh_record_active());
    uint16_t*image=calloc(320*240,sizeof(uint16_t));assert(image);fh_record_draw(image,1);free(image);
    DIR*dir=opendir("sdmc:/3ds/foxhollow/replays/cases");assert(dir);struct dirent*entry;char path[256]={0};
    while((entry=readdir(dir)))if(!strncmp(entry->d_name,"record-",7))snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/replays/cases/%s/scenario.fhr",entry->d_name);closedir(dir);assert(path[0]);
    FILE*f=fopen(path,"rb");assert(f);FhReplayPlan plan;char error[128];assert(fh_replay_parse(f,&plan,error,sizeof(error)));fclose(f);assert(plan.count==2&&plan.steps[0].kind==FH_R_TAPE&&plan.save_bytes==0);
    assert(plan.graphics_options==0);
    assert(plan.graphics_effects==FH_EFFECT_RIPPLES);
    assert(plan.original_geometry==0&&plan.original_textures==1);
    strcpy(strrchr(path,'/')+1,"recording.fht");assert(fh_tape_stream_open(path,0));assert(fh_tape_stream_ring()->version==5);
    unsigned anchors=0,polls=0,clocks=0,ios=0,times=0;FhTapeSample sample;
    for(;;){if(!fh_tape_peek(fh_tape_stream_ring(),&sample)){svcSleepThread(1000000);continue;}
        fh_tape_pop(fh_tape_stream_ring());fh_tape_stream_notify();if(sample.flags==FH_TAPE_END)break;
        if(sample.phase==FH_TAPE_ANCHOR)anchors++;else if(sample.phase==FH_TAPE_PAD)polls++;else if(sample.phase==FH_TAPE_CLOCK)clocks++;else if(sample.phase==FH_TAPE_IO){assert(sample.io.file==42&&sample.io.bytes==32);ios++;}else if(sample.phase==FH_TAPE_TIME){assert(sample.time.site==1&&sample.time.high==8);times++;}else assert(0);
    }
    assert(anchors==1&&polls==498&&clocks==498&&ios==497&&times==497);assert(fh_tape_stream_close(0));
    puts("PASS: next boot snapshots once; all clock and PAD events stream with CRC, including repeated polls and zero deltas; stop publishes recipe; saved panel stays in bounds");
}
