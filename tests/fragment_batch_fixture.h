/* Same CPU material, shaded vertices, order and framebuffer before/after
 * submission batching. Covers tiny triangles, overlap, clipping, varied Q,
 * alpha/depth masks, fog and atlas wrap. No timing-based correctness gate. */
static void fragment_batch_device_test(void){
    extern unsigned fh_fragment_batch_enabled,fh_fragment_batch_submissions;
    unsigned old_batch=fh_fragment_batch_enabled,old_reflect=fh_reflective_disabled;
    unsigned checks=0,draws[2]={0,0};
    unsigned char tex[3][256];
    unsigned char *copy[2]={malloc(320*240*4),malloc(320*240*4)};
    if(!copy[0] || !copy[1])fh_platform_panic("Fragment comparison allocation");
    FhDrawState s={0};s.viewport[2]=640;s.viewport[3]=480;s.viewport[5]=1;s.scissor[2]=640;s.scissor[3]=480;
    s.color_write=s.alpha_write=s.depth_test=s.depth_write=1;s.depth_compare=3;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=s.texgens=5;
    const unsigned colors[5][4]={{15,8,10,15},{15,14,8,15},{15,2,8,0},{15,14,8,15},{15,2,8,0}};
    const unsigned alpha[5][4]={{7,4,5,7},{7,7,7,0},{7,7,7,0},{7,7,7,0},{7,7,7,0}};
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s.swaps[i][c]=c;
    for(unsigned i=0;i<5;i++){
        s.tev[i]=(FhTevStage){.color_clamp=1,.alpha_clamp=1,.map=(i+1)/2,.coord=i,.color_dest=(i==1 || i==3),.channel=4,.kcolor=i==3?13:12};
        memcpy(s.tev[i].color,colors[i],16);memcpy(s.tev[i].alpha,alpha[i],16);
    }
    for(unsigned t=0;t<3;t++){
        for(unsigned i=0;i<256;i++)tex[t][i]=13+(i*17+t*37)%230;
        s.textures[t]=(FhTexture){.pixels=tex[t],.width=8,.height=8,.format=0x46,.min_filter=1,.mag_filter=1};
    }
    for(unsigned k=0;k<2;k++)for(unsigned c=0;c<3;c++)s.konst_colors[k][c]=.2f+.17f*((k+c)%4);
    FhVertex v[384];
    for(unsigned scenario=0;scenario<8;scenario++){
        s.fog_type=scenario==4?2:0;s.fog_near=1;s.fog_far=100;s.fog_start=1;s.fog_end=80;s.fog_color=0x395b71ff;
        s.alpha_write=scenario!=5;s.color_write=scenario!=6;
        s.blend_mode=scenario==3;s.blend_src=4;s.blend_dst=5;
        for(unsigned tri=0;tri<128;tri++)for(unsigned corner=0;corner<3;corner++){
            FhVertex *p=v+tri*3+corner;memset(p,0,sizeof(*p));p->matrix_index=-1;
            float w=1+.2f*corner;
            float x=(tri%16)*38.f+(corner==2?10.f:0),y=(tri/16)*55.f+(corner==1?10.f:0);
            if(scenario==2){x=(tri%4)*6.f+(corner==2?14.f:0);y=(tri%3)*6.f+(corner==1?14.f:0);}
            if(scenario==7)x-=18;
            p->position[0]=(x/320-1)*w;p->position[1]=(1-y/240)*w;p->position[2]=(-.2f-.1f*(tri%5))*w;p->position[3]=w;
            for(unsigned c=0;c<4;c++)p->color[0][c]=.2f+.1f*((tri+corner+c)%7);
            for(unsigned t=0;t<5;t++){p->uv[t][0]=.1f+.17f*((tri+t+corner)%5);p->uv[t][1]=.15f+.12f*((tri+t+corner*2)%6);p->uv[t][2]=1+.25f*corner;}
        }
        for(unsigned route=0;route<2;route++){
            fh_fragment_batch_enabled=route;fh_reflective_disabled=scenario!=1;
            /* Start the same atlas placement and framebuffer generation for
             * both routes; absolute atlas UVs are float24 on PICA. */
            fh_renderer_present();fh_renderer_invalidate(NULL,0);
            fh_renderer_clear(0x20304077,0xffffff);
            unsigned before=fh_fragment_batch_submissions;
            for(unsigned repeat=0;repeat<(scenario==7?36u:1u);repeat++)fh_renderer_draw(v,384,&s);
            draws[route]+=fh_fragment_batch_submissions-before;
            const uint32_t rect[]={0,0,640,480};fh_renderer_copy(copy[route],rect,320,240,6,1,0,0,0,&s);
        }
        for(unsigned i=0;i<320*240*4;i++){
            if(copy[0][i]!=copy[1][i]){char msg[100];snprintf(msg,sizeof(msg),"Fragment batch image differs scenario%u byte%u: %u/%u",scenario,i,copy[0][i],copy[1][i]);fh_platform_panic(msg);}
            checks++;
        }
    }
    if(!draws[1] || draws[1]*2>=draws[0])fh_platform_panic("Fragment fixture did not batch submissions");
    fh_renderer_sync();fh_renderer_invalidate(tex,sizeof(tex));fh_fragment_batch_enabled=old_batch;fh_reflective_disabled=old_reflect;
    free(copy[0]);free(copy[1]);
    FILE *f=fopen("sdmc:/3ds/foxhollow/fragment-batch.txt","wb");if(!f)fh_platform_panic("Fragment batch report");
    fprintf(f,"%u %u %u\n",checks,draws[0],draws[1]);fclose(f);
}
