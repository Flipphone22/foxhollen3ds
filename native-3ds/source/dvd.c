#include "fh_install_bindings.h"
#include <dolphin/dvd.h>
#include <dolphin/ai.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include "fh_platform.h"
#include "fh_prepared_movie.h"
#include "fh_movie_prefetch.h"
#include "fh_input_tape.h"

/* Boot owns a 94-entry DVDFileInfo pool. Real asynchronous completion allows
 * that burst to remain pending; 64 only worked with immediate main-thread I/O.
 * Match Melee's bounded 128-entry queue, with headroom for movie requests. */
enum { MAX_ENTRIES=8192, MAX_FST_BYTES=2*1024*1024, REQUESTS=128 };
typedef struct { uint32_t offset,bytes,parent,next; const char *name; int directory; } Entry;
typedef struct { DVDFileInfo *file; void *destination; int bytes,offset; DVDCallback callback; } Request;
static FILE *disc;
static char disc_path[1024];
static DVDFileInfo *audio_stream;
static FILE *prepared_movie;
static uint32_t prepared_movie_id,prepared_movie_bytes;
static uint8_t *fst;
static Entry *entries;
static uint32_t entry_count,current_directory;
static long disc_bytes;
static DVDDiskID identity;
static Request requests[REQUESTS];
static unsigned head,tail;
static int pumping,async_read,completion_ready,completion_result;
static unsigned queue_peak,queue_rejected;

static uint32_t be32(const uint8_t *p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static int raw_read(uint32_t offset,void *destination,uint32_t bytes) {
    if (!disc || (uint64_t)offset+bytes>(uint64_t)disc_bytes || (!destination && bytes)) return -1;
    if (fseek(disc,(long)offset,SEEK_SET)) return -1;
    return fread(destination,1,bytes,disc)==bytes?(int)bytes:-1;
}
void fh_dvd_shutdown(void) {
    /* Join borrowed destination writes before game storage is released. */
    fh_disc_async_join();
    FhMoviePrefetchStats disc_stats;fh_disc_async_stats(&disc_stats);
    if(disc_stats.active){char line[192];snprintf(line,sizeof(line),
        "DVD async: queued=%u errors=%u read_ticks=%llu stack=%u\n",disc_stats.queued,disc_stats.errors,
        (unsigned long long)disc_stats.read_ticks,disc_stats.stack_bytes);fh_platform_log(line);}
    fh_disc_async_close();async_read=completion_ready=0;
    if(queue_peak){char line[128];snprintf(line,sizeof(line),"DVD queue: peak=%u capacity=%u rejected=%u\n",queue_peak,REQUESTS,queue_rejected);fh_platform_log(line);}
    queue_peak=queue_rejected=0;
    FhMoviePrefetchStats stream_stats;fh_stream_prefetch_stats(&stream_stats);
    if(stream_stats.active){char line[256];snprintf(line,sizeof(line),
        "Stream prefetch: queued=%u hits=%u misses=%u errors=%u wait_ticks=%llu read_ticks=%llu buffer=%u stack=%u\n",
        stream_stats.queued,stream_stats.hits,stream_stats.misses,stream_stats.errors,
        (unsigned long long)stream_stats.wait_ticks,(unsigned long long)stream_stats.read_ticks,stream_stats.capacity,stream_stats.stack_bytes);fh_platform_log(line);}
    fh_stream_prefetch_close();audio_stream=NULL;disc_path[0]=0;
    fh_movie_prefetch_close();
    if (prepared_movie) fclose(prepared_movie);
    prepared_movie=NULL;prepared_movie_id=prepared_movie_bytes=0;
    if (disc) fclose(disc);
    disc=NULL; free(fst); free(entries); fst=NULL; entries=NULL;
    entry_count=current_directory=head=tail=0; pumping=0;
    memset(&identity,0,sizeof(identity));
}
int fh_dvd_mount(const char *path) {
    uint8_t header[0x440];
    fh_dvd_shutdown();
    if(!path || strlen(path)>=sizeof(disc_path))return 0;
    strcpy(disc_path,path);
    disc=fopen(path,"rb");
    if (!disc || fseek(disc,0,SEEK_END) || (disc_bytes=ftell(disc))<sizeof(header)) goto fail;
    if (raw_read(0,header,sizeof(header))<0 || memcmp(header,"GSAP01",6) || header[6]!=0 || header[7]!=1 || be32(header+0x1c)!=0xc2339f3d) goto fail;
    uint32_t offset=be32(header+0x424),bytes=be32(header+0x428);
    if (bytes<12 || bytes>MAX_FST_BYTES) goto fail;
    fst=malloc(bytes);
    if (!fst || raw_read(offset,fst,bytes)<0 || fst[0]!=1) goto fail;
    entry_count=be32(fst+8);
    if (!entry_count || entry_count>MAX_ENTRIES || (uint64_t)entry_count*12>=bytes) goto fail;
    entries=calloc(entry_count,sizeof(*entries));
    if (!entries) goto fail;
    size_t strings=entry_count*12;
    uint32_t stack[128],depth=1; stack[0]=0;
    for (uint32_t i=0;i<entry_count;++i) {
        uint8_t *record=fst+i*12;
        Entry *e=&entries[i];
        uint32_t name=be32(record)&0xffffff;
        if (record[0]>1 || name>=bytes-strings || !memchr(fst+strings+name,0,bytes-strings-name)) goto fail;
        while (depth>1 && i>=entries[stack[depth-1]].next) --depth;
        e->name=(char*)fst+strings+name; e->directory=record[0]; e->parent=i?stack[depth-1]:0;
        e->offset=be32(record+4); e->bytes=be32(record+8); e->next=i+1;
        if (e->directory) {
            e->next=e->bytes;
            if (e->next<=i || e->next>entry_count || (i && (e->next>entries[e->parent].next || e->offset!=e->parent))) goto fail;
            if (i) { if (depth==128) goto fail; stack[depth++]=i; }
        } else if ((uint64_t)e->offset+e->bytes>(uint64_t)disc_bytes) goto fail;
    }
    fh_disc_async_open(path,(unsigned)disc_bytes);
    memcpy(&identity,header,sizeof(identity));
    return 1;
fail:
    fh_dvd_shutdown(); return 0;
}
void DVDInit(void) {
    if (disc) return;
    const char *path=fh_install_bindings_disc();
    if(!path)path=getenv("FOXHOLLOW_DISC");
    if (!path) path="sdmc:/3ds/foxhollow/disc.iso";
    if (!fh_dvd_mount(path)) fh_platform_panic("PAL revision 1 disc mount failed");
}
s32 DVDConvertPathToEntrynum(const char *path) {
    if (!disc || !path || !*path || strlen(path)>1023) return -1;
    uint32_t current=*path=='/'?0:current_directory;
    while (*path) {
        while (*path=='/') ++path;
        if (!*path) break;
        const char *end=strchr(path,'/'); size_t length=end?(size_t)(end-path):strlen(path);
        if (length==1 && path[0]=='.') { path+=length; continue; }
        if (length==2 && path[0]=='.' && path[1]=='.') { current=entries[current].parent; path+=length; continue; }
        if (!entries[current].directory) return -1;
        uint32_t child=current+1;
        for (;child<entries[current].next;child=entries[child].next)
            if (strlen(entries[child].name)==length && !strncasecmp(entries[child].name,path,length)) break;
        if (child>=entries[current].next) return -1;
        current=child; path+=length;
    }
    return current;
}
BOOL DVDConvertEntrynumToPath(s32 id,char *path,u32 capacity) {
    if (id<0 || (uint32_t)id>=entry_count || !path || !capacity) return 0;
    uint32_t chain[128],count=0;
    for (uint32_t i=id;i;i=entries[i].parent) {
        if (count==128) return 0;
        chain[count++]=i;
    }
    size_t needed=1;
    for (unsigned i=0;i<count;++i) needed+=strlen(entries[chain[i]].name)+1;
    if (needed+1>capacity) return 0;
    strcpy(path,"/");
    while (count) { strcat(path,entries[chain[--count]].name); if (count) strcat(path,"/"); }
    return 1;
}
BOOL DVDChangeDir(const char *path) {
    int id=DVDConvertPathToEntrynum(path);
    if (id<0 || !entries[id].directory) return 0;
    current_directory=id; return 1;
}
BOOL DVDGetCurrentDir(char *path,u32 capacity) { return DVDConvertEntrynumToPath(current_directory,path,capacity); }
#include "fh_hardware.h"
static void open_prepared_movie_from(unsigned id,const char *forced) {
    if(prepared_movie || strcmp(entries[id].name,"starfox.thp"))return;
    unsigned char h[192],original[48],info[40];long size;
    const char *path=forced?forced:fh_install_bindings_movie();
    if(!path)path=getenv("FOXHOLLOW_PREPARED_MOVIE");
    if(!path)path="sdmc:/3ds/foxhollow/starfox-pica.thp";
    FILE *candidate=fopen(path,"rb");if(!candidate)return;
    if(setvbuf(candidate,NULL,_IONBF,0))goto fail;
    if(fseek(candidate,0,SEEK_END) || (size=ftell(candidate))<192 || fseek(candidate,0,SEEK_SET) || fread(h,1,sizeof(h),candidate)!=sizeof(h))goto fail;
    if(raw_read(entries[id].offset,original,sizeof(original))<0)goto fail;
    uint32_t component=be32(original+32);
    if(entries[id].bytes<sizeof(info) || component>entries[id].bytes-sizeof(info) || raw_read(entries[id].offset+component,info,sizeof(info))<0)goto fail;
    if(!fh_movie_header_valid(h,(size_t)size,original,info,entries[id].offset,entries[id].bytes))goto fail;
    /* Disable stdio read-ahead: movie frames are large, owned direct reads.
     * This avoids a small buffered prefix splitting every sequential frame. */
    prepared_movie=candidate;prepared_movie_id=id+1;prepared_movie_bytes=(uint32_t)size;
    fh_movie_prefetch_open_native(candidate,be32(h+8),(unsigned)size,fh_movie_video_format(h+128,8)==FH_TEXTURE_PICA_RGB8);
    printf("DVD: prepared PAL movie active (%lu bytes)\n",(unsigned long)size);return;
fail:
    fclose(candidate);printf("DVD: prepared movie rejected; using original THP\n");
}
static void open_prepared_movie(unsigned id) {
    if(!fh_hardware_is_new)open_prepared_movie_from(id,"sdmc:/3ds/foxhollow/starfox-pica.thp");
    open_prepared_movie_from(id,NULL);
}
BOOL DVDFastOpen(s32 id,DVDFileInfo *file) {
    if (!file || id<0 || (uint32_t)id>=entry_count || entries[id].directory) return 0;
    memset(file,0,sizeof(*file));
    open_prepared_movie(id);
    file->startAddr=entries[id].offset; file->length=entries[id].bytes; file->cb.userData=(void*)(uintptr_t)(id+1);
    if((unsigned)id+1==prepared_movie_id)file->length=prepared_movie_bytes;
    return 1;
}
BOOL DVDOpen(const char *path,DVDFileInfo *file) { return DVDFastOpen(DVDConvertPathToEntrynum(path),file); }
void fh_dvd_stream_prepare(DVDFileInfo *file){
    fh_stream_prefetch_close();audio_stream=NULL;
    uintptr_t id=file?(uintptr_t)file->cb.userData:0;
    if(!disc || !id || id>entry_count || id==prepared_movie_id ||
       file->startAddr!=entries[id-1].offset || file->length!=entries[id-1].bytes)return;
    audio_stream=file;
    fh_stream_prefetch_open(disc_path,file->startAddr,file->length);
}
static int read_file(DVDFileInfo *file,void *destination,int bytes,int offset) {
    uintptr_t id=file?(uintptr_t)file->cb.userData:0;
    if (!id || id>entry_count || !disc || !file || bytes<0 || offset<0 || (!destination && bytes)) return -1;
    if ((uint32_t)offset>file->length || (uint64_t)(uint32_t)offset+(uint32_t)bytes>=(uint64_t)file->length+32) return -1;
    file->cb.command=DVD_COMMAND_READ;file->cb.addr=destination;file->cb.offset=file->startAddr+offset;file->cb.length=bytes;
    uint32_t actual=file->length-offset;
    if (actual>(uint32_t)bytes) actual=bytes;
    int got=0;
    if(actual) {
        if(id==prepared_movie_id){
            if(!fh_movie_prefetch_read(offset,destination,actual,&got))
                got=fseek(prepared_movie,offset,SEEK_SET) || fread(destination,1,actual,prepared_movie)!=actual?-1:(int)actual;
        }
        else if(file!=audio_stream || !fh_stream_prefetch_read(offset,destination,actual,&got))
            got=raw_read(file->startAddr+offset,destination,actual);
    }
    if (got<0) return -1;
    if ((uint32_t)bytes>actual) memset((uint8_t*)destination+actual,0,bytes-actual);
    if (bytes) fh_platform_cache(destination,bytes,1);
    file->cb.transferredSize=bytes; file->cb.currTransferSize=bytes;
    return bytes;
}
void fh_dvd_movie_prefetch(DVDFileInfo *file,unsigned bytes,unsigned offset){
    if(file && (uintptr_t)file->cb.userData==prepared_movie_id && prepared_movie_id)
        fh_movie_prefetch_hint(offset,bytes);
}
/* Only the source synchronous DVDRead loop calls this. Async pumps remain
 * nonblocking; main still owns completion gates, cache flush and callbacks. */
void fh_dvd_wait_pending(DVDFileInfo *file){
    if(!file || pumping || completion_ready || async_read || head==tail ||
       file->cb.state!=DVD_STATE_BUSY || requests[tail%REQUESTS].file!=file ||
       !prepared_movie_id || (uintptr_t)file->cb.userData!=prepared_movie_id)return;
    fh_movie_prefetch_wait_pending();
}
s32 DVDReadPrio(DVDFileInfo *file,void *destination,s32 bytes,s32 offset,s32 priority) {
    (void)priority;
    while (file && file->cb.state==DVD_STATE_BUSY) fh_dvd_pump();
    int result=read_file(file,destination,bytes,offset);
    if (file) file->cb.state=result<0?DVD_STATE_FATAL_ERROR:DVD_STATE_END;
    return result;
}
BOOL DVDReadAsyncPrio(DVDFileInfo *file,void *destination,s32 bytes,s32 offset,DVDCallback callback,s32 priority) {
    (void)priority;
    if (!file || !file->cb.userData || file->cb.state==DVD_STATE_BUSY || bytes<0 || offset<0 || (!destination && bytes)) return 0;
    if(head-tail>=REQUESTS){queue_rejected++;return 0;}
    if ((uint32_t)offset>file->length || (uint64_t)(uint32_t)offset+(uint32_t)bytes>=(uint64_t)file->length+32) return 0;
    requests[head++%REQUESTS]=(Request){file,destination,bytes,offset,callback};
    if(head-tail>queue_peak)queue_peak=head-tail;
    file->cb.addr=destination;file->cb.offset=file->startAddr+offset;file->cb.length=bytes;
    file->cb.transferredSize=file->cb.currTransferSize=0;file->callback=callback;
    file->cb.command=DVD_COMMAND_READ; file->cb.state=DVD_STATE_BUSY;
    return 1;
}
static int replay_wait(const Request *request){
#ifdef FH_REPLAY_BUILD
    extern int fh_replay_dvd_wait(unsigned,unsigned,unsigned);
    return fh_replay_dvd_wait((unsigned)(uintptr_t)request->file->cb.userData,request->offset,request->bytes);
#else
    (void)request;return 0;
#endif
}
void fh_dvd_pump(void) {
    if (head==tail || pumping) return;
    pumping=1;
    Request request=requests[tail%REQUESTS];
    int result;
retry:
    if(completion_ready)result=completion_result;
    else if(async_read){
        result=fh_disc_async_poll();
        if(result==FH_DISC_ASYNC_BUSY){if(replay_wait(&request))goto retry;pumping=0;return;}
        async_read=0;
        if(request.file->cb.state==DVD_STATE_CANCELED)result=DVD_RESULT_CANCELED;
        else if(result>=0){
            if(result<request.bytes)memset((u8*)request.destination+result,0,request.bytes-result);
            result=request.bytes;
            if(result)fh_platform_cache(request.destination,result,1);
        }
    }else if(request.file->cb.state==DVD_STATE_CANCELED)result=DVD_RESULT_CANCELED;
    else{
        uintptr_t id=(uintptr_t)request.file->cb.userData;
        unsigned actual=request.file->length-request.offset;
        if(actual>(unsigned)request.bytes)actual=request.bytes;
        if(disc && id && id<=entry_count && id!=prepared_movie_id && actual &&
           fh_disc_async_begin(request.file->startAddr+request.offset,request.destination,actual)){
            async_read=1;if(replay_wait(&request))goto retry;pumping=0;return;
        }
        if(id==prepared_movie_id && actual &&
           fh_movie_prefetch_poll(request.offset,request.destination,actual,&result)){
            if(result==FH_DISC_ASYNC_BUSY){if(replay_wait(&request))goto retry;pumping=0;return;}
            if(result>=0){
                if(actual<(unsigned)request.bytes)memset((u8*)request.destination+actual,0,request.bytes-actual);
                result=request.bytes;
                fh_platform_cache(request.destination,result,1);
            }
        }else result=read_file(request.file,request.destination,request.bytes,request.offset);
    }
    completion_ready=1;completion_result=result;
    /* The fallback reader also writes these fields. Do not publish them
     * before the replay gate releases the real completed request. */
    request.file->cb.transferredSize=request.file->cb.currTransferSize=0;
    if(request.file->cb.state==DVD_STATE_CANCELED)result=DVD_RESULT_CANCELED;
    if(!fh_input_dvd_complete((unsigned)(uintptr_t)request.file->cb.userData,
        (unsigned)request.offset,(unsigned)request.bytes,result)){pumping=0;return;}
    completion_ready=0;
    if(result>=0)request.file->cb.transferredSize=request.file->cb.currTransferSize=result;
    tail++;
    request.file->cb.state=result==DVD_RESULT_CANCELED?DVD_STATE_CANCELED:result<0?DVD_STATE_FATAL_ERROR:DVD_STATE_END;
    pumping=0;
    if (request.callback) request.callback(result,request.file);
}
s32 DVDGetFileInfoStatus(const DVDFileInfo *file) { fh_dvd_pump(); return file?file->cb.state:DVD_STATE_FATAL_ERROR; }
s32 DVDGetCommandBlockStatus(const DVDCommandBlock *block) { fh_dvd_pump(); return block?block->state:DVD_STATE_FATAL_ERROR; }
s32 DVDGetDriveStatus(void) { fh_dvd_pump(); return disc?DVD_STATE_END:DVD_STATE_NO_DISK; }
BOOL DVDClose(DVDFileInfo *file) {
    if (!file || !file->cb.userData) return 0;
    for (;;) {
        int pending=0;
        for (unsigned i=tail;i!=head;++i) if (requests[i%REQUESTS].file==file) { pending=1; break; }
        if (!pending) break;
        fh_dvd_pump();
    }
    file->cb.userData=NULL; return 1;
}
s32 DVDCancel(volatile DVDCommandBlock *block) {
    if (!block) return -1;
    /* The worker may still own this request's destination. A successful
     * cancel must release it before the caller can reuse/free it. */
    if(async_read && tail!=head && block==&requests[tail%REQUESTS].file->cb)fh_disc_async_join();
    block->state=DVD_STATE_CANCELED;
    return 0;
}
int DVDCancelAsync(DVDCommandBlock *block,DVDCBCallback callback) {
    if (DVDCancel(block)) return 0;
    while (tail!=head) fh_dvd_pump();
    if (callback) callback(0,block);
    return 1;
}
BOOL DVDCheckDisk(void) { return disc!=NULL; }
DVDDiskID *DVDGetCurrentDiskID(void) { return &identity; }
BOOL DVDSetAutoInvalidation(BOOL enabled) { (void)enabled; return 1; }
void *DVDGetFSTLocation(void) { return fst; }

void fh_dvd_replay_diagnostic(void){
    Request *r=&requests[tail%REQUESTS];char line[256];
    snprintf(line,sizeof(line),"DVD replay pending: head=%u tail=%u pumping=%d async=%d ready=%d file=%u offset=%d bytes=%d result=%d\n",head,tail,pumping,async_read,completion_ready,head!=tail?(unsigned)(uintptr_t)r->file->cb.userData:0,r->offset,r->bytes,completion_result);fh_platform_log(line);
}

int fh_dvd_pending(void){return head!=tail;}
