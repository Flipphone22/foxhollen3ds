#include "fh_pica_common_shader.h"
#include "fh_platform.h"
#include "pica_common_shader.h"
#include <stdio.h>
static DVLB_s *binary[2];
static unsigned refs[FH_COMMON_COUNT],total[2];
DVLE_s *fh_pica_common_shader_acquire(unsigned entry){
    if(entry>=FH_COMMON_COUNT || refs[entry]>=16)fh_platform_panic("PICA common shader acquire");
    unsigned bank=entry>=FH_COMMON_HUD,index=bank?entry-FH_COMMON_HUD:entry;
    if(!binary[bank]){
        unsigned size=bank?sizeof(pica_common_aux_shader):sizeof(pica_common_shader);
        binary[bank]=DVLB_ParseFile((u32*)(bank?pica_common_aux_shader:pica_common_shader),size);
        if(!binary[bank] || binary[bank]->numDVLE!=(bank?5:6) || binary[bank]->DVLP.codeSize!=(bank?129:500))
            fh_platform_panic("PICA common shader contract");
        char line[128];snprintf(line,sizeof line,"PICA common shader: bank=%u entries=%u instructions=%u bytes=%u shared_DVLP=1\n",
            bank,(unsigned)binary[bank]->numDVLE,(unsigned)binary[bank]->DVLP.codeSize,size);fh_platform_log(line);
    }
    refs[entry]++;total[bank]++;return binary[bank]->DVLE+index;
}
void fh_pica_common_shader_release(unsigned entry){
    if(entry>=FH_COMMON_COUNT)fh_platform_panic("PICA common shader release range");
    unsigned bank=entry>=FH_COMMON_HUD;
    if(!refs[entry] || !total[bank] || !binary[bank])fh_platform_panic("PICA common shader release");
    refs[entry]--;total[bank]--;
    if(!total[bank]){DVLB_Free(binary[bank]);binary[bank]=NULL;}
}
unsigned fh_pica_common_shader_instructions(void){return binary[0]?binary[0]->DVLP.codeSize:0;}
unsigned fh_pica_common_shader_bytes(void){return sizeof(pica_common_shader);}
