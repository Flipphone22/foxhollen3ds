#include "fh_texture_pack_padded.h"
#include "fh_texture_pack.h"
#include "fh_asset_staging.h"
#include "fh_texture_pack_layout.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static unsigned be32(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int extent(unsigned n){return n>=8 && n<=1024 && !(n&(n-1));}
void fh_texture_pack_window_drop(FhTexturePack *p){
    p->window=NULL;p->window_pages=0;p->window_clock=0;
    memset(p->window_age,0,sizeof(p->window_age));
}
int fh_texture_pack_window(FhTexturePack *p,void *storage,unsigned bytes){
    fh_texture_pack_window_drop(p);
    if(!p->file || p->resident || !storage || bytes<FH_TEXTURE_WINDOW_PAGE ||
       bytes>FH_TEXTURE_WINDOW_PAGE*FH_TEXTURE_WINDOW_PAGES || bytes%FH_TEXTURE_WINDOW_PAGE)return 0;
    p->window=storage;p->window_pages=bytes/FH_TEXTURE_WINDOW_PAGE;return 1;
}
void fh_texture_pack_resident_drop(FhTexturePack *p){
    p->resident=NULL;free(p->resident_offsets);p->resident_offsets=NULL;p->resident_bytes=0;
}
void fh_texture_pack_close(FhTexturePack *p){
    fh_texture_pack_resident_drop(p);
    fh_texture_pack_window_drop(p);p->file_bytes=0;
    if(p->file)fclose(p->file);
    p->file=NULL;free(p->directory);p->directory=NULL;p->count=p->directory_bytes=p->stride=0;
    free(p->mips);p->mips=NULL;p->mip_count=0;
}
static int read_bytes(FhTexturePack *p,void *dst,unsigned n){
    p->read_calls++;size_t got=fread(dst,1,n,p->file);p->read_bytes+=got;return got==n;
}
static int read_at(FhTexturePack *p,unsigned offset,void *dst,unsigned bytes,unsigned record_end){
    if(record_end>p->file_bytes || offset>record_end || bytes>record_end-offset)return 0;
    if(!p->window){
        return !fseek(p->file,offset,SEEK_SET) && read_bytes(p,dst,bytes);
    }
    unsigned char stage[FH_TEXTURE_WINDOW_PAGE];
    while(bytes){
        unsigned slot=p->window_pages,victim=0;
        for(unsigned i=0;i<p->window_pages;i++){
            if(p->window_age[i] && offset>=p->window_tags[i] && offset-p->window_tags[i]<p->window_lengths[i]){slot=i;break;}
            if(p->window_age[i]<p->window_age[victim])victim=i;
        }
        if(slot==p->window_pages){
            slot=victim;unsigned n=record_end-offset;
            if(n>sizeof(stage))n=sizeof(stage);
            /* Filesystem IPC never writes directly to caller-owned linear RAM. */
            if(fseek(p->file,offset,SEEK_SET) || !read_bytes(p,stage,n))return 0;
            memcpy(p->window+slot*FH_TEXTURE_WINDOW_PAGE,stage,n);
            p->window_tags[slot]=offset;p->window_lengths[slot]=n;p->window_misses++;
        }else p->window_hits++;
        if(p->window_clock==UINT64_MAX){memset(p->window_age,0,sizeof(p->window_age));p->window_clock=0;}
        p->window_age[slot]=++p->window_clock;
        unsigned within=offset-p->window_tags[slot],n=p->window_lengths[slot]-within;if(n>bytes)n=bytes;
        memcpy(dst,p->window+slot*FH_TEXTURE_WINDOW_PAGE+within,n);
        p->window_bytes+=n;offset+=n;dst=(unsigned char*)dst+n;bytes-=n;
    }
    return 1;
}
static int reject(FhTexturePack *p){p->rejected++;fh_texture_pack_close(p);return 0;}
static unsigned mip_chain_bytes(unsigned format,unsigned w,unsigned h,unsigned first,unsigned levels){
    unsigned total=0;
    for(unsigned i=first;i<first+levels;i++){
        unsigned lw=w>>i,lh=h>>i;
        if(lw<8 || lh<8)return 0;
        unsigned n=fh_pack_native_bytes(format,lw,lh);if(!n)return 0;
        total+=n;
    }
    return total;
}
static int parse_mips(FhTexturePack *p,unsigned at,unsigned size){
    unsigned char header[16];
    if(at>size || size-at<16 || fseek(p->file,at,SEEK_SET) || !read_bytes(p,header,16) || memcmp(header,"FHTXMIP1",8))return 0;
    unsigned count=be32(header+8);
    if(!count || count>FH_TEXTURE_MIP_ENTRIES || count*FH_TEXTURE_MIP_STRIDE>size-at-16)return 0;
    unsigned char *d=malloc(count*FH_TEXTURE_MIP_STRIDE);if(!d)return 0;
    if(!read_bytes(p,d,count*FH_TEXTURE_MIP_STRIDE) || (unsigned)crc32(0,d,count*FH_TEXTURE_MIP_STRIDE)!=be32(header+12)){free(d);return 0;}
    unsigned end=at+16+count*FH_TEXTURE_MIP_STRIDE;
    for(unsigned i=0;i<count;i++){
        const unsigned char *e=d+i*FH_TEXTURE_MIP_STRIDE;
        unsigned w=be32(e+4),h=be32(e+8),fmt=be32(e+16),levels=be32(e+20),offset=be32(e+24),bytes=be32(e+28);
        if(i){
            const unsigned char *q=e-FH_TEXTURE_MIP_STRIDE;
            if(memcmp(q,e,16)>0 || !memcmp(q,e,16)){free(d);return 0;}
        }
        if(!extent(w) || !extent(h) || !levels || levels>9 || offset!=end || bytes!=mip_chain_bytes(fmt,w,h,1,levels) || bytes>size-end){free(d);return 0;}
        end+=bytes;
    }
    if(end!=size){free(d);return 0;}
    p->mips=d;p->mip_count=count;return 1;
}
static int mip_key_compare(const unsigned char *e,unsigned crc,unsigned w,unsigned h,unsigned source_format){
    unsigned key[4]={crc,w,h,source_format};
    for(unsigned i=0;i<4;i++){unsigned v=be32(e+i*4);if(v!=key[i])return v<key[i]?-1:1;}
    return 0;
}
int fh_texture_pack_mips(FhTexturePack *p,const FhPackedTexture *base,unsigned w,unsigned h,unsigned source_format,FhPackedMips *out){
    if(!p->file || !p->mips || !base || !out)return 0;
    unsigned lo=0,hi=p->mip_count;
    while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(mip_key_compare(p->mips+mid*FH_TEXTURE_MIP_STRIDE,base->source_crc,w,h,source_format)<0)lo=mid+1;else hi=mid;}
    if(lo==p->mip_count)return 0;
    const unsigned char *e=p->mips+lo*FH_TEXTURE_MIP_STRIDE;
    if(mip_key_compare(e,base->source_crc,w,h,source_format) || be32(e+16)!=base->format)return 0;
    unsigned levels=be32(e+20),first=0;
    for(unsigned i=1;i<=levels;i++)if((w>>i)*2==base->width && (h>>i)*2==base->height){first=i;break;}
    if(!first || base->width!=w>>(first-1) || base->height!=h>>(first-1))return 0;
    unsigned count=levels-first+1;
    *out=(FhPackedMips){be32(e+24),be32(e+28),be32(e+32),mip_chain_bytes(base->format,w,h,1,first-1),
        mip_chain_bytes(base->format,w,h,first,count),count};
    return out->bytes!=0;
}
int fh_texture_pack_read_mips(FhTexturePack *p,const FhPackedMips *m,void *output,unsigned capacity){
    if(!p->file || !m || !output || capacity<m->bytes || m->skip>m->chain_bytes || m->bytes>m->chain_bytes-m->skip ||
       m->offset>p->file_bytes || m->chain_bytes>p->file_bytes-m->offset)return 0;
    if(!m->skip && m->bytes==m->chain_bytes){
        if(fseek(p->file,m->offset,SEEK_SET) || !read_bytes(p,output,m->bytes) || (unsigned)crc32(0,output,m->bytes)!=m->crc)return 0;
        p->mip_reads++;return 1;
    }
    unsigned char *chain=malloc(m->chain_bytes);if(!chain)return 0;
    if(fseek(p->file,m->offset,SEEK_SET) || !read_bytes(p,chain,m->chain_bytes) || (unsigned)crc32(0,chain,m->chain_bytes)!=m->crc){free(chain);return 0;}
    memcpy(output,chain+m->skip,m->bytes);free(chain);p->mip_reads++;return 1;
}

int fh_texture_pack_open(FhTexturePack *p,const char *path,unsigned budget){
    fh_texture_pack_close(p);
    p->file=fh_asset_open_staged(path);if(!p->file)return 0;
    unsigned char header[16];long size;
    if(fseek(p->file,0,SEEK_END) || (size=ftell(p->file))<16 || size>128*1024*1024 ||
       fseek(p->file,0,SEEK_SET) || !read_bytes(p,header,16))return reject(p);
    if(!memcmp(header,"FHTXPK1",8))p->stride=32;
    else if(!memcmp(header,"FHTXPK2",8))p->stride=40;
    else if(!memcmp(header,"FHTXPK3",8))p->stride=44;
    else if(!memcmp(header,"FHTXPK4",8))p->stride=44;
    else if(!memcmp(header,"FHTXPK5",8))p->stride=44;
    else return reject(p);
    unsigned count=be32(header+8);
    if(!count || count>4096 || count*p->stride>budget || 16+count*p->stride>(unsigned)size)return reject(p);
    p->directory=malloc(count*p->stride);if(!p->directory)return reject(p);
    p->directory_bytes=count*p->stride;
    if(!read_bytes(p,p->directory,p->directory_bytes) || (unsigned)crc32(0,p->directory,p->directory_bytes)!=be32(header+12))return reject(p);
    unsigned end=16+p->directory_bytes,previous=0;
    for(unsigned i=0;i<count;i++){
        const unsigned char *e=p->directory+i*p->stride;
        unsigned crc=be32(e),w=be32(e+4),h=be32(e+8),source=be32(e+12),at=be32(e+16),fmt=be32(e+20),gpu=be32(e+24);
        unsigned gw=p->stride>=40?be32(e+32):w,gh=p->stride>=40?be32(e+36):h,source_format=p->stride==44?be32(e+40):14;
        if((i && crc<previous) || (p->stride<44&&(!extent(w)||!extent(h))) ||
           !((header[6]=='4'||header[6]=='5')?fh_pack_entry_valid_padded(w,h,source_format,source,fmt,gw,gh,gpu):fh_pack_entry_valid(w,h,source_format,source,fmt,gw,gh,gpu)) || at!=end ||
           at>(unsigned)size || source+gpu>(unsigned)size-at)return reject(p);
        end=at+source+gpu;previous=crc;
    }
    if(header[6]=='5'){if(!parse_mips(p,end,(unsigned)size))return reject(p);}
    else if(end!=(unsigned)size)return reject(p);
    p->count=count;p->file_bytes=(unsigned)size;return 1;
}
static int same_source_key(const FhTexturePack*p,const unsigned char *a,const unsigned char *b){
    return !memcmp(a,b,16)&&(p->stride!=44||!memcmp(a+40,b+40,4)); /* Not identity proof. */
}
unsigned fh_texture_pack_resident_size(const FhTexturePack *p){
    if(!p->file)return 0;
    unsigned size=0;
    for(unsigned i=0;i<p->count;i++){
        const unsigned char *e=p->directory+i*p->stride;unsigned duplicate=0;
        for(unsigned j=0;j<i;j++)if(same_source_key(p,e,p->directory+j*p->stride)){duplicate=1;break;}
        size+=be32(e+24);if(!duplicate)size+=be32(e+12);
    }
    return size;
}
int fh_texture_pack_resident_load(FhTexturePack *p,void *storage,unsigned capacity){
    fh_texture_pack_resident_drop(p);
    if(!p->file || !storage || !capacity)return 0;
    unsigned *offsets=malloc(p->count*2*sizeof(unsigned));if(!offsets)return 0;
    unsigned char *data=storage,chunk[FH_ASSET_STAGE_BYTES];unsigned used=0;
    for(unsigned i=0;i<p->count;i++){
        const unsigned char *e=p->directory+i*p->stride;
        unsigned bytes=be32(e+12),gpu=be32(e+24),candidate=i;
        for(unsigned j=0;j<i;j++)if(same_source_key(p,e,p->directory+j*p->stride)){candidate=j;break;}
        if(fseek(p->file,be32(e+16),SEEK_SET))goto failed;
        if(candidate<i){
            unsigned at=0;int same=1;
            while(at<bytes){unsigned n=bytes-at;if(n>sizeof(chunk))n=sizeof(chunk);
                if(!read_bytes(p,chunk,n))goto failed;
                if(memcmp(chunk,data+offsets[candidate*2]+at,n))same=0;
                at+=n;
            }
            /* A real CRC collision cannot borrow another source. If the
             * estimated compact capacity is insufficient, use the disk path. */
            if(!same){candidate=i;if(fseek(p->file,be32(e+16),SEEK_SET))goto failed;}
        }
        if(candidate==i){
            if(used>capacity || bytes>capacity-used)goto failed;
            offsets[i*2]=used;
            for(unsigned at=0;at<bytes;){unsigned n=bytes-at;if(n>sizeof(chunk))n=sizeof(chunk);
                if(!read_bytes(p,chunk,n))goto failed;
                memcpy(data+used+at,chunk,n);at+=n;}
            used+=bytes;
        }else offsets[i*2]=offsets[candidate*2];
        if(used>capacity || gpu>capacity-used)goto failed;
        offsets[i*2+1]=used;unsigned crc=0;
        for(unsigned at=0;at<gpu;){unsigned n=gpu-at;if(n>sizeof(chunk))n=sizeof(chunk);
            if(!read_bytes(p,chunk,n))goto failed;
            crc=crc32(crc,chunk,n);memcpy(data+used+at,chunk,n);at+=n;}
        if(crc!=be32(e+28))goto failed;
        used+=gpu;
    }
    p->resident=data;p->resident_offsets=offsets;p->resident_bytes=used;return 1;
failed:
    free(offsets);return 0;
}
int fh_texture_pack_find_typed(FhTexturePack *p,const void *source,unsigned bytes,unsigned w,unsigned h,unsigned source_format,unsigned reduced,FhPackedTexture *out){
    unsigned needed=fh_pack_source_bytes(source_format,w,h);
    if(!p->file || !source || !out || !needed || bytes!=needed ||
       (p->stride<44&&(source_format!=14||!extent(w)||!extent(h))))return 0;
    unsigned crc=crc32(0,source,bytes),lo=0,hi=p->count;
    while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(be32(p->directory+mid*p->stride)<crc)lo=mid+1;else hi=mid;}
    unsigned char chunk[FH_ASSET_STAGE_BYTES];
    for(unsigned i=lo;i<p->count;i++){
        const unsigned char *e=p->directory+i*p->stride;
        if(be32(e)!=crc)break;
        if(be32(e+4)!=w || be32(e+8)!=h || be32(e+12)!=bytes || (p->stride==44?be32(e+40):14)!=source_format)continue;
        unsigned gw=p->stride>=40?be32(e+32):w,gh=p->stride>=40?be32(e+36):h;
        if(!reduced && (gw!=fh_pack_extent(w) || gh!=fh_pack_extent(h)))continue;
        unsigned at=0;
        if(p->resident){
            p->resident_reads++;p->resident_read_bytes+=bytes;
            if(memcmp(p->resident+p->resident_offsets[i*2],source,bytes))continue;
            at=bytes;
        }
        while(at<bytes){
            unsigned n=bytes-at;if(n>sizeof(chunk))n=sizeof(chunk);
            if(!read_at(p,be32(e+16)+at,chunk,n,be32(e+16)+bytes+be32(e+24)))return reject(p);
            if(memcmp(chunk,(const unsigned char*)source+at,n))break;
            at+=n;
        }
        if(at!=bytes)continue; /* CRC collision never admits another image. */
        *out=(FhPackedTexture){be32(e+16)+bytes,be32(e+20),be32(e+24),be32(e+28),
            fh_pack_native_bytes(fh_pack_exact_format(source_format),fh_pack_extent(w),fh_pack_extent(h)),gw,gh,crc};
        p->hits++;return 1;
    }
    p->misses++;return 0;
}
int fh_texture_pack_find(FhTexturePack *p,const void *s,unsigned w,unsigned h,FhPackedTexture *out){
    return extent(w)&&extent(h)?fh_texture_pack_find_typed(p,s,w*h/2,w,h,14,0,out):0;
}
int fh_texture_pack_find_surface(FhTexturePack *p,const void *s,unsigned w,unsigned h,FhPackedTexture *out){
    return extent(w)&&extent(h)?fh_texture_pack_find_typed(p,s,w*h/2,w,h,14,1,out):0;
}
int fh_texture_pack_read(FhTexturePack *p,const FhPackedTexture *e,void *output,unsigned capacity){
    if(!p->file || !output || capacity<e->bytes || e->offset>p->file_bytes || e->bytes>p->file_bytes-e->offset)return 0;
    if(p->resident){
        for(unsigned i=0;i<p->count;i++){
            const unsigned char *entry=p->directory+i*p->stride;
            if(be32(entry+16)+be32(entry+12)!=e->offset)continue;
            if(be32(entry+24)!=e->bytes || be32(entry+28)!=e->crc)return 0;
            const void *data=p->resident+p->resident_offsets[i*2+1];
            if((unsigned)crc32(0,data,e->bytes)!=e->crc)return reject(p);
            memcpy(output,data,e->bytes);p->resident_reads++;p->resident_read_bytes+=e->bytes;
            p->uploads++;p->saved_upload_bytes+=e->original_bytes-e->bytes;return 1;
        }
        return 0;
    }
    /* SD reads land in CPU memory; only owned, unpublished GPU storage is written. */
    unsigned char chunk[FH_ASSET_STAGE_BYTES];unsigned at=0,crc=0;
    while(at<e->bytes){
        unsigned n=e->bytes-at;if(n>sizeof(chunk))n=sizeof(chunk);
        if(!read_at(p,e->offset+at,chunk,n,e->offset+e->bytes))return reject(p);
        crc=crc32(crc,chunk,n);memcpy((unsigned char*)output+at,chunk,n);at+=n;
    }
    if(crc!=e->crc)return reject(p);
    p->uploads++;p->saved_upload_bytes+=e->original_bytes-e->bytes;return 1;
}
