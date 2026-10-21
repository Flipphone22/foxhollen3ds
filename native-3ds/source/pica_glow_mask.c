#include "fh_pica_glow_mask.h"

static unsigned morton8(unsigned x, unsigned y)
{
    return (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) |
           ((y & 2) << 2) | ((x & 4) << 2) | ((y & 4) << 3);
}

int fh_pica_glow_mask_compile(uint8_t *dst, size_t capacity,
                              const uint8_t *rgba, size_t source_bytes,
                              unsigned width, unsigned height, size_t row_bytes)
{
    if (!dst || !rgba || width < 8 || height < 8 || width > 1024 || height > 1024 ||
        (width & (width-1)) || (height & (height-1)) || row_bytes < (size_t)width*4)
        return 0;
    size_t bytes=(size_t)width*height;
    if (row_bytes > (SIZE_MAX-(size_t)width*4)/(height-1)) return 0;
    size_t source_needed=(height-1)*row_bytes+(size_t)width*4;
    if (capacity < bytes || source_bytes < source_needed) return 0;
    uintptr_t d=(uintptr_t)dst, s=(uintptr_t)rgba;
    if (d > UINTPTR_MAX-bytes || s > UINTPTR_MAX-source_needed ||
        (d < s+source_needed && s < d+bytes)) return 0;
    for (unsigned y=0; y<height; ++y) {
        const uint8_t *row=rgba+(size_t)y*row_bytes;
        unsigned native_y=y;
        for (unsigned x=0; x<width; ++x) {
            size_t offset=((size_t)(native_y/8)*(width/8)+x/8)*64+morton8(x,native_y);
            unsigned mask=row[(size_t)x*4+3]&248u;
            dst[offset]=(uint8_t)((mask*255u+124u)/248u);
        }
    }
    return 1;
}

int fh_pica_glow_mask_rgb5a3(uint8_t *dst, size_t capacity,
                            const uint8_t *source, size_t source_bytes,
                            unsigned width, unsigned height)
{
    if (!dst || !source || width<8 || height<8 || width>1024 || height>1024 ||
        (width&(width-1)) || (height&(height-1))) return 0;
    size_t bytes=(size_t)width*height;
    uintptr_t d=(uintptr_t)dst,s=(uintptr_t)source;
    if (capacity<bytes || source_bytes<2*bytes || d>UINTPTR_MAX-bytes ||
        s>UINTPTR_MAX-2*bytes || (d<s+2*bytes && s<d+bytes)) return 0;
    for (unsigned y=0;y<height;++y) for (unsigned x=0;x<width;++x) {
        size_t input=(((size_t)(y/4)*(width/4)+x/4)*16+(y%4)*4+x%4)*2;
        unsigned word=((unsigned)source[input]<<8)|source[input+1];
        unsigned alpha=word&0x8000?255:(((word>>12)&7)<<5)|(((word>>12)&7)<<2)|(((word>>12)&7)>>1);
        size_t output=((size_t)(y/8)*(width/8)+x/8)*64+morton8(x,y);
        dst[output]=(uint8_t)(((alpha&248)*255+124)/248);
    }
    return 1;
}
