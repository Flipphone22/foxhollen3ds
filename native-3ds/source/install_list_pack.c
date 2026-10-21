#include "fh_install_list_pack.h"
#include <stdint.h>
#include <string.h>
typedef struct{unsigned char*output;const unsigned char*packets;unsigned stride,at,vat;} Writer;
static int same(unsigned a,unsigned b,unsigned c,const unsigned*t){return (a==t[0]&&b==t[1]&&c==t[2])||(a==t[1]&&b==t[2]&&c==t[0])||(a==t[2]&&b==t[0]&&c==t[1]);}
static unsigned next(unsigned a,unsigned b,unsigned length,const unsigned*face){
    if((length-2)%2){unsigned v=a;a=b;b=v;}
    for(unsigned i=0;i<3;i++)if(same(a,b,face[i],face))return face[i];return ~0u;
}
static unsigned strip(const unsigned*indices,unsigned triangles,unsigned at,unsigned rotation){
    const unsigned*f=indices+at*3;unsigned a=f[(rotation+1)%3],b=f[(rotation+2)%3],length=3;
    for(unsigned i=at+1;i<triangles&&length<65535;i++){
        unsigned c=next(a,b,length,indices+i*3);if(c==~0u)break;a=b;b=c;length++;
    }return length;
}
static void header(Writer*w,unsigned op,unsigned count){
    if(w->output){w->output[w->at]=op|w->vat;w->output[w->at+1]=count>>8;w->output[w->at+2]=count;}w->at+=3;
}
static void vertex(Writer*w,unsigned i){if(w->output)memcpy(w->output+w->at,w->packets+i*w->stride,w->stride);w->at+=w->stride;}
static void triangles(Writer*w,const unsigned*indices,unsigned count){
    for(unsigned at=0;at<count;){unsigned n=count-at;if(n>65535)n=65535;header(w,0x90,n);for(unsigned i=0;i<n;i++)vertex(w,indices[at+i]);at+=n;}
}
static unsigned encode(Writer*w,const unsigned*indices,unsigned count,unsigned ordered){
    if(!ordered)triangles(w,indices,count);
    else{
        unsigned at=0,pending=0;
        while(at<count/3){
            unsigned best=3,rotation=0;for(unsigned r=0;r<3;r++){unsigned n=strip(indices,count/3,at,r);if(n>best){best=n;rotation=r;}}
            if(best>3){
                if(at>pending)triangles(w,indices+pending*3,(at-pending)*3);header(w,0x98,best);
                const unsigned*f=indices+at*3;for(unsigned i=0;i<3;i++)vertex(w,f[(rotation+i)%3]);
                unsigned a=f[(rotation+1)%3],b=f[(rotation+2)%3];
                for(unsigned length=3;length<best;length++){
                    unsigned c=next(a,b,length,indices+(at+length-2)*3);vertex(w,c);a=b;b=c;
                }
                at+=best-2;pending=at;
            }else at++;
        }
        if(at>pending)triangles(w,indices+pending*3,(at-pending)*3);
    }
    unsigned padded=(w->at+31)&~31u;if(w->output)memset(w->output+w->at,0,padded-w->at);return padded;
}
static int overlap(const void*a,unsigned an,const void*b,unsigned bn){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(x>UINTPTR_MAX-an||y>UINTPTR_MAX-bn)return 1;
    return x<y+bn&&y<x+an;
}
int fh_install_list_pack(void*output,unsigned capacity,const unsigned*indices,unsigned count,const void*packets,unsigned vertices,unsigned stride,unsigned vat,unsigned*bytes){
    if(bytes)*bytes=0;
    if(!output||!bytes||!indices||!packets||!count||count%3||count>32768*3||!vertices||vertices>32768||!stride||stride>32||vat>7)return 0;
    for(unsigned i=0;i<count;i++)if(indices[i]>=vertices)return 0;
    Writer a={NULL,packets,stride,0,vat},b=a;unsigned raw=encode(&a,indices,count,0),ordered=encode(&b,indices,count,1),small=ordered<raw?ordered:raw;
    if(small>capacity||small>65535||overlap(output,small,indices,count*4)||overlap(output,small,packets,vertices*stride))return 0;
    Writer w={output,packets,stride,0,vat};*bytes=encode(&w,indices,count,ordered<raw);return 1;
}
