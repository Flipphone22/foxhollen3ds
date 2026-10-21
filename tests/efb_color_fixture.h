static void efb_depth_device_test(void){
    enum { BYTES=320*240 };
    unsigned char *expected=malloc(BYTES),*deferred=malloc(BYTES);
    unsigned char *image=malloc(BYTES*4),*reference=malloc(BYTES*4);
    if(!expected || !deferred || !image || !reference)fh_platform_panic("Deferred depth fixture allocation");
    FhDrawState s={0};s.viewport[2]=640;s.viewport[3]=480;s.viewport[5]=1;s.scissor[2]=640;s.scissor[3]=480;
    s.color_write=s.alpha_write=s.depth_write=s.depth_test=1;s.depth_compare=7;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=1;
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.color_clamp=1,.alpha_clamp=1,.channel=4};
    for(unsigned t=0;t<4;t++)for(unsigned c=0;c<4;c++)s.swaps[t][c]=c;
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        unsigned k=corners[i];v[i].position[0]=2*xy[k][0]-1;v[i].position[1]=1-2*xy[k][1];v[i].position[3]=1;
        for(unsigned c=0;c<4;c++)v[i].color[0][c]=1;
        v[i].uv[0][0]=xy[k][0];v[i].uv[0][1]=xy[k][1];v[i].uv[0][2]=1;
    }
    unsigned checks=0,filtered_differences=0,filtered_max_error=0;uint32_t rect[]={0,0,640,480};
#ifdef FH_EFB_DEPTH_VERIFIED
    /* A perfectly flat image cannot qualify the GPU path, even if every
     * sampled byte matches. The following varied image must resolve it. */
    extern unsigned fh_renderer_depth_gpu_admission(void);
    for(unsigned flat=0;flat<12;flat++){
    s.texgens=0;s.tev[0].color[3]=10;s.tev[0].alpha[3]=5;
    for(unsigned i=0;i<6;i++)v[i].position[2]=-.5f;
    fh_renderer_draw(v,6,&s);
    memset(deferred,0xcd,BYTES);
    if(!fh_renderer_copy_gpu_depth(deferred,rect,320,240,0x11,1,0))fh_platform_panic("Blank depth copy not admitted");
    s.texgens=1;s.tev[0].color[3]=8;s.tev[0].alpha[3]=4;
    s.textures[0]=(FhTexture){.pixels=deferred,.width=320,.height=240,.format=1};
    fh_renderer_draw(v,6,&s);
    if(fh_renderer_depth_gpu_admission()!=0)fh_platform_panic("Flat depth image incorrectly resolved GPU support");
    fh_renderer_present();
    }
    FILE *blank=fopen("sdmc:/3ds/foxhollow/depth-admission-blank.txt","wb");
    if(!blank)fh_platform_panic("Blank depth admission report");
    fprintf(blank,"%u\n",fh_renderer_depth_gpu_admission());fclose(blank);
    /* The first varied CPU fallback wakes admission; it cannot itself certify
     * the GPU. The following ordinary sample must run the complete comparison. */
    s.texgens=0;s.tev[0].color[3]=10;s.tev[0].alpha[3]=5;
    for(unsigned i=0;i<6;i++)v[i].position[2]=-.85371f+.1f*corners[i];
    fh_renderer_draw(v,6,&s);
    if(!fh_renderer_copy_gpu_depth(deferred,rect,320,240,0x11,1,0))fh_platform_panic("Varied depth copy not admitted");
    s.texgens=1;s.tev[0].color[3]=8;s.tev[0].alpha[3]=4;
    s.textures[0]=(FhTexture){.pixels=deferred,.width=320,.height=240,.format=1};
    fh_renderer_draw(v,6,&s);
    if(fh_renderer_depth_gpu_admission()!=0)fh_platform_panic("CPU variation incorrectly certified GPU");
    fh_renderer_present();
#endif
    for(unsigned repeat=0;repeat<3;repeat++){
        s.texgens=0;s.tev[0].color[3]=10;s.tev[0].alpha[3]=5;
        /* Avoid exact Z8 bucket boundaries: Azahar's float depth roundtrip
         * can move D24 by one LSB when the snapshot becomes an RGBA view. */
        for(unsigned i=0;i<6;i++)v[i].position[2]=-.85371f+.1f*corners[i]+repeat*.05f;
        fh_renderer_draw(v,6,&s);fh_renderer_copy(expected,rect,320,240,0x11,1,0,0,0,&s);
        memset(deferred,0xcd,BYTES);
        if(!fh_renderer_copy_gpu_depth(deferred,rect,320,240,0x11,1,0))fh_platform_panic("Deferred depth not admitted");
        fh_renderer_invalidate(NULL,0);
        /* A stale unused binding must not materialize an authoritative GPU
         * snapshot merely because another material uses the CPU fallback. */
        FhDrawState unrelated=s;fur_fixture(&unrelated);unrelated.texgens=5;
        unrelated.viewport[2]=unrelated.viewport[3]=unrelated.scissor[2]=unrelated.scissor[3]=16;
        unrelated.depth_test=unrelated.depth_write=0;
        for(unsigned i=0;i<5;i++)unrelated.textures[i]=(FhTexture){.pixels=pixels[i],.width=8,.height=8,.format=0x46};
        unrelated.textures[3].pixels=fur_noise;unrelated.textures[3].format=3;
        unrelated.textures[7]=(FhTexture){.pixels=deferred,.width=320,.height=240,.format=1};
        FhVertex quad[6];memcpy(quad,v,sizeof(quad));
        for(unsigned i=0;i<6;i++)for(unsigned t=0;t<5;t++){quad[i].uv[t][0]=quad[i].uv[t][1]=.25f;quad[i].uv[t][2]=1;}
        fh_renderer_draw(quad,6,&unrelated);
        for(unsigned i=0;i<BYTES;i++){if(deferred[i]!=0xcd)fh_platform_panic("Unused texture binding forced depth readback");checks++;}
        for(unsigned i=0;i<6;i++)v[i].position[2]=-.1f;
        fh_renderer_draw(v,6,&s);
        /* Sampling must preserve copy-time depth after later draws overwrite
         * the EFB. The optional GPU view must leave CPU bytes untouched. */
        s.texgens=1;s.tev[0].color[3]=8;s.tev[0].alpha[3]=4;
        for(unsigned route=0;route<2;route++){
            s.textures[0]=(FhTexture){.pixels=route?deferred:expected,.width=320,.height=240,.format=1,.min_filter=1,.mag_filter=1};
            fh_renderer_invalidate(expected,BYTES);
            fh_renderer_draw(v,6,&s);fh_renderer_copy(route?image:reference,rect,320,240,6,1,0,0,0,&s);
        }
        for(unsigned i=0;i<BYTES*4;i++){
            unsigned error=abs((int)image[i]-(int)reference[i]);
            filtered_differences+=error!=0;if(error>filtered_max_error)filtered_max_error=error;
            if(image[i]!=reference[i]){
                FILE *f=fopen("sdmc:/3ds/foxhollow/depth-gpu-image.bin","wb");if(f){fwrite(image,1,BYTES*4,f);fclose(f);}
                f=fopen("sdmc:/3ds/foxhollow/depth-gpu-reference.bin","wb");if(f){fwrite(reference,1,BYTES*4,f);fclose(f);}
                char message[128];snprintf(message,sizeof(message),"Deferred depth sampling differs: repeat=%u byte=%u expected=%u actual=%u",repeat,i,reference[i],image[i]);fh_platform_panic(message);
            }
            checks++;
        }
#ifdef FH_EFB_DEPTH_GPU
#ifdef FH_EFB_DEPTH_VERIFIED
        extern unsigned fh_renderer_depth_gpu_admission(void);
        unsigned admission=fh_renderer_depth_gpu_admission();
        if(admission!=1 && admission!=2)fh_platform_panic("Depth admission did not resolve on varied fixture");
        if(admission==1)
#endif
        for(unsigned i=0;i<BYTES;i++){if(deferred[i]!=0xcd)fh_platform_panic("GPU depth sampling forced CPU readback");checks++;}
#endif
        fh_renderer_materialize_texture(deferred);
        for(unsigned i=0;i<BYTES;i++){
            if(deferred[i]!=expected[i]){
                char message[160];snprintf(message,sizeof(message),"Deferred depth bytes differ: repeat=%u byte=%u expected=%u actual=%u",repeat,i,expected[i],deferred[i]);fh_platform_panic(message);
            }checks++;
        }
        fh_renderer_present();
    }
    memset(deferred,0,BYTES);fh_renderer_invalidate(deferred,BYTES);fh_renderer_materialize_texture(deferred);
    for(unsigned i=0;i<BYTES;i++){if(deferred[i])fh_platform_panic("Depth copy replaced newer CPU bytes");checks++;}
    fh_renderer_invalidate(expected,BYTES);free(expected);free(deferred);free(image);free(reference);
    FILE*f=fopen("sdmc:/3ds/foxhollow/efb-depth.txt","wb");if(!f)fh_platform_panic("Depth fixture report");fprintf(f,"%u\n",checks);fclose(f);
    f=fopen("sdmc:/3ds/foxhollow/efb-depth-filter.txt","wb");if(!f)fh_platform_panic("Depth filter report");fprintf(f,"%u %u\n",filtered_differences,filtered_max_error);fclose(f);
#ifdef FH_EFB_DEPTH_VERIFIED
    extern unsigned fh_renderer_depth_gpu_admission(void);
    f=fopen("sdmc:/3ds/foxhollow/depth-admission.txt","wb");if(!f)fh_platform_panic("Depth admission report");fprintf(f,"%u\n",fh_renderer_depth_gpu_admission());fclose(f);
#endif
}
static void efb_color_device_test(void){
    unsigned char *expected=malloc(320*240*2),*deferred=malloc(320*240*2);
    unsigned char *image=malloc(320*240*4),*reference=malloc(320*240*4);
    if(!expected || !deferred || !image || !reference)fh_platform_panic("EFB ownership fixture allocation");
    FhDrawState s={0};s.viewport[2]=640;s.viewport[3]=480;s.viewport[5]=1;s.scissor[2]=640;s.scissor[3]=480;
    s.color_write=s.alpha_write=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=1;
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.color_clamp=1,.alpha_clamp=1,.channel=4};
    for(unsigned t=0;t<4;t++)for(unsigned c=0;c<4;c++)s.swaps[t][c]=c;
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        unsigned k=corners[i];v[i].position[0]=2*xy[k][0]-1;v[i].position[1]=1-2*xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        v[i].color[0][k%3]=1;v[i].color[0][3]=1;v[i].uv[0][0]=xy[k][0];v[i].uv[0][1]=xy[k][1];v[i].uv[0][2]=1;
    }
    unsigned checks=0;uint32_t rect[]={0,0,640,480};
    for(unsigned repeat=0;repeat<3;repeat++){
        s.texgens=0;s.tev[0].color[3]=10;s.tev[0].alpha[3]=5;
        for(unsigned i=0;i<6;i++)for(unsigned c=0;c<3;c++)v[i].color[0][c]=((corners[i]+repeat)%3==c)?1:.1f*repeat;
        fh_renderer_draw(v,6,&s);fh_renderer_copy(expected,rect,320,240,4,1,0,0,0,&s);
        memset(deferred,0xcd,320*240*2);
        if(!fh_renderer_copy_gpu_color(deferred,rect,320,240,4,1,0))fh_platform_panic("Experimental copy not admitted");
        /* A global GX invalidation must not discard authoritative GPU pixels. */
        fh_renderer_invalidate(NULL,0);
        s.texgens=1;s.tev[0].color[3]=8;s.tev[0].alpha[3]=4;
        for(unsigned i=0;i<6;i++)for(unsigned c=0;c<4;c++)v[i].color[0][c]=1;
        for(unsigned route=0;route<2;route++){
            s.textures[0]=(FhTexture){.pixels=route?deferred:expected,.width=320,.height=240,.format=4,.min_filter=1,.mag_filter=1};
            fh_renderer_invalidate(expected,320*240*2);
            fh_renderer_draw(v,6,&s);fh_renderer_copy(route?image:reference,rect,320,240,6,1,0,0,0,&s);
        }
        for(unsigned i=0;i<320*240*4;i++){if(image[i]!=reference[i])fh_platform_panic("Deferred GPU copy sampling differs");checks++;}
        /* Materialization must still return the original captured image after
         * later draws overwrite the main framebuffer. */
        s.textures[0]=(FhTexture){.pixels=expected,.width=320,.height=240,.format=4,.min_filter=1,.mag_filter=1};
        fh_renderer_draw(v,6,&s);
        extern unsigned fh_renderer_texture_hashed_bytes(void);
        unsigned hashes=fh_renderer_texture_hashed_bytes();
        fh_renderer_materialize_texture(deferred);
        fh_renderer_draw(v,6,&s);
        if(fh_renderer_texture_hashed_bytes()!=hashes)fh_platform_panic("GPU materialization invalidated unrelated texture content");
        for(unsigned i=0;i<320*240*2;i++){if(deferred[i]!=expected[i])fh_platform_panic("Deferred CPU pixels differ");checks++;}
        fh_renderer_present();
    }
    /* Explicit CPU writes invalidate alias ownership, unlike GXInvalidateTexAll. */
    memset(deferred,0,320*240*2);fh_renderer_invalidate(deferred,320*240*2);fh_renderer_materialize_texture(deferred);
    for(unsigned i=0;i<320*240*2;i++){if(deferred[i])fh_platform_panic("CPU write resurrected GPU pixels");checks++;}
    fh_renderer_invalidate(expected,320*240*2);free(expected);free(deferred);free(image);free(reference);
    FILE*f=fopen("sdmc:/3ds/foxhollow/efb-color.txt","wb");if(!f)fh_platform_panic("EFB fixture report");fprintf(f,"%u\n",checks);fclose(f);
    efb_depth_device_test();
}
