#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main/dll/FRONT/attract_movie.h"
AttractMoviePlayer gAttractMoviePlayer;
extern void fhTHPVideoSetCompressedSize(u32),referenceSize(u32);
extern void *fhTHPVideoGetRGB(const void*),*referenceRGB(const void*);
extern s32 THPVideoDecode(void*,void*,void*,void*,void*),referenceDecode(void*,void*,void*,void*,void*);
int main(int argc,char **argv){
    assert(argc==5);unsigned w=atoi(argv[2]),h=atoi(argv[3]);size_t pixels=(size_t)w*h;
    FILE *f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long bytes=ftell(f);rewind(f);assert(bytes>0);
    unsigned char *source=malloc(bytes),*y=malloc(pixels),*u=malloc(pixels/4),*v=malloc(pixels/4);
    assert(source&&y&&u&&v&&fread(source,1,bytes,f)==(size_t)bytes);fclose(f);
    gAttractMoviePlayer.videoInfo=(AttractMovieVideoInfo){w,h};
    gAttractMoviePlayer.textureSet[0]=(AttractMovieTextureSet){y,u,v,0};
    referenceSize(bytes);fhTHPVideoSetCompressedSize(bytes);
    assert(referenceDecode(source,y,u,v,NULL)==0);
    assert(THPVideoDecode(source,y,u,v,NULL)==0);
    const unsigned char *a=referenceRGB(y),*b=fhTHPVideoGetRGB(y);assert(a&&b);
    unsigned maximum=0;unsigned long long total=0,changed=0;
    for(size_t i=0;i<pixels*4;i++){
        unsigned difference=abs((int)a[i]-b[i]);if(difference>maximum)maximum=difference;
        total+=difference;changed+=difference!=0;if((i&3)==3)assert(a[i]==255&&b[i]==255);
    }
    printf("{\"pixels\":%zu,\"max_channel_error\":%u,\"total_error\":%llu,\"changed_channels\":%llu}\n",pixels,maximum,total,changed);
    /* Desktop conversion truncates its matrix products; libjpeg rounds them. */
    assert(maximum<=2);
    f=fopen(argv[4],"wb");assert(f&&fwrite(b,4,pixels,f)==pixels);fclose(f);
    /* Invalid dimensions must fail instead of exposing a partial frame. */
    gAttractMoviePlayer.videoInfo.xSize=w+16;assert(THPVideoDecode(source,y,u,v,NULL)==19);
    assert(THPVideoDecode(NULL,y,u,v,NULL)==27);
    free(source);free(y);free(u);free(v);return 0;
}
