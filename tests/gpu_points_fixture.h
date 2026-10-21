#include "fh_primitive.h"
static void gpu_points_device_test(void) {
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=64;s.viewport[5]=1;
    s.scissor[2]=s.scissor[3]=64;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=1;
    for(unsigned c=0;c<4;c++){s.swaps[0][c]=c;s.position_matrix[c][c]=1;}
    for(unsigned c=0;c<3;c++)s.model_palette[c][c]=1;
    const unsigned widths[]={0,1,6,12,63,96,192,255},corners[]={0,1,2,2,1,3};
    unsigned char reference[4096],actual[4096];unsigned checks=0,maximum=0;
    FILE *captures=fopen("sdmc:/3ds/foxhollow/gpu-points.bin","wb");if(!captures)fh_platform_panic("Point capture open");
    for(unsigned trial=0;trial<96;trial++)for(unsigned mode=0;mode<2;mode++){
        unsigned side=trial>=32?(trial-32)/16:0;
        s.viewport[0]=s.scissor[0]=trial>=32 && side==1?576:0;
        s.viewport[1]=s.scissor[1]=trial>=32 && side==3?416:0;
        FhDrawState clear=s;clear.texgens=0;clear.raw_position=0;
        clear.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.channel=4,.map=255,.coord=255,.color_clamp=1,.alpha_clamp=1};
        FhVertex background[6]={0};const float xy[4][2]={{-1,1},{-1,-1},{1,1},{1,-1}};
        for(unsigned i=0;i<6;i++){
            background[i].position[0]=xy[corners[i]][0];background[i].position[1]=xy[corners[i]][1];
            background[i].position[2]=-.5f;background[i].position[3]=1;background[i].matrix_index=-1;
            background[i].color[0][0]=.05f;background[i].color[0][1]=.1f;background[i].color[0][2]=.15f;background[i].color[0][3]=1;
        }
        fh_renderer_draw(background,6,&clear);
        s.texgens=trial&1;s.raw_position=trial<16?1:0;s.tev[0]=clear.tev[0];
        if(s.texgens){
            s.textures[0]=(FhTexture){.pixels=pixels[0],.width=6,.height=8,.format=0x46};
            s.tev[0]=(FhTevStage){.color={15,8,10,15},.alpha={7,4,5,7},.channel=4,.map=0,.coord=0,.color_clamp=1,.alpha_clamp=1};
        }
        FhVertex v={0};v.position[0]=trial%3?-.25f:-1.01f;v.position[1]=.125f;
        v.position[2]=trial/8==1?-1:trial/8==2?-1.01f:-.5f;v.position[3]=trial/8==3?-1:1;
        v.matrix_index=s.raw_position?0:-1;
        if(trial>=32){
            /* Move the capture viewport to each physical screen edge. Widths
             * up to42.5pixels must survive when their center is just outside;
             * distant centers exercise rejection with exactly the same shader. */
            float edge=(trial&8)?3.f:1.01f;
            v.position[0]=side==0?-edge:side==1?edge:0;
            v.position[1]=side==2?edge:side==3?-edge:0;
            v.position[2]=-.5f;v.position[3]=1;
            s.raw_position=trial&1;v.matrix_index=s.raw_position?0:-1;
        }
        v.color[0][0]=.8f;v.color[0][1]=.6f;v.color[0][2]=.4f;v.color[0][3]=1;
        v.uv[0][2]=trial&2?2:1;v.uv[0][0]=.1f*v.uv[0][2];v.uv[0][1]=.2f*v.uv[0][2];
        if(trial==7)v.uv[0][2]=0;
        if(!fh_renderer_points_available(&s))fh_platform_panic("Point fixture admission failed");
        if(mode)fh_renderer_draw_points(&v,1,&s,widths[trial%8],1,.5f);
        else {
            FhVertex q[4],triangles[6];FhDrawState cpu=s;cpu.raw_position=0;
            if(s.raw_position){fh_vertex_clip_position(v.position,v.position,0,&s);v.matrix_index=-1;}
            if(fh_expand_point(&v,widths[trial%8],1,.5f,64,64,q)){
                for(unsigned i=0;i<6;i++)triangles[i]=q[corners[i]];
                fh_renderer_draw(triangles,6,&cpu);
            }
        }
        uint32_t rect[]={s.scissor[0],s.scissor[1],64,64};fh_renderer_copy(mode?actual:reference,rect,32,32,6,1,0,0,0,&s);
        fwrite(mode?actual:reference,1,4096,captures);
        if(mode)for(unsigned i=0;i<4096;i++){
            unsigned error=abs((int)actual[i]-reference[i]);if(error>maximum)maximum=error;
            if(error>2){char message[120];snprintf(message,sizeof(message),"GPU point mismatch trial=%u byte=%u actual=%u reference=%u",trial,i,actual[i],reference[i]);fh_platform_panic(message);}checks++;
        }
    }
    fclose(captures);FILE *f=fopen("sdmc:/3ds/foxhollow/gpu-points.txt","wb");if(!f)fh_platform_panic("Point report open");
    fprintf(f,"%u %u\n",checks,maximum);fclose(f);
    /* Retained native centers must keep point size and model uniforms live.
     * A triangle replay with the same ID/count must not alias the GS buffer. */
    s.viewport[0]=s.viewport[1]=s.scissor[0]=s.scissor[1]=0;
    s.texgens=0;s.raw_position=9;
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.channel=4,.map=255,.coord=255,.color_clamp=1,.alpha_clamp=1};
    FhVertex center={0};center.position[0]=-.2f;center.position[1]=.1f;center.position[2]=-.5f;center.position[3]=1;
    center.color[0][0]=.8f;center.color[0][1]=.4f;center.color[0][2]=.2f;center.color[0][3]=1;
    unsigned hits_before,hits_after,n,b;fh_renderer_native_geometry_stats(&hits_before,&n,&b);
    checks=0;unsigned char discard[4096];uint32_t rect[]={0,0,64,64};
    const uint64_t id=0xfedc001234ULL;
    for(unsigned trial=0;trial<16;trial++){
        s.model_palette[0][3]=.025f*((int)trial-8);s.model_palette[1][3]=-.025f*((int)trial-8);
        s.tev[0].color[3]=2;s.tev[0].alpha[3]=1;
        s.tev_colors[1][0]=.05f*(trial+1);s.tev_colors[1][1]=.9f-.04f*trial;
        s.tev_colors[1][2]=.4f;s.tev_colors[1][3]=1;
        for(unsigned mode=0;mode<2;mode++){
            fh_renderer_copy(discard,rect,32,32,6,1,1,0x000000ff,0xffffff,&s);
            if(!mode)fh_renderer_draw_points(&center,1,&s,widths[trial%8],0,0);
            else if(!fh_renderer_replay_retained_points(id,1,&s,widths[trial%8],0,0))
                fh_renderer_draw_retained_points(id,&center,1,&s,widths[trial%8],0,0);
            fh_renderer_copy(mode?actual:reference,rect,32,32,6,1,0,0,0,&s);
        }
        for(unsigned i=0;i<4096;i++){if(reference[i]!=actual[i])fh_platform_panic("Retained point live state mismatch");checks++;}
        if(fh_renderer_replay_retained(id,1,&s))fh_platform_panic("Point cache aliased triangle topology");
    }
    fh_renderer_native_geometry_stats(&hits_after,&n,&b);
    if(hits_after-hits_before<12)fh_platform_panic("Retained points did not replay");
    f=fopen("sdmc:/3ds/foxhollow/native-points.txt","wb");if(!f)fh_platform_panic("Retained point report open");
    fprintf(f,"%u %u\n",checks,hits_after-hits_before);fclose(f);
}
