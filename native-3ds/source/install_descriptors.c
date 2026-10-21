#include "fh_install_descriptors.h"
#include <string.h>
typedef struct {const unsigned char*p;unsigned bytes,bit,bad;} Bits;
static unsigned take(Bits*b,unsigned n){
    if(n>16||b->bad||b->bit>b->bytes*8||n>b->bytes*8-b->bit){b->bad=1;return 0;}
    unsigned value=0;for(unsigned i=0;i<n;i++,b->bit++)value|=((b->p[b->bit/8]>>(b->bit%8))&1)<<i;
    return value;
}
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static const unsigned char*range(const unsigned char*p,unsigned bytes,unsigned at,unsigned n){return at<=bytes&&n<=bytes-at?p+at:NULL;}
static int walk(const unsigned char*p,unsigned bytes,unsigned world,FhInstallDrawEmit emit,void*context,unsigned*draws){
    if(bytes<(world?164u:252u)||bytes>4*1024*1024)return 0;
    unsigned shader_count=p[world?0xa2:0xf8],list_count=p[world?0xa1:0xf5];
    const unsigned char*shaders=range(p,bytes,word(p+(world?0x64:0x38)),shader_count*68);
    const unsigned char*lists=range(p,bytes,word(p+(world?0x68:0xd0)),list_count*28);
    if(!shaders||!lists)return 0;
    *draws=0;
    if(!world&&!list_count&&!shader_count&&!word(p+0xd4)&&!half(p+0xd8))return 1;
    for(unsigned stream=0;stream<(world?3u:1u);stream++){
        unsigned at=word(p+(world?0x78+stream*4:0xd4)),length=half(p+(world?0x84+stream*2:0xd8));
        if(world&&!length)continue;
        const unsigned char*instructions=range(p,bytes,at,length);if(!instructions)return 0;
        Bits b={instructions,length,0,0};FhInstallDrawSource d={0};d.stream=stream;unsigned has_shader=0,has_layout=0;
        for(unsigned i=0;i<15;i++)d.palette[i]=world?0:65535;
        if(!world&&(half(p+2)&0x8000)){
            if(take(&b,4)!=3)return 0;
            d.layout[0]=p[0xf3]>1;d.layout[1]=1+take(&b,1);d.layout[4]=1+take(&b,1);d.layout[5]=1;
            if(take(&b,4)!=4)return 0;
            unsigned count=take(&b,4);for(unsigned i=0;i<count;i++)d.palette[i]=take(&b,8);
            if(take(&b,4)!=2)return 0;
            d.bit_offset=b.bit;d.list=take(&b,8);
            if(b.bad||d.list>=list_count||!shader_count)return 0;
            const unsigned char*entry=lists+d.list*28;
            if(!range(p,bytes,word(entry),half(entry+4)))return 0;
            if(emit&&!emit(context,&d))return -1;
            ++*draws;continue;
        }
        unsigned terminated=0;
        for(unsigned steps=0;steps<65536;steps++){
            unsigned op=take(&b,4);if(b.bad)return 0;
            if(op==5){terminated=1;break;}
            if(op==1){d.shader=take(&b,6);if(b.bad||d.shader>=shader_count)return 0;has_shader=1;}
            else if(op==3){
                if(!has_shader)return 0;
                const unsigned char*s=shaders+d.shader*68;memset(d.layout,0,sizeof(d.layout));
                if(world){
                    d.layout[1]=1+take(&b,1);d.layout[3]=1+take(&b,1);d.layout[4]=1+take(&b,1);
                    d.layout[5]=word(s+0x3c)&0x80000000?1:s[0x41];
                }else{
                    d.layout[5]=s[0x41];
                    if(p[0xf3]>1){
                        unsigned flags=half(p+0xe2),refs=(word(s+8)&&(flags&0xc))||(word(s+0x14)&&(flags&0xe00));
                        d.layout[0]=1+p[0xfa]+(refs?(word(s+0x34)!=0xffffffff?3:1):0);
                        if(d.layout[0]>9)return 0;
                    }
                    d.layout[1]=1+take(&b,1);d.layout[2]=s[0x40]&1?1+take(&b,1):0;
                    d.layout[3]=s[0x40]&2?1+take(&b,1):0;d.layout[4]=1+take(&b,1);
                }
                if(b.bad||d.layout[5]>2)return 0;has_layout=1;
            }else if(op==2){
                d.bit_offset=b.bit;d.list=take(&b,8);
                if(b.bad||!has_layout||!has_shader||d.list>=list_count)return 0;
                const unsigned char*entry=lists+d.list*28;
                if(!range(p,bytes,word(entry),half(entry+4)))return 0;
                if(emit&&!emit(context,&d))return -1;
                ++*draws;
            }else if(op==4){
                unsigned count=take(&b,4);
                for(unsigned i=0;i<count;i++){unsigned value=take(&b,8);if(!world)d.palette[i]=value;}
                if(b.bad)return 0;
            }else return 0;
        }
        if(!terminated)return 0;
    }
    return 1;
}
int fh_install_descriptors(const void*source,uint32_t bytes,unsigned world,FhInstallDrawEmit emit,void*context,unsigned*draws){
    if(draws)*draws=0;
    if(!source||!draws||world>1)return 0;
    unsigned total=0;int result=walk(source,bytes,world,NULL,NULL,&total);
    if(result!=1)return result;
    if(emit){result=walk(source,bytes,world,emit,context,&total);if(result!=1)return result;}
    *draws=total;return 1;
}
