#include "fh_install_triangles.h"
#include <string.h>
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
int fh_install_triangles(const void*source,uint32_t bytes,unsigned stride,
                         FhInstallTriangleEmit emit,void*context,FhInstallTriangles*out){
    if(out)memset(out,0,sizeof(*out));
    if(!out||(!source&&bytes)||bytes>FH_INSTALL_TRI_LIST_CAP||!stride||stride>FH_INSTALL_TRI_STRIDE_CAP)return FH_INSTALL_TRI_INVALID;
    const unsigned char*p=source;FhInstallTriangles found={0,FH_INSTALL_TRI_NO_VAT};
    /* This preflight is installation work, never performed on a game frame. */
    for(unsigned at=0;at<bytes;){
        unsigned op=p[at++];if(!op)continue;
        unsigned kind=op&0xf8,vat=op&7;
        if((kind!=0x80&&kind!=0x90&&kind!=0x98&&kind!=0xa0)||bytes-at<2||
           (found.vat!=FH_INSTALL_TRI_NO_VAT&&found.vat!=vat))return FH_INSTALL_TRI_INVALID;
        found.vat=vat;unsigned n=half(p+at);at+=2;
        if(n>(bytes-at)/stride||(kind==0x80&&n%4)||(kind==0x90&&n%3))return FH_INSTALL_TRI_INVALID;
        found.triangles+=kind==0x80?n/4*2:kind==0x90?n/3:n>2?n-2:0;
        at+=n*stride;
    }
    if(emit)for(unsigned at=0;at<bytes;){
        unsigned op=p[at++];if(!op)continue;
        unsigned kind=op&0xf8,n=half(p+at);at+=2;const unsigned char*v=p+at;at+=n*stride;
#define EMIT(a,b,c) do{if(!emit(context,v+(a)*stride,v+(b)*stride,v+(c)*stride,stride))return FH_INSTALL_TRI_CANCEL;}while(0)
        if(kind==0x90){for(unsigned i=0;i<n;i+=3)EMIT(i,i+1,i+2);}
        else if(kind==0x80){for(unsigned i=0;i<n;i+=4){EMIT(i,i+1,i+2);EMIT(i,i+2,i+3);}}
        else if(kind==0x98){for(unsigned i=2;i<n;i++)EMIT(i-2+i%2,i-1-i%2,i);}
        else {for(unsigned i=2;i<n;i++)EMIT(0,i-1,i);}
#undef EMIT
    }
    *out=found;return FH_INSTALL_TRI_OK;
}
