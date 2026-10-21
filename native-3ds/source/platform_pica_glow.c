#include "fh_pica_options.h"
/* Native Expgfx instances: one live record, four GPU-emitted corners.
 * Bounded append-only segments retain every queued GPU reader until retirement. */
#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_pica_glow.h"
#include "fh_pica_fog.h"
#include "fh_pica_scene.h"
#include "fh_pica_hud.h"
#include "fh_pica_image.h"
#include "fh_pica_viewport.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"
#include "fh_graphics_options.h"
#include "fh_minimal_graphics.h"

enum {SEGMENTS=3, CAPACITY=1024, DYNAMIC_CAPACITY=256};
typedef FhPicaGlowParticle Particle;
typedef struct {Particle particle;int16_t corners[4][3];} DynamicParticle;
_Static_assert(sizeof(DynamicParticle)==60,"Dynamic particle instance ABI");
_Static_assert(sizeof(Particle)==36,"Native particle instance ABI");
_Static_assert(__builtin_offsetof(Particle,angles)==16 && __builtin_offsetof(Particle,color)==24 &&
               __builtin_offsetof(Particle,uv)==28,"Native particle attribute offsets");
static unsigned stream_capacity,stream_discard,stream_frame;
static Particle *instances;
static DynamicParticle *dynamic_instances;
static unsigned dynamic_segment,dynamic_used,dynamic_marker,dynamic_last[SEGMENTS];
static unsigned dynamic_first[SEGMENTS],dynamic_end[SEGMENTS],batch_dynamic,bound_dynamic;
static unsigned dynamic_quads,dynamic_draws,dynamic_wraps,dynamic_drains,dynamic_frame_instances,dynamic_peak;
static unsigned segment,used,frame_marker,last_used[SEGMENTS];
static unsigned dirty_first[SEGMENTS],dirty_end[SEGMENTS],publications;
static unsigned segment_wraps,overwrite_drains,instance_bytes,peak_frame_instances,frame_instances;
static shaderProgram_s program,dynamic_program;
static float templates[8][4],view[3][4],projection[4][4];
static const float shapes[4][4]={{1,1,1,0},{1,8,1,0},{1,1,32,1},{32,1,1,0}};
static unsigned ready,active,count,page,raster,scissor[4];
static unsigned grouping,group_frame,source_clip[4],groups,joined_pools,retirement_flushes;
static float fade,near_z,far_z;
static unsigned pools,quads,draws,rejected[2];
static unsigned bound,camera_dirty,clip_dirty,last_page,last_raster;
static float last_near,last_far,uv_scale[2],last_fog_scale;
static float bounds[4][2],planes[4][5];
static unsigned submitted,culled,disabled;
static unsigned template_publications,camera_publications;
/* Sole native geometry program: Citro3D's c0..20 CPU shadow belongs to us.
 * DEVICE values do not survive ordinary VSH draws: with GS disabled and
 * non-exclusive configuration, PICA mirrors VSH uniform writes into GS.
 * Re-dirty the shadow on rebind; only changed live data needs CPU rebuilding.
 * Program constants are republished by shader binding. */
void fh_pica_glow_invalidate(void){bound=0;clip_dirty=1;}
void fh_pica_glow_publish(void){
    if(stream_capacity)fh_platform_panic("PICA glow active stream lease");

    for(unsigned s=0;s<SEGMENTS;s++)if(dirty_end[s]>dirty_first[s]){
        unsigned bytes=(dirty_end[s]-dirty_first[s])*sizeof(Particle);
        fh_gpu_cache_flush(instances+s*CAPACITY+dirty_first[s],bytes);
        instance_bytes+=bytes;publications++;dirty_first[s]=dirty_end[s];
    }
    for(unsigned s=0;s<SEGMENTS;s++)if(dynamic_end[s]>dynamic_first[s]){
        unsigned bytes=(dynamic_end[s]-dynamic_first[s])*sizeof(DynamicParticle);
        fh_gpu_cache_flush(dynamic_instances+s*DYNAMIC_CAPACITY+dynamic_first[s],bytes);
        instance_bytes+=bytes;publications++;dynamic_first[s]=dynamic_end[s];
    }
}

#include "platform_pica_glow_reflected.h"
#include "platform_pica_glow_adaptive.h"
void fh_pica_glow_init(void *entry){
    glow_reflected_init();
    glow_adaptive_init();
    shaderProgramInit(&program);shaderProgramSetVsh(&program,(DVLE_s*)entry);
    shaderProgramSetGsh(&program,(DVLE_s*)entry+1,4);
    shaderProgramInit(&dynamic_program);shaderProgramSetVsh(&dynamic_program,(DVLE_s*)entry+2);
    shaderProgramSetGsh(&dynamic_program,(DVLE_s*)entry+1,7);
    /* libctru reports b0 as register 0x78 minus float base 0x10. */
    if(shaderInstanceGetUniformLocation(program.geometryShader,"dynamic_quad")!=104)
        fh_platform_panic("PICA dynamic particle bool ABI");
    shaderInstanceSetBool(program.geometryShader,0,false);
    shaderInstanceSetBool(dynamic_program.geometryShader,0,true);
    if(shaderInstanceGetUniformLocation(program.geometryShader,"templates")!=7 ||
       shaderInstanceGetUniformLocation(program.geometryShader,"shapes")!=15 ||
       shaderInstanceGetUniformLocation(program.geometryShader,"fade")!=19 ||
       shaderInstanceGetUniformLocation(program.geometryShader,"uv_scale")!=20)
        fh_platform_panic("PICA glow uniform ABI");
    extern u32 __ctru_linear_heap_size;
    unsigned bytes=SEGMENTS*(CAPACITY*sizeof(Particle)+DYNAMIC_CAPACITY*sizeof(DynamicParticle));
    if(bytes>linearSpaceFree() || linearSpaceFree()-bytes<__ctru_linear_heap_size*15/100)
        fh_platform_panic("PICA glow linear reserve");
    instances=linearMemAlign(bytes,128);
    if(!instances)fh_platform_panic("PICA glow instance allocation");
    dynamic_instances=(DynamicParticle*)(instances+SEGMENTS*CAPACITY);
}
void fh_pica_glow_templates(const int16_t a[4][3],const int16_t b[4][3]){
    if(!fh_pica_scene_actors || ready)return;
    for(unsigned i=0;i<8;i++)for(unsigned c=0;c<3;c++)templates[i][c]=(i<4?a:b)[i%4][c];
    /* GPU reconstructs the four signed-short shapes. Bounds still derive once
     * from the exact narrowed source coordinates, never from unclamped scale. */
    for(unsigned s=0;s<4;s++)for(unsigned t=0;t<2;t++)for(unsigned c=0;c<4;c++){
        const int16_t *base=(t?b:a)[c];float p[3];
        for(unsigned k=0;k<3;k++)p[k]=(int16_t)(base[s==2?(k==0?2:k==2?0:1):k]*(int)shapes[s][k]);
        float radius=sqrtf(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
        if(radius>bounds[s][t])bounds[s][t]=radius;
    }
    ready=1;
}
static void flush(void){
    if(stream_capacity)fh_platform_panic("PICA glow active stream lease");

    if(!count)return;
    unsigned reflected=!!(raster&4);
    const C3D_Tex *reflection_mask=NULL,*reflection_capture=NULL;
    if(reflected){
        reflection_mask=fh_pica_hud_glow_mask_sampler(page);
        reflection_capture=fh_pica_glow_reflection_image();
        if(!reflection_mask || !reflection_capture){glow_reflected_missing+=count;count=0;return;}
    }
    if(fh_graphics_options&FH_GRAPHICS_NO_RENDER){count=0;return;}
    static unsigned options_seen;
    if(options_seen!=fh_graphics_options){bound=0;options_seen=fh_graphics_options;}
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    fh_pica_scene_reserve();
    if(reflected && glow_adaptive_flush(reflection_mask,reflection_capture))return;
    if(!bound || bound_dynamic!=batch_dynamic || bound_reflected!=reflected){
        fh_pica_scene_invalidate();C3D_BindProgram(reflected?glow_reflected_program+!!batch_dynamic:(batch_dynamic?&dynamic_program:&program));
        C3D_FVUnifWritePtr(GPU_GEOMETRY_SHADER,0,reflected?24:21);
        glow_reflected_matrix_dirty=1;
        C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
        C3D_TexEnvBufUpdate(C3D_Both,0);
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);C3D_CullFace(GPU_CULL_NONE);
        C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
        AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_SHORT,4);
        AttrInfo_AddLoader(a,2,GPU_UNSIGNED_BYTE,4);AttrInfo_AddLoader(a,3,GPU_SHORT,4);
        if(batch_dynamic)for(unsigned i=4;i<7;i++)AttrInfo_AddLoader(a,i,GPU_SHORT,4);
        C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
        if(BufInfo_Add(b,batch_dynamic?(void*)dynamic_instances:(void*)instances,
            batch_dynamic?sizeof(DynamicParticle):sizeof(Particle),batch_dynamic?7:4,
            batch_dynamic?0x6543210:0x3210)<0)fh_platform_panic("PICA glow instance ABI");
        if(!template_publications){
            for(unsigned r=0;r<8;r++)C3D_FVUnifSet(GPU_GEOMETRY_SHADER,7+r,templates[r][0],templates[r][1],templates[r][2],0);
            for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_GEOMETRY_SHADER,15+r,shapes[r][0],shapes[r][1],shapes[r][2],shapes[r][3]);
            template_publications++;
        }
        fh_pica_fog_invalidate();last_fog_scale=-1;
        last_page=last_raster=~0u;bound=1;bound_dynamic=batch_dynamic;bound_reflected=reflected;
    }
    /* Source drawGlow applies _gxSetFogParams before drawing the pool.
     * Native range/color are published by fogSetRange/fogSetColor already.
     * Restore after our TEV reset, including OFF and ordinary/GS handoffs. */
    unsigned uv_dirty=last_page!=page || (reflected && last_raster!=raster);
    if(uv_dirty){
    uv_scale[0]=uv_scale[1]=1;unsigned intensity=0;
    if(reflected){glow_reflected_bind(raster,reflection_mask,reflection_capture);last_page=page;}
    else {
    const C3D_Tex *texture=page?fh_pica_hud_sampler(page,uv_scale,&intensity):NULL;
    C3D_TexEnv *e=C3D_GetTexEnv(0);
    C3D_TexEnvInit(e);
    C3D_TexEnvSrc(e,C3D_Both,GPU_PRIMARY_COLOR,GPU_TEXTURE0,0);
    C3D_TexEnvFunc(e,C3D_Both,page?GPU_MODULATE:GPU_REPLACE);
    if(intensity)C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
    C3D_TexBind(0,fh_pica_options_texture(texture,FH_GRAPHICS_NO_PARTICLE_TEX));last_page=page;
    }
    }
    fh_pica_fog_bind(1);
    float fog_scale=fh_pica_fog_distance_scale();
    if(uv_dirty || last_fog_scale!=fog_scale){
        C3D_FVUnifSet(GPU_GEOMETRY_SHADER,20,uv_scale[0],uv_scale[1],fog_scale,0);
        last_fog_scale=fog_scale;
    }
    if(clip_dirty){C3D_SetScissor(GPU_SCISSOR_NORMAL,scissor[0],scissor[1],scissor[2],scissor[3]);clip_dirty=0;}
    if(last_raster==~0u || last_near!=near_z || last_far!=far_z){
        float depth[2];fh_pica_viewport_depth(depth,near_z,far_z);C3D_DepthMap(true,depth[0],depth[1]);last_near=near_z;last_far=far_z;
    }
    if(last_raster!=raster){
    C3D_DepthTest(true,GPU_GEQUAL,(raster&3)==0?GPU_WRITE_ALL:GPU_WRITE_COLOR);
    C3D_AlphaTest((raster&3)==0,GPU_GREATER,254);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,(raster&3)==0?GPU_ONE:GPU_SRC_ALPHA,
        (raster&3)==0?GPU_ZERO:(raster&3)==1?GPU_ONE:GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
    last_raster=raster;
    }
    if(camera_dirty){
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_GEOMETRY_SHADER,r,view[r][0],view[r][1],view[r][2],view[r][3]);
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_GEOMETRY_SHADER,3+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
    C3D_FVUnifSet(GPU_GEOMETRY_SHADER,19,fade,1.f/(-fade-2.5f),1.f/255,1.f/256);camera_dirty=0;camera_publications++;
    }
    if(reflected && glow_reflected_matrix_dirty){
        for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_GEOMETRY_SHADER,21+r,
            glow_reflected_matrix[r][0],glow_reflected_matrix[r][1],glow_reflected_matrix[r][2],glow_reflected_matrix[r][3]);
        glow_reflected_matrix_dirty=0;
    }
    if(reflected)glow_reflected_draws++;
    unsigned first=batch_dynamic?dynamic_segment*DYNAMIC_CAPACITY+dynamic_used-count:segment*CAPACITY+used-count;
    C3D_DrawArrays(GPU_GEOMETRY_PRIM,first,count);
    if(batch_dynamic){dynamic_last[dynamic_segment]=fh_minimal_frames+1;dynamic_draws++;}
    else last_used[segment]=fh_minimal_frames+1;
    fh_pica_hud_account(count*6);draws++;count=0;
}
static void acquire_segment(void){
    if(count)fh_platform_panic("PICA glow unsubmitted segment");
    segment=(segment+1)%SEGMENTS;
    if(last_used[segment]){
        overwrite_drains+=last_used[segment]==fh_minimal_frames+1;
        fh_minimal_prepare_write(last_used[segment]);
    }
    if(dirty_first[segment]!=dirty_end[segment])fh_platform_panic("PICA glow unpublished reuse");
    dirty_first[segment]=dirty_end[segment]=0;
    used=0;
}
void fh_pica_glow_begin(const float v[3][4],const float p[4][4],float f,const unsigned clip[4]){
    if(active || (count&&!grouping) || !ready || !isfinite(f) || f==-2.5f ||
       (grouping && group_frame!=fh_minimal_frames))fh_platform_panic("PICA glow pool state");
    unsigned reflected_changed=(fh_graphics_effects&FH_EFFECT_PARTICLE_REFLECTION) &&
        memcmp(glow_reflected_matrix,gCameraLightPerspectiveFlipYMatrix,sizeof(glow_reflected_matrix));
    unsigned changed=reflected_changed || fade!=f || memcmp(view,v,sizeof(view)) || memcmp(projection,p,sizeof(projection));
    /* Submit with the PREVIOUS camera/target/scissor before changing any state.
     * Group callers guarantee no interleaved HUD or other native pipeline. */
    if(count && (changed || memcmp(source_clip,clip,sizeof(source_clip))))flush();
    if(count)joined_pools++;
    fh_pica_hud_flush();
    if(changed){
        if(reflected_changed){memcpy(glow_reflected_matrix,gCameraLightPerspectiveFlipYMatrix,sizeof(glow_reflected_matrix));glow_reflected_matrix_dirty=1;}
        memcpy(view,v,sizeof(view));memcpy(projection,p,sizeof(projection));fade=f;camera_dirty=1;
        /* Four homogeneous side planes; deliberately no near/far rejection.
         * L1 plane norms enclose the sphere with an extra conservative cube. */
        for(unsigned i=0;i<4;i++){
            unsigned axis=i/2;float sign=(i&1)?-1:1;
            float eye[4];for(unsigned k=0;k<4;k++)eye[k]=p[3][k]+sign*p[axis][k];
            for(unsigned c=0;c<4;c++)planes[i][c]=eye[0]*v[0][c]+eye[1]*v[1][c]+eye[2]*v[2][c]+(c==3?eye[3]:0);
            planes[i][4]=fabsf(planes[i][0])+fabsf(planes[i][1])+fabsf(planes[i][2]);
        }
    }
    unsigned next_clip[4];active=fh_pica_hud_target(clip,next_clip)?1:2;
    if(active==1 && memcmp(scissor,next_clip,sizeof(scissor))){memcpy(scissor,next_clip,sizeof(scissor));clip_dirty=1;}pools++;
    memcpy(source_clip,clip,sizeof(source_clip));
}

static void acquire_dynamic(void){
    if(count)fh_platform_panic("PICA dynamic unsubmitted segment");
    dynamic_segment=(dynamic_segment+1)%SEGMENTS;
    if(dynamic_last[dynamic_segment]){
        dynamic_drains+=dynamic_last[dynamic_segment]==fh_minimal_frames+1;
        fh_minimal_prepare_write(dynamic_last[dynamic_segment]);
    }
    if(dynamic_first[dynamic_segment]!=dynamic_end[dynamic_segment])fh_platform_panic("PICA dynamic unpublished reuse");
    dynamic_first[dynamic_segment]=dynamic_end[dynamic_segment]=dynamic_used=0;
}
static int visible_dynamic(const Particle *particle,const int16_t corners[4][3]){
    /* Source already owns animated local corners. Only a conservative bound
     * is computed here; PICA performs every world/view/projection transform. */
    float radius=0;
    for(unsigned i=0;i<4;i++){
        float r=abs((int)corners[i][0])+abs((int)corners[i][1])+abs((int)corners[i][2]);
        if(r>radius)radius=r;
    }
    radius=radius*fabsf(particle->position[3])*1.0625f+1;
    for(unsigned i=0;i<4;i++){
        const float *p=planes[i];
        if(p[0]*particle->position[0]+p[1]*particle->position[1]+p[2]*particle->position[2]+p[3]<-radius*p[4])return 0;
    }
    return 1;
}
void fh_pica_glow_dynamic(unsigned image,unsigned mode,float n,float f,const Particle *particle,const int16_t corners[4][3]){
    if(!active || ((mode&~12u)>2 || ((mode&8)&&!(mode&4))))fh_platform_panic("PICA dynamic material state");
    if(active==2)return;
    submitted++;
    if((fh_graphics_options&FH_GRAPHICS_NO_RENDER) || !(fh_graphics_options&FH_GRAPHICS_PARTICLES)){disabled++;return;}
    if(!visible_dynamic(particle,corners)){culled++;return;}
    if(!batch_dynamic){flush();batch_dynamic=1;}
    if(dynamic_marker!=fh_minimal_frames+1){acquire_dynamic();dynamic_marker=fh_minimal_frames+1;dynamic_frame_instances=0;}
    if(count && (page!=image || raster!=mode || near_z!=n || far_z!=f))flush();
    if(dynamic_used==DYNAMIC_CAPACITY){flush();acquire_dynamic();dynamic_wraps++;}
    if(!count){page=image;raster=mode;near_z=n;far_z=f;}
    DynamicParticle *dst=dynamic_instances+dynamic_segment*DYNAMIC_CAPACITY+dynamic_used;
    dst->particle=*particle;dst->particle.angles[3]=0;memcpy(dst->corners,corners,sizeof(dst->corners));
    if(dynamic_first[dynamic_segment]==dynamic_end[dynamic_segment])dynamic_first[dynamic_segment]=dynamic_used;
    dynamic_end[dynamic_segment]=++dynamic_used;count++;quads++;dynamic_quads++;dynamic_frame_instances++;
    if(dynamic_frame_instances>dynamic_peak)dynamic_peak=dynamic_frame_instances;
}

/* A lease covers only unused words. No GPU consumer may see it until commit.
 * Only the ordered source producer may hold a lease; no renderer reentry. */

static unsigned stream_leases,stream_compacted,stream_direct;
Particle *fh_pica_glow_stream_begin(unsigned image,unsigned mode,float n,float f,unsigned *capacity){
    if(stream_capacity || !active || ((mode&~12u)>2 || ((mode&8)&&!(mode&4))) || !capacity)fh_platform_panic("PICA glow stream begin");
    stream_discard=active==2?2:
        ((fh_graphics_options&FH_GRAPHICS_NO_RENDER) || !(fh_graphics_options&FH_GRAPHICS_PARTICLES));
    if(stream_discard){stream_capacity=*capacity=32;stream_frame=fh_minimal_frames;return NULL;}
    if(batch_dynamic){flush();batch_dynamic=0;}
    if(frame_marker!=fh_minimal_frames+1){
        acquire_segment();frame_marker=fh_minimal_frames+1;frame_instances=0;
    }
    if(count && (page!=image || raster!=mode || near_z!=n || far_z!=f))flush();
    if(used==CAPACITY){flush();acquire_segment();segment_wraps++;}
    page=image;raster=mode;near_z=n;far_z=f;
    unsigned available=CAPACITY-used;
    stream_capacity=*capacity=available<32?available:32;
    stream_frame=fh_minimal_frames;stream_leases++;
    return instances+segment*CAPACITY+used;
}
void fh_pica_glow_stream_commit(unsigned length){
    if(!stream_capacity || length>stream_capacity || stream_frame!=fh_minimal_frames)
        fh_platform_panic("PICA glow stream commit");
    stream_capacity=0;
    if(stream_discard==2)return;
    submitted+=length;
    if(stream_discard){disabled+=length;return;}
    Particle *base=instances+segment*CAPACITY+used;
    /* PICA performs homogeneous clipping after the existing VSH/GS.
     * Keep source order and publish each supplied static instance once.
     * Do not CPU-test four planes or copy survivors over rejected records. */
    unsigned kept=length;
    for(unsigned i=0;i<length;i++){
        unsigned shape=(unsigned)base[i].angles[3]/8;
        if(shape>3)fh_platform_panic("PICA glow stream shape");
    }
    stream_direct+=length;
    if(kept){
        if(dirty_first[segment]==dirty_end[segment])dirty_first[segment]=used;
        used+=kept;dirty_end[segment]=used;count+=kept;quads+=kept;frame_instances+=kept;
        if(frame_instances>peak_frame_instances)peak_frame_instances=frame_instances;
    }
}
void fh_pica_glow_quad(unsigned image,unsigned mode,float n,float f,const Particle *particle){
    unsigned capacity;Particle *dst=fh_pica_glow_stream_begin(image,mode,n,f,&capacity);
    if(dst)*dst=*particle;
    fh_pica_glow_stream_commit(1);
}
void fh_pica_glow_unsupported(unsigned reason){if(reason>1)fh_platform_panic("PICA glow unsupported reason");rejected[reason]++;}
void fh_pica_glow_group_begin(void){
    if(stream_capacity)fh_platform_panic("PICA glow active stream lease");

    if(grouping || active || count)fh_platform_panic("PICA glow group begin");
    fh_pica_hud_flush();grouping=1;group_frame=fh_minimal_frames;groups++;
}
void fh_pica_glow_barrier(void){
    if(stream_capacity)fh_platform_panic("PICA glow active stream lease");

    if(active)fh_platform_panic("PICA glow active retirement");
    if(count){retirement_flushes++;flush();}
}
void fh_pica_glow_group_end(void){
    if(stream_capacity)fh_platform_panic("PICA glow active stream lease");

    if(!grouping || active || group_frame!=fh_minimal_frames)fh_platform_panic("PICA glow group end");
    flush();grouping=0;
}
void fh_pica_glow_end(void){
    if(stream_capacity)fh_platform_panic("PICA glow active stream lease");
    if(!grouping)flush();
    active=0;
}
void fh_pica_glow_shutdown(void){
    if(stream_capacity)fh_platform_panic("PICA glow active stream lease");

    extern unsigned fh_glow_preparation_skipped,fh_glow_hidden_rng;
    char preparation[128];
    snprintf(preparation,sizeof(preparation),"PICA glow preparation: skipped=%u rng_retained=%u\n",
        fh_glow_preparation_skipped,fh_glow_hidden_rng);fh_platform_log(preparation);

    if(count || active || grouping)fh_platform_panic("PICA glow unfinished pool");
    char line[288];snprintf(line,sizeof(line),"PICA glow: pools=%u supplied=%u culled=%u disabled=%u quads=%u draws=%u unsupported_reflection=%u unsupported_shape=%u vertex_bytes=%u index_bytes=%u CPU_vertices=0\n",
        pools,submitted,culled,disabled,quads,draws,rejected[0],rejected[1],SEGMENTS*CAPACITY*(unsigned)sizeof(Particle),0u);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA glow instances: bytes=%u publications=%u peak_per_frame=%u segment_wraps=%u overwrite_drains=%u parameter_uniform_rows=0\n",instance_bytes,publications,peak_frame_instances,segment_wraps,overwrite_drains);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA glow resident uniforms: template_publications=%u camera_publications=%u\n",template_publications,camera_publications);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA glow groups: groups=%u joined_pools=%u retirement_flushes=%u\n",groups,joined_pools,retirement_flushes);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA dynamic glow: quads=%u draws=%u vertex_bytes=%u wraps=%u overwrite_drains=%u peak_per_frame=%u\n",dynamic_quads,dynamic_draws,SEGMENTS*DYNAMIC_CAPACITY*(unsigned)sizeof(DynamicParticle),dynamic_wraps,dynamic_drains,dynamic_peak);fh_platform_log(line);
    extern unsigned fh_expgfx_direct_pools,fh_expgfx_staged_pools;
    extern uint64_t fh_expgfx_copy_bytes;
    snprintf(line,sizeof(line),"Particle pool ownership: direct=%u staged=%u copied_bytes=%llu\n",fh_expgfx_direct_pools,fh_expgfx_staged_pools,(unsigned long long)fh_expgfx_copy_bytes);fh_platform_log(line);
    /* Host shutdown has retired every GPU reader. */
    snprintf(line,sizeof(line),"PICA glow stream: leases=%u direct=%u compacted=%u\n",stream_leases,stream_direct,stream_compacted);fh_platform_log(line);
    glow_adaptive_shutdown();
    glow_reflected_shutdown();
    linearFree(instances);shaderProgramFree(&program);shaderProgramFree(&dynamic_program);
}
