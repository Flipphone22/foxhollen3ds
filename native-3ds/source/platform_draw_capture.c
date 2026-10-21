#include "fh_draw_capture.h"
#include "fh_texture_dependencies.h"
#include "fh_gpu_lighting.h"
#include "fh_gpu_texgen.h"
#ifdef FH_DRAW_CAPTURE
#ifndef FH_EMULATOR_TEST
#error Draw capture is restricted to emulator development builds
#endif
#include "fh_platform.h"
#include <stdio.h>
#include <string.h>
static unsigned requested,minimum_vertices=96,requested_mode=2,minimum_ms;
static unsigned capture_limit=1,captured;
static uint64_t previous_ticks;
void fh_capture_draw_init(void) {
    requested=minimum_ms=captured=0;minimum_vertices=96;requested_mode=2;capture_limit=1;previous_ticks=0;
    FILE *f=fopen("sdmc:/3ds/foxhollow/capture_draw.txt","r");
    if(f){if(fscanf(f,"%u %u %u %u %u",&requested,&minimum_vertices,&requested_mode,&minimum_ms,&capture_limit)<1)requested=0;fclose(f);}
    /* Optional worst-draw discovery is emulator-only and bounded. Each new
     * capture must double the previous cost, avoiding repeated disk writes
     * for timing noise. Never silently enable it for fur/compiled geometry. */
    if(!capture_limit || capture_limit>12 || (capture_limit>1 && (requested_mode!=1 || !minimum_ms)))requested=0;
}
static void write_bytes(FILE *f,const void *data,size_t bytes) {
    if(bytes && fwrite(data,1,bytes,f)!=bytes)fh_platform_panic("Draw capture write failed");
}
static void capture_draw(unsigned frame,const FhVertex *vertices,unsigned count,const FhDrawState *state,int failure) {
    unsigned used=fh_texture_dependencies(state);
    uint32_t lengths[8][2]={{0}};unsigned total=0;
    for(unsigned i=0;i<8;i++)if(used&(1u<<i)){
        const FhTexture *t=&state->textures[i];
        lengths[i][0]=fh_texture_source_bytes(t->format,t->width,t->height);
        lengths[i][1]=t->palette?t->palette_count*2:0;
        if(!t->pixels || !lengths[i][0] || lengths[i][0]>4*1024*1024 || lengths[i][1]>32768)fh_platform_panic("Draw capture texture bounds");
        total+=lengths[i][0]+lengths[i][1];
    }
    if(total>16*1024*1024)fh_platform_panic("Draw capture storage bound");
    char temporary[96],destination[96];
    const char *stem=failure?"material-draw-capture":"draw-capture";
    if(!failure && captured){
        snprintf(temporary,sizeof(temporary),"sdmc:/3ds/foxhollow/%s-%02u-%u.tmp",stem,captured,frame);
        snprintf(destination,sizeof(destination),"sdmc:/3ds/foxhollow/%s-%02u-%u.bin",stem,captured,frame);
    }else{
        snprintf(temporary,sizeof(temporary),"sdmc:/3ds/foxhollow/%s.tmp",stem);
        snprintf(destination,sizeof(destination),"sdmc:/3ds/foxhollow/%s.bin",stem);
    }
    FILE *f=fopen(temporary,"wb");
    if(!f)fh_platform_panic("Draw capture open failed");
    const uint32_t header[]={frame,count,sizeof(*state),sizeof(*vertices)};
    write_bytes(f,state->raw_position&16?"FHDRAW04":state->raw_position&4?"FHDRAW03":state->raw_position&2?"FHDRAW02":"FHDRAW01",8);write_bytes(f,header,sizeof(header));
    FhDrawState snapshot=*state;
    for(unsigned i=0;i<8;i++)snapshot.textures[i].pixels=snapshot.textures[i].palette=NULL;
    write_bytes(f,&snapshot,sizeof(snapshot));write_bytes(f,vertices,count*sizeof(*vertices));
    if(state->raw_position&2)write_bytes(f,fh_renderer_prepared_lighting(),sizeof(FhGpuLighting));
    if(state->raw_position&4)write_bytes(f,fh_renderer_prepared_texgen(),48);
    for(unsigned i=0;i<8;i++){
        write_bytes(f,lengths[i],sizeof(lengths[i]));
        write_bytes(f,state->textures[i].pixels,lengths[i][0]);
        write_bytes(f,state->textures[i].palette,lengths[i][1]);
    }
    if(fclose(f) || rename(temporary,destination))fh_platform_panic("Draw capture commit failed");
    fh_platform_log("Captured private fragment draw for isolated graphics replay\n");
}
int fh_capture_draw_pending(unsigned frame,unsigned count,unsigned mode) {
    return requested && frame>=requested && mode==requested_mode && count>=minimum_vertices && count<=384;
}
void fh_capture_draw(unsigned frame,const FhVertex *vertices,unsigned count,const FhDrawState *state,unsigned mode) {
    if(minimum_ms || !fh_capture_draw_pending(frame,count,mode))return;
    requested=0;capture_draw(frame,vertices,count,state,0);
}
void fh_capture_draw_timed(unsigned frame,const FhVertex *vertices,unsigned count,const FhDrawState *state,unsigned mode,uint64_t ticks) {
    if(!minimum_ms || !fh_capture_draw_pending(frame,count,mode) || ticks*1000/268123480<minimum_ms)return;
    if(captured && ticks/2<previous_ticks)return;
    capture_draw(frame,vertices,count,state,0);
    FILE *index=fopen("sdmc:/3ds/foxhollow/draw-captures.csv",captured?"a":"w");
    if(!index)fh_platform_panic("Draw capture index open failed");
    if(!captured)fputs("capture,frame,vertices,ticks\n",index);
    int result=fprintf(index,"%u,%u,%u,%llu\n",captured,frame,count,(unsigned long long)ticks);
    if(fclose(index) || result<0)fh_platform_panic("Draw capture index write failed");
    previous_ticks=ticks;
    if(++captured>=capture_limit)requested=0;
}
void fh_capture_failed_draw(unsigned frame,const FhVertex *vertices,unsigned count,const FhDrawState *state) {
    if(!count || count>384 || count%3)return;
    capture_draw(frame,vertices,count,state,1);
}
#endif
