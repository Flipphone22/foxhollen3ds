#include "fh_pica_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
int main(int argc,char **argv){
    assert(argc==5);unsigned w=atoi(argv[2]),h=atoi(argv[3]);
    assert(w>=8 && h>=8 && w<=1024 && h<=1024 && !(w&(w-1)) && !(h&(h-1)));
    size_t bytes=(size_t)w*h/2;unsigned char *data=malloc(bytes);assert(data);
    FILE *in=fopen(argv[1],"rb"),*out=fopen(argv[4],"wb");assert(in && out);
    assert(fread(data,1,bytes,in)==bytes && fgetc(in)==EOF);fclose(in);
    for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
        uint32_t p=fh_pica_etc1_pixel(data,w,x,y);unsigned char rgba[]={p>>24,p>>16,p>>8,p};
        assert(fwrite(rgba,1,4,out)==4);
    }
    fclose(out);free(data);return 0;
}
