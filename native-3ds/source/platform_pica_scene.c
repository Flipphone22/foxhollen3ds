#include "fh_pica_world_stream.h"
#include "fh_pica_actor_stream.h"
#include "fh_pica_shadow_project.h"
#include "fh_pica_shadow_image.h"
/* Ground-up world geometry stage. No GX interpreter, material resolver,
 * prepared draw copies, texture comparisons or per-range index gathering.
 * Fences/publication remain with the verified host (pinned Melee ownership).
 * Full specular and effect parity remain incomplete; see RENDERING_ACCEPTANCE.
 */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include "fh_pica_scene.h"
#include "fh_pica_receiver_plane.h"
static unsigned receiver_rejected;
#include "fh_graphics_options.h"
#include "fh_pica_hud.h"
#include "fh_pica_hologram.h"
#include "fh_pica_stars.h"
#include "fh_pica_glow.h"
#include "fh_pica_splash.h"
#include "fh_pica_drops.h"
#include "fh_pica_weather.h"
#include "fh_pica_lightning.h"
#include "fh_pica_fur.h"
#include "fh_pica_fur_plan.h"
#include "fh_pica_specular.h"
#include "fh_pica_diffuse_map.h"
#include "fh_pica_layered.h"
#include "fh_pica_model.h"
#include "fh_pica_morph.h"
#include "fh_pica_morph_layout.h"
#include "fh_pica_color_fade.h"
#include "fh_pica_fog.h"
#include "fh_pica_height_fog.h"
#include "fh_pica_actor_fog_draw.h"
#include "fh_actor_fog_resident.h"
#include "pica_actor_height_fog_shader.h"
#include "pica_actor_fog_layout.h"
#include "fh_ground_receiver.h"
#include "fh_pica_viewport.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"
#include "pica_scene_shader.h"
#include "pica_scene_texture_shader.h"
#include "pica_scene_actor_hud_shader.h"
#include "pica_ambient_shader.h"
#include "pica_fur_gpu_shader.h"
#include "fh_pica_material_shader.h"
unsigned fh_pica_scene_textured=1;
unsigned fh_pica_scene_actors=1;
unsigned fh_pica_unimplemented_actor_callbacks,fh_pica_unimplemented_world_passes;
unsigned fh_pica_cpu_normal_calls_avoided,fh_pica_cpu_normal_groups_avoided;
static DVLB_s *binary;
static shaderProgram_s program;
static DVLB_s *actor_fog_binary;
static shaderProgram_s actor_fog_program;
static C3D_AttrInfo actor_fog_layout;
static unsigned actor_fog_active,actor_fog_draws,actor_fog_pipeline_reuses;
static shaderProgram_s ui_program;
static unsigned ui_active,ui_draws;
static DVLB_s *ambient_binary;
static shaderProgram_s ambient_program,ambient_ordinary_program;
static unsigned ambient_active,ambient_draws,ambient_indices,ambient_handoffs;
static void ambient_validate_abi(void){
    const shaderInstance_s *a=ambient_program.vertexShader,*b=program.vertexShader;
    /* BindProgram loads constants/defaults. Retention is legal only when those
     * writes cannot replace live camera, palette, material or lighting lanes. */
    if(ambient_program.geometryShader || program.geometryShader ||
       a->boolUniformMask || b->boolUniformMask || a->intUniformMask || b->intUniformMask ||
       a->numFloat24Uniforms!=b->numFloat24Uniforms ||
       memcmp(a->float24Uniforms,b->float24Uniforms,a->numFloat24Uniforms*sizeof(float24Uniform_s)))
        fh_platform_panic("PICA ambient retained ABI");
    const shaderInstance_s *c=ambient_ordinary_program.vertexShader;
    if(ambient_ordinary_program.geometryShader || c->dvle->dvlp!=a->dvle->dvlp ||
       c->boolUniformMask || c->intUniformMask || c->numFloat24Uniforms!=a->numFloat24Uniforms ||
       memcmp(c->float24Uniforms,a->float24Uniforms,a->numFloat24Uniforms*sizeof(float24Uniform_s)))
        fh_platform_panic("PICA ambient shared bank ABI");
    for(unsigned i=0;i<a->numFloat24Uniforms;i++)
        if(a->float24Uniforms[i].id<95)fh_platform_panic("PICA ambient live constant");
}
static shaderProgram_s cloud_program;
static unsigned cloud_active,cloud_shader_draws;
#include "../vendor/citro3d/internal.h"
static unsigned sky_depth_skipped,sky_depth_stripped;
static int sky_depth_original(void){
    static int mode=-1;
    if(mode<0){FILE *f=fopen("sdmc:/3ds/foxhollow/sky_depth.txt","rb");int c=f?fgetc(f):EOF;if(f)fclose(f);mode=c=='1';}
    return mode;
}
static int actor_uniform=-1,actor_uv_uniform=-1;
static int lighting_uniform=-1,vertex_material_uniform=-1,alpha_lighting_uniform=-1,light_uniform[8];
static FhPicaActorLighting lighting;
static int environment_uniform=-1;
static unsigned lighting_objects,lighting_enabled,lighting_lights,lighting_partial,lighting_max,lighting_alpha_objects;
static unsigned bound,blocks,draws,indices,pipeline_publications;
static unsigned world_bounds_tests,world_bounds_rejected;
static u64 world_bounds_indices;
static float depth_near,depth_far;
static u64 ticks;

/* Native camera owner. Never retain caller pointers: stack-backed camera data
 * can change in place. Bit equality preserves signed zero and NaN payloads.
 * Ordinary/ambient share this ABI; all other program binds and external
 * render-target/command owners invalidate it. World position scaling uses
 * different rows and explicitly retires this owner before publication.
 * Adapted from pinned Melee uniform_upload.h's installed-value discipline. */
typedef u32 fh_camera_word __attribute__((may_alias));
static unsigned camera_valid,camera_requests,camera_reuses;
#include <math.h>
#include <string.h>
static float cloud_clip_projection[4][4];
static unsigned cloud_clip_draws,cloud_clip_failures;
static int cloud_clip_plane(float out[4],float depth){
    float m[4][5];
    for(unsigned r=0;r<4;r++){for(unsigned c=0;c<4;c++)m[r][c]=cloud_clip_projection[c][r];m[r][4]=0;}
    m[2][4]=-1;m[3][4]=-depth;
    for(unsigned c=0;c<4;c++){
        unsigned p=c;for(unsigned r=c+1;r<4;r++)if(fabsf(m[r][c])>fabsf(m[p][c]))p=r;
        if(fabsf(m[p][c])<1e-12f)return 0;
        if(p!=c)for(unsigned k=0;k<5;k++){float t=m[c][k];m[c][k]=m[p][k];m[p][k]=t;}
        for(unsigned r=0;r<4;r++)if(r!=c){float f=m[r][c]/m[c][c];for(unsigned k=c;k<5;k++)m[r][k]-=f*m[c][k];}
    }
    for(unsigned c=0;c<4;c++){out[c]=m[c][4]/m[c][c];if(!isfinite(out[c]))return 0;}
    return 1;
}
static void scene_camera(const float view[3][4],const float projection_source[4][4]){
    camera_requests++;
    float sky[4][4];const float (*projection)[4]=projection_source;
    if(cloud_active){
        for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)sky[r][c]=projection_source[r][c];
        for(unsigned c=0;c<4;c++)sky[2][c]-=0.001f*sky[3][c];
        projection=(const float (*)[4])sky;
        memcpy(cloud_clip_projection,sky,sizeof sky);
    }
    if(camera_valid){
        const fh_camera_word *installed=(const void*)&C3D_FVUnif[GPU_VERTEX_SHADER][0];
        for(unsigned r=0;r<7;r++){
            const fh_camera_word *source=(const void*)(r<3?view[r]:projection[r-3]);
            const fh_camera_word *row=installed+r*4;
            /* C3D_FVec stores w,z,y,x; sources store x,y,z,w. */
            if(row[3]!=source[0] || row[2]!=source[1] || row[1]!=source[2] || row[0]!=source[3])goto publish;
        }
        camera_reuses++;return;
    }
publish:
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0],view[r][1],view[r][2],view[r][3]);
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    camera_valid=1;
}

static unsigned actors,actor_draws,actor_indices,actor_bones;
static unsigned actor_bones_written,actor_state_publications;
static u64 actor_ticks;
static unsigned shadow_active,shadow_alpha,shadow_objects,shadow_draws,shadow_indices;
static FhPicaFur *fur_active;
static const FhPicaFurPlan *fur_gpu_active;
static DVLB_s *fur_binary;
static shaderProgram_s fur_program;
static shaderProgram_s *layered_program;
static shaderProgram_s *height_program;
static unsigned height_active;
static unsigned layered_active,layered_draws;
static unsigned fur_gpu_objects,fur_gpu_draws,fur_gpu_rows,groom_draws;
static unsigned fur_groups,fur_source_shells,fur_omitted_shells;
static int fur_uniform,specular_uniform;
static unsigned fur_objects,fur_draws,fur_indices,fur_unsupported;
static u64 fur_ticks;

/* Bounded hardware/emulator C-menu differential. CPU publication only. */
const C3D_Tex *fh_cmenu_capture_sampler;
static struct {
    unsigned seen,frame,format,vertices,indices,cull,bones,bools,pipeline;
    unsigned clip[4],texture[7],vertex_hash,index_hash;
    u32 uniform[95][4];
} cmenu_capture[3];
static unsigned cmenu_hash(const void *p,unsigned bytes){
    const unsigned char *b=p;unsigned h=2166136261u;
    for(unsigned i=0;i<bytes;i++)h=(h^b[i])*16777619u;
    return h;
}
static void cmenu_observe(const FhSceneAsset *asset,const FhPicaModelDraw *d,const unsigned clip[4],unsigned pipeline){
    if(asset->id<1212 || asset->id>1214)return;
    unsigned slot=asset->id-1212;
    const C3D_Tex *t=fh_cmenu_capture_sampler;
    if(!cmenu_capture[slot].seen && t){
        /* The small resident image is read only on the first ring draw. */
        unsigned *v=cmenu_capture[slot].texture;
        v[0]=t->width;v[1]=t->height;v[2]=t->fmt;v[3]=t->param;
        v[4]=t->lodParam;v[5]=t->size;
        v[6]=t->size<=65536?cmenu_hash(t->data,t->size):0;
        cmenu_capture[slot].vertex_hash=cmenu_hash(asset->gpu+d->vertex_offset,d->vertices*(asset->compact?40:64));
        cmenu_capture[slot].index_hash=cmenu_hash(asset->gpu+d->index_offset,d->indices*2);
    }
    extern unsigned fh_minimal_frames;
    cmenu_capture[slot].seen++;cmenu_capture[slot].frame=fh_minimal_frames;
    cmenu_capture[slot].format=d->format;cmenu_capture[slot].vertices=d->vertices;
    cmenu_capture[slot].indices=d->indices;cmenu_capture[slot].cull=d->cull;
    cmenu_capture[slot].bones=d->bones;cmenu_capture[slot].pipeline=pipeline;
    cmenu_capture[slot].bools=C3D_BoolUnifs[GPU_VERTEX_SHADER];
    memcpy(cmenu_capture[slot].clip,clip,sizeof(cmenu_capture[slot].clip));
    memcpy(cmenu_capture[slot].uniform,C3D_FVUnif[GPU_VERTEX_SHADER],sizeof(cmenu_capture[slot].uniform));
}
static void cmenu_report(void){
    char line[256];
    snprintf(line,sizeof line,"CMENU capture v1 bytes=%u scope=CPU-publication first-image/latest-uniforms\n",(unsigned)sizeof cmenu_capture);fh_platform_log(line);
    for(unsigned s=0;s<3;s++)if(cmenu_capture[s].seen){
        snprintf(line,sizeof line,"CMENU model=%u seen=%u frame=%u format=%u vertices=%u indices=%u cull=%u bones=%u bools=%04x pipeline=%u vhash=%08x ihash=%08x\n",
            s+1212,cmenu_capture[s].seen,cmenu_capture[s].frame,cmenu_capture[s].format,
            cmenu_capture[s].vertices,cmenu_capture[s].indices,cmenu_capture[s].cull,
            cmenu_capture[s].bones,cmenu_capture[s].bools,cmenu_capture[s].pipeline,
            cmenu_capture[s].vertex_hash,cmenu_capture[s].index_hash);fh_platform_log(line);
        const unsigned *t=cmenu_capture[s].texture,*c=cmenu_capture[s].clip;
        snprintf(line,sizeof line,"CMENU texture=%u %u %u %u %08x %08x %u %08x clip=%u %u %u %u\n",s+1212,t[0],t[1],t[2],t[3],t[4],t[5],t[6],c[0],c[1],c[2],c[3]);fh_platform_log(line);
        for(unsigned u=0;u<95;u++){
            const u32 *v=cmenu_capture[s].uniform[u];
            snprintf(line,sizeof line,"CMENU uniform=%u %u %08lx %08lx %08lx %08lx\n",s+1212,u,(unsigned long)v[0],(unsigned long)v[1],(unsigned long)v[2],(unsigned long)v[3]);fh_platform_log(line);
        }
    }
}

static void shadow_pipeline_init(void);
#include "platform_pica_vertex_layouts.h"
#include "platform_pica_buffer_layouts.h"
#include "platform_pica_world_near.h"
void fh_pica_scene_init(void){
    fh_pica_vertex_layouts_init();
    if(fh_pica_game_model_draw_stride!=sizeof(FhPicaModelDraw))fh_platform_panic("PICA game/native draw-plan ABI mismatch; rebuild model.c");
    fh_pica_fur_material_init();
    shadow_pipeline_init();
    if(fh_pica_scene_actors){fh_pica_receiver_init();fh_pica_shadow_project_init();}
    binary=fh_pica_scene_actors?DVLB_ParseFile((u32*)pica_scene_actor_hud_shader,sizeof(pica_scene_actor_hud_shader)):
        fh_pica_scene_textured?DVLB_ParseFile((u32*)pica_scene_texture_shader,sizeof(pica_scene_texture_shader)):
        DVLB_ParseFile((u32*)pica_scene_shader,sizeof(pica_scene_shader));
    if(!binary || binary->numDVLE!=(fh_pica_scene_actors?FH_PICA_SHARED_ENTRIES:1) || binary->DVLP.codeSize>512)fh_platform_panic("PICA scene shader");
    shaderProgramInit(&program);shaderProgramSetVsh(&program,binary->DVLE);
    if(fh_pica_scene_actors){
        ambient_binary=DVLB_ParseFile((u32*)pica_ambient_shader,sizeof(pica_ambient_shader));
        if(!ambient_binary || ambient_binary->numDVLE!=4 || ambient_binary->DVLP.codeSize!=305)fh_platform_panic("PICA ambient shader");
        shaderProgramInit(&ui_program);shaderProgramSetVsh(&ui_program,ambient_binary->DVLE+2);
        const char *unames[]={"view","projection","tint","uvscale","bones","decode","fog_distance_scale"};
        const int uslots[]={0,3,7,8,10,76,94};
        for(unsigned i=0;i<sizeof(uslots)/sizeof(*uslots);i++)if(shaderInstanceGetUniformLocation(ui_program.vertexShader,unames[i])!=uslots[i])fh_platform_panic("PICA UI float ABI");
        shaderProgramInit(&ambient_program);shaderProgramSetVsh(&ambient_program,ambient_binary->DVLE+1);
        shaderProgramInit(&ambient_ordinary_program);shaderProgramSetVsh(&ambient_ordinary_program,ambient_binary->DVLE);
        for(unsigned i=0;i<sizeof(uslots)/sizeof(*uslots);i++)if(shaderInstanceGetUniformLocation(ambient_program.vertexShader,unames[i])!=uslots[i])fh_platform_panic("PICA ambient float ABI");
        if(shaderInstanceGetUniformLocation(ambient_program.vertexShader,"ambient")!=77 ||
           shaderInstanceGetUniformLocation(ambient_program.vertexShader,"actor_second_uv")!=0x69 ||
           shaderInstanceGetUniformLocation(ambient_program.vertexShader,"actor_vertex_material")!=0x6b ||
           shaderInstanceGetUniformLocation(ambient_program.vertexShader,"actor_alpha_lighting")!=0x75)
            fh_platform_panic("PICA ambient parameter ABI");
        ambient_validate_abi();
        shaderProgramInit(&cloud_program);shaderProgramSetVsh(&cloud_program,ambient_binary->DVLE+3);
        for(unsigned i=0;i<sizeof(uslots)/sizeof(*uslots);i++)if(shaderInstanceGetUniformLocation(cloud_program.vertexShader,unames[i])!=uslots[i])fh_platform_panic("PICA cloud float ABI");
        fur_binary=DVLB_ParseFile((u32*)pica_fur_gpu_shader,sizeof(pica_fur_gpu_shader));
        if(!fur_binary || fur_binary->numDVLE!=1 || fur_binary->DVLP.codeSize>512)fh_platform_panic("PICA fur shader");
        shaderProgramInit(&fur_program);shaderProgramSetVsh(&fur_program,fur_binary->DVLE);
        const char *names[]={"view","projection","tint","uvscale","bones","fur_shape","fur_shell","fur_index_decode","decode","ambient","light_position","light_color","fog_distance_scale"};
        const int slots[]={0,3,7,8,10,58,74,75,76,77,78,86,94};
        for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++)if(shaderInstanceGetUniformLocation(fur_program.vertexShader,names[i])!=slots[i])fh_platform_panic("PICA fur float ABI");
        if(shaderInstanceGetUniformLocation(fur_program.vertexShader,"actor_fur")!=0x76 || shaderInstanceGetUniformLocation(fur_program.vertexShader,"actor_specular")!=0x77)fh_platform_panic("PICA fur bool ABI");
        layered_program=fh_pica_material_shader_acquire(FH_MATERIAL_SHADER_ACTOR);
        height_program=fh_pica_material_shader_acquire(FH_MATERIAL_SHADER_HEIGHT_FOG);
        if(shaderInstanceGetUniformLocation(height_program->vertexShader,"height_fog")!=10 ||
           shaderInstanceGetUniformLocation(height_program->vertexShader,"fog_distance_scale")!=94 ||
           shaderInstanceGetUniformLocation(height_program->vertexShader,"height_fog_on")!=0x68 ||
           shaderInstanceGetUniformLocation(height_program->vertexShader,"height_fog_layered")!=0x69)
            fh_platform_panic("PICA height fog shader ABI");
        const char *lnames[]={"view","projection","tint","uvscale","bones","decode","ambient","light_position","light_color","fog_distance_scale"};
        const int lslots[]={0,3,7,8,10,76,77,78,86,94};
        for(unsigned i=0;i<sizeof(lslots)/sizeof(*lslots);i++)if(shaderInstanceGetUniformLocation(layered_program->vertexShader,lnames[i])!=lslots[i])fh_platform_panic("PICA layered float ABI");
        if(shaderInstanceGetUniformLocation(layered_program->vertexShader,"actor_fur")!=0x76 || shaderInstanceGetUniformLocation(layered_program->vertexShader,"actor_specular")!=0x77)fh_platform_panic("PICA layered bool ABI");
    }
    if(fh_pica_scene_actors)fh_pica_fog_init(shaderInstanceGetUniformLocation(program.vertexShader,"fog_distance_scale"));
    if(fh_pica_scene_actors){
        actor_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"actor");
        actor_uv_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"actor_second_uv");
        if(actor_uniform<0x68 || actor_uniform>=0x78)fh_platform_panic("PICA actor boolean ABI");
        if(actor_uv_uniform<0x68 || actor_uv_uniform>=0x78 || actor_uv_uniform==actor_uniform)
            fh_platform_panic("PICA actor UV boolean ABI");
        lighting_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"actor_lighting");
        environment_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"actor_environment");
        if(environment_uniform<0x68 || environment_uniform>=0x78)fh_platform_panic("PICA environment boolean ABI");
        vertex_material_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"actor_vertex_material");
        alpha_lighting_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"actor_alpha_lighting");
        fur_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"actor_fur");
        if(fur_uniform!=0x76)fh_platform_panic("PICA fur boolean ABI");
        specular_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"actor_specular");
        if(specular_uniform!=0x77)fh_platform_panic("PICA specular boolean ABI");
        if(lighting_uniform<0x68 || lighting_uniform>=0x78 || vertex_material_uniform<0x68 || vertex_material_uniform>=0x78)
            fh_platform_panic("PICA light boolean ABI");
        unsigned mask=(1u<<(actor_uniform-0x68))|(1u<<(actor_uv_uniform-0x68));
        if(mask&(1u<<(lighting_uniform-0x68)))fh_platform_panic("PICA light boolean alias");
        mask|=1u<<(lighting_uniform-0x68);
        if(mask&(1u<<(vertex_material_uniform-0x68)))fh_platform_panic("PICA material boolean alias");
        mask|=1u<<(vertex_material_uniform-0x68);
        if(alpha_lighting_uniform<0x68 || alpha_lighting_uniform>=0x78 ||
           (mask&(1u<<(alpha_lighting_uniform-0x68))))fh_platform_panic("PICA alpha lighting boolean ABI");
        mask|=1u<<(alpha_lighting_uniform-0x68);
        if(mask&(1u<<(environment_uniform-0x68)))fh_platform_panic("PICA environment boolean alias");
        mask|=1u<<(environment_uniform-0x68);
        if(shaderInstanceGetUniformLocation(program.vertexShader,"ambient")!=77 ||
           shaderInstanceGetUniformLocation(program.vertexShader,"light_position")!=78 ||
           shaderInstanceGetUniformLocation(program.vertexShader,"light_color")!=86)
            fh_platform_panic("PICA lighting float ABI");
        for(unsigned i=0;i<8;i++){
            char name[16];snprintf(name,sizeof(name),"light%u",i);
            light_uniform[i]=shaderInstanceGetUniformLocation(program.vertexShader,name);
            if(light_uniform[i]<0x68 || light_uniform[i]>=0x78)fh_platform_panic("PICA light mask ABI");
            if(mask&(1u<<(light_uniform[i]-0x68)))fh_platform_panic("PICA light mask alias");
            mask|=1u<<(light_uniform[i]-0x68);
        }
    }
    fh_platform_log(fh_pica_scene_actors?"PICA scene replacement: world and actor base textures; no legacy submission\n":fh_pica_scene_textured?"PICA scene replacement: native textures; no models/HUD/effects; no legacy submission\n":
        "PICA scene replacement: geometry-only; no textures/models/HUD/effects; no legacy submission\n");
    fh_pica_materials_init();
    fh_pica_environment_init();
    if(fh_pica_scene_actors)fh_pica_hud_init(binary->DVLE+1);
    if(fh_pica_scene_actors)fh_pica_hologram_init(binary->DVLE+2);
    if(fh_pica_scene_actors)fh_pica_stars_init(binary->DVLE+3);
    if(fh_pica_scene_actors)fh_pica_drops_init(binary->DVLE+3);
    if(fh_pica_scene_actors)fh_pica_weather_init();
    if(fh_pica_scene_actors)fh_pica_lightning_init();
    if(fh_pica_scene_actors)fh_pica_glow_init(binary->DVLE+4);
    if(fh_pica_scene_actors)fh_pica_splash_init(binary->DVLE+7);
    if(fh_pica_scene_actors)fh_platform_log(FH_PICA_ACTOR_LIGHTING_EXPERIMENT?
        "PICA actors: native diffuse and analytic environment lighting; full fidelity incomplete\n":
        "PICA actors: native GPU skinning/base textures/opacity/dual-UV/morph positions; lighting disabled\n");
}
void fh_pica_scene_actor_lighting(const FhPicaActorLighting *state){
    fh_pica_color_fade_set(0); /* UI/custom actors cannot inherit another tint. */
    lighting=*state;lighting_objects++;lighting_enabled+=state->enabled!=0;lighting_partial+=state->partial_lights;
    lighting_alpha_objects+=state->alpha_lighting!=0;
    unsigned count=0;for(unsigned i=0;i<8;i++)count+=(state->active>>i)&1;
    lighting_lights+=count;if(count>lighting_max)lighting_max=count;
}
void fh_pica_scene_actor_second_uv(unsigned enabled){
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uv_uniform,enabled!=0);
}
void fh_pica_scene_environment(unsigned enabled){
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,environment_uniform,enabled!=0);
}
void fh_pica_scene_invalidate(void){camera_valid=0;bound=0;fh_pica_shadow_project_invalidate();fh_pica_lightning_invalidate();fh_pica_weather_invalidate();fh_pica_height_fog_invalidate();fh_pica_hud_invalidate();fh_pica_glow_invalidate();fh_pica_color_fade_invalidate();}
static void bind_scene_pipeline(void){
    unsigned selected=actor_fog_active?9:ambient_active?8:cloud_active?7:shadow_active?6:ui_active?5:height_active?4:layered_active?3:fur_gpu_active?2:1;
    if(bound==selected)return;
    /* PICA_AMBIENT_HANDOFF_V2: only these two programs share the ordinary
     * fragment/raster owner and input/output ABI. External invalidations and
     * every other shader continue through the complete publication below.
     * SDK binding still publishes program/code/attributes/constants/dirty bits. */
    if((bound==1 || bound==8) && (selected==1 || selected==8)){
        /* PICA_AMBIENT_BANK_V3: keep the general entry in this same DVLP
         * after admission. A full external reset still selects the original
         * shared HUD/particle bank, preserving its existing code residency. */
        C3D_BindProgram(selected==8?&ambient_program:&ambient_ordinary_program);
        bound=selected;ambient_handoffs++;return;
    }
    camera_valid=0;
    if(shadow_active==2)fh_pica_shadow_image_rebind();
    fh_pica_height_fog_invalidate();
    fh_pica_materials_invalidate();
    fh_pica_hud_invalidate();
    fh_pica_glow_invalidate();
    C3D_BindProgram(actor_fog_active?&actor_fog_program:ambient_active?&ambient_program:cloud_active?&cloud_program:shadow_active?fh_pica_receiver_caster_program():ui_active?&ui_program:height_active?height_program:layered_active?layered_program:fur_gpu_active?&fur_program:fh_pica_scene_actors?&ambient_ordinary_program:&program);
    C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    fh_pica_fog_invalidate();
    fh_pica_color_fade_invalidate();
    for(unsigned i=0;i<3;i++)C3D_TexBind(i,NULL);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvBufUpdate(C3D_Both,0);
    C3D_TexEnv *e=C3D_GetTexEnv(0);
    C3D_TexEnvSrc(e,C3D_Both,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(e,C3D_Both,GPU_REPLACE);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
    depth_near=0;depth_far=1;
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    bound=selected;
    pipeline_publications++;
}
unsigned fh_pica_scene_layered_active(void){return layered_active;}
static void select_layered(unsigned enabled){
    if(layered_active!=enabled){layered_active=enabled;bound=0;}
}
static void select_cloud(unsigned enabled){
    if(cloud_active!=enabled){cloud_active=enabled;bound=0;}
}
static void select_ambient(unsigned enabled){
    if(ambient_active!=enabled){
        ambient_active=enabled;
        if(bound!=1 && bound!=8)bound=0;
    }
}
static void select_ui(unsigned enabled){
    if(ui_active!=enabled){ui_active=enabled;bound=0;}
}
static void select_height(unsigned enabled){
    if(height_active!=enabled){height_active=enabled;bound=0;}
}
static void scene_depth(float near_z,float far_z){
    if(depth_near==near_z && depth_far==far_z)return;
    float depth[2];fh_pica_viewport_depth(depth,near_z,far_z);
    C3D_DepthMap(true,depth[0],depth[1]);depth_near=near_z;depth_far=far_z;
}
unsigned fh_pica_scene_specular(unsigned enabled){
    enabled=enabled && lighting.enabled;
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,specular_uniform,enabled);
    return enabled;
}
#include "platform_pica_shadow_draw.h"

int fh_pica_world_stream_geometry(const FhSceneAsset *asset,const FhPicaWorldDraw *g,FhPicaDrawDescription *out){
    if(!asset || !g || !out)return 0;
    unsigned selected=(!!asset->compact)*2+1;
    out->attributes=vertex_layouts[selected];
    memset(&out->buffers,0,sizeof out->buffers);
    out->buffers.base_paddr=0x18000000;out->buffers.bufCount=1;
    u32 address=osConvertVirtToPhys(g->vertex);
    if(address<out->buffers.base_paddr)return 0;
    out->buffers.buffers[0].offset=address-out->buffers.base_paddr;
    memcpy(out->buffers.buffers[0].flags,buffer_layouts[selected].flags[0],8);
    out->index_address=osConvertVirtToPhys(g->index);out->index_count=g->indices;
    out->effect.zBuffer=1;out->effect.zScale=f32tof24(-1.f);out->effect.zOffset=f32tof24(0.f);
    out->effect.cullMode=g->cull?GPU_CULL_FRONT_CCW:GPU_CULL_NONE;out->effect.fragOpMode=0x100;
    return 1;
}

unsigned fh_pica_scene_draw(const FhSceneAsset *asset,const FhPicaWorldPlan *plan,
    const float view[3][4],const float projection[4][4],const unsigned clip[4],unsigned *out_indices){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 start=svcGetSystemTick();unsigned count=0,vertices=0,format=~0u;
    *out_indices=0;
    select_layered(0);select_ui(0);select_cloud(0);select_ambient(0);
    if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_WORLD))return 0;
    if(!asset || !plan || !asset->block)fh_platform_panic("PICA scene asset");
    if(fh_pica_scene_textured && !plan->materials.items)return 0;
    if(fh_pica_scene_actors)fh_pica_shadow_project_world(asset,plan,view,projection,clip);
    unsigned height_requested=fh_pica_scene_actors && fh_pica_height_fog_world_active();
    select_height(height_requested);
    unsigned height_prepared=0;
    FhPicaStreamState native_state;FhPicaDrawLive native_live={0};
    unsigned native_active=0,native_fog=~0u;
    /* Original per-draw receiver bounds already share this plan's lifetime
     * and refresh on every published world-position mutation. This does not
     * select the separately batched shadow geometry or depend on its toggle. */
    const FhPicaShadowReceiver *bounds=fh_pica_shadow_receivers_for_asset(asset);
    float planes[4][4];FhWorldPlane prepared[4];
    fh_world_clip_planes(planes,view,projection);fh_world_planes_prepare(prepared,planes);
    world_near_prepare(view,projection);
    for(unsigned i=0;i<plan->count;i++){
        const FhPicaWorldDraw *g=plan->draw+i;unsigned n=g->indices;
        world_bounds_tests++;
        if(bounds && bounds[i].draw==g && fh_world_prepared_outside(&bounds[i].bounds,prepared)){
            world_bounds_rejected++;world_bounds_indices+=n;continue;
        }

        unsigned near_cpu=bounds && bounds[i].draw==g && world_near_straddles(&bounds[i].bounds);
        FhPicaWorldSelection selection;
        unsigned selected=(!near_cpu && !height_requested && fh_pica_scene_textured)?
            fh_pica_world_stream_select(&plan->materials,i,&native_live,&selection):0;
        if(selected==2)continue;
        if(selected==1){
            if(fh_pica_scene_reserve_changed()){native_active=0;format=~0u;}
            if(!native_active || !bound || format!=g->format || native_fog!=selection.fog){
                fh_pica_stream_end();native_active=0;bind_scene_pipeline();
                if(fh_pica_scene_actors){
                    C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uniform,false);
                    fh_pica_environment_bind(0,0);
                }
                scene_depth(0,1);camera_valid=0;
                float scale=asset->compact?(g->format==7?1.f:g->format==5?1.f/8:1.f/256):1.f;
                for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0]*scale,view[r][1]*scale,view[r][2]*scale,view[r][3]);
                for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
                C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
                fh_pica_world_stream_common(selection.fog);
                if(!fh_pica_stream_begin(&native_state) || !fh_pica_stream_environment(&native_live))
                    fh_platform_panic("PICA world command ownership");
                native_active=1;native_fog=selection.fog;format=g->format;
            }
            native_live.texture_config=(native_live.texture_config&~7u)|selection.draw->texture_units;
            native_live.environment_buffer=(native_live.environment_buffer&~0xff00u)|((selection.buffer_mask&255u)<<8);
            unsigned group=fh_pica_world_packet_count(&plan->materials,i);
            if(group){
                unsigned visible=1,group_indices=n;
                for(unsigned k=1;k<group;k++){
                    const FhPicaWorldDraw *next=plan->draw+i+k;
                    if(bounds && bounds[i+k].draw==next && (fh_world_prepared_outside(&bounds[i+k].bounds,prepared) ||
                       world_near_straddles(&bounds[i+k].bounds))){
                        visible=0;break;
                    }
                    group_indices+=next->indices;
                }
                if(visible && fh_pica_world_packet_submit(&plan->materials,i,&native_live,selection.fog)){
                    world_bounds_tests+=group-1;count+=group;vertices+=group_indices;i+=group-1;continue;
                }
            }
            if(!fh_pica_stream_draw(selection.draw,&native_live,0))fh_platform_panic("PICA world command publication");
            count++;vertices+=n;continue;
        }
        if(native_active){fh_pica_stream_end();native_active=0;format=~0u;}
        unsigned height=height_requested ?
            fh_pica_material_height_fog_eligible(&plan->materials,g->material):0;
        if(height && !height_prepared){fh_pica_height_fog_world_prepare();height_prepared=1;}
        if(!height)fh_pica_height_fog_world_skipped();
        fh_pica_scene_reserve();
        unsigned was_bound=bound;bind_scene_pipeline();
        if(fh_pica_scene_actors){
            C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uniform,false);
            if(!was_bound || format==~0u)fh_pica_environment_bind(0,0);
        }
        if(!was_bound || format!=g->format){
            scene_depth(0,1);
            fh_pica_world_layout_bind(asset->compact,fh_pica_scene_textured);
            camera_valid=0; /* World rows include vertex position scale. */
            float scale=asset->compact?(g->format==7?1.f:g->format==5?1.f/8:1.f/256):1.f;
            for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0]*scale,view[r][1]*scale,view[r][2]*scale,view[r][3]);
            for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
            float color=asset->compact?1.f/15:1.f;C3D_FVUnifSet(GPU_VERTEX_SHADER,7,color,color,color,color);
            format=g->format;
        }
        if(fh_pica_scene_textured){
            int material=fh_pica_material_bind(&plan->materials,g->material,g->format,asset->compact);
            if(!material){char line[160];snprintf(line,sizeof(line),"PICA missing material: asset=%u material=%u count=%u draw=%u\n",asset->id,g->material,plan->materials.count,i);fh_platform_log(line);fh_platform_panic("PICA scene missing material");}
            if(material==2)continue;
        }
        C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
        C3D_CullFace(g->cull?GPU_CULL_FRONT_CCW:GPU_CULL_NONE);
        fh_pica_world_buffers(asset->compact,fh_pica_scene_textured,g->vertex);
        if(!height)fh_pica_color_fade_bind(0);
        if(height_requested)fh_pica_height_fog_world_bind(height);
        if(near_cpu && world_near_draw(g,asset->compact,fh_pica_scene_textured,view,
            asset->compact?(g->format==7?1.f:g->format==5?1.f/8:1.f/256):1.f,projection))format=~0u;
        else C3D_DrawElements(GPU_TRIANGLES,n,C3D_UNSIGNED_SHORT,g->index);
        count++;vertices+=n;
    }
    fh_pica_stream_end();
    blocks++;draws+=count;indices+=vertices;ticks+=svcGetSystemTick()-start;*out_indices=vertices;return count;
}
static inline __attribute__((always_inline)) void actor_publish_lighting(unsigned lighting_on){
    fh_pica_scene_specular(0); /* Fur/shadow and special callbacks start clean. */
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,fur_uniform,fur_active!=NULL);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,lighting_uniform,lighting.enabled!=0 && lighting_on);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,alpha_lighting_uniform,lighting.alpha_lighting!=0 && lighting_on);
#if FH_PICA_ACTOR_LIGHTING_EXPERIMENT
    if(lighting_on){
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,vertex_material_uniform,lighting.vertex_material!=0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,77,lighting.ambient[0],lighting.ambient[1],lighting.ambient[2],lighting.ambient[3]);
    for(unsigned i=0;i<8;i++){
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,light_uniform[i],(lighting.active>>i)&1);
        if(lighting.active&(1u<<i)){
            const float *p=lighting.position[i],*c=lighting.color[i];
            C3D_FVUnifSet(GPU_VERTEX_SHADER,78+i,p[0],p[1],p[2],p[3]);
            C3D_FVUnifSet(GPU_VERTEX_SHADER,86+i,c[0],c[1],c[2],c[3]);
        }
    }
    }
#endif
}
#include "platform_pica_actor_fog_draw.h"

static unsigned actor_state_reuses;
int fh_pica_actor_stream_geometry(const FhSceneAsset *asset,const FhPicaModelDraw *g,FhPicaDrawDescription *out){
    if(!asset || !g || !out || !asset->compact)return 0;
    out->attributes=vertex_layouts[7];
    memset(&out->buffers,0,sizeof out->buffers);
    out->buffers.base_paddr=0x18000000;out->buffers.bufCount=1;
    u32 address=osConvertVirtToPhys(asset->gpu+g->vertex_offset);
    if(address<out->buffers.base_paddr)return 0;
    out->buffers.buffers[0].offset=address-out->buffers.base_paddr;
    memcpy(out->buffers.buffers[0].flags,buffer_layouts[6].flags[0],8);
    out->index_address=osConvertVirtToPhys(asset->gpu+g->index_offset);out->index_count=g->indices;
    out->effect.cullMode=g->cull?GPU_CULL_FRONT_CCW:GPU_CULL_NONE;
    return 1;
}

#include "platform_pica_world_slice.h"
unsigned fh_pica_declined_material_draws;
unsigned fh_pica_scene_actor_draw(const FhSceneAsset *asset,const FhPicaModelPlan *plan,const float *poses,unsigned joint_count,
    const float world[3][4],const float view[3][4],const float projection[4][4],const unsigned clip[4],float near_z,float far_z,unsigned *out_indices){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 start=svcGetSystemTick();unsigned count=0,total=0;
    *out_indices=0;
    select_height(0);
    /* Ordinary materials select their pipeline before the first draw. Resetting
     * here needlessly discarded it between compatible actors. Shadow's compact
     * early path and fur still explicitly require the ordinary/fur pipeline. */
    if(shadow_active || fur_active){select_layered(0);select_ui(0);select_cloud(0);select_ambient(0);}
    if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS))return 0;
    if(!shadow_active && plan && !plan->materials.items){fh_pica_declined_material_draws++;return 0;}
    if(!fh_pica_scene_actors || !asset || !plan || asset->block)
        fh_platform_panic("PICA actor immutable storage");
    if(shadow_active==1){
        FhPicaReceiverPlane receiver;
        if(!fh_pica_receiver_plane(&receiver,view,projection,near_z,far_z)){receiver_rejected++;return 0;}
        fh_pica_receiver_draw(&receiver,clip);
    }
    /* SPECULAR_PREFETCH_V1: auxiliary target passes invalidate shared uniforms.
     * Resolve before the actor publishes ANY lighting, camera or pose state. */
    if(!shadow_active && !fur_active && lighting.enabled)
        fh_pica_material_specular_require(&plan->materials,asset->compact);
    unsigned lighting_on=!shadow_active && !(fh_graphics_options&FH_GRAPHICS_NO_LIGHTING);
    actor_publish_lighting(lighting_on);
    const void *morph_view=fh_pica_morph_view(asset,poses);
    if(shadow_active==2 && !morph_view){ /* compact mask order only */
        const FhPicaShadowPlan *shadow=fh_native_scene_shadow_plan(asset);
        if(shadow){
            count=shadow_compact_draw(asset,shadow,poses,joint_count,world,view,projection,clip,near_z,far_z,&total);
            compact_shadow_saved+=plan->count-count;
            if(shadow_active==1){count++;total+=6;}
            actors++;actor_draws+=count;actor_indices+=total;actor_ticks+=svcGetSystemTick()-start;
            *out_indices=total;return count;
        }
    }
    const FhPicaFurGroomPlan*groom=fur_gpu_active&&(fh_graphics_options&FH_GRAPHICS_FUR_GROOM)?fur_gpu_active->groom:NULL;
    static FhPicaActorCommandState native_state;FhPicaActorParameters native_live={0};
    unsigned native_active=0,native_key=~0u;
    unsigned previous_format=~0u;
    unsigned layers=fur_active?fur_active->layers:1;
    for(unsigned shell=0;shell<layers;shell++){
    unsigned first_shell_draw=1;
    if(fur_active){
        fur_active->layer=fh_pica_fur_layer(shell,layers,fur_active->source_layers);
        fur_active->noise_page=fur_active->noise_pages[shell];
    }
    for(unsigned i=0;i<plan->count;i++){
        const FhPicaModelDraw *d=plan->draw+i;
        if(fur_active && !(d->flags&0x200))continue;
        const void*groom_vertices=groom && i<groom->count && groom->draw[i].ready?(const unsigned char*)groom->gpu+groom->draw[i].offset:NULL;
        unsigned stream_skin=asset->meta[7]=='B';
        select_cloud(!shadow_active && !fur_active && fh_pica_material_cloud_selected(&plan->materials,d->material));
        select_ui(!cloud_active && !shadow_active && !fur_active && !lighting.enabled && fh_pica_material_ui_selected(&plan->materials,d->material));
        select_layered(!cloud_active && !shadow_active && !fur_active && asset->compact && lighting.enabled &&
            fh_pica_material_layered_selected(&plan->materials,d->material));
        /* PICA_AMBIENT_V1: no normal-dependent material can enter this lane.
         * Keep live skinning, ambient/alpha and both UV planes on the GPU. */
        select_ambient(!shadow_active && !fur_active && !cloud_active && !ui_active && !layered_active &&
            lighting_on && lighting.enabled && !lighting.active &&
            fh_pica_material_ambient_selected(&plan->materials,d->material));

        unsigned native_pipeline=ambient_active?8:1;
        FhPicaActorSelection selection;
        unsigned native_selected=asset->compact && !morph_view && !shadow_active && !fur_active &&
            !cloud_active && !ui_active && !layered_active && !actor_fog_active &&
            fh_pica_actor_stream_select(&plan->materials,i,lighting.enabled,&native_live,&selection);
        fh_pica_scene_reserve();
        if(native_active && !fh_pica_actor_stream_active()){native_active=0;previous_format=~0u;first_shell_draw=1;}
        if(native_selected){
            if(!native_active || !bound || bound!=native_pipeline || native_key!=selection.key){
                native_active=0;
                bind_scene_pipeline();scene_depth(near_z,far_z);
                C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uniform,true);scene_camera(view,projection);
                C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
                fh_pica_actor_stream_common(&selection);
                if(!fh_pica_actor_stream_sync(&native_state) || !fh_pica_actor_stream_environment(&native_live))
                    fh_platform_panic("PICA actor command ownership");
                actor_state_reuses+=native_state.valid!=0;
                native_active=1;native_key=selection.key;first_shell_draw=1;actor_state_publications++;
            }
            native_live.texture_config=(native_live.texture_config&~7u)|selection.texture_units;
            float scale[4]={lighting.normal_scale/64.f,stream_skin?1.f/128:.25f,1.f/15,
                d->format==7?1.f:d->format==5?1.f/8:1.f/256};
            memcpy(native_live.scale,scale,sizeof scale);
            FhPicaActorPose pose={poses,&world[0][0],(const float*)(asset->meta+asset->tails_at),
                d->bone,joint_count,d->bones,first_shell_draw?(1u<<d->bones)-1:d->palette_changed};
            if(!fh_pica_actor_stream_draw(selection.geometry,&native_live,&pose,0))
                fh_platform_panic("PICA actor command publication");
            actor_bones_written+=__builtin_popcount(pose.changed);
            ambient_draws+=ambient_active!=0;ambient_indices+=ambient_active?d->indices:0;
            count++;total+=d->indices;actor_bones+=d->bones;first_shell_draw=0;previous_format=d->format;
            continue;
        }
        if(fh_pica_actor_stream_active()){
            fh_pica_actor_stream_end();native_active=0;bound=0;previous_format=~0u;first_shell_draw=1;
        }
        unsigned publish=!count || !bound;bind_scene_pipeline();
        if(publish){
            scene_depth(near_z,far_z);
            C3D_BoolUnifSet(GPU_VERTEX_SHADER,actor_uniform,true);
            scene_camera(view,projection);
            C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
            fh_pica_actor_layout_bind(asset->compact,fur_gpu_active!=NULL,groom!=NULL);
            actor_state_publications++;
        }
        float ps=asset->compact?(d->format==7?1.f:d->format==5?1.f/8:1.f/256):1.f;
        if(publish || previous_format!=d->format)
            C3D_FVUnifSet(GPU_VERTEX_SHADER,76,ps,asset->compact?1.f/15:1.f,stream_skin?1.f/128:asset->compact?.25f:1.f,lighting.normal_scale*(asset->compact?1.f/64:1.f));
        previous_format=d->format;
        /* Every actor starts fresh; within it, immutable prepared palettes
         * state which slots changed. GPU retirement invalidation forces all
         * rows again. No live matrix comparisons or stale pose reuse. */
        unsigned changed=(publish||first_shell_draw)?(1u<<d->bones)-1:fur_active?d->fur_palette_changed:d->palette_changed;
        if(fur_gpu_active){
            if(d->bones>16)fh_platform_panic("PICA GPU fur palette admission");
            if(!publish && shell && first_shell_draw)changed=fur_gpu_active->wrap_changed;
            C3D_FVUnifSet(GPU_VERTEX_SHADER,74,(float)(fur_active->layer+1),groom_vertices?(float)fur_active->layer/4096.f:0,0,0);
            if(publish)C3D_FVUnifSet(GPU_VERTEX_SHADER,75,.33334f,0,0,0);
        }
        for(unsigned b=0;b<d->bones;b++)if(changed&(1u<<b)){
            unsigned bone=d->bone[b];float rows[3][4];
            const float *matrix=bone<joint_count?poses+bone*16:&world[0][0];
            if(bone>joint_count)fh_platform_panic("PICA actor live bone slot");
            const float *tail=(const float*)(asset->meta+asset->tails_at)+bone*3;
            if(fur_active && !fur_gpu_active)fh_pica_fur_pose(rows,matrix,tail,world,fur_active,bone,joint_count);
            else fh_pica_model_pose(rows,matrix,tail);
            for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,10+b*3+r,rows[r][0],rows[r][1],rows[r][2],rows[r][3]);
            actor_bones_written++;
            if(fur_gpu_active){
                if(bone>=fur_gpu_active->count)fh_platform_panic("PICA GPU fur shape admission");
                const FhPicaFurShape*s=fur_gpu_active->shape+bone;
                C3D_FVUnifSet(GPU_VERTEX_SHADER,58+b,s->pivot_tail[0],s->pivot_tail[1],s->pivot_tail[2],s->log_growth[fur_active->step-1]);
                fur_gpu_rows+=4;
            }
        }
        const void *positions=fh_pica_morph_positions(morph_view,i);
        const void *attributes=positions?fh_pica_morph_attributes(morph_view,i):NULL;
        const void *groom_stream=groom?(groom_vertices?groom_vertices:groom->gpu):NULL;
        fh_pica_actor_buffers(asset->compact,asset->gpu+d->vertex_offset,positions,attributes,groom_stream);
        if(shadow_active){
            if(publish){
                /* D24S8 is already owned/cleared by the frame target. Bit zero
                 * forms a union: overlapping caster triangles darken once,
                 * including across command-buffer splits. No depth writes. */
                /* Same resident vertex program as the ordinary actor. Only
                 * replace material-owned stages; fog owns stage 5 and retains
                 * its real publication state. No texture/source is changed. */
                /* Fog the black shadow colour at the receiver distance while
                 * preserving opacity. Blending onto the already fogged ground
                 * then preserves its fog contribution instead of darkening it. */
                shadow_raster();
            }
        }else if(fur_active){
            int material=fh_pica_fur_material_bind(&plan->materials,d->material,d->format,asset->compact,fur_active);
            if(!material)fh_platform_panic("PICA fur material identity");
            if(material==2){fur_unsupported++;continue;}
            C3D_CullFace(GPU_CULL_FRONT_CCW);
        }else{
        C3D_CullFace(d->cull?GPU_CULL_FRONT_CCW:GPU_CULL_NONE);
        int material=fh_pica_material_bind(&plan->materials,d->material,d->format,asset->compact);
        if(!material)
            fh_platform_panic("PICA actor missing material");
        if(material==2)continue;
        }
        fh_pica_color_fade_bind(!shadow_active);
        float cloud_plane[4];
        unsigned cloud_clip=cloud_active && cloud_clip_plane(cloud_plane,1000.f);
        cloud_clip_failures+=cloud_active && !cloud_clip;
        if(cloud_clip){
            u32 data[4];for(unsigned c=0;c<4;c++)data[c]=f32tof24(cloud_plane[c]);
            GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP,1);GPUCMD_AddIncrementalWrites(GPUREG_FRAGOP_CLIP_DATA0,data,4);
            cloud_clip_draws++;
        }
        u32 sky_depth=0;
        if(cloud_active && !sky_depth_original()){
            sky_depth=C3Di_GetContext()->effect.depthTest;
            if(!((sky_depth>>8)&0xf)){
                sky_depth_skipped++;
                if(cloud_clip)GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP,0);
                continue;
            }
            if(sky_depth&(GPU_WRITE_DEPTH<<8)){
                C3D_DepthTest(sky_depth&1,(GPU_TESTFUNC)((sky_depth>>4)&7),(GPU_WRITEMASK)((sky_depth>>8)&0xf));sky_depth_stripped++;
            }else sky_depth=0;
        }
        C3D_DrawElements(GPU_TRIANGLES,d->indices,C3D_UNSIGNED_SHORT,asset->gpu+d->index_offset);
        if(sky_depth)C3D_DepthTest(sky_depth&1,(GPU_TESTFUNC)((sky_depth>>4)&7),(GPU_WRITEMASK)((sky_depth>>8)&0x1f));
        if(cloud_clip)GPUCMD_AddWrite(GPUREG_FRAGOP_CLIP,0);
        cmenu_observe(asset,d,clip,bound);
        ambient_draws+=ambient_active!=0;ambient_indices+=ambient_active?d->indices:0;
        layered_draws+=layered_active!=0;ui_draws+=ui_active!=0;cloud_shader_draws+=cloud_active!=0;
        if(fur_gpu_active)fur_gpu_draws++;
        groom_draws+=groom_vertices!=NULL;
        count++;total+=d->indices;actor_bones+=d->bones;first_shell_draw=0;
    }
    }
    /* World calls publish their own format/camera from the first group;
     * actor calls publish on their first draw. The shared pipeline remains
     * bound until the real frame/command-buffer owner invalidates it. */
    if(shadow_active==1){count++;total+=6;}
    actors++;actor_draws+=count;actor_indices+=total;actor_ticks+=svcGetSystemTick()-start;
    *out_indices=total;return count;
}
void fh_pica_scene_shadow(const FhSceneAsset *asset,const float *poses,unsigned joints,
    const float world[3][4],const float view[3][4],const float projection[4][4],const unsigned clip[4],unsigned alpha){
    if(!alpha || !(fh_graphics_options&FH_GRAPHICS_SILHOUETTE_SHADOWS) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS|FH_GRAPHICS_NO_GROUND_SHADOWS)))return;
    /* Flush old HUD before changing mode; its real shader handoff is retained.
     * A shadow shares the native actor program, attributes and owner. Replacing
     * the material does not require unbinding/rebinding every texture and shader.
     * Actual HUD/GS/command-buffer handoffs still invalidate the scene normally. */
    fh_pica_hud_flush();fh_pica_materials_invalidate();
    unsigned old_draws=actor_draws,old_indices=actor_indices;
    shadow_active=1;shadow_alpha=alpha>255?255:alpha;
    fh_pica_scene_actor(asset,poses,joints,world,view,projection,clip);
    shadow_active=0;shadow_objects++;shadow_draws+=actor_draws-old_draws;shadow_indices+=actor_indices-old_indices;
    C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_StencilOp(GPU_STENCIL_KEEP,GPU_STENCIL_KEEP,GPU_STENCIL_KEEP);
    /* All normal materials republish raster/UV/light state. Invalidate their
     * retained combiner identity because the shadow replaced stages 0..3. */
    fh_pica_materials_invalidate();
}
void fh_pica_scene_fur(const FhSceneAsset *asset,const float *poses,unsigned joints,
    const float world[3][4],const float view[3][4],const float projection[4][4],const unsigned clip[4],const FhPicaFur *state){
    if(!(fh_graphics_options&FH_GRAPHICS_FUR) || (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS)))return;
    u64 start=svcGetSystemTick();
    fh_pica_hud_flush();fh_pica_materials_invalidate();
    unsigned old_draws=actor_draws,old_indices=actor_indices;
    if(!state->source_layers || state->source_layers>15 || !state->layers ||
       state->layers>state->source_layers || (state->layers!=state->source_layers && state->layers>2))
        fh_platform_panic("PICA fur layer selection");
    FhPicaFur live=*state;fur_active=&live;
    if(!state->stream_skin && state->step>=1 && state->step<=2)fur_gpu_active=fh_native_scene_fur_plan(asset);
    if(fur_gpu_active){fh_pica_scene_invalidate();fur_gpu_objects++;}
    fh_pica_scene_actor(asset,poses,joints,world,view,projection,clip);
    if(fur_gpu_active){fur_gpu_active=NULL;fh_pica_scene_invalidate();}
    fur_active=NULL;fur_objects+=state->layers;fur_groups++;
    fur_source_shells+=state->source_layers;fur_omitted_shells+=state->source_layers-state->layers;
    fur_draws+=actor_draws-old_draws;fur_indices+=actor_indices-old_indices;
    fh_pica_materials_invalidate();
    fur_ticks+=svcGetSystemTick()-start;
}
void fh_pica_scene_shutdown(void){
    fh_pica_actor_stream_end();
    {char line[128];snprintf(line,sizeof line,"PICA actor command continuity: retained=%u\n",actor_state_reuses);fh_platform_log(line);}
    {char line[144];snprintf(line,sizeof(line),"PICA camera owner: requests=%u reuses=%u rows_avoided=%u resident_bytes=%u\n",camera_requests,camera_reuses,camera_reuses*7,(unsigned)sizeof(camera_valid));fh_platform_log(line);}
    {char line[192];snprintf(line,sizeof(line),"PICA world visibility: tests=%u rejected=%u rejected_indices=%llu added_bounds_bytes=0\n",world_bounds_tests,world_bounds_rejected,(unsigned long long)world_bounds_indices);fh_platform_log(line);}
    fh_pica_vertex_layouts_report();
    cmenu_report();
    /* The parent has joined GPU work. Drop any material-bank binding before
     * releasing its entries; this main program outlives both owners. */
    C3D_BindProgram(&program);
    char effect_line[256];
    snprintf(effect_line,sizeof(effect_line),"PICA GPU fur: objects=%u draws=%u uniform_rows=%u groom_draws=%u CPU_vertices=0\n",fur_gpu_objects,fur_gpu_draws,fur_gpu_rows,groom_draws);fh_platform_log(effect_line);
    snprintf(effect_line,sizeof(effect_line),"PICA fur: objects=%u draws=%u indices=%u unsupported=%u submit_ticks=%llu CPU_vertices=0 shell_limit=15 indirect=0 groups=%u source_shells=%u omitted_shells=%u\n",fur_objects,fur_draws,fur_indices,fur_unsupported,(unsigned long long)fur_ticks,fur_groups,fur_source_shells,fur_omitted_shells);
    fh_platform_log(effect_line);
    snprintf(effect_line,sizeof(effect_line),"PICA compact shadows: objects=%u draws=%u saved_draws=%u vertex_bytes=24\n",compact_shadow_objects,compact_shadow_draws,compact_shadow_saved);
    fh_platform_log(effect_line);
    if(fh_pica_scene_actors){fh_pica_shadow_project_shutdown();fh_pica_receiver_shutdown();}
    snprintf(effect_line,sizeof(effect_line),"PICA receiver quad rejected: %u\n",receiver_rejected);fh_platform_log(effect_line);
    fh_pica_drops_shutdown();
    fh_pica_weather_shutdown();
    fh_pica_lightning_shutdown();
    fh_pica_height_fog_shutdown();
    fh_pica_color_fade_report();
    fh_pica_morph_shutdown();
    snprintf(effect_line,sizeof(effect_line),"PICA planar shadows: objects=%u draws=%u indices=%u worker=0 resident_mesh=1\n",shadow_objects,shadow_draws,shadow_indices);
    fh_platform_log(effect_line);
    fh_pica_fog_report();
    fh_native_ground_receiver_report();
    printf("PICA unimplemented: actor_callbacks=%u secondary_world_passes=%u\n",fh_pica_unimplemented_actor_callbacks,fh_pica_unimplemented_world_passes);
    {char line[160];snprintf(line,sizeof(line),"PICA normal ownership: CPU_calls_avoided=%u groups=%u GPU_skinning=1\n",fh_pica_cpu_normal_calls_avoided,fh_pica_cpu_normal_groups_avoided);fh_platform_log(line);}
    if(!binary)return;
    char line[224];snprintf(line,sizeof(line),"PICA scene: blocks=%u draws=%u indices=%u submit_ticks=%llu shader_bytes=%u\n",blocks,draws,indices,(unsigned long long)ticks,(unsigned)(fh_pica_scene_actors?sizeof(pica_scene_actor_hud_shader):fh_pica_scene_textured?sizeof(pica_scene_texture_shader):sizeof(pica_scene_shader)));fh_platform_log(line);
    if(fh_pica_scene_actors)fh_pica_splash_shutdown();
    fh_pica_hud_shutdown();
    if(fh_pica_scene_actors)fh_pica_hologram_shutdown();
    if(fh_pica_scene_actors)fh_pica_stars_shutdown();
    if(fh_pica_scene_actors)fh_pica_glow_shutdown();
    fh_pica_diffuse_map_shutdown();
    fh_pica_specular_shutdown();
    fh_pica_materials_shutdown();
    extern void fh_pica_movie_shutdown(void);fh_pica_movie_shutdown();
    fh_pica_environment_shutdown();
    extern unsigned long long fh_program_publication_stats[5];
    snprintf(line,sizeof(line),"PICA program publication: configurations=%llu words=%llu vsh_requests=%llu gsh_requests=%llu invalid=%llu\n",
        fh_program_publication_stats[0],fh_program_publication_stats[1],fh_program_publication_stats[2],fh_program_publication_stats[3],fh_program_publication_stats[4]);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA pipeline: publications=%u ambient_handoffs=%u\n",pipeline_publications,ambient_handoffs);fh_platform_log(line);
    if(fh_pica_scene_actors){snprintf(line,sizeof(line),"PICA lighting: objects=%u enabled=%u lights=%u max_active=%u partial_lights=%u\n",lighting_objects,lighting_enabled,lighting_lights,lighting_max,lighting_partial);fh_platform_log(line);}
    if(fh_pica_scene_actors){snprintf(line,sizeof(line),"PICA alpha lighting: objects=%u resident_uniforms=1\n",lighting_alpha_objects);fh_platform_log(line);}
    snprintf(line,sizeof(line),"PICA actor fog: draws=%u pipeline_reuses=%u\n",actor_fog_draws,actor_fog_pipeline_reuses);fh_platform_log(line);
    {FhActorFogResidentStats f=fh_actor_fog_resident_stats();char fog_line[320];
     snprintf(fog_line,sizeof(fog_line),"Actor fog residency: cpu=%u gpu=%u cpu_peak=%u gpu_peak=%u records=%u publications=%u rejected=%u allocation_failures=%u streamed=%llu\n",
        f.cpu_bytes,f.gpu_bytes,f.cpu_peak,f.gpu_peak,f.records,f.publications,f.rejected,f.allocation_failures,(unsigned long long)f.streamed_bytes);fh_platform_log(fog_line);}
    if(fh_pica_scene_actors){snprintf(line,sizeof(line),"PICA actor geometry: actors=%u draws=%u indices=%u bone_rows=%u submit_ticks=%llu\n",actors,actor_draws,actor_indices,actor_bones*3,(unsigned long long)actor_ticks);fh_platform_log(line);}
    if(fh_pica_scene_actors){snprintf(line,sizeof(line),"PICA actor publication: requested_rows=%u written_rows=%u shared_state=%u\n",actor_bones*3,actor_bones_written*3,actor_state_publications);fh_platform_log(line);}
    /* All four instances borrow one bank. Parent GPU join and unbinding
     * precede freeing any descriptor; release the bank after every instance. */
    if(ambient_binary){
        snprintf(line,sizeof(line),"PICA cloud shader: draws=%u instructions=183 CPU_vertices=0\n",cloud_shader_draws);fh_platform_log(line);
        snprintf(line,sizeof(line),"PICA world CPU near clip: draws=%u kept=%u clipped=%u dropped=%u fallbacks=%u large=%u full=%u\n",
            world_near_draws,world_near_kept,world_near_clipped,world_near_dropped,world_near_fallbacks,world_near_large,world_near_full);fh_platform_log(line);
        snprintf(line,sizeof(line),"PICA sky depth: original=%d skipped=%u stripped=%u\n",sky_depth_original(),sky_depth_skipped,sky_depth_stripped);fh_platform_log(line);
        snprintf(line,sizeof(line),"PICA cloud near clip: draws=%u failures=%u depth=1000\n",cloud_clip_draws,cloud_clip_failures);fh_platform_log(line);
        snprintf(line,sizeof(line),"PICA UI shader: draws=%u instructions=43 CPU_vertices=0\n",ui_draws);fh_platform_log(line);
        snprintf(line,sizeof(line),"PICA ambient shader: draws=%u indices=%u instructions=58 CPU_vertices=0\n",ambient_draws,ambient_indices);fh_platform_log(line);
        shaderProgramFree(&cloud_program);shaderProgramFree(&ui_program);
        shaderProgramFree(&ambient_program);shaderProgramFree(&ambient_ordinary_program);
        DVLB_Free(ambient_binary);ambient_binary=NULL;cloud_active=ui_active=ambient_active=0;
    }
    if(actor_fog_binary){
        shaderProgramFree(&actor_fog_program);DVLB_Free(actor_fog_binary);actor_fog_binary=NULL;
        actor_fog_active=0;
    }
    if(fur_binary){shaderProgramFree(&fur_program);DVLB_Free(fur_binary);fur_binary=NULL;}
    if(height_program){fh_pica_material_shader_release(FH_MATERIAL_SHADER_HEIGHT_FOG);height_program=NULL;}
    if(layered_program){snprintf(line,sizeof(line),"PICA layered shader: draws=%u instructions=%u shared_entries=3\n",layered_draws,fh_pica_material_shader_instructions());fh_platform_log(line);fh_pica_material_shader_release(FH_MATERIAL_SHADER_ACTOR);layered_program=NULL;}
    shaderProgramFree(&program);DVLB_Free(binary);binary=NULL;bound=0;
}

unsigned fh_pica_scene_shadow_capture(const FhSceneAsset*asset,const float*poses,unsigned joints,
    const float world[3][4],const float view[3][4],const float projection[4][4],unsigned*indices){
    const FhPicaModelPlan*plan=fh_native_scene_pica_model(asset);
    if(!plan){*indices=0;return 0;}
    const unsigned clip[4]={0,0,FH_SHADOW_IMAGE_SIZE,FH_SHADOW_IMAGE_SIZE};
    shadow_active=2;shadow_alpha=255;fh_pica_scene_invalidate();
    unsigned draws=fh_pica_scene_actor_draw(asset,plan,poses,joints,world,view,projection,clip,0,1,indices);
    shadow_active=0;fh_pica_scene_invalidate();fh_pica_materials_invalidate();
    return draws;
}
