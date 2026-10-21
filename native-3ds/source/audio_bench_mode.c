#include "fh_audio_bench_mode.h"
static unsigned native_selected=1,latched,old_worker_core=1;
unsigned fh_audio_bench_mode(void){return native_selected;}
int fh_audio_bench_select(unsigned native_dsp){
    if(latched || native_dsp>1)return 0;
    native_selected=native_dsp;return 1;
}
unsigned fh_audio_bench_latch(void){latched=1;return native_selected;}

unsigned fh_audio_bench_old_core(void){return old_worker_core;}
int fh_audio_bench_select_old_core(unsigned core){
    if(latched || core>1)return 0;
    old_worker_core=core;return 1;
}
