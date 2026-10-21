#include "fh_install_bindings.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "fh_graphics_options.h"
#include "fh_hardware.h"
#include "fh_asset_paths.h"
#include "fh_platform.h"
#include "fh_heap_profile.h"
unsigned fh_hardware_is_new,fh_hardware_large_memory;
static unsigned private_replay_assets;
static const char*original_paths[2][3]={
    {"sdmc:/3ds/foxhollow/original/model-lod.bin","sdmc:/3ds/foxhollow/original/native-scene.bin","sdmc:/3ds/foxhollow/original/native-scene.cache"},
    {"sdmc:/3ds/foxhollow-replay/assets/original/model-lod.bin","sdmc:/3ds/foxhollow-replay/assets/original/native-scene.bin","sdmc:/3ds/foxhollow-replay/assets/original/native-scene.cache"}};
static const char*original_path(unsigned kind){
    const char*installed=private_replay_assets?NULL:fh_install_bindings_asset(kind,1);
    return installed?installed:original_paths[private_replay_assets][kind];
}
void fh_graphics_geometry_init(void){
    /* Original topology must be the explicitly empty patch set. Native scene
     * packets still undergo the existing exact source identity/CRC checks. */
    unsigned char h[16];FILE*f=fopen(original_path(0),"rb");
    unsigned ok=f&&fread(h,1,16,f)==16&&!memcmp(h,"FHMLPK1\0\0\0\0\0\0\0\0\0",16)&&fgetc(f)==EOF;
    if(f)fclose(f);
    if(ok){f=fopen(original_path(1),"rb");
        ok=f&&fread(h,1,16,f)==16&&!memcmp(h,"FHNSSET1",8);
        if(f)fclose(f);
    }
    fh_graphics_original_available=ok;
    if(fh_graphics_original_requested&&!ok)fh_platform_panic("Original geometry files missing or invalid");
    fh_graphics_original_active=fh_graphics_original_requested;
    fh_graphics_textures_active=fh_graphics_textures_requested;
    fh_platform_log(fh_graphics_original_active?"Geometry profile: ORIGINAL (native PICA)\n":"Geometry profile: PERF (native PICA)\n");
    fh_platform_log(fh_graphics_textures_active?"Texture profile: ORIGINAL RESOLUTION (PICA formats)\n":"Texture profile: PERF (PICA formats)\n");
}
const char* fh_asset_path(unsigned kind){
    static const char* paths[2][4]={
        {"sdmc:/3ds/foxhollow/model-lod.bin","sdmc:/3ds/foxhollow/native-scene.bin",
         "sdmc:/3ds/foxhollow/native-scene.cache","sdmc:/3ds/foxhollow/texture-pack.bin"},
        {"sdmc:/3ds/foxhollow-replay/assets/model-lod.bin","sdmc:/3ds/foxhollow-replay/assets/native-scene.bin",
         "sdmc:/3ds/foxhollow-replay/assets/native-scene.cache","sdmc:/3ds/foxhollow-replay/assets/texture-pack.bin"}};
    if(kind==FH_ASSET_ACTOR_FOG){
        const char*installed=private_replay_assets?NULL:fh_install_bindings_asset(kind,fh_graphics_original_active);
        if(installed)return installed;
        static const char*fog_paths[2][2]={
            {"sdmc:/3ds/foxhollow/actor-fog/collection.bin","sdmc:/3ds/foxhollow/original/actor-fog/collection.bin"},
            {"sdmc:/3ds/foxhollow-replay/assets/actor-fog/collection.bin","sdmc:/3ds/foxhollow-replay/assets/original/actor-fog/collection.bin"}};
        return fog_paths[private_replay_assets][!!fh_graphics_original_active];
    }
    if(kind>FH_ASSET_ACTOR_FOG)fh_platform_panic("Invalid prepared asset path");
    const char*installed=private_replay_assets?NULL:fh_install_bindings_asset(kind,fh_graphics_original_active);
    if(installed)return installed;
    if(fh_graphics_original_active&&kind<3)return original_path(kind);
    return paths[private_replay_assets][kind];
}
extern u32 __ctru_heap_size,__ctru_linear_heap_size;
void __real___system_allocateHeaps(void);
/* Wrap the SDK allocator rather than replacing its mapping/cleanup contract.
 * APT/SRV and malloc are not available at this boundary: only kernel queries. */
void __wrap___system_allocateHeaps(void){
    Handle resource;ResourceLimitType type=RESLIMIT_COMMIT;
    s64 maximum=0,current=0;
    FhHeapProfile profile={46,6,0};
    if(R_SUCCEEDED(svcGetResourceLimit(&resource,CUR_PROCESS_HANDLE))){
        Result a=svcGetResourceLimitLimitValues(&maximum,resource,&type,1);
        Result b=svcGetResourceLimitCurrentValues(&current,resource,&type,1);
        svcCloseHandle(resource);
        if(R_SUCCEEDED(a) && R_SUCCEEDED(b))profile=fh_heap_profile(maximum,current);
    }
    fh_hardware_large_memory=profile.large;
    __ctru_heap_size=profile.normal_mib*1024*1024;
    __ctru_linear_heap_size=profile.linear_mib*1024*1024;
    __real___system_allocateHeaps();
}
void fh_hardware_init(void){
    bool model=false;
    if(R_FAILED(APT_CheckNew3DS(&model)))fh_platform_panic("Console model query failed");
    fh_hardware_is_new=model;
    extern unsigned fh_audio_shared_executor_enabled;
    if(!model)fh_audio_shared_executor_enabled=0;
    if(!model && fh_hardware_large_memory)fh_platform_panic("Unexpected Old 3DS memory entitlement");
#if defined(FH_REPLAY_BUILD) && !defined(FH_UNIFIED_APP)
    FILE* marker=fopen("sdmc:/3ds/foxhollow-replay/assets/enabled.txt","r");
    if(marker){private_replay_assets=fgetc(marker)=='1';fclose(marker);}
#endif
}
