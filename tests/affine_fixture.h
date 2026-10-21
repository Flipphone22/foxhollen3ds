#include "fh_material.h"
static void affine_device_test(void) {
    extern unsigned fh_affine_reference,fh_affine_vertices;
    unsigned char reference[64*32*4],actual[64*32*4];
    unsigned checks=0,maximum=0,start_vertices=fh_affine_vertices;
    FhDrawState s={0};s.viewport[2]=128;s.viewport[3]=64;s.viewport[5]=1;
    s.scissor[2]=128;s.scissor[3]=64;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=2;
    for(unsigned t=0;t<4;t++)for(unsigned c=0;c<4;c++)s.swaps[t][c]=c;
    s.tev[0]=(FhTevStage){.color={15,10,14,15},.alpha={7,5,6,7},.color_clamp=1,.alpha_clamp=1,.channel=4,.map=255,.coord=255,.kcolor=6,.kalpha=6};
    s.tev[1]=(FhTevStage){.color={15,10,14,0},.alpha={7,5,6,0},.color_clamp=1,.alpha_clamp=1,.channel=5,.map=255,.coord=255,.kcolor=4,.kalpha=4};
    FhVertex v[6]={0},batch[384];const unsigned corners[]={0,1,2,0,2,3};
    const float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
    for(unsigned scenario=0;scenario<4;scenario++) {
        s.tev[1].color_scale=scenario&1?3:0;s.tev[1].alpha_scale=scenario&2?3:0;
        s.tev[1].color_op=s.tev[1].alpha_op=scenario==2;
        s.tev[1].color_bias=s.tev[1].alpha_bias=scenario==2;
        s.raw_position=scenario==2;
        memset(s.position_matrix,0,sizeof(s.position_matrix));memset(s.model_palette,0,sizeof(s.model_palette));
        for(unsigned c=0;c<4;c++)s.position_matrix[c][c]=1;
        for(unsigned c=0;c<3;c++)s.model_palette[c][c]=1;
        for(unsigned i=0;i<6;i++) {
            unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
            for(unsigned c=0;c<8;c++)((float*)v[i].color)[c]=((k*29+c*37+scenario*11)%251)/250.f;
        }
        FhMaterial plan;
        if(!fh_material_compile(&plan,&s) || plan.pipeline.count || plan.sphere_lighting)fh_platform_panic("Affine fixture did not compile to affine material");
        for(unsigned i=0;i<384;i++)batch[i]=v[i%6];
        for(unsigned route=0;route<2;route++) {
            fh_affine_reference=!route;
            unsigned before=fh_affine_vertices;
            unsigned repeat=scenario==3?86:1,count=scenario==3?384:6;
            for(unsigned i=0;i<repeat;i++)fh_renderer_draw(batch,count,&s);
            if(route && fh_affine_vertices-before!=count*repeat)fh_platform_panic("Affine GPU route was not exercised");
            if(!route && fh_affine_vertices!=before)fh_platform_panic("Affine reference unexpectedly used GPU arithmetic");
            uint32_t rect[]={0,0,128,64};
            fh_renderer_copy(route?actual:reference,rect,64,32,6,1,0,0,0,&s);
            if(route)for(unsigned i=0;i<sizeof(actual);i++) {
                unsigned error=abs((int)actual[i]-reference[i]);if(error>maximum)maximum=error;
                if(error>2)fh_platform_panic("Affine GPU color differs from CPU reference");checks++;
            }
            char path[120];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/affine-%u-%u.bin",scenario,route);
            FILE *file=fopen(path,"wb");if(!file || fwrite(route?actual:reference,1,sizeof(actual),file)!=sizeof(actual))fh_platform_panic("Affine capture write failed");fclose(file);
            fh_renderer_present();
        }
    }
    fh_affine_reference=0;
    FILE *file=fopen("sdmc:/3ds/foxhollow/affine.txt","wb");if(!file)fh_platform_panic("Affine report open failed");
    fprintf(file,"%u %u %u\n",checks,maximum,fh_affine_vertices-start_vertices);fclose(file);
}
