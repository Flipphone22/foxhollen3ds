#ifndef FH_MOVIE_PREFETCH_H
#define FH_MOVIE_PREFETCH_H
#include <stdio.h>
#include <stdint.h>
typedef struct {unsigned active,capacity,stack_bytes,queued,hits,misses,checks,errors,busy_polls,slow_reads;
    uint64_t wait_ticks,read_ticks,max_read_ticks;} FhMoviePrefetchStats;
extern unsigned fh_movie_prefetch_mode;
void fh_movie_prefetch_open(FILE *file,unsigned capacity,unsigned file_bytes);
void fh_movie_prefetch_open_native(FILE *file,unsigned capacity,unsigned file_bytes,unsigned rgb8);
void fh_movie_prefetch_close(void);
int fh_movie_prefetch_read(unsigned offset,void *destination,unsigned bytes,int *result);
/* Returns 1 for handled requests; result may be FH_DISC_ASYNC_BUSY. */
int fh_movie_prefetch_poll(unsigned offset,void *destination,unsigned bytes,int *result);
void fh_movie_prefetch_hint(unsigned offset,unsigned bytes);
/* Synchronous caller only; completion event or at most 1 ms. */
void fh_movie_prefetch_wait_pending(void);
void fh_movie_prefetch_stats(FhMoviePrefetchStats *out);
void fh_stream_prefetch_open(const char *path,unsigned base,unsigned bytes);
void fh_stream_prefetch_close(void);
int fh_stream_prefetch_read(unsigned offset,void *destination,unsigned bytes,int *result);
void fh_stream_prefetch_stats(FhMoviePrefetchStats *out);
enum { FH_DISC_ASYNC_BUSY=-2147483647 };
void fh_disc_async_open(const char *path,unsigned file_bytes);
void fh_disc_async_close(void);
int fh_disc_async_begin(unsigned offset,void *destination,unsigned bytes);
int fh_disc_async_poll(void);
void fh_disc_async_join(void);
void fh_disc_async_stats(FhMoviePrefetchStats *out);
#endif
