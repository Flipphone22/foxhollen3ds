#include "fh_install_skin.h"
#include <string.h>
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
int fh_install_skin_stream(const FhInstallVertexSource*s,unsigned normal,FhInstallSkinWeights*out,unsigned capacity){
    if(!s||!s->raw||s->world||normal>1)return 0;
    unsigned limit=normal?s->normals_count:s->positions_count;
    if(limit>capacity||(!out&&limit))return 0;
    if(limit)memset(out,0,limit*sizeof(*out));
    const unsigned char*p=s->raw;unsigned table=word(p+(normal?0xc8:0xa4));
    if(!table)return 1;
    unsigned count=half(p+(normal?0xae:0x8a)),base=word(p+(normal?0xcc:0xa8));
    unsigned stride=normal?s->normal_stride:6;
    if(!stride||table>s->bytes||count>(s->bytes-table)/0x74)goto invalid;
    for(unsigned i=0;i<count;i++){
        const unsigned char*e=p+table+i*0x74;
        uint64_t offset=(uint64_t)word(e+0x60)+e[0x72];unsigned n=half(e+0x70),a=e[0x6c],b=e[0x6d];
        if(a>=s->joints_count||b>=s->joints_count||offset%stride||offset/stride>limit||n>limit-offset/stride)goto invalid;
        unsigned relative=word(e+0x64);
        if(base>s->bytes||relative>s->bytes-base||n>(s->bytes-base-relative)/2)goto invalid;
        const unsigned char*weights=p+base+relative;
        for(unsigned j=0;j<n;j++){
            unsigned wa=weights[j*2],wb=weights[j*2+1];FhInstallSkinWeights entry={.present=1};
            if(a==b){if(wa+wb){entry.joint[0]=a;entry.weight[0]=wa+wb;entry.count=1;}}
            else{
                if(wa){entry.joint[0]=a;entry.weight[0]=wa;entry.count=1;}
                if(wb){unsigned k=entry.count++;entry.joint[k]=b;entry.weight[k]=wb;}
                if(entry.count==2&&entry.joint[0]>entry.joint[1]){
                    unsigned v=entry.joint[0];entry.joint[0]=entry.joint[1];entry.joint[1]=v;
                    v=entry.weight[0];entry.weight[0]=entry.weight[1];entry.weight[1]=v;
                }
            }
            FhInstallSkinWeights*dest=out+(unsigned)(offset/stride)+j;
            if(dest->present&&memcmp(dest,&entry,sizeof(entry)))goto invalid;
            *dest=entry;
        }
    }
    return 1;
invalid:
    if(limit)memset(out,0,limit*sizeof(*out));return 0;
}
