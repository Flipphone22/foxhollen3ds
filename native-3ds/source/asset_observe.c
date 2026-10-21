#include <stdint.h>
#include <stdio.h>
#include "fh_platform.h"
#include "fh_graphics.h"
#include "main/dll/FRONT/dll_44.h"
void *__real_loadAndDecompressDataFile(int,void*,int,uint32_t,int*,int,uint32_t);
void *__wrap_loadAndDecompressDataFile(int file,void *destination,int offset,uint32_t length,int *size,int index,uint32_t flags) {
    void *result=__real_loadAndDecompressDataFile(file,destination,offset,length,size,index,flags);
    static unsigned calls;
    if(calls++<100) {
        char message[256];const unsigned char *p=destination?destination:result;
        snprintf(message,sizeof(message),"asset file=%x offset=%x length=%u entry=%d flags=%u destination=%p result=%p header=%02x%02x%02x%02x dimensions=%02x%02x/%02x%02x\n",file,offset,(unsigned)length,index,(unsigned)flags,destination,result,p?p[0]:0,p?p[1]:0,p?p[2]:0,p?p[3]:0,p?p[10]:0,p?p[11]:0,p?p[12]:0,p?p[13]:0);
        fh_platform_log(message);
    }
    return result;
}
int __real_THPVideoDecode(void*,void*,void*,void*,void*);
uint64_t fh_video_decode_ticks;
extern void *fhTHPVideoGetRGB(const void*);
extern unsigned fhTHPVideoGetNativeFormat(const void*);
extern void fhTHPVideoGetTextureSize(const void*,unsigned*,unsigned*);
int __wrap_THPVideoDecode(void *source,void *y,void *u,void *v,void *work) {
    /* Join leased movie texture readers before decoding/reallocating pixels;
     * a post-write cache notification alone would be too late. */
    extern void fh_gx_dispatch_barrier(void);fh_gx_dispatch_barrier();
    static uint64_t elapsed,maximum;static unsigned frames,prepared;
    uint64_t start=fh_platform_ticks();
    int result=__real_THPVideoDecode(source,y,u,v,work);
    uint64_t ticks=fh_platform_ticks()-start;fh_video_decode_ticks+=ticks;elapsed+=ticks;if(ticks>maximum)maximum=ticks;
    frames++;
    if(!result){
        void *pixels=fhTHPVideoGetRGB(y);unsigned w=gAttractMoviePlayer.videoInfo.xSize,h=gAttractMoviePlayer.videoInfo.ySize;
        unsigned format=fhTHPVideoGetNativeFormat(pixels);fhTHPVideoGetTextureSize(pixels,&w,&h);
        /* Prepared frames are decoded into owned native images and explicitly
         * flushed by their commit. They do not enter the old texture cache. */
        if(pixels && format!=0x4c && format!=0x41)fh_gx_cache_range(pixels,fh_texture_source_bytes(format,w,h),1);
        if(format==0x4c || format==0x41)prepared++;
    }
    extern unsigned fh_movie_audio_buffers,fh_movie_audio_starved;
    if(!(frames&63)) {
        char message[160];snprintf(message,sizeof(message),"THP video frames=%u prepared=%u audio_buffers=%u audio_starved=%u avg_us=%llu max_us=%llu\n",frames,prepared,fh_movie_audio_buffers,fh_movie_audio_starved,(unsigned long long)(elapsed*1000/40500/frames),(unsigned long long)(maximum*1000/40500));fh_platform_log(message);
    }
    if(result) { char message[128];snprintf(message,sizeof(message),"THP decode error=%d expected=%ux%u source=%p\n",result,gAttractMoviePlayer.videoInfo.xSize,gAttractMoviePlayer.videoInfo.ySize,source);fh_platform_log(message); }
    return result;
}
