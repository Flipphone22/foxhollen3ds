/* Included inside platform_graphics.c to share its bounded stream, texture
 * ownership, raster lowering and GPU fences. No separate GPU arena or queue. */
#include "fh_native_model_draw.h"
#include "native_model_shader.h"
static const FhNativeModelDraw *model_draw;
static unsigned model_draw_accepted;
static DVLB_s *model_shader_binary;
static shaderProgram_s model_program;
unsigned fh_native_model_draws,fh_native_model_lists,fh_native_model_vertices_drawn,fh_native_model_failures;

static void native_model_emit(const FhDrawState *state,const FhMaterial *plan,Texture *bound[3]){
    const FhNativeModelDraw *d=model_draw;const FhModelGroup *g=&d->group;
    unsigned persistent=d->persistent_vertex!=NULL;
    unsigned slots=persistent?0:(g->vertices*sizeof(FhModelVertex)+sizeof(GpuVertex)-1)/sizeof(GpuVertex);
    if(!persistent && (vertex_count+slots>VERTICES || index_count+g->indices>VERTICES))return;
    flush_draw();FhModelVertex *output=(FhModelVertex*)(vertices+vertex_count);
    const FhModelVertex *bound_vertices=persistent?d->persistent_vertex:output;
    if(!persistent && !fh_native_model_vertices(output,g->vertices,g->refs,g->vertices,&d->source,g->bone_ids,g->bones))return;
    for(unsigned i=0;!persistent && i<g->vertices;i++){
        float color[2][4]={{0},{0}};
        for(unsigned c=0;c<4;c++)color[0][c]=d->color[c]*(d->color_from_vertex[c]?output[i].color[c]:1.f);
        if(plan->pipeline.count){
            for(unsigned c=0;c<4;c++)output[i].color[c]=plan->pipeline.primary[c]<0?0:((float*)color)[plan->pipeline.primary[c]];
        }else memcpy(output[i].color,color[0],sizeof(output[i].color));
    }
    const uint16_t *bound_indices=d->persistent_index;
    if(!persistent){
        memcpy(indices+index_count,g->index,g->indices*sizeof(*indices));
        bound_indices=indices+index_count;index_count+=g->indices;vertex_count+=slots;
    }
    /* Same append-only flushed stream as ordinary/compiled draws. Frame sync
     * above joins prior users before any wrap; temporary poses are copied into
     * uniforms synchronously, and no game-owned pointer reaches the GPU. */
    flush_stream_buffers();
    C3D_BindProgram(&model_program);fh_uniform_rows_reset(&uniform_rows);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);
    AttrInfo_AddLoader(a,2,GPU_FLOAT,2);AttrInfo_AddLoader(a,3,GPU_FLOAT,3);AttrInfo_AddLoader(a,4,GPU_FLOAT,3);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,bound_vertices,sizeof(*output),5,0x43210)<0)fh_platform_panic("Native model vertex binding failed");
    float alpha=persistent?d->color[3]:1;
    if(persistent && plan->pipeline.count && plan->pipeline.primary[3]<0)alpha=0;
    uniform_row(94,persistent?d->color[0]:1,persistent?d->color[1]:1,persistent?d->color[2]:1,alpha);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x70,!persistent || d->color_from_vertex[0]);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x71,!persistent || d->color_from_vertex[3]);
    for(unsigned bone=0;bone<g->bones;bone++)for(unsigned r=0;r<3;r++){
        const float *v=d->palette[bone][r];uniform_row(bone*3+r,v[0],v[1],v[2],v[3]);
    }
    float projection[4][4];fh_vertex_pica_projection(projection,state);
    for(unsigned r=0;r<4;r++)uniform_row(66+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    for(unsigned r=0;r<3;r++)uniform_row(70+r,d->post[r][0],d->post[r][1],d->post[r][2],d->post[r][3]);
    unsigned units=plan->pipeline.count?plan->pipeline.textures:plan->map>=0?1:0;
    float scale[8]={1,1,1,1,1,1,d->normal_scale,0};
    for(unsigned unit=0;unit<3;unit++){
        int normal=0;
        if(unit<units){
            int coord=plan->pipeline.count?plan->pipeline.coord[unit]:plan->coord;
            normal=d->coordinates[coord]==2;
            scale[unit*2]=(float)bound[unit]->source.width/bound[unit]->gpu.width;
            scale[unit*2+1]=(float)bound[unit]->source.height/bound[unit]->gpu.height;
        }
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68+4+unit,normal);
    }
    uniform_row(73,scale[0],scale[1],scale[2],scale[3]);uniform_row(74,scale[4],scale[5],scale[6],0);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x72,d->direct_uv!=0);
    if(d->direct_uv)for(unsigned unit=0;unit<units;unit++){
        unsigned coord=plan->pipeline.coord[unit];const float *v=d->uv_transform[coord];
        uniform_row(89+unit,v[0],v[1],v[2],v[3]);
    }
    uniform_row(75,d->lighting.ambient[0],d->lighting.ambient[1],d->lighting.ambient[2],1);
    for(unsigned i=0;i<2;i++)for(unsigned r=0;r<5;r++){
        const float *v=d->lighting.lights[i][r];uniform_row(76+i*5+r,v[0],v[1],v[2],v[3]);
    }
    for(unsigned r=0;r<3;r++)uniform_row(86+r,d->lighting.normal[r][0],d->lighting.normal[r][1],d->lighting.normal[r][2],d->lighting.lights[1][r][3]);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x6f,!plan->pipeline.count);
    if(!plan->pipeline.count){
        float bias[4];
        for(unsigned r=0;r<4;r++){
            float scale=1.f/plan->scale[r==3];const float *v=plan->color[r];
            uniform_row(89+r,v[1]*scale,v[2]*scale,v[3]*scale,v[4]*scale);bias[r]=v[0]*scale;
        }
        uniform_row(93,bias[0],bias[1],bias[2],bias[3]);
    }
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,d->lit!=0);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x69,(d->lighting.mask&1)!=0);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x6a,(d->lighting.mask&2)!=0);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x6b,(d->lighting.mask&FH_LIGHT_DIFFUSE_FILL)!=0);
    C3D_DrawElements(GPU_TRIANGLES,g->indices,C3D_UNSIGNED_SHORT,bound_indices);
    draws++;total_vertices+=g->indices;fh_native_model_draws++;fh_native_model_lists+=g->draws;fh_native_model_vertices_drawn+=g->vertices;
    fh_vertex_unique+=g->vertices;fh_vertex_reused+=g->indices>g->vertices?g->indices-g->vertices:0;
    model_draw_accepted=1;
    /* Invalidate all ordinary shader uniform/attribute assumptions. Texture,
     * combiner and raster state are subsequently restored by their usual path. */
    bind_geometry();
}
int fh_renderer_native_model_draw(const FhNativeModelDraw *d){
    if(!d || model_draw || compiled_mesh || native_replay || point_context.active ||
       !d->group.vertices || d->group.vertices>FH_MODEL_VERTICES || !d->group.indices || d->group.indices>FH_MODEL_INDICES){fh_native_model_admission[8]++;return 0;}
    if((d->persistent_vertex!=NULL)!=(d->persistent_index!=NULL)){fh_native_model_admission[8]++;return 0;}
    if(d->persistent_vertex && (d->color_from_vertex[0]!=d->color_from_vertex[1] ||
        d->color_from_vertex[0]!=d->color_from_vertex[2] ||
        !osConvertVirtToPhys(d->persistent_vertex) || !osConvertVirtToPhys(d->persistent_index))){fh_native_model_admission[8]++;return 0;}
    const FhMaterial *found=material(&d->state,NULL,0,1);
    if(!found || found->cpu_fragment || found->sphere_lighting || found->movie_layer){fh_native_model_admission[9]++;return 0;}
    FhMaterial plan=*found;
    if(d->direct_uv && !plan.pipeline.count){fh_native_model_admission[9]++;return 0;}
    unsigned units=plan.pipeline.count?plan.pipeline.textures:plan.map>=0?1:0;
    if(units>3){fh_native_model_admission[9]++;return 0;}
    for(unsigned i=0;i<units;i++){
        int coord=plan.pipeline.count?plan.pipeline.coord[i]:plan.coord;
        if(coord<0 || coord>=8 || !d->coordinates[coord]){fh_native_model_admission[10]++;return 0;}
    }
    /* Unlit affine primary colors can be lowered once per unique vertex.
     * Lit inputs need an identity primary mapping after GPU lighting. */
    if(!plan.pipeline.count){
        if(!affine_supported(&plan)){fh_native_model_admission[11]++;return 0;}
        for(unsigned c=0;c<4;c++)for(unsigned j=5;j<9;j++)if(plan.color[c][j]!=0){fh_native_model_admission[11]++;return 0;}
    }else if((d->lit || d->persistent_vertex) && !gpu_lighting_supported(&plan)){fh_native_model_admission[11]++;return 0;}
    if(!model_shader_binary){
        model_shader_binary=DVLB_ParseFile((u32*)native_model_shader,sizeof(native_model_shader));
        if(!model_shader_binary)fh_platform_panic("Native model shader parse failed");
        shaderProgramInit(&model_program);shaderProgramSetVsh(&model_program,&model_shader_binary->DVLE[0]);
    }
    service_audio();begin();reserve_commands();
    if(!d->persistent_vertex && index_count+d->group.indices>VERTICES)fh_renderer_sync();
    model_draw=d;model_draw_accepted=0;
    render_direct(NULL,d->group.indices,&d->state,&plan);
    model_draw=NULL;
    if(!model_draw_accepted)fh_native_model_failures++;
    return model_draw_accepted;
}
