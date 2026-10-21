#include "fh_fragment.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "fur_fixture.h"
static unsigned char pixels[8*8*4],indirect[8*8*4];
static unsigned coordinate_calls;
void __real_fh_fur_samples(float out[2][3],const FhDrawState *s,const FhVertex *v);
void __wrap_fh_fur_samples(float out[2][3],const FhDrawState *s,const FhVertex *v) {
    ++coordinate_calls;__real_fh_fur_samples(out,s,v);
}
#include "fur_coefficients.h"
int main(void) {
    printf("PASS %u cached fur coefficient checks\n",test_fur_coefficients());
    FhDrawState s={0};FhVertex v={0};
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s.swaps[i][c]=c;
    for(unsigned i=0;i<8;i++)v.uv[i][2]=1;
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++) {
        unsigned char *p=pixels+(y*8+x)*4;p[0]=x*31;p[1]=y*29;p[2]=x*11+y*13;p[3]=255;
        p=indirect+(y*8+x)*4;p[0]=0;p[1]=128;p[2]=127;p[3]=130;
    }
    s.textures[0]=(FhTexture){.pixels=pixels,.width=8,.height=8,.format=0x46,.wrap_s=1,.wrap_t=1};
    s.textures[1]=(FhTexture){.pixels=indirect,.width=8,.height=8,.format=0x46};
    s.stages=1;s.indirect_count=1;s.indirect_order[0][1]=1;
    s.indirect_matrix[0][0][0]=1;s.indirect_matrix[0][1][1]=1;
    s.tev[0]=(FhTevStage){.color={15,8,12,15},.alpha={7,4,5,7},.color_clamp=1,.alpha_clamp=1,.channel=8,.indirect={0,0,7,1,0,0,0,0,1}};
    assert(fh_fragment_supported(&s));unsigned checks=0;
    /* Full indirect fetches remain an independent execution oracle. Zero
     * matrices must retain add-previous UV chaining; dynamic S/T and bump
     * alpha must never be classified as unused. */
    {
        FhDrawState z=s;z.indirect_count=2;z.indirect_order[1][1]=1;
        z.stages=2;z.tev[1]=z.tev[0];z.tev[1].indirect[0]=1;
        z.tev[1].indirect[6]=1;
        memset(z.indirect_matrix,0,sizeof(z.indirect_matrix));
        for(unsigned mode=0;mode<4;mode++){
            z.tev[0].channel=z.tev[1].channel=mode==1?8:4;
            z.tev[0].indirect[3]=z.tev[1].indirect[3]=mode==2?5:mode==3?9:1;
            FhFragmentProgram fast,full;fh_fragment_prepare(&fast,&z);full=fast;full.indirect_mask=3;
            assert(fast.indirect_mask==(mode?3u:0u));checks++;
            for(int y=-16;y<16;y++)for(int x=-16;x<16;x++){
                v.uv[0][0]=(x+.37f)/8;v.uv[0][1]=(y+.19f)/8;
                float a[4],b[4];fh_fragment_evaluate_prepared(a,&z,&v,&fast);
                fh_fragment_evaluate_prepared(b,&z,&v,&full);
                assert(!memcmp(a,b,sizeof(a)));checks++;
            }
        }
    }

    for(int y=-32;y<32;y++)for(int x=-32;x<32;x++) {
        v.uv[0][0]=(x+.25f)/8;v.uv[0][1]=(y+.25f)/8;float out[4];assert(fh_fragment_evaluate(out,&s,&v));
        unsigned tx=(x+2+128)%8,ty=(y-1+128)%8;const unsigned char *p=pixels+(ty*8+tx)*4;
        for(unsigned c=0;c<3;c++){assert(fabsf(out[c]-p[c]/255.f)<1e-6f);checks++;}
        assert(fabsf(out[3]-128/248.f)<1e-6f);checks++;
    }
    {
        /* S/T bump stages must produce offsets despite GX_TEX_DISABLE. The
         * independently calculated sample index checks exponent selection,
         * format/bias, projection, zero wrapping and add-previous ordering. */
        FhDrawState d=s;FhVertex dv={0};d.stages=3;d.indirect_order[0][0]=3;
        for(unsigned i=0;i<8;i++)dv.uv[i][2]=1;
        dv.uv[0][2]=dv.uv[1][2]=2;
        d.tev[0]=(FhTevStage){.color={15,15,15,15},.alpha={7,7,7,7},.coord=0,.map=256,.channel=255,.indirect={0,0,3,5,6,6,0,0,0}};
        d.tev[1]=d.tev[0];d.tev[1].coord=1;d.tev[1].indirect[3]=9;d.tev[1].indirect[6]=1;
        d.tev[2]=(FhTevStage){.color={15,15,15,8},.alpha={7,7,7,4},.coord=2,.map=0,.channel=255,.color_clamp=1,.alpha_clamp=1,.indirect={0,0,0,0,0,0,1,0,0}};
        const int exponents[]={-3,0,3,7};
        for(unsigned format=0;format<4;format++)for(unsigned matrix=0;matrix<3;matrix++)for(unsigned e=0;e<4;e++) {
            d.tev[0].indirect[1]=d.tev[1].indirect[1]=format;
            d.tev[0].indirect[3]=5+matrix;d.tev[1].indirect[3]=9+matrix;
            d.indirect_exponent[matrix]=exponents[e];
            d.indirect_matrix[matrix][0][0]=19;d.indirect_matrix[matrix][1][1]=-13;
            assert(fh_fragment_supported(&d));
            int shift=format?format+2:0,bias=format?1:-128;
            float a=(130>>shift)+bias,b=(127>>shift)+bias,factor=ldexpf(1,exponents[e])/256;
            for(int y=-16;y<16;y++)for(int x=-16;x<16;x++) {
                dv.uv[0][0]=2*(x+.25f)/8;dv.uv[0][1]=2*(y+.25f)/8;
                dv.uv[1][0]=2*(y+.75f)/8;dv.uv[1][1]=-2*(x+.75f)/8;
                dv.uv[2][0]=(x*3+.5f)/8;dv.uv[2][1]=(y*5+.25f)/8;
                int tx=(int)floorf(x*3+.5f+((x+.25f)*a+(y+.75f)*b)*factor)%8;
                int ty=(int)floorf(y*5+.25f+((y+.25f)*a-(x+.75f)*b)*factor)%8;
                if(tx<0)tx+=8;if(ty<0)ty+=8;
                float actual[4];assert(fh_fragment_evaluate(actual,&d,&dv));
                for(unsigned c=0;c<4;c++){assert(fabsf(actual[c]-pixels[(ty*8+tx)*4+c]/255.f)<1e-6f);checks++;}
            }
        }
        for(unsigned invalid=4;invalid<=12;invalid+=4){d.tev[0].indirect[3]=invalid;assert(!fh_fragment_supported(&d));}
    }
    s.stages=4;s.indirect_count=0;
    const unsigned colors[4][4]={{15,15,15,8},{15,8,6,10},{15,0,4,15},{8,14,0,0}};
    for(unsigned i=0;i<4;i++) {
        s.tev[i]=(FhTevStage){.alpha={7,7,7,7},.color_clamp=1,.alpha_clamp=1,.channel=4};memcpy(s.tev[i].color,colors[i],16);
    }
    s.tev[1].color_dest=2;s.tev[2].map=255;s.tev[3].color_op=1;s.tev[3].color_bias=1;s.tev[3].kcolor=4;
    for(unsigned c=0;c<3;c++)s.tev_colors[3][c]=(c+1)*.2f;
    for(unsigned k=0;k<10000;k++) {
        for(unsigned c=0;c<3;c++)v.color[0][c]=((k*(c+3))%251)/250.f;
        v.uv[0][0]=((k%8)+.5f)/8;v.uv[0][1]=(((k/8)%8)+.5f)/8;
        float out[4];assert(fh_fragment_evaluate(out,&s,&v));const unsigned char *p=pixels+((k/8)%8*8+k%8)*4;
        for(unsigned c=0;c<3;c++) {
            float tex=p[c]/255.f,light=fminf(1,tex*s.tev_colors[3][c]+v.color[0][c]),base=tex*light;
            float expected=fmaxf(0,fminf(1,.5f+base-(tex*(1-base)+.5f*base)));
            assert(fabsf(out[c]-expected)<1e-6f);checks++;
        }
    }
    for(unsigned size=3;size<=8;size+=5)for(unsigned mode=0;mode<3;mode++)for(unsigned filter=0;filter<2;filter++) {
        FhTexture t={.pixels=pixels,.width=size,.height=size,.format=0x46,.wrap_s=mode,.wrap_t=mode,.mag_filter=filter};
        for(int k=-2000;k<2000;k++) {
            float u=k/73.f,vv=(k*7+11)/91.f,out[4],expected[4]={0};fh_texture_sample(out,&t,u,vv);
            float xx=u*size-(filter?.5f:0),yy=vv*size-(filter?.5f:0);int ix=(int)floorf(xx),iy=(int)floorf(yy);
            float fx=xx-ix,fy=yy-iy;
            for(unsigned dy=0;dy<=filter;dy++)for(unsigned dx=0;dx<=filter;dx++) {
                int coords[2]={ix+(int)dx,iy+(int)dy};
                for(unsigned a=0;a<2;a++) {
                    if(!mode)coords[a]=coords[a]<0?0:coords[a]>=(int)size?(int)size-1:coords[a];
                    else {int period=size*(mode==2?2:1);coords[a]%=period;if(coords[a]<0)coords[a]+=period;if(mode==2 && coords[a]>=(int)size)coords[a]=2*size-1-coords[a];}
                }
                const unsigned char *p=pixels+(coords[1]*size+coords[0])*4;
                float w=filter?(dx?fx:1-fx)*(dy?fy:1-fy):1;
                for(unsigned c=0;c<4;c++)expected[c]+=p[c]*w/255.f;
            }
            for(unsigned c=0;c<4;c++){assert(fabsf(out[c]-expected[c])<2e-5f);checks++;}
        }
    }
    unsigned char noise[128];
    memset(&s,0,sizeof(s));fur_fixture(&s);
    for(unsigned i=0;i<5;i++)s.textures[i]=(FhTexture){.pixels=indirect,.width=8,.height=8,.format=0x46,.wrap_s=1,.wrap_t=2,.mag_filter=1};
    s.textures[3].pixels=noise;s.textures[3].format=3;
    memset(pixels,0,sizeof(pixels));s.textures[0].pixels=pixels;
    for(unsigned i=0;i<128;i++)noise[i]=(i*71+39)%256;
    for(unsigned i=0;i<256;i++)indirect[i]=(i*59+11)%256;
    for(unsigned i=0;i<2;i++)for(unsigned a=0;a<2;a++)for(unsigned c=0;c<3;c++)s.indirect_matrix[i][a][c]=(int)(i*9+a*3+c-5)*.013f;
    for(unsigned i=0;i<8;i++)v.uv[i][2]=1;
    for(unsigned variant=0;variant<2;variant++)for(unsigned k=0;k<10000;k++) {
        const unsigned alpha[2][4]={{7,4,5,0},{4,7,7,0}};
        memcpy(s.tev[3].alpha,alpha[variant],16);
        s.tev[3].channel=variant?255:8;s.tev[3].indirect[8]=variant?0:1;
        s.tev_colors[3][3]=(k%101)/100.f-.4f;
        s.indirect_exponent[0]=(int)(k%7)-3;s.indirect_exponent[1]=(int)(k%5)-2;
        float matrix[2][2][3],full[4],fast[4];
        for(unsigned i=0;i<2;i++)for(unsigned a=0;a<2;a++)for(unsigned c=0;c<3;c++)matrix[i][a][c]=ldexpf(s.indirect_matrix[i][a][c],s.indirect_exponent[i]);
        for(unsigned i=0;i<8;i++){v.uv[i][0]=((int)(k*(i+3)%109)-50)/23.f;v.uv[i][1]=((int)(k*(i+1)%79)-30)/17.f;}
        assert(fh_fur_material(&s));assert(fh_fragment_evaluate(full,&s,&v));fh_fur_noise(fast,&s,&v,matrix);
        assert(fabsf(full[0]-fmaxf(0,.5f-fast[0]*2))<1e-5f);
        assert(fabsf(full[3]-fast[3])<1e-5f);checks+=2;
    }
    s.tev[3].indirect[8]=1;assert(!fh_fur_material(&s));checks++;
    s.tev[3].indirect[8]=0;s.tev[3].alpha[0]=7;assert(!fh_fur_material(&s));checks++;
    s.tev[3].alpha[0]=4;
    FhVertex triangle[3]={v,v,v},mesh[192];
    memset(indirect,128,sizeof(indirect));
    for(unsigned i=0;i<3;i++)for(unsigned c=0;c<8;c++) {
        triangle[i].uv[c][0]=.1f+(i==1?.01f:0);triangle[i].uv[c][1]=.2f+(i==2?.01f:0);triangle[i].uv[c][2]=1;
    }
    float matrix[2][2][3]={{{.1f,0,0},{0,.1f,0}},{{.05f,0,0},{0,.05f,0}}};
    {
        FhDrawState unmasked=s;FhVertex independent[3],result[3];
        memcpy(independent,triangle,sizeof(independent));
        for(unsigned i=0;i<3;i++)independent[i].uv[3][0]+=.37f;
        unmasked.textures[2].wrap_s=0;unmasked.textures[2].mag_filter=0;
        unmasked.indirect_scale[1][0]=1;
        assert(fh_fur_vertices(result,independent,3,&unmasked,matrix)==3);checks++;
        for(unsigned i=0;i<3;i++){
            float uv[3];fh_fur_coordinate(uv,&unmasked,independent+i,matrix);
            for(unsigned c=0;c<2;c++){assert(result[i].uv[7][c]==uv[c]*independent[i].uv[4][2]);checks++;}
        }
        unmasked.tev[3].channel=8;
        assert(!fh_fur_vertices(result,independent,3,&unmasked,matrix));checks++;
    }
    unsigned n=fh_fur_mesh(mesh,192,triangle,&s,matrix);assert(n==3);
    for(unsigned i=0;i<3;i++) {
        float c[3];fh_fur_coordinate(c,&s,&triangle[i],matrix);
        assert(fabsf(mesh[i].uv[7][0]/mesh[i].uv[7][2]-c[0])<1e-6f);
        assert(fabsf(mesh[i].color[0][3]-c[2])<1e-6f);checks+=2;
    }
    assert(fh_fur_mesh(mesh,2,triangle,&s,matrix)==0);checks++;
    FhVertex repeated[192];for(unsigned i=0;i<192;i++)repeated[i]=triangle[i%3];
    for(unsigned pass=0;pass<3;pass++) {
        matrix[0][0][0]+=.1f;memset(indirect,128+pass,sizeof(indirect));
        assert(fh_fur_vertices(mesh,repeated,192,&s,matrix)==192);
        for(unsigned i=0;i<192;i++) {
            float c[3];fh_fur_coordinate(c,&s,&repeated[i],matrix);
            for(unsigned axis=0;axis<2;axis++){assert(mesh[i].uv[7][axis]==c[axis]*repeated[i].uv[4][2]);checks++;}
        }
    }
    /* Quantized game UVs have many zero low mantissa bits. They must not all
     * evict one another from a cache indexed by low hash bits. */
    for(unsigned i=0;i<192;i++)for(unsigned c=0;c<8;c++){
        repeated[i].uv[c][0]=(i%4)*.125f;repeated[i].uv[c][1]=((i/4)%4)*.125f;repeated[i].uv[c][2]=1;
    }
    coordinate_calls=0;assert(fh_fur_vertices(mesh,repeated,192,&s,matrix)==192);
    unsigned cached_calls=coordinate_calls;
    assert(cached_calls<=48);
    for(unsigned i=0;i<192;i++){
        float c[3];fh_fur_coordinate(c,&s,&repeated[i],matrix);
        for(unsigned axis=0;axis<2;axis++){assert(mesh[i].uv[7][axis]==c[axis]);checks++;}
    }
    printf("Quantized UV cache: %u coordinate evaluations / 192 vertices\n",cached_calls);
    coordinate_calls=0;
    assert(fh_fur_vertices_cached(mesh,repeated,192,&s,matrix)==192);
    assert(coordinate_calls==0);
    /* A different shell position/alpha does not affect dependent coordinates. */
    for(unsigned i=0;i<192;i++){repeated[i].position[0]+=.1f;repeated[i].color[0][3]=.7f;}
    s.tev_colors[3][3]=-.25f;
    fh_fur_cache_invalidate_range(pixels,sizeof(pixels)); /* unrelated base color */
    assert(fh_fur_vertices_cached(mesh,repeated,192,&s,matrix)==192);
    assert(coordinate_calls==0);
    for(unsigned mutation=0;mutation<7;mutation++){
        if(mutation==0)matrix[0][0][0]+=.3f;
        if(mutation==1)s.textures[s.indirect_order[0][1]].mag_filter^=1;
        if(mutation==2)s.indirect_scale[0][0]++;
        if(mutation==3)s.textures[3].width*=2;
        if(mutation==4)s.indirect_order[0][0]=0;
        if(mutation==5){memset(indirect,53,sizeof(indirect));fh_fur_cache_invalidate_range(indirect+128,1);}
        if(mutation==6)fh_fur_cache_invalidate(); /* new frame */
        coordinate_calls=0;
        assert(fh_fur_vertices_cached(mesh,repeated,192,&s,matrix)==192);
        float compact[192][3];assert(fh_fur_coordinates_cached(compact,repeated,192,&s,matrix)==192);
        for(unsigned i=0;i<192;i++){assert(!memcmp(compact[i],mesh[i].uv[7],12));checks++;}
        assert((coordinate_calls>0)==(mutation!=0 && mutation!=3));
        for(unsigned i=0;i<192;i++){
            float c[3];fh_fur_coordinate(c,&s,&repeated[i],matrix);
            for(unsigned axis=0;axis<2;axis++){assert(mesh[i].uv[7][axis]==c[axis]);checks++;}
        }
    }
    /* Base projective coordinates are independent of the cached indirect UVs.
     * Same source sample with a different base/q must recompute the result. */
    for(unsigned pass=0;pass<12;pass++){
        for(unsigned i=0;i<192;i++){
            repeated[i].uv[4][0]=(i%7)*.03125f+pass*.015625f;
            repeated[i].uv[4][1]=(i%5)*-.0625f;
            repeated[i].uv[4][2]=pass&1?.5f:2.f;
        }
        if(pass%3==0)matrix[1][1][2]+=.00390625f;
        if(pass%4==0)s.textures[3].height=s.textures[3].height==8?16:8;
        assert(fh_fur_vertices_cached(mesh,repeated,192,&s,matrix)==192);
        float compact[192][3];assert(fh_fur_coordinates_cached(compact,repeated,192,&s,matrix)==192);
        for(unsigned i=0;i<192;i++){assert(!memcmp(compact[i],mesh[i].uv[7],12));checks++;}
        for(unsigned i=0;i<192;i++){
            FhVertex expected=repeated[i];float c[3];fh_fur_coordinate(c,&s,&expected,matrix);
            for(unsigned a=0;a<2;a++)expected.uv[7][a]=c[a]*expected.uv[4][2];
            expected.uv[7][2]=expected.uv[4][2];
            assert(!memcmp(&expected,&mesh[i],sizeof(expected)));checks++;
        }
    }
    FhVertex base[48];memcpy(base,repeated,sizeof(base));
    for(unsigned pass=0;pass<16;pass++){
        unsigned ids[192];float reference[192][3],identified[192][3];
        for(unsigned i=0;i<192;i++){
            unsigned source=(i/3+i%3)%48;
            repeated[i]=base[source];repeated[i].uv[4][0]+=pass*.015625f;
            ids[i]=pass&1?source*16+1:source+1;
            if(pass&2 && i%3==0)ids[i]=0;
        }
        matrix[0][1][2]+=.001953125f;
        assert(fh_fur_coordinates_cached(reference,repeated,192,&s,matrix)==192);
        assert(fh_fur_coordinates_identified(identified,repeated,192,&s,matrix,ids)==192);
        for(unsigned i=0;i<192;i++){assert(!memcmp(reference[i],identified[i],12));checks++;}
    }
    printf("PASS %u fragment comparisons: dependent reads, bump alpha, fur arithmetic and specialized sampling\n",checks);
}
