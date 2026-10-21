#include "gx_internal.h"
#include "fh_gpu_lighting.h"
#include "fh_light_lod.h"
#include "fh_gpu_texgen.h"
#include "fh_uvgen_matrix.h"
#include "fh_geometry_cache.h"
#include "fh_retained_geometry.h"
#include "fh_compiled_geometry.h"
#include "fh_primitive.h"
#include "fh_vertex_copy.h"
#include <math.h>
#include <string.h>
#include "fh_memory_bytes.h"
#include "fh_bounds_hint.h"
#include <stdio.h>
#include "fh_cpu_light_cache.h"
#include "fh_texgen_cache.h"
#include "fh_material_scope.h"
#include "fh_source_identity.h"
#include "fh_model_resources.h"

uint64_t fh_gx_phase_ticks[4]; /* setup, visibility, retained replay, decode/submit */
unsigned fh_render_floor_lists,fh_render_floor_primitives,fh_render_floor_copies;
unsigned fh_render_floor_setters,fh_render_floor_matrix_ops;
uint64_t fh_gx_shading_ticks[3]; /* preparation, lighting, texgen; inclusive */
#if defined(FH_EMULATOR_TEST) || defined(__arm__)
#ifdef __arm__
extern uint64_t svcGetSystemTick(void);
extern unsigned fh_render_attribution_active;
#define PROFILE_TICKS() svcGetSystemTick()
#else
#define PROFILE_TICKS() fh_platform_ticks()
#endif
#ifdef FH_EMULATOR_TEST
#define PROFILE_ENABLED 1
#else
#define PROFILE_ENABLED fh_render_attribution_active
#endif
#define PHASE_START uint64_t phase_start=PROFILE_ENABLED?PROFILE_TICKS():0
#define PHASE_END(i) do { if(PROFILE_ENABLED) { uint64_t end=PROFILE_TICKS();fh_gx_phase_ticks[i]+=end-phase_start;phase_start=end; } } while(0)
#else
#define PHASE_START
#define PHASE_END(i)
#endif

enum { BATCH=384, VERTEX_BYTES=256 };
static unsigned primitive,format,expected,received,stride,pending_bytes,batch_count;
static unsigned gpu_points;
static FhVertex *compiled_capture;
static const void *compiled_list;
static unsigned compiled_bytes;
unsigned fh_compiled_decode_hits,fh_compiled_decode_misses,fh_compiled_decode_vertices;
unsigned fh_compiled_direct_vertices;
static unsigned char pending[VERTEX_BYTES],*recording;
static unsigned record_capacity,record_size;
#ifdef FH_EMULATOR_TEST
#include "fh_light_workload.h"
/* Profiling observes native ARM11 ticks. Do not convert each measurement to
 * the game's 40.5 MHz timebase; that conversion adds two software divisions.
 * Host decoder fixtures use their existing synthetic clock. */
static void capture_cpu_lighting(const float output[2][4],const float input[2][4],const float *eye,const float *normal){
    static unsigned checked,count;static FILE *file;static FhLightWorkload captured;
    if(!checked){FILE *request=fopen("sdmc:/3ds/foxhollow/capture_lighting.txt","rb");checked=1;if(request){checked=2;fclose(request);}}
    if(checked!=2 || count>=128)return;
    if(fh_gx.channel[0].attenuation!=GX_AF_SPEC || fh_gx.channel[1].attenuation!=GX_AF_SPEC ||
       !fh_gx.channel[0].enabled || !fh_gx.channel[1].enabled || !fh_gx.channel[2].enabled || !fh_gx.channel[3].enabled)return;
    FhLightWorkload current={.magic="FHLIGHT1",.count=128};
    _Static_assert(sizeof(current.channels)==sizeof(fh_gx.channel) && sizeof(current.lights)==sizeof(fh_gx.lights),"Lighting capture layout");
    memcpy(current.channels,fh_gx.channel,sizeof(current.channels));memcpy(current.lights,fh_gx.lights,sizeof(current.lights));
    memcpy(current.ambient,fh_gx.ambient,sizeof(current.ambient));memcpy(current.material,fh_gx.material,sizeof(current.material));
    if(!file){file=fopen("sdmc:/3ds/foxhollow/lighting-workload.bin","wb");if(!file){checked=1;return;}captured=current;fwrite(&captured,sizeof(captured),1,file);}
    if(memcmp(&current,&captured,sizeof(current)))return;
    FhLightSample sample;memcpy(sample.eye,eye,12);memcpy(sample.normal,normal,12);memcpy(sample.input,input,32);memcpy(sample.output,output,32);
    if(fwrite(&sample,sizeof(sample),1,file)!=1)fh_platform_panic("Lighting workload write failed");
    if(++count==128){fclose(file);file=NULL;}
}
unsigned fh_gx_last_list_vertices;
const FhDrawState *fh_gx_profile_state(unsigned *channels,unsigned *generators){
    _Static_assert(sizeof(fh_gx.channel)==24*sizeof(unsigned) && sizeof(fh_gx.texgens)==40*sizeof(unsigned),"Profile state words");
    if(channels)memcpy(channels,fh_gx.channel,sizeof(fh_gx.channel));
    if(generators)memcpy(generators,fh_gx.texgens,sizeof(fh_gx.texgens));
    return &fh_gx.draw;
}
unsigned fh_gx_cpu_lighting[36]; /* channel * 9 + attenuation * 3 + diffuse */
unsigned fh_gx_uncached_vertices[7]; /* size/topology, texgens, CPU light,
                                      normal palette, emboss, CPU transform, miss */
unsigned fh_gx_uncached_lists[7]; /* empty, byte limit, topology, format invalid,
                                   mixed formats, truncated, vertex limit */
#endif
static FhVertex batch[BATCH],first,previous[3];
static uint32_t batch_identity[BATCH],first_identity,previous_identity[3],vertex_identity;
typedef struct { unsigned hash,epoch,format,stride,raw_position,identity;unsigned char bytes[VERTEX_BYTES];FhVertex vertex; } Decoded;
/* Bounded per-list cache. Captured opening lists lose useful repeated keys
 * with 64 slots; 256 retains more without introducing cross-list lifetimes. */
enum { DECODED_ENTRIES=256 };
static Decoded decoded_cache[DECODED_ENTRIES];
_Static_assert(sizeof(decoded_cache)<=107*1024,"Decoded vertex cache budget");
static unsigned cache_enabled,cache_epoch,decoded_vertices,reused_vertices;
static unsigned list_batching;
typedef struct { unsigned attr,mode,bytes,components,component_bytes,offset;float scale;FhVtxFormat format; } PreparedField;
static PreparedField fields[GX_VA_TEX7+1];
static unsigned field_count;
static const PreparedField *position_field;
static unsigned position_matrix_offset;
unsigned fh_gx_early_cull_checks,fh_gx_early_culled_lists,fh_gx_early_culled_vertices;
/* Pinned Melee retains local bounds alongside owned source snapshots. Keep
 * matrices out of this key: camera/skinning still change the live plane test.
 * A private arena prevents source mutation or allocator reuse hiding changes. */
enum { BOUNDS_BYTES=1024*1024,BOUNDS_SETS=64,BOUNDS_WAYS=4 };
typedef struct { PreparedField position;FhVtxArray array;unsigned format,stride,matrix_offset,fpscr; } BoundsKey;
typedef struct {
    const void *list;BoundsKey key;float low[10][3],high[10][3];
    unsigned valid,offset,capacity,list_bytes,source_token,used,examined,stamp;
    unsigned plane_hint,model_version;
} BoundsEntry;
static BoundsEntry bounds_entries[BOUNDS_SETS*BOUNDS_WAYS];
static unsigned char *bounds_storage;
static unsigned bounds_used,bounds_clock;
unsigned fh_gx_bounds_hits,fh_gx_bounds_misses,fh_gx_bounds_disabled;
#ifdef FH_EMULATOR_TEST
unsigned fh_bounds_hint_mode;
#else
unsigned fh_bounds_hint_mode=1;
#endif
unsigned fh_bounds_hint_hits,fh_bounds_hint_misses,fh_bounds_hint_checks;
const unsigned fh_bounds_hint_bytes=sizeof(unsigned)*BOUNDS_SETS*BOUNDS_WAYS;
void fh_gx_bounds_reset(void){memset(bounds_entries,0,sizeof(bounds_entries));bounds_used=0;}
static BoundsEntry *bounds_find(const void *list,unsigned bytes,BoundsKey *key) {
    if(fh_gx_bounds_disabled || bytes>65536 || !bytes)return NULL;
    memset(key,0,sizeof(*key));key->position=*position_field;key->format=format;key->stride=stride;key->matrix_offset=position_matrix_offset;
    if(position_field->mode!=GX_DIRECT)key->array=fh_gx.arrays[GX_VA_POS];
    const void *array_data=key->array.data;
    key->array.data=fh_source_identity(array_data,key->array.bytes);
    const void *identity=fh_source_identity(list,bytes);
#ifdef __arm__
    __asm__ volatile("vmrs %0, fpscr":"=r"(key->fpscr));key->fpscr&=0x03c00000u;
#endif
    unsigned array_bytes=key->array.bytes,required=(bytes+3)&~3u;
    if(array_bytes>256*1024 || (array_bytes && !key->array.data))return NULL;
    if(!bounds_storage){bounds_storage=fh_renderer_cpu_cache_alloc(BOUNDS_BYTES);if(!bounds_storage)return NULL;}
    uintptr_t hash=(uintptr_t)identity;BoundsEntry *set=bounds_entries+(((hash>>4)^(hash>>13))&(BOUNDS_SETS-1))*BOUNDS_WAYS,*entry=NULL;
    ++bounds_clock;
    for(unsigned i=0;i<BOUNDS_WAYS;i++){
        BoundsEntry *e=set+i;
        if(e->valid && e->list==identity && e->list_bytes==bytes && !fh_memory_compare(&e->key,key,sizeof(*key)) &&
           (fh_model_resources_mode?fh_model_resource_match(&e->model_version,list,bounds_storage+e->offset,bytes):!fh_memory_compare(bounds_storage+e->offset,list,bytes)) &&
           (!array_bytes || fh_retained_source_valid(e->source_token,array_data,array_bytes))){
            e->stamp=bounds_clock;fh_gx_bounds_hits++;return e;
        }
        if(!entry || !e->valid || (entry->valid && e->stamp<entry->stamp))entry=e;
    }
    fh_gx_bounds_misses++;
    /* Share the complete position snapshot with other lists/decoded meshes.
     * Duplicating the same array per list exhausted this arena and reset all
     * boxes repeatedly. Matrices are still evaluated live below. */
    unsigned token=array_bytes?fh_retained_source_acquire(array_data,array_bytes):0;
    if(array_bytes && !token)return NULL;
    unsigned offset,capacity;
    if(entry->capacity>=required){offset=entry->offset;capacity=entry->capacity;}
    else {
        if(required>BOUNDS_BYTES-bounds_used){fh_gx_bounds_reset();entry=set;}
        offset=bounds_used;capacity=(required+3)&~3u;bounds_used+=capacity;
    }
    *entry=(BoundsEntry){.list=identity,.key=*key,.offset=offset,.capacity=capacity,.list_bytes=bytes,.source_token=token,.stamp=bounds_clock};
    fh_copy_bytes(bounds_storage+offset,list,bytes);
    return entry;
}
static unsigned lighting_needs_normal,raster_channels,gpu_lighting,gpu_lighting_palette,gpu_lighting_flat;
unsigned fh_palette_scope_mode,fh_palette_scope_calls,fh_palette_scope_matrices,fh_palette_scope_checks;
static unsigned lighting_palette_mask=0x3ffu;
static unsigned texgen_needs_eye;
static unsigned gpu_texgen;
unsigned fh_post_uvgen_enabled,fh_post_uvgen_draws;
unsigned fh_post_uvgen_attempts,fh_post_uvgen_matrix_rejections,fh_post_uvgen_material_rejections;
float fh_post_uvgen_last[12];
unsigned fh_gx_active_texgens;
unsigned fh_gx_texgen_inputs;
/* Only finiteness is needed for deferred-lighting eligibility, not the full
 * libc classification into zero/subnormal/normal/infinity/NaN. The GX ABI
 * already requires IEEE binary32. Preserve NaN/Inf fallbacks exactly. */
static inline unsigned finite_vertex_float(float value) {
    union { float f; uint32_t u; } bits={value};
    return (bits.u&0x7f800000u)!=0x7f800000u;
}
typedef struct { const float (*matrix)[4],(*post)[4];unsigned dynamic,alias,cache_id,pending; } PreparedTexGen;
static PreparedTexGen prepared_texgen[8];
unsigned fh_texgen_lazy_mode,fh_texgen_lazy_planned,fh_texgen_lazy_resolved,fh_texgen_lazy_checks;
unsigned fh_fur_post_mode,fh_fur_post_vertices,fh_fur_post_checks;
static unsigned compiled_post_template;
const float (*fh_compiled_fur_post(void))[4]{
    const FhTexGen *g=&fh_gx.texgens[1];
    if(!fh_fur_post_mode || !list_batching || fh_gx.draw.raw_position!=7 ||
       fh_gx_active_texgens!=5 || !fh_renderer_compiled_fur_available(&fh_gx.draw) ||
       prepared_texgen[1].dynamic || prepared_texgen[1].alias!=1 || !prepared_texgen[1].post ||
       g->type!=GX_TG_MTX2x4 || g->source<GX_TG_TEX0 || g->source>GX_TG_TEX7 ||
       g->matrix!=GX_IDENTITY || g->normalize)return NULL;
    /* No generated coordinate may consume this unresolved value, including
     * an alias that would otherwise copy it before the post transform. */
    for(unsigned i=0;i<5;i++)if(i!=1){
        unsigned source=fh_gx.texgens[i].source;
        if((i>1 && prepared_texgen[i].alias==1) ||
           (source>=GX_TG_TEXCOORD0 && source<=GX_TG_TEXCOORD6))return NULL;
    }
    return prepared_texgen[1].post;
}
/* Channel pairs may share geometric light factors, but never their ambient,
 * material, or accumulated colors. Rebuilt at each primitive boundary. */
static unsigned lighting_group[4];
static unsigned constant_lighting;
static unsigned cpu_light_state_id;
static float constant_colors[2][4];
void fh_gx_vertex_counters(unsigned *decoded,unsigned *reused) { *decoded=decoded_vertices;*reused=reused_vertices; }
static u32 be16(const u8 *p) { return (u32)p[0]<<8|p[1]; }
static u32 word(const u8 *p,int le) { return le?(u32)p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24:(u32)p[0]<<24|(u32)p[1]<<16|(u32)p[2]<<8|p[3]; }
static unsigned component_bytes(unsigned type) { fh_gx_require(type<=GX_F32,"Invalid vertex component type");return type<2?1:type<4?2:4; }
static unsigned count(unsigned attr,FhVtxFormat f) {
    if(attr==GX_VA_POS) return f.count==GX_POS_XY?2:3;
    if(attr==GX_VA_NRM) return f.count==GX_NRM_XYZ?3:9;
    return f.count==GX_TEX_S?1:2;
}
static unsigned attribute_bytes(unsigned attr,FhVtxFormat f) {
    if(attr<GX_VA_POS) return 1;
    if(attr==GX_VA_CLR0 || attr==GX_VA_CLR1) {
        static const unsigned sizes[]={2,3,4,2,3,4};fh_gx_require(f.type<6,"Invalid color format");return sizes[f.type];
    }
    return count(attr,f)*component_bytes(f.type);
}
static void components(float *out,const u8 *p,unsigned n,unsigned type,int le,float scale) {
    /* A GX attribute has one component format. Dispatch once per attribute,
     * rather than once per coordinate in the common position/normal/UV loops. */
    switch(type) {
    case GX_U8:for(unsigned i=0;i<n;i++)out[i]=p[i]*scale;break;
    case GX_S8:for(unsigned i=0;i<n;i++)out[i]=(s8)p[i]*scale;break;
    case GX_U16:
        if(le)for(unsigned i=0;i<n;i++)out[i]=((unsigned)p[i*2]|(unsigned)p[i*2+1]<<8)*scale;
        else for(unsigned i=0;i<n;i++)out[i]=be16(p+i*2)*scale;
        break;
    case GX_S16:
        if(le)for(unsigned i=0;i<n;i++)out[i]=(s16)((unsigned)p[i*2]|(unsigned)p[i*2+1]<<8)*scale;
        else for(unsigned i=0;i<n;i++)out[i]=(s16)be16(p+i*2)*scale;
        break;
    case GX_F32:
        if(le)fh_copy_bytes(out,p,n*4);
        else for(unsigned i=0;i<n;i++){u32 bits=word(p+i*4,0);fh_copy_bytes(out+i,&bits,4);}
        break;
    default:fh_platform_panic("Invalid vertex type");
    }
}
static const u8 *indexed(unsigned attr,unsigned index,unsigned offset,unsigned bytes,int *le) {
    FhVtxArray *array=&fh_gx.arrays[attr];uint64_t begin=(uint64_t)index*array->stride+offset;
    if(!array->data || begin+bytes>array->bytes) {
        char message[160];snprintf(message,sizeof(message),"Vertex array %u index %u offset %u bytes %u exceeds %u",attr,index,offset,bytes,array->bytes);fh_platform_panic(message);
    }
    *le=array->little_endian;return array->data+(size_t)begin;
}
static void decode_color(float out[4],const u8 *p,unsigned type,int le) {
    unsigned r,g,b,a=255,value;
    switch(type) {
    case GX_RGB565:value=le?p[0]|(unsigned)p[1]<<8:be16(p);r=((value>>11)&31)*255/31;g=((value>>5)&63)*255/63;b=(value&31)*255/31;break;
    case GX_RGB8:case GX_RGBX8:r=p[0];g=p[1];b=p[2];break;
    case GX_RGBA4:value=le?p[0]|(unsigned)p[1]<<8:be16(p);r=(value>>12)*17;g=((value>>8)&15)*17;b=((value>>4)&15)*17;a=(value&15)*17;break;
    case GX_RGBA6:value=(unsigned)p[0]<<16|(unsigned)p[1]<<8|p[2];r=((value>>18)&63)*255/63;g=((value>>12)&63)*255/63;b=((value>>6)&63)*255/63;a=(value&63)*255/63;break;
    case GX_RGBA8:r=p[0];g=p[1];b=p[2];a=p[3];break;
    default:fh_platform_panic("Unsupported vertex color format");
    }
    out[0]=r/255.f;out[1]=g/255.f;out[2]=b/255.f;out[3]=a/255.f;
}
static unsigned list_outside(const unsigned char *list,unsigned bytes) {
    if(!position_field)return 0;
    unsigned cpu=fh_renderer_cpu_cull_available(&fh_gx.draw);
    if(!cpu && !fh_renderer_gpu_cull_available(&fh_gx.draw))return 0;
    float pica[4][4];if(!cpu)fh_vertex_pica_projection(pica,&fh_gx.draw);
    /* Build local bounds for larger lists instead of transforming every
     * position. Separate boxes preserve mixed-matrix skinning. No cross-list
     * cache or stale array lifetime is introduced. Small draws stay exact. */
    unsigned use_bounds=bytes/stride>=32,bounds_used=0;
    float low[10][3],high[10][3];
    unsigned at=0,outside=cpu?63:255,examined=0;fh_gx_early_cull_checks++;
    BoundsKey bounds_key;BoundsEntry *retained=use_bounds?bounds_find(list,bytes,&bounds_key):NULL;
    if(retained && retained->valid){
        bounds_used=retained->used;examined=retained->examined;
        memcpy(low,retained->low,sizeof(low));memcpy(high,retained->high,sizeof(high));
        goto test_bounds;
    }
    while(at<bytes) {
        unsigned op=list[at++];if(!op)continue;
        unsigned kind=op&0xf8;
        if((op&7)!=format || (kind!=GX_TRIANGLES && kind!=GX_TRIANGLESTRIP && kind!=GX_TRIANGLEFAN && kind!=GX_QUADS) || bytes-at<2)return 0;
        unsigned n=be16(list+at);at+=2;
        if(n>(bytes-at)/stride)return 0;
        for(unsigned i=0;i<n;i++) {
            const unsigned char *v=list+at+i*stride,*data=v+position_field->offset;int le=0;
            unsigned matrix=position_matrix_offset==~0u?0:v[position_matrix_offset];
            if(matrix>27 || matrix%3)return 0;
            if(position_field->mode!=GX_DIRECT) {
                unsigned index=position_field->mode==GX_INDEX8?*data:be16(data);
                data=indexed(GX_VA_POS,index,0,position_field->bytes,&le);
            }
            float p[4]={0,0,0,1},q[4];
            components(p,data,position_field->components,position_field->format.type,le,position_field->scale);
            if(use_bounds){
                unsigned slot=matrix/3;
                for(unsigned c=0;c<3;c++)if(!__builtin_isfinite(p[c]))return 0;
                if(!(bounds_used&(1u<<slot))){
                    for(unsigned c=0;c<3;c++)low[slot][c]=high[slot][c]=p[c];
                    bounds_used|=1u<<slot;
                }else for(unsigned c=0;c<3;c++){
                    if(p[c]<low[slot][c])low[slot][c]=p[c];
                    if(p[c]>high[slot][c])high[slot][c]=p[c];
                }
                /* If this first vertex is inside either required volume, a
                 * common-plane rejection is already impossible. */
                if(i || examined)continue;
            }
            if(!cpu){
                outside&=fh_vertex_conservative_outside(p,matrix,&fh_gx.draw,pica);
                if(!(outside&15) || !(outside&240))return 0;
                continue;
            }
            fh_vertex_clip_position(q,p,matrix,&fh_gx.draw);
            /* Nonfinite coordinates must retain the ordinary processing path.
             * This mask is exactly the later CPU renderer's common-plane test. */
            for(unsigned c=0;c<4;c++)if(!__builtin_isfinite(q[c]))return 0;
            unsigned mask=(q[0]<-q[3]?1u:0)|(q[0]>q[3]?2u:0)|(q[1]<-q[3]?4u:0)|(q[1]>q[3]?8u:0)|(q[2]<-q[3]?16u:0)|(q[2]>0?32u:0);
            outside&=mask;if(!outside)return 0;
        }
        at+=n*stride;examined+=n;
    }
    if(!examined)return 0;
    if(use_bounds && retained){
        retained->used=bounds_used;retained->examined=examined;
        memcpy(retained->low,low,sizeof(low));memcpy(retained->high,high,sizeof(high));
        retained->valid=1;
    }
test_bounds:
    if(use_bounds){
        unsigned reject;
        if(fh_bounds_hint_mode && retained){
            unsigned reused=0;
            reject=fh_bounds_set_hinted(low,high,bounds_used,&fh_gx.draw,cpu?NULL:pica,cpu,outside,&retained->plane_hint,&reused);
            if(reused)fh_bounds_hint_hits++;else fh_bounds_hint_misses++;
            if(fh_bounds_hint_mode==2){
                unsigned scalar=!!fh_bounds_set_original(low,high,bounds_used,&fh_gx.draw,cpu?NULL:pica,cpu,outside);
                fh_bounds_hint_checks++;if(reject!=scalar)fh_platform_panic("Bounds hint scalar mismatch");
            }
        }else reject=!!fh_bounds_set_original(low,high,bounds_used,&fh_gx.draw,cpu?NULL:pica,cpu,outside);
        if(!reject)return 0;
    }
    fh_gx_early_culled_lists++;fh_gx_early_culled_vertices+=examined;return 1;
}
static float dot(const float *a,const float *b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static void normalize(float *v) { float length=sqrtf(dot(v,v));if(length>1e-20f) for(unsigned i=0;i<3;i++) v[i]/=length; }
static float clamp(float value) { return value<0?0:value>1?1:value; }
static void transform(float *out,const float matrix[3][4],const float *value,float w) { for(unsigned i=0;i<3;i++) out[i]=dot(matrix[i],value)+matrix[i][3]*w; }
static void illuminate(float output[2][4],float color[2][4],const float *position,const float *normal,unsigned deferred) {
    FhCpuLightEntry *cached=NULL;FhCpuLightInput key;
    if(cpu_light_state_id && !deferred){
        fh_copy_bytes(key.eye,position,12);fh_copy_bytes(key.normal,normal,12);fh_copy_bytes(key.color,color,32);
        int hit;cached=fh_cpu_light_probe(cpu_light_state_id,&key,&hit);
        if(hit){fh_copy_bytes(output,cached->output,32);return;}
    }
    float factors[4][8][2],directions[8][3],distances[8];
    unsigned factors_ready=0,directions_ready=0;
    for(unsigned channel=0;channel<2;channel++) for(unsigned part=0;part<2;part++) {
        if(!(raster_channels&(1u<<channel)))continue;
        unsigned index=channel+part*2,group=lighting_group[index];
        const FhChannel *state=&fh_gx.channel[index];
        unsigned first=part?3:0,end=part?4:3;
        if(deferred && ((fh_gx.draw.raw_position&64) || index==0 || (index==2 && (fh_gx.draw.raw_position&32)))){
            for(unsigned j=first;j<end;j++)output[channel][j]=state->material_source==GX_SRC_REG?1.f:color[channel][j];
            continue;
        }
        if(constant_lighting&(1u<<index)) {
            for(unsigned j=first;j<end;j++)output[channel][j]=constant_colors[channel][j];
            continue;
        }
        const float *material=state->material_source==GX_SRC_VTX?color[channel]:fh_gx.material[channel];
        if(!state->enabled){for(unsigned j=first;j<end;j++)output[channel][j]=material[j];continue;}
#ifdef FH_EMULATOR_TEST
        if(state->attenuation<3 && state->diffuse<3)fh_gx_cpu_lighting[index*9+state->attenuation*3+state->diffuse]++;
#endif
        const float *ambient=state->ambient_source==GX_SRC_VTX?color[channel]:fh_gx.ambient[channel];
        float light[4];fh_copy_bytes(light,ambient,16);
        for(unsigned i=0;i<8;i++) if(state->mask&(1u<<i)) {
            const FhLight *source=&fh_gx.lights[i];
            if(state->diffuse==GX_DF_NONE && state->attenuation==GX_AF_NONE) {
                for(unsigned j=first;j<end;j++)light[j]+=source->color[j];
                continue;
            }
            if(!(factors_ready&(1u<<group))){
            float *delta=directions[i];
            if(!(directions_ready&(1u<<i))){
                for(unsigned j=0;j<3;j++)delta[j]=source->position[j]-position[j];
                float distance=distances[i]=sqrtf(dot(delta,delta));
                if(distance>0)for(unsigned j=0;j<3;j++)delta[j]/=distance;
                directions_ready|=1u<<i;
            }
            float distance=distances[i];
            float diffuse=state->diffuse==GX_DF_NONE?1:dot(normal,delta);
            if(state->diffuse==GX_DF_CLAMP && diffuse<0) diffuse=0;
            float attenuation=1;
            if(state->attenuation!=GX_AF_NONE) {
                float cosine=state->attenuation==GX_AF_SPEC?dot(normal,source->direction):dot(delta,source->direction);
                if(cosine<0 || (state->attenuation==GX_AF_SPEC && dot(normal,delta)<0)) cosine=0;
                float denominator_input=state->attenuation==GX_AF_SPEC?cosine:distance;
                float numerator=source->a[0]+source->a[1]*cosine+source->a[2]*cosine*cosine;
                float denominator=source->k[0]+source->k[1]*denominator_input+source->k[2]*denominator_input*denominator_input;
                attenuation=numerator>0 && denominator>0?numerator/denominator:0;
            }
            factors[group][i][0]=diffuse;factors[group][i][1]=attenuation;
            }
            for(unsigned j=first;j<end;j++)light[j]+=source->color[j]*factors[group][i][0]*factors[group][i][1];
        }
        factors_ready|=1u<<group;
        for(unsigned j=first;j<end;j++)output[channel][j]=material[j]*clamp(light[j]);
    }
#ifdef FH_EMULATOR_TEST
    if(raster_channels==3 && !deferred)capture_cpu_lighting(output,color,position,normal);
#endif
    if(cached)fh_cpu_light_store(cached,cpu_light_state_id,&key,output);
}
static void flush(void) {
    if(batch_count) {
        unsigned cull=fh_gx.draw.cull;
        if(primitive==GX_POINTS || primitive==GX_LINES || primitive==GX_LINESTRIP) fh_gx.draw.cull=GX_CULL_NONE;
        if(gpu_points){
            fh_retained_record(batch,batch_count,batch_identity);
            fh_renderer_draw_points(batch,batch_count,&fh_gx.draw,fh_gx.point_size,fh_gx.point_offsets,fh_tex_offset(fh_gx.point_offset));
        }
        else {
            fh_geometry_record(batch,batch_count,batch_identity);
            fh_retained_record(batch,batch_count,batch_identity);
            fh_renderer_draw_identified(batch,batch_count,&fh_gx.draw,batch_identity);
        }
        fh_gx.draw.cull=cull;batch_count=0;
    }
}
static void triangle(const FhVertex *a,const FhVertex *b,const FhVertex *c,uint32_t ia,uint32_t ib,uint32_t ic) {
    if(batch_count+3>BATCH)flush();
    batch_identity[batch_count]=ia;fh_vertex_copy(&batch[batch_count++],a);
    batch_identity[batch_count]=ib;fh_vertex_copy(&batch[batch_count++],b);
    batch_identity[batch_count]=ic;fh_vertex_copy(&batch[batch_count++],c);
}
static void expanded(const FhVertex *a,const FhVertex *b) {
    FhVertex q[4];float vw=fh_gx.draw.viewport[2],vh=fh_gx.draw.viewport[3];
    int visible=b?fh_expand_line(a,b,fh_gx.line_width,fh_gx.line_offsets,fh_tex_offset(fh_gx.line_offset),vw,vh,q):
        fh_expand_point(a,fh_gx.point_size,fh_gx.point_offsets,fh_tex_offset(fh_gx.point_offset),vw,vh,q);
    if(visible) { triangle(&q[0],&q[1],&q[2],0,0,0);triangle(&q[2],&q[1],&q[3],0,0,0); }
}
static void output_vertex(const FhVertex *v,uint32_t identity) {
    if(compiled_capture){if(compiled_capture!=v)fh_vertex_copy(compiled_capture,v);return;}
    switch(primitive) {
    case GX_TRIANGLES:{
        /* Stage incomplete triangles without publishing them. The third vertex
         * commits the batch, preserving GX's discarded trailing vertices. */
        unsigned part=received%3;if(!part && batch_count+3>BATCH)flush();
        fh_vertex_copy(&batch[batch_count+part],v);batch_identity[batch_count+part]=identity;
        if(part==2)batch_count+=3;
        break;
    }
    case GX_TRIANGLESTRIP:{
        /* A two-slot ring supplies alternating winding without copying the
         * previous vertex into a second staging slot on every input. */
        if(received>=2)triangle(&previous[0],&previous[1],v,previous_identity[0],previous_identity[1],identity);
        unsigned slot=received&1;fh_vertex_copy(&previous[slot],v);previous_identity[slot]=identity;break;
    }
    case GX_TRIANGLEFAN:if(!received){first=*v;first_identity=identity;}else if(received>=2)triangle(&first,&previous[0],v,first_identity,previous_identity[0],identity);previous[0]=*v;previous_identity[0]=identity;break;
    case GX_QUADS:if(received%4==3) { triangle(&previous[0],&previous[1],&previous[2],previous_identity[0],previous_identity[1],previous_identity[2]);triangle(&previous[0],&previous[2],v,previous_identity[0],previous_identity[2],identity); }else {previous[received%4]=*v;previous_identity[received%4]=identity;}break;
    case GX_POINTS:
        if(gpu_points){if(batch_count==BATCH)flush();batch_identity[batch_count]=identity;fh_vertex_copy(batch+batch_count++,v);}
        else expanded(v,NULL);
        break;
    case GX_LINES:if(received&1) expanded(&previous[0],v);previous[0]=*v;break;
    case GX_LINESTRIP:if(received) expanded(&previous[0],v);previous[0]=*v;break;
    default:fh_platform_panic("Invalid GX primitive");
    }
}
static void decode_vertex(const u8 *bytes) {
    Decoded *cached=NULL;unsigned hash=2166136261u^format;
    /* Compiled callers already disable the packet cache. Keep their direct
     * output independent of that cache even if another caller enables it. */
    if(cache_enabled && !compiled_capture) {
        for(unsigned i=0;i<stride;i++)hash=(hash^bytes[i])*16777619u;
        cached=&decoded_cache[hash&(DECODED_ENTRIES-1)];
        if(cached->epoch==cache_epoch && cached->hash==hash && cached->format==format && cached->stride==stride && cached->raw_position==fh_gx.draw.raw_position && !fh_memory_compare(cached->bytes,bytes,stride)) {
            output_vertex(&cached->vertex,cached->identity);received++;reused_vertices++;
            if(!list_batching && expected!=GX_AUTO && received==expected)flush();
            return;
        }
    }
    float position[3]={0},normals[3][3]={{0,0,1},{0,1,0},{1,0,0}},colors[2][4]={{1,1,1,1},{1,1,1,1}},uv[8][3];fh_memory_zero(uv,sizeof(uv));
    unsigned matrix=fh_gx.current_matrix,tex_matrix[8];for(unsigned i=0;i<8;i++) { tex_matrix[i]=fh_gx.texgens[i].matrix;uv[i][2]=1; }
    const u8 *cursor=bytes;
    for(unsigned field=0;field<field_count;field++) {
        const PreparedField *prepared=&fields[field];unsigned attr=prepared->attr,mode=prepared->mode;
        if(attr<GX_VA_POS) { unsigned value=*cursor++;if(!attr) matrix=value;else tex_matrix[attr-1]=value;continue; }
        FhVtxFormat f=prepared->format;unsigned size=prepared->bytes;int le=0;
        const u8 *data=cursor;
        if(mode==GX_DIRECT) cursor+=size;
        else if(attr==GX_VA_NRM && f.count==GX_NRM_NBT3) {
            unsigned cb=prepared->component_bytes;float scale=prepared->scale;
            for(unsigned slice=0;slice<3;slice++) {
                unsigned index=mode==GX_INDEX8?*cursor++:be16(cursor);if(mode==GX_INDEX16) cursor+=2;
                data=indexed(attr,index,slice*3*cb,3*cb,&le);
                components(normals[slice],data,3,f.type,le,scale);
            }
            continue;
        } else {
            unsigned index=mode==GX_INDEX8?*cursor++:be16(cursor);if(mode==GX_INDEX16) cursor+=2;
            data=indexed(attr,index,0,size,&le);
        }
        if(attr==GX_VA_CLR0 || attr==GX_VA_CLR1) { decode_color(colors[attr-GX_VA_CLR0],data,f.type,le);continue; }
        float scale=prepared->scale;
        float *output=attr==GX_VA_POS?position:attr==GX_VA_NRM?&normals[0][0]:uv[attr-GX_VA_TEX0];
        components(output,data,prepared->components,f.type,le,scale);
    }
    fh_gx_require((unsigned)(cursor-bytes)==stride && matrix%3==0 && matrix/3<64,"Invalid decoded vertex layout or matrix");
    FhVertex temporary,*vertex=compiled_capture?compiled_capture:cached?&cached->vertex:&temporary;
    if(compiled_capture)fh_compiled_direct_vertices++;
    fh_memory_zero(vertex,sizeof(*vertex));
    float eye[4]={0,0,0,1},normal[3]={0};
    unsigned deferred=gpu_lighting;float palette_normal[3]={0};
    if(deferred) {
        float largest=0;
        for(unsigned c=0;c<3;c++) {
            float n=fabsf(normals[0][c]);if(n>largest)largest=n;
            if(!finite_vertex_float(position[c]) || fabsf(position[c])>1e5f || (!gpu_lighting_flat && (!finite_vertex_float(n) || n>64)))deferred=0;
        }
        if(!gpu_lighting_flat && largest>0 && largest<1e-6f)deferred=0;
        if(gpu_lighting_palette){
            if(matrix>=30)deferred=0;
            else if(gpu_lighting_palette>=2) {
                /* The palette eligibility bound guarantees a nonsingular
                 * transform and components below 64 for these raw normals.
                 * Keep tiny/extreme normals on the original scalar route. */
                float maximum=gpu_lighting_palette==3?8.f:16.f;
                float minimum=gpu_lighting_palette==3?8e-6f:4e-6f;
                if(largest>maximum || (largest>0 && largest<minimum))deferred=0;
            }else {
                transform(palette_normal,fh_gx.normal[matrix/3],normals[0],0);
                largest=0;
                for(unsigned c=0;c<3;c++){
                    float n=fabsf(palette_normal[c]);if(n>largest)largest=n;
                    if(!finite_vertex_float(n) || n>64)deferred=0;
                }
                if(largest>0 && largest<1e-6f)deferred=0;
            }
        }
    }
    vertex->matrix_index=-1;
    if((!gpu_lighting || deferred) && fh_gx.draw.raw_position && (!fh_gx.descriptors[GX_VA_PNMTXIDX] || matrix<30)){
        fh_copy_bytes(vertex->position,position,12);vertex->position[3]=1;vertex->matrix_index=fh_gx.descriptors[GX_VA_PNMTXIDX]?matrix:0;
    }
    else {transform(eye,fh_gx.position[matrix/3],position,1);for(unsigned i=0;i<4;i++)vertex->position[i]=dot(fh_gx.projection[i],eye)+fh_gx.projection[i][3];}
    /* CPU lighting/emboss still needs eye space. Final projection can use
     * the existing PICA palette path independently of that CPU calculation. */
    if(vertex->matrix_index>=0 && (texgen_needs_eye || lighting_needs_normal || (gpu_lighting && !deferred)))transform(eye,fh_gx.position[matrix/3],position,1);
    if(lighting_needs_normal || (gpu_lighting && !deferred)){transform(normal,fh_gx.normal[matrix/3],normals[0],0);normalize(normal);}
    illuminate(vertex->color,colors,eye,normal,deferred);
    for(unsigned i=0;i<fh_gx_active_texgens;i++) {
        if(!(fh_gx_texgen_inputs&(1u<<i)))continue;
        if(!i && (fh_gx.draw.raw_position&16) && vertex->matrix_index>=0){
            fh_copy_bytes(vertex->uv[0],uv[fh_gx.texgens[0].source-GX_TG_TEX0],12);
            vertex->uv[6][0]=1;continue;
        }
        if(!i && gpu_texgen && deferred && tex_matrix[0]==matrix+GX_TEXMTX0){vertex->uv[0][2]=1;vertex->uv[6][0]=1;continue;}
        FhTexGen *gen=&fh_gx.texgens[i];float source[3]={0,0,1};
        PreparedTexGen *prepared=&prepared_texgen[i];
        if(prepared->alias<i && tex_matrix[i]==tex_matrix[prepared->alias]) {
            fh_copy_bytes(vertex->uv[i],vertex->uv[prepared->alias],12);continue;
        }
        if(gen->type>=GX_TG_BUMP0 && gen->type<=GX_TG_BUMP7){
            fh_gx_require(gen->source>=GX_TG_TEXCOORD0 && gen->source<=GX_TG_TEXCOORD6 && gen->source-GX_TG_TEXCOORD0<i,"Invalid emboss texgen source");
            float direction[3],tangent[3],binormal[3];
            const FhLight *light=&fh_gx.lights[gen->type-GX_TG_BUMP0];
            for(unsigned c=0;c<3;c++)direction[c]=light->position[c]-eye[c];
            normalize(direction);
            transform(tangent,fh_gx.normal[matrix/3],normals[2],0);
            transform(binormal,fh_gx.normal[matrix/3],normals[1],0);
            fh_copy_bytes(vertex->uv[i],vertex->uv[gen->source-GX_TG_TEXCOORD0],12);
            vertex->uv[i][0]+=dot(direction,tangent);vertex->uv[i][1]+=dot(direction,binormal);
            continue;
        }
        if(gen->source<=GX_TG_TANGENT) fh_copy_bytes(source,gen->source==GX_TG_POS?position:normals[gen->source-1],12);
        else if(gen->source>=GX_TG_TEX0 && gen->source<=GX_TG_TEX7) fh_copy_bytes(source,uv[gen->source-GX_TG_TEX0],12);
        else if(gen->source>=GX_TG_TEXCOORD0 && gen->source<=GX_TG_TEXCOORD6) fh_copy_bytes(source,vertex->uv[gen->source-GX_TG_TEXCOORD0],12);
        else if(gen->source==GX_TG_COLOR0 || gen->source==GX_TG_COLOR1) { source[0]=vertex->color[gen->source-GX_TG_COLOR0][0];source[1]=vertex->color[gen->source-GX_TG_COLOR0][1]; }
        FhTexgenValue *generated=NULL;unsigned generated_hash=0,verify_generated=0;
        /* Resident GPU geometry and deferred coordinates never consume the
         * CPU sample cache. Resolve its owned matrix key only on CPU demand. */
        if(prepared->pending && !(compiled_post_template && i==1)){
            prepared->cache_id=texgen_state(prepared->matrix,prepared->post,gen->type==GX_TG_MTX2x4 || gen->type==GX_TG_SRTG,gen->normalize);
            prepared->pending=0;fh_texgen_lazy_resolved++;
        }
        if(prepared->cache_id && !(compiled_post_template && i==1)){
            generated=texgen_probe(prepared->cache_id,source,&generated_hash);
            if(generated->id==prepared->cache_id && generated->hash==generated_hash &&
               !fh_memory_compare(generated->input,source,12)){
                fh_texgen_cache_hits++;
                if(fh_texgen_lazy_mode!=2){fh_copy_bytes(vertex->uv[i],generated->output,12);continue;}
                verify_generated=1;
            }
            else fh_texgen_cache_misses++;
        }
        unsigned index=tex_matrix[i];
        if(index==GX_IDENTITY || gen->type==GX_TG_SRTG) fh_copy_bytes(vertex->uv[i],source,12);
        else {
            /* GX position and texture transforms share XF matrix indices.
             * SFA projects overlays through PNMTX0 before a post transform.
             * Match pinned Melee/Aurora: indices 0..27 read position matrices. */
            const float (*mtx)[4]=prepared->matrix;
            if(prepared->dynamic) {
                fh_gx_require(index%3==0 && index/3<64,"Invalid vertex texture matrix");
                mtx=index<GX_TEXMTX0?fh_gx.position[index/3]:fh_gx.texture[index/3];
            }
            transform(vertex->uv[i],mtx,source,1);
        }
        if(gen->type==GX_TG_MTX2x4 || gen->type==GX_TG_SRTG) vertex->uv[i][2]=1;
        if(gen->normalize) normalize(vertex->uv[i]);
        if(prepared->post && !(compiled_post_template && i==1)) {
            float result[3];transform(result,prepared->post,vertex->uv[i],1);fh_copy_bytes(vertex->uv[i],result,12);
        }
        if(generated){
            if(verify_generated){
                fh_gx_require(!fh_memory_compare(generated->output,vertex->uv[i],12),"Lazy texgen cache differs from scalar coordinates");
                fh_texgen_lazy_checks++;
            }
            fh_copy_bytes(generated->input,source,12);fh_copy_bytes(generated->output,vertex->uv[i],12);
            generated->hash=generated_hash;generated->id=prepared->cache_id;
        }
    }
    if(gpu_lighting)fh_copy_bytes(vertex->uv[7],gpu_lighting_palette==1?palette_normal:normals[0],12);
    if(!++vertex_identity)++vertex_identity;
    if(cached){cached->epoch=cache_epoch;cached->hash=hash;cached->format=format;cached->stride=stride;cached->raw_position=fh_gx.draw.raw_position;cached->identity=vertex_identity;fh_copy_bytes(cached->bytes,bytes,stride);}
    decoded_vertices++;output_vertex(vertex,vertex_identity);received++;
    if(!list_batching && expected!=GX_AUTO && received==expected) flush();
}
void fh_gx_end_primitive(void) {
    if(!stride) return;
    fh_gx_require(!pending_bytes && (expected==GX_AUTO || received==expected),"Incomplete GX primitive");
    if(!list_batching)flush();stride=0;
}
static void decode_compiled(FhVertex *output,const unsigned char *packet){
    compiled_capture=output;decode_vertex(packet);compiled_capture=NULL;
}
#include "gx_compiled_fast.h"
/* Admission history contains no reusable vertex values or source identities.
 * Dynamic meshes were repeatedly copying CPU snapshots and allocating GPU
 * packets that the very next animation update invalidated. Keep exact probes
 * for existing entries, but re-record sustained misses only once per 32 draws.
 * A verified hit immediately restores ordinary admission. Collisions merely
 * reset this performance heuristic; all output still uses current attributes. */
typedef struct {const void *list;unsigned bytes,vertices,format,misses;} CompiledAdmission;
static CompiledAdmission compiled_admission[128];
unsigned fh_compiled_admission_skips,fh_compiled_admission_reprobes;
unsigned fh_compiled_native_mode,fh_compiled_native_vertices;
unsigned fh_compiled_cpu_packed_vertices;
static CompiledAdmission *compiled_admission_for(const FhCompiledMesh *mesh){
    const void *identity=fh_source_identity(compiled_list,compiled_bytes);
    uintptr_t p=(uintptr_t)identity;
    CompiledAdmission *e=compiled_admission+((p>>4 ^ p>>13)&127);
    if(e->list!=identity || e->bytes!=compiled_bytes || e->vertices!=mesh->unique || e->format!=format)
        *e=(CompiledAdmission){identity,compiled_bytes,mesh->unique,format,0};
    return e;
}
#include "gx_compiled_compact.h"
#include "gx_vertex_program.h"
/* Mutable CPU lighting/emboss cannot use the immutable vertex cache. Decode
 * into one stack vertex, then pack straight into the fenced PICA stream instead
 * of writing and rereading the full 148-byte-per-vertex staging array. */
static int decode_cpu_native(FhNativeVertex *output,const FhCompiledMesh *mesh,const FhNativePacking *pack){
    FhVertexProgram program;if(!vertex_program_prepare(&program,pack))return 0;
    for(unsigned i=0;i<mesh->unique;i++)
        if(!vertex_program_execute(output+i,mesh->packets+mesh->offsets[i],&program,pack))return 0;
    /* Publish accounting only after the entire packet succeeds. In checked
     * mode the original decoder/sink below owns accounting and submission. */
    if(fh_compiled_native_mode!=2){
        received+=mesh->unique;decoded_vertices+=mesh->unique;
        for(unsigned i=0;i<mesh->unique;i++)if(!++vertex_identity)++vertex_identity;
    }
    fh_retained_end();fh_compiled_cpu_packed_vertices+=mesh->unique;
    fh_compiled_native_vertices+=mesh->unique;fh_compiled_decode_misses++;
    return fh_compiled_native_mode==2?2:1;
}
static int decode_compiled_native(FhNativeVertex *output,const FhCompiledMesh *mesh,const FhNativePacking *packing){
    if(!fh_compiled_native_mode || fh_retained_disabled || mesh->unique>FH_COMPILED_RETAINED_UNIQUE)return 0;
    if(fh_gx.draw.raw_position==1 && !gpu_lighting){
        if(!output)return 1;
        return decode_cpu_native(output,mesh,packing);
    }
    CompiledAdmission *admission=compiled_admission_for(mesh);
    if(admission->misses<2 || !(admission->misses&31u))return 0;
    FastCompiled fast;if(!prepare_fast_compiled(&fast))return 0;
    if(!output)return 1;
    FhNativeVertex base;if(!prepare_native_compiled(&base,&fast,packing))return 0;
    for(unsigned i=0;i<mesh->unique;i++)
        if(!decode_native_compiled(output+i,mesh->packets+mesh->offsets[i],&fast,packing,&base))return 0;
    /* Only a complete packet can change admission/accounting. Failed attempts
     * leave an unpublished stream span for the original decoder to overwrite. */
    admission->misses++;fh_retained_end();fh_compiled_admission_skips++;
    fh_compiled_native_vertices+=mesh->unique;fh_compiled_decode_misses++;
    if(fh_compiled_native_mode!=2)for(unsigned i=0;i<mesh->unique;i++){
        decoded_vertices++;received++;if(!++vertex_identity)++vertex_identity;
    }
    return fh_compiled_native_mode==2?2:1;
}
static int draw_compiled_native(const FhCompiledMesh *mesh);
static void finish_compiled_post(FhVertex *output,const FhCompiledMesh *mesh,const float post[3][4]){
    if(!post)return;
    for(unsigned i=0;i<mesh->unique;i++){
        float result[3];transform(result,post,output[i].uv[1],1);fh_copy_bytes(output[i].uv[1],result,12);
        fh_fur_post_vertices++;
        if(fh_fur_post_mode==2){
            FhVertex reference;unsigned saved_received=received,saved_decoded=decoded_vertices,saved_id=vertex_identity;
            decode_compiled(&reference,mesh->packets+mesh->offsets[i]);
            fh_gx_require(!fh_memory_compare(&reference,output+i,sizeof(reference)),"Fur template differs from scalar decoder");
            received=saved_received;decoded_vertices=saved_decoded;vertex_identity=saved_id;fh_fur_post_checks++;
        }
    }
}
static uint64_t decode_compiled_batch(FhVertex *output,const FhCompiledMesh *mesh){
    /* Melee separates live palettes/texgen uniforms from owned attributes.
     * Reuse indexed ordinary/fur packets only when CPU lighting and eye-space
     * coordinates have no hidden palette dependency. The existing arena checks
     * all list/array bytes and every remaining baked shading input. */
    unsigned retain=fh_gx.draw.raw_position && !(fh_gx.draw.raw_position&8) &&
        (gpu_lighting?gpu_lighting_palette!=1:!lighting_needs_normal) &&
        !texgen_needs_eye && mesh->unique<=FH_COMPILED_RETAINED_UNIQUE && !fh_retained_disabled;
    CompiledAdmission *admission=retain?compiled_admission_for(mesh):NULL;
    const float (*post)[4]=fh_compiled_fur_post();
    if(!output){
#ifdef FH_EMULATOR_TEST
        extern unsigned fh_compiled_profile[12];
        fh_compiled_profile[0]+=!!retain;
        fh_compiled_profile[1]+=gpu_lighting && gpu_lighting_palette==1;
        fh_compiled_profile[2]+=!!texgen_needs_eye;
        fh_compiled_profile[3]+=mesh->unique>FH_COMPILED_RETAINED_UNIQUE;
#endif
        uint64_t id=retain?fh_retained_probe_compiled(compiled_list,compiled_bytes,mesh->unique,format,raster_channels):0;
        if(id)admission->misses=0;
        return id;
    }
    if(admission && admission->misses>=2){
        if(admission->misses&31u){retain=0;fh_retained_end();fh_compiled_admission_skips++;}
        else fh_compiled_admission_reprobes++;
    }
    if(retain && fh_retained_read_compiled(compiled_list,compiled_bytes,mesh->unique,format,raster_channels,output)){
#ifdef FH_EMULATOR_TEST
        extern unsigned fh_compiled_profile[12];fh_compiled_profile[4]++;
#endif
        admission->misses=0;
        finish_compiled_post(output,mesh,post);
        fh_compiled_decode_hits++;fh_compiled_decode_vertices+=mesh->unique;return fh_retained_compiled_identity();
    }
    if(admission)admission->misses++;
#ifdef FH_EMULATOR_TEST
    {extern unsigned fh_compiled_profile[12];fh_compiled_profile[5]++;}
#endif
    fh_compiled_decode_misses++;
    FastCompiled fast;unsigned specialized=prepare_fast_compiled(&fast);
    compiled_post_template=post!=NULL;
    for(unsigned i=0;i<mesh->unique;i++){
        const u8 *packet=mesh->packets+mesh->offsets[i];
        if(specialized && decode_fast_compiled(output+i,packet,&fast)){
#ifdef FH_EMULATOR_TEST
            if(fh_compiled_fast_mode==2){
                FhVertex reference;unsigned saved_received=received,saved_decoded=decoded_vertices,saved_id=vertex_identity;
                decode_compiled(&reference,packet);
                fh_gx_require(!fh_memory_compare(&reference,output+i,sizeof(reference)),"Specialized vertex differs from scalar decoder");
                received=saved_received;decoded_vertices=saved_decoded;vertex_identity=saved_id;fh_compiled_fast_checks++;
            }
#endif
            decoded_vertices++;received++;if(!++vertex_identity)++vertex_identity;fh_compiled_fast_vertices++;
        }else decode_compiled(output+i,packet);
    }
    compiled_post_template=0;
    if(retain){
        uint32_t ids[BATCH];
        for(unsigned first=0;first<mesh->unique;first+=BATCH){
            unsigned n=mesh->unique-first;if(n>BATCH)n=BATCH;
            for(unsigned i=0;i<n;i++)ids[i]=first+i+1;
            fh_retained_record(output+first,n,ids);
        }
        fh_retained_end();
    }
    finish_compiled_post(output,mesh,post);
    return retain?fh_retained_compiled_identity():0;
}
static int draw_compiled_native(const FhCompiledMesh *mesh){
    FhCompiledMesh current=*mesh;current.decode_native=decode_compiled_native;
    current.decode_compact=decode_compiled_compact;
    current.fur_post=fh_compiled_fur_post();
    return fh_renderer_draw_compiled(&current,&fh_gx.draw,decode_compiled,decode_compiled_batch);
}
static void record(const void *bytes,unsigned count) {
    fh_gx_require(count<=record_capacity-record_size,"GX display-list recording overflow");fh_copy_bytes(recording+record_size,bytes,count);record_size+=count;
}
static unsigned prepare_sphere_lighting(void){
    if(fh_gpu_sphere_disabled || raster_channels!=3 || !fh_gx.draw.raw_position ||
       fh_gx.descriptors[GX_VA_PNMTXIDX] || fh_gx_active_texgens!=1 ||
       fh_gx.texgens[0].source!=GX_TG_NRM || fh_gx.texgens[0].type!=GX_TG_MTX2x4 ||
       fh_gx.texgens[0].normalize || fh_gx.texgens[0].postmatrix!=GX_PTIDENTITY)return 0;
    FhGpuLighting data={0};
    fh_copy_bytes(data.normal,fh_gx.normal[fh_gx.current_matrix/3],sizeof(data.normal));
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++){
        float value=fh_gx.draw.model_palette[r][c];
        if(!finite_vertex_float(value) || fabsf(value)>(c==3?1e5f:16.f))return 0;
    }
    for(unsigned channel=0;channel<2;channel++){
        unsigned slot=channel*3;
        for(unsigned part=0;part<2;part++){
            const FhChannel *ch=&fh_gx.channel[channel+part*2];
            if(!ch->enabled || ch->ambient_source!=GX_SRC_REG || ch->material_source!=GX_SRC_REG ||
               ch->mask>255 || __builtin_popcount(ch->mask)!=(part?1:2) ||
               ch->attenuation!=(part?GX_AF_SPOT:GX_AF_SPEC) || ch->diffuse!=(part?GX_DF_CLAMP:GX_DF_NONE))return 0;
            for(unsigned i=0;i<8;i++)if(ch->mask&(1u<<i)){
                const FhLight *src=fh_gx.lights+i;float (*dst)[4]=data.lights[slot++];
                fh_copy_bytes(dst[0],src->position,12);fh_copy_bytes(dst[1],src->direction,12);
                fh_copy_bytes(dst[2],src->color,16);fh_copy_bytes(dst[3],src->a,12);fh_copy_bytes(dst[4],src->k,12);
            }
        }
        fh_copy_bytes(data.lights[6][channel],fh_gx.ambient[channel],16);
        fh_copy_bytes(data.lights[6][channel+2],fh_gx.material[channel],16);
    }
    if(!fh_gpu_sphere_validate(&data) || !fh_renderer_prepare_lighting(&fh_gx.draw,&data))return 0;
    fh_gx.draw.raw_position|=64;return 1;
}
static unsigned prepare_lighting(void) {
    if(prepare_sphere_lighting())return 1;
    const FhChannel *ch=&fh_gx.channel[0];
    const FhTexGen *g0=&fh_gx.texgens[0],*g1=&fh_gx.texgens[1];
    /* Melee keeps normal-derived coordinates independent of lighting. An
     * unlit five-coordinate surface still needs its live normal palette:
     * baking that palette into UV0 prevents residence of the entire mesh.
     * Use the same zero-light PICA program for the surface family. The later
     * texgen proof still checks palette equality, coordinate dependencies and
     * projective bounds; no texture sample or shell is removed here. */
    unsigned unlit_surface=fh_gx_active_texgens==5 &&
        fh_renderer_fur_texgen_available(&fh_gx.draw);
    unsigned unlit_pair=fh_gx_active_texgens==2 &&
        !fh_gx.descriptors[GX_VA_TEX1MTXIDX] &&
        g1->type==GX_TG_MTX2x4 && g1->source>=GX_TG_TEX0 && g1->source<=GX_TG_TEX7 &&
        g1->matrix==GX_IDENTITY && !g1->normalize && g1->postmatrix==GX_PTIDENTITY;
    unsigned unlit_normal=!ch->enabled && raster_channels<=1 &&
        (unlit_pair || unlit_surface) && fh_gx.descriptors[GX_VA_PNMTXIDX] &&
        fh_gx.descriptors[GX_VA_TEX0MTXIDX] &&
        g0->type==GX_TG_MTX2x4 && g0->source==GX_TG_NRM && !g0->normalize;
    const FhChannel *alpha=&fh_gx.channel[2];
    if(list_batching && fh_gx.draw.raw_position && raster_channels==1 &&
       (!alpha->enabled || !alpha->mask || (alpha->diffuse==GX_DF_NONE && alpha->attenuation==GX_AF_NONE)) &&
       (!ch->enabled || (ch->ambient_source==GX_SRC_REG && (!ch->mask || (ch->diffuse==GX_DF_NONE && ch->attenuation==GX_AF_NONE))))) {
        unsigned dependent_texgen=0;
        for(unsigned i=0;i<fh_gx_active_texgens;i++){
            if(!(fh_gx_texgen_inputs&(1u<<i)))continue;
            const FhTexGen *g=&fh_gx.texgens[i];
            /* Position-derived reflection coordinates do not consume raster
             * lighting. They retain their ordinary CPU matrix/post evaluation
             * and exact key dependencies while the flat color stays live.
             * Normal/color/emboss dependencies retain the existing path. */
            dependent_texgen|=(g->source!=GX_TG_POS && (g->source<GX_TG_TEX0 || g->source>GX_TG_TEX7)) ||
                (g->type>=GX_TG_BUMP0 && g->type<=GX_TG_BUMP7);
        }
        if(!dependent_texgen){
            /* Flat RGB still used to bake live ambient/material/light colors
             * into every cached vertex. Melee's unlit shader path avoids all
             * normal/light work. Our flat branch uses four existing uniform
             * rows; eligible flat alpha shares the ambient W lane. */
            FhGpuLighting data={0};data.mask=FH_LIGHT_LIVE_MATERIAL|FH_LIGHT_FLAT;
            for(unsigned c=0;c<3;c++){
                float sum=ch->enabled?fh_gx.ambient[0][c]:1.f;
                if(ch->enabled)for(unsigned i=0;i<8;i++)if(ch->mask&(1u<<i))sum+=fh_gx.lights[i].color[c];
                data.ambient[c]=clamp(sum);data.normal[c][c]=1;
                data.normal[c][3]=ch->material_source==GX_SRC_REG?fh_gx.material[0][c]:1.f;
                if(!finite_vertex_float(sum) || !finite_vertex_float(data.normal[c][3]) || fabsf(data.normal[c][3])>4)return 0;
            }
            /* Independent flat alpha follows the same ordered GX sum. Keep
             * vertex ambient alpha on the scalar route: its clamp is per
             * vertex, and cannot be replaced by a uniform multiplier. */
            if(!alpha->enabled || alpha->ambient_source==GX_SRC_REG){
                float sum=alpha->enabled?fh_gx.ambient[0][3]:1.f;
                if(alpha->enabled)for(unsigned i=0;i<8;i++)if(alpha->mask&(1u<<i))sum+=fh_gx.lights[i].color[3];
                float material=alpha->material_source==GX_SRC_REG?fh_gx.material[0][3]:1.f;
                if(!finite_vertex_float(sum) || !finite_vertex_float(material) || fabsf(material)>4)return 0;
                data.ambient[3]=material*clamp(sum);data.mask|=FH_LIGHT_LIVE_ALPHA;
            }else data.ambient[3]=1.f;
            data.lights[0][3][3]=1;
            gpu_lighting_palette=0;gpu_lighting_flat=fh_renderer_prepare_lighting(&fh_gx.draw,&data);
            if(gpu_lighting_flat && (data.mask&FH_LIGHT_LIVE_ALPHA))fh_gx.draw.raw_position|=32;
            return gpu_lighting_flat;
        }
    }
    if(!fh_gx.draw.raw_position || fh_gx.draw.texgens>=8 || fh_gx.channel[2].enabled ||
       (!unlit_normal && (raster_channels!=1 || !ch->enabled || !ch->mask || ch->ambient_source!=GX_SRC_REG ||
        ch->diffuse!=GX_DF_CLAMP || ch->attenuation!=GX_AF_SPOT)))return 0;
    for(unsigned i=0;i<fh_gx_active_texgens;i++)if((fh_gx_texgen_inputs&(1u<<i)) && (fh_gx.texgens[i].source==GX_TG_COLOR0 || fh_gx.texgens[i].source==GX_TG_COLOR1))return 0;
    unsigned light_mask=unlit_normal?0:ch->mask;
    FhGpuLighting data={0};data.mask=light_mask;
    unsigned palette=fh_gx.descriptors[GX_VA_PNMTXIDX]!=0;
    if(palette){
        for(unsigned r=0;r<3;r++)data.normal[r][r]=1;
        for(unsigned m=0;m<10;m++)if(lighting_palette_mask&(1u<<m))for(unsigned r=0;r<3;r++)for(unsigned c=0;c<3;c++)
            if(!finite_vertex_float(fh_gx.normal[m][r][c]) || fabsf(fh_gx.normal[m][r][c])>16.f)return 0;
        unsigned shared=1;
        for(unsigned m=0;m<10 && shared;m++)if(lighting_palette_mask&(1u<<m))for(unsigned r=0;r<3;r++) {
            const float *a=fh_gx.normal[m][r],*b=fh_gx.position[m][r];
            for(unsigned c=0;c<3;c++)if(a[c]!=b[c])shared=0;
            if(a[3]!=0)shared=0;
            /* Gershgorin bounds on A*A^T: eigenvalues > .23, row lengths
             * < 2. Raw normals in [4e-6,16] therefore cannot enter the
             * former tiny/extreme transformed-normal fallback region. */
            float squared=dot(a,a);if(!(squared>.25f && squared<4.f))shared=0;
            for(unsigned k=0;k<r;k++)if(fabsf(dot(a,fh_gx.normal[m][k]))>=.01f)shared=0;
        }
        if(shared)data.mask|=FH_LIGHT_MODEL_NORMAL;
    }else fh_copy_bytes(data.normal,fh_gx.normal[fh_gx.current_matrix/3],sizeof(data.normal));
    fh_copy_bytes(data.ambient,fh_gx.ambient[0],sizeof(data.ambient));
    if(unlit_normal)for(unsigned c=0;c<4;c++)data.ambient[c]=1;
    for(unsigned i=0;i<8;i++)if(light_mask&(1u<<i)) {
        const FhLight *l=&fh_gx.lights[i];float (*dst)[4]=data.lights[i];
        fh_copy_bytes(dst[0],l->position,12);fh_copy_bytes(dst[1],l->direction,12);fh_copy_bytes(dst[2],l->color,16);
        fh_copy_bytes(dst[3],l->a,12);fh_copy_bytes(dst[4],l->k,12);
        /* Large directional-light sentinels exceed PICA float24 range when
         * squared. Retain CPU semantics instead of approximating their range. */
        for(unsigned j=0;j<5;j++)for(unsigned c=0;c<4;c++)if(!finite_vertex_float(dst[j][c]) || fabsf(dst[j][c])>(j==0?1e7f:100.f))return 0;
    }
    for(unsigned row=0;row<3;row++)for(unsigned c=0;c<4;c++)
        if(!finite_vertex_float(data.normal[row][c]) || fabsf(data.normal[row][c])>16.f)return 0;
    for(unsigned row=0;row<(palette?30:3);row++)if(!palette || (lighting_palette_mask&(1u<<(row/3))))for(unsigned c=0;c<4;c++) {
        float value=fh_gx.draw.model_palette[row][c];
        if(!finite_vertex_float(value) || fabsf(value)>(c==3?1e7f:64.f))return 0;
    }
    unsigned bounded_normal_palette=0;
    if(palette && !(data.mask&FH_LIGHT_MODEL_NORMAL) && (__builtin_popcount(light_mask&255)<=2 || fh_light_lod_enabled)){
        unsigned bounded=1;
        for(unsigned m=0;m<10 && bounded;m++)if(lighting_palette_mask&(1u<<m)){
            float magnitude=0;for(unsigned r=0;r<3;r++)for(unsigned c=0;c<3;c++)magnitude+=fabsf(fh_gx.normal[m][r][c]);
            if(!magnitude)continue; /* An all-zero normal stays exactly zero. */
            for(unsigned r=0;r<3;r++){
                const float *a=fh_gx.normal[m][r];float diagonal=dot(a,a),radius=0;
                for(unsigned k=0;k<3;k++)if(k!=r)radius+=fabsf(dot(a,fh_gx.normal[m][k]));
                if(!(diagonal-radius>.0625f && diagonal+radius<16.f))bounded=0;
            }
        }
        bounded_normal_palette=bounded;
    }
    unsigned reduced=0,original_lights=__builtin_popcount(light_mask&255);
    if(fh_light_lod_enabled && palette && (bounded_normal_palette || (data.mask&FH_LIGHT_MODEL_NORMAL)) && original_lights>2){
        float center[3]={0};unsigned matrices=0;
        for(unsigned m=0;m<10;m++)if(lighting_palette_mask&(1u<<m)){
            for(unsigned c=0;c<3;c++)center[c]+=fh_gx.draw.model_palette[m*3+c][3];
            matrices++;
        }
        if(matrices){
            for(unsigned c=0;c<3;c++)center[c]/=matrices;
            reduced=fh_light_lod_reduce(&data,center);
            if(reduced)light_mask=data.mask&255;
        }
    }
    if(bounded_normal_palette && __builtin_popcount(light_mask&255)<=2){
            unsigned active=0;
            for(unsigned i=0;i<8;i++)if(light_mask&(1u<<i)){
                if(i!=active)fh_copy_bytes(data.lights[active],data.lights[i],sizeof(data.lights[0]));
                active++;
            }
            data.mask=(data.mask&~255u)|((1u<<active)-1u);
            fh_copy_bytes(data.lights[2],fh_gx.normal,30*16);
            data.mask|=FH_LIGHT_NORMAL_PALETTE;
    }
    /* Unused normal W lanes carry live RGB material; a spare light lane marks
     * the convention without changing the capture ABI or uniform budget. */
    data.mask|=FH_LIGHT_LIVE_MATERIAL;data.lights[0][3][3]=1;
    for(unsigned c=0;c<3;c++)data.normal[c][3]=ch->material_source==GX_SRC_REG?fh_gx.material[0][c]:1.f;
    unsigned accepted=fh_renderer_prepare_lighting(&fh_gx.draw,&data);
    if(accepted && reduced){fh_light_lod_draws++;fh_light_lod_source_lights+=original_lights;fh_light_lod_palette_draws+=!!(data.mask&FH_LIGHT_NORMAL_PALETTE);}
    gpu_lighting_palette=accepted && palette?((data.mask&FH_LIGHT_MODEL_NORMAL)?2:(data.mask&FH_LIGHT_NORMAL_PALETTE)?3:1):0;return accepted;
}
static void prepare_vertex_shading(void) {
#if defined(FH_EMULATOR_TEST) || defined(__arm__)
    uint64_t shade_start=PROFILE_ENABLED?PROFILE_TICKS():0,shade_phase;
#endif
    fh_gx_active_texgens=fh_gx.draw.texgens;
    /* A compiled untextured GPU material has no coordinate consumers. Keep
     * the GX register state unchanged for subsequent draws, but do not bake
     * unused animated matrices into vertex snapshots. */
    if(list_batching && fh_gx_active_texgens && fh_renderer_texgens_unused(&fh_gx.draw))fh_gx_active_texgens=0;
    fh_gx_texgen_inputs=(1u<<fh_gx_active_texgens)-1u;
    if(list_batching)fh_gx_texgen_inputs&=fh_renderer_texgen_inputs(&fh_gx.draw);
    /* A consumed generated coordinate can read earlier generators. Forward
     * references read the scalar decoder's initial zero and add no producer. */
    for(int i=(int)fh_gx_active_texgens-1;i>=0;i--)if(fh_gx_texgen_inputs&(1u<<i)){
        unsigned source=fh_gx.texgens[i].source;
        if(source>=GX_TG_TEXCOORD0 && source<=GX_TG_TEXCOORD6 && source-GX_TG_TEXCOORD0<(unsigned)i)
            fh_gx_texgen_inputs|=1u<<(source-GX_TG_TEXCOORD0);
    }
    /* Conservative dependency scan, as in pinned Melee GXBegin. Keep every
     * referenced channel, including stages whose results may later be unused.
     * Color-driven texgens consume raster colors independently of TEV. */
    raster_channels=0;
    for(unsigned i=0;i<fh_gx.draw.stages;i++) {
        const FhTevStage *t=&fh_gx.draw.tev[i];
        if(t->channel>=6)continue;
        for(unsigned j=0;j<4;j++)if(t->color[j]==GX_CC_RASC || t->color[j]==GX_CC_RASA || t->alpha[j]==GX_CA_RASA)
            raster_channels|=1u<<(t->channel&1);
    }
    for(unsigned i=0;i<fh_gx_active_texgens;i++) {
        if(!(fh_gx_texgen_inputs&(1u<<i)))continue;
        unsigned source=fh_gx.texgens[i].source;
        if(source==GX_TG_COLOR0 || source==GX_TG_COLOR1)raster_channels|=1u<<(source-GX_TG_COLOR0);
    }
    gpu_lighting=gpu_lighting_palette=gpu_lighting_flat=0;
    lighting_needs_normal=constant_lighting=0;
    for(unsigned i=0;i<4;i++) {
        if(!(raster_channels&(1u<<(i&1))))continue;
        const FhChannel *state=&fh_gx.channel[i];
        unsigned geometric=state->enabled && state->mask &&
            (state->diffuse!=GX_DF_NONE || state->attenuation!=GX_AF_NONE);
        lighting_needs_normal|=geometric;
        /* Register-only flat channels are identical at every vertex. Keep
         * their original ordered additions, clamp, and final multiplication.
         * Vertex material/ambient sources still use per-vertex evaluation. */
        if(!geometric && state->material_source==GX_SRC_REG &&
            (!state->enabled || state->ambient_source==GX_SRC_REG)) {
            unsigned channel=i&1,first=i>=2?3:0,end=i>=2?4:3;
            constant_lighting|=1u<<i;
            for(unsigned j=first;j<end;j++) {
                float value=fh_gx.material[channel][j];
                if(state->enabled) {
                    float sum=fh_gx.ambient[channel][j];
                    for(unsigned light=0;light<8;light++)if(state->mask&(1u<<light))sum+=fh_gx.lights[light].color[j];
                    value*=clamp(sum);
                }
                constant_colors[channel][j]=value;
            }
        }
    }
    texgen_needs_eye=0;
    for(unsigned i=0;i<fh_gx_active_texgens;i++)if(fh_gx_texgen_inputs&(1u<<i))texgen_needs_eye|=fh_gx.texgens[i].type>=GX_TG_BUMP0 && fh_gx.texgens[i].type<=GX_TG_BUMP7;
    for(unsigned i=0;i<fh_gx_active_texgens;i++) {
        const FhTexGen *g=&fh_gx.texgens[i];PreparedTexGen *p=&prepared_texgen[i];
        *p=(PreparedTexGen){.dynamic=fh_gx.descriptors[GX_VA_TEX0MTXIDX+i]!=GX_NONE,.alias=i};
        if(!(fh_gx_texgen_inputs&(1u<<i)))continue;
        if(g->type>=GX_TG_BUMP0 && g->type<=GX_TG_BUMP7)continue;
        if(!p->dynamic && g->matrix!=GX_IDENTITY && g->type!=GX_TG_SRTG) {
            unsigned index=g->matrix;fh_gx_require(index%3==0 && index/3<64,"Invalid vertex texture matrix");
            p->matrix=index<GX_TEXMTX0?fh_gx.position[index/3]:fh_gx.texture[index/3];
        }
        if(g->postmatrix!=GX_PTIDENTITY) {
            unsigned index=g->postmatrix;
            fh_gx_require(index>=GX_PTTEXMTX0 && (index-GX_PTTEXMTX0)%3==0 && (index-GX_PTTEXMTX0)/3<64,"Invalid post texgen matrix");
            p->post=fh_gx.post[(index-GX_PTTEXMTX0)/3];
        }
        for(unsigned j=0;j<i;j++) {
            if(!(fh_gx_texgen_inputs&(1u<<j)))continue;
            /* Self/forward generated-coordinate reads can change between
             * stages. Raw attributes and already-generated sources cannot. */
            if(g->source>=GX_TG_TEXCOORD0 && g->source<=GX_TG_TEXCOORD6 && g->source-GX_TG_TEXCOORD0>=j)continue;
            if(!fh_memory_compare(g,&fh_gx.texgens[j],sizeof(*g))){p->alias=j;break;}
        }
        /* Repeated shell UVs survive changing position arrays and materials.
         * Dynamic per-vertex matrices and emboss retain ordinary evaluation.
         * Cheap single-generator draws avoid the probe overhead. */
        if(!p->dynamic && p->alias==i && (g->normalize || fh_gx_active_texgens>=4)){
            if(fh_texgen_lazy_mode){p->pending=1;fh_texgen_lazy_planned++;}
            else p->cache_id=texgen_state(p->matrix,p->post,g->type==GX_TG_MTX2x4 || g->type==GX_TG_SRTG,g->normalize);
        }
    }
    /* Visit order matches illuminate: color0, alpha0, color1, alpha1. */
    static const unsigned channel_order[]={0,2,1,3};
    for(unsigned n=0;n<4;n++){
        unsigned i=channel_order[n];lighting_group[i]=i;
        const FhChannel *a=&fh_gx.channel[i];
        if(!a->enabled)continue;
        for(unsigned m=0;m<n;m++){
            unsigned j=channel_order[m];const FhChannel *b=&fh_gx.channel[j];
            if((raster_channels&(1u<<(j&1))) && b->enabled && a->mask==b->mask && a->diffuse==b->diffuse && a->attenuation==b->attenuation){lighting_group[i]=lighting_group[j];break;}
        }
    }
#if defined(FH_EMULATOR_TEST) || defined(__arm__)
    shade_phase=PROFILE_ENABLED?PROFILE_TICKS():0;
#endif
    if((gpu_lighting=prepare_lighting())){fh_gx.draw.raw_position|=2;lighting_needs_normal=0;}
#if defined(FH_EMULATOR_TEST) || defined(__arm__)
    if(PROFILE_ENABLED)fh_gx_shading_ticks[1]+=PROFILE_TICKS()-shade_phase;
    shade_phase=PROFILE_ENABLED?PROFILE_TICKS():0;
#endif
    cpu_light_state_id=0;
    if(raster_channels==3 && lighting_needs_normal && !gpu_lighting){
        FhCpuLightState key={.raster=raster_channels};
        fh_copy_bytes(key.channels,fh_gx.channel,sizeof(key.channels));fh_copy_bytes(key.lights,fh_gx.lights,sizeof(key.lights));
        fh_copy_bytes(key.ambient,fh_gx.ambient,sizeof(key.ambient));fh_copy_bytes(key.material,fh_gx.material,sizeof(key.material));
#ifdef __arm__
        __asm__ volatile("vmrs %0, fpscr":"=r"(key.fpscr));key.fpscr&=0x03c00000u;
#endif
        cpu_light_state_id=fh_cpu_light_state(&key);
    }
    gpu_texgen=0;
    /* Recognize the opening's normal-map generator, not a game-specific draw
     * address. All palette entries must agree before any work is deferred.
     * Generated-coordinate dependencies and other materials stay scalar. */
    const FhTexGen *g0=&fh_gx.texgens[0],*g1=&fh_gx.texgens[1];
    unsigned fur_texgen=fh_gx.draw.texgens==5 && fh_renderer_fur_texgen_available(&fh_gx.draw);
    unsigned simple_texgen=fh_gx.draw.texgens==2 &&
       g1->source>=GX_TG_TEX0 && g1->source<=GX_TG_TEX7 && g1->matrix==GX_IDENTITY &&
       !g1->normalize && g1->postmatrix==GX_PTIDENTITY && !fh_gx.descriptors[GX_VA_TEX1MTXIDX];
    if(gpu_lighting_palette>=2 && (simple_texgen || fur_texgen) && fh_gx.descriptors[GX_VA_TEX0MTXIDX] &&
       g0->type==GX_TG_MTX2x4 && g0->source==GX_TG_NRM && !g0->normalize &&
       !texgen_needs_eye) {
        unsigned shared=1;
        for(unsigned i=1;i<fh_gx_active_texgens;i++)if((fh_gx_texgen_inputs&(1u<<i)) && fh_gx.texgens[i].source>=GX_TG_TEXCOORD0 &&
            fh_gx.texgens[i].source<=GX_TG_TEXCOORD6)shared=0;
        for(unsigned m=0;m<10;m++)if((lighting_palette_mask&(1u<<m)) && fh_memory_compare(fh_gx.texture[10+m],fh_gx.normal[m],48))shared=0;
        float identity[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
        const float (*post)[4]=prepared_texgen[0].post?prepared_texgen[0].post:identity;
        for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)if(!finite_vertex_float(post[r][c]) || fabsf(post[r][c])>64)shared=0;
        if(post[2][0]!=0 || post[2][1]!=0 || post[2][2]!=0 || post[2][3]!=1)shared=0;
        if(shared && fh_renderer_prepare_texgen(&fh_gx.draw,post)){gpu_texgen=1;fh_gx.draw.raw_position|=4;}
    }
    /* Pinned Melee gx.c keeps animated TObj matrices out of immutable
     * geometry. Start with one ordinary 2x4 generator on triangle lists;
     * all sampled units must use that coordinate and no CPU fallback. */
    if(list_batching && (fh_gx.draw.raw_position==1 || gpu_lighting_flat) && !lighting_needs_normal &&
       fh_gx.draw.texgens==1 && g0->type==GX_TG_MTX2x4 &&
       g0->source>=GX_TG_TEX0 && g0->source<=GX_TG_TEX7 && !g0->normalize &&
       !fh_gx.descriptors[GX_VA_TEX0MTXIDX]){
        const PreparedTexGen *p=prepared_texgen;
        unsigned post_only=g0->matrix==GX_IDENTITY && p->post;
        float matrix[3][4];
        if(!post_only || fh_post_uvgen_enabled){
            if(post_only){fh_post_uvgen_attempts++;memcpy(fh_post_uvgen_last,p->post,sizeof(fh_post_uvgen_last));}
            if(!fh_uvgen_matrix(matrix,p->matrix,p->post))fh_post_uvgen_matrix_rejections+=post_only;
            else if(!fh_renderer_prepare_uvgen(&fh_gx.draw,matrix))fh_post_uvgen_material_rejections+=post_only;
            else {fh_gx.draw.raw_position|=20;fh_post_uvgen_draws+=post_only;}
        }
    }
#if defined(FH_EMULATOR_TEST) || defined(__arm__)
    if(PROFILE_ENABLED){
    uint64_t shade_end=PROFILE_TICKS();
    fh_gx_shading_ticks[2]+=shade_end-shade_phase;
    fh_gx_shading_ticks[0]+=shade_end-shade_start;
    }
#endif
}
static void begin_primitive(GXPrimitive kind,GXVtxFmt vertex_format,u16 vertices,unsigned defer_shading) {
    if(fh_render_floor && !recording){fh_render_floor_primitives++;return;}
    if(kind!=GX_QUADS && kind!=GX_TRIANGLES && kind!=GX_TRIANGLESTRIP && kind!=GX_TRIANGLEFAN)
        fh_renderer_material_scope_invalidate();
    if(recording) { fh_gx_require(vertices!=GX_AUTO,"Automatic count in recorded display list");u8 header[]={(u8)(kind|vertex_format),(u8)(vertices>>8),(u8)vertices};record(header,3);return; }
    fh_gx_end_primitive();fh_gx_require((unsigned)vertex_format<8,"Invalid primitive vertex format");
    primitive=kind;format=vertex_format;expected=vertices;received=pending_bytes=0;stride=field_count=0;
    lighting_palette_mask=0x3ffu;
    gpu_points=kind==GX_POINTS && fh_renderer_points_available(&fh_gx.draw);
    fh_gx.draw.raw_position=(kind!=GX_POINTS || gpu_points) && kind!=GX_LINES && kind!=GX_LINESTRIP && fh_renderer_transform_available();
    if(gpu_points)fh_gx.draw.raw_position|=8; /* Immutable centers, live GS expansion. */
    if(fh_gx.draw.raw_position){
        unsigned matrix=fh_gx.current_matrix;
        fh_gx_require(matrix%3==0 && matrix/3<64,"Invalid GPU transform matrix");
        fh_copy_bytes(fh_gx.draw.position_matrix,fh_gx.projection,sizeof(fh_gx.projection));
        if(fh_gx.descriptors[GX_VA_PNMTXIDX])fh_copy_bytes(fh_gx.draw.model_palette,fh_gx.position,sizeof(fh_gx.draw.model_palette));
        else {memset(fh_gx.draw.model_palette,0,sizeof(fh_gx.draw.model_palette));fh_copy_bytes(fh_gx.draw.model_palette,fh_gx.position[matrix/3],sizeof(fh_gx.position[0]));}
    }
    position_field=NULL;position_matrix_offset=~0u;
    for(unsigned attr=0;attr<=GX_VA_TEX7;attr++) {
        unsigned mode=fh_gx.descriptors[attr];if(!mode) continue;
        FhVtxFormat f=fh_gx.formats[format][attr];
        unsigned size=attribute_bytes(attr,f);
        PreparedField *prepared=&fields[field_count++];
        *prepared=(PreparedField){.attr=attr,.mode=mode,.bytes=size,.format=f,.offset=stride};
        if(attr==GX_VA_POS)position_field=prepared;
        if(attr==GX_VA_PNMTXIDX)position_matrix_offset=stride;
        if(attr>=GX_VA_POS && attr!=GX_VA_CLR0 && attr!=GX_VA_CLR1) {
            prepared->components=count(attr,f);prepared->component_bytes=component_bytes(f.type);
            prepared->scale=attr==GX_VA_NRM?(f.type==GX_S8?1.f/64:f.type==GX_S16?1.f/16384:1):ldexpf(1.f,-(int)f.fraction);
        }
        unsigned bytes=mode==GX_DIRECT?size:mode==GX_INDEX8?1:2;
        if(attr==GX_VA_NRM && mode!=GX_DIRECT && f.count==GX_NRM_NBT3) bytes*=3;
        stride+=bytes;
    }
    fh_gx_require(stride>0 && stride<=VERTEX_BYTES,"Invalid primitive vertex stride");
    if(!defer_shading)prepare_vertex_shading();
}
void GXBegin(GXPrimitive kind,GXVtxFmt vertex_format,u16 vertices) {begin_primitive(kind,vertex_format,vertices,0);}

#include "fh_native_model_draw.h"
unsigned fh_native_model_admission[16];
#include "gx_native_scene_state.h"
#include "fh_native_sky.h"
int fh_gx_sky_begin(const void *const *lists,const uint16_t *sizes,unsigned count){
    extern unsigned fh_surface_detail;
    static unsigned reported;
#ifdef FH_GX_DISPATCH_BUILD
    extern void fh_gx_dispatch_barrier(void);fh_gx_dispatch_barrier();
#endif
    if(!reported++ && fh_native_scene_enabled){char line[160];snprintf(line,sizeof(line),"Native sky input count=%u matrix=%u format=%u/%u/%u first=%02x bytes=%u\n",count,fh_gx.current_matrix,fh_gx.formats[0][GX_VA_POS].count,fh_gx.formats[0][GX_VA_POS].type,fh_gx.formats[0][GX_VA_POS].fraction,lists?*(const unsigned char*)lists[0]:0,sizes?sizes[0]:0);fh_platform_log(line);}
    if((!fh_native_scene_enabled && !fh_surface_detail) || recording || fh_gx.current_matrix%3 || fh_gx.current_matrix/3>=64)return 0;
    const FhVtxFormat *position=&fh_gx.formats[GX_VTXFMT0][GX_VA_POS];
    if(position->count!=GX_POS_XYZ || position->type!=GX_S16 || position->fraction)return 0;
    fh_gx_end_primitive();FhDrawState state=fh_gx.draw;
    memcpy(state.position_matrix,fh_gx.projection,sizeof(fh_gx.projection));state.raw_position=1;
    return fh_renderer_sky_begin(lists,sizes,count,&state,fh_gx.position[fh_gx.current_matrix/3]);
}
static int gx_model_draw(const FhModelGroup *group,const FhModelSource *source,
    const float *poses,unsigned pose_count,const float view[3][4],float normal_scale,
    const FhModelVertex *persistent_vertex,const uint16_t *persistent_index,unsigned direct_uv){
#ifdef FH_GX_DISPATCH_BUILD
    extern void fh_gx_dispatch_barrier(void);fh_gx_dispatch_barrier();
#endif
    fh_native_model_admission[0]++;
    if(recording || !group || !source || !isfinite(normal_scale) || normal_scale<=0){fh_native_model_admission[1]++;return 0;}
    FhNativeModelDraw draw={0};draw.group=*group;draw.source=*source;draw.normal_scale=normal_scale;
    draw.persistent_vertex=persistent_vertex;draw.persistent_index=persistent_index;
    draw.direct_uv=direct_uv;
    if(!fh_native_model_palette(draw.palette,group->bone_ids,group->bones,poses,pose_count,source->tails,view)){fh_native_model_admission[2]++;return 0;}
    fh_gx_end_primitive();
    if(persistent_vertex){
        if(!scene_model_state(&draw)){fh_native_model_admission[3]++;return 0;}
        return fh_renderer_native_model_draw(&draw);
    }
    list_batching=1;begin_primitive(GX_TRIANGLES,(GXVtxFmt)group->format,0,0);list_batching=0;
    if(!(fh_gx.draw.raw_position&1) || (fh_gx.draw.raw_position&(16|64)) || raster_channels>1 ||
       (lighting_needs_normal && !gpu_lighting)){fh_native_model_admission[3]++;return 0;}
    draw.state=fh_gx.draw;
    if(gpu_lighting){
        draw.lighting=*fh_renderer_prepared_lighting();
        if(draw.lighting.mask&FH_LIGHT_SPHERE){fh_native_model_admission[4]++;return 0;}
        unsigned mask=draw.lighting.mask&255;
        /* The first integrated program owns two complete light records.
         * Existing material LOD can supply diffuse fill for further lights. */
        if(mask&~3u){fh_native_model_admission[4]++;return 0;}
        draw.lit=mask || (draw.lighting.mask&FH_LIGHT_DIFFUSE_FILL);
    }
    for(unsigned c=0;c<4;c++){
        unsigned channel=c==3?2:0;const FhChannel *ch=&fh_gx.channel[channel];
        draw.color_from_vertex[c]=ch->material_source==GX_SRC_VTX;
        draw.color[c]=draw.color_from_vertex[c]?1.f:fh_gx.material[0][c];
        if(constant_lighting&(1u<<channel)){draw.color[c]=constant_colors[0][c];draw.color_from_vertex[c]=0;}
        else if(!draw.lit && ch->enabled){
            if(ch->ambient_source!=GX_SRC_REG){fh_native_model_admission[5]++;return 0;}
            float sum=fh_gx.ambient[0][c];
            for(unsigned i=0;i<8;i++)if(ch->mask&(1u<<i))sum+=fh_gx.lights[i].color[c];
            draw.color[c]*=clamp(sum);
        }
    }
    if(gpu_texgen && !group->normal_palette_matches){fh_native_model_admission[7]++;return 0;}
    if(gpu_texgen)memcpy(draw.post,fh_renderer_prepared_texgen(),sizeof(draw.post));
    else {draw.post[0][0]=draw.post[1][1]=draw.post[2][3]=1;}
    for(unsigned i=0;i<8;i++){
        if(!(fh_gx_texgen_inputs&(1u<<i)))continue;
        const FhTexGen *g=&fh_gx.texgens[i];
        if(i==0 && gpu_texgen){draw.coordinates[i]=2;continue;}
        if(g->source!=GX_TG_TEX0 || g->type!=GX_TG_MTX2x4 || g->normalize ||
           g->matrix!=GX_IDENTITY || g->postmatrix!=GX_PTIDENTITY || fh_gx.descriptors[GX_VA_TEX0MTXIDX+i]){fh_native_model_admission[6]++;return 0;}
        draw.coordinates[i]=1;
    }
    return fh_renderer_native_model_draw(&draw);
}
int fh_gx_native_model_draw(const FhModelGroup *group,const FhModelSource *source,
    const float *poses,unsigned pose_count,const float view[3][4],float normal_scale){
    return gx_model_draw(group,source,poses,pose_count,view,normal_scale,NULL,NULL,0);
}
int fh_gx_native_scene_draw(const FhSceneAsset *asset,const FhSceneGroup *g,const FhModelSource *source,
    const float *poses,unsigned pose_count,const float view[3][4],float normal_scale){
    if(!asset || !g || !source)return 0;
    fh_native_scene_admission[11]++;
    FhModelGroup group={.format=g->format,.cull=g->cull,.first=g->first,.draws=g->draws,.bones=g->bones,
        .vertices=g->vertices,.indices=g->indices,.bone_ids=g->bone_ids,.index=g->index,.refs=g->refs,.normal_palette_matches=1};
    FhModelSource s=*source;s.bindings=(const FhModelBinding*)(asset->meta+asset->bindings_at);s.binding_count=asset->bindings;
    s.tails=(const float(*)[3])(asset->meta+asset->tails_at);s.bone_count=asset->bones;s.format=g->format;
    int ok=gx_model_draw(&group,&s,poses,pose_count,view,normal_scale,g->vertex,g->index,asset->direct);
    fh_native_scene_admission[ok?12:13]++;
    if(ok)fh_native_scene_used(asset);
    return ok;
}

void GXEnd(void) { if(!recording) fh_gx_end_primitive(); }
void fh_gx_fifo_write(const void *data,unsigned bytes) {
    if(fh_render_floor && !recording)return;
#ifdef FH_GX_DISPATCH_BUILD
    extern void fh_gx_dispatch_barrier(void);fh_gx_dispatch_barrier();
#endif
    if(recording) { record(data,bytes);return; }
    const u8 *source=data;
    fh_gx_require(stride && (expected==GX_AUTO || received<expected),"Vertex data outside primitive");
    while(bytes) {
        unsigned count=stride-pending_bytes;if(count>bytes) count=bytes;
        fh_copy_bytes(pending+pending_bytes,source,count);pending_bytes+=count;source+=count;bytes-=count;
        if(pending_bytes==stride) { decode_vertex(pending);pending_bytes=0; }
    }
}
static void byte(u8 value) { fh_gx_fifo_write(&value,1); }
static void half(u16 value) { u8 b[]={value>>8,value};fh_gx_fifo_write(b,2); }
/* Attribute-sized writes retain GX's byte order without unaligned word stores.
 * FIFO consumers copy synchronously; these small buffers need no retained lifetime. */
static void real_bytes(u8 *b,float value) { u32 bits;fh_copy_bytes(&bits,&value,4);b[0]=bits>>24;b[1]=bits>>16;b[2]=bits>>8;b[3]=bits; }
static void real3(float x,float y,float z) { u8 b[12];real_bytes(b,x);real_bytes(b+4,y);real_bytes(b+8,z);fh_gx_fifo_write(b,sizeof(b)); }
void GXPosition3f32(f32 x,f32 y,f32 z) { real3(x,y,z); }
void GXPosition3s16(s16 x,s16 y,s16 z) { u8 b[]={(u16)x>>8,x,(u16)y>>8,y,(u16)z>>8,z};fh_gx_fifo_write(b,sizeof(b)); }
void GXPosition1x8(u8 index) { byte(index); }
void GXPosition1x16(u16 index) { half(index); }
void GXNormal3f32(f32 x,f32 y,f32 z) { real3(x,y,z); }
void GXNormal3s8(s8 x,s8 y,s8 z) { u8 b[]={x,y,z};fh_gx_fifo_write(b,sizeof(b)); }
void GXColor4u8(u8 r,u8 g,u8 b,u8 a) { u8 bytes[]={r,g,b,a};fh_gx_fifo_write(bytes,sizeof(bytes)); }
void GXColor1x16(u16 index) { half(index); }
void GXTexCoord2f32(f32 s,f32 t) { u8 b[8];real_bytes(b,s);real_bytes(b+4,t);fh_gx_fifo_write(b,sizeof(b)); }
void GXTexCoord2s16(s16 s,s16 t) { u8 b[]={(u16)s>>8,s,(u16)t>>8,t};fh_gx_fifo_write(b,sizeof(b)); }
void GXTexCoord1x8(u8 index) { byte(index); }
void GXTexCoord1x16(u16 index) { half(index); }
void GXBeginDisplayList(void *buffer,u32 capacity) { fh_gx_end_primitive();fh_gx_require(!recording && buffer && capacity,"Invalid display-list recording");recording=buffer;record_capacity=capacity;record_size=0; }
u32 GXEndDisplayList(void) {
    fh_gx_require(recording!=NULL,"No active display list");unsigned bytes=(record_size+31)&~31u;
    fh_gx_require(bytes<=record_capacity,"Display-list alignment overflow");memset(recording+record_size,0,bytes-record_size);recording=NULL;return bytes;
}
void GXCallDisplayList(const void *list,u32 bytes) {
    if(fh_render_floor && !recording){fh_render_floor_lists++;return;}
    if(recording) { record(list,bytes);return; }
    PHASE_START;
#ifdef FH_EMULATOR_TEST
    unsigned decoded_before=decoded_vertices;
#endif
    fh_gx_end_primitive();
    unsigned material_scope __attribute__((cleanup(fh_renderer_material_scope_end)))=fh_renderer_material_scope_begin(&fh_gx.draw);
    const u8 *data=list;unsigned offset=0,last_stride=0,last_vertices=0,last_command=0;
    unsigned geometry_vertices=fh_geometry_vertex_count(list,bytes),geometry_started=0;
#ifdef FH_EMULATOR_TEST
    fh_gx_last_list_vertices=geometry_vertices;
    extern unsigned fh_geometry_decline_reason;unsigned decline_reason=fh_geometry_decline_reason;
#endif
    cache_enabled=1;if(!++cache_epoch){memset(decoded_cache,0,sizeof(decoded_cache));cache_epoch=1;}
    while(offset<bytes) {
        unsigned command=data[offset++];if(!command) continue;
        unsigned kind=command&0xf8;
        if(kind==GX_QUADS || kind==GX_TRIANGLES || kind==GX_TRIANGLESTRIP || kind==GX_TRIANGLEFAN || kind==GX_LINES || kind==GX_LINESTRIP || kind==GX_POINTS) {
            fh_gx_require(bytes-offset>=2,"Truncated display-list draw");unsigned vertices=be16(data+offset);offset+=2;
            /* This decoder accepts only draw commands, so consecutive triangle
             * primitives retain the same raster state and matrix palette.
             * Expand strips/fans normally, then batch their triangles together.
             * Lines/points change culling/position handling and form boundaries. */
            unsigned merge=kind==GX_QUADS || kind==GX_TRIANGLES || kind==GX_TRIANGLESTRIP || kind==GX_TRIANGLEFAN;
            unsigned reuse=merge && list_batching && last_command && (last_command&7)==(command&7);
            if(!merge)flush();
            list_batching=merge;
            unsigned defer_shading=!last_command && merge;
            if(reuse){
                /* The previous primitive is complete and the list cannot
                 * change GX state. Keep its prepared formats/matrix palette. */
                primitive=kind;expected=vertices;received=pending_bytes=0;stride=last_stride;
            }else begin_primitive(kind,command&7,vertices,defer_shading);
            PHASE_END(0);
            if(!last_command && merge && (fh_gx.draw.raw_position&1) && list_outside(list,bytes)) {
                PHASE_END(1);
                list_batching=cache_enabled=stride=batch_count=0;return;
            }
            PHASE_END(1);
            /* Pinned Melee early visibility: matrices and positions suffice
             * to reject a list. Prepare lighting/texgen only for survivors. */
            const FhCompiledMesh *prepared_mesh=NULL;
            if(defer_shading && fh_palette_scope_mode && (fh_gx.draw.raw_position&1) && geometry_vertices>=96 &&
               fh_gx.descriptors[GX_VA_PNMTXIDX]==GX_DIRECT && position_matrix_offset==0){
                prepared_mesh=fh_compiled_mesh(list,bytes,stride,format);
                if(prepared_mesh){
                    lighting_palette_mask=prepared_mesh->matrix_mask;
                    if(fh_palette_scope_mode==2){
                        /* Independent walk includes duplicate packets and trailing
                         * vertices discarded by triangle assembly. Never use an
                         * early visibility exit as a complete dependency proof. */
                        unsigned scan=0,mask=0;
                        while(scan<bytes){
                            unsigned op=data[scan++];if(!op)continue;
                            unsigned n=be16(data+scan);scan+=2;
                            for(unsigned i=0;i<n;i++){
                                unsigned row=data[scan+i*stride];
                                mask|=row<=27 && row%3==0?1u<<(row/3):0x3ffu;
                            }
                            scan+=n*stride;
                        }
                        fh_gx_require(lighting_palette_mask==mask,"Compiled matrix dependency mismatch");fh_palette_scope_checks++;
                    }
                    fh_palette_scope_calls++;fh_palette_scope_matrices+=__builtin_popcount(lighting_palette_mask);
                }
            }
            if(defer_shading)prepare_vertex_shading();
            unsigned mutable_shading=lighting_needs_normal || gpu_lighting_palette==1 || texgen_needs_eye;
            if(!last_command && merge && (fh_gx.draw.raw_position&1) &&
               ((geometry_vertices>=384 && !mutable_shading) ||
                (geometry_vertices>=96 && (mutable_shading || fh_renderer_compiled_fur_available(&fh_gx.draw))))){
                const FhCompiledMesh *mesh=prepared_mesh?prepared_mesh:fh_compiled_mesh(list,bytes,stride,format);
                if(mesh){
                    unsigned old_expected=expected;expected=GX_AUTO;cache_enabled=0;
                    compiled_list=list;compiled_bytes=bytes;
                    int drawn=draw_compiled_native(mesh);
                    expected=old_expected;received=0;cache_enabled=1;
                    if(drawn){
                        fh_compiled_unique+=mesh->unique;fh_compiled_indices+=mesh->count;
                        list_batching=cache_enabled=stride=batch_count=0;PHASE_END(3);return;
                    }
                }
            }
            if(!geometry_started && geometry_vertices) {
                geometry_started=1;
                unsigned retained=fh_gx.draw.raw_position && !texgen_needs_eye &&
                    (gpu_lighting?gpu_lighting_palette!=1:!lighting_needs_normal) && !fh_retained_disabled;
                if(fh_retained_begin(list,bytes,geometry_vertices,format,raster_channels,retained)) {
                    PHASE_END(2);
                    list_batching=cache_enabled=stride=batch_count=0;return;
                }
                if(fh_retained_prefer_compiled){
                    const FhCompiledMesh *mesh=prepared_mesh?prepared_mesh:fh_compiled_mesh(list,bytes,stride,format);
                    if(mesh){
                        unsigned old_expected=expected;expected=GX_AUTO;cache_enabled=0;
                        compiled_list=list;compiled_bytes=bytes;
                        int drawn=draw_compiled_native(mesh);
                        expected=old_expected;received=0;cache_enabled=1;
                        if(drawn){
                            fh_compiled_unique+=mesh->unique;fh_compiled_indices+=mesh->count;
                            list_batching=cache_enabled=stride=batch_count=0;PHASE_END(3);return;
                        }
                    }
                    /* Unsupported materials still receive the full ordinary
                     * decode, and can acquire a fresh retained snapshot. */
                    if(fh_retained_begin(list,bytes,geometry_vertices,format,raster_channels,2)){
                        PHASE_END(2);list_batching=cache_enabled=stride=batch_count=0;return;
                    }
                }
                if(!retained && fh_gx.draw.raw_position==1 && fh_geometry_candidate(list,bytes) &&
                     fh_geometry_begin(list,bytes,geometry_vertices,raster_channels)) {
                    PHASE_END(2);
                    list_batching=cache_enabled=stride=batch_count=0;return;
                }
            }
            PHASE_END(2);
            last_stride=stride;last_vertices=vertices;last_command=command;
            uint64_t length=(uint64_t)stride*vertices;fh_gx_require(length<=bytes-offset,"Truncated display-list vertices");
            /* A complete list vertex needs no incremental FIFO staging copy. */
            for(unsigned i=0;i<vertices;i++)decode_vertex(data+offset+i*stride);
            offset+=length;fh_gx_end_primitive();
            PHASE_END(3);
        } else {
            FILE *file=fopen("sdmc:/3ds/foxhollow/display-list-failure.bin","wb");if(file) { fwrite(list,1,bytes,file);fclose(file); }
            file=fopen("sdmc:/3ds/foxhollow/display-list-state.txt","w");if(file) {
                fprintf(file,"last command=%x count=%u stride=%u\n",last_command,last_vertices,last_stride);
                for(unsigned attr=0;attr<26;attr++) fprintf(file,"attr=%u descriptor=%u count=%u type=%u fraction=%u\n",attr,fh_gx.descriptors[attr],fh_gx.formats[format][attr].count,fh_gx.formats[format][attr].type,fh_gx.formats[format][attr].fraction);
                fclose(file);
            }
            char message[128];snprintf(message,sizeof(message),"GX display-list opcode %02x at %u; preceding draw=%x vertices=%u stride=%u",command,offset-1,last_command,last_vertices,last_stride);fh_platform_panic(message);
        }
    }
    list_batching=0;flush();cache_enabled=0;fh_geometry_end();fh_retained_end();
#ifdef FH_EMULATOR_TEST
    unsigned why=!geometry_vertices?0:fh_gx.draw.texgens>8?1:lighting_needs_normal?2:gpu_lighting_palette==1?3:texgen_needs_eye?4:!fh_gx.draw.raw_position?5:6;
    fh_gx_uncached_vertices[why]+=decoded_vertices-decoded_before;
    if(!geometry_vertices)fh_gx_uncached_lists[decline_reason]+=decoded_vertices-decoded_before;
#endif
    PHASE_END(3);
}
