/* Actual devkitARM newlib, deterministic read-only device. Counts low-level
 * requests, not filesystem duration. No console/GPU performance inference. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <reent.h>
#include <errno.h>
#include <assert.h>
#include "fh_asset_staging.h"
static unsigned position, reads, bytes_read, largest, seeks, file_size=2*1024*1024;
static unsigned char heap[1024*1024], stage[32768];
static unsigned heap_used, short_limit;
unsigned results[3][5], failure_line;
/* Single-thread harness; filesystem and locks are the mocked boundary. */
static struct _reent reent_state=_REENT_INIT(reent_state);
struct _reent *__getreent(void){return &reent_state;}
void __libc_lock_acquire_recursive(_LOCK_RECURSIVE_T *p){}
void __libc_lock_release_recursive(_LOCK_RECURSIVE_T *p){}
_ssize_t _write_r(struct _reent *r,int fd,const void *p,size_t n){r->_errno=EBADF;return -1;}
__attribute__((noinline,noreturn)) void test_pass(void){for(;;){}}
__attribute__((noinline,noreturn)) void test_fail(void){for(;;){}}
void __assert_func(const char *f,int line,const char *fn,const char *s){failure_line=line;test_fail();}
void *_sbrk_r(struct _reent *r,ptrdiff_t n){
    if(n<0 || (unsigned)n>sizeof heap-heap_used){r->_errno=ENOMEM;return (void*)-1;}
    void *p=heap+heap_used;heap_used+=(unsigned)n;return p;
}
int _open_r(struct _reent *r,const char *p,int flags,int mode){position=0;return 3;}
int _close_r(struct _reent *r,int fd){return 0;}
int _isatty_r(struct _reent *r,int fd){return 0;}
int _fstat_r(struct _reent *r,int fd,struct stat *s){
    memset(s,0,sizeof(*s));s->st_mode=S_IFREG|0444;s->st_size=file_size;s->st_blksize=4096;return 0;
}
_off_t _lseek_r(struct _reent *r,int fd,_off_t offset,int whence){
    int at=offset+(whence==SEEK_CUR?position:whence==SEEK_END?file_size:0);
    if(at<0){r->_errno=EINVAL;return -1;}position=(unsigned)at;seeks++;return position;
}
_ssize_t _read_r(struct _reent *r,int fd,void *out,size_t n){
    if(position>=file_size)return 0;
    if(n>file_size-position)n=file_size-position;
    if(short_limit && n>short_limit)n=short_limit;
    unsigned char *p=out;for(unsigned i=0;i<n;i++)p[i]=(unsigned char)((position+i)*37+11);
    reads++;bytes_read+=n;if(n>largest)largest=n;position+=n;return n;
}
static void check(FILE *f,unsigned at,unsigned n){
    assert(!fseek(f,at,SEEK_SET));assert(fread(stage,1,n,f)==n);
    for(unsigned i=0;i<n;i++)assert(stage[i]==(unsigned char)((at+i)*37+11));
}
void test_main(void){
    static char buffer[32768];
    for(unsigned mode=0;mode<3;mode++){
        FILE *f=mode==1?fh_asset_open_staged("fixture"):fopen("fixture","rb");assert(f);
        if(mode==2)assert(!setvbuf(f,buffer,_IOFBF,sizeof buffer));
        reads=bytes_read=largest=seeks=0;
        /* Repeated source verification + GPU payload reads, varied alignment. */
        for(unsigned i=0;i<64;i++){
            unsigned at=(i*17017u)%(file_size-65536);
            check(f,at,32768);check(f,at+32768,32768);
        }
        results[mode][0]=reads;results[mode][1]=bytes_read;
        results[mode][2]=largest;results[mode][3]=seeks;results[mode][4]=128;
        short_limit=317;check(f,11,32768);short_limit=0;
        assert(!fseek(f,file_size-7,SEEK_SET));assert(fread(stage,1,32,f)==7);assert(feof(f));
        check(f,17,91);assert(!feof(f));assert(!fclose(f));
    }
    assert(results[1][0]==128);assert(results[1][1]==128*32768);
    assert(results[0][0]>results[1][0]);test_pass();
}
