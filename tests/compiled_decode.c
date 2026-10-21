/* Appended to the production decoder by test_compiled_decode.py. */
#include <assert.h>
FhGx fh_gx;
void fh_platform_panic(const char *message){fprintf(stderr,"%s\n",message);abort();}
void fh_gx_require(int condition,const char *message){if(!condition)fh_platform_panic(message);}
int fh_memory_compare(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
void fh_memory_zero(void *p,size_t n){memset(p,0,n);}
void *fh_memory_copy(void *a,const void *b,size_t n){return memcpy(a,b,n);}
/* Retention service double: force misses while comparing every actual batch
 * output to the production scalar decoder. Then expose a verified static hit.
 * No decoder, packet reader, matrix or shading arithmetic is replaced. */
unsigned fh_retained_disabled;
/* This fixture exercises ordinary compiled materials, never a fur pipeline. */
int fh_renderer_compiled_fur_available(const FhDrawState *state){assert(state->texgens!=5);return 0;}
int fh_renderer_fur_texgen_available(const FhDrawState *state){return fh_renderer_compiled_fur_available(state);}
static unsigned retention_reads,retention_records,retention_pending,retention_hit;
static uint64_t retention_id;
static FhVertex retention_value;
uint64_t fh_retained_probe_compiled(const void *p,unsigned b,unsigned n,unsigned f,unsigned r){
    (void)p;(void)b;(void)n;(void)f;(void)r;retention_pending=0;retention_id=retention_hit?42:0;return retention_id;
}
int fh_retained_read_compiled(const void *p,unsigned b,unsigned n,unsigned f,unsigned r,FhVertex *out){
    (void)p;(void)b;(void)f;(void)r;assert(n==1);retention_reads++;retention_id=0;
    if(retention_hit){*out=retention_value;retention_id=42;return 1;}
    retention_pending=1;return 0;
}
uint64_t fh_retained_compiled_identity(void){return retention_id;}
void fh_retained_record(const FhVertex *v,unsigned n,const uint32_t *ids){assert(retention_pending && n==1 && ids[0]==1);retention_value=*v;retention_records++;}
void fh_retained_end(void){if(retention_pending){retention_pending=0;retention_id=42;}}
static uint32_t random_state=1;
static unsigned next_random(void){random_state=random_state*1664525u+1013904223u;return random_state;}
static void put_component(u8 *p,unsigned type,int le){
    unsigned bytes=component_bytes(type),bits=next_random();
    if(type==GX_F32){float f=(int)(bits%129)-64;f/=128;memcpy(&bits,&f,4);}
    for(unsigned i=0;i<bytes;i++)p[le?i:bytes-1-i]=bits>>(i*8);
}
int main(void){
    unsigned compared=0,fallbacks=0;u8 arrays[3][64],packet[128];
    cache_enabled=0;expected=GX_AUTO;list_batching=1;
    for(unsigned trial=0;trial<60000;trial++){
        memset(&fh_gx,0,sizeof(fh_gx));memset(fields,0,sizeof(fields));memset(prepared_texgen,0,sizeof(prepared_texgen));
        unsigned type=trial%5,mode=1+(trial/5)%3,le=(trial/15)%2,raw=(trial/30)%2?7:3;
        gpu_lighting=1;gpu_lighting_flat=0;gpu_lighting_palette=(trial/60)%4;
        if(gpu_lighting_palette==1)gpu_lighting_palette=2;
        gpu_texgen=!!(raw&4);lighting_needs_normal=texgen_needs_eye=constant_lighting=cpu_light_state_id=0;
        raster_channels=1;fh_gx.draw.raw_position=raw;fh_gx_active_texgens=gpu_texgen?2:1;
        fh_gx.material[0][3]=(trial%17)/16.f;
        fh_gx.texgens[0]=(FhTexGen){GX_TG_MTX2x4,gpu_texgen?GX_TG_NRM:GX_TG_TEX0,gpu_texgen?GX_TEXMTX0:GX_IDENTITY,0,GX_PTIDENTITY};
        if(gpu_texgen)fh_gx.texgens[1]=(FhTexGen){GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,0,GX_PTIDENTITY};
        field_count=0;stride=0;
        if(trial&1){
            fh_gx.descriptors[GX_VA_PNMTXIDX]=GX_DIRECT;
            fields[field_count++]=(PreparedField){.attr=GX_VA_PNMTXIDX,.mode=GX_DIRECT,.offset=stride};packet[stride++]=((trial/120)%10)*3;
            if(gpu_texgen){fh_gx.descriptors[GX_VA_TEX0MTXIDX]=GX_DIRECT;fields[field_count++]=(PreparedField){.attr=GX_VA_TEX0MTXIDX,.mode=GX_DIRECT,.offset=stride};packet[stride++]=packet[0]+GX_TEXMTX0;}
        }
        for(unsigned a=0;a<3;a++){
            unsigned attr=a==0?GX_VA_POS:a==1?GX_VA_NRM:GX_VA_TEX0,n=a==2?2:3,cb=component_bytes(type),size=n*cb;
            fh_gx.descriptors[attr]=mode;
            PreparedField *f=&fields[field_count++];*f=(PreparedField){.attr=attr,.mode=mode,.bytes=size,.components=n,.component_bytes=cb,.offset=stride,.scale=type==GX_F32?1.f:1.f/128,.format={a==0?GX_POS_XYZ:a==1?GX_NRM_XYZ:GX_TEX_ST,type,7}};
            u8 *data;
            if(mode==GX_DIRECT){data=packet+stride;stride+=size;}
            else {unsigned index=1;packet[stride++]=mode==GX_INDEX8?index:0;if(mode==GX_INDEX16)packet[stride++]=index;fh_gx.arrays[attr]=(FhVtxArray){arrays[a],sizeof(arrays[a]),size,(unsigned)le};data=arrays[a]+size;}
            for(unsigned c=0;c<n;c++)put_component(data+c*cb,type,mode==GX_DIRECT?0:le);
        }
        FastCompiled fast;assert(prepare_fast_compiled(&fast));FhVertex actual,reference;
        if(decode_fast_compiled(&actual,packet,&fast)){
            decode_compiled(&reference,packet);
            if(memcmp(&actual,&reference,sizeof(actual))){fprintf(stderr,"Mismatch trial %u type %u mode %u le %u raw %u\n",trial,type,mode,le,raw);abort();}
            compared++;
        }else fallbacks++;
        fh_gx.channel[2].material_source=GX_SRC_VTX;fh_gx.channel[2].enabled=1;
        assert(!prepare_fast_compiled(&fast));
        fh_gx.channel[2].material_source=GX_SRC_REG;fh_gx.channel[2].enabled=0;
        texgen_needs_eye=1;assert(!prepare_fast_compiled(&fast));
    }
    assert(compared>10000 && fallbacks>10000);
    texgen_needs_eye=0;compiled_list=packet;compiled_bytes=stride;format=0;
    uint32_t offset=0;uint16_t index=0;FhCompiledMesh mesh={packet,&offset,&index,1,1};
    for(unsigned frame=0;frame<1024;frame++){
        /* Live indexed position data changes between every cache miss. */
        put_component(arrays[0]+fh_gx.arrays[GX_VA_POS].stride,GX_F32,1);
        FhVertex actual,reference;decode_compiled(&reference,packet);
        assert(!decode_compiled_batch(NULL,&mesh));
        decode_compiled_batch(&actual,&mesh);
        assert(!memcmp(&actual,&reference,sizeof(actual)));
    }
    assert(retention_reads<40 && retention_records==retention_reads);
    retention_hit=1;decode_compiled(&retention_value,packet);
    assert(decode_compiled_batch(NULL,&mesh)==42);
    FhVertex recovered;unsigned before=retention_reads;
    assert(decode_compiled_batch(&recovered,&mesh)==42 && retention_reads==before+1);
    assert(!memcmp(&recovered,&retention_value,sizeof(recovered)));
    printf("1024 changing batches exactly match scalar; %u cache recordings; static reuse recovers\n",retention_records);
    printf("PASS: %u exact vertices, %u extreme-value scalar fallbacks; 120000 unsupported-state rejections\n",compared,fallbacks);
    return 0;
}
