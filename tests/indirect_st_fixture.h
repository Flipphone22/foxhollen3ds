#include "fh_fragment.h"
#include "fh_draw_capture.h"
static void indirect_st_device_test(void) {
    unsigned char noise[8*8*4],light[8*8*4],base[8*8*4],copy[8*8*4];
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++) {
        unsigned at=(y*8+x)*4;
        memcpy(noise+at,(unsigned char[]){0,128,127,130},4);
        memcpy(base+at,(unsigned char[]){200,160,80,192},4);
        light[at]=x*31;light[at+1]=y*29;light[at+2]=x*11+y*13;light[at+3]=255;
    }
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;
    s.scissor[2]=s.scissor[3]=16;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=4;s.texgens=5;s.indirect_count=1;
    s.indirect_order[0][0]=2;s.indirect_exponent[0]=7;
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s.swaps[i][c]=c;
    for(unsigned i=0;i<3;i++)s.textures[i]=(FhTexture){.pixels=i==0?noise:i==1?light:base,.width=8,.height=8,.format=0x46,.wrap_s=1,.wrap_t=1};
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.color_clamp=1,.alpha_clamp=1,.coord=0,.map=257,.channel=255,.indirect={0,0,3,5,6,6,0,0,0}};
    s.tev[1]=(FhTevStage){.color={15,15,15,0},.alpha={7,7,7,0},.color_clamp=1,.alpha_clamp=1,.coord=1,.map=257,.channel=255,.tex_swap=3,.indirect={0,0,3,9,6,6,1,0,0}};
    s.tev[2]=(FhTevStage){.color={15,8,14,10},.alpha={7,7,7,7},.color_clamp=1,.alpha_clamp=1,.color_dest=2,.coord=2,.map=1,.channel=4,.indirect={0,0,0,0,0,0,1,0,0}};
    s.tev[3]=(FhTevStage){.color={15,8,4,6},.alpha={7,4,6,7},.color_clamp=1,.alpha_clamp=1,.coord=4,.map=2,.channel=255};
    FhVertex v[6]={0};const unsigned corners[]={0,1,2,0,2,3};const float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++) {
        unsigned k=corners[i];float x=xy[k][0],y=xy[k][1];
        v[i].position[0]=2*x-1;v[i].position[1]=1-2*y;v[i].position[2]=-.5f;v[i].position[3]=1;
        for(unsigned t=0;t<8;t++)v[i].uv[t][2]=1;
        v[i].uv[0][0]=.25f;v[i].uv[1][1]=.25f;
        v[i].uv[2][0]=v[i].uv[4][0]=x;v[i].uv[2][1]=v[i].uv[4][1]=y;
    }
    if(!fh_fragment_supported(&s))fh_platform_panic("Dynamic S/T material rejected");
#ifdef FH_DRAW_CAPTURE
    fh_capture_failed_draw(0,v,6,&s);
#endif
    fh_renderer_draw(v,6,&s);
    uint32_t rect[]={0,0,16,16};fh_renderer_copy(copy,rect,8,8,6,1,0,0,0,&s);
    FILE *file=fopen("sdmc:/3ds/foxhollow/indirect-st.bin","wb");if(!file)fh_platform_panic("S/T capture open failed");fwrite(copy,1,sizeof(copy),file);fclose(file);
    unsigned checks=0,maximum=0;
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++) {
        unsigned tx=(x+2)&7,ty=(y+7)&7,at=((y/4)*2+x/4)*64+((y%4)*4+x%4)*2;
        unsigned indices[]={at+1,at+32,at+33,at};
        unsigned expected[]={((tx*31)*200+127)/255,((ty*29)*160+127)/255,((tx*11+ty*13)*80+127)/255,192};
        for(unsigned c=0;c<4;c++) {
            unsigned error=abs((int)copy[indices[c]]-(int)expected[c]);if(error>maximum)maximum=error;
            if(error>2)fh_platform_panic("Dynamic S/T rendered sample mismatch");checks++;
        }
    }
    file=fopen("sdmc:/3ds/foxhollow/indirect-st.txt","wb");if(!file)fh_platform_panic("S/T report open failed");fprintf(file,"%u %u\n",checks,maximum);fclose(file);
    fh_renderer_present();
}
