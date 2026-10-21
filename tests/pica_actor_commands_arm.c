/* Reuse the actual pinned SDK context/serializer fixture, not its test loop. */
#define test_main previous_world_test_main
#include "pica_draw_owner_arm.c"
#undef test_main
#include "fh_pica_actor_command_owner.h"
#include "fh_pica_model.h"
static uint32_t actor_words[2048],arena_storage[8192],prior_regs[1024],expected_regs[1024];
static uint32_t prior_uniforms[2][384],expected_uniforms[2][384];
static float joints[24*16],tails[25*3],world[12];
static uint16_t bones[22];
static C3D_Tex actor_textures[3];
static C3D_TexEnv actor_stages[2][4];
static void setrow(unsigned row,const uint32_t *words){
    float f[4];memcpy(f,words,16);C3D_FVUnifSet(GPU_VERTEX_SHADER,row,f[3],f[2],f[1],f[0]);
}
static void reference_actor(const FhPicaDrawDescription *description,unsigned material,
    const FhPicaActorParameters *p,const FhPicaActorPose *pose){
    C3D_Context *ctx=&__C3D_Context;
    C3D_SetAttrInfo(&description->attributes);C3D_SetBufInfo(&description->buffers);
    if(p->offsets)for(unsigned i=0;i<12;i++)ctx->bufInfo.buffers[i].offset=p->offsets[i];
    ctx->flags|=C3DiF_BufInfo;
    C3D_CullFace(description->effect.cullMode);
    C3D_DepthTest(p->depth_test&1,(p->depth_test>>4)&7,(p->depth_test>>8)&31);
    C3D_AlphaTest(p->alpha_test&1,(p->alpha_test>>4)&7,(p->alpha_test>>8)&255);
    C3D_AlphaBlend(p->blend&255,(p->blend>>8)&255,(p->blend>>16)&15,(p->blend>>20)&15,(p->blend>>24)&15,(p->blend>>28)&15);
    for(unsigned i=0;i<3;i++)ctx->tex[i]=(p->texture_config&(1u<<i))?actor_textures+i:NULL;
    ctx->texConfig=p->texture_config;ctx->texShadow=p->texture_shadow;
    ctx->texEnvBuf=(p->environment_buffer&~0xff00u)|(p->material->buffer_mask<<8);
    ctx->texEnvBufClr=p->environment_color;ctx->fogClr=p->fog_color;
    ctx->flags|=C3DiF_TexAll|C3DiF_TexStatus|C3DiF_TexEnvBuf;
    for(unsigned i=0;i<4;i++){C3D_SetTexEnv(i,actor_stages[material]+i);C3D_TexEnvColor(C3D_GetTexEnv(i),p->colors[i]);}
    for(unsigned i=0;i<3;i++)setrow(7+i,p->uniforms+i*4);
    setrow(76,p->scale);
    for(unsigned i=0;i<pose->count;i++){
        unsigned bone=pose->bones[i];float rows[3][4];
        CHECK(fh_pica_model_pose(rows,bone<pose->joint_count?pose->joints+16*bone:pose->world,pose->tails+3*bone));
        for(unsigned j=0;j<3;j++)C3D_FVUnifSet(GPU_VERTEX_SHADER,10+i*3+j,rows[j][0],rows[j][1],rows[j][2],rows[j][3]);
    }
    C3D_DrawElements(GPU_TRIANGLES,description->index_count,C3D_UNSIGNED_SHORT,(void*)description->index_address);drain();
}
void test_main(void){
    setup_sdk();FhPicaStreamArena arena;CHECK(fh_pica_stream_arena_init(&arena,arena_storage,8192));
    FhPicaActorGeometry geometry[2];FhPicaDrawDescription description[2]={0};
    FhPicaActorMaterial materials[2];FhPicaSamplerCommands samplers[3];
    for(unsigned i=0;i<2;i++){
        description[i].attributes=__C3D_Context.attrInfo;description[i].buffers=__C3D_Context.bufInfo;
        description[i].buffers.buffers[0].offset+=0x2000*i;
        description[i].effect.cullMode=i?GPU_CULL_FRONT_CCW:GPU_CULL_NONE;
        description[i].index_address=0x18020000+0x100*i;description[i].index_count=3+i*3;
        for(unsigned j=0;j<4;j++){
            C3D_TexEnvInit(actor_stages[i]+j);actor_stages[i][j].srcRgb=i?0x300:0x100;
            description[i].stages[j]=actor_stages[i][j];
        }
        CHECK(fh_pica_actor_geometry_compile(&arena,geometry+i,description+i));
        fh_pica_actor_material_compile(materials+i,actor_stages[i],i?0x55:0xaa);
    }
    for(unsigned i=0;i<3;i++){
        actor_textures[i]=(C3D_Tex){0};actor_textures[i].data=(void*)(0x18500000+0x10000*i);
        actor_textures[i].width=64;actor_textures[i].height=64;actor_textures[i].fmt=GPU_RGBA8;
        FhPicaStreamSampler input={0,actor_textures[i].dim,0,0,(uint32_t)actor_textures[i].data,GPU_RGBA8};
        /* All three descriptors intentionally use unit0 encoding. */
        CHECK(fh_pica_stream_sampler(samplers+i,0,&input));
    }
    for(unsigned i=0;i<24*16;i++)joints[i]=(float)((int)(i%19)-9)*.125f;
    for(unsigned i=0;i<25*3;i++)tails[i]=(float)((int)(i%7)-3)*.0625f;
    for(unsigned i=0;i<12;i++)world[i]=(float)i*.25f;
    for(unsigned i=0;i<22;i++)bones[i]=(i*7)%24;
    bones[21]=24; /* explicit world slot */
    FhPicaActorPose pose={joints,world,tails,bones,24,22,0};
    FhPicaActorParameters parameters={0};FhPicaActorCommandState state={0};
    for(unsigned i=0;i<3;i++)parameters.samplers[i]=samplers+i;
    for(unsigned i=0;i<12;i++)parameters.uniforms[i]=0x3f000000+i;
    for(unsigned i=0;i<4;i++)parameters.scale[i]=0x3f800000;
    uint32_t offsets[12];memcpy(offsets,geometry[1].offsets,sizeof offsets);
    for(unsigned step=0;step<128;step++){
        unsigned index=(step/5)&1,material=(step/7)&1;
        if(!(step%11))fh_pica_actor_command_reset(&state);
        parameters.material=materials+material;parameters.texture_config=step%8;
        parameters.offsets=step%3?NULL:offsets;
        parameters.alpha_test=(step&1)|(GPU_GREATER<<4)|((step&255)<<8);
        parameters.depth_test=1|(GPU_GEQUAL<<4)|((step%3?GPU_WRITE_ALL:GPU_WRITE_COLOR)<<8);
        parameters.blend=GPU_BLEND_ADD|(GPU_BLEND_ADD<<8)|(GPU_SRC_ALPHA<<16)|(GPU_ONE_MINUS_SRC_ALPHA<<20)|(GPU_ONE<<24);
        for(unsigned i=0;i<4;i++)parameters.colors[i]=0xff102030+i+step;
        pose.changed=step%4?1u<<(step%22):0x3fffff;
        for(unsigned i=0;i<22;i++)if(pose.changed&(1u<<i)){
            unsigned b=bones[i];float *m=b<24?joints+16*b:world;m[3]+=.03125f;m[7]-=.015625f;
        }
        memcpy(prior_regs,registers,sizeof registers);memcpy(prior_uniforms,uniforms,sizeof uniforms);
        reference_actor(description+index,material,&parameters,&pose);
        memcpy(expected_regs,registers,sizeof registers);memcpy(expected_uniforms,uniforms,sizeof uniforms);
        memcpy(registers,prior_regs,sizeof registers);memcpy(uniforms,prior_uniforms,sizeof uniforms);
        unsigned cursor=2;actor_words[0]=0x12345678;actor_words[1]=0xabcdef01;
        FhPicaActorCommandState old=state;
        CHECK(!fh_pica_actor_command_append(actor_words,3,&cursor,&state,geometry+index,&parameters,&pose,0));
        CHECK(cursor==2 && !memcmp(&old,&state,sizeof old) && actor_words[0]==0x12345678 && actor_words[1]==0xabcdef01);
        unsigned written=fh_pica_actor_command_append(actor_words,2048,&cursor,&state,geometry+index,&parameters,&pose,0);
        CHECK(written && written==cursor-2);execute(actor_words+2,written);
        for(unsigned i=0;i<1024;i++)if(!transient(i))CHECK(registers[i]==expected_regs[i]);
        CHECK(!memcmp(uniforms,expected_uniforms,sizeof uniforms));
        pose.changed=0;cursor=0;
        CHECK(fh_pica_actor_command_append(actor_words,30,&cursor,&state,geometry+index,&parameters,&pose,0)==30);
        CHECK(cursor==30);execute(actor_words,30);
        reset_commands();
    }
    /* Actual SDK handoff restores all and only the raw writer's dirty rows. */
    C3Di_UpdateContext();drain();snapshot();
    FhPicaStreamState world_state;
    CHECK(fh_pica_actor_stream_begin(&state));CHECK(!fh_pica_stream_begin(&world_state));
    CHECK(!fh_pica_actor_stream_begin(&state));
    FhPicaActorParameters snapshot_parameters=parameters;CHECK(fh_pica_actor_stream_environment(&snapshot_parameters));
    CHECK(snapshot_parameters.environment_color==__C3D_Context.texEnvBufClr);
    parameters.colors[0]^=0x00343434;joints[3]+=10;pose.changed=0;
    unsigned cursor=gpuCmdBufOffset,size=gpuCmdBufSize;gpuCmdBufSize=cursor+1;
    CHECK(!fh_pica_actor_stream_draw(geometry,&parameters,&pose,0));CHECK(gpuCmdBufOffset==cursor);
    gpuCmdBufSize=size;__C3D_Context.flags&=~C3DiF_DrawUsed;
    CHECK(fh_pica_actor_stream_draw(geometry,&parameters,&pose,0));
    CHECK(__C3D_Context.flags&C3DiF_DrawUsed);drain();
    CHECK(state.uniform_dirty[2]&(1u<<12));CHECK(state.uniform_dirty[2]&0xfff); /* rows64..76 */
    fh_pica_actor_stream_end();CHECK(!fh_pica_actor_stream_active());
    C3Di_UpdateContext();drain();verify_restored();
    CHECK(fh_pica_actor_stream_begin(&state));CHECK(fh_pica_actor_stream_draw(geometry,&parameters,&pose,0));
    C3D_FrameSplit(GX_CMDLIST_FLUSH);CHECK(!fh_pica_actor_stream_active());
    C3Di_UpdateContext();drain();verify_restored();
    CHECK(fh_pica_stream_begin(&world_state));CHECK(!fh_pica_actor_stream_begin(&state));fh_pica_stream_end();
    float24Uniform_s constant={.id=74};vshader.numFloat24Uniforms=1;vshader.float24Uniforms=&constant;
    CHECK(!fh_pica_actor_stream_begin(&state));vshader.numFloat24Uniforms=0;vshader.float24Uniforms=NULL;
    CHECK(fh_pica_actor_stream_begin(&state));C3Di_AptEventHook(APTHOOK_ONRESTORE,NULL);CHECK(!fh_pica_actor_stream_active());
    C3Di_UpdateContext();drain();
    /* No stale SDK palette upload before native publication, and no loss of
     * pending SDK values when an admitted instance ends without drawing. */
    uint32_t pending[4]={0x3fc00000,0x40200000,0x40600000,0x40900000};
    memcpy(prior_uniforms,uniforms,sizeof uniforms);
    unsigned prior_offset=registers[GPUREG_ATTRIBBUFFER0_OFFSET];
    __C3D_Context.bufInfo.buffers[0].offset=prior_offset+0x8000;
    __C3D_Context.flags|=C3DiF_BufInfo;
    for(unsigned i=7;i<=76;i++)setrow(i,pending);
    setrow(0,pending);CHECK(fh_pica_actor_stream_begin(&state));drain();
    CHECK(registers[GPUREG_ATTRIBBUFFER0_OFFSET]==prior_offset);
    CHECK(!memcmp(uniforms[0],pending,sizeof pending));
    for(unsigned i=7;i<=76;i++)CHECK(!memcmp(uniforms[0]+4*i,prior_uniforms[0]+4*i,16));
    fh_pica_actor_stream_end();C3Di_UpdateContext();drain();
    CHECK(registers[GPUREG_ATTRIBBUFFER0_OFFSET]==prior_offset+0x8000);
    for(unsigned i=7;i<=76;i++)CHECK(!memcmp(uniforms[0]+4*i,pending,sizeof pending));
    /* Live instance boundaries keep immutable native state, while publishing
     * camera/light changes and retaining unrelated pending SDK palette values. */
    CHECK(fh_pica_actor_stream_begin(&state));
    pose.changed=0x3fffff;CHECK(fh_pica_actor_stream_draw(geometry,&parameters,&pose,0));drain();
    for(unsigned step=0;step<64;step++){
        reset_commands(); /* Fixture has consumed every word; GPU queue is mocked. */
        FhPicaActorCommandState saved=state;
        memcpy(prior_uniforms,uniforms,sizeof uniforms);
        unsigned old_offset=registers[GPUREG_ATTRIBBUFFER0_OFFSET];
        pending[0]+=17;setrow(0,pending);setrow(78,pending);setrow(10,pending);
        __C3D_Context.bufInfo.buffers[0].offset=old_offset+0x1000+step*16;
        __C3D_Context.flags|=C3DiF_BufInfo;
        FhPicaActorCommandState foreign={0};CHECK(!fh_pica_actor_stream_sync(&foreign));
        CHECK(fh_pica_actor_stream_sync(&state));drain();
        CHECK(fh_pica_actor_stream_active() && !memcmp(&saved,&state,sizeof state));
        CHECK(!memcmp(uniforms[0],pending,16));CHECK(!memcmp(uniforms[0]+78*4,pending,16));
        CHECK(!memcmp(uniforms[0]+10*4,prior_uniforms[0]+10*4,16));
        CHECK(registers[GPUREG_ATTRIBBUFFER0_OFFSET]==old_offset);
        pose.changed=0;
        CHECK(fh_pica_actor_stream_draw(geometry,&parameters,&pose,0)==30);drain();
        /* A new actor still writes its own pose even with identical material. */
        joints[3]+=.0625f;pose.changed=0x3fffff;
        CHECK(fh_pica_actor_stream_draw(geometry,&parameters,&pose,0)>30);drain();
        for(unsigned i=0;i<22;i++){
            unsigned bone=bones[i];float rows[3][4];
            CHECK(fh_pica_model_pose(rows,bone<24?joints+16*bone:world,tails+bone*3));
            for(unsigned row=0;row<3;row++)for(unsigned c=0;c<4;c++){
                uint32_t bits;memcpy(&bits,&rows[row][3-c],4);
                CHECK(uniforms[0][(10+i*3+row)*4+c]==bits);
            }
        }
    }
    /* Shared effect changes can overwrite raw raster, so the next actor must
     * restore it even when its material compares equal to the cached one. */
    C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    CHECK(fh_pica_actor_stream_sync(&state));drain();pose.changed=0;
    CHECK(fh_pica_actor_stream_draw(geometry,&parameters,&pose,0)==42);drain();
    CHECK(registers[(GPUREG_FRAGOP_ALPHA_TEST+3)]==parameters.depth_test);
    /* A program boundary retires state; a subsequent SDK consumer restores its
     * pending palette/geometry rather than inheriting the previous actor. */
    __C3D_Context.flags|=C3DiF_Program;
    CHECK(fh_pica_actor_stream_sync(&state));CHECK(!state.valid);
    CHECK(fh_pica_actor_stream_draw(geometry,&parameters,&pose,0));drain();
    fh_pica_actor_stream_end();C3Di_UpdateContext();drain();
    CHECK(!memcmp(uniforms[0]+10*4,pending,16));
    CHECK(fh_pica_actor_stream_sync(&state));C3D_Fini();CHECK(!fh_pica_actor_stream_active());
    test_pass();
}
