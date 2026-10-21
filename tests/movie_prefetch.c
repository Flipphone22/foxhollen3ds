#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "fh_movie_prefetch.h"
u32 __ctru_heap_size=68*1024*1024,__stacksize__=512*1024;
aptHookCookie *test_hooks;
void (*test_apt_callback)(APT_HookType,void*);
void fh_platform_panic(const char *message){fprintf(stderr,"PANIC: %s\n",message);abort();}
int main(void){
    unsigned char original[65536],out[32768];for(unsigned i=0;i<sizeof(original);i++)original[i]=(i*73+i/251)&255;
    unsigned cases=0;
    for(unsigned mode=1;mode<=2;mode++){
        FILE *f=tmpfile();assert(f && fwrite(original,1,sizeof(original),f)==sizeof(original));fflush(f);
        fh_movie_prefetch_mode=mode;fh_movie_prefetch_open(f,32768,sizeof(original));
        FhMoviePrefetchStats s;fh_movie_prefetch_stats(&s);assert(s.active);
        for(unsigned i=0;i<256;i++){
            unsigned n=1+(i*197)%32768,at=(i*997)%(65536-n);int result;
            fh_movie_prefetch_hint(at,n);
            if(!(i&7)){assert(test_apt_callback);test_apt_callback(APTHOOK_ONSLEEP,NULL);fh_movie_prefetch_hint(0,1);test_apt_callback(APTHOOK_ONWAKEUP,NULL);}
            assert(fh_movie_prefetch_read(at,out,n,&result) && result==(int)n && !memcmp(out,original+at,n));
            assert(!fh_movie_prefetch_read(at,out,n,&result));cases++;
        }
        /* A nonmatching demand joins the private read before direct FILE I/O. */
        fh_movie_prefetch_hint(100,1000);int result;
        assert(!fh_movie_prefetch_read(1000,out,100,&result));assert(!fseek(f,1000,SEEK_SET));
        assert(fread(out,1,100,f)==100 && !memcmp(out,original+1000,100));
        assert(fh_movie_prefetch_read(100,out,1000,&result) && !memcmp(out,original+100,1000));
        fh_movie_prefetch_stats(&s);assert(s.hits==257 && !s.errors && s.checks==(mode==2?257u:0u));
        assert(!ftruncate(fileno(f),128));fh_movie_prefetch_hint(1024,4096);
        assert(fh_movie_prefetch_read(1024,out,4096,&result) && result==-1);
        fh_movie_prefetch_stats(&s);assert(s.errors==1);
        fh_movie_prefetch_close();assert(!test_apt_callback);fclose(f);
    }
    for(unsigned i=0;i<8;i++){
        FILE *f=tmpfile();assert(f && fwrite(original,1,sizeof(original),f)==sizeof(original));fflush(f);
        fh_movie_prefetch_open(f,32768,sizeof(original));fh_movie_prefetch_hint(0,32768);
        fh_movie_prefetch_close();fclose(f);
    }
    char path[]="/tmp/foxhollow-stream-XXXXXX";int fd=mkstemp(path);assert(fd>=0);
    assert(write(fd,original,sizeof(original))==sizeof(original));close(fd);
    FILE *main_file=fopen(path,"rb");assert(main_file);
    fh_movie_prefetch_open(main_file,32768,sizeof(original));
    for(unsigned pass=0;pass<16;pass++){
        /* Independent cursors, concurrent requests, partial last chunk and
         * restart/close during a read. The movie context remains alive. */
        unsigned base=pass*32,bytes=sizeof(original)-base;
        fh_stream_prefetch_open(path,base,bytes);
        for(unsigned at=0;at<bytes;){
            fh_movie_prefetch_hint(100,1234);
            if(pass&1){test_apt_callback(APTHOOK_ONSUSPEND,NULL);test_apt_callback(APTHOOK_ONRESTORE,NULL);}
            unsigned n=bytes-at;if(n>32768)n=32768;int result;
            assert(fh_stream_prefetch_read(at,out,n,&result) && result==(int)n && !memcmp(out,original+base+at,n));
            at+=n;assert(fh_movie_prefetch_read(100,out,1234,&result) && result==1234 && !memcmp(out,original+100,1234));cases++;
        }
        int result;assert(!fh_stream_prefetch_read(bytes,out,32,&result));
        assert(fh_stream_prefetch_read(32,out,64,&result) && result==64 && !memcmp(out,original+base+32,64));
        fh_stream_prefetch_close();assert(test_apt_callback);
    }
    fh_stream_prefetch_open(path,32,65504);fh_stream_prefetch_close();
    fh_movie_prefetch_close();assert(!test_apt_callback);fclose(main_file);unlink(path);
    printf("PASS: %u byte-exact queued reads, checked hits, misses/seeks, suspend/resume, read failure and joined in-flight closes\n",cases);
}
