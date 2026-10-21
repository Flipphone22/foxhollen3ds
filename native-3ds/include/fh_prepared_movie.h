#ifndef FH_PREPARED_MOVIE_H
#define FH_PREPARED_MOVIE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "fh_pica_texture.h"
static inline uint32_t fh_movie_le32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static inline uint32_t fh_movie_be32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static inline unsigned fh_movie_video_format(const void*data,size_t bytes){
    if(!data||bytes<8)return 0;
    return !memcmp(data,"FHPICA01",8)?FH_TEXTURE_PICA_ETC1:!memcmp(data,"FHRGB801",8)?FH_TEXTURE_PICA_RGB8:0;
}
/* Version 1 is deliberately restricted to the verified PAL movie. No short
 * fixture can replace the full asset. Host preparation hashes the complete
 * original; runtime checks that identity tag and the mounted original header. */
static inline int fh_movie_header_valid(const unsigned char *h,size_t bytes,const unsigned char *original,const unsigned char *info,uint32_t offset,uint32_t original_bytes){
    static const unsigned char hash[32]={0x19,0xd0,0x8f,0xf1,0xab,0x4c,0x18,0x58,0x93,0xdf,0x57,0xd8,0x9e,0x48,0x8a,0xa0,0xb5,0xc4,0xe6,0x56,0xbe,0xbb,0x5d,0x70,0x31,0xde,0x65,0x22,0x8a,0x21,0x66,0xe6};
    if(bytes<192 || bytes>300*1024*1024 || original_bytes!=48931320 || memcmp(h,"THP\0\0\1\0\0",8) || memcmp(h,original,8) || memcmp(h+12,original+12,12))return 0;
    unsigned format=fh_movie_video_format(h+128,8);
    if(!format || memcmp(h+160,hash,32) || memcmp(h+48,info,40))return 0;
    if(fh_movie_le32(h+136)!=offset || fh_movie_le32(h+140)!=original_bytes || fh_movie_le32(h+144)!=1797 || fh_movie_le32(h+148)!=1797 || fh_movie_le32(h+152)!=(format==FH_TEXTURE_PICA_RGB8?1u:12u) || (fh_movie_le32(h+156)!=1 && fh_movie_le32(h+156)!=2))return 0;
    if(format==FH_TEXTURE_PICA_RGB8&&fh_movie_le32(h+156)!=2)return 0;
    if(fh_movie_be32(h+20)!=1797 || fh_movie_be32(h+32)!=48 || fh_movie_be32(h+36)!=0 || fh_movie_be32(h+40)!=192 || fh_movie_be32(h+28)!=bytes-192)return 0;
    uint32_t maximum=fh_movie_be32(h+8),first=fh_movie_be32(h+24),last=fh_movie_be32(h+44);
    return maximum>=64 && maximum<=196608 && !(maximum&31) && first>=64 && first<=maximum && !(first&31) && last>=192 && last+16u<=bytes && !(last&31);
}
static inline size_t fh_movie_video_bytes(const void *data,size_t size,unsigned width,unsigned height){
    const unsigned char *p=data;
    unsigned format=fh_movie_video_format(p,size);
    if(!format || size<32 || width!=512 || height!=384)return 0;
    unsigned w=fh_movie_le32(p+8),h=fh_movie_le32(p+12);
    if(!((w==width && h==height) || (w==width/2 && h==height/2)))return 0;
    if(format==FH_TEXTURE_PICA_RGB8&&(w!=256||h!=192||fh_movie_le32(p+28)==1))return 0;
    unsigned pw=fh_texture_extent(w),ph=fh_texture_extent(h),bytes=format==FH_TEXTURE_PICA_RGB8?pw*ph*3:pw*ph/2;
    if(fh_movie_le32(p+16)!=pw || fh_movie_le32(p+20)!=ph || fh_movie_le32(p+24)!=bytes || fh_movie_le32(p+28)>2)return 0;
    if(fh_movie_le32(p+28)==2 && size<=36)return 0;
    if(fh_movie_le32(p+28)?(size<=32 || size>bytes+32):(size!=bytes+32))return 0;
    return bytes;
}
#endif
