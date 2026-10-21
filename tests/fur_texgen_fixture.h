/* Eligible indexed fur must use the live sphere generator on sampler two,
 * including a separate normal palette and CPU material fallback. */
static uint64_t fur_fixture_identity;
static uint64_t fur_fixture_decode(FhVertex *out,const FhCompiledMesh *mesh){
    if(!out)return fur_fixture_identity;
    for(unsigned i=0;i<mesh->unique;i++)memcpy(out+i,mesh->packets+mesh->offsets[i],sizeof(*out));
    return fur_fixture_identity;
}
static void fur_texgen_device_test(void){
    FILE *config=fopen("sdmc:/3ds/foxhollow/fur_mesh.txt","rb");
    unsigned enabled=config && fgetc(config)=='v';if(config)fclose(config);if(!enabled)return;
    FILE *capture=fopen("sdmc:/3ds/foxhollow/fur-texgen.bin","wb");
    if(!capture)fh_platform_panic("Fur texgen capture open failed");
    unsigned checks=0,maximum=0,visible=0;
    extern unsigned fh_gpu_fur_draws,fh_gpu_fur_replays;
    unsigned before_draws=fh_gpu_fur_draws,before_hits=fh_gpu_fur_replays;
    const unsigned corners[]={0,1,2,0,2,3};
    const float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
    static unsigned char sphere[8*8*4],caustic[8*8*4],scroll_noise[128];
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned c=0;c<4;c++)
        sphere[(y*8+x)*4+c]=c==3?255:32+((x*(c+1)+y)%8)*28;
    for(unsigned test=0;test<12;test++){
        FhDrawState s={0};fur_fixture(&s);s.texgens=5;
        s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;s.scissor[2]=s.scissor[3]=16;
        s.color_write=s.alpha_write=1;s.pixel_format=1;s.alpha_compare[0]=s.alpha_compare[3]=7;
        for(unsigned c=0;c<4;c++)s.position_matrix[c][c]=1;
        for(unsigned m=0;m<10;m++)for(unsigned c=0;c<3;c++)s.model_palette[m*3+c][c]=1;
        for(unsigned unit=0;unit<5;unit++)s.textures[unit]=(FhTexture){.pixels=pixels[unit],.width=8,.height=8,.format=0x46};
        s.textures[1].pixels=sphere;s.textures[3].pixels=fur_noise;s.textures[3].format=3;
        memcpy(caustic,pixels[4],sizeof(caustic));s.textures[4].pixels=caustic;fh_renderer_invalidate(caustic,sizeof(caustic));
        if(test==1){
            for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
                caustic[(y*8+x)*4+3]=32+x*28;caustic[(y*8+x)*4+2]=20+y*25;
                scroll_noise[(y*8+x)*2]=192;scroll_noise[(y*8+x)*2+1]=32+x*24+y*3;
            }
            s.textures[3].pixels=scroll_noise;fh_renderer_invalidate(scroll_noise,sizeof(scroll_noise));
            fh_renderer_invalidate(caustic,sizeof(caustic));
        }
        for(unsigned c=0;c<3;c++)s.tev_colors[3][c]=.6f;
        s.tev_colors[3][3]=-.1f;
        if(test>=8)s.textures[2].wrap_s=1; /* Existing scalar fallback. */
        FhGpuLighting light={0};light.mask=test&1?FH_LIGHT_NORMAL_PALETTE:FH_LIGHT_MODEL_NORMAL;
        light.ambient[0]=.2f;light.ambient[1]=.3f;light.ambient[2]=.4f;
        if(test&1)for(unsigned m=0;m<10;m++){
            float (*normal)[4]=(float(*)[4])light.lights+10+m*3;
            float angle=.13f*(test+m);
            normal[0][0]=cosf(angle);normal[0][1]=-sinf(angle);
            normal[1][0]=sinf(angle);normal[1][1]=cosf(angle);normal[2][2]=1;
        }
        float post[3][4]={{-.3f,0,0,.3f+test*.025f},{0,-.3f,0,.6f},{0,0,0,1}};
        s.indirect_exponent[0]=s.indirect_exponent[1]=-1;
        fur_fixture_identity=((uint64_t)(test+1)<<48); /* New owned inputs. */
        unsigned char reference[256],actual[256];
        for(unsigned sample=0;sample<3;sample++){
        float strength=(test+sample+1)/32.f;
        float scroll[3][4]={{1,0,0,sample*.25f},{0,1,0,sample*.125f},{0,0,1,0}};
        s.indirect_matrix[0][0][0]=s.indirect_matrix[0][1][1]=s.indirect_matrix[1][0][1]=s.indirect_matrix[1][1][2]=strength;
        if(test==0 && sample==1){for(unsigned i=3;i<sizeof(caustic);i+=4)caustic[i]^=64;fh_renderer_invalidate(caustic,sizeof(caustic));}
        if(test==6 && sample==2)s.indirect_matrix[0][0][1]=.125f;
        if(test==7 && sample==2)s.indirect_exponent[0]=0;
        for(unsigned mode=0;mode<2;mode++){
            FhVertex v[6]={0};s.raw_position=mode?7:1;
            for(unsigned i=0;i<6;i++){
                unsigned k=corners[i];v[i].matrix_index=(i%3)*3;
                v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
                for(unsigned c=0;c<4;c++)v[i].color[0][c]=1;
                for(unsigned c=0;c<5;c++){v[i].uv[c][0]=v[i].uv[c][1]=.25f;v[i].uv[c][2]=1;}
                if(test==1){v[i].uv[1][0]+=scroll[0][3];v[i].uv[1][1]+=scroll[1][3];}
                v[i].uv[4][0]=.2f+.1f*xy[k][0];v[i].uv[4][1]=.4f+.1f*xy[k][1];
                v[i].uv[4][2]=.6f+.1f*k;
                v[i].uv[7][0]=xy[k][0]*.8f;v[i].uv[7][1]=xy[k][1]*.7f;v[i].uv[7][2]=1;
                if(mode)v[i].uv[6][0]=1;
                else {
                    float n[3]={0,0,1};
                    for(unsigned r=0;r<2;r++){
                        const float *row=test&1?((float(*)[4])light.lights)[10+(unsigned)v[i].matrix_index+r]:s.model_palette[(unsigned)v[i].matrix_index+r];
                        for(unsigned c=0;c<3;c++)n[r]+=row[c]*v[i].uv[7][c];
                    }
                    for(unsigned r=0;r<3;r++)v[i].uv[0][r]=post[r][0]*n[0]+post[r][1]*n[1]+post[r][2]+post[r][3];
                    lighting_reference(v+i,&s,&light);
                }
            }
            if(mode && (!fh_renderer_prepare_lighting(&s,&light) || !fh_renderer_prepare_texgen(&s,post)))fh_platform_panic("Fur GPU generator declined");
            if(test<8){
                uint32_t offsets[6];uint16_t indices[6];for(unsigned i=0;i<6;i++){offsets[i]=i*sizeof(FhVertex);indices[i]=i;}
                FhCompiledMesh mesh={(const unsigned char*)v,offsets,indices,6,6};
                /* One CPU template identity, three post matrices in the same
                 * frame. Native reuse must still distinguish the samples. */
                if(test==1 && mode)mesh.fur_post=scroll;
                if(!fh_renderer_draw_compiled(&mesh,&s,compiled_fixture_decode,mode?fur_fixture_decode:NULL))fh_platform_panic("Fur indexed generator declined");
            }else fh_renderer_draw(v,6,&s);
            uint32_t rect[]={0,0,16,16};fh_renderer_copy(mode?actual:reference,rect,8,8,6,1,0,0,0,&s);
            fwrite(mode?actual:reference,1,256,capture);
            if(mode)for(unsigned c=0;c<256;c++){
                unsigned error=abs((int)actual[c]-reference[c]);if(error>maximum)maximum=error;
                if(error>3){char message[100];snprintf(message,sizeof(message),"Fur texgen mismatch test=%u byte=%u actual=%u ref=%u",test,c,actual[c],reference[c]);fh_platform_panic(message);}
                checks++;visible+=reference[c]!=0;
            }
        }
        }
    }
    fclose(capture);if(!visible)fh_platform_panic("Fur texgen fixture blank");
    FILE *report=fopen("sdmc:/3ds/foxhollow/fur-texgen.json","w");
    if(!report)fh_platform_panic("Fur texgen report open failed");
    unsigned gpu_draws=fh_gpu_fur_draws-before_draws,hits=fh_gpu_fur_replays-before_hits;
    if(gpu_draws!=22 || hits!=11)fh_platform_panic("GPU fur shell reuse/invalidation mismatch");
    fprintf(report,"{\"checks\":%u,\"maximum_error\":%u,\"nonzero_channels\":%u,\"gpu_shell_draws\":%u,\"gpu_shell_replays\":%u}\n",checks,maximum,visible,gpu_draws,hits);fclose(report);
}
