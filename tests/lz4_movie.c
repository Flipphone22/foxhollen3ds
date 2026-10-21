#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include "fh_lz4_block.h"
#include "fh_prepared_movie.h"
extern int LZ4_decompress_safe(const char*,char*,int,int);
extern int LZ4_compress_default(const char*,char*,int,int);
static uint32_t random_state=1;
static unsigned random_byte(void){random_state=random_state*1664525u+1013904223u;return random_state>>24;}
int main(int argc,char **argv){
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);
    unsigned char *movie=malloc(size);assert(movie && fread(movie,1,size,f)==(size_t)size);fclose(f);
    unsigned at=fh_movie_be32(movie+40),frames=fh_movie_be32(movie+20),decoded=0;
    for(unsigned i=0;i<frames;i++){
        assert(at+16u<=(unsigned)size);unsigned video=fh_movie_be32(movie+at+8);
        const unsigned char *p=movie+at+16;assert(at+16+video<=(unsigned)size);
        size_t bytes=fh_movie_video_bytes(p,video,512,384);assert(bytes);
        if(fh_movie_le32(p+28)==2){
            unsigned char *out=malloc(bytes),*ref=malloc(bytes);assert(out && ref);
            assert(fh_lz4_block(out,bytes,p+32,video-36));
            assert(LZ4_decompress_safe((const char*)p+32,(char*)ref,video-36,bytes)==(int)bytes);
            assert(!memcmp(out,ref,bytes) && crc32(0,out,bytes)==fh_movie_le32(p+video-4));
            for(unsigned cut=1;cut<=4;cut++)assert(!fh_lz4_block(out,bytes,p+32,video-36-cut));
            free(out);free(ref);decoded++;
        }
        unsigned audio=fh_movie_be32(movie+at+12);at+=(16+video+audio+31)&~31u;
    }
    assert(at==(unsigned)size && frames==1797 && decoded);
    free(movie);unsigned cases=0;
    for(unsigned n=1;n<=65536;n+=137){
        unsigned char *data=malloc(n),*packed=malloc(n+n/255+32),*out=malloc(n),*ref=malloc(n);
        assert(data && packed && out && ref);
        for(unsigned mode=0;mode<4;mode++){
            for(unsigned i=0;i<n;i++)data[i]=mode==0?0:mode==1?i%3:mode==2?i%257:random_byte();
            int bytes=LZ4_compress_default((const char*)data,(char*)packed,n,n+n/255+32);assert(bytes>0);
            assert(fh_lz4_block(out,n,packed,bytes) && !memcmp(data,out,n));cases++;
            /* Mutations need not be invalid, but every accepted output must
             * agree with the independent decoder at the exact output bound. */
            for(unsigned j=0;j<8;j++){
                unsigned pos=(random_byte()*257u+random_byte())%(unsigned)bytes;unsigned old=packed[pos];packed[pos]^=1u<<(j&7);
                int accepted=fh_lz4_block(out,n,packed,bytes);
                int count=LZ4_decompress_safe((const char*)packed,(char*)ref,bytes,n);
                if(accepted && count==(int)n)assert(!memcmp(out,ref,n));
                packed[pos]=old;cases++;
            }
        }
        free(data);free(packed);free(out);free(ref);
    }
    printf("PASS: %u full movie blocks, %u synthetic/mutated packets, bounded output and independent decode\n",decoded,cases);
}
