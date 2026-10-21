#include "fh_draw_capture.h"
#include "fh_gpu_lighting.h"
#include "fh_gpu_texgen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
void fh_platform_panic(const char *s){fprintf(stderr,"%s\n",s);abort();}
void fh_platform_log(const char *s){(void)s;}
size_t fh_texture_source_bytes(unsigned f,unsigned w,unsigned h){(void)f;(void)w;(void)h;return 0;}
const FhGpuLighting *fh_renderer_prepared_lighting(void){abort();}
const float (*fh_renderer_prepared_texgen(void))[4]{abort();}
static FhVertex vertices[3];static FhDrawState state;
static void setup(const char *name,const char *config){
    assert(!mkdir(name,0700));assert(!chdir(name));
    assert(!mkdir("sdmc:",0700));assert(!mkdir("sdmc:/3ds",0700));
    assert(!mkdir("sdmc:/3ds/foxhollow",0700));
    FILE *f=fopen("sdmc:/3ds/foxhollow/capture_draw.txt","w");assert(f);
    assert(fputs(config,f)>=0);assert(!fclose(f));fh_capture_draw_init();
}
static void check(const char *name,unsigned frame){
    char path[128];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/%s",name);
    FILE *f=fopen(path,"rb");assert(f);char magic[8];unsigned h[4];
    assert(fread(magic,1,8,f)==8 && !memcmp(magic,"FHDRAW01",8));
    assert(fread(h,4,4,f)==4);assert(h[0]==frame && h[1]==3 && h[2]==sizeof(state) && h[3]==sizeof(vertices[0]));
    assert(!fseek(f,0,SEEK_END));assert(ftell(f)==24+sizeof(state)+sizeof(vertices)+64);assert(!fclose(f));
}
int main(void){
    const uint64_t t=268123480/100+1;
    setup("worst","50 3 1 10 3\n");
    assert(!fh_capture_draw_pending(49,3,1));assert(!fh_capture_draw_pending(50,3,2));
    assert(!fh_capture_draw_pending(50,2,1));assert(fh_capture_draw_pending(50,3,1));
    fh_capture_draw_timed(50,vertices,3,&state,1,t-2);
    assert(access("sdmc:/3ds/foxhollow/draw-capture.bin",F_OK));
    fh_capture_draw_timed(50,vertices,3,&state,1,t);check("draw-capture.bin",50);
    fh_capture_draw_timed(51,vertices,3,&state,1,2*t-1);
    assert(access("sdmc:/3ds/foxhollow/draw-capture-01-51.bin",F_OK));
    fh_capture_draw_timed(52,vertices,3,&state,1,2*t);check("draw-capture-01-52.bin",52);
    fh_capture_draw_timed(53,vertices,3,&state,1,4*t);check("draw-capture-02-53.bin",53);
    check("draw-capture.bin",50);assert(!fh_capture_draw_pending(54,3,1));
    FILE *f=fopen("sdmc:/3ds/foxhollow/draw-captures.csv","r");assert(f);
    char line[128];unsigned lines=0;while(fgets(line,sizeof(line),f))lines++;assert(lines==4);fclose(f);
    assert(!chdir(".."));setup("legacy","70 3 1 10\n");
    fh_capture_draw_timed(70,vertices,3,&state,1,t);check("draw-capture.bin",70);
    assert(!fh_capture_draw_pending(71,3,1));
    assert(!chdir(".."));setup("invalid","70 3 2 10 12\n");
    assert(!fh_capture_draw_pending(70,3,2));
    puts("PASS: legacy single capture; doubled-cost selection; retained file/header/index identity; capture limit; invalid mode rejection");
}
