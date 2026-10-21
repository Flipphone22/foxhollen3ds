#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "fh_gpu_work.h"
static u64 ticks;
static Result result;
static int immediate=-1;
static unsigned forwards,submits;
Result __wrap_gspSubmitGxCommand(const u32 command[8]);
void __wrap_gxCmdQueueInterrupt(GSPGPU_Event event);
u64 svcGetSystemTick(void){return ticks;}
Result __real_gspSubmitGxCommand(const u32 command[8]){
    assert(command[7]==0x12345678);++submits;
    if(immediate>=0)__wrap_gxCmdQueueInterrupt((GSPGPU_Event)immediate);
    return result;
}
void __real_gxCmdQueueInterrupt(GSPGPU_Event event){(void)event;++forwards;}
static void submit(unsigned kind){u32 cmd[8]={kind,0,0,0,0,0,0,0x12345678};assert(__wrap_gspSubmitGxCommand(cmd)==result);}
static FhGpuWork sample(void){FhGpuWork s;memset(&s,0xff,sizeof s);fh_gpu_work_snapshot(&s);assert(s.sampled_ticks==ticks);return s;}
int main(int argc,char **argv){
    if(argc==2 && !strcmp(argv[1],"failure")){
        ticks=10;result=-2;submit(1);ticks=15;FhGpuWork s=sample();
        assert(s.failed==1 && s.last_result==-2 && s.pending==1 && s.completed==0 && submits==1);
        puts("Rejected submission retained as invalid evidence; no retry or false completion");return 0;
    }
    ticks=10;submit(1);ticks=20;submit(4);ticks=25;
    FhGpuWork s=sample();assert(s.busy_ticks==15 && s.pending==2);
    __wrap_gxCmdQueueInterrupt(GSPGPU_EVENT_VBlank0);
    __wrap_gxCmdQueueInterrupt(GSPGPU_EVENT_PSC1);
    ticks=30;__wrap_gxCmdQueueInterrupt(GSPGPU_EVENT_P3D);
    ticks=40;s=sample();assert(s.busy_ticks==30 && s.pending==1);
    ticks=50;__wrap_gxCmdQueueInterrupt(GSPGPU_EVENT_PPF);
    ticks=70;s=sample();assert(s.busy_ticks==40 && s.pending==0 && s.completed==2);
    // Snapshot of an open span cannot double-count its eventual completion.
    ticks=80;submit(2);ticks=85;__wrap_gxCmdQueueInterrupt(GSPGPU_EVENT_P3D);
    s=sample();assert(s.unmatched==1 && s.pending==1);
    ticks=90;__wrap_gxCmdQueueInterrupt(GSPGPU_EVENT_PSC0);
    ticks=100;immediate=GSPGPU_EVENT_DMA;submit(0);immediate=-1;
    ticks=110;submit(5); // Cache-only operation has no completion IRQ.
    s=sample();assert(s.busy_ticks==50 && s.submitted==4 && s.completed==4 && s.pending==0);
    assert(s.command_count[5]==1 && s.event_count[GSPGPU_EVENT_PSC1]==1);
    assert(submits==5 && forwards==7 && !s.failed && !s.unsupported);
    puts("Overlap, open snapshots, idle gaps, IRQ classes, immediate completion and forwarding pass");
    return 0;
}
