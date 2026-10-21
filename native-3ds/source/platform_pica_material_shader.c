#include "fh_pica_material_shader.h"
#include "fh_platform.h"
#include "pica_material_shader.h"
static DVLB_s *binary;
static shaderProgram_s programs[FH_MATERIAL_SHADER_COUNT];
static unsigned acquired;
shaderProgram_s *fh_pica_material_shader_acquire(unsigned entry){
    if(entry>=FH_MATERIAL_SHADER_COUNT || (acquired&(1u<<entry)))fh_platform_panic("PICA material shader ownership");
    if(!binary){
        binary=DVLB_ParseFile((u32*)pica_material_shader,sizeof(pica_material_shader));
        if(!binary || binary->numDVLE!=FH_MATERIAL_SHADER_COUNT || binary->DVLP.codeSize>=512)
            fh_platform_panic("PICA material shader bank");
    }
    shaderProgramInit(programs+entry);
    if(shaderProgramSetVsh(programs+entry,binary->DVLE+entry)<0)fh_platform_panic("PICA material shader entry");
    acquired|=1u<<entry;return programs+entry;
}
void fh_pica_material_shader_release(unsigned entry){
    if(entry>=FH_MATERIAL_SHADER_COUNT || !(acquired&(1u<<entry)))fh_platform_panic("PICA material shader release");
    shaderProgramFree(programs+entry);acquired&=~(1u<<entry);
    if(!acquired){DVLB_Free(binary);binary=NULL;}
}
unsigned fh_pica_material_shader_instructions(void){return binary?binary->DVLP.codeSize:0;}
