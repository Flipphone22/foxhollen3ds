#ifndef FH_LOG_IO_H
#define FH_LOG_IO_H
#include <3ds.h>
typedef Result (*FhLogWrite)(Handle,u32*,u64,const void*,u32,u32);
enum { FH_LOG_IO_SLOTS=1,FH_LOG_IO_CHUNK=65536 };
typedef struct { unsigned active,queued,peak,waits,errors,stack_bytes; } FhLogIoStats;
void fh_log_io_init(FhLogWrite write);
void fh_log_io_track(Handle handle,FS_Path path);
int fh_log_io_write(Handle,u32*,u64,const void*,u32,u32,Result*);
Result fh_log_io_drain(Handle handle);
void fh_log_io_forget(Handle handle);
Result fh_log_io_shutdown(void);
void fh_log_io_stats(FhLogIoStats *out);
#endif
