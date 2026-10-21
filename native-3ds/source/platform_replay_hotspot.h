/* Opt-in hardware profiling, shared by replay and the gameplay diagnostic panel.
 * Register semantics:
 * https://www.3dbrew.org/wiki/GPU/External_Registers#Hotspot_Profiling
 * Eight stage labels are hypotheses, not utilization/cycle measurements.
 * Fixed 4096 x 1024-cycle windows; never read an unfinished FIFO, busy-wait,
 * touch framebuffer controls, or write SD during a measured interval. */
enum { HOTSPOT_SAMPLES=32, HOTSPOT_INTERVALS=4096, HOTSPOT_LENGTH=1024 };
typedef struct {
    u64 start,end,ipc_ticks;
    u32 frame,step,words[4],sum;
    Result result;
    unsigned valid;
} HotspotSample;
static HotspotSample hotspot_samples[HOTSPOT_SAMPLES];
static unsigned hotspot_requested,hotspot_owned,hotspot_pending,hotspot_count,hotspot_failed;
static u32 hotspot_saved[2];
static u64 hotspot_next,hotspot_ipc_ticks;
static Result hotspot_init_result,hotspot_restore_result;
static void hotspot_restore(void){
    if(!hotspot_owned)return;
    u32 zero=hotspot_saved[0]&~1u;
    Result a=GSPGPU_WriteHWRegs(0x400064,&zero,4);
    Result b=GSPGPU_WriteHWRegs(0x400068,hotspot_saved+1,4);
    hotspot_restore_result=R_FAILED(a)?a:b;
    hotspot_owned=hotspot_pending=0;
}
static void hotspot_poll(unsigned measured,unsigned frame,unsigned step){
    if(!hotspot_requested || hotspot_failed || hotspot_count==HOTSPOT_SAMPLES)return;
    u64 before=svcGetSystemTick();
    if(hotspot_pending){
        HotspotSample *s=hotspot_samples+hotspot_count;
        /* ARM system ticks and GPU clock share the documented 268111856Hz
         * base. Add 1ms beyond the fixed capture length before FIFO reads. */
        if(before-s->start < (u64)HOTSPOT_INTERVALS*HOTSPOT_LENGTH+268112)return;
        for(unsigned i=0;i<4;i++){
            Result r=GSPGPU_ReadHWRegs(0x40006c,s->words+i,4);
            if(R_FAILED(r)){s->result=r;hotspot_failed=1;break;}
            s->sum+=(s->words[i]&65535)+(s->words[i]>>16);
        }
        s->end=svcGetSystemTick();s->ipc_ticks+=s->end-before;
        hotspot_ipc_ticks+=s->end-before;
        s->valid=!hotspot_failed && s->sum==HOTSPOT_INTERVALS;
        /* Unsupported emulators/permissions must not produce valid counters. */
        if(!s->valid)hotspot_failed=1;
        hotspot_pending=0;hotspot_count++;
        if(hotspot_failed)hotspot_restore();
        return;
    }
    if(!measured || before<hotspot_next)return;
    if(!hotspot_owned){
        hotspot_init_result=GSPGPU_ReadHWRegs(0x400064,hotspot_saved,8);
        if(R_FAILED(hotspot_init_result) || (hotspot_saved[0]&1)){
            hotspot_failed=1;hotspot_ipc_ticks+=svcGetSystemTick()-before;return;
        }
        hotspot_owned=1;
    }
    HotspotSample *s=hotspot_samples+hotspot_count;
    *s=(HotspotSample){.frame=frame,.step=step};
    u32 interval=((HOTSPOT_LENGTH-1)<<16)|HOTSPOT_INTERVALS,one=hotspot_saved[0]|1u;
    s->result=GSPGPU_WriteHWRegs(0x400068,&interval,4);
    if(R_SUCCEEDED(s->result))s->result=GSPGPU_WriteHWRegs(0x400064,&one,4);
    s->start=svcGetSystemTick();s->ipc_ticks=s->start-before;hotspot_ipc_ticks+=s->ipc_ticks;
    if(R_FAILED(s->result)){hotspot_failed=1;hotspot_count++;hotspot_restore();return;}
    hotspot_pending=1;hotspot_next=s->start+268111856/2; /* At most twice/sec. */
}
#ifndef FH_HOTSPOT_NO_FILE_OUTPUT
static int hotspot_finish(const char *directory){
    if(!hotspot_requested)return 1;
    /* No extra waiting at shutdown. Mark a premature final window invalid. */
    hotspot_poll(0,0,0);
    if(hotspot_pending){hotspot_samples[hotspot_count++].end=svcGetSystemTick();}
    hotspot_restore();
    char path[256];snprintf(path,sizeof(path),"%s/gpu-hotspot.json",directory);
    FILE *f=fopen(path,"wb");if(!f)return 0;
    fprintf(f,"{\"schema\":1,\"requested\":true,\"failed\":%u,\"init_result\":%ld,\"restore_result\":%ld,\"saved_control\":%lu,\"saved_interval\":%lu,\"ipc_ticks\":%llu,\"intervals\":4096,\"interval_cycles\":1024,\"stage_labels_tentative\":true,\"samples\":[",
        hotspot_failed,(long)hotspot_init_result,(long)hotspot_restore_result,
        (unsigned long)hotspot_saved[0],(unsigned long)hotspot_saved[1],(unsigned long long)hotspot_ipc_ticks);
    for(unsigned i=0;i<hotspot_count;i++){
        HotspotSample *s=hotspot_samples+i;
        fprintf(f,"%s{\"start\":%llu,\"end\":%llu,\"frame\":%lu,\"step\":%lu,\"ipc_ticks\":%llu,\"result\":%ld,\"valid\":%u,\"sum\":%lu,\"words\":[%lu,%lu,%lu,%lu]}",
            i?",":"",(unsigned long long)s->start,(unsigned long long)s->end,
            (unsigned long)s->frame,(unsigned long)s->step,(unsigned long long)s->ipc_ticks,
            (long)s->result,s->valid,(unsigned long)s->sum,(unsigned long)s->words[0],
            (unsigned long)s->words[1],(unsigned long)s->words[2],(unsigned long)s->words[3]);
    }
    fputs("]}\n",f);int ok=!ferror(f);if(fclose(f))ok=0;return ok;
}
#endif
