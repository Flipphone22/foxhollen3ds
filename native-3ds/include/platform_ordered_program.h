/* A synchronous native submission program for ordered two-pass materials.
 * Preparation uses the existing renderer as its lowering oracle. No source,
 * stack descriptor or texture lease escapes this call. Unlike pass grouping,
 * this preserves base/overlay adjacency even for overlapping triangles.
 *
 * Citro3D's pinned internal.h/effect.c define the copied POD effect block.
 * Texture descriptors are rebound through stable bound_gpu[] storage, and
 * TexEnv setters retain SDK dirty tracking. No raw GPU command is replayed.
 */
typedef struct {
    const FhDrawState *state;
    const FhMaterial *plan;
    VertexSink sink;
    C3D_Effect effect;
    C3D_TexEnv env[6];
    C3D_Tex texture[3];
    unsigned used[3],scissor[3],buffer,ready;
} OrderedProgram;
unsigned fh_ordered_program_recipe_bytes=2*sizeof(OrderedProgram),fh_ordered_program_command_peak;
static OrderedProgram *ordered_capture;

static void ordered_program_capture(OrderedProgram *out,const VertexSink *sink){
    C3D_Context *ctx=C3Di_GetContext();
    out->sink=*sink;out->effect=ctx->effect;out->buffer=ctx->texEnvBuf;
    memcpy(out->env,ctx->texEnv,sizeof(out->env));
    memcpy(out->scissor,ctx->scissor,sizeof(out->scissor));
    for(unsigned i=0;i<3;i++){
        out->used[i]=bound_used[i];out->texture[i]=bound_gpu[i];
        /* The other pass can upload/evict textures during its preparation. */
        if(sink->textures[i])sink->textures[i]->pinned=1;
    }
    out->ready=1;
}
static void ordered_program_release(OrderedProgram *program){
    if(program->ready)for(unsigned i=0;i<3;i++)
        if(program->sink.textures[i])program->sink.textures[i]->pinned=0;
}
static void ordered_program_bind(const OrderedProgram *program){
    C3D_Context *ctx=C3Di_GetContext();
    ctx->effect=program->effect;ctx->flags|=C3DiF_Effect;
    memcpy(ctx->scissor,program->scissor,sizeof(program->scissor));ctx->flags|=C3DiF_Scissor;
    for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,(C3D_TexEnv*)&program->env[i]);
    C3D_TexEnvBufUpdate(C3D_RGB,(program->buffer>>8)&15);
    C3D_TexEnvBufUpdate(C3D_Alpha,(program->buffer>>12)&15);
    for(unsigned i=0;i<3;i++){
        bound_used[i]=program->used[i];
        if(program->used[i]){bound_gpu[i]=program->texture[i];C3D_TexBind(i,&bound_gpu[i]);}
        else C3D_TexBind(i,NULL);
    }
    configure_affine(program->plan);
}
static int ordered_movie_program(const FhVertex *input,unsigned count,
    const FhDrawState *state,const FhMaterial *base,
    const FhDrawState *overlay,const FhMaterial *top){
    /* These routes have additional uniforms, sources or primitive semantics.
     * Keep them on the existing path until independently lowered. */
    if(ordered_capture || compiled_mesh || native_replay || point_context.active ||
       state->raw_position>1 || base->sphere_lighting || base->cpu_fragment ||
       top->sphere_lighting || top->cpu_fragment || base->movie_layer || top->movie_layer)
        return 0;
    for(unsigned first=0;first<count;){
        /* Six pairs fit the existing 64 KiB command reservation, including
         * a full shader/uniform rebind after a stream fence. Never fence or
         * reuse geometry between preparation and the final ordered draw. */
        unsigned n=count-first;if(n>18)n=18;
        service_audio();reserve_commands();flush_draw();
        if(vertex_count+2*n>VERTICES || index_count+2*n>VERTICES)fh_renderer_sync();
        OrderedProgram pair[2]={{.state=state,.plan=base},{.state=overlay,.plan=top}};
        for(unsigned p=0;p<2;p++){
            ordered_capture=&pair[p];
            render_direct_coordinates(input+first,n,pair[p].state,pair[p].plan,NULL);
            ordered_capture=NULL;
        }
        if(!pair[0].ready || !pair[1].ready){
            ordered_program_release(&pair[0]);ordered_program_release(&pair[1]);
            if(first)fh_platform_panic("Ordered material changed admission inside a batch");
            return 0;
        }
        /* Lazy EFB texture preparation may have consumed stream capacity. */
        if(vertex_count+2*n>VERTICES || index_count+2*n>VERTICES){
            ordered_program_release(&pair[0]);ordered_program_release(&pair[1]);
            fh_renderer_sync();continue;
        }
        unsigned prepared_barriers=barriers;
        reserve_commands();
        if(barriers!=prepared_barriers){
            ordered_program_release(&pair[0]);ordered_program_release(&pair[1]);continue;
        }
        u64 submission_started=svcGetSystemTick();
        u32 command_start=0;GPUCMD_GetBuffer(NULL,NULL,&command_start);
        unsigned start[2];
        for(unsigned p=0;p<2;p++){
            start[p]=index_count;
            emit_vertices(input+first,n,&pair[p].sink);
        }
#ifdef FH_ORDERED_PROGRAM_NEGATIVE
        for(unsigned p=0;p<2;p++)for(unsigned tri=0;tri<n;tri+=3){
#else
        for(unsigned tri=0;tri<n;tri+=3)for(unsigned p=0;p<2;p++){
#endif
            ordered_program_bind(&pair[p]);
            C3D_DrawElements(GPU_TRIANGLES,3,C3D_UNSIGNED_SHORT,indices+start[p]+tri);
            draws++;total_vertices+=3;
        }
        /* The SDK now owns copied register commands, and the stream remains
         * append-only until its existing GPU fence. Restore software cache
         * validity through normal preparation on the next caller. */
        combiner_valid=raster_valid=0;
        u32 command_end=0;GPUCMD_GetBuffer(NULL,NULL,&command_end);
        unsigned command_bytes=(command_end-command_start)*4;
        if(command_bytes>fh_ordered_program_command_peak)fh_ordered_program_command_peak=command_bytes;
        if(command_bytes>64*1024)fh_platform_panic("Ordered program command reservation exceeded");
        submit_ticks+=svcGetSystemTick()-submission_started;
        ordered_program_release(&pair[0]);ordered_program_release(&pair[1]);
        fh_ordered_program_chunks++;fh_ordered_program_triangles+=n/3;first+=n;
    }
    return 1;
}
