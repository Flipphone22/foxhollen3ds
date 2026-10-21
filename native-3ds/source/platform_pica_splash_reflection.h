/* Included only by the persistent native splash owner. Immutable geometry and
 * source pose/alpha remain shared. No old expanded-vertex renderer is called. */
#include "pica_splash_reflection_shader.h"
#include "fh_pica_texture_coordinates.h"
#include "fh_pica_layered.h"
extern C3D_Tex *fh_pica_splash_capture(void);
typedef struct {uint8_t r,g,b,a;} SplashSourceColor;
extern SplashSourceColor gWaterReflectionKColorR,gWaterReflectionKColorG,gWaterReflectionKColorB;
extern float gCameraLightPerspectiveFlipYMatrix[3][4];
static DVLB_s *splash_reflected_binary;
static shaderProgram_s splash_reflected_program;
static C3D_Tex splash_reflected_sampler;
static unsigned splash_reflected_draws,splash_reflected_fog_draws;
static shaderProgram_s *splash_reflection_program(void){
    if(!splash_reflected_binary){
        splash_reflected_binary=DVLB_ParseFile((u32*)pica_splash_reflection_shader,sizeof(pica_splash_reflection_shader));
        if(!splash_reflected_binary || splash_reflected_binary->numDVLE!=1 ||
           splash_reflected_binary->DVLP.codeSize>64)fh_platform_panic("Splash reflection shader");
        shaderProgramInit(&splash_reflected_program);
        shaderProgramSetVsh(&splash_reflected_program,splash_reflected_binary->DVLE);
        const char *names[]={"projection","view","bands","colors","uvscale","reflected","fog_distance_scale"};
        const unsigned locations[]={0,4,7,31,39,41,94};
        for(unsigned i=0;i<7;i++)if(shaderInstanceGetUniformLocation(splash_reflected_program.vertexShader,names[i])!=(int)locations[i])
            fh_platform_panic("Splash reflection uniform ABI");
    }
    return &splash_reflected_program;
}
static uint32_t splash_pack(SplashSourceColor c){return c.r|(c.g<<8)|(c.b<<16)|((uint32_t)c.a<<24);}
static void splash_reflection_bind(C3D_Tex *capture,uint32_t low){
    splash_reflected_sampler=*capture;
    splash_reflected_sampler.param=(splash_reflected_sampler.param&~(7u<<28))|GPU_TEXTURE_MODE(GPU_TEX_PROJECTION);
    C3D_TexBind(0,&splash_reflected_sampler);C3D_TexBind(1,samplers);C3D_TexBind(2,samplers+1);
    fh_pica_texture2_coord1(0);
    /* Three source contributions, with their live RGB constants, use the same
     * projected sample until native indirect distortion is implemented. */
    const uint32_t weights[3]={splash_pack(gWaterReflectionKColorR),splash_pack(gWaterReflectionKColorG),splash_pack(gWaterReflectionKColorB)};
    splash_material_publish(2,low,weights);
    /* Existing shared fragment-light owner restores ordinary illumination on
     * handoff. Slot 3 is deliberately fog-only, so no original lights are lost.
     * Stage 5 preserves material alpha and all three water texture coordinates. */
    C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    float parameters[2];unsigned fog=fh_pica_fog_layered_bind(1,parameters);
    if(!fh_pica_environment_layered_bind(0,0,fog,parameters))
        fh_platform_panic("Splash fragment fog admission");
    splash_reflected_fog_draws+=fog?2:0;
    for(unsigned r=0;r<3;r++){
        const float *m=gCameraLightPerspectiveFlipYMatrix[r];
        C3D_FVUnifSet(GPU_VERTEX_SHADER,41+r,m[0],m[1],m[2],m[3]);
    }
    splash_reflected_draws+=2;
}
static void splash_reflection_shutdown(void){
    if(splash_reflected_binary){shaderProgramFree(&splash_reflected_program);DVLB_Free(splash_reflected_binary);splash_reflected_binary=NULL;}
    char line[160];snprintf(line,sizeof(line),"PICA splash reflection: draws=%u distortion=0 distance_fog=%u cpu_vertices=0\n",splash_reflected_draws,splash_reflected_fog_draws);fh_platform_log(line);
}
