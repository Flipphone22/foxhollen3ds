static void raster_channel_device_test(void) {
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;
    s.scissor[2]=s.scissor[3]=16;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;
    FhVertex v[6]={0};const unsigned corners[]={0,1,2,0,2,3};
    const float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
    for(unsigned i=0;i<6;i++) {
        unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        for(unsigned ch=0;ch<2;ch++)for(unsigned c=0;c<4;c++)v[i].color[ch][c]=(30+ch*100+c*30)/255.f;
    }
    unsigned checks=0,maximum=0;unsigned char copy[256];
    FILE *captures=fopen("sdmc:/3ds/foxhollow/raster-channels.bin","wb");
    if(!captures)fh_platform_panic("Raster channel capture open failed");
    for(unsigned channel=0;channel<6;channel++)for(unsigned swap=0;swap<4;swap++)
    for(unsigned mode=0;mode<2;mode++)for(unsigned alpha=0;alpha<2;alpha++) {
        for(unsigned c=0;c<4;c++)s.swaps[0][c]=(c+swap)&3;
        s.stages=mode+1;
        s.tev[0]=(FhTevStage){.color={15,15,15,alpha?11:10},.alpha={7,7,7,5},
            .channel=channel,.coord=255,.map=255,.color_clamp=1,.alpha_clamp=1};
        if(mode) {
            memcpy(s.tev[0].color,(unsigned[]){15,alpha?11:10,12,alpha?11:10},16);
            memcpy(s.tev[0].alpha,(unsigned[]){7,5,6,5},16);
        }
        s.tev[1]=(FhTevStage){.color={15,0,13,15},.alpha={7,0,6,7},.channel=255,.coord=255,.map=255,
            .color_clamp=1,.alpha_clamp=1,.kalpha=4};
        FhMaterial plan;
        if(mode) {
            if(fh_material_compile(&plan,&s) || !fh_pipeline_compile(&plan,&s))fh_platform_panic("Raster fixture must exercise PICA pipeline");
        } else if(!fh_material_compile(&plan,&s))fh_platform_panic("Raster fixture must exercise affine color");
        fh_renderer_draw(v,6,&s);uint32_t rect[]={0,0,16,16};
        fh_renderer_copy(copy,rect,8,8,6,1,0,0,0,&s);fwrite(copy,1,sizeof(copy),captures);
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned c=0;c<4;c++) {
            unsigned source=s.swaps[0][c==3 || alpha?3:c];float expected=30+(channel&1)*100+source*30;
            if(mode){expected*=2;if(expected>255)expected=255;expected*=.5f;}
            unsigned at=((y/4)*2+x/4)*64+((y%4)*4+x%4)*2,indices[]={at+1,at+32,at+33,at};
            unsigned error=abs((int)copy[indices[c]]-(int)(expected+.5f));if(error>maximum)maximum=error;
            if(error>2)fh_platform_panic("Raster alias or swapped-alpha mismatch");checks++;
        }
    }
    fclose(captures);FILE *file=fopen("sdmc:/3ds/foxhollow/raster-channels.txt","wb");
    if(!file)fh_platform_panic("Raster channel report open failed");fprintf(file,"%u %u\n",checks,maximum);fclose(file);fh_renderer_present();
}
