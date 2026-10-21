static void efb_shadow_device_test(void) {
    unsigned char *expected=malloc(32768),*copies[10];unsigned checks=0;
    unsigned char reference[4096],actual[4096];
    if(!expected)fh_platform_panic("Shadow fixture reference allocation");
    for(unsigned i=0;i<10;i++){copies[i]=malloc(32768);if(!copies[i])fh_platform_panic("Shadow fixture destination allocation");}
    FhDrawState s={0};s.viewport[2]=640;s.viewport[3]=528;s.viewport[5]=1;s.scissor[2]=640;s.scissor[3]=528;
    s.color_write=s.alpha_write=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=1;
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.color_clamp=1,.alpha_clamp=1,.channel=4};
    for(unsigned t=0;t<4;t++)for(unsigned c=0;c<4;c++)s.swaps[t][c]=c;
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        unsigned k=corners[i];v[i].position[0]=2*xy[k][0]-1;v[i].position[1]=1-2*xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        v[i].uv[0][0]=xy[k][0];v[i].uv[0][1]=xy[k][1];v[i].uv[0][2]=1;
    }
    for(unsigned trial=0;trial<12;trial++) {
        unsigned w=8u<<(trial%6),bytes=w*w/2;uint32_t rect[]={0,0,2*w,2*w};
        s.viewport[2]=640;s.viewport[3]=528;s.scissor[2]=640;s.scissor[3]=528;
        s.texgens=0;s.tev[0].color[3]=10;s.tev[0].alpha[3]=5;
        for(unsigned i=0;i<6;i++)for(unsigned c=0;c<4;c++)v[i].color[0][c]=((corners[i]*61+trial*13+c*37)&255)/255.f;
        fh_renderer_draw(v,6,&s);fh_renderer_copy(expected,rect,w,w,0x20,1,0,0,0,&s);
        memset(copies[0],0xcd,bytes);
        if(!fh_renderer_copy_gpu_shadow(copies[0],rect,w,w,0x20,1,trial/6,0x183048ff,0xffffff,&s))fh_platform_panic("Shadow copy not admitted");
        for(unsigned i=0;i<bytes;i++){if(copies[0][i]!=0xcd)fh_platform_panic("Shadow copy was not deferred");checks++;}
        s.viewport[2]=s.viewport[3]=s.scissor[2]=s.scissor[3]=64;
        s.texgens=1;s.tev[0].color[3]=8;s.tev[0].alpha[3]=4;
        for(unsigned i=0;i<6;i++)for(unsigned c=0;c<4;c++)v[i].color[0][c]=1;
        for(unsigned route=0;route<2;route++) {
            s.textures[0]=(FhTexture){.pixels=route?copies[0]:expected,.width=w,.height=w,.format=0,.min_filter=1,.mag_filter=1};
            fh_renderer_invalidate(expected,bytes);fh_renderer_draw(v,6,&s);
            uint32_t sample[]={0,0,64,64};fh_renderer_copy(route?actual:reference,sample,32,32,6,1,0,0,0,&s);
        }
        for(unsigned i=0;i<sizeof(actual);i++){if(actual[i]!=reference[i])fh_platform_panic("GPU R4 shadow sampling differs");checks++;}
        if(trial&1)fh_renderer_cache_read(copies[0]+bytes/2,1);
        else fh_renderer_materialize_texture(copies[0]);
        for(unsigned i=0;i<bytes;i++){if(copies[0][i]!=expected[i])fh_platform_panic("GPU R4 shadow materialization differs");checks++;}
        memset(copies[0],0,bytes);fh_renderer_invalidate(copies[0],bytes);fh_renderer_materialize_texture(copies[0]);
        for(unsigned i=0;i<bytes;i++){if(copies[0][i])fh_platform_panic("GPU R4 shadow resurrected overwritten CPU content");checks++;}
        fh_renderer_present();
    }
    /* More live destinations than fit the VRAM budget. Eviction must restore
     * exact CPU bytes before reusing a slot, including queued texture reads. */
    s.viewport[2]=640;s.viewport[3]=528;s.scissor[2]=640;s.scissor[3]=528;
    s.texgens=0;s.tev[0].color[3]=10;s.tev[0].alpha[3]=5;
    fh_renderer_draw(v,6,&s);uint32_t rect[]={0,0,512,512};
    fh_renderer_copy(expected,rect,256,256,0x20,1,0,0,0,&s);
    for(unsigned i=0;i<10;i++) {
        memset(copies[i],0xcd,32768);
        if(!fh_renderer_copy_gpu_shadow(copies[i],rect,256,256,0x20,1,0,0,0,&s))fh_platform_panic("Shadow eviction fixture fallback");
    }
    for(unsigned i=0;i<10;i++) {
        fh_renderer_materialize_texture(copies[i]);
        for(unsigned j=0;j<32768;j++){if(copies[i][j]!=expected[j])fh_platform_panic("Shadow eviction lost image bytes");checks++;}
        fh_renderer_invalidate(copies[i],32768);free(copies[i]);
    }
    fh_renderer_invalidate(expected,32768);free(expected);
    extern unsigned fh_shadow_copies,fh_shadow_materializations,fh_shadow_evictions,fh_shadow_fallbacks;
    FILE*f=fopen("sdmc:/3ds/foxhollow/efb-shadow.txt","wb");if(!f)fh_platform_panic("Shadow fixture report");
    fprintf(f,"%u %u %u %u %u\n",checks,fh_shadow_copies,fh_shadow_materializations,fh_shadow_evictions,fh_shadow_fallbacks);fclose(f);
}
