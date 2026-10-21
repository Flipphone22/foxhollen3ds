#include "fh_fur_detail.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
    for(unsigned n=1;n<=16;n++)for(unsigned b=0;b<=16;b++){
        unsigned count=fh_fur_detail_count(n,b),previous=0,total=0;
        assert(count && count<=n);
        float full=1.f,reduced=1.f,scale=1.008f;
        unsigned completed=0;
        for(unsigned i=0;i<count;i++){
            unsigned layer=fh_fur_detail_index(i,count,n);
            assert(layer<n && (!i?layer==0:layer>previous));
            unsigned advance=i?layer-previous:1;total+=advance;
            while(completed<=layer){full*=scale;completed++;}
            reduced*=fh_fur_detail_scale(scale,advance);
            assert(fabsf(full-reduced)<0.000002f);
            previous=layer;
        }
        assert(previous==n-1 && total==n);
        if(!b || b>=n)assert(count==n);
    }
    puts("PASS: original endpoints, monotonic shell/noise indices, cumulative fur depth, full-detail identity");
}
