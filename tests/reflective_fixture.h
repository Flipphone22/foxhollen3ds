#include "fh_material.h"
/* Scalar oracle versus ordered GPU passes, including saturation, fog after
 * saturation, alpha/color masks, coplanar overlap, depth occlusion and clipping.
 * All images are actual rendered EFB copies, never the candidate's arithmetic. */
static void reflective_device_test(void){
    unsigned char tex[3][256],copy[2][4096];unsigned checks=0,maximum=0,rejections=0;
    unsigned saved=fh_reflective_disabled;fh_reflective_disabled=0;
    unsigned saved_lod=fh_reflective_lod_enabled;fh_reflective_lod_enabled=0;
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=64;s.viewport[5]=1;s.scissor[2]=s.scissor[3]=64;
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
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned c=0;c<4;c++)tex[t][(y*8+x)*4+c]=13+t*29+c*17+x*7+y*5;
        s.textures[t]=(FhTexture){.pixels=tex[t],.width=8,.height=8,.format=0x46,.min_filter=1,.mag_filter=1};
    }
    if(!fh_reflective_material(&s))fh_platform_panic("Reflection fixture not recognized");
    for(unsigned mutation=0;mutation<8;mutation++){
        FhDrawState bad=s;
        switch(mutation){case 0:bad.blend_mode=1;break;case 1:bad.depth_write=0;break;case 2:bad.depth_compare=1;break;
        case 3:bad.tev[2].color_op=1;break;case 4:bad.tev[3].kcolor=12;break;case 5:bad.alpha_compare[0]=1;break;
        case 6:bad.swaps[0][0]=1;break;case 7:bad.konst_colors[0][0]=-1;break;}
        if(fh_reflective_material(&bad))fh_platform_panic("Unsafe reflection state accepted");rejections++;
    }
    const unsigned fog[]={0,2,4,5,6,7,10,12,13,14,15,0};
    for(unsigned scenario=0;scenario<24;scenario++){
        s.fog_type=fog[scenario%12];s.fog_near=1;s.fog_far=100;s.fog_start=1;s.fog_end=80;s.fog_color=0x59c529ff;
        s.alpha_write=scenario!=10;s.color_write=scenario!=11;
        for(unsigned k=0;k<2;k++)for(unsigned c=0;c<3;c++)s.konst_colors[k][c]=scenario>=12?1.f:(.15f+.19f*((k+c)%4));
        FhVertex v[9]={0};const float xy[9][2]={{-1.4f,1.2f},{-.7f,-1.3f},{1.2f,.8f},{-.7f,.8f},{-.2f,-.8f},{1.1f,.7f},{-.9f,.6f},{.2f,-1.1f},{.9f,.9f}};
        for(unsigned i=0;i<9;i++){
            float w=1+.25f*(i%3),z=i<3?-.45f:i<6?-.45f:scenario&1?-.2f:-.7f;
            v[i].position[0]=xy[i][0]*w;v[i].position[1]=xy[i][1]*w;v[i].position[2]=z*w;v[i].position[3]=w;v[i].matrix_index=-1;
            if(scenario==23 && !i)v[i].position[2]=.15f*w;
            for(unsigned c=0;c<4;c++)v[i].color[0][c]=scenario>=12?.95f:.2f+.1f*((i+c)%7);
            for(unsigned t=0;t<5;t++){v[i].uv[t][0]=.13f+.13f*((i+t)%6);v[i].uv[t][1]=.17f+.12f*((i*2+t)%6);v[i].uv[t][2]=1;}
#ifdef FH_REFLECTION_LOD_ONLY
            /* In this special case the reduced sampling has the original
             * expression. Compare the real PICA draw to the scalar oracle,
             * including alpha masks, fog, saturation, overlap and clipping. */
            memcpy(v[i].uv[2],v[i].uv[1],sizeof(v[i].uv[2]));
            memcpy(v[i].uv[4],v[i].uv[3],sizeof(v[i].uv[4]));
#endif
        }
        if(!fh_reflective_triangle(v+3,&s))fh_platform_panic("Reflection triangle declined");
        if(!scenario){FhVertex bad[3];memcpy(bad,v+3,sizeof(bad));bad[1].uv[2][2]=2;
            if(fh_reflective_triangle(bad,&s))fh_platform_panic("Mixed reflection Q accepted");rejections++;}
        unsigned before=fh_reflective_triangles;
#ifdef FH_REFLECTION_LOD_ONLY
        before=fh_reflective_lod_plans;
#endif
        for(unsigned route=0;route<2;route++){
            fh_reflective_disabled=!route;fh_renderer_clear(0x20304077,0xffffff);
#ifdef FH_REFLECTION_LOD_ONLY
            fh_reflective_lod_enabled=route;
#endif
            fh_renderer_draw(v,9,&s);
            const uint32_t rect[]={0,0,64,64};fh_renderer_copy(copy[route],rect,32,32,6,1,0,0,0,&s);
        }
#ifdef FH_REFLECTION_LOD_ONLY
        if(!fh_reflective_lod_plans)fh_platform_panic("Reflection LOD fixture did not exercise GPU plan");
#else
        if(fh_reflective_triangles==before)fh_platform_panic("Reflection fixture did not exercise GPU passes");
#endif
        for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++)for(unsigned c=0;c<4;c++){
            unsigned at=((y/4)*8+x/4)*64+((y&3)*4+(x&3))*2,indices[]={at+1,at+32,at+33,at};
            unsigned error=abs((int)copy[0][indices[c]]-(int)copy[1][indices[c]]);
            if(error>maximum)maximum=error;checks++;
            /* PICA interpolates and multiplies at 8-bit precision; the scalar
             * oracle keeps floating-point intermediates until final packing. */
            if(error>(c==3?2u:5u)){char reason[128];snprintf(reason,sizeof(reason),"Reflection scenario %u pixel %u,%u channel %u error %u",scenario,x,y,c,error);fh_platform_panic(reason);}
        }
    }
    fh_renderer_sync();fh_renderer_invalidate(tex,sizeof(tex));fh_reflective_disabled=saved;
    fh_reflective_lod_enabled=saved_lod;
    FILE*f=fopen("sdmc:/3ds/foxhollow/reflective.txt","wb");if(!f)fh_platform_panic("Reflection report");
    fprintf(f,"%u %u %u\n",checks,maximum,rejections);fclose(f);
}
