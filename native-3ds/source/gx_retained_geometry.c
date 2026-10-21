#include "gx_internal.h"
#include "fh_retained_geometry.h"
#include "fh_memory_bytes.h"
#include "fh_primitive.h"
#include "fh_gpu_lighting.h"
#include "fh_gx_dispatch.h"
#include "fh_compiled_geometry.h"
#include "fh_geometry_sets.h"
#include "fh_gx_source_ranges.h"
#include "fh_source_identity.h"
#include "fh_model_resources.h"
#include <string.h>
#include <stddef.h>
/* Melee gx.c separates immutable attributes from live GPU uniforms. Retain
 * only draws whose CPU outputs have no hidden palette/light dependencies.
 * Every consumed original list/array byte is owned and compared on reuse. */
enum { CAPACITY=4*1024*1024,SOURCE_CAPACITY=1024*1024,SOURCE_ENTRIES=256,SETS=128,WAYS=4,ENTRIES=SETS*WAYS };
typedef struct {
    unsigned raw,raster,texgens,fpscr,source_mask,flat,compiled,fur_post,texgen_inputs;
    uint32_t descriptors[26];FhVtxFormat formats[26];FhVtxArray arrays[26];
    FhChannel channel[4];float material[2][4],ambient[2][4],light_colors[8][4];
    FhTexGen gen[8];float matrix[8][3][4],post[8][3][4];
} Key;
typedef struct { const void *list;unsigned valid,offset,bytes,list_bytes,source_end,vertices,output_end,stamp,changing_misses,key_bytes,key_mode,model_version;uint64_t identity;unsigned sources[12];FhGxSourceRange ranges[12]; } Entry;
/* Melee geometry source sharing, adapted to one fixed normal-heap allocation.
 * The first MiB owns shared byte snapshots, the other three decoded geometry.
 * Tokens expire on overlap/replacement. Every reuse still compares all bytes;
 * SFA has no proven equivalent of Melee's tracked source-write epochs. */
typedef struct { const void *data;unsigned bytes,offset,token;const void *verified_data;unsigned verified_scope,verified_owned,verified_model; } Source;
static Source source_entries[SOURCE_ENTRIES];
static unsigned source_cursor,source_serial,source_slot;
static struct {const void *data;unsigned bytes;} scope_ranges[4];
static typeof(scope_ranges) producer_ranges;
static unsigned scope_serial,scope_active,scope_dispatch,producer_active;
unsigned fh_retained_scope_disabled,fh_retained_scope_validate;
unsigned fh_retained_model_scope_enabled;
unsigned fh_retained_two_sets,fh_retained_secondary_lookups,fh_retained_secondary_hits,fh_retained_secondary_records;
unsigned fh_retained_ranges_enabled,fh_retained_range_scans,fh_retained_range_fallbacks;
unsigned fh_retained_key_mode,fh_retained_key_calls,fh_retained_key_checks;
uint64_t fh_retained_key_bytes,fh_retained_key_dense_bytes;
uint64_t fh_retained_range_full_bytes,fh_retained_range_used_bytes,fh_retained_validation_bytes;
unsigned fh_retained_scope_reuses,fh_retained_scope_checks,fh_retained_scope_violations;
uint64_t fh_retained_scope_reused_bytes;
void fh_retained_reader_scope_begin(const void *const addresses[4],const unsigned bytes[4]){
    fh_gx_require(!scope_active,"Nested geometry reader scope");
    if(!++scope_serial){for(unsigned i=0;i<SOURCE_ENTRIES;i++)source_entries[i].verified_scope=0;scope_serial=1;}
    for(unsigned i=0;i<4;i++)scope_ranges[i]=(typeof(scope_ranges[0])){addresses[i],bytes[i]};
    scope_active=1;
}
void fh_retained_reader_scope_end(void){scope_active=0;}
static void scope_begin(const void *const addresses[4],const unsigned bytes[4],unsigned dispatch){
    fh_gx_require(!producer_active,"Nested immutable geometry scope");
    if(fh_retained_scope_disabled || fh_render_floor)return;
#ifdef FH_GX_DISPATCH_BUILD
    /* The source-only mode also writes reader metadata on the producer. */
    fh_gx_dispatch_join_deferred();
#endif
    for(unsigned i=0;i<4;i++)producer_ranges[i]=(typeof(producer_ranges[0])){addresses[i],bytes[i]};
    producer_active=1;scope_dispatch=0;
#ifdef FH_GX_DISPATCH_BUILD
    if(dispatch)scope_dispatch=fh_gx_dispatch_begin(addresses,bytes);
#endif
    if(!scope_dispatch)fh_retained_reader_scope_begin(addresses,bytes);
}
void fh_retained_scope_begin(const void *const addresses[4],const unsigned bytes[4]){
    scope_begin(addresses,bytes,1);
}
void fh_retained_scope_end(void){
    if(!producer_active)return;
#ifdef FH_GX_DISPATCH_BUILD
    if(scope_dispatch)fh_gx_dispatch_end();
#endif
    if(!scope_dispatch)fh_retained_reader_scope_end();
    producer_active=scope_dispatch=0;
}
unsigned fh_retained_model_scope_begin(const void *const addresses[4],const unsigned bytes[4]){
    if(!fh_retained_model_scope_enabled || producer_active || fh_retained_scope_disabled || fh_render_floor)return 0;
    /* Mode2 shares only source verification. The audited model loop keeps
     * running synchronously, without a new worker scope or its reader join.
     * Existing fur scopes still use the original explicit dispatch lifetime. */
    scope_begin(addresses,bytes,fh_retained_model_scope_enabled!=2);return 1;
}
void fh_retained_scope_invalidate(const void *address,size_t bytes){
    fh_model_resources_written(address,bytes);
    if(!scope_active)return;
    uintptr_t a=(uintptr_t)address;
    /* Every verified source is contained in one declared range. Matrix cache
     * notifications outside all four ranges cannot affect those sources. */
    if(address){
        unsigned overlaps=0;
        for(unsigned i=0;i<4;i++){
            uintptr_t b=(uintptr_t)scope_ranges[i].data;
            unsigned size=scope_ranges[i].bytes;
            if(b && size && bytes && (a>=b?a-b<size:b-a<bytes)){overlaps=1;break;}
        }
        if(!overlaps)return;
    }
    for(unsigned i=0;i<SOURCE_ENTRIES;i++){
        Source *s=source_entries+i;uintptr_t b=(uintptr_t)s->verified_data;
        if(s->verified_scope==scope_serial && (!address || (bytes &&
            (a>=b?a-b<s->bytes:b-a<bytes))))s->verified_scope=0;
    }
}
static int scope_contains(const void *data,unsigned bytes){
    if(!scope_active)return 0;
    uintptr_t a=(uintptr_t)data;
    for(unsigned i=0;i<4;i++){
        uintptr_t b=(uintptr_t)scope_ranges[i].data;unsigned size=scope_ranges[i].bytes;
        if(b && a>=b && a-b<=size && bytes<=size-(a-b))return 1;
    }
    return 0;
}
int fh_retained_scope_contains(const void *data,unsigned bytes){return scope_contains(data,bytes);}
int fh_retained_producer_scope_contains(const void *data,unsigned bytes){
    if(!producer_active)return 0;
    uintptr_t a=(uintptr_t)data;
    for(unsigned i=0;i<4;i++){
        uintptr_t b=(uintptr_t)producer_ranges[i].data;unsigned size=producer_ranges[i].bytes;
        if(b && a>=b && a-b<=size && bytes<=size-(a-b))return 1;
    }
    return 0;
}
static Entry entries[ENTRIES],*recording;
static unsigned char *arena;
static unsigned used,allocation_cursor,cursor,record_vertices,clock,hits,misses,replayed,packed_stride;
static unsigned replay_raster,replay_texgens,replay_raw;
static unsigned record_format,record_limit,record_ring;
/* Unique attribute payloads are shared across all chunks in a compiled list.
 * IDs come from the decoder's exact byte-verified cache, never pointer aliases. */
static struct { uint32_t id,offset; } record_unique[2048];
unsigned fh_retained_disabled;
unsigned fh_retained_prefer_compiled;
unsigned fh_retained_max_vertices=16384,fh_retained_max_list_bytes=65536;
static uint64_t identity;
static FhVertex *compiled_output;
static unsigned compiled_record;
static unsigned compiled_probe;
static uint64_t compiled_identity;
uint64_t fh_retained_compiled_identity(void){return compiled_identity;}
#ifdef FH_EMULATOR_TEST
unsigned fh_retained_reasons[12];
unsigned fh_compiled_profile[12];
#define RETAIN_REASON(n) (++fh_retained_reasons[n])
unsigned fh_retained_large_changes[9]; /* missing entry, key, list, source, gen, matrix, post, material, arrays */
unsigned fh_retained_large_layout[16];
#else
#define RETAIN_REASON(n) ((void)0)
#endif
_Static_assert(sizeof(Key)<2304,"Retained geometry key budget");
static unsigned aligned(unsigned n){return (n+3)&~3u;}
void fh_retained_reset(void){memset(entries,0,sizeof(entries));memset(source_entries,0,sizeof(source_entries));scope_active=producer_active=scope_dispatch=0;source_cursor=source_slot=0;used=0;allocation_cursor=SOURCE_CAPACITY;recording=NULL;compiled_output=NULL;compiled_probe=compiled_record=0;compiled_identity=0;}
static unsigned make_key(Key *k,unsigned format,unsigned raster) {
    if(fh_gx.draw.texgens>8)return 0;
    memset(k,0,sizeof(*k));k->raw=fh_gx.draw.raw_position;k->raster=raster;k->texgens=fh_gx_active_texgens;k->compiled=compiled_record;
    k->texgen_inputs=fh_gx_texgen_inputs;
    k->fur_post=compiled_record && fh_compiled_fur_post()!=NULL;
#ifdef __arm__
    __asm__ volatile("vmrs %0, fpscr":"=r"(k->fpscr));k->fpscr&=0x03c00000u;
#endif
    for(unsigned a=0;a<26;a++)if((k->descriptors[a]=fh_gx.descriptors[a])) {
        k->formats[a]=fh_gx.formats[format][a];
        if(a>=GX_VA_POS && fh_gx.descriptors[a]>=GX_INDEX8){
            k->arrays[a]=fh_gx.arrays[a];k->arrays[a].data=NULL;
        }
    }
    for(unsigned a=GX_VA_POS;a<=GX_VA_TEX7;a++)if(fh_gx.descriptors[a]>=GX_INDEX8)k->source_mask|=1u<<(a-GX_VA_POS);
    k->flat=(k->raw&2) && (fh_renderer_prepared_lighting()->mask&FH_LIGHT_FLAT);
    if(k->flat){
        /* Keep binding/format metadata and original list bytes validated,
         * but do not snapshot flat-shading inputs with no output consumer. */
        unsigned consumed=1u;
        const FhChannel *rgb=&fh_gx.channel[0],*alpha=&fh_gx.channel[2];
        if(rgb->material_source==GX_SRC_VTX || alpha->material_source==GX_SRC_VTX ||
           (alpha->enabled && alpha->ambient_source==GX_SRC_VTX))consumed|=1u<<(GX_VA_CLR0-GX_VA_POS);
        for(unsigned i=0;i<k->texgens;i++){
            if(!(k->texgen_inputs&(1u<<i)))continue;
            unsigned source=fh_gx.texgens[i].source;
            if(source==GX_TG_POS)continue; /* Positions are always consumed. */
            if(source<GX_TG_TEX0 || source>GX_TG_TEX7)return 0;
            consumed|=1u<<(GX_VA_TEX0+source-GX_TG_TEX0-GX_VA_POS);
        }
        k->source_mask&=consumed;
    }
    /* Melee geometry_key retains only baked attribute dependencies. SFA may
     * still bake RGB/alpha independently: unused lights and register channels
     * must not invalidate an otherwise identical owned mesh. */
    for(unsigned i=0;i<4;i++)if(raster&(1u<<(i&1))){
        const FhChannel *src=&fh_gx.channel[i];FhChannel *dst=&k->channel[i];
        unsigned channel=i&1,first=i>=2?3:0,end=i>=2?4:3;
        dst->material_source=src->material_source;
        if(k->raw&64)continue; /* Both RGB/alpha pairs live in the sphere program. */
        if(i==0 && (k->raw&2))continue; /* RGB material/lighting live on GPU. */
        if(i==2 && (k->raw&32))continue; /* Flat alpha also lives on GPU. */
        for(unsigned c=first;c<end;c++)if(src->material_source==GX_SRC_REG)k->material[channel][c]=fh_gx.material[channel][c];
        if(!src->enabled)continue;
        dst->enabled=src->enabled;dst->ambient_source=src->ambient_source;dst->mask=src->mask;
        if(src->mask){dst->diffuse=src->diffuse;dst->attenuation=src->attenuation;}
        for(unsigned c=first;c<end;c++){
            if(src->ambient_source==GX_SRC_REG)k->ambient[channel][c]=fh_gx.ambient[channel][c];
            for(unsigned light=0;light<8;light++)if(src->mask&(1u<<light))k->light_colors[light][c]=fh_gx.lights[light].color[c];
        }
    }
    for(unsigned i=0;i<k->texgens;i++) {
        if(!(k->texgen_inputs&(1u<<i)))continue;
        const FhTexGen *g=&fh_gx.texgens[i];k->gen[i]=*g;
        if(!i && (k->raw&4))continue; /* Original normal and post live on GPU. */
        if(fh_gx.descriptors[GX_VA_TEX0MTXIDX+i] || (g->type>=GX_TG_BUMP0 && g->type<=GX_TG_BUMP7))return 0;
        if(g->matrix!=GX_IDENTITY && g->type!=GX_TG_SRTG)
            memcpy(k->matrix[i],g->matrix<GX_TEXMTX0?fh_gx.position[g->matrix/3]:fh_gx.texture[g->matrix/3],48);
        if(g->postmatrix!=GX_PTIDENTITY && !(k->fur_post && i==1))memcpy(k->post[i],fh_gx.post[(g->postmatrix-GX_PTTEXMTX0)/3],48);
    }
    return 1;
}
#include "gx_compact_key.h"
static unsigned sources_size(unsigned mask) {
    unsigned n=0;
    for(unsigned a=GX_VA_POS;a<=GX_VA_TEX7;a++)if(mask&(1u<<(a-GX_VA_POS))) {
        const FhVtxArray *p=&fh_gx.arrays[a];
        if(!p->data || !p->bytes || p->bytes>256*1024 || n>512*1024-aligned(p->bytes))return ~0u;
        n+=aligned(p->bytes);
    }
    return n;
}
static unsigned source_acquire(const FhVtxArray *p) {
    const void *source_identity=fh_source_identity(p->data,p->bytes);
    Source *oldest=NULL;unsigned versions=0;
    for(unsigned i=0;i<SOURCE_ENTRIES;i++){
        Source *s=source_entries+i;
        if(s->token && s->data==source_identity && s->bytes==p->bytes){
            /* Acquisition and replay obey the same source lifetime. In an
             * audited scope a prior full comparison already covers this
             * binding; checked mode still verifies the current bytes. */
            if(fh_retained_source_valid(s->token,p->data,p->bytes))return s->token;
            versions++;if(!oldest || s->token<oldest->token)oldest=s;
        }
    }
    unsigned bytes=aligned(p->bytes);
    if(bytes>SOURCE_CAPACITY)return 0;
    if(source_serial==0xffffffu){memset(entries,0,sizeof(entries));memset(source_entries,0,sizeof(source_entries));source_serial=source_cursor=0;oldest=NULL;versions=0;}
    /* Streaming skin arrays change every frame. Keeping their entire history
     * would evict unrelated static meshes. Retain two exact versions per
     * binding, then replace the older allocation with a new generation token.
     * No prior owner can reuse the overwritten bytes under its old token. */
    if(versions>=2){
        unsigned slot=(unsigned)(oldest-source_entries),token=(++source_serial<<8)|slot;
          fh_copy_bytes(arena+oldest->offset,p->data,p->bytes);oldest->token=token;oldest->verified_scope=oldest->verified_owned=oldest->verified_model=0;return token;
    }
    if(bytes>SOURCE_CAPACITY-source_cursor)source_cursor=0;
    for(unsigned i=0;i<SOURCE_ENTRIES;i++){
        Source *s=source_entries+i;
        if(s->token && s->offset<source_cursor+bytes && source_cursor<s->offset+aligned(s->bytes))s->token=0;
    }
    unsigned slot=source_slot++&(SOURCE_ENTRIES-1),token=(++source_serial<<8)|slot;
    source_entries[slot]=(Source){source_identity,p->bytes,source_cursor,token};
    fh_copy_bytes(arena+source_cursor,p->data,p->bytes);source_cursor+=bytes;
    return token;
}
static int sources_live(const unsigned tokens[12]) {
    for(unsigned i=0;i<12;i++)if(tokens[i] && source_entries[tokens[i]&255].token!=tokens[i])return 0;
    return 1;
}
unsigned fh_retained_source_acquire(const void *data,unsigned bytes){
    if(!data || !bytes || bytes>256*1024)return 0;
    if(!arena){arena=fh_renderer_retained_alloc(CAPACITY);if(!arena)return 0;allocation_cursor=SOURCE_CAPACITY;}
    FhVtxArray source={.data=data,.bytes=bytes};return source_acquire(&source);
}
int fh_retained_source_valid(unsigned token,const void *data,unsigned bytes){
        const void *source_identity=fh_source_identity(data,bytes);
        unsigned version=fh_source_version(data,bytes);
        Source *s=source_entries+(token&255);
        /* Pointer identity is not a decoded attribute. Equivalent bindings
         * may share geometry only after comparing their complete owned source
         * snapshots; size, format, stride and byte order remain in the key. */
          if(!token || s->token!=token || s->bytes!=bytes || !data)return 0;
          if(version){
              /* An immutable owned version does not borrow the producer's
               * current model scope. Only the renderer owner touches these
               * cache observations; scope changes cannot revoke a live copy. */
              if(s->verified_owned==version && s->verified_data==source_identity){
                  if(fh_retained_scope_validate){
                      fh_retained_scope_checks++;fh_retained_validation_bytes+=bytes;
                      if(fh_memory_compare(arena+s->offset,data,bytes)){fh_retained_scope_violations++;return 0;}
                  }
                  fh_retained_scope_reuses++;fh_retained_scope_reused_bytes+=bytes;return 1;
              }
              fh_retained_validation_bytes+=bytes;
              if(fh_memory_compare(arena+s->offset,data,bytes))return 0;
              s->verified_data=source_identity;s->verified_owned=version;s->verified_scope=0;return 1;
          }
        if(fh_model_resources_mode && fh_model_resource_version(data,bytes)){
            if(s->verified_data!=source_identity)s->verified_model=0;
            int matched=fh_model_resource_match(&s->verified_model,data,arena+s->offset,bytes);
            if(matched){s->verified_data=source_identity;s->verified_scope=s->verified_owned=0;}
            return matched;
        }
        if(scope_active && s->verified_scope==scope_serial && s->verified_data==source_identity && s->verified_owned==version){
            if(fh_retained_scope_validate){
                fh_retained_scope_checks++;
                fh_retained_validation_bytes+=bytes;
                if(fh_memory_compare(arena+s->offset,data,bytes)){
                    fh_retained_scope_violations++;scope_active=0;return 0;
                }
            }
            fh_retained_scope_reuses++;fh_retained_scope_reused_bytes+=bytes;return 1;
        }
        fh_retained_validation_bytes+=bytes;
        if(fh_memory_compare(arena+s->offset,data,bytes))return 0;
        if(scope_contains(source_identity,bytes)){s->verified_data=source_identity;s->verified_scope=scope_serial;s->verified_owned=version;}
        return 1;
}
static int sources(const unsigned tokens[12],unsigned mask,const FhGxSourceRange ranges[12]) {
    for(unsigned a=GX_VA_POS;a<=GX_VA_TEX7;a++)if(mask&(1u<<(a-GX_VA_POS))) {
        const FhVtxArray *p=&fh_gx.arrays[a];
        unsigned slot=a-GX_VA_POS;const FhGxSourceRange *range=ranges+slot;
        if(!p->data || range->offset>p->bytes || range->bytes>p->bytes-range->offset ||
           !fh_retained_source_valid(tokens[slot],p->data+range->offset,range->bytes))return 0;
    }
    return 1;
}
static void replay(const Entry *e,const Key *key) {
    if(compiled_probe){compiled_identity=e->identity<<8;return;}
    if(compiled_output)compiled_identity=e->identity<<8;
    FhVertex batch[384];uint32_t ids[384];unsigned at=e->offset+e->source_end,chunk=0;
    while(at<e->output_end){
        unsigned count,bytes;fh_copy_bytes(&count,arena+at,4);fh_copy_bytes(&bytes,arena+at+4,4);unsigned end=at+bytes;at+=8;
        uint64_t id=(e->identity<<8)|++chunk;
        unsigned hit=0;
        if(compiled_output){} /* CPU replay cannot hit a GPU-only allocation. */
        else if(key->raw&8){
            unsigned cull=fh_gx.draw.cull;fh_gx.draw.cull=GX_CULL_NONE;
            hit=fh_renderer_replay_retained_points(id,count,&fh_gx.draw,fh_gx.point_size,fh_gx.point_offsets,fh_tex_offset(fh_gx.point_offset));
            fh_gx.draw.cull=cull;
        }else hit=fh_renderer_replay_retained(id,count,&fh_gx.draw);
        if(hit){at=end;continue;}
        fh_memory_zero(batch,count*sizeof(*batch));
        for(unsigned i=0;i<count;i++){
            unsigned token;fh_copy_bytes(&token,arena+at+i*4,4);unsigned source=token;
            FhVertex *v=&batch[i];fh_copy_bytes(v->position,arena+source,16);source+=16;
            for(unsigned c=0;c<2;c++)if(key->raster&(1u<<c)){fh_copy_bytes(v->color[c],arena+source,16);source+=16;}
            fh_copy_bytes(v->uv,arena+source,key->texgens*12);source+=key->texgens*12;
            if(key->raw&2){fh_copy_bytes(v->uv[7],arena+source,12);source+=12;}
            if(key->raw&4)v->uv[6][0]=1;
            fh_copy_bytes(&v->matrix_index,arena+source,4);source+=4;
            fh_copy_bytes(ids+i,arena+source,4);source+=4;
        }
        at=end;
        if(compiled_output){fh_copy_bytes(compiled_output,batch,count*sizeof(*batch));compiled_output+=count;}
        else if(key->raw&8){
            unsigned cull=fh_gx.draw.cull;fh_gx.draw.cull=GX_CULL_NONE;
            fh_renderer_draw_retained_points(id,batch,count,&fh_gx.draw,fh_gx.point_size,fh_gx.point_offsets,fh_tex_offset(fh_gx.point_offset));
            fh_gx.draw.cull=cull;
        }
        else fh_renderer_draw_retained(id,batch,count,&fh_gx.draw,ids);
    }
}
int fh_retained_begin(const void *list,unsigned bytes,unsigned vertices,unsigned format,unsigned raster,unsigned enabled) {
    const void *list_identity=fh_source_identity(list,bytes);
    compiled_record=compiled_output!=NULL || compiled_probe;
    recording=NULL;fh_retained_prefer_compiled=0;
#ifdef FH_EMULATOR_TEST
    if(vertices>4096){
        unsigned *p=fh_retained_large_layout;
        p[0]=fh_gx.draw.raw_position;p[1]=fh_gx.draw.texgens;p[2]=enabled;p[3]=format;
        p[4]=fh_gx.descriptors[GX_VA_TEX0MTXIDX];p[5]=fh_gx.descriptors[GX_VA_TEX1MTXIDX];
        memcpy(p+6,fh_gx.texgens,40);
    }
#endif
    if(fh_retained_disabled || !enabled || !vertices || vertices>fh_retained_max_vertices || vertices>16384 || !list || bytes>fh_retained_max_list_bytes || bytes>256*1024){RETAIN_REASON(1);return 0;}
    Key key;unsigned key_bytes=make_stored_key(&key,format,raster);if(!key_bytes){RETAIN_REASON(2);return 0;}
    unsigned array_bytes=sources_size(key.source_mask);if(array_bytes==~0u){RETAIN_REASON(3);return 0;}
    unsigned source_end=key_bytes+aligned(bytes);
    unsigned stride=24+((raster&1)?16:0)+((raster&2)?16:0)+key.texgens*12+((key.raw&2)?12:0);
    unsigned required=source_end+vertices*(stride+4)+((vertices+383)/384)*8;
    if(required>CAPACITY-SOURCE_CAPACITY){RETAIN_REASON(4);return 0;}
    if(!arena){arena=fh_renderer_retained_alloc(CAPACITY);if(!arena){RETAIN_REASON(5);return 0;}allocation_cursor=SOURCE_CAPACITY;}
    uintptr_t hash=(uintptr_t)list_identity;Entry *set=entries+fh_geometry_primary_set(hash,SETS-1)*WAYS,*entry=NULL,*second=NULL;
    ++clock;Entry *changed=NULL;
    for(unsigned i=0;i<(fh_retained_two_sets?WAYS*2:WAYS);i++) {
        if(i==WAYS){second=entries+fh_geometry_secondary_set(hash,SETS-1)*WAYS;fh_retained_secondary_lookups++;}
        Entry *e=i<WAYS?set+i:second+i-WAYS;
        if(e->valid && e->list==list_identity && !sources_live(e->sources))e->valid=0;
#ifdef FH_EMULATOR_TEST
        if(!fh_retained_key_mode && vertices>4096 && e->valid && e->list==list_identity && e->list_bytes==bytes && e->source_end==source_end && !e->key_mode){
            const Key *old=(const Key*)(arena+e->offset);
            fh_retained_large_changes[1]+=!!fh_memory_compare(old,&key,sizeof(key));
            fh_retained_large_changes[2]+=!!fh_memory_compare(arena+e->offset+sizeof(key),list,bytes);
            fh_retained_large_changes[3]+=!sources(e->sources,key.source_mask,e->ranges);
            fh_retained_large_changes[4]+=!!fh_memory_compare(old->gen,key.gen,sizeof(key.gen));
            fh_retained_large_changes[5]+=!!fh_memory_compare(old->matrix,key.matrix,sizeof(key.matrix));
            fh_retained_large_changes[6]+=!!fh_memory_compare(old->post,key.post,sizeof(key.post));
            fh_retained_large_changes[7]+=!!fh_memory_compare(old->material,key.material,sizeof(key.material));
            fh_retained_large_changes[8]+=!!fh_memory_compare(old->arrays,key.arrays,sizeof(key.arrays));
        }
#endif
        if(e->valid && e->list==list_identity && e->list_bytes==bytes && e->vertices==vertices && e->source_end==source_end && e->key_bytes==key_bytes && e->key_mode==!!fh_retained_key_mode &&
           !fh_memory_compare(arena+e->offset,&key,key_bytes) &&
            (fh_model_resources_mode?fh_model_resource_match(&e->model_version,list,arena+e->offset+key_bytes,bytes):!fh_memory_compare(arena+e->offset+key_bytes,list,bytes)) && sources(e->sources,key.source_mask,e->ranges)) {
            if(i>=WAYS)fh_retained_secondary_hits++;
            RETAIN_REASON(6);e->stamp=clock;hits++;replayed+=vertices;replay(e,&key);return 1;
        }
        if(e->valid && e->list==list_identity && e->list_bytes==bytes && e->vertices==vertices)changed=e;
        if(!entry || (!e->valid && (i<WAYS || entry->valid)) || (entry->valid && e->stamp<entry->stamp))entry=e;
    }
    if(enabled==3){
#ifdef FH_EMULATOR_TEST
        if(!changed)fh_compiled_profile[11]++;
        else if(changed->key_bytes!=key_bytes || changed->key_mode!=!!fh_retained_key_mode || fh_memory_compare(arena+changed->offset,&key,key_bytes))fh_compiled_profile[8]++;
        else if(fh_memory_compare(arena+changed->offset+key_bytes,list,bytes))fh_compiled_profile[10]++;
        else fh_compiled_profile[9]++;
#endif
        return 0; /* Identity-only miss: caller performs ordinary decode. */
    }
    if(enabled!=2){
        misses++;
#ifdef FH_EMULATOR_TEST
        if(!changed)RETAIN_REASON(7);
        else if(changed->key_bytes!=key_bytes || changed->key_mode!=!!fh_retained_key_mode || fh_memory_compare(arena+changed->offset,&key,key_bytes))RETAIN_REASON(8);
        else RETAIN_REASON(9);
#endif
    }
    /* Repeated dynamic keys used to rebuild thousands of expanded vertices
     * and immediately evict them again. Preserve an existing complete snapshot
     * while the caller tries the indexed decoder. Re-record periodically so
     * geometry that becomes static can regain the ordinary retained path. */
    if(enabled==1 && changed && vertices>=384 && !(key.raw&8) && (++changed->changing_misses&31u)){
        RETAIN_REASON(10);changed->stamp=clock;fh_retained_prefer_compiled=1;return 0;
    }
#ifdef FH_EMULATOR_TEST
    if(vertices>4096)fh_retained_large_changes[0]++;
#endif
    unsigned snapshots[12]={0};
    FhGxSourceRange ranges[12]={0};
    for(unsigned a=GX_VA_POS;a<=GX_VA_TEX7;a++)if(key.source_mask&(1u<<(a-GX_VA_POS)))ranges[a-GX_VA_POS].bytes=fh_gx.arrays[a].bytes;
    if(fh_retained_ranges_enabled){
        fh_retained_range_scans++;
        if(!fh_gx_source_ranges(list,bytes,format,key.source_mask,ranges)){
            fh_retained_range_fallbacks++;
            for(unsigned a=GX_VA_POS;a<=GX_VA_TEX7;a++)ranges[a-GX_VA_POS]=(FhGxSourceRange){0,fh_gx.arrays[a].bytes};
        }
    }
    for(unsigned a=GX_VA_POS;a<=GX_VA_TEX7;a++)if(key.source_mask&(1u<<(a-GX_VA_POS))){
        unsigned slot=a-GX_VA_POS;FhVtxArray span=fh_gx.arrays[a];
        fh_retained_range_full_bytes+=span.bytes;fh_retained_range_used_bytes+=ranges[slot].bytes;
        span.data+=ranges[slot].offset;span.bytes=ranges[slot].bytes;snapshots[slot]=source_acquire(&span);
    }
    /* Ring wrap can retire a snapshot acquired earlier in this same call.
     * Decline recording rather than publishing a partial dependency set. */
    if(!sources(snapshots,key.source_mask,ranges))return 0;
    unsigned offset;
    /* Reserve address range for the worst case, but evict and own only bytes
     * actually written. Repeated indices must not evict a megabyte of meshes. */
    /* A compact entry is normally smaller than the pessimistic expanded
     * reservation. Rebuild in its old address range whenever it fits; actual
     * growth below retires overlaps before writing. Requiring worst-case
     * capacity here would turn every animated miss into whole-arena churn. */
    record_ring=!entry->bytes || entry->offset<SOURCE_CAPACITY || required>CAPACITY-entry->offset;
    if(!record_ring)offset=entry->offset;
    else {
        if(required>CAPACITY-allocation_cursor)allocation_cursor=SOURCE_CAPACITY;
        offset=allocation_cursor;
    }
    record_limit=offset+required;
    for(unsigned i=0;i<ENTRIES;i++){
        Entry *other=entries+i;
        if(other!=entry && other->bytes && other->offset<offset+source_end && offset<other->offset+other->bytes)
            memset(other,0,sizeof(*other));
    }
    /* These compact bytes are copied into the ordinary GPU stream before
     * replay returns, so replacing an old CPU cache block cannot overwrite
     * an in-flight GPU source. Reuse avoids whole-arena churn on animation. */
    if(second && entry>=second && entry<second+WAYS)fh_retained_secondary_records++;
    *entry=(Entry){.list=list_identity,.offset=offset,.bytes=source_end,.list_bytes=bytes,.source_end=source_end,.vertices=vertices,.stamp=clock,.identity=++identity,.key_bytes=key_bytes,.key_mode=!!fh_retained_key_mode};
    memcpy(entry->sources,snapshots,sizeof(snapshots));
    memcpy(entry->ranges,ranges,sizeof(ranges));
    fh_copy_bytes(arena+offset,&key,key_bytes);fh_copy_bytes(arena+offset+key_bytes,list,bytes);
    cursor=offset+source_end;record_vertices=0;packed_stride=stride;
    if(record_ring)allocation_cursor=cursor;if(cursor>used)used=cursor;
    memset(record_unique,0,sizeof(record_unique));
    replay_raster=raster;replay_texgens=key.texgens;replay_raw=key.raw;record_format=format;recording=entry;return 0;
}
int fh_retained_read_compiled(const void *list,unsigned bytes,unsigned vertices,unsigned format,unsigned raster,FhVertex *output){
    compiled_identity=0;
    compiled_output=output;
    int hit=fh_retained_begin(list,bytes,vertices,format,raster,2);
    compiled_output=NULL;
    return hit;
}
uint64_t fh_retained_probe_compiled(const void *list,unsigned bytes,unsigned vertices,unsigned format,unsigned raster){
    compiled_identity=0;compiled_probe=1;
    fh_retained_begin(list,bytes,vertices,format,raster,3);
    compiled_probe=0;return compiled_identity;
}
void fh_retained_record(const FhVertex *v,unsigned count,const uint32_t *ids) {
    if(!recording)return;
    if(count>384 || !ids){recording=NULL;return;}
    for(unsigned i=0;i<count;i++)if(v[i].matrix_index<0 || ((replay_raw&4) && !v[i].uv[6][0])){
#ifdef FH_EMULATOR_TEST
        fh_compiled_profile[v[i].matrix_index<0?6:7]++;
#endif
        recording=NULL;return;
    }
    unsigned tokens[384],fresh[384],unique=0,start=cursor,payload=cursor+8+count*4;
    for(unsigned i=0;i<count;i++){
        unsigned slot=ids[i]&2047;
        if(ids[i] && record_unique[slot].id==ids[i])tokens[i]=record_unique[slot].offset;
        else {
            tokens[i]=payload+unique*packed_stride;fresh[unique++]=i;
            if(ids[i]){record_unique[slot].id=ids[i];record_unique[slot].offset=tokens[i];}
        }
    }
    unsigned end=payload+unique*packed_stride;
    if(end>record_limit){recording=NULL;return;}
    for(unsigned i=0;i<ENTRIES;i++){
        Entry *other=entries+i;
        if(other!=recording && other->bytes && other->offset<end && start<other->offset+other->bytes)
            memset(other,0,sizeof(*other));
    }
    unsigned bytes=end-start;fh_copy_bytes(arena+start,&count,4);fh_copy_bytes(arena+start+4,&bytes,4);
    fh_copy_bytes(arena+start+8,tokens,count*4);cursor=payload;
    for(unsigned n=0;n<unique;n++) {
        unsigned i=fresh[n];
        fh_copy_bytes(arena+cursor,v[i].position,16);cursor+=16;
        for(unsigned c=0;c<2;c++)if(replay_raster&(1u<<c)){fh_copy_bytes(arena+cursor,v[i].color[c],16);cursor+=16;}
        fh_copy_bytes(arena+cursor,v[i].uv,replay_texgens*12);cursor+=replay_texgens*12;
        if(replay_raw&2){fh_copy_bytes(arena+cursor,v[i].uv[7],12);cursor+=12;}
        fh_copy_bytes(arena+cursor,&v[i].matrix_index,4);cursor+=4;
        fh_copy_bytes(arena+cursor,ids+i,4);cursor+=4;
    }
    recording->bytes=cursor-recording->offset;
    if(record_ring)allocation_cursor=cursor;if(cursor>used)used=cursor;
    record_vertices+=count;
}
void fh_retained_end(void) {
    if(!recording)return;
    /* Lists cannot mutate GX state; renderer work consumes them synchronously.
     * Check source storage again in case a service callback touched an array. */
    Key key;unsigned key_bytes=make_stored_key(&key,record_format,replay_raster);
    recording->valid=record_vertices==recording->vertices && cursor<=recording->offset+recording->bytes &&
        key_bytes && recording->key_bytes==key_bytes && recording->key_mode==!!fh_retained_key_mode && !fh_memory_compare(&key,arena+recording->offset,key_bytes) &&
        !fh_memory_compare(recording->list,arena+recording->offset+key_bytes,recording->list_bytes) &&
        sources(recording->sources,key.source_mask,recording->ranges);
    if(!recording->valid)RETAIN_REASON(11);
    if(recording->valid && compiled_record)compiled_identity=recording->identity<<8;
    recording->output_end=cursor;recording=NULL;
}
void fh_retained_stats(unsigned *h,unsigned *m,unsigned *v,unsigned *n){*h=hits;*m=misses;*v=replayed;*n=used;}
