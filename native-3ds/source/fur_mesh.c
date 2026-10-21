#include "fh_fragment.h"
#include "fh_vertex_reuse.h"
#include "fh_fur_gpu.h"
#include "fh_source_identity.h"
#include <math.h>
#include <string.h>

typedef struct { unsigned hash,epoch,matrix_epoch;float uv[6],samples[2][3],offset[2]; } Coordinate;
/* Shells revisit the same indirect samples. Keep this bounded CPU cache large
 * enough to avoid measured collisions; it never owns GPU texture memory. */
enum { COORDINATES=2048 };
static Coordinate cache[COORDINATES];
_Static_assert(sizeof(cache)<=136*1024,"Fur coordinate cache budget");
static unsigned epoch=1,matrix_epoch=1;
static float cached_matrix[2][2][3];
static unsigned matrix_valid;
static struct {
    FhTexture textures[2];
    unsigned order[2][2],scale[2][2];
} context;
static unsigned context_valid;
unsigned fh_fur_sample_hits,fh_fur_sample_misses;
void fh_fur_cache_invalidate(void) {
    context_valid=0;
    if(!++epoch){memset(cache,0,sizeof(cache));epoch=1;}
}
void fh_fur_cache_invalidate_range(const void *address,size_t bytes) {
    if(!address){fh_fur_cache_invalidate();return;}
    if(!context_valid || !bytes)return;
    uintptr_t begin=(uintptr_t)address;
    for(unsigned i=0;i<2;i++){
        const FhTexture *t=&context.textures[i];
        uintptr_t sources[2]={(uintptr_t)t->pixels,(uintptr_t)t->palette};
        size_t sizes[2]={fh_texture_source_bytes(t->format,t->width,t->height),t->palette?t->palette_count*2:0};
        for(unsigned j=0;j<2;j++)if(sizes[j] && (begin>=sources[j]?begin-sources[j]<sizes[j]:sources[j]-begin<bytes)){
            fh_fur_cache_invalidate();return;
        }
    }
}
static void prepare_coordinates(const FhDrawState *s,const float matrix[2][2][3]) {
    if(!matrix_valid || memcmp(cached_matrix,matrix,sizeof(cached_matrix))) {
        memcpy(cached_matrix,matrix,sizeof(cached_matrix));matrix_valid=1;
        if(!++matrix_epoch){for(unsigned i=0;i<COORDINATES;i++)cache[i].matrix_epoch=0;matrix_epoch=1;}
    }
    typeof(context) next={0};
    for(unsigned i=0;i<2;i++){next.textures[i]=s->textures[s->indirect_order[i][1]];fh_texture_identity(&next.textures[i]);}
    memcpy(next.order,s->indirect_order,sizeof(next.order));
    memcpy(next.scale,s->indirect_scale,sizeof(next.scale));
    if(!context_valid || memcmp(&context,&next,sizeof(next))){
        fh_fur_cache_invalidate();context=next;context_valid=1;
    }
}
static Coordinate *samples_cached(const FhDrawState *s,const FhVertex *v) {
    float uv[6];memcpy(uv,v->uv[s->indirect_order[0][0]],12);
    memcpy(uv+3,v->uv[s->indirect_order[1][0]],12);
    unsigned hash=2166136261u;
    for(unsigned i=0;i<6;i++){unsigned bits;memcpy(&bits,uv+i,4);bits^=bits>>16;hash=(hash^bits)*16777619u;}
    /* Wordwise FNV alone leaves low bucket bits dependent only on low float
     * mantissa bits. Mix each word before combining: duplicated UV pairs can
     * otherwise collide in the full hash, beyond what a final avalanche fixes. */
    hash^=hash>>16;hash*=0x85ebca6bu;hash^=hash>>13;hash*=0xc2b2ae35u;hash^=hash>>16;
    Coordinate *entry=&cache[hash&(COORDINATES-1)];
    if(!(entry->epoch==epoch && entry->hash==hash && !memcmp(entry->uv,uv,sizeof(uv)))){
        fh_fur_sample_misses++;
        fh_fur_samples(entry->samples,s,v);entry->epoch=epoch;entry->hash=hash;entry->matrix_epoch=0;memcpy(entry->uv,uv,sizeof(uv));
    }else fh_fur_sample_hits++;
    return entry;
}
int fh_fur_gpu_coefficients_cached(float out[][3],const FhVertex *v,unsigned count,const FhDrawState *s){
    /* Reuse the existing bounded sample cache, including its frame/range
     * invalidation. Own all six UV words; descriptors, palettes, ordering and
     * scales remain context dependencies. Live shell strength stays on PICA.
     * Pinned Melee's source validation principle applies; there is no new
     * texture lifetime assumption or pointer-only cross-frame reuse. */
    if(!fh_fur_gpu_coordinates_valid(v,count,s))return 0;
    const float unused[2][2][3]={0};prepare_coordinates(s,unused);
    unsigned zero_second=fh_fur_gpu_zero_second(s);
    for(unsigned i=0;i<count;i++){
        Coordinate *e=samples_cached(s,v+i);
        out[i][0]=(zero_second?e->samples[0][0]-128:e->samples[0][0]+e->samples[1][1]-256)*.5f/s->textures[3].width;
        out[i][1]=(zero_second?e->samples[0][1]-128:e->samples[0][1]+e->samples[1][2]-256)*.5f/s->textures[3].height;
        out[i][2]=0;
    }
    return 1;
}
static void coordinate(float out[3],const FhDrawState *s,const FhVertex *v,const float matrix[2][2][3]) {
    Coordinate *entry=samples_cached(s,v);
    /* Shell matrices change while sampled textures/UVs stay equal. Retain
     * samples, then apply each shell's matrix in the original arithmetic order. */
    if(entry->matrix_epoch!=matrix_epoch) {
        fh_fur_offset(entry->offset,matrix,entry->samples);entry->matrix_epoch=matrix_epoch;
    }
    float q=v->uv[4][2];if(fabsf(q)<1e-20f)q=1;
    out[0]=v->uv[4][0]/q+entry->offset[0]/s->textures[3].width;
    out[1]=v->uv[4][1]/q+entry->offset[1]/s->textures[3].height;
    out[2]=((unsigned)entry->samples[1][0]&248)/248.f;
}
static void average(FhVertex *out,const FhVertex *a,const FhVertex *b) {
    for(unsigned i=0;i<36;i++)((float*)out)[i]=.5f*(((const float*)a)[i]+((const float*)b)[i]);
}
static unsigned refine(FhVertex *out,unsigned capacity,const FhVertex v[3],const FhDrawState *s,const float matrix[2][2][3],unsigned depth) {
    if(capacity<3)return 0;
    float coords[3][3];for(unsigned i=0;i<3;i++)coordinate(coords[i],s,&v[i],matrix);
    FhVertex mid[3];int split=0;
    for(unsigned i=0;i<3;i++) {
        unsigned j=(i+1)%3;average(&mid[i],&v[i],&v[j]);float sample[3];coordinate(sample,s,&mid[i],matrix);
        float q=v[i].uv[4][2]+v[j].uv[4][2];
        if(fabsf(q)<1e-10f)return 0;
        for(unsigned a=0;a<2;a++) {
            float expected=(coords[i][a]*v[i].uv[4][2]+coords[j][a]*v[j].uv[4][2])/q;
            if(fabsf(expected-sample[a])*(a?s->textures[3].height:s->textures[3].width)>.25f)split=1;
        }
        for(unsigned t=0;t<2;t++) {
            int active=0;for(unsigned a=0;a<2;a++)for(unsigned c=0;c<3;c++)active|=matrix[t][a][c]!=0;
            if(!active)continue;
            unsigned c=s->indirect_order[t][0],m=s->indirect_order[t][1];
            for(unsigned a=0;a<2;a++) {
                float extent=fabsf(v[i].uv[c][a]/v[i].uv[c][2]-v[j].uv[c][a]/v[j].uv[c][2])*(a?s->textures[m].height:s->textures[m].width);
                if(extent>2)split=1;
            }
        }
    }
    FhVertex center;for(unsigned i=0;i<36;i++)((float*)&center)[i]=(((const float*)&v[0])[i]+((const float*)&v[1])[i]+((const float*)&v[2])[i])/3;
    float sample[3];coordinate(sample,s,&center,matrix);
    float q=v[0].uv[4][2]+v[1].uv[4][2]+v[2].uv[4][2];if(fabsf(q)<1e-10f)return 0;
    for(unsigned a=0;a<2;a++) {
        float expected=(coords[0][a]*v[0].uv[4][2]+coords[1][a]*v[1].uv[4][2]+coords[2][a]*v[2].uv[4][2])/q;
        if(fabsf(expected-sample[a])*(a?s->textures[3].height:s->textures[3].width)>.25f)split=1;
    }
    if(split) {
        if(depth>=4)return 0;
        unsigned used=0;
        for(unsigned i=0;i<4;i++) {
            FhVertex child[3];
            if(i==3)memcpy(child,mid,sizeof(child));
            else {child[0]=v[i];child[1]=mid[i];child[2]=mid[(i+2)%3];}
            unsigned n=refine(out+used,capacity-used,child,s,matrix,depth+1);if(!n)return 0;used+=n;
        }
        return used;
    }
    for(unsigned i=0;i<3;i++) {
        out[i]=v[i];out[i].color[0][3]=coords[i][2];
        for(unsigned a=0;a<2;a++)out[i].uv[7][a]=coords[i][a]*v[i].uv[4][2];
        out[i].uv[7][2]=v[i].uv[4][2];
    }
    return 3;
}
static int valid(const FhVertex triangle[3],const FhDrawState *state) {
    unsigned masked=state->tev[3].channel!=255;
    if(masked && (state->indirect_scale[1][0] || state->indirect_scale[1][1] || state->textures[0].wrap_s!=state->textures[2].wrap_s || state->textures[0].wrap_t!=state->textures[2].wrap_t || state->textures[0].mag_filter!=state->textures[2].mag_filter))return 0;
    for(unsigned i=0;i<3;i++) {
        if(masked && memcmp(triangle[i].uv[2],triangle[i].uv[3],12))return 0;
        if(triangle[i].uv[0][2]!=triangle[0].uv[0][2] || triangle[i].uv[2][2]!=triangle[0].uv[2][2])return 0;
        for(unsigned c=0;c<5;c++)if(fabsf(triangle[i].uv[c][2])<1e-10f)return 0;
    }
    return 1;
}
unsigned fh_fur_coordinates_identified(float output[][3],const FhVertex *triangle,unsigned count,const FhDrawState *state,const float matrix[2][2][3],const uint32_t *identities) {
    if(!count || count%3)return 0;
    for(unsigned i=0;i<count;i+=3)if(!valid(triangle+i,state))return 0;
    prepare_coordinates(state,matrix);
    FhVertexReuse reuse={0};
    for(unsigned i=0;i<count;i++) {
        unsigned previous=identities?fh_vertex_reuse(&reuse,identities[i],i):i;
        if(previous!=i){memcpy(output[i],output[previous],sizeof(output[i]));continue;}
        float coord[3];coordinate(coord,state,&triangle[i],matrix);
        for(unsigned c=0;c<2;c++)output[i][c]=coord[c]*triangle[i].uv[4][2];
        output[i][2]=triangle[i].uv[4][2];
    }
    return count;
}
unsigned fh_fur_coordinates_cached(float output[][3],const FhVertex *triangle,unsigned count,const FhDrawState *state,const float matrix[2][2][3]) {
    return fh_fur_coordinates_identified(output,triangle,count,state,matrix,NULL);
}
unsigned fh_fur_vertices_cached(FhVertex *output,const FhVertex *triangle,unsigned count,const FhDrawState *state,const float matrix[2][2][3]) {
    if(!count || count%3)return 0;
    for(unsigned i=0;i<count;i+=3)if(!valid(triangle+i,state))return 0;
    prepare_coordinates(state,matrix);
    for(unsigned i=0;i<count;i++) {
        float coord[3];coordinate(coord,state,&triangle[i],matrix);output[i]=triangle[i];
        for(unsigned c=0;c<2;c++)output[i].uv[7][c]=coord[c]*triangle[i].uv[4][2];
        output[i].uv[7][2]=triangle[i].uv[4][2];
    }
    return count;
}
unsigned fh_fur_vertices(FhVertex *output,const FhVertex *triangle,unsigned count,const FhDrawState *state,const float matrix[2][2][3]) {
    fh_fur_cache_invalidate();
    return fh_fur_vertices_cached(output,triangle,count,state,matrix);
}
unsigned fh_fur_mesh(FhVertex *output,unsigned capacity,const FhVertex triangle[3],const FhDrawState *state,const float matrix[2][2][3]) {
    if(!valid(triangle,state))return 0;
    fh_fur_cache_invalidate();prepare_coordinates(state,matrix);
    return refine(output,capacity,triangle,state,matrix,0);
}
