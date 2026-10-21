/* Bounded C/u16 adaptation of pinned meshoptimizer optimizeVertexCacheFifo.
 * Allocation and adjacency layout changed; candidate selection is retained.
 * MIT License, Copyright (c) 2016-2025 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "fh_pica_index_cache.h"
#include <limits.h>
#include <string.h>

unsigned fh_pica_index_cache_scratch(unsigned vertices,unsigned indices){
    if(!vertices || vertices>32768 || !indices || indices>32766 || indices%3)return 0;
    /* Counts, offsets, live counts, timestamps; u16 adjacency and dead ends;
     * one byte per triangle. All arithmetic bounded by the checks above. */
    return (16*vertices+4*indices+indices/3+3)&~3u;
}
static int disjoint(const void*a,unsigned an,const void*b,unsigned bn){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=UINTPTR_MAX-an && y<=UINTPTR_MAX-bn && (x+an<=y || y+bn<=x);
}
int fh_pica_index_cache_order(uint16_t*out,const uint16_t*in,unsigned vertices,
    unsigned indices,unsigned cache_size,void*scratch,unsigned scratch_bytes){
    unsigned need=fh_pica_index_cache_scratch(vertices,indices);
    if(!need || !out || !in || !scratch || scratch_bytes<need || cache_size<3 || cache_size>64 ||
       ((uintptr_t)out&1) || ((uintptr_t)in&1) || ((uintptr_t)scratch&3) ||
       !disjoint(out,indices*2,in,indices*2) || !disjoint(out,indices*2,scratch,need) ||
       !disjoint(in,indices*2,scratch,need))return 0;
    for(unsigned i=0;i<indices;i++)if(in[i]>=vertices)return 0;
    uint32_t*counts=scratch,*offsets=counts+vertices,*live=offsets+vertices,*stamp=live+vertices;
    uint16_t*adjacent=(uint16_t*)(stamp+vertices),*dead=adjacent+indices;
    unsigned char*emitted=(unsigned char*)(dead+indices);
    memset(scratch,0,need);
    for(unsigned i=0;i<indices;i++)counts[in[i]]++;
    unsigned offset=0;
    for(unsigned i=0;i<vertices;i++){offsets[i]=offset;offset+=counts[i];}
    for(unsigned i=0;i<indices;i++)adjacent[offsets[in[i]]+live[in[i]]++]=(uint16_t)(i/3);
    unsigned current=0,top=0,cursor=1,timestamp=cache_size+1,written=0;
    while(current!=UINT_MAX){
        unsigned begin=top;
        for(unsigned i=0;i<counts[current];i++){
            unsigned triangle=adjacent[offsets[current]+i];
            if(emitted[triangle])continue;
            for(unsigned k=0;k<3;k++){
                unsigned v=in[triangle*3+k];out[written++]=(uint16_t)v;
                dead[top++]=(uint16_t)v;live[v]--;
                if(timestamp-stamp[v]>cache_size)stamp[v]=timestamp++;
            }
            emitted[triangle]=1;
        }
        current=UINT_MAX;int best=-1;
        for(unsigned i=begin;i<top;i++){
            unsigned v=dead[i];if(!live[v])continue;
            int priority=2*live[v]+timestamp-stamp[v]<=cache_size?(int)(timestamp-stamp[v]):0;
            if(priority>best){current=v;best=priority;}
        }
        if(current==UINT_MAX){
            while(top){unsigned v=dead[--top];if(live[v]){current=v;break;}}
            if(current==UINT_MAX){
                while(cursor<vertices && !live[cursor])cursor++;
                if(cursor<vertices)current=cursor;
            }
        }
    }
    return written==indices;
}
