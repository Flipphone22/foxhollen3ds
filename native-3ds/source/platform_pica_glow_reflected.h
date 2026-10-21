/* Native reflection program: same packed instances, separate VSH/GS binary.
 * Current opt-in approximation omits indirect RGB distortion. A8 mask filtering
 * has its separately measured quantization error; do not claim source parity. */
#include "pica_glow_reflected_shader.h"
#include "fh_pica_glow_reflection_material.h"
#include "fh_pica_texture_coordinates.h"
extern float gCameraLightPerspectiveFlipYMatrix[3][4];
static DVLB_s *glow_reflected_binary;
static shaderProgram_s glow_reflected_program[2];
static C3D_TexEnv glow_reflected_material[2][6];
static C3D_Tex glow_reflected_sampler;
static float glow_reflected_matrix[3][4];
static unsigned bound_reflected,glow_reflected_matrix_dirty=1;
static unsigned glow_reflected_draws,glow_reflected_missing;

static void glow_reflected_init(void){
    glow_reflected_binary=DVLB_ParseFile((u32*)pica_glow_reflected_shader,sizeof(pica_glow_reflected_shader));
    if(!glow_reflected_binary || glow_reflected_binary->numDVLE!=3 ||
       glow_reflected_binary->DVLP.codeSize>512)fh_platform_panic("Reflected particle shader");
    for(unsigned i=0;i<2;i++){
        shaderProgram_s *p=glow_reflected_program+i;
        shaderProgramInit(p);shaderProgramSetVsh(p,glow_reflected_binary->DVLE+(i?2:0));
        shaderProgramSetGsh(p,glow_reflected_binary->DVLE+1,i?7:4);
        const char *names[]={"templates","shapes","fade","uv_scale","reflected","dynamic_quad"};
        const int loc[]={7,15,19,20,21,104};
        for(unsigned n=0;n<6;n++)if(shaderInstanceGetUniformLocation(p->geometryShader,names[n])!=loc[n])
            fh_platform_panic("Reflected particle uniform ABI");
        shaderInstanceSetBool(p->geometryShader,0,i!=0);
    }
    fh_pica_glow_reflection_material_prepare(glow_reflected_material);
}
static void glow_reflected_bind(unsigned mode,const C3D_Tex *mask,const C3D_Tex *capture){
    for(unsigned i=0;i<6;i++)*C3D_GetTexEnv(i)=glow_reflected_material[(mode>>3)&1][i];
    glow_reflected_sampler=*capture;
    glow_reflected_sampler.param=(glow_reflected_sampler.param&~(7u<<28))|GPU_TEXTURE_MODE(GPU_TEX_PROJECTION);
    /* SDK bind retains the descriptor pointer; it does not modify its fields. */
    C3D_TexBind(0,&glow_reflected_sampler);C3D_TexBind(1,(C3D_Tex*)mask);C3D_TexBind(2,NULL);
    fh_pica_texture2_coord1(0);
    /* Publishing all material stages retired the previous fog overlay. */
    fh_pica_fog_invalidate();
}
static void glow_reflected_shutdown(void){
    shaderProgramFree(glow_reflected_program);shaderProgramFree(glow_reflected_program+1);
    if(glow_reflected_binary)DVLB_Free(glow_reflected_binary);
    glow_reflected_binary=NULL;
    char line[160];snprintf(line,sizeof(line),"PICA reflected particles: draws=%u missing=%u indirect=0\n",
        glow_reflected_draws,glow_reflected_missing);fh_platform_log(line);
}
