#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fh_graphics_options.h"
#include "../native-3ds/source/graphics_settings.c"
#include "../native-3ds/source/graphics_presets.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#c);return 1;}}while(0)
unsigned fh_graphics_options=FH_GRAPHICS_DEFAULT,fh_graphics_fps_visible;
unsigned fh_graphics_original_requested,fh_graphics_original_active,fh_graphics_original_available;
unsigned fh_graphics_textures_requested,fh_graphics_textures_active;
unsigned fh_graphics_perf_visible,fh_graphics_gpu_visible;
unsigned fh_graphics_effects;
void fh_graphics_options_set(unsigned v){fh_graphics_options=v;}
void fh_graphics_effects_set(unsigned v){fh_graphics_effects=v;}
void fh_graphics_quarantine_reload(void){}
int main(int argc,char**argv){
    CHECK(argc==2);char name[288];fh_graphics_settings_load(argv[1]);
    CHECK(fh_graphics_options==FH_GRAPHICS_DEFAULT&&!fh_graphics_fps_visible&&settings_serial==0);
    CHECK(fh_graphics_settings_save()&&settings_serial==0); /* No unnecessary writes. */
    fh_graphics_options=FH_GRAPHICS_NO_WORLD_TEX|FH_GRAPHICS_PARTICLES;
    fh_graphics_fps_visible=1;CHECK(fh_graphics_settings_save()&&settings_serial==1);
    CHECK(fh_graphics_settings_save()&&settings_serial==1);
    fh_graphics_options=1;fh_graphics_fps_visible=0;fh_graphics_settings_load(argv[1]);
    CHECK(fh_graphics_options==65&&fh_graphics_fps_visible==1);
    fh_graphics_options=FH_GRAPHICS_VALID;fh_graphics_original_requested=1;CHECK(fh_graphics_settings_save()&&settings_serial==2);
    CHECK(fh_graphics_original_active==0); /* Pending choice cannot swap live geometry. */
    CHECK(path(name,settings_slot));FILE*f=fopen(name,"wb");CHECK(f);fputs("FHGRAPH1",f);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(settings_serial==1&&fh_graphics_options==65&&!fh_graphics_original_requested);
    /* A failed new write cannot overwrite the valid slot. */
    char old[256];strcpy(old,settings_root);strcat(settings_root,"/no-such-directory");
    fh_graphics_options=3;CHECK(!fh_graphics_settings_save()&&settings_serial==1);
    strcpy(settings_root,old);fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options==65);
    /* Serial wrapping still selects the newer valid slot. */
    settings_serial=0xfffffffeu;fh_graphics_options=7;CHECK(fh_graphics_settings_save());
    fh_graphics_options=9;CHECK(fh_graphics_settings_save());
    fh_graphics_settings_load(argv[1]);CHECK(settings_serial==0&&fh_graphics_options==9);
    /* Reserved old display bits and corrupt payloads are never accepted.
     * All32 graphics bits now have defined semantics, including wake sheets. */
    unsigned char bytes[28];memcpy(bytes,"FHGRAPH2",8);put(bytes+8,1);put(bytes+12,1);put(bytes+16,2);put(bytes+20,0);put(bytes+24,checksum(bytes,24));
    CHECK(path(name,settings_slot^1));f=fopen(name,"wb");CHECK(f);CHECK(fwrite(bytes,1,28,f)==28);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options==9);
    bytes[12]=1;f=fopen(name,"wb");CHECK(f);CHECK(fwrite(bytes,1,28,f)==28);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options==9);
    /* Migrate the deployed v2 settings without coupling geometry/textures. */
    put(bytes+12,1);put(bytes+16,1);put(bytes+20,1);put(bytes+24,checksum(bytes,24));
    f=fopen(name,"wb");CHECK(f);CHECK(fwrite(bytes,1,28,f)==28);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_original_requested&&!fh_graphics_textures_requested&&!fh_graphics_perf_visible&&!fh_graphics_gpu_visible);
    CHECK(fh_graphics_options&FH_GRAPHICS_NO_GROUND_SHADOWS);
    fh_graphics_textures_requested=1;fh_graphics_original_requested=0;fh_graphics_perf_visible=fh_graphics_gpu_visible=1;
    CHECK(fh_graphics_settings_save());fh_graphics_textures_requested=0;fh_graphics_perf_visible=0;
    fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_textures_requested&&!fh_graphics_original_requested&&fh_graphics_perf_visible&&fh_graphics_gpu_visible);
    CHECK(!fh_graphics_textures_active&&!fh_graphics_original_active);
    /* Deployed v3 slots also adopt opt-in shadows; a v4 explicit ON survives. */
    unsigned char v3[32];memcpy(v3,"FHGRAPH3",8);put(v3+8,settings_serial+1);
    put(v3+12,FH_GRAPHICS_PARTICLES|FH_GRAPHICS_NO_WORLD_TEX);put(v3+16,7);
    put(v3+20,1);put(v3+24,0);put(v3+28,checksum(v3,28));
    CHECK(path(name,settings_slot^1));f=fopen(name,"wb");CHECK(f);CHECK(fwrite(v3,1,32,f)==32);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options==(65|FH_GRAPHICS_NO_GROUND_SHADOWS));
    CHECK(fh_graphics_original_requested&&!fh_graphics_textures_requested&&display()==7);
    fh_graphics_options&=~FH_GRAPHICS_NO_GROUND_SHADOWS;CHECK(fh_graphics_settings_save());
    fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options==65);
    fh_graphics_options|=FH_GRAPHICS_SILHOUETTE_SHADOWS;CHECK(fh_graphics_settings_save());
    fh_graphics_options=FH_GRAPHICS_DEFAULT;fh_graphics_settings_load(argv[1]);
    CHECK(fh_graphics_options==(65|FH_GRAPHICS_SILHOUETTE_SHADOWS));
    fh_graphics_options|=FH_GRAPHICS_NO_MORPHS|FH_GRAPHICS_SPLASHES|FH_GRAPHICS_WATER_DROPS|FH_GRAPHICS_FUR|FH_GRAPHICS_SPECULAR|FH_GRAPHICS_SOURCE_FUR|FH_GRAPHICS_DIFFUSE_MAPS|FH_GRAPHICS_HEIGHT_FOG|FH_GRAPHICS_FUR_GROOM;
    CHECK(fh_graphics_settings_save());fh_graphics_options=FH_GRAPHICS_DEFAULT;
    fh_graphics_settings_load(argv[1]);
    CHECK(fh_graphics_options==(65|FH_GRAPHICS_SILHOUETTE_SHADOWS|FH_GRAPHICS_NO_MORPHS|FH_GRAPHICS_SPLASHES|FH_GRAPHICS_WATER_DROPS|FH_GRAPHICS_FUR|FH_GRAPHICS_SPECULAR|FH_GRAPHICS_SOURCE_FUR|FH_GRAPHICS_DIFFUSE_MAPS|FH_GRAPHICS_HEIGHT_FOG|FH_GRAPHICS_FUR_GROOM));
    fh_graphics_options|=FH_GRAPHICS_SPLASH_REFLECTION|FH_GRAPHICS_MOTION_BLUR|FH_GRAPHICS_BLUR_FILTER;
    CHECK(fh_graphics_settings_save());fh_graphics_options=0;
    fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options&FH_GRAPHICS_SPLASH_REFLECTION);
    CHECK(fh_graphics_options&FH_GRAPHICS_MOTION_BLUR);
    CHECK(fh_graphics_options&FH_GRAPHICS_BLUR_FILTER);
    fh_graphics_options|=FH_GRAPHICS_WAKES;CHECK(fh_graphics_settings_save());
    fh_graphics_options=0;fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options&FH_GRAPHICS_WAKES);
    CHECK(!fh_graphics_effects);fh_graphics_effects=FH_EFFECT_RIPPLES;
    CHECK(fh_graphics_settings_save());unsigned serial=settings_serial;
    CHECK(fh_graphics_settings_save()&&settings_serial==serial);
    fh_graphics_effects=0;fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_effects==FH_EFFECT_RIPPLES);
    /* Unknown extended bits with a valid checksum still fail closed. */
    unsigned char ext[37]={0};memcpy(ext,"FHGRAPH5",8);put(ext+8,serial+1);
    put(ext+12,1);put(ext+28,FH_EFFECT_VALID+1);put(ext+32,checksum(ext,32));
    CHECK(path(name,settings_slot^1));f=fopen(name,"wb");CHECK(f);CHECK(fwrite(ext,1,36,f)==36);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(settings_serial==serial&&fh_graphics_effects==FH_EFFECT_RIPPLES);
    put(ext+28,0);put(ext+32,checksum(ext,32));
    f=fopen(name,"wb");CHECK(f);CHECK(fwrite(ext,1,37,f)==37);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(settings_serial==serial&&fh_graphics_effects==FH_EFFECT_RIPPLES);
    /* v4 has no extra word: importing it must never inherit a live effect. */
    memcpy(ext,"FHGRAPH4",8);put(ext+28,checksum(ext,28));
    f=fopen(name,"wb");CHECK(f);CHECK(fwrite(ext,1,32,f)==32);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(settings_serial==serial+1&&!fh_graphics_effects&&fh_graphics_options==1);
    const unsigned legacy[][2]={{0x07fb0001u,0},{0x9ffb0001u,0x1d},{0xfffb0001u,0x1d},{0xfffb0001u,0x3d}};
    for(unsigned i=0;i<4;i++){
        memcpy(ext,"FHGRAPH5",8);serial=settings_serial;put(ext+8,serial+1);put(ext+12,legacy[i][0]);put(ext+16,0);put(ext+20,1);put(ext+24,1);put(ext+28,legacy[i][1]);put(ext+32,checksum(ext,32));
        CHECK(path(name,settings_slot^1));f=fopen(name,"wb");CHECK(f);CHECK(fwrite(ext,1,36,f)==36);fclose(f);
        fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options==fh_graphics_preset(FH_PRESET_NEW_FANCY)->mask&&fh_graphics_effects==fh_graphics_preset(FH_PRESET_NEW_FANCY)->effects);
        CHECK(!(fh_graphics_options&(FH_GRAPHICS_HEAT|FH_GRAPHICS_BLUR_FILTER))&&!(fh_graphics_effects&(FH_EFFECT_DISTORTION|FH_EFFECT_RIPPLES)));
        CHECK(fh_graphics_preset_match(fh_graphics_options,fh_graphics_original_requested,fh_graphics_textures_requested)==FH_PRESET_NEW_FANCY);
    }
    put(ext+20,0);put(ext+28,legacy[0][1]);put(ext+12,legacy[0][0]);put(ext+8,settings_serial+1);put(ext+32,checksum(ext,32));
    CHECK(path(name,settings_slot^1));f=fopen(name,"wb");CHECK(f);CHECK(fwrite(ext,1,36,f)==36);fclose(f);
    fh_graphics_settings_load(argv[1]);CHECK(fh_graphics_options==legacy[0][0]&&!fh_graphics_effects);
    puts("PASS: v2-v5 settings, effects migration/roundtrip, no-change/no-write, interrupted write recovery, serial wrap, bit31, unknown bits and checksum");
    return 0;
}
