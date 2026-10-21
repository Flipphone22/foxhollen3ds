#include "fh_light_workload.h"
static void sphere_lighting_device_test(void){
    FILE *f=fopen("sdmc:/3ds/foxhollow/lighting-workload.bin","rb");
    FhLightWorkload work;
    if(!f || fread(&work,sizeof(work),1,f)!=1 || memcmp(work.magic,"FHLIGHT1",8) || work.count!=128)fh_platform_panic("Sphere lighting workload");
    FhGpuLighting light={0};for(unsigned c=0;c<3;c++)light.normal[c][c]=1;
    for(unsigned i=0;i<6;i++){
        const float *src=work.lights[i];float (*dst)[4]=light.lights[i];
        memcpy(dst[0],src+6,12);memcpy(dst[1],src+9,12);memcpy(dst[2],src+12,16);
        memcpy(dst[3],src,12);memcpy(dst[4],src+3,12);
    }
    memcpy(light.lights[6],work.ambient,32);memcpy(light.lights[6]+2,work.material,32);
    if(!fh_gpu_sphere_validate(&light))fh_platform_panic("Sphere workload not eligible");
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;s.scissor[2]=s.scissor[3]=16;
    s.color_write=s.alpha_write=1;s.pixel_format=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=2;s.texgens=1;
    for(unsigned c=0;c<4;c++)s.swaps[0][c]=c;
    for(unsigned c=0;c<3;c++)s.model_palette[c][c]=1;
    s.position_matrix[0][0]=s.position_matrix[1][1]=100;s.position_matrix[2][3]=-.5f;s.position_matrix[3][3]=1;
    s.textures[0]=(FhTexture){.pixels=pixels[0],.width=8,.height=8,.format=0x46};
    for(unsigned i=0;i<2;i++)s.tev[i]=(FhTevStage){.color={15,10,11,i?0:8},.alpha={7,7,7,7},.color_clamp=1,.alpha_clamp=1,.channel=4+i};
    fh_gpu_sphere_disabled=0;
    const float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};const unsigned corners[]={0,1,2,0,2,3};
    unsigned checks=0,maximum=0,changed=0,nonzero=0;float scalar_error=0;
    unsigned char first_image[256];
    FILE *images=fopen("sdmc:/3ds/foxhollow/sphere-images.bin","wb");if(!images)fh_platform_panic("Sphere image report");
    for(unsigned sample=0;sample<work.count;sample++){
        FhLightSample captured;if(fread(&captured,sizeof(captured),1,f)!=1)fh_platform_panic("Short sphere workload");
        FhVertex probe={0};memcpy(probe.position,captured.eye,12);probe.position[3]=1;memcpy(probe.uv[7],captured.normal,12);
        fh_gpu_sphere_resolve(&probe,&s,&light);
        for(unsigned c=0;c<8;c++){
            float error=fabsf(((float*)probe.color)[c]-((float*)captured.output)[c]);if(error>scalar_error)scalar_error=error;
            if(error>1e-6f)fh_platform_panic("Sphere scalar differs from captured lighting");
        }
        /* The captured specular and alpha directions oppose one another;
         * their product can be identically black. Isolate all four channel
         * terms as well, retaining the original captured light arithmetic. */
        for(unsigned mode=0;mode<5;mode++){
        FhGpuLighting isolated=light;
        if(mode){
            unsigned channel=(mode==2 || mode==4),other=1-channel;
            memset(isolated.lights[6][other+2],0,16);
            if(mode<=2){isolated.lights[6][channel][3]=1;isolated.lights[6][channel+2][3]=1;}
            else for(unsigned c=0;c<3;c++){isolated.lights[6][channel][c]=1;isolated.lights[6][channel+2][c]=1;}
        }
        FhVertex input[6]={0},resolved[6];
        s.position_matrix[0][3]=-captured.eye[0]*100;s.position_matrix[1][3]=-captured.eye[1]*100;
        for(unsigned i=0;i<6;i++){
            input[i]=probe;memcpy(input[i].color,captured.input,32);
            input[i].position[0]+=xy[corners[i]][0]*.01f;input[i].position[1]+=xy[corners[i]][1]*.01f;
            input[i].uv[0][0]=input[i].uv[0][1]=.5f;input[i].uv[0][2]=1;
            resolved[i]=input[i];fh_gpu_sphere_resolve(resolved+i,&s,&isolated);
        }
        unsigned char reference[256],actual[256];uint32_t rect[]={0,0,16,16};
        s.raw_position=1;fh_renderer_draw(resolved,6,&s);fh_renderer_copy(reference,rect,8,8,6,1,0,0,0,&s);
        s.raw_position=67;if(!fh_renderer_prepare_lighting(&s,&isolated))fh_platform_panic("Sphere GPU lighting declined");
        fh_renderer_draw(input,6,&s);fh_renderer_copy(actual,rect,8,8,6,1,0,0,0,&s);
        for(unsigned i=0;i<256;i++){
            unsigned error=abs((int)reference[i]-actual[i]);if(error>maximum)maximum=error;checks++;
            if(!sample && !mode)first_image[i]=reference[i];else changed+=reference[i]!=first_image[i];
            nonzero+=reference[i]!=0;
        }
        fwrite(reference,1,256,images);fwrite(actual,1,256,images);
        if(sample==127 && mode==4){
            fh_renderer_copy(actual,rect,8,8,6,1,1,0x112233ff,0x800000,&s);
            fh_renderer_copy(actual,rect,8,8,6,1,0,0,0,&s);
            if(actual[0]!=255 || actual[1]!=17 || actual[32]!=34 || actual[33]!=51)
                fh_platform_panic("Sphere shader leaked into EFB clear");
        }
        }
    }
    fclose(images);fclose(f);f=fopen("sdmc:/3ds/foxhollow/sphere-lighting.txt","wb");
    if(!f)fh_platform_panic("Sphere lighting report");
    fprintf(f,"%u %u %.9g %u %u %u\n",checks,maximum,scalar_error,fh_gpu_sphere_draws,changed,nonzero);fclose(f);
    if(changed<256 || nonzero<256)fh_platform_panic("Sphere fixture lacks visible lighting variation");
    if(maximum>3)fh_platform_panic("Sphere GPU lighting differs from scalar");
}
