#include "fh_material.h"
static void blur_fixture(FhDrawState *s) {
    s->stages=6;s->texgens=6;s->depth_test=s->depth_write=s->fog_type=s->cull=s->indirect_count=0;
    s->blend_mode=1;s->blend_src=4;s->blend_dst=5;
    s->alpha_compare[0]=s->alpha_compare[3]=7;s->alpha_compare[2]=0;
    for(unsigned i=0;i<6;i++){
        s->tev[i]=(FhTevStage){.color={15,8,14,i==1?15:0},.alpha={7,7,7,0},
            .color_scale=i==5?3:0,.alpha_scale=i<2?3:2,.color_clamp=i==0 || i==5,.alpha_clamp=1,
            .coord=i==0?1:i==1?0:i,.map=i==0?1:0,.channel=255,.kcolor=13};
    }
    s->tev[0]=(FhTevStage){.color={15,15,15,15},.alpha={6,7,7,4},.alpha_op=1,
        .alpha_scale=3,.color_clamp=1,.alpha_clamp=1,.coord=1,.map=1,.channel=255,.kalpha=28};
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s->swaps[i][c]=c;
    for(unsigned c=0;c<3;c++)s->konst_colors[1][c]=.4f;
    s->konst_colors[0][3]=254/255.f;
}
static void blur_device_test(void) {
    unsigned char rgb[128],mask[64],reference[64*32*4],actual[64*32*4];
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
        unsigned word=((x*4)<<11)|((y*9)<<5)|((x+y)*2);
        unsigned index=((y/4)*2+x/4)*32+((y%4)*4+x%4)*2;
        rgb[index]=word>>8;rgb[index+1]=word;
        mask[(y/4)*32+(y%4)*8+x]=(x+y)%6+250;
    }
    FhDrawState s={0};s.viewport[2]=128;s.viewport[3]=64;s.viewport[5]=1;
    s.scissor[2]=128;s.scissor[3]=64;s.color_write=s.alpha_write=1;s.pixel_format=1;
    blur_fixture(&s);
    s.textures[0]=(FhTexture){.pixels=rgb,.width=8,.height=8,.format=4};
    s.textures[1]=(FhTexture){.pixels=mask,.width=8,.height=8,.format=1};
    FhVertex v[6]={0};const unsigned corner[]={0,1,2,0,2,3};const float xy[4][2]={{0,0},{1,0},{1,1},{0,1}};
    for(unsigned i=0;i<6;i++){
        float x=xy[corner[i]][0],y=xy[corner[i]][1];
        v[i].position[0]=2*x-1;v[i].position[1]=1-2*y;v[i].position[2]=-.5f;v[i].position[3]=1;
        for(unsigned t=0;t<6;t++){v[i].uv[t][0]=x;v[i].uv[t][1]=y;v[i].uv[t][2]=1;}
        v[i].uv[0][1]-=.125f;v[i].uv[2][1]-=.125f;
        v[i].uv[3][0]+=.125f;v[i].uv[4][0]-=.125f;
        for(unsigned c=0;c<4;c++)v[i].color[0][c]=(c+1)*.15f;
    }
    if(!fh_blur_material(&s) || !fh_blur_quad(v,6,&s))fh_platform_panic("Blur fixture not recognized");
    FhDrawState invalid=s;invalid.tev[4].alpha_op=1;
    if(fh_blur_material(&invalid))fh_platform_panic("Invalid blur accepted");
    invalid=s;invalid.textures[1].mag_filter=1;
    if(fh_blur_quad(v,6,&invalid) || fh_blur_quad(v,3,&s))fh_platform_panic("Unsupported blur accepted");
    invalid.viewport[2]=invalid.viewport[3]=16;
    FhVertex perspective[6];memcpy(perspective,v,sizeof(v));
    for(unsigned c=0;c<4;c++)perspective[1].position[c]*=2;
    if(fh_blur_quad(perspective,6,&invalid))fh_platform_panic("Perspective filtered blur accepted");
    FhVertex overlap[6];memcpy(overlap,v,sizeof(v));overlap[5]=overlap[1];
    if(fh_blur_quad(overlap,6,&s))fh_platform_panic("Overlapping blur accepted");
    invalid=s;invalid.fog_type=4;invalid.fog_start=72.4825f;invalid.fog_end=10010;invalid.fog_near=2.5f;invalid.fog_far=10000;
    if(!fh_blur_material(&invalid) || !fh_blur_quad(v,6,&invalid))fh_platform_panic("Clear dialogue fog rejected");
    memcpy(overlap,v,sizeof(v));for(unsigned i=0;i<6;i++)overlap[i].position[2]=-.01f;
    if(fh_blur_quad(overlap,6,&invalid))fh_platform_panic("Fogged additive blur accepted");
    overlap[0].position[2]=overlap[3].position[2]=-.9f;
    if(fh_blur_quad(overlap,6,&invalid))fh_platform_panic("Fog crossing additive blur accepted");
    unsigned checks=0,max_error=0;
    uint32_t rect[]={0,0,128,64};
    for(unsigned test=0;test<36;test++){
        if(test==12){
            for(unsigned i=6;i>0;i--)s.tev[i]=s.tev[i-1];s.stages=7;
            s.tev[0].alpha[0]=4;s.tev[0].alpha[3]=6;s.tev[0].color_dest=s.tev[0].alpha_dest=3;
            s.tev[2].alpha[0]=0;s.tev[2].alpha[3]=3;
        }
        if(test==24)for(unsigned i=0;i<7;i++)s.tev[i].alpha_scale=i<2 || i==6?0:2;
        if(!fh_blur_material(&s))fh_platform_panic("Blur variant rejected");
        s.viewport[2]=rect[2]=128;s.viewport[3]=rect[3]=64;s.fog_type=0;
        s.textures[1].min_filter=s.textures[1].mag_filter=0;
        if(test%12>=6){
            s.viewport[2]=s.viewport[3]=16;rect[2]=rect[3]=16;
            s.textures[1].min_filter=s.textures[1].mag_filter=1;
            s.fog_type=4;s.fog_start=12419.3965f;s.fog_end=12429.3926f;s.fog_near=2.5f;s.fog_far=10000;
            if(!fh_blur_material(&s) || !fh_blur_quad(v,6,&s))fh_platform_panic("Native filtered blur rejected");
        }
        if(test%12<3){
            s.fog_type=4;s.fog_start=72.4825f;s.fog_end=10010;s.fog_near=2.5f;s.fog_far=10000;
            if(!fh_blur_material(&s) || !fh_blur_quad(v,6,&s))fh_platform_panic("Dialogue blur rejected");
        }
        s.konst_colors[0][3]=(test%3==0?254:test%3==1?252:0)/255.f;
        for(unsigned c=0;c<3;c++)s.konst_colors[1][c]=.4f;
        if(test%6>=3){s.konst_colors[1][0]=.2f;s.konst_colors[1][1]=0;}
        s.alpha_write=test%2;
        for(unsigned route=0;route<2;route++){
            FhDrawState base=s;base.stages=1;base.blend_mode=0;base.alpha_write=1;
            base.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.color_clamp=1,.alpha_clamp=1,.channel=4,.map=255,.coord=255};
            fh_renderer_draw(v,6,&base);
            if(test==0){
                unsigned char copy[64];uint32_t small[]={0,0,8,8};FhDrawState clear=base;clear.alpha_write=0;
                fh_renderer_copy(copy,small,4,4,6,1,1,0x26667311,0,&clear);
                fh_renderer_copy(copy,small,4,4,6,1,0,0,0,&clear);
                if(copy[0]!=153 || copy[1]!=38 || copy[32]!=102 || copy[33]!=115)fh_platform_panic("RGB-only clear changed alpha");
            }
            if(route)fh_renderer_draw(v,6,&s);
            else {fh_renderer_draw(v,3,&s);fh_renderer_draw(v+3,3,&s);}
            fh_renderer_copy(route?actual:reference,rect,rect[2]/2,rect[3]/2,6,1,0,0,0,&s);
        }
        for(unsigned i=0;i<rect[2]*rect[3];i++){
            unsigned error=abs((int)actual[i]-reference[i]);if(error>max_error)max_error=error;
            if(error>5){
                FILE *f=fopen("sdmc:/3ds/foxhollow/blur-reference.bin","wb");fwrite(reference,1,sizeof(reference),f);fclose(f);
                f=fopen("sdmc:/3ds/foxhollow/blur-actual.bin","wb");fwrite(actual,1,sizeof(actual),f);fclose(f);
                char error_text[160];snprintf(error_text,sizeof(error_text),"Blur mismatch case=%u byte=%u gpu=%u cpu=%u",test,i,actual[i],reference[i]);fh_platform_panic(error_text);
            }
            checks++;
        }
        fh_renderer_present();fh_renderer_invalidate(NULL,0);
    }
    FILE *f=fopen("sdmc:/3ds/foxhollow/blur.txt","wb");
    if(!f)fh_platform_panic("Blur report failed");fprintf(f,"%u %u\n",checks,max_error);fclose(f);
}
