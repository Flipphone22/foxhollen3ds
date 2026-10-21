#include "fh_install_vertex.h"
#include <string.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
static int array(const unsigned char**out,const unsigned char*p,unsigned bytes,unsigned field,unsigned count,unsigned stride){
    *out=NULL;if(!count)return 1;
    unsigned at=word(p+field);
    if(at>bytes||count>(bytes-at)/stride)return 0;
    *out=p+at;return 1;
}
int fh_install_vertex_source(FhInstallVertexSource*out,const void*source,unsigned bytes,unsigned world){
    if(!out)return 0;memset(out,0,sizeof(*out));
    if(!source||world>1||bytes<(world?164u:252u)||bytes>4*1024*1024)return 0;
    const unsigned char*p=source;FhInstallVertexSource s={0};s.raw=p;s.bytes=bytes;s.world=world;
    s.positions_count=half(p+(world?0x90:0xe4));s.normals_count=world?0:half(p+0xe6);
    s.colors_count=half(p+(world?0x94:0xe8));s.uvs_count=half(p+(world?0x96:0xea));
    s.normal_stride=world?0:p[0x24]&8?9:3;s.joints_count=world?0:p[0xf3];s.blends_count=world?0:p[0xf4];
    s.shaders_count=p[world?0xa2:0xf8];s.rigid=world||!s.joints_count;s.fixed_world=!s.rigid&&word(p+0xa4)!=0;
    if(!array(&s.positions,p,bytes,world?0x58:0x28,s.positions_count,6)||
       !array(&s.colors,p,bytes,world?0x5c:0x30,s.colors_count,2)||
       !array(&s.uvs,p,bytes,world?0x60:0x34,s.uvs_count,4))return 0;
    if(!world&&(!array(&s.normals,p,bytes,0x2c,s.normals_count,s.normal_stride)||
       !array(&s.joints,p,bytes,0x3c,s.joints_count,28)||
       !array(&s.blends,p,bytes,0x54,s.blends_count,4)))return 0;
    *out=s;return 1;
}
int fh_install_vertex_binding(const FhInstallVertexSource*s,unsigned joint,FhModelBinding*out){
    if(!s||!s->raw||!out)return 0;
    FhModelBinding binding={0,0,4,0};unsigned n=s->joints_count;
    if(s->rigid){if(joint)return 0;*out=binding;return 1;}
    if(s->fixed_world){if(joint==n){binding.a=binding.b=n;*out=binding;return 1;}if(joint>n)joint--;}
    if(joint<n){binding.a=binding.b=joint;*out=binding;return 1;}
    if(joint>=n+s->blends_count)return 0;
    const unsigned char*p=s->blends+(joint-n)*4;
    if(p[0]>=n||p[1]>=n||p[2]>4)return 0;
    binding=(FhModelBinding){p[0],p[1],p[2],1};*out=binding;return 1;
}
static unsigned read_index(const unsigned char*p,unsigned*at,unsigned n){
    if(!n)return 65535;
    unsigned value=n==1?p[*at]:half(p+*at);*at+=n;return value;
}
int fh_install_vertex_reference(const FhInstallVertexSource*s,const FhInstallDrawSource*d,
                                const void*packet,unsigned bytes,FhModelRef*out){
    if(!s||!s->raw||!d||!packet||!out||d->shader>=s->shaders_count)return 0;
    const unsigned*l=d->layout;
    if(l[0]>9||l[1]<1||l[1]>2||l[2]>2||l[3]>2||l[4]<1||l[4]>2||l[5]>2||
       bytes!=l[0]+l[1]+l[2]+l[3]+l[4]*l[5])return 0;
    if(s->world&&(l[0]||l[2]||!l[3]))return 0;
    const unsigned char*p=packet;unsigned at=l[0];FhModelRef ref;
    ref.position=read_index(p,&at,l[1]);ref.normal=read_index(p,&at,l[2]);
    ref.color=read_index(p,&at,l[3]);ref.uv=l[5]?read_index(p,&at,l[4]):65535;
    ref.material=l[5]==2?read_index(p,&at,l[4]):d->shader;ref.joint=0;
    if(s->rigid){if(l[0])return 0;}
    else if(l[0]){
        unsigned slot=p[0];if(slot>27||slot%3)return 0;
        if(s->fixed_world&&slot==27)ref.joint=s->joints_count;
        else{
            unsigned joint=d->palette[slot/3];if(joint==65535)return 0;
            if(s->fixed_world&&joint>=s->joints_count)joint++;
            if(joint>65535)return 0;ref.joint=joint;
        }
    }
    if(ref.position==65535||ref.position>=s->positions_count||
       (ref.normal!=65535&&ref.normal>=s->normals_count)||
       (ref.color!=65535&&ref.color>=s->colors_count)||
       (ref.uv!=65535&&ref.uv>=s->uvs_count)||
       (l[5]==2&&ref.material>=s->uvs_count))return 0;
    FhModelBinding binding;if(!fh_install_vertex_binding(s,ref.joint,&binding))return 0;
    *out=ref;return 1;
}
