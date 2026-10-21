#include "fh_pica_texture.h"
static void prepared_texture_device_test(void){
    FILE *file=fopen("sdmc:/3ds/foxhollow/prepared-texture-fixture.bin","rb");if(!file)return;
    unsigned checks=0,maximum=0,trials=0;
    FhDrawState s={0};s.viewport[2]=128;s.viewport[3]=64;s.viewport[5]=1;s.scissor[2]=128;s.scissor[3]=64;
    s.color_write=s.alpha_write=1;s.pixel_format=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=s.texgens=1;
    for(unsigned t=0;t<4;t++)for(unsigned c=0;c<4;c++)s.swaps[t][c]=c;
    s.tev[0]=(FhTevStage){.color={15,8,12,15},.alpha={7,4,6,7},.color_clamp=1,.alpha_clamp=1,.channel=4};
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        unsigned k=corners[i];v[i].position[0]=-1+2*xy[k][0];v[i].position[1]=1-2*xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        for(unsigned c=0;c<4;c++)v[i].color[0][c]=1;
        v[i].uv[0][0]=xy[k][0];v[i].uv[0][1]=xy[k][1];v[i].uv[0][2]=1;
    }
    unsigned dims[3];unsigned char reference[64*32*4],actual[64*32*4];
    while(fread(dims,1,sizeof(dims),file)==sizeof(dims)){
        unsigned w=dims[0],h=dims[1],n=dims[2];
        if(w<8 || h<8 || w>1024 || h>1024 || n!=w*h/2)fh_platform_panic("Prepared texture fixture extent");
        unsigned char *source=malloc(n),*rgba=malloc(w*h*4);
        if(!source || !rgba || fread(source,1,n,file)!=n || fread(rgba,4,w*h,file)!=w*h)fh_platform_panic("Prepared texture fixture read");
        for(unsigned filter=0;filter<2;filter++)for(unsigned route=0;route<2;route++){
            s.textures[0]=(FhTexture){.pixels=route?source:rgba,.width=w,.height=h,.format=route?14:0x46,.min_filter=filter,.mag_filter=filter};
            fh_renderer_invalidate(route?source:rgba,route?n:w*h*4);
            fh_renderer_draw(v,6,&s);uint32_t rect[]={0,0,128,64};fh_renderer_copy(route?actual:reference,rect,64,32,6,1,0,0,0,&s);
            if(route)for(unsigned i=0;i<sizeof(actual);i++){
                unsigned error=abs((int)actual[i]-reference[i]);if(error>maximum)maximum=error;checks++;
                if(error>1)fh_platform_panic("Prepared ETC1/A4 GPU image differs from decoded reference");
            }
            fh_renderer_present();
        }
        fh_renderer_invalidate(source,n);fh_renderer_invalidate(rgba,w*h*4);free(source);free(rgba);trials++;
    }
    fclose(file);file=fopen("sdmc:/3ds/foxhollow/prepared-texture.txt","wb");
    if(!file)fh_platform_panic("Prepared texture fixture report");
    fprintf(file,"%u %u %u\n",trials,checks,maximum);fclose(file);
}
static void pica_texture_device_test(void){
    unsigned char *native=malloc(131072),*rgba=malloc(512*384*4);
    unsigned char reference[64*32*4],actual[64*32*4];unsigned checks=0,maximum=0;
    if(!native || !rgba)fh_platform_panic("ETC fixture allocation");
    uint32_t random=0x6182b49d;
    for(unsigned b=0;b<131072;b+=8){
        random=random*1664525+1013904223;memcpy(native+b,&random,4);
        /* Exercise individual/differential, flips, tables and all selectors.
         * Differential bases 16 keep signed deltas inside valid 5-bit RGB. */
        uint32_t high=(random&2)?(0x80808000|(random&0x070707ff)):random;
        memcpy(native+b+4,&high,4);
    }
    FhDrawState s={0};s.viewport[2]=128;s.viewport[3]=64;s.viewport[5]=1;s.scissor[2]=128;s.scissor[3]=64;
    s.color_write=s.alpha_write=1;s.pixel_format=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=s.texgens=1;
    for(unsigned t=0;t<4;t++)for(unsigned c=0;c<4;c++)s.swaps[t][c]=c;
    s.tev[0]=(FhTevStage){.color={15,8,12,15},.alpha={7,4,6,7},.color_clamp=1,.alpha_clamp=1,.channel=4};
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned trial=0;trial<8;trial++){
        unsigned widths[]={8,16,64,512},heights[]={8,8,48,384};unsigned w=widths[trial/2],h=heights[trial/2],pw=fh_texture_extent(w);
        for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
            uint32_t p=fh_pica_etc1_pixel(native,pw,x,y);unsigned char *dst=rgba+4*(y*w+x);
            dst[0]=p>>24;dst[1]=p>>16;dst[2]=p>>8;dst[3]=p;
        }
        for(unsigned i=0;i<6;i++){
            unsigned k=corners[i];v[i].position[0]=-1+2*xy[k][0];v[i].position[1]=1-2*xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
            for(unsigned c=0;c<4;c++)v[i].color[0][c]=1;
            v[i].uv[0][0]=xy[k][0];v[i].uv[0][1]=xy[k][1];v[i].uv[0][2]=1;
        }
        for(unsigned route=0;route<2;route++){
            s.textures[0]=(FhTexture){.pixels=route?native:rgba,.width=w,.height=h,.format=route?FH_TEXTURE_PICA_ETC1:0x46,.min_filter=trial&1,.mag_filter=trial&1,.serial=trial+1};
            fh_renderer_invalidate(route?native:rgba,route?131072:512*384*4);
            fh_renderer_draw(v,6,&s);uint32_t rect[]={0,0,128,64};fh_renderer_copy(route?actual:reference,rect,64,32,6,1,0,0,0,&s);
            if(route)for(unsigned i=0;i<sizeof(actual);i++){
                unsigned error=abs((int)actual[i]-reference[i]);if(error>maximum)maximum=error;checks++;
                if(error>1)fh_platform_panic("Native ETC texture differs from scalar-decoded RGBA");
            }
            fh_renderer_present();
        }
    }
    FILE *f=fopen("sdmc:/3ds/foxhollow/pica-texture.txt","wb");if(!f)fh_platform_panic("ETC report");fprintf(f,"%u %u\n",checks,maximum);fclose(f);
    fh_renderer_invalidate(native,131072);fh_renderer_invalidate(rgba,512*384*4);free(native);free(rgba);
    prepared_texture_device_test();
}
