#include "fh_gpu_transfer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char rows_source[422400],rows_destination[1048576];
int main(void) {
    /* Command 5 in the pending tail on console 9561746b785b, 20261004-112443.
     * Both flags8 and12 builds stalled; the exact command was not isolated.
     * Admit the layout only
     * into the registered GPU-sampled linear allocation used by Melee. */
    const uintptr_t a=0x1f000000,b=0x1f367200;
    const unsigned n=0x67200,in=0x210,out=0x01f00210;
    assert(fh_gpu_texture_copy_error(a,n,in,b,512*512*4,out,n,8));
    assert(fh_gpu_texture_copy_error(a,n,in,b,512*512*4,out,n,12));
    assert(!fh_gpu_sampled_copy_error(a,n,in,0x30200000,512*512*4,out,n,12,0x30200000,512*512*4));
    assert(fh_gpu_sampled_copy_error(a,n,in,0x30512000,512*512*4,out,n,12,0x30200000,512*512*4));
    assert(fh_gpu_sampled_copy_error(a,n,in,0x30200000,512*512*4,out,n,12,0,0));
    assert(fh_gpu_copy_span(out,n)==49*512*8*4+264*8*4);
    assert(fh_gpu_copy_rows(in,out,n)==50);
    assert(!fh_gpu_copy_rows(0x10001,out,n));
    assert(!fh_gpu_copy_rows(in,out,n-16));
    memset(rows_destination,0x5a,sizeof(rows_destination));
    for(unsigned i=0;i<n;i++)rows_source[i]=(i^(i>>11))&255;
    for(unsigned i=0;i<50;i++){
        FhGpuCopyRow row=fh_gpu_copy_row(out,i);
        assert(row.source_offset+row.bytes<=sizeof(rows_source));
        assert(row.destination_offset+row.bytes<=sizeof(rows_destination));
        memcpy(rows_destination+row.destination_offset,rows_source+row.source_offset,row.bytes);
    }
    /* Independent padded-image oracle: padding must remain untouched. */
    for(unsigned i=0;i<sizeof(rows_destination);i++){
        unsigned y=i/(512*8*4),x=i%(512*8*4);
        unsigned expected=y<50 && x<264*8*4?rows_source[y*264*8*4+x]:0x5a;
        assert(rows_destination[i]==expected);
    }
    assert(fh_gpu_texture_copy_error(a,n-1,in,b,512*512*4,out,n,12));
    assert(fh_gpu_texture_copy_error(a,n,in,b,fh_gpu_copy_span(out,n)-1,out,n,12));
    assert(fh_gpu_texture_copy_error(a,n,in,b,512*512*4,0x01f00000,n,12));
    assert(fh_gpu_texture_copy_error(a,n,in,0x30512000,512*512*4,out,n,12));
    assert(fh_gpu_texture_copy_error(a,n,in,a,512*512*4,out,n,12));
    assert(!fh_gpu_texture_copy_error(a+768,n-768,0x003001e0,b,384000,0,384000,12));
    assert(!fh_gpu_texture_copy_error(a,n,0,b,n,0,n,8));
    assert(fh_gpu_texture_copy_error(a,n,0,b,n,0,0,8));
    assert(fh_gpu_texture_copy_error(a,n,in,b,n,out,176,12));
    assert(fh_gpu_texture_copy_error(a,n,0,b,n,0,17,8));
    assert(fh_gpu_texture_copy_error(a+1,n,0,b,n,0,n,8));
    assert(fh_gpu_texture_copy_error(a,n,0,0x1f600000-16,n,0,n,8));
    unsigned copy_checks=18;
    /* Independently walk all touched tile rows, including shadow subranges
     * and partial final rows. Payload size alone is not a buffer bound. */
    for(unsigned first=0;first<50;first++)for(unsigned last=first+1;last<=50;last++){
        unsigned payload=(last-first)*264*8*4;
        unsigned occupied=0;
        for(unsigned row=first;row<last;row++)occupied=(row-first)*512*8*4+264*8*4;
        assert(fh_gpu_copy_span(out,payload)==occupied);
        assert(!fh_gpu_sampled_copy_error(a+first*264*8*4,n-first*264*8*4,in,
            0x30200000+first*512*8*4,512*512*4-first*512*8*4,out,payload,12,0x30200000,512*512*4));
        assert(fh_gpu_copy_span(out,payload-16)==occupied-16);copy_checks+=3;
    }
    const uintptr_t source=0x1f300000,destination=0x30512000;
    const size_t bytes=400*264*4;
    assert(!fh_gpu_readback_error(source,bytes,destination,bytes,264,400,0));
    assert(!fh_gpu_readback_error(source,bytes,destination,bytes,240,400,0));
    /* Original raw-copy descriptor is excluded from the port's readback
     * contract, even though Azahar accepts it. Not a universal GPU rule. */
    assert(fh_gpu_readback_error(source,bytes,destination,bytes,264,400,8));
    unsigned checks=3;
    for(unsigned offset=1;offset<8;offset++) {
        assert(fh_gpu_readback_error(source+offset,bytes,destination,bytes,264,400,0));
        assert(fh_gpu_readback_error(source,bytes,destination+offset,bytes,264,400,0));checks+=2;
    }
    assert(fh_gpu_readback_error(source,bytes-1,destination,bytes,264,400,0));
    assert(fh_gpu_readback_error(source,bytes,destination,bytes-1,264,400,0));
    assert(fh_gpu_readback_error(source,bytes,source+8,bytes,264,400,0));
    assert(fh_gpu_readback_error(0,bytes,destination,bytes,264,400,0));checks+=4;
    const unsigned invalid[][2]={{0,400},{264,0},{56,400},{264,8},{263,400},{264,399},{1032,400},{264,1032}};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);i++) {
        assert(fh_gpu_readback_error(source,bytes,destination,bytes,invalid[i][0],invalid[i][1],0));checks++;
    }
    printf("PASS %u GPU readback contract checks\n",checks);
    printf("PASS %u GPU-owned texture-copy checks including physical-crash negative controls\n",copy_checks);
}
