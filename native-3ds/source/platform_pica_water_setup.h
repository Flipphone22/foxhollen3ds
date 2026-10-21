/* Native command owner; shares the existing bounded scene snapshot. */
#include "fh_pica_water_sheet.h"
#include "pica_water_sheet_shader.h"
static DVLB_s *water_sheet_binary;
int fh_pica_water_sheet_prepare(void){
    if(water_sheet_binary)return 1;
    water_sheet_binary=DVLB_ParseFile((u32*)pica_water_sheet_shader,sizeof(pica_water_sheet_shader));
    if(!water_sheet_binary)return 0;
    fh_pica_water_sheet_init(water_sheet_binary->DVLE);return 1;
}
static void water_sheet_shutdown(void){
    fh_pica_water_sheet_shutdown();
    if(water_sheet_binary)DVLB_Free(water_sheet_binary);
    water_sheet_binary=NULL;
}
