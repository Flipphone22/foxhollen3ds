#ifndef FH_MOVIE_LZ4_H
#define FH_MOVIE_LZ4_H
#include <stddef.h>
#include <string.h>
/* Original bounded decoder for independent LZ4 blocks. No dictionary, wild
 * copies, unaligned word loads or reads beyond the encoded packet. */
/* Calls here are disjoint and length checked by the decoder. Fixed byte
 * copies retain ARM11 alignment safety; zero length performs no access. */
static inline void fh_movie_lz4_copy(unsigned char *d,const unsigned char *s,size_t n){
    if(n>8){memcpy(d,s,n);return;}
    switch(n){
    case 8:d[7]=s[7]; /* fall through */
    case 7:d[6]=s[6]; /* fall through */
    case 6:d[5]=s[5]; /* fall through */
    case 5:d[4]=s[4]; /* fall through */
    case 4:d[3]=s[3]; /* fall through */
    case 3:d[2]=s[2]; /* fall through */
    case 2:d[1]=s[1]; /* fall through */
    case 1:d[0]=s[0]; /* fall through */
    default:break;
    }
}
static inline int fh_movie_lz4_block(void *output,size_t capacity,const void *input,size_t bytes){
    unsigned char *out=output;const unsigned char *in=input;size_t p=0,n=0;
    while(p<bytes){
        unsigned token=in[p++];size_t count=token>>4;
        if(count==15){unsigned extra;do{
            if(p==bytes)return 0;
            extra=in[p++];
            if(count>capacity || extra>capacity-count)return 0;
            count+=extra;
        }while(extra==255);}
        if(count>bytes-p || count>capacity-n)return 0;
        fh_movie_lz4_copy(out+n,in+p,count);p+=count;n+=count;
        if(p==bytes)return n==capacity;
        if(bytes-p<2)return 0;
        unsigned distance=in[p]|(unsigned)in[p+1]<<8;p+=2;
        if(!distance || distance>n)return 0;
        count=(token&15)+4;
        if((token&15)==15){unsigned extra;do{
            if(p==bytes)return 0;
            extra=in[p++];
            if(count>capacity || extra>capacity-count)return 0;
            count+=extra;
        }while(extra==255);}
        if(count>capacity-n)return 0;
        /* Each chunk is disjoint even for repeated-byte/small-offset matches.
         * Doubling published history avoids a byte loop for long repeats. */
        size_t available=distance;
        while(count){size_t chunk=count<available?count:available;
            fh_movie_lz4_copy(out+n,out+n-available,chunk);n+=chunk;count-=chunk;available+=chunk;}
    }
    return 0;
}
#endif
