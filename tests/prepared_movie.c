#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "main/dll/FRONT/attract_movie.h"
#include "fh_prepared_movie.h"
#include "fh_graphics.h"
AttractMoviePlayer gAttractMoviePlayer;
extern void fhTHPVideoSetCompressedSize(u32);
extern void *fhTHPVideoGetRGB(const void*);
extern unsigned fhTHPVideoGetNativeFormat(const void*);
extern void fhTHPVideoGetTextureSize(const void*,unsigned*,unsigned*);
extern s32 THPVideoDecode(void*,void*,void*,void*,void*);
static void put(unsigned char *p,unsigned v){for(unsigned i=0;i<4;i++)p[i]=v>>(8*i);}
static unsigned char *load(const char *name,size_t *size){
    FILE *f=fopen(name,"rb");assert(f);fseek(f,0,SEEK_END);*size=ftell(f);rewind(f);
    unsigned char *p=malloc(*size);assert(p && fread(p,1,*size,f)==*size);fclose(f);return p;
}
int main(int argc,char **argv){
    assert(argc==4);size_t native_size,jpeg_size,header_size;
    unsigned char *header=load(argv[3],&header_size);assert(header_size==288);
    unsigned total=fh_movie_le32(header+284),offset=fh_movie_le32(header+280);
    assert(fh_movie_header_valid(header,total,header+192,header+240,offset,48931320));
    const unsigned fields[]={0,4,8,12,16,20,24,28,32,36,40,44,48,128,136,140,144,148,152,156,160};
    for(unsigned i=0;i<sizeof(fields)/sizeof(*fields);i++){
        header[fields[i]]^=128;assert(!fh_movie_header_valid(header,total,header+192,header+240,offset,48931320));header[fields[i]]^=128;
    }
    assert(!fh_movie_header_valid(header,total-32,header+192,header+240,offset,48931320));free(header);
    unsigned char *encoded=load(argv[1],&native_size),*jpeg=load(argv[2],&jpeg_size);
    assert(native_size==131104 && fh_movie_video_bytes(encoded,native_size,512,384)==131072);
    unsigned char identifiers[3][3];void *owned[3];
    gAttractMoviePlayer.videoInfo=(AttractMovieVideoInfo){512,384};
    for(unsigned i=0;i<3;i++)gAttractMoviePlayer.textureSet[i]=(AttractMovieTextureSet){&identifiers[i][0],&identifiers[i][1],&identifiers[i][2],0};
    for(unsigned i=0;i<3;i++){
        fhTHPVideoSetCompressedSize(native_size);
        assert(!THPVideoDecode(encoded,&identifiers[i][0],&identifiers[i][1],&identifiers[i][2],NULL));
        owned[i]=fhTHPVideoGetRGB(&identifiers[i][0]);assert(owned[i] && !memcmp(owned[i],encoded+32,131072));
        assert(fhTHPVideoGetNativeFormat(owned[i])==FH_TEXTURE_PICA_ETC1);
        for(unsigned j=0;j<i;j++)assert(owned[i]!=owned[j]);
    }
    unsigned char *original=malloc(131072);memcpy(original,encoded+32,131072);
    memset(encoded+32,0,131072);for(unsigned i=0;i<3;i++)assert(!memcmp(owned[i],original,131072));
    memcpy(encoded+32,original,131072);
    unsigned char *packed=malloc(compressBound(131072)+32);memcpy(packed,encoded,32);put(packed+28,1);
    uLongf packed_size=compressBound(131072);assert(compress2(packed+32,&packed_size,original,131072,1)==Z_OK);
    fhTHPVideoSetCompressedSize(packed_size+32);
    assert(!THPVideoDecode(packed,&identifiers[0][0],&identifiers[0][1],&identifiers[0][2],NULL));
    assert(!memcmp(fhTHPVideoGetRGB(&identifiers[0][0]),original,131072));
    packed[32+packed_size-1]^=1;
    assert(THPVideoDecode(packed,&identifiers[0][0],&identifiers[0][1],&identifiers[0][2],NULL)==3);
    /* Independent LZ4 encoding: one zero literal, overlapping zero run,
     * five final literals and decoded CRC. Exercise the real shim branch. */
    memcpy(packed,encoded,32);put(packed+28,2);unsigned at=32,extra=131072-6-19;
    packed[at++]=0x1f;packed[at++]=0;packed[at++]=1;packed[at++]=0;
    while(extra>=255){packed[at++]=255;extra-=255;}packed[at++]=extra;
    packed[at++]=0x50;memset(packed+at,0,5);at+=5;
    unsigned char *zeros=calloc(1,131072);assert(zeros);
    put(packed+at,crc32(0,zeros,131072));at+=4;fhTHPVideoSetCompressedSize(at);
    assert(!THPVideoDecode(packed,&identifiers[0][0],&identifiers[0][1],&identifiers[0][2],NULL));
    assert(!memcmp(fhTHPVideoGetRGB(&identifiers[0][0]),zeros,131072));free(zeros);
    packed[at-1]^=1;
    assert(THPVideoDecode(packed,&identifiers[0][0],&identifiers[0][1],&identifiers[0][2],NULL)==3);
    for(unsigned size=0;size<32;size++)assert(!fh_movie_video_bytes(encoded,size,512,384));
    for(unsigned field=8;field<32;field++){
        encoded[field]^=128;assert(!fh_movie_video_bytes(encoded,native_size,512,384));encoded[field]^=128;
    }
    assert(!fh_movie_video_bytes(encoded,native_size-1,512,384));assert(!fh_movie_video_bytes(encoded,native_size,256,384));
    FhTexture t={.pixels=original,.width=512,.height=384,.format=FH_TEXTURE_PICA_ETC1};
    unsigned char *gpu=malloc(131072+32);memset(gpu,0xa5,131072+32);
    assert(fh_texture_native_format(&t)==12 && fh_texture_source_bytes(t.format,512,384)==131072);
    assert(fh_texture_native_bytes(&t,512,512)==131072 && !fh_texture_native_bytes(&t,512,384));
    assert(!fh_texture_convert(gpu,131071,512,512,&t,131072));assert(!fh_texture_convert(gpu,131072,512,512,&t,131071));
    assert(!fh_texture_convert(gpu,131072,1024,512,&t,131072));
    assert(fh_texture_convert(gpu,131072,512,512,&t,131072) && !memcmp(gpu,original,131072));
    for(unsigned i=131072;i<131104;i++)assert(gpu[i]==0xa5);
    fhTHPVideoSetCompressedSize(jpeg_size);
    assert(!THPVideoDecode(jpeg,&identifiers[0][0],&identifiers[0][1],&identifiers[0][2],NULL));
    assert(fhTHPVideoGetNativeFormat(fhTHPVideoGetRGB(&identifiers[0][0]))==0x46);
    fhTHPVideoSetCompressedSize(native_size);
    assert(!THPVideoDecode(encoded,&identifiers[0][0],&identifiers[0][1],&identifiers[0][2],NULL));
    assert(fhTHPVideoGetNativeFormat(fhTHPVideoGetRGB(&identifiers[0][0]))==FH_TEXTURE_PICA_ETC1);
    unsigned char *half=malloc(32800);memcpy(half,encoded,32800);
    put(half+8,256);put(half+12,192);put(half+16,256);put(half+20,256);put(half+24,32768);
    assert(fh_movie_video_bytes(half,32800,512,384)==32768);
    fhTHPVideoSetCompressedSize(32800);
    assert(!THPVideoDecode(half,&identifiers[1][0],&identifiers[1][1],&identifiers[1][2],NULL));
    unsigned w=512,h=384;fhTHPVideoGetTextureSize(fhTHPVideoGetRGB(&identifiers[1][0]),&w,&h);assert(w==256 && h==192);
    assert(!memcmp(fhTHPVideoGetRGB(&identifiers[1][0]),half+32,32768));
    fhTHPVideoSetCompressedSize(jpeg_size);assert(!THPVideoDecode(jpeg,&identifiers[1][0],&identifiers[1][1],&identifiers[1][2],NULL));
    fhTHPVideoGetTextureSize(fhTHPVideoGetRGB(&identifiers[1][0]),&w,&h);assert(w==512 && h==384);free(half);
    free(encoded);free(jpeg);free(original);free(packed);free(gpu);
    puts("PASS: owned triple buffers, source reuse, compressed blocks/checksum, malformed headers, native texture bounds and JPEG/native switching");
}
