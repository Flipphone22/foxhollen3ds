#include <math.h>
static void layered_five_device_test(void){
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;
    s.scissor[2]=s.scissor[3]=16;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=5;s.texgens=3;
    unsigned char pixels[4][256],copy[256];FhVertex v[6]={0};
    unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
    for(unsigned i=0;i<6;i++){
        unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        for(unsigned t=0;t<3;t++)v[i].uv[t][2]=1;
    }
    const unsigned colors[5][4]={{15,8,14,10},{15,8,4,15},{15,6,8,15},{15,8,4,6},{8,0,1,15}};
    const unsigned alpha[5][4]={{7,7,7,7},{7,4,3,7},{7,7,7,7},{7,4,6,7},{7,4,0,7}};
    const unsigned maps[]={0,1,2,2,3},coords[]={0,0,1,1,2},dest[]={2,3,3,0,0};
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s.swaps[i][c]=i==3?(c==3?1:2):c;
    for(unsigned i=0;i<5;i++){
        s.tev[i]=(FhTevStage){.color_clamp=1,.alpha_clamp=1,.map=maps[i],.coord=coords[i],.color_dest=dest[i],.channel=i?255:4,.tex_swap=i==1?3:0};
        memcpy(s.tev[i].color,colors[i],16);memcpy(s.tev[i].alpha,alpha[i],16);
    }
    s.tev[1].indirect[4]=s.tev[1].indirect[5]=6;s.tev[1].indirect[6]=1;
    for(unsigned t=0;t<4;t++)s.textures[t]=(FhTexture){.pixels=pixels[t],.width=8,.height=8,.format=6};
    unsigned checks=0,maximum=0,ordinary_max=0,fade_max=0;
    for(unsigned scenario=0;scenario<320;scenario++){
        unsigned tint=scenario>=192?(scenario&1):scenario>=64;
        unsigned second_family=(scenario>=64 && scenario<128) || scenario>=256;
        s.stages=(second_family?6:5)+tint;s.tev[3].color[2]=second_family?14:4;s.tev[3].color[3]=second_family?15:6;
        if(second_family)s.tev[5]=(FhTevStage){.color={15,0,4,6},.alpha={7,7,7,0},.color_clamp=1,.alpha_clamp=1,.coord=255,.map=255,.channel=255};
        if(tint){
            s.tev[s.stages-1]=(FhTevStage){.color={0,14,3,15},.alpha={7,7,7,0},.color_clamp=1,.alpha_clamp=1,.coord=255,.map=255,.channel=255,.kcolor=12};
            s.tev_colors[1][3]=((scenario*29)&255)/255.f;
            for(unsigned c=0;c<3;c++)s.konst_colors[0][c]=((scenario*17+c*59)&255)/255.f;
        }
        s.tev[3].kalpha=scenario>=192?28:0;
        s.konst_colors[0][3]=((scenario*23)&255)/255.f;
        float opacity=scenario>=192?s.konst_colors[0][3]:1;
        unsigned color[4][4],raster[4];
        for(unsigned c=0;c<4;c++){
            raster[c]=(7+scenario*19+c*31)&255;
            for(unsigned t=0;t<4;t++)color[t][c]=(scenario*47+t*71+c*59)&255;
        }
        for(unsigned t=0;t<4;t++)for(unsigned k=0;k<64;k++){
            unsigned at=(k/16)*64+(k%16)*2;
            pixels[t][at]=color[t][3];pixels[t][at+1]=color[t][0];pixels[t][at+32]=color[t][1];pixels[t][at+33]=color[t][2];
        }
        for(unsigned i=0;i<6;i++)for(unsigned c=0;c<4;c++)v[i].color[0][c]=raster[c]/255.f;
        FhMaterial plan;if(!fh_layered_material_compile(&plan,&s) || plan.pipeline.count!=s.stages-1)fh_platform_panic("Layered pipeline declined");
        fh_renderer_invalidate(pixels,sizeof(pixels));fh_renderer_draw(v,6,&s);
        const uint32_t rect[]={0,0,16,16};fh_renderer_copy(copy,rect,8,8,6,1,0,0,0,&s);
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned c=0;c<4;c++){
            float expected;
            if(c==3)expected=color[2][3]*color[3][3]/255.f*opacity;
            else {
                float l=fminf(1,(color[0][c]+raster[c])/255.f),b=color[1][2]/255.f,t=color[2][c]/255.f;
                float sum=fminf(1,t*l+t*(b*l)),a=color[2][3]/255.f*opacity;
                expected=255*(sum*a+color[3][c]/255.f*(1-a));
                if(second_family)expected=255*fminf(1,(t*a+color[3][c]/255.f*(1-a))*l+t*(b*l));
                if(tint){
                    float base=expected/255.f;
                    expected=255*(base*(1-s.tev_colors[1][3])+s.konst_colors[0][c]*s.tev_colors[1][3]);
                }
            }
            unsigned at=((y/4)*2+x/4)*64+((y&3)*4+(x&3))*2,indices[]={at+1,at+32,at+33,at};
            unsigned error=abs((int)copy[indices[c]]-(int)(expected+.5f));if(error>maximum)maximum=error;
            if(second_family && scenario>=256){if(error>fade_max)fade_max=error;}
            else if(error>ordinary_max)ordinary_max=error;
            /* Six-stage fades retain half-range intermediates to avoid an
             * incorrect early clamp. Quantizing K/2 and the alpha product,
             * then doubling, adds at most 3 levels to the existing 3-level
             * combiner bound. Earlier opaque and five-stage cases stay at3. */
            unsigned limit=second_family && scenario>=256?6:3;
            if(error>limit){char message[128];snprintf(message,sizeof(message),"Layered pixel mismatch scenario=%u channel=%u actual=%u expected=%.5f error=%u",scenario,c,copy[indices[c]],expected,error);fh_platform_panic(message);}checks++;
        }
    }
    fh_renderer_invalidate(pixels,sizeof(pixels));fh_renderer_present();
    s.stages=5;s.tev[3].color[2]=4;s.tev[3].color[3]=6;s.tev[3].kalpha=0;
    /* Prime an evictable texture, then occupy unrelated linear storage.
     * The next three-sampler draw must reclaim old cache bytes while keeping
     * all of its freshly pinned textures valid and preserving the reserve. */
    unsigned char *large=malloc(1024*1024);if(!large)fh_platform_panic("Texture pressure source");
    memset(large,0x55,1024*1024);
    FhDrawState prime=s;prime.stages=1;
    prime.tev[0]=(FhTevStage){.color={15,15,15,8},.alpha={7,7,7,4},.color_clamp=1,.alpha_clamp=1};
    prime.textures[0]=(FhTexture){.pixels=large,.width=512,.height=512,.format=0x46};
    fh_renderer_draw(v,6,&prime);fh_renderer_sync();
    unsigned free_bytes=linearSpaceFree();
    if(free_bytes<4*1024*1024)fh_platform_panic("Texture pressure setup reserve");
    unsigned fill_bytes=free_bytes>4*1024*1024+128*1024?(free_bytes-4*1024*1024-128*1024)&~31u:0;
    void *filler=fill_bytes?linearMemAlign(fill_bytes,32):NULL;
    if(fill_bytes && !filler)fh_platform_panic("Texture pressure allocation");
    for(unsigned t=0;t<4;t++)s.textures[t]=(FhTexture){.pixels=large+t*256*256*4,.width=256,.height=256,.format=6};
    for(unsigned round=0;round<3;round++){
        for(unsigned t=0;t<4;t++)memset(large+t*256*256*4,40+t*53+round*11,256*256*4);
        fh_renderer_invalidate(large,1024*1024);fh_renderer_draw(v,6,&s);
        const uint32_t rect[]={0,0,16,16};fh_renderer_copy(copy,rect,8,8,6,1,0,0,0,&s);
        if(linearSpaceFree()<4*1024*1024)fh_platform_panic("Texture cache consumed linear reserve");
        for(unsigned c=0;c<4;c++){
            float t0=(40+round*11)/255.f,b=(93+round*11)/255.f,t=(146+round*11)/255.f,d=(199+round*11)/255.f;
            float l=fminf(1,t0+v[0].color[0][c]);float expected=c==3?t*d:fminf(1,t*l+t*b*l)*t+d*(1-t);
            const unsigned indices[]={1,32,33,0};
            if(abs((int)copy[indices[c]]-(int)(expected*255+.5f))>3)fh_platform_panic("Pinned texture changed under memory pressure");
        }
    }
    fh_renderer_sync();if(filler)linearFree(filler);fh_renderer_invalidate(large,1024*1024);free(large);
    FILE*f=fopen("sdmc:/3ds/foxhollow/layered-five.txt","wb");if(!f)fh_platform_panic("Layered material report");fprintf(f,"%u %u %u %u\n",checks,maximum,ordinary_max,fade_max);fclose(f);
}
