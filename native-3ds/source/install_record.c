#include "fh_install_record.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
static uint32_t word(const unsigned char*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int fh_install_record_info(FhInstallDisc*d,unsigned file,uint32_t offset,FhInstallRecord*out){
    unsigned char h[16];FhInstallEntry e;
    if(!out || !fh_install_disc_entry(d,file,&e) || !fh_install_disc_read(d,file,offset,h,sizeof(h)))return 0;
    unsigned compressed=!memcmp(h,"ZLB\0",4),stored=!memcmp(h,"DIRn",4);
    uint32_t size=word(h+8),packed=word(h+12),header=compressed?16:32;
    if((!compressed&&!stored) || word(h+4)!=(compressed?1u:0u) || !size || size>FH_INSTALL_RECORD_CAP ||
       !packed || packed>FH_INSTALL_PACKED_CAP || (stored&&packed!=size) ||
       offset>e.bytes || header>e.bytes-offset || packed>e.bytes-offset-header)return 0;
    *out=(FhInstallRecord){offset,size,packed,header,compressed};return 1;
}
int fh_install_record_decode(FhInstallDisc*d,unsigned file,const FhInstallRecord*record,void*out,uint32_t capacity){
    FhInstallRecord actual;
    if(!record || !out || !fh_install_record_info(d,file,record->offset,&actual) || capacity<actual.decoded ||
       actual.offset!=record->offset || actual.decoded!=record->decoded || actual.packed!=record->packed ||
       actual.header!=record->header || actual.compressed!=record->compressed)return 0;
    if(!actual.compressed)return fh_install_disc_read(d,file,actual.offset+actual.header,out,actual.decoded);
    unsigned char*packed=malloc(actual.packed);if(!packed)return 0;
    int ok=fh_install_disc_read(d,file,actual.offset+actual.header,packed,actual.packed);
    if(ok){
        z_stream s;memset(&s,0,sizeof(s));s.next_in=packed;s.avail_in=actual.packed;s.next_out=out;s.avail_out=actual.decoded;
        ok=inflateInit(&s)==Z_OK;
        if(ok){int result=inflate(&s,Z_FINISH);ok=result==Z_STREAM_END&&s.total_out==actual.decoded&&!s.avail_in;inflateEnd(&s);}
    }
    free(packed);return ok;
}
