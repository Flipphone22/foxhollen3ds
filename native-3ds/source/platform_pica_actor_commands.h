/* Material owner's native actor lane; only resource admission allocates. */
typedef struct {FhPicaActorGeometry geometry;unsigned material,format,ready;uint32_t packet[30];} FhPicaActorCommandDraw;
struct FhPicaActorCommands {
    unsigned bytes,count;const void *storage;unsigned address;
    FhPicaActorCommandDraw draw[];
};
static unsigned actor_stream_bytes,actor_stream_peak,actor_stream_plans,actor_stream_draws,actor_stream_other;
static FhPicaSamplerCommands actor_neutral;
static uint32_t actor_attributes[12],actor_layout[40];
static unsigned actor_layout_ready,actor_stream_error,actor_stream_requested;
static int actor_stream_prepare(const FhSceneAsset *asset,FhPicaMaterialSet *set){
    actor_stream_error=1;actor_stream_requested=0;
    if(asset->block || !asset->compact || !fh_pica_scene_actors)return 1;
    const FhPicaModelPlan *plan=fh_native_scene_pica_model(asset);
    if(!plan || !set->items || !set->count)return 0;
    Material *materials=set->items;if(materials->actor_commands)return 0;
    unsigned frames=0;for(unsigned i=0;i<set->count;i++)for(unsigned j=0;j<2;j++)frames+=materials[i].count[j];
    size_t bytes=sizeof(FhPicaActorCommands)+(size_t)plan->count*sizeof(FhPicaActorCommandDraw)+
        (size_t)frames*sizeof(FhPicaSamplerCommands);
    actor_stream_requested=bytes;
    if(bytes>192u*1024u)return 0;
    actor_stream_error=2;
    FhPicaActorCommands *stream=allocate_command_plan(bytes);if(!stream)return 0;
    stream->bytes=bytes;stream->count=plan->count;stream->storage=asset->gpu;stream->address=osConvertVirtToPhys(asset->gpu);
    FhPicaSamplerCommands *samplers=(void*)(stream->draw+plan->count);
    actor_stream_error=3;
    if(!fh_pica_actor_sampler_prepare(&actor_neutral,&neutral_texture))goto failed;
    for(unsigned i=0;i<set->count;i++)for(unsigned j=0;j<2;j++)for(unsigned k=0;k<materials[i].count[j];k++){
        Frame *f=materials[i].frames[j]+k;
        if(!f->handle || !images[f->handle-1].refs || !fh_pica_actor_sampler_prepare(samplers,&f->sampler))goto failed;
        f->commands=samplers++;
    }
    actor_stream_error=4;
    for(unsigned i=0;i<plan->count;i++){
        const FhPicaModelDraw *d=plan->draw+i;FhPicaActorCommandDraw *entry=stream->draw+i;
        if(d->material>=set->count)goto failed;
        entry->material=d->material;entry->format=d->format;
        if(!materials[d->material].ready)continue;
        FhPicaDrawDescription desc={0};uint32_t scratch[256];FhPicaStreamArena arena;
        if(!fh_pica_stream_arena_init(&arena,scratch,256) || !fh_pica_actor_stream_geometry(asset,d,&desc) ||
           !fh_pica_actor_geometry_compile(&arena,&entry->geometry,&desc))goto failed;
        if(!actor_layout_ready){
            memcpy(actor_attributes,entry->geometry.attributes,sizeof actor_attributes);
            memcpy(actor_layout,entry->geometry.layout,sizeof actor_layout);actor_layout_ready=1;
        }else if(memcmp(actor_attributes,entry->geometry.attributes,sizeof actor_attributes) ||
                 memcmp(actor_layout,entry->geometry.layout,sizeof actor_layout))goto failed;
        memcpy(entry->packet,entry->geometry.draw,sizeof entry->packet);
        entry->geometry.attributes=actor_attributes;entry->geometry.layout=actor_layout;entry->geometry.draw=entry->packet;
        entry->ready=1;
    }
    materials->actor_commands=stream;actor_stream_bytes+=bytes;
    if(actor_stream_bytes>actor_stream_peak)actor_stream_peak=actor_stream_bytes;
    actor_stream_plans++;actor_stream_error=0;return 1;
failed:
    for(unsigned i=0;i<set->count;i++)for(unsigned j=0;j<2;j++)for(unsigned k=0;k<materials[i].count[j];k++)materials[i].frames[j][k].commands=NULL;
    deallocate_command_plan(stream,bytes);return 0;
}
static void actor_stream_release(FhPicaMaterialSet *set){
    fh_pica_actor_stream_end(); /* Program/sampler slots may also belong to world sets. */
    if(!set || !set->items || !set->count)return;
    Material *m=set->items;FhPicaActorCommands *stream=m->actor_commands;
    if(!stream)return;
    actor_stream_bytes-=stream->bytes;deallocate_command_plan(stream,stream->bytes);m->actor_commands=NULL;
}
/* First native material admission follows the game's separate actor material
 * loader. Later prepared model views may select a different GPU allocation.
 * Update only native binding addresses at that publication boundary; vertices,
 * images, indices and already submitted command buffers remain untouched. */
static int actor_stream_admit(const FhSceneAsset *asset){
    FhPicaMaterialSet *set=plan_for(asset);
    if(!set || !set->items || !set->count)return 0;
    Material *m=set->items;FhPicaActorCommands *stream=m->actor_commands;
    if(!stream)return actor_stream_prepare(asset,set);
    if(stream->storage==asset->gpu)return 1;
    unsigned address=osConvertVirtToPhys(asset->gpu),delta=address-stream->address;
    if(address<0x18000000u || (address&15))return 0;
    for(unsigned i=0;i<stream->count;i++)if(stream->draw[i].ready){
        stream->draw[i].geometry.offsets[0]+=delta;
        stream->draw[i].geometry.offset_candidates=(stream->draw[i].geometry.offset_candidates&~1u)|
            (stream->draw[i].geometry.offsets[0]!=0);
        stream->draw[i].packet[4]+=delta;
    }
    stream->storage=asset->gpu;stream->address=address;return 1;
}
unsigned fh_pica_actor_stream_select(const FhPicaMaterialSet *set,unsigned index,unsigned lit,
    FhPicaActorParameters *p,FhPicaActorSelection *out){
    const Material *materials=set->items;const FhPicaActorCommands *stream=materials?materials->actor_commands:NULL;
    if(!stream || index>=stream->count)return 0;
    const FhPicaActorCommandDraw *entry=stream->draw+index;const Material *m=materials+entry->material;
    if(!entry->ready || !m->actor || m->ui.kind || m->moon_alpha || m->movie_active || m->cloud_state || m->post_state){actor_stream_other++;return 0;}
    unsigned environment=m->environment && !(fh_graphics_options&(FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ENVIRONMENT));
    const FhPicaSamplerCommands *sphere=NULL;unsigned specular=0,diffuse=0;
    if(m->specular_program && !m->layered && (fh_graphics_options&FH_GRAPHICS_SPECULAR) &&
       !(fh_graphics_options&(FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ACTOR_TEX))){
        sphere=lit?fh_pica_specular_commands(m->specular_slot):NULL;specular=sphere!=NULL;
        if(!sphere)specular_unready++;
    }
    if(!m->specular_program || !(fh_graphics_options&FH_GRAPHICS_SPECULAR)){
        if(m->diffuse_program && environment && !m->layered && !(fh_graphics_options&FH_GRAPHICS_NO_LIGHTING) &&
           (fh_graphics_options&FH_GRAPHICS_DIFFUSE_MAPS) && !(fh_graphics_options&FH_GRAPHICS_NO_ACTOR_TEX)){
            sphere=lit?fh_pica_diffuse_commands(m->environment_slot):NULL;diffuse=sphere!=NULL;
            if(!sphere)diffuse_unready++;
        }
    }
    unsigned swizzle_handle=m->environment?(m->swizzle>=6?m->swizzle_environment[fh_pica_texture_swizzle_missing(m->swizzle)]:0):m->swizzle_program;
    if(diffuse)swizzle_handle=m->environment?(m->swizzle>=6?m->swizzle_diffuse[1+fh_pica_texture_swizzle_missing(m->swizzle)]:0):m->swizzle_diffuse[0];
    unsigned swizzle=m->swizzle && swizzle_handle && !specular;
    unsigned handle=swizzle?swizzle_handle:specular?m->specular_program:diffuse?m->diffuse_program:m->program;
    p->material=&programs[handle-1].commands;
    for(unsigned i=0;i<4;i++){
        unsigned color=m->environment && !i?0xff000000u|(environment?m->environment_strength*0x010101u:0):m->konst;
        if(swizzle)color=m->environment?fh_pica_texture_swizzle_environment_color(m->swizzle,i,m->konst,environment?m->environment_strength:0):fh_pica_texture_swizzle_color(m->swizzle,i,m->konst);
        else if(specular && i==1)color=m->specular_alpha<<24;
        p->colors[i]=color;
    }
    float values[12]={m->tint[3],m->tint[2],m->tint[1],m->tint[0]};
    float us=entry->format==5?1.f/256:1.f/1024; /* Lane admits compact assets. */
    p->samplers[0]=p->samplers[1]=p->samplers[2]=NULL;unsigned units=0;
    for(unsigned i=0;i<2;i++){
        const Frame *f=i<m->layers?m->frames[i]+m->current[i]:NULL;
        float sx=f?f->scale[0]:1,sy=f?f->scale[1]:1;unsigned unit=sphere?i+1:i;
        if(f){p->samplers[unit]=(fh_graphics_options&FH_GRAPHICS_NO_ACTOR_TEX)?&actor_neutral:f->commands;units|=1u<<unit;}
        if(!sphere || !i){values[4+unit*4]=m->scroll[i][1]*sy;values[5+unit*4]=m->scroll[i][0]*sx;values[6+unit*4]=sy*us;values[7+unit*4]=sx*us;}
    }
    if(sphere){p->samplers[0]=sphere;units|=1;values[4]=.5f;values[5]=.5f;values[6]=-.5f;values[7]=-.5f;}
    memcpy(p->uniforms,values,sizeof values);p->texture_config=(p->texture_config&~7u)|units;
    unsigned raster=m->actor_raster;
    p->alpha_test=(!!(raster&FH_AR_ALPHA))|(GPU_GREATER<<4)|(((raster>>8)&255)<<8);
    p->depth_test=(!!(raster&FH_AR_DEPTH))|(GPU_GEQUAL<<4)|((GPU_WRITE_COLOR|((raster&FH_AR_WRITE)?GPU_WRITE_DEPTH:0))<<8);
    p->blend=GPU_BLEND_ADD|(GPU_BLEND_ADD<<8)|((raster&FH_AR_BLEND)?GPU_SRC_ALPHA:GPU_ONE)<<16|
        ((raster&FH_AR_BLEND)?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO)<<20|(GPU_ONE<<24);
    out->geometry=&entry->geometry;out->fog=m->actor_fog;out->environment_slot=m->environment_slot;
    out->environment_on=environment&&!diffuse;out->sphere=sphere!=NULL;out->second_uv=m->layered;
    out->mask_uv=sphere && m->masked;out->texture_units=units;
    out->key=(!!out->fog)|(out->environment_on<<1)|(out->environment_slot<<2)|(out->sphere<<5)|(!!out->second_uv<<6)|(out->mask_uv<<7);
    swizzle_draws+=swizzle;swizzle_missing+=m->swizzle && !swizzle;swizzle_diffuse_draws+=swizzle && diffuse;
    swizzle_environment_draws+=swizzle && m->environment;swizzle_masked_draws+=swizzle && m->masked;
    specular_bindings+=specular;specular_mask_bindings+=specular && m->masked;diffuse_bindings+=diffuse && environment;
    diffuse_mask_bindings+=diffuse && m->masked;actor_stream_draws++;bindings++;return 1;
}
void fh_pica_actor_stream_common(const FhPicaActorSelection *s){
    fh_pica_fog_bind(s->fog);fh_pica_scene_specular(s->sphere);
    fh_pica_scene_actor_second_uv(s->second_uv);fh_pica_environment_bind(s->environment_slot,s->environment_on);
    fh_pica_texture2_coord1(s->mask_uv);fh_pica_color_fade_bind(1);
}
