#include <assert.h>
#include <stdio.h>
#include "../native-3ds/source/fragment.c"
static unsigned rng=0x534641;
static unsigned random_word(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static void reference_sample(float out[4],const FhTexture *t,float s,float v) {
    float x=s*t->width,y=v*t->height;
    if(!t->wrap_s)x=fmaxf(-1,fminf(t->width+1,x));
    if(!t->wrap_t)y=fmaxf(-1,fminf(t->height+1,y));
    if(!t->mag_filter) {
        uint32_t p=fh_texture_pixel(t,wrap((int)floorf(x),t->width,t->wrap_s),wrap((int)floorf(y),t->height,t->wrap_t));
        for(unsigned c=0;c<4;c++)out[c]=((p>>(24-8*c))&255)/255.f;
    } else {
        x-=.5f;y-=.5f;int ix=floorf(x),iy=floorf(y);float fx=x-ix,fy=y-iy;
        memset(out,0,16);
        for(unsigned dy=0;dy<2;dy++)for(unsigned dx=0;dx<2;dx++) {
            uint32_t p=fh_texture_pixel(t,wrap(ix+dx,t->width,t->wrap_s),wrap(iy+dy,t->height,t->wrap_t));
            float weight=(dx?fx:1-fx)*(dy?fy:1-fy)/255.f;
            for(unsigned c=0;c<4;c++)out[c]+=((p>>(24-8*c))&255)*weight;
        }
    }
}
int main(void) {
    unsigned checks=0;
    const float bounds[][2]={{0,1},{-1,1025},{-1024/255.f,1023/255.f}};
    for(unsigned i=0;i<2000000;i++) {
        union { unsigned u;float f; } x={.u=random_word()};
        for(unsigned b=0;b<3;b++) {
            float expected=fmaxf(bounds[b][0],fminf(bounds[b][1],x.f));
            float actual=sample_clamp(x.f,bounds[b][0],bounds[b][1]);
            assert(!memcmp(&expected,&actual,sizeof(float)));checks++;
        }
        if(isfinite(x.f) && isfinite(x.f*255)) {
            assert(compare_byte(x.f)==((unsigned)(int)fmodf(roundf(x.f*255),256)&255));checks++;
        }
    }
    for(int i=-4096;i<=4096;i++) {
        float x=(i+.5f)/255;
        for(unsigned j=0;j<3;j++) {
            float v=j==0?x:nextafterf(x,j==1?-INFINITY:INFINITY);
            assert(compare_byte(v)==((unsigned)(int)fmodf(roundf(v*255),256)&255));checks++;
        }
    }
    for(unsigned i=0;i<2000000;i++) {
        union { unsigned u;float f; } x={.u=random_word()%0x43800001u};
        assert(sample_round(x.f)==roundf(x.f));checks++;
        x.u=random_word()%0x49800001u;
        if(i&1)x.f=-x.f;
        assert(sample_floor(x.f)==(int)floorf(x.f));checks++;
    }
    for(int i=-1024;i<=1024;i++) {
        float x=i;
        for(unsigned j=0;j<32;j++) {
            assert(sample_floor(x)==(int)floorf(x));checks++;x=nextafterf(x,-INFINITY);
        }
        x=i;
        for(unsigned j=0;j<32;j++) {
            assert(sample_floor(x)==(int)floorf(x));checks++;x=nextafterf(x,INFINITY);
        }
    }
    for(unsigned i=0;i<256;i++) {
        float x=i+.5f;
        for(unsigned j=0;j<32;j++) {
            assert(sample_round(x)==roundf(x));checks++;x=nextafterf(x,-INFINITY);
        }
        x=i+.5f;
        for(unsigned j=0;j<32;j++) {
            assert(sample_round(x)==roundf(x));checks++;x=nextafterf(x,INFINITY);
        }
    }
    for(unsigned i=0;i<256;i++){assert(indirect_scale(i)==ldexpf(1,i));checks++;}
    unsigned char compressed[2048],rgba[64*64*4],mask_bytes[8*8*4];uint32_t actual[64*64];
    for(unsigned i=0;i<sizeof(compressed);i++)compressed[i]=random_word();
    for(unsigned i=0;i<sizeof(rgba);i++)rgba[i]=random_word();
    for(unsigned i=0;i<sizeof(mask_bytes);i++)mask_bytes[i]=random_word();
    const unsigned sizes[]={1,3,4,5,7,8,9,13,16,31,32,63,64};
    for(unsigned format=0;format<2;format++)for(unsigned w=0;w<13;w++)for(unsigned h=0;h<13;h++)
    for(unsigned filter=0;filter<2;filter++)for(unsigned wrap_mode=0;wrap_mode<3;wrap_mode++) {
        FhTexture base={.pixels=format?rgba:compressed,.width=sizes[w],.height=sizes[h],.format=format?0x46:14,.mag_filter=filter,.wrap_s=wrap_mode,.wrap_t=wrap_mode};
        FhTexture mask={.pixels=mask_bytes,.width=8,.height=8,.format=0x46,.mag_filter=filter,.wrap_s=wrap_mode,.wrap_t=wrap_mode};
        unsigned width=(base.width+7)&~7u,height=(base.height+7)&~7u;
        assert(fh_texture_pack_fur(actual,width,height,&base,&mask));
        for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++) {
            unsigned sx=x<base.width?x:base.width-1,sy=y<base.height?y:base.height-1;
            float sample[4];fh_texture_sample(sample,&mask,(sx+.5f)/base.width,(sy+.5f)/base.height);
            unsigned bump=(unsigned)roundf(sample[3]*255)&248;
            unsigned offset=((y/8)*(width/8)+x/8)*64+mp_texture_morton(x,y);
            assert(actual[offset]==((fh_texture_pixel(&base,sx,sy)&0xffffff00)|((bump*255+124)/248)));checks++;
        }
        /* The per-call palette must never survive changed compressed bytes. */
        compressed[0]^=0x81;
        for(unsigned sample=0;sample<32;sample++) {
            float u=((int)(random_word()%8192)-4096)/1024.f,v=((int)(random_word()%8192)-4096)/1024.f;
            float expected[4],got[4];reference_sample(expected,&base,u,v);fh_texture_sample(got,&base,u,v);
            assert(!memcmp(expected,got,sizeof(got)));checks+=4;
        }
    }
    printf("PASS %u bounded sampling floor/round/scale comparisons\n",checks);
}
