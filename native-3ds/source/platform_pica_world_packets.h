/* The inline transport CPU-copies these templates into SDK command memory.
 * Only that destination is GPU-visible; platform_gpu_cache.c publishes every
 * submitted segment. Do not flush CPU-only templates at load or live update.
 * Storage/lifetimes and dirty-word accounting are otherwise unchanged. */
/* Included after the ordinary material selector. Prepared commands borrow
 * exactly the material/geometry lifetime; release follows the existing fence.
 * No geometry conversion, allocation or grouping in the frame loop. */
enum { WORLD_PACKET_LINEAR_LIMIT=128*1024, WORLD_PACKET_CPU_LIMIT=24*1024 };
struct FhWorldPacketGroup {
    FhPicaWorldPacket packet;
    uint64_t used_epoch;
    unsigned allocated,cpu_allocated;
    FhPicaWorldPacketEntry entries[];
};
static unsigned packet_linear,packet_linear_peak,packet_cpu,packet_cpu_peak;
static unsigned packet_groups,packet_calls,packet_draws,packet_changed,packet_unchanged;
static unsigned packet_epoch_reject,packet_live_reject,packet_call_reject;
static unsigned packet_candidates,packet_single_runs,packet_unready,packet_selector_reject,packet_budget_reject;
static unsigned packet_prepared_draws,packet_linear_saved,packet_cpu_saved;
static void world_packets_release(FhPicaWorldCommands *stream){
    for(unsigned i=0;i<stream->count;i++){
        FhWorldPacketGroup *g=stream->draw[i].group;if(!g)continue;
        packet_linear-=g->allocated;packet_cpu-=g->cpu_allocated;
        linearFree(g);stream->draw[i].group=NULL;
    }
}
static void world_packets_prepare(const FhPicaWorldPlan *plan,FhPicaMaterialSet *set){
    Material *m=set->items;FhPicaWorldCommands *stream=m->world_commands;
    for(unsigned first=0;first<stream->count;){
        if(!stream->draw[first].ready){packet_unready++;first++;continue;}
        unsigned count=1;
        while(count<4 && first+count<stream->count &&
              stream->draw[first+count].ready &&
              plan->draw[first+count].format==plan->draw[first].format)count++;
        if(count<2){packet_single_runs++;first+=count;continue;}
        packet_candidates++;
        FhPicaDrawLive live[FH_PICA_WORLD_PACKET_DRAWS]={0};
        const FhPicaResidentDraw *draw[FH_PICA_WORLD_PACKET_DRAWS];
        unsigned good=1;
        for(unsigned k=0;k<count;k++){
            FhPicaWorldSelection select;
            if(world_stream_select(set,first+k,live+k,&select,0)!=1){good=0;break;}
            draw[k]=select.draw;live[k].texture_config=select.draw->texture_units;
            live[k].environment_buffer=(select.buffer_mask&255u)<<8;
        }
        if(!good){packet_selector_reject++;first+=count;continue;}
        /* Load-only scratch: compile once, then allocate the exact published
         * command extent and count-sized persistent metadata. Never per-frame. */
        uint32_t prepared[FH_PICA_WORLD_PACKET_WORDS] __attribute__((aligned(16)));
        FhPicaWorldPacketEntry entries[FH_PICA_WORLD_PACKET_DRAWS];
        FhPicaWorldPacket packet={.entry=entries,.entry_capacity=FH_PICA_WORLD_PACKET_DRAWS};
        if(!fh_pica_world_packet_prepare(&packet,prepared,FH_PICA_WORLD_PACKET_WORDS,draw,live,count))
            fh_platform_panic("PICA world packet load compilation");
        unsigned command_bytes=packet.words*4,cpu_bytes=sizeof(FhWorldPacketGroup)+count*sizeof(*entries);
        unsigned metadata_extent=(cpu_bytes+15u)&~15u,bytes=metadata_extent+command_bytes;
        unsigned free_bytes=linearSpaceFree();extern u32 __ctru_linear_heap_size;
        if(bytes>WORLD_PACKET_LINEAR_LIMIT-packet_linear ||
           cpu_bytes>WORLD_PACKET_CPU_LIMIT-packet_cpu || free_bytes<bytes ||
           (u64)(free_bytes-bytes)*100<(u64)__ctru_linear_heap_size*15){packet_budget_reject++;break;}
        /* One CPU-owned allocation: both template and its compact patch offsets.
         * The combined extent counts against the linear limit and 15% reserve.
         * No GPU reads this metadata; the inline owner publishes SDK copies. */
        FhWorldPacketGroup *group=linearAlloc(bytes);if(!group)break;
        uint32_t *commands=(uint32_t*)((unsigned char*)group+metadata_extent);
        memcpy(commands,prepared,command_bytes);memcpy(group->entries,entries,count*sizeof(*entries));
        group->packet=packet;group->packet.commands=commands;
        group->packet.entry=group->entries;group->packet.entry_capacity=count;
        group->allocated=bytes;group->cpu_allocated=cpu_bytes;group->used_epoch=UINT64_MAX;
        packet_prepared_draws+=count;packet_linear_saved+=(count*184+4)*4-command_bytes;
        packet_cpu_saved+=(FH_PICA_WORLD_PACKET_DRAWS-count)*sizeof(*entries);

        stream->draw[first].group=group;
        packet_linear+=bytes;packet_cpu+=cpu_bytes;packet_groups++;
        if(packet_linear>packet_linear_peak)packet_linear_peak=packet_linear;
        if(packet_cpu>packet_cpu_peak)packet_cpu_peak=packet_cpu;
        first+=count;
    }
}
static FhWorldPacketGroup *world_packet_group(const FhPicaMaterialSet *set,unsigned index){
    if(!set || !set->items || !set->count)return NULL;
    const Material *m=set->items;FhPicaWorldCommands *s=m->world_commands;
    return s && index<s->count?s->draw[index].group:NULL;
}
unsigned fh_pica_world_packet_count(const FhPicaMaterialSet *set,unsigned index){
    FhWorldPacketGroup *g=world_packet_group(set,index);
    if(!g)return 0;
    if(g->used_epoch==fh_pica_packet_epoch()){packet_epoch_reject++;return 0;}
    return g->packet.count;
}
unsigned fh_pica_world_packet_submit(const FhPicaMaterialSet *set,unsigned index,
                                    const FhPicaDrawLive *environment,unsigned fog){
    FhWorldPacketGroup *g=world_packet_group(set,index);
    if(!g || !environment || g->used_epoch==fh_pica_packet_epoch())return 0;
    FhPicaDrawLive live[FH_PICA_WORLD_PACKET_DRAWS];
    /* The scene has selected and merged entry zero before calling us. No
     * material mutation occurs between that selection and this synchronous
     * submission. Validate every remaining member before updating the packet. */
    live[0]=*environment;
    for(unsigned k=1;k<g->packet.count;k++){
        FhPicaWorldSelection select;live[k]=*environment;
        if(world_stream_select(set,index+k,live+k,&select,0)!=1 || select.fog!=fog){
            packet_live_reject++;return 0;
        }
        live[k].texture_config=(environment->texture_config&~7u)|select.draw->texture_units;
        live[k].environment_buffer=(environment->environment_buffer&~0xff00u)|((select.buffer_mask&255u)<<8);
    }
    FhPicaWorldPacketDirty dirty;
    if(!fh_pica_world_packet_update(&g->packet,live,&dirty))fh_platform_panic("PICA world packet live state");
    if(dirty.writes){

        packet_changed+=dirty.writes;
    }else packet_unchanged++;
    if(!fh_pica_stream_packet(&g->packet)){packet_call_reject++;return 0;}
    g->used_epoch=fh_pica_packet_epoch();packet_calls++;packet_draws+=g->packet.count;
    /* The scene already accounted the first selector before entering here. */
    world_stream_draws+=g->packet.count-1;bindings+=g->packet.count-1;
    return g->packet.count;
}
static void world_packets_report(void){
    char line[320];snprintf(line,sizeof line,
        "PICA world packets: groups=%u calls=%u draws=%u changed=%u unchanged=%u epoch_reject=%u live_reject=%u call_reject=%u linear=%u linear_peak=%u cpu=%u cpu_peak=%u metadata_in_linear=1\n",
        packet_groups,packet_calls,packet_draws,packet_changed,packet_unchanged,
        packet_epoch_reject,packet_live_reject,packet_call_reject,packet_linear,packet_linear_peak,packet_cpu,packet_cpu_peak);
    fh_platform_log(line);
    snprintf(line,sizeof line,"PICA world packet admission: candidates=%u singles=%u unready=%u selector_reject=%u budget_reject=%u prepared_draws=%u linear_saved=%u cpu_saved=%u\n",
        packet_candidates,packet_single_runs,packet_unready,packet_selector_reject,packet_budget_reject,packet_prepared_draws,packet_linear_saved,packet_cpu_saved);fh_platform_log(line);
    if(packet_linear || packet_cpu)fh_platform_panic("PICA world packet lifetime leak");
}
