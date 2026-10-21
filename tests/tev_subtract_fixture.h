static void tev_subtract_device_test(void){
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;
    s.scissor[2]=s.scissor[3]=16;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=s.texgens=3;
    unsigned char pixels[3][256],copy[256];
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
    for(unsigned i=0;i<6;i++){
        unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        for(unsigned t=0;t<3;t++)v[i].uv[t][2]=1;
    }
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s.swaps[i][c]=c;
    for(unsigned t=0;t<3;t++)s.textures[t]=(FhTexture){.pixels=pixels[t],.width=8,.height=8,.format=6};
    s.tev[0]=(FhTevStage){.color={15,8,10,15},.alpha={7,4,5,7},.color_clamp=1,.alpha_clamp=1,.channel=4};
    s.tev[1]=(FhTevStage){.color={15,15,15,0},.alpha={7,7,7,4},.color_clamp=1,.alpha_clamp=1,.alpha_dest=2,.coord=1,.map=1,.channel=255,.tex_swap=1};
    s.tev[2]=(FhTevStage){.color={15,15,15,0},.alpha={7,2,4,6},.alpha_op=1,.color_clamp=1,.alpha_clamp=1,.coord=2,.map=2,.channel=255,.tex_swap=1,.kalpha=28};
    unsigned checks=0,maximum=0;
    for(unsigned scenario=0;scenario<64;scenario++){
        unsigned color[3][4],raster[4];
        for(unsigned c=0;c<4;c++){
            raster[c]=(43+scenario*19+c*31)&255;
            for(unsigned t=0;t<3;t++)color[t][c]=(scenario*47+t*71+c*59)&255;
        }
        for(unsigned t=0;t<3;t++)for(unsigned k=0;k<64;k++){
            unsigned at=(k/16)*64+(k%16)*2;
            pixels[t][at]=color[t][3];pixels[t][at+1]=color[t][0];pixels[t][at+32]=color[t][1];pixels[t][at+33]=color[t][2];
        }
        for(unsigned i=0;i<6;i++)for(unsigned c=0;c<4;c++)v[i].color[0][c]=raster[c]/255.f;
        unsigned component=scenario%4,scale=scenario%3,k=(scenario*29)&255;
        s.swaps[1][3]=component;s.tev[2].alpha_scale=scale;s.konst_colors[0][3]=k/255.f;
        FhMaterial plan;if(!fh_pipeline_compile(&plan,&s) || plan.pipeline.count!=4)fh_platform_panic("Terminal subtract pipeline declined");
        fh_renderer_invalidate(pixels,sizeof(pixels));fh_renderer_draw(v,6,&s);
        const uint32_t rect[]={0,0,16,16};fh_renderer_copy(copy,rect,8,8,6,1,0,0,0,&s);
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned c=0;c<4;c++){
            float expected=c==3?(k-color[1][component]*color[2][component]/255.f)*(1u<<scale):color[0][c]*raster[c]/255.f;
            if(expected<0)expected=0;if(expected>255)expected=255;
            unsigned at=((y/4)*2+x/4)*64+((y&3)*4+(x&3))*2,indices[]={at+1,at+32,at+33,at};
            unsigned error=abs((int)copy[indices[c]]-(int)(expected+.5f));if(error>maximum)maximum=error;
            if(error>3)fh_platform_panic("Terminal alpha multiply-subtract pixel mismatch");checks++;
        }
    }
    /* Pause backdrop: primary alpha * (1-texture alpha), then konst.
     * Exercise native intensity alpha remapping, swapped RGBA and saturation. */
    s.stages=s.texgens=2;
    s.tev[0]=(FhTevStage){.color={15,15,15,15},.alpha={7,4,5,5},.alpha_op=1,.color_clamp=1,.alpha_clamp=1,.channel=4,.tex_swap=1};
    s.tev[1]=(FhTevStage){.color={2,15,15,8},.alpha={7,0,6,7},.color_clamp=1,.alpha_clamp=1,.coord=1,.map=1,.channel=255,.kalpha=28};
    for(unsigned scenario=0;scenario<64;scenario++){
        unsigned color[2][4],raster=(scenario*43+17)&255,k=(scenario*29+80)&255;
        unsigned intensity=scenario&1,component=(scenario/2)%4,scale=scenario%3;
        for(unsigned t=0;t<2;t++)for(unsigned c=0;c<4;c++)color[t][c]=(scenario*47+t*71+c*59)&255;
        for(unsigned t=0;t<2;t++)for(unsigned i=0;i<64;i++){
            unsigned at=(i/16)*64+(i%16)*2;
            pixels[t][at]=color[t][3];pixels[t][at+1]=color[t][0];pixels[t][at+32]=color[t][1];pixels[t][at+33]=color[t][2];
        }
        s.textures[0].format=intensity?1:6;
        if(intensity)memset(pixels[0],color[0][0],64);
        s.swaps[1][3]=component;s.konst_colors[0][3]=k/255.f;s.tev[0].alpha_scale=scale;
        for(unsigned c=0;c<3;c++)s.tev_colors[1][c]=((scenario*17+c*27)&255)/255.f;
        for(unsigned i=0;i<6;i++)v[i].color[0][3]=raster/255.f;
        FhMaterial plan;if(!fh_pipeline_compile(&plan,&s) || plan.pipeline.count!=2)fh_platform_panic("Factored subtract declined");
        fh_renderer_invalidate(pixels,sizeof(pixels));fh_renderer_draw(v,6,&s);
        const uint32_t rect[]={0,0,16,16};fh_renderer_copy(copy,rect,8,8,6,1,0,0,0,&s);
        float a=raster*(1-(intensity?color[0][0]:color[0][component])/255.f)*(1u<<scale);
        if(a>255)a=255;a*=k/255.f;
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned c=0;c<4;c++){
            float expected=c==3?a:color[1][c]+s.tev_colors[1][c]*255;
            if(expected>255)expected=255;
            unsigned at=((y/4)*2+x/4)*64+((y&3)*4+(x&3))*2,indices[]={at+1,at+32,at+33,at};
            unsigned error=abs((int)copy[indices[c]]-(int)(expected+.5f));if(error>maximum)maximum=error;
            if(error>3)fh_platform_panic("Factored subtract pixel mismatch");checks++;
        }
    }
    fh_renderer_invalidate(pixels,sizeof(pixels));fh_renderer_present();
    FILE*f=fopen("sdmc:/3ds/foxhollow/tev-subtract.txt","wb");if(!f)fh_platform_panic("Terminal subtract report");fprintf(f,"%u %u\n",checks,maximum);fclose(f);
}
