#include "fh_install_texture_search.h"
#include "fh_install_texture_iso.h"
#include "fh_install_texture_layout.h"
#include "fh_install_font.h"
#include "../vendor/melee/texture_visibility.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int fail(FhInstallTransaction*t,const char*message){t->failed=1;snprintf(t->error,sizeof(t->error),"%s",message);return 0;}
/* TEXPRE's stored record can contain one complete ZLB plus alignment padding.
 * Own just the decoded image, reject tails/nonzero padding/truncated streams.
 * Never recursively expand arbitrary source records. */
static void*preload(const void*data,unsigned bytes,unsigned*out_bytes){
    const unsigned char*p=data;*out_bytes=0;
    if(bytes<16||memcmp(p,"ZLB\0",4)||word(p+4)!=1)return NULL;
    unsigned decoded=word(p+8),packed=word(p+12);
    if(decoded<96||decoded>FH_INSTALL_RECORD_CAP||!packed||packed>bytes-16||bytes-16-packed>31)return NULL;
    for(unsigned i=16+packed;i<bytes;i++)if(p[i])return NULL;
    void*out=malloc(decoded);if(!out)return NULL;
    z_stream z={0};z.next_in=(Bytef*)p+16;z.avail_in=packed;z.next_out=out;z.avail_out=decoded;
    int ok=inflateInit(&z)==Z_OK;
    if(ok){ok=inflate(&z,Z_FINISH)==Z_STREAM_END&&z.total_out==decoded&&!z.avail_in;inflateEnd(&z);}
    if(!ok){free(out);return NULL;}*out_bytes=decoded;return out;
}
typedef struct {FhInstallTextureStageProgress function;void*context;} Progress;
static int assembly(void*context,uint32_t done,uint32_t total){Progress*p=context;return !p->function||p->function(p->context,FH_INSTALL_TEX_ASSEMBLE,done,total);}
/* Byte-weighted source traversal, not the current image's tile counter. The
 * UI explicitly calls this an estimate: compression, duplicates and quality
 * fallback mean source bytes are not uniform encoding time. No extra SD reads.
 * The resume cursor is included, but cannot become measured new throughput. */
static int collection(FhInstallTextureScan*s,unsigned offset,FhInstallTextureStageProgress progress,void*context){
    uint64_t done=offset,total=0;
    for(unsigned i=0;i<s->banks_count;i++){total+=s->banks[i].bytes;if(i<s->bank)done+=s->banks[i].bytes;}
    if(done>total||total>UINT32_MAX)return 0;
    return !progress||progress(context,FH_INSTALL_TEX_COLLECTION,(uint32_t)done,(uint32_t)total);
}
int fh_install_texture_prepare_bases(FhInstallTransaction*t,FhInstallDisc*d,unsigned spool_id,unsigned output,unsigned edge,FhInstallTextureCoverage*coverage,FhInstallTextureStageProgress progress,void*context){
    if(coverage)memset(coverage,0,sizeof(*coverage));
    if(!t||t->failed||!d||!d->file||spool_id>=FH_INSTALL_ARTIFACTS||output>=FH_INSTALL_ARTIFACTS||spool_id==output)return 0;
    FhInstallTextureScan*scan=NULL;FhInstallTextureSpool spool={0};void*unwrapped=NULL;int result=0;
    if(!(t->state.complete&(1u<<spool_id))){
        if(!fh_install_texture_search_start(t->state.source)){fail(t,"Texture search metadata failed validation.");goto done;}
        if(!fh_install_texture_spool_begin(&spool,t,spool_id))goto done;
        /* Bounded complete-record batching; same bytes and quality. Explicit
         * cancellation/finish and bank changes commit the pending prefix. */
        spool.duplicate_batch=64;
        spool.payload_batch_bytes=1024*1024;
        if(!(t->state.cursor[0]&FH_INSTALL_FONT_CURSOR)){
        scan=calloc(1,sizeof(*scan));if(!scan){fail(t,"Not enough startup memory for texture scanner.");goto done;}
        if(!fh_install_texture_scan_begin(scan,d,t->state.cursor[0],t->state.cursor[1]))goto scan_error;
        if(!collection(scan,scan->resume_offset,progress,context)){result=-1;goto done;}
        for(;;){
            free(unwrapped);unwrapped=NULL;
            if(progress&&!progress(context,FH_INSTALL_TEX_SCAN,scan->bank,scan->banks_count)){result=-1;goto done;}
            FhInstallTextureRecord record;int next=fh_install_texture_scan_record(scan,&record);
            if(next==FH_INSTALL_TEX_ERROR)goto scan_error;if(next==FH_INSTALL_TEX_DONE)break;
            if(next!=FH_INSTALL_TEX_IMAGE){fail(t,"Unexpected texture record status.");goto done;}
            const void*data=record.data;unsigned bytes=record.bytes;
            if(bytes<96){
                unwrapped=preload(data,bytes,&bytes);if(!unwrapped){fail(t,"Invalid nested texture preload record.");goto done;}
                data=unwrapped;if(coverage)coverage->preload_wrappers++;
            }
            FhInstallTextureLayout layout;
            if(!fh_install_texture_layout(data,bytes,&layout)){fail(t,"Unsupported original texture layout.");goto done;}
            if(coverage){coverage->records++;coverage->incomplete_mip_chains+=!layout.chain_complete;}
            for(unsigned i=0;i<layout.count;i++){
                const FhInstallExactTexture*image=&layout.planes[i].image;
                /* If cancelled while converting opacity, rescan this record.
                 * The already durable colour plane deduplicates without work. */
                result=fh_install_texture_spool_add_typed(&spool,image->pixels,image->pixel_bytes,image->width,image->height,image->format,edge,
                    record.file,record.offset,progress,context);
                if(result!=1)goto done;result=0;if(coverage)coverage->planes++;
            }
            if(layout.chain_complete&&!layout.min_lod&&layout.max_lod)for(unsigned i=0;i<layout.count;i++){
                const FhInstallExactTexture*image=&layout.planes[i].image;
                unsigned w=image->width,h=image->height,crc=(unsigned)crc32(0,image->pixels,image->pixel_bytes);
                if(w&(w-1)||h&(h-1))continue;
                const unsigned char*level=(const unsigned char*)image->pixels+image->pixel_bytes;
                for(unsigned k=1;k<=layout.max_lod&&k<=9&&(w>>k)>=8&&(h>>k)>=8;k++){
                    unsigned n=mp_texture_source_bytes(image->format,w>>k,h>>k);
                    result=fh_install_texture_spool_add_mip(&spool,level,n,w,h,image->format,crc,k,record.file,record.offset,progress,context);
                    if(result==2)break;
                    if(result!=1)goto done;
                    level+=n;
                }
                result=0;
            }
            if(!fh_install_texture_spool_advance(&spool,record.next_file,record.next_offset))goto done;
            if(!collection(scan,record.next_offset,progress,context)){result=-1;goto done;}
        }
        if(progress&&!progress(context,FH_INSTALL_TEX_SCAN,scan->banks_count,scan->banks_count)){result=-1;goto done;}
        if(!collection(scan,0,progress,context)){result=-1;goto done;}
        fh_install_texture_scan_close(scan);free(scan);scan=NULL;
        free(unwrapped);unwrapped=NULL;
        }
        result=fh_install_font_prepare(&spool,d,edge,progress,context);
        if(result!=1)goto done;result=0;
        if(!fh_install_texture_spool_finish(&spool))goto done;
    }
    {Progress tracking={progress,context};result=fh_install_texture_pack(t,spool_id,output,assembly,&tracking);}
    goto done;
scan_error:
    fail(t,scan->error);
done:
    fh_install_texture_search_stop();
    if(result==-1&&spool.entries&&!fh_install_texture_spool_flush(&spool))result=0;
    free(unwrapped);if(scan){fh_install_texture_scan_close(scan);free(scan);}fh_install_texture_spool_close(&spool);
    if(result<=0)fh_install_transaction_close(t);return result;
}
