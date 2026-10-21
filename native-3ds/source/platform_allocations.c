#include <stddef.h>
#include <stdio.h>
#include "fh_platform.h"
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void *__real_realloc(void*,size_t);
static void record(const char *kind,size_t bytes,void *memory,void *caller) {
    static unsigned records;
    if((bytes>=1024*1024 || (!memory && bytes)) && records++<64) {
        char line[128];snprintf(line,sizeof(line),"allocation %s bytes=%lu address=%p caller=%p\n",kind,(unsigned long)bytes,memory,caller);fh_platform_log(line);
    }
}
void *__wrap_malloc(size_t bytes) { void *p=__real_malloc(bytes);record("malloc",bytes,p,__builtin_return_address(0));return p; }
void *__wrap_calloc(size_t count,size_t size) { void *p=__real_calloc(count,size);record("calloc",count*size,p,__builtin_return_address(0));return p; }
void *__wrap_realloc(void *old,size_t bytes) { void *p=__real_realloc(old,bytes);record("realloc",bytes,p,__builtin_return_address(0));return p; }
