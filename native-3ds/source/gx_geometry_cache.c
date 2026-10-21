#include "fh_source_identity.h"
#include "gx_internal.h"
#include "fh_geometry_cache.h"
#include "fh_retained_geometry.h"
#include <string.h>
#include "fh_memory_bytes.h"
/* Melee gx.c retains owned geometry and verifies its source snapshots. This
 * narrower cache owns whole list/array bytes and only triangle draw output.
 * Every reuse compares all sources; no write-notification assumption is made. */
enum { ARENA_BYTES=512*1024,ENTRIES=32,MAX_ARRAY_BYTES=256*1024,
       STATE_START=offsetof(FhGx,descriptors),STATE_END=offsetof(FhGx,palettes) };
typedef struct { unsigned raster,raw,texgens,float_mode,texgen_inputs;unsigned char state[STATE_END-STATE_START]; } GeometryKey;
typedef struct { unsigned valid,offset,bytes,list_bytes,source_end,vertices,output_end; } Entry;
static unsigned char arena[2*ARENA_BYTES] __attribute__((aligned(32)));
void *fh_geometry_compiled_storage(void){return arena+ARENA_BYTES;}
static Entry entries[ENTRIES],*recording;
static unsigned arena_used,record_cursor,record_vertices,hits,misses,replayed;
/* The title and Palace traces had no hits: recording every animated list was
 * pure overhead. After 32 consecutive misses, probe two consecutive candidates
 * every 256 calls. A verified hit immediately restores ordinary admission.
 * Bypassed calls still perform the complete decoder and renderer work. */
static unsigned consecutive_misses,probe_phase;
void fh_geometry_reset(void) {
    memset(entries,0,sizeof(entries));recording=NULL;arena_used=0;
    consecutive_misses=probe_phase=0;
}
_Static_assert(sizeof(GeometryKey)<17*1024,"Geometry state snapshot budget");
static unsigned aligned(unsigned bytes){return (bytes+3)&~3u;}
static void key(GeometryKey *out,unsigned raster) {
    out->raster=raster;out->raw=fh_gx.draw.raw_position;out->texgens=fh_gx.draw.texgens;
    out->texgen_inputs=fh_gx_texgen_inputs;
    unsigned fpscr=0;
#ifdef __arm__
    __asm__ volatile("vmrs %0, fpscr":"=r"(fpscr));
#endif
    out->float_mode=fpscr&0x03c00000u;
    fh_copy_bytes(out->state,(const unsigned char*)&fh_gx+STATE_START,sizeof(out->state));
}
static unsigned vertex_stride(unsigned format) {
    unsigned stride=0;
    for(unsigned attr=0;attr<=GX_VA_TEX7;attr++) {
        unsigned mode=fh_gx.descriptors[attr];if(!mode)continue;
        const FhVtxFormat *f=&fh_gx.formats[format][attr];unsigned bytes=0;
        if(attr<GX_VA_POS)bytes=1;
        else if(attr==GX_VA_CLR0 || attr==GX_VA_CLR1){static const unsigned sizes[]={2,3,4,2,3,4};if(f->type>=6)return 0;bytes=sizes[f->type];}
        else {if(f->type>GX_F32)return 0;unsigned n=attr==GX_VA_POS?(f->count==GX_POS_XY?2:3):attr==GX_VA_NRM?(f->count==GX_NRM_XYZ?3:9):(f->count==GX_TEX_S?1:2);bytes=n*(f->type<2?1:f->type<4?2:4);}
        if(mode!=GX_DIRECT){bytes=mode==GX_INDEX8?1:2;if(attr==GX_VA_NRM && f->count==GX_NRM_NBT3)bytes*=3;}
        stride+=bytes;
    }
    return stride<=256?stride:0;
}
unsigned fh_geometry_candidate(const void *list,unsigned bytes) {
    if(consecutive_misses>=32 && (probe_phase++&255u)>=2)return 0;
    return fh_geometry_vertex_count(list,bytes);
}
unsigned fh_geometry_decline_reason;
unsigned fh_geometry_vertex_count(const void *list,unsigned bytes) {
    fh_geometry_decline_reason=0;
    if(!list || bytes>fh_retained_max_list_bytes || bytes>256*1024){fh_geometry_decline_reason=1;return 0;}
    const unsigned char *p=list;unsigned at=0,format=8,stride=0,expanded=0,points=2;
    while(at<bytes) {
        unsigned op=p[at++];if(!op)continue;
        unsigned kind=op&0xf8;
        if(kind!=GX_TRIANGLES && kind!=GX_TRIANGLESTRIP && kind!=GX_TRIANGLEFAN && kind!=GX_QUADS && kind!=GX_POINTS){fh_geometry_decline_reason=2;return 0;}
        if(points==2)points=kind==GX_POINTS;
        if(points!=(kind==GX_POINTS)){fh_geometry_decline_reason=2;return 0;}
        if(format==8){format=op&7;stride=vertex_stride(format);if(!stride){fh_geometry_decline_reason=3;return 0;}}
        if(format!=(op&7)){fh_geometry_decline_reason=4;return 0;}
        if(bytes-at<2){fh_geometry_decline_reason=5;return 0;}
        unsigned count=(unsigned)p[at]<<8|p[at+1];at+=2;
        unsigned size=stride*count;if(size>bytes-at){fh_geometry_decline_reason=5;return 0;}at+=size;
        expanded+=kind==GX_POINTS?count:kind==GX_TRIANGLES?count/3*3:kind==GX_QUADS?count/4*6:count>2?(count-2)*3:0;
        if(expanded>fh_retained_max_vertices || expanded>16384){fh_geometry_decline_reason=6;return 0;}
    }
    return expanded;
}
static unsigned source_size(void) {
    unsigned total=0;
    for(unsigned i=GX_VA_POS;i<=GX_VA_TEX7;i++)if(fh_gx.descriptors[i]>=GX_INDEX8) {
        const FhVtxArray *a=&fh_gx.arrays[i];
        if(!a->data || !a->bytes || a->bytes>MAX_ARRAY_BYTES || total>MAX_ARRAY_BYTES-aligned(a->bytes))return ~0u;
        total+=aligned(a->bytes);
    }
    return total;
}
static int sources(unsigned offset,int copy) {
    for(unsigned i=GX_VA_POS;i<=GX_VA_TEX7;i++)if(fh_gx.descriptors[i]>=GX_INDEX8) {
        const FhVtxArray *a=&fh_gx.arrays[i];
        if(copy)fh_copy_bytes(arena+offset,a->data,a->bytes);
        else if(fh_memory_compare(arena+offset,a->data,a->bytes))return 0;
        offset+=aligned(a->bytes);
    }
    return 1;
}
int fh_geometry_begin(const void *list,unsigned bytes,unsigned vertices,unsigned raster) {
    const void *list_identity=fh_source_identity(list,bytes);
    if(recording){recording=NULL;return 0;}
    unsigned array_bytes=source_size();if(array_bytes==~0u)return 0;
    unsigned source_end=sizeof(GeometryKey)+aligned(bytes)+array_bytes;
    unsigned required=source_end+vertices*(sizeof(FhVertex)+4)+((vertices+383)/384)*4;
    if(required>ARENA_BYTES)return 0;
    GeometryKey snapshot;key(&snapshot,raster);
    uintptr_t slot=(uintptr_t)list_identity>>2;slot^=slot>>12;Entry *entry=&entries[slot&(ENTRIES-1)];
    if(entry->valid && entry->list_bytes==bytes && entry->vertices==vertices && entry->source_end==source_end &&
       !fh_memory_compare(arena+entry->offset,&snapshot,sizeof(snapshot)) &&
       !fh_memory_compare(arena+entry->offset+sizeof(snapshot),list,bytes) &&
       sources(entry->offset+sizeof(snapshot)+aligned(bytes),0)) {
        unsigned cursor=entry->offset+entry->source_end;
        while(cursor<entry->output_end) {
            unsigned count;fh_copy_bytes(&count,arena+cursor,4);cursor+=4;
            const FhVertex *v=(const FhVertex*)(arena+cursor);cursor+=count*sizeof(*v);
            const uint32_t *ids=(const uint32_t*)(arena+cursor);cursor+=count*4;
            fh_renderer_draw_identified(v,count,&fh_gx.draw,ids);
        }
        consecutive_misses=probe_phase=0;hits++;replayed+=vertices;return 1;
    }
    misses++;if(consecutive_misses<32)consecutive_misses++;
    if(required>ARENA_BYTES-arena_used){memset(entries,0,sizeof(entries));arena_used=0;}
    *entry=(Entry){.offset=arena_used,.bytes=required,.list_bytes=bytes,.source_end=source_end,.vertices=vertices};
    fh_copy_bytes(arena+arena_used,&snapshot,sizeof(snapshot));fh_copy_bytes(arena+arena_used+sizeof(snapshot),list,bytes);
    sources(arena_used+sizeof(snapshot)+aligned(bytes),1);
    record_cursor=arena_used+source_end;record_vertices=0;arena_used+=required;recording=entry;
    return 0;
}
void fh_geometry_record(const FhVertex *vertices,unsigned count,const uint32_t *identities) {
    if(!recording)return;
    unsigned bytes=4+count*(sizeof(*vertices)+4);
    if(bytes>recording->offset+recording->bytes-record_cursor){recording=NULL;return;}
    fh_copy_bytes(arena+record_cursor,&count,4);record_cursor+=4;
    fh_copy_bytes(arena+record_cursor,vertices,count*sizeof(*vertices));record_cursor+=count*sizeof(*vertices);
    fh_copy_bytes(arena+record_cursor,identities,count*4);record_cursor+=count*4;record_vertices+=count;
}
void fh_geometry_end(void) {
    if(!recording)return;
    GeometryKey snapshot;const GeometryKey *stored=(const GeometryKey*)(arena+recording->offset);key(&snapshot,stored->raster);
    recording->valid=record_vertices==recording->vertices && record_cursor==recording->offset+recording->bytes &&
        !fh_memory_compare(stored,&snapshot,sizeof(snapshot)) && sources(recording->offset+sizeof(snapshot)+aligned(recording->list_bytes),0);
    recording->output_end=record_cursor;recording=NULL;
}
void fh_geometry_counters(unsigned *h,unsigned *m,unsigned *v,unsigned *used){*h=hits;*m=misses;*v=replayed;*used=arena_used;}
