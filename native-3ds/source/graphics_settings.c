/* Startup/shutdown only. Two checksummed slots preserve the previous settings
 * if a write is interrupted; no filesystem activity in the touch/render loop.
 * The pinned Melee file owner model is retained: no new shared FILE or worker. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fh_graphics_options.h"
#include "fh_graphics_presets.h"

static unsigned settings_loaded,settings_slot,settings_serial;
static unsigned saved_mask,saved_display,saved_original,saved_textures,saved_effects;
static char settings_root[256];
static uint32_t word(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(unsigned char*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(8*i));}
static uint32_t checksum(const unsigned char*p,unsigned n){uint32_t c=2166136261u;for(unsigned i=0;i<n;i++)c=(c^p[i])*16777619u;return c;}
static unsigned display(void){return fh_graphics_fps_visible|(fh_graphics_perf_visible<<1)|(fh_graphics_gpu_visible<<2);}
static int path(char*out,unsigned slot){int n=snprintf(out,288,"%s/graphics-%u.cfg",settings_root,slot);return n>0&&n<288;}
static int read_slot(unsigned slot,unsigned*out){
    char name[288];unsigned char bytes[37];if(!path(name,slot))return 0;
    FILE*f=fopen(name,"rb");if(!f)return 0;
    size_t n=fread(bytes,1,sizeof(bytes),f);int failed=ferror(f);fclose(f);
    if(failed)return 0;
    unsigned old=n==28&&!memcmp(bytes,"FHGRAPH2",8);
    unsigned v3=n==32&&!memcmp(bytes,"FHGRAPH3",8);
    unsigned v4=n==32&&!memcmp(bytes,"FHGRAPH4",8);
    unsigned v5=n==36&&!memcmp(bytes,"FHGRAPH5",8);
    if(!old&&!v3&&!v4&&!v5)return 0;
    unsigned end=old?24:v5?32:28;if(word(bytes+end)!=checksum(bytes,end))return 0;
    out[0]=word(bytes+8);out[1]=word(bytes+12);out[2]=word(bytes+16);out[3]=word(bytes+20);
    out[4]=old?0:word(bytes+24);out[5]=v5?word(bytes+28):0;
    if((out[1]&~FH_GRAPHICS_VALID)||out[2]>(old?1u:7u)||out[3]>1||out[4]>1||(out[5]&~FH_EFFECT_VALID))return 0;
    /* Previously deployed versions did not offer contact shadows. Preserve
     * their choices while introducing the expensive new effect as opt-in.
     * Version 4 preserves either subsequent user choice across restarts. */
    if(old||v3)out[1]|=FH_GRAPHICS_NO_GROUND_SHADOWS;
    static const unsigned fancy[][2]={{0x07fb0001u,0},{0x9ffb0001u,0x1d},{0xfffb0001u,0x1d},{0xfffb0001u,0x3d}};
    const FhGraphicsPreset*preset=fh_graphics_preset(FH_PRESET_NEW_FANCY);
    for(unsigned i=0;i<sizeof fancy/sizeof*fancy;i++)
        if(out[1]==fancy[i][0]&&out[5]==fancy[i][1]&&out[3]==preset->original_geometry&&out[4]==preset->original_textures){out[1]=preset->mask;out[5]=preset->effects;}
    return 1;
}
void fh_graphics_settings_load(const char*root){
    fh_graphics_quarantine_reload();
    settings_loaded=0;if(!root||strlen(root)>=sizeof(settings_root))return;
    strcpy(settings_root,root);unsigned a[6],b[6];int va=read_slot(0,a),vb=read_slot(1,b);
    settings_slot=vb&&(!va||(int32_t)(b[0]-a[0])>0);unsigned*p=settings_slot?b:a;
    settings_serial=va||vb?p[0]:0;
    if(va||vb){fh_graphics_options_set(p[1]);fh_graphics_effects_set(p[5]);fh_graphics_fps_visible=p[2]&1;fh_graphics_perf_visible=(p[2]>>1)&1;fh_graphics_gpu_visible=(p[2]>>2)&1;fh_graphics_original_requested=p[3];fh_graphics_textures_requested=p[4];}
    saved_mask=fh_graphics_options;saved_display=display();saved_original=fh_graphics_original_requested;saved_textures=fh_graphics_textures_requested;saved_effects=fh_graphics_effects;settings_loaded=1;
}
int fh_graphics_settings_save(void){
    if(!settings_loaded||(saved_mask==fh_graphics_options&&saved_display==display()&&saved_original==fh_graphics_original_requested&&saved_textures==fh_graphics_textures_requested&&saved_effects==fh_graphics_effects))return 1;
    unsigned char bytes[36];char name[288];unsigned slot=settings_slot^1;
    memcpy(bytes,"FHGRAPH5",8);put(bytes+8,settings_serial+1);put(bytes+12,fh_graphics_options);
    put(bytes+16,display());put(bytes+20,fh_graphics_original_requested);put(bytes+24,fh_graphics_textures_requested);put(bytes+28,fh_graphics_effects);put(bytes+32,checksum(bytes,32));
    if(!path(name,slot))return 0;FILE*f=fopen(name,"wb");if(!f)return 0;
    int ok=fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes);if(fclose(f))ok=0;
    if(!ok)return 0;
    unsigned verify[6];if(!read_slot(slot,verify)||verify[0]!=settings_serial+1||verify[1]!=fh_graphics_options||verify[2]!=display()||verify[3]!=fh_graphics_original_requested||verify[4]!=fh_graphics_textures_requested||verify[5]!=fh_graphics_effects)return 0;
    settings_slot=slot;settings_serial++;saved_mask=fh_graphics_options;saved_display=display();saved_original=fh_graphics_original_requested;saved_textures=fh_graphics_textures_requested;saved_effects=fh_graphics_effects;return 1;
}
