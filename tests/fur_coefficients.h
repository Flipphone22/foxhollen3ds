#include "fh_fur_gpu.h"
static unsigned test_fur_coefficients(void){
    unsigned char a[256],b[256];unsigned checks=0;
    for(unsigned i=0;i<256;i++){a[i]=(i*37+19)&255;b[i]=(i*13+71)&255;}
    FhDrawState s={0};fur_fixture(&s);s.raw_position=7;
    s.textures[4]=(FhTexture){.pixels=a,.width=8,.height=8,.format=0x46};
    s.textures[2]=(FhTexture){.pixels=b,.width=8,.height=8,.format=0x46};
    s.textures[0]=s.textures[2];s.textures[3].width=64;s.textures[3].height=32;
    FhVertex v[16]={0};float reference[16][3],cached[16][3];
    for(unsigned i=0;i<16;i++){
        v[i].matrix_index=(i%10)*3;v[i].uv[6][0]=1;
        for(unsigned j=0;j<5;j++){
            v[i].uv[j][0]=i*.125f-.5f;v[i].uv[j][1]=i*.0625f+.25f;v[i].uv[j][2]=1;
        }
    }
    for(unsigned trial=0;trial<48;trial++){
        s.textures[0].wrap_s=s.textures[2].wrap_s=trial%3;
        s.textures[0].wrap_t=s.textures[2].wrap_t=trial/3%3;
        s.textures[0].mag_filter=s.textures[2].mag_filter=trial&1;
        s.indirect_scale[0][0]=trial%4;s.indirect_scale[0][1]=trial/4%4;
        s.textures[3].width=1u<<(trial%7);s.textures[3].height=1u<<(trial/7%7);
        fh_fur_cache_invalidate();
        for(unsigned round=0;round<5;round++){
            if(round==1){a[trial]^=127;fh_fur_cache_invalidate_range(a+trial,1);}
            if(round==2){b[trial+17]^=211;fh_fur_cache_invalidate_range(b+trial+17,1);}
            if(round==3){v[0].uv[1][0]+=.03125f;v[1].uv[2][1]-=.0625f;v[1].uv[3][1]=v[1].uv[2][1];}
            if(round==4){s.textures[4].pixels=b;s.textures[2].pixels=a;}
            assert(fh_fur_gpu_coefficients(reference,v,16,&s));
            assert(fh_fur_gpu_coefficients_cached(cached,v,16,&s));
            assert(!memcmp(reference,cached,sizeof(cached)));checks+=48;
            unsigned before=coordinate_calls;
            assert(fh_fur_gpu_coefficients_cached(cached,v,16,&s));
            assert(!memcmp(reference,cached,sizeof(cached)));
            /* Direct-mapped bucket collisions may resample, but cannot change
             * the result. Repeating the final vertex immediately must hit. */
            assert(coordinate_calls-before<16);
            before=coordinate_calls;
            assert(fh_fur_gpu_coefficients_cached(cached,v+15,1,&s));
            assert(!memcmp(reference[15],cached[0],12));
            assert(coordinate_calls==before);checks+=52;
        }
        s.textures[4].pixels=a;s.textures[2].pixels=b;
    }
    /* Paletted sampling shares exactly the same source-write invalidation. */
    unsigned char indices[32]={0},palette[32];
    for(unsigned i=0;i<32;i++)palette[i]=i*17;
    s.textures[4]=(FhTexture){.pixels=indices,.palette=palette,.palette_count=16,
        .palette_format=1,.width=8,.height=8,.format=8};
    for(unsigned i=0;i<16;i++){
        palette[i*2]^=93;fh_fur_cache_invalidate_range(palette+i*2,2);
        assert(fh_fur_gpu_coefficients(reference,v,16,&s));
        assert(fh_fur_gpu_coefficients_cached(cached,v,16,&s));
        assert(!memcmp(reference,cached,sizeof(cached)));checks+=48;
    }
    FhVertex invalid=v[0];
    for(unsigned i=0;i<5;i++){
        invalid=v[0];
        if(i==0)invalid.matrix_index=-1;
        if(i==1)invalid.uv[6][0]=0;
        if(i==2)invalid.uv[2][0]+=1;
        if(i==3)invalid.uv[1][2]=0;
        if(i==4)invalid.uv[4][0]=NAN;
        memset(cached,0x39,sizeof(cached));memcpy(reference,cached,sizeof(cached));
        assert(!fh_fur_gpu_coefficients_cached(cached,&invalid,1,&s));
        assert(!fh_fur_gpu_coefficients(reference,&invalid,1,&s));
        assert(!memcmp(reference,cached,sizeof(cached)));checks++;
    }
    /* Peppy uses the first indirect matrix and a zero second one, exponent
     * -15. Compare GPU coefficients with the original general matrix equation,
     * not only the cached implementation of those coefficients. */
    s.tev[3].channel=255;
    s.indirect_exponent[0]=-1;s.indirect_exponent[1]=-15;
    memset(s.indirect_matrix,0,sizeof(s.indirect_matrix));
    for(unsigned shell=0;shell<=16;shell++){
        float f=shell/32.f,strength=-1;
        s.indirect_matrix[0][0][0]=s.indirect_matrix[0][1][1]=f;
        assert(fh_fur_gpu_zero_second(&s));
        assert(fh_fur_gpu_strength(&s,&strength) && strength==f);checks+=2;
        assert(fh_fur_gpu_coefficients(reference,v,16,&s));
        assert(fh_fur_gpu_coefficients_cached(cached,v,16,&s));
        assert(!memcmp(reference,cached,sizeof(cached)));checks++;
        float matrix[2][2][3]={0};matrix[0][0][0]=matrix[0][1][1]=f*.5f;
        for(unsigned i=0;i<16;i++){
            float samples[2][3],coordinate[3];fh_fur_samples(samples,&s,v+i);
            fh_fur_coordinate_sampled(coordinate,&s,v+i,matrix,samples);
            for(unsigned axis=0;axis<2;axis++){
                float actual=v[i].uv[4][axis]/v[i].uv[4][2]+cached[i][axis]*f;
                assert(fabsf(actual-coordinate[axis])<2e-6f);checks++;
            }
        }
    }
    for(unsigned word=0;word<6;word++){
        ((float*)s.indirect_matrix[1])[word]=1.f/1024;
        float strength;assert(!fh_fur_gpu_zero_second(&s));assert(!fh_fur_gpu_strength(&s,&strength));checks+=2;
        ((float*)s.indirect_matrix[1])[word]=0;
    }
    s.tev[3].channel=8;float strength;
    assert(!fh_fur_gpu_strength(&s,&strength));checks++;
    fh_fur_cache_invalidate();return checks;
}
