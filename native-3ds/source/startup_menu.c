#include "fh_startup_menu.h"
#include "fh_audio_bench_mode.h"
#include "fh_gpu_queue_mode.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
void fh_startup_menu_init(FhStartupMenu*m,const char*build,unsigned hardware_new,unsigned ready,unsigned probes){
    memset(m,0,sizeof(*m));m->hardware_new=hardware_new!=0;m->assets_ready=ready!=0;
    m->row=ready?0:1;m->probe_count=probes>16?16:probes;
    snprintf(m->build,sizeof(m->build),"%.12s",build?build:"unknown");
}
static unsigned rows(const FhStartupMenu*m){return m->page==FH_STARTUP_HOME?(m->hardware_new?8:9):m->page==FH_STARTUP_PRESETS?FH_PRESET_COUNT:m->probe_count;}
unsigned fh_startup_menu_input(FhStartupMenu*m,unsigned buttons){
    if(m->busy)return buttons&FH_STARTUP_BACK?FH_STARTUP_CANCEL:FH_STARTUP_NONE;
    if(buttons&FH_STARTUP_BACK){if(m->page==FH_STARTUP_HOME)return FH_STARTUP_EXIT;m->page=FH_STARTUP_HOME;m->row=0;return FH_STARTUP_NONE;}
    unsigned count=rows(m);
    if(count && (buttons&(FH_STARTUP_UP|FH_STARTUP_DOWN)))m->row=(m->row+(buttons&FH_STARTUP_UP?count-1:1))%count;
    if(!(buttons&FH_STARTUP_ACCEPT))return FH_STARTUP_NONE;
    if(m->page==FH_STARTUP_PRESETS){
        if(!fh_graphics_preset_apply(m->row))return FH_STARTUP_NONE;
        m->page=FH_STARTUP_HOME;m->row=2;return FH_STARTUP_SAVE_SETTINGS;
    }
    if(m->page==FH_STARTUP_DIAGNOSTICS){
        if(!count)return FH_STARTUP_NONE;
        m->probe=m->row;m->busy=1;m->done=m->total=0;m->phase[0]=0;return FH_STARTUP_RUN_PROBE;
    }
    switch(m->row){
    case 0:if(m->assets_ready)return FH_STARTUP_PLAY;
        snprintf(m->message,sizeof(m->message),"Prepare assets before starting.");return FH_STARTUP_NONE;
    case 1:m->busy=1;m->done=m->total=0;m->phase[0]=0;return FH_STARTUP_PREPARE;
    case 2:m->page=FH_STARTUP_PRESETS;m->row=fh_graphics_preset_match(fh_graphics_options,fh_graphics_original_requested,fh_graphics_textures_requested);break;
    case 3:m->page=FH_STARTUP_DIAGNOSTICS;m->row=0;break;
    case 4:if(m->assets_ready)return FH_STARTUP_BENCHMARK;
        snprintf(m->message,sizeof(m->message),"Prepare assets before benchmarking.");break;
    case 5:fh_audio_bench_select(!fh_audio_bench_mode());break;
    case 6:
        if(m->hardware_new){fh_gpu_queue_select(!fh_gpu_queue_early());break;}
        fh_audio_bench_select_old_core(!fh_audio_bench_old_core());break;
    case 7:
        if(m->hardware_new)return FH_STARTUP_EXIT;
        fh_gpu_queue_select(!fh_gpu_queue_early());break;
    case 8:return FH_STARTUP_EXIT;
    }
    return FH_STARTUP_NONE;
}
int fh_startup_menu_progress(FhStartupMenu*m,const char*phase,uint32_t done,uint32_t total){
    if(!m->busy || !phase || done>total)return 0;
    snprintf(m->phase,sizeof(m->phase),"%.39s",phase);m->done=done;m->total=total;return 1;
}
void fh_startup_menu_finish(FhStartupMenu*m,unsigned ready,const char*message){
    m->busy=0;m->assets_ready=ready!=0;m->page=FH_STARTUP_HOME;m->row=ready?0:1;
    snprintf(m->message,sizeof(m->message),"%.127s",message?message:"");
}
static int append(char**out,size_t*left,const char*fmt,...){
    va_list args;va_start(args,fmt);int n=vsnprintf(*out,*left,fmt,args);va_end(args);
    if(n<0 || (size_t)n>=*left)return 0;
    *out+=n;*left-=n;return 1;
}
int fh_startup_menu_text(const FhStartupMenu*m,const char*const*probes,char*out,size_t capacity){
    if(!out || !capacity)return 0;
    char*p=out;size_t left=capacity;
#define LINE(...) do{if(!append(&p,&left,__VA_ARGS__))return 0;}while(0)
    LINE("Foxhollow - %s 3DS\nBuild %s\n\n",m->hardware_new?"New":"Old",m->build);
    if(m->busy){
        if(m->estimate.enabled){
            const FhInstallEta*e=&m->estimate;
            LINE("Overall (estimate): ~%u%%\n",fh_install_eta_percent(e));
            LINE("Elapsed: %llum %02llus\n",(unsigned long long)((e->last-e->started)/60000),(unsigned long long)((e->last-e->started)/1000%60));
            if(e->sampled){
                unsigned long long minutes=(fh_install_eta_remaining(e)+59999)/60000;
                LINE("ETA remaining: ~%llum (rough)\n",minutes?minutes:1);
            }else LINE("ETA remaining: calculating...\n");
            LINE("Later stages are estimated.\n\n");
        }
        LINE("%s\n\n",m->phase[0]?m->phase:"Starting...");
        if(m->total){
            unsigned cells=(unsigned)((uint64_t)m->done*20/m->total),percent=(unsigned)((uint64_t)m->done*100/m->total);
            char bar[21];for(unsigned i=0;i<20;i++)bar[i]=i<cells?'#':'-';bar[20]=0;
            LINE("Current task: %u%%\n[%s]\n%lu / %lu\n",percent,bar,(unsigned long)m->done,(unsigned long)m->total);
        }else LINE("Calculating work...\n");
        LINE("\nB: cancel safely\n");
    }else if(m->page==FH_STARTUP_HOME){
        static const char*labels[]={"Start game","Prepare / resume assets","Graphics preset","Diagnostics","Benchmarks / playback","Audio","Exit"};
        unsigned preset=fh_graphics_preset_match(fh_graphics_options,fh_graphics_original_requested,fh_graphics_textures_requested);
        LINE("Assets: %s\nPreset: %s\n\n",m->assets_ready?"Ready":"Preparation needed",fh_graphics_preset(preset)->name);
        unsigned gpu_row=m->hardware_new?6:7;
        for(unsigned i=0;i<rows(m);i++){
            if(!m->hardware_new && i==6){
                LINE("%c Audio worker: %s\n",i==m->row?'>':' ',
                    fh_audio_bench_old_core()?"CORE 1 (30%)":"CORE 0");continue;
            }
            if(i==gpu_row){LINE("%c GPU queue: %s\n",i==m->row?'>':' ',fh_gpu_queue_early()?"EARLY":"FRAME END");continue;}
            unsigned label=i+1==rows(m)?6:i;
            LINE("%c %s",i==m->row?'>':' ',labels[label]);
            if(i==5)LINE(": %s",fh_audio_bench_mode()?"DSP":"CPU");
            LINE("\n");
        }
        LINE("\nISO: /3ds/foxhollow/disc.iso\n\nA: choose   B: exit\n");
    }else if(m->page==FH_STARTUP_PRESETS){
        for(unsigned i=0;i<FH_PRESET_COUNT;i++)LINE("%c %s\n",i==m->row?'>':' ',fh_graphics_preset(i)->name);
        LINE("\nCustom keeps per-effect settings.\nDisplay counters stay unchanged.\n\nA: apply   B: back\n");
    }else{
        LINE("Diagnostics\n\n");
        for(unsigned i=0;i<m->probe_count;i++)LINE("%c %.30s\n",i==m->row?'>':' ',probes&&probes[i]?probes[i]:"Unnamed test");
        if(!m->probe_count)LINE("No tests in this build.\n");
        LINE("\nA: run   B: back\n");
    }
    if(!m->busy && m->message[0])LINE("\n%s\n",m->message);
    return 1;
#undef LINE
}
