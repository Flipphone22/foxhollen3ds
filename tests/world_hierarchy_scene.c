#include "fh_native_scene.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned word(const unsigned char *p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
int main(int argc,char **argv){
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));long length=ftell(f);rewind(f);
    unsigned char *pack=malloc(length);assert(pack && fread(pack,1,length,f)==(size_t)length);fclose(f);
    unsigned packets=0,groups=0,checks=0;
    for(unsigned i=0;i<word(pack+8);i++){
        const unsigned char *entry=pack+16+i*32,*packet=pack+word(entry+12);
        if(memcmp(packet,"FHNSCN08",8))continue;
        unsigned mb=word(packet+68),gb=word(packet+72);
        unsigned char *meta=malloc(mb),*gpu=aligned_alloc(16,(gb+15)&~15u);
        assert(meta && gpu);memcpy(meta,packet,mb);memcpy(gpu,packet+mb,gb);
        FhSceneAsset a;assert(fh_native_scene_open(&a,meta,mb,gpu,gb,word(entry),word(entry+4),word(entry+8)));
        for(unsigned j=0;j<a.groups;j++){
            FhSceneGroup g;assert(fh_native_scene_group(&a,j,&g));
            unsigned chunks=(g.indices+11)/12,coarse=(chunks+15)/16;
            FhWorldBounds *bounds=malloc(chunks*sizeof(*bounds)),*parents=malloc(coarse*sizeof(*parents));
            assert(bounds && parents);
            for(unsigned c=0;c<chunks;c++){
                unsigned n=g.indices-c*12;if(n>12)n=12;
                fh_world_bounds_build(bounds+c,g.vertex,g.index+c*12,n,a.compact,g.format);
            }
            for(unsigned c=0;c<coarse;c++){
                unsigned n=chunks-c*16;if(n>16)n=16;
                fh_world_parent_build(parents+c,bounds+c*16,n);
            }
            for(unsigned pose=0;pose<12;pose++){
                float planes[4][4]={{0}},center[3];
                for(unsigned k=0;k<3;k++)center[k]=(bounds[0].lo[k]+bounds[0].hi[k])*.5f;
                for(unsigned p=0;p<4;p++){
                    unsigned axis=(pose+p/2)%3;float sign=(p&1)?-1:1;
                    planes[p][axis]=sign;planes[p][3]=-sign*center[axis]+(float)(pose*25)-50;
                }
                unsigned first=pose%g.indices;first-=first%3;
                unsigned count=g.indices-first;
                FhWorldRange original[344],candidate[344];
                unsigned n=fh_world_visible_ranges(original,344,bounds,g.indices,first,count,planes);
                unsigned m=fh_world_visible_hierarchy(candidate,344,bounds,parents,g.indices,first,count,planes);
                assert(n==m && !memcmp(original,candidate,n*sizeof(*original)));checks++;
            }
            free(parents);free(bounds);groups++;
        }
        free(meta);free(gpu);packets++;
    }
    assert(packets && checks);free(pack);
    printf("{\"world_packets\":%u,\"groups\":%u,\"exact_ordered_range_checks\":%u}\n",packets,groups,checks);
}
