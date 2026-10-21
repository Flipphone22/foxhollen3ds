/* Included by the resident material owner after its ordinary-world admission
 * predicate. All allocations and descriptor compilation happen during LOAD.
 * This borrows the same image/geometry lifetime as the source material set. */
typedef struct FhWorldPacketGroup FhWorldPacketGroup;
typedef struct {
    FhPicaResidentDraw draw;
    FhWorldPacketGroup *group;
    const uint32_t *raster[3];
    unsigned material,ready,buffer_mask;
    float color_scale,uv_scale;
} FhPicaWorldCommandDraw;
struct FhPicaWorldCommands {
    unsigned bytes,count;
    FhPicaStreamArena arena;
    FhPicaSamplerCommands neutral[2];
    FhPicaWorldCommandDraw draw[];
};
static void world_packets_release(FhPicaWorldCommands *);
static void world_packets_prepare(const FhPicaWorldPlan *,FhPicaMaterialSet *);
static unsigned world_stream_bytes,world_stream_peak,world_stream_plans,world_stream_draws;
static unsigned world_stream_other,world_stream_hidden;
static int world_sampler(FhPicaSamplerCommands *out,unsigned unit,const C3D_Tex *s){
    FhPicaStreamSampler input={s->border,s->dim,s->param,s->lodParam,osConvertVirtToPhys(s->data),s->fmt};
    return fh_pica_stream_sampler(out,unit,&input);
}
static void world_stream_release(FhPicaMaterialSet *set){
    if(!set || !set->items || !set->count)return;
    Material *m=set->items;FhPicaWorldCommands *stream=m->world_commands;
    if(!stream)return;
    /* The scene owner has already retired GPU readers before material release. */
    world_packets_release(stream);
    world_stream_bytes-=stream->bytes;deallocate_command_plan(stream,stream->bytes);m->world_commands=NULL;
}
static int world_stream_prepare(const FhSceneAsset *asset,FhPicaMaterialSet *set){
    if(!asset->block || !fh_pica_scene_textured)return 1;
    const FhPicaWorldPlan *plan=fh_native_scene_pica_plan(asset);
    if(!plan || !set->items || !set->count)return 0;
    Material *materials=set->items;if(materials->world_commands)return 0;
    unsigned frames=0;for(unsigned i=0;i<set->count;i++)for(unsigned j=0;j<2;j++)frames+=materials[i].count[j];
    /* One native vertex layout, one fetch layout, six cull/raster states,
     * at most one TEV program per material and one index packet per draw.
     * Each interned block includes three metadata words. No worst-case
     * full-template allocation per raster variant. */
    unsigned words=1+15+43+6*21+set->count*27+plan->count*33;
    size_t bytes=sizeof(FhPicaWorldCommands)+(size_t)plan->count*sizeof(FhPicaWorldCommandDraw)+
        (size_t)frames*sizeof(FhPicaSamplerCommands)+(size_t)words*4;
    if(bytes>192u*1024u)return 0;
    FhPicaWorldCommands *stream=allocate_command_plan((unsigned)bytes);if(!stream)return 0;
    stream->bytes=(unsigned)bytes;stream->count=plan->count;
    FhPicaSamplerCommands *samplers=(void*)(stream->draw+plan->count);
    uint32_t *arena=(void*)(samplers+frames);
    if(!fh_pica_stream_arena_init(&stream->arena,arena,words))goto failed;
    for(unsigned j=0;j<2;j++)if(!world_sampler(stream->neutral+j,j,&neutral_texture))goto failed;
    for(unsigned i=0;i<set->count;i++)for(unsigned j=0;j<2;j++)for(unsigned k=0;k<materials[i].count[j];k++){
        Frame *f=materials[i].frames[j]+k;
        if(!f->handle || !images[f->handle-1].refs || !world_sampler(samplers,j,&f->sampler))goto failed;
        f->commands=samplers++;
    }
    for(unsigned i=0;i<plan->count;i++){
        const FhPicaWorldDraw *g=plan->draw+i;FhPicaWorldCommandDraw *entry=stream->draw+i;
        if(g->material>=set->count)goto failed;
        Material *m=materials+g->material;entry->material=g->material;
        if(!m->ready || !world_material_plain(m))continue;
        if(!m->program || !programs[m->program-1].refs)goto failed;
        const Program *p=programs+m->program-1;
        FhPicaDrawDescription description={0};
        if(!fh_pica_world_stream_geometry(asset,g,&description))goto failed;
        memcpy(description.stages,p->stages,sizeof description.stages);
        description.texture_units=(1u<<m->layers)-1;
        for(unsigned variant=0;variant<3;variant++){
            unsigned blend=variant==2;
            description.effect.alphaTest=(variant==1)|((unsigned)GPU_GREATER<<4);
            description.effect.depthTest=1|((unsigned)GPU_GEQUAL<<4)|((blend?GPU_WRITE_COLOR:GPU_WRITE_ALL)<<8);
            description.effect.alphaBlend=GPU_BLEND_ADD|(GPU_BLEND_ADD<<8)|
                ((blend?GPU_SRC_ALPHA:GPU_ONE)<<16)|((blend?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO)<<20)|
                (GPU_ONE<<24)|(GPU_ZERO<<28);
            FhPicaResidentDraw compiled;
            if(!fh_pica_stream_world_compile(&stream->arena,&compiled,&description))goto failed;
            entry->raster[variant]=compiled.effect;
            if(!variant)entry->draw=compiled;
        }
        entry->color_scale=asset->compact?1.f/15:1.f;
        entry->uv_scale=asset->compact?(g->format==5?1.f/256:1.f/1024):1.f;
        entry->buffer_mask=p->buffer_mask;entry->ready=1;
    }
    materials->world_commands=stream;world_stream_bytes+=stream->bytes;
    if(world_stream_bytes>world_stream_peak)world_stream_peak=world_stream_bytes;
    world_stream_plans++;world_packets_prepare(plan,set);return 1;
failed:
    for(unsigned i=0;i<set->count;i++)for(unsigned j=0;j<2;j++)for(unsigned k=0;k<materials[i].count[j];k++)
        materials[i].frames[j][k].commands=NULL;
    deallocate_command_plan(stream,stream->bytes);return 0;
}
static unsigned world_stream_select(const FhPicaMaterialSet *set,unsigned index,
                                     FhPicaDrawLive *live,FhPicaWorldSelection *selection,unsigned account){
    if(!set || !set->items || !set->count)return 0;
    const Material *materials=set->items;
    const FhPicaWorldCommands *stream=materials->world_commands;
    if(!stream || index>=stream->count)return 0;
    const FhPicaWorldCommandDraw *entry=stream->draw+index;
    const Material *m=materials+entry->material;
    if(!entry->ready || !world_material_plain(m)){if(account)world_stream_other++;return 0;}
    if(m->flags&(0x80000000u|2u)){if(account)world_stream_hidden++;return 2;}
    unsigned variant=(m->flags&0x60000000u)?2:!!(m->masked || ((m->flags&0x400)&&!(m->flags&0x80)));
    selection->draw=&entry->draw;live->effect=entry->raster[variant];
    selection->fog=fh_pica_scene_actors && !!(m->flags&4);
    selection->buffer_mask=entry->buffer_mask;
    float values[12]={m->tint[3]*entry->color_scale,m->tint[2]*entry->color_scale,
        m->tint[1]*entry->color_scale,m->tint[0]*entry->color_scale};
    for(unsigned i=0;i<2;i++){
        const Frame *f=i<m->layers?m->frames[i]+m->current[i]:NULL;
        float sx=f?f->scale[0]:1,sy=f?f->scale[1]:1;
        live->samplers[i]=f?((fh_graphics_options&FH_GRAPHICS_NO_WORLD_TEX)?stream->neutral+i:f->commands):NULL;
        values[4+i*4]=m->scroll[i][1]*sy;values[5+i*4]=m->scroll[i][0]*sx;
        values[6+i*4]=sy*entry->uv_scale;values[7+i*4]=sx*entry->uv_scale;
    }
    memcpy(live->material_uniforms,values,sizeof values);
    for(unsigned i=0;i<4;i++)live->colors[i]=m->konst;
    if(account){world_stream_draws++;bindings++;}return 1;
}
unsigned fh_pica_world_stream_select(const FhPicaMaterialSet *set,unsigned index,
    FhPicaDrawLive *live,FhPicaWorldSelection *selection){
    return world_stream_select(set,index,live,selection,1);
}
#include "platform_pica_world_packets.h"
void fh_pica_world_stream_common(unsigned fog){
    C3D_LightEnvBind(NULL);fh_pica_fog_bind(fog);fh_pica_scene_specular(0);
    fh_pica_texture2_coord1(0);C3D_TexBind(2,NULL);fh_pica_color_fade_bind(0);
}
