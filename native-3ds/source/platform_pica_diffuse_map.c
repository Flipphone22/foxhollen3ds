/* Native reproduction of source Rcp mode-0 diffuse targets. Immutable map
 * geometry, live light uniforms, one draw per changed target; no CPU vertices,
 * readback, private queue, game callback replay or per-update allocations.
 * Exact duplicate source slots share a result; no approximate light reuse. */
#include <3ds.h>
#include <citro3d.h>
#include "fh_pica_actor_stream.h"
static FhPicaSamplerCommands map_commands[3];
#include <string.h>
#include <stdio.h>
#include "fh_pica_diffuse_map.h"
#include "fh_pica_diffuse_slots.h"
#include "fh_pica_specular_map.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_pica_material_shader.h"
extern void fh_minimal_auxiliary_begin(void);
extern void fh_minimal_auxiliary_end(unsigned draws,unsigned indices);
static FhPicaSpecularMapMesh *mesh;
static C3D_Tex maps[3];
static C3D_FrameBuf targets[3];
static C3D_AttrInfo attributes;
static C3D_BufInfo buffers;
static shaderProgram_s *program;
static FhPicaDiffuseSlots cache;
static unsigned updates,reuses,shared,light_histogram[9];
static u64 update_ticks;
void fh_pica_diffuse_map_prepare(void){
    if(mesh)fh_platform_panic("PICA diffuse duplicate preparation");
    extern u32 __ctru_linear_heap_size;
    unsigned bytes=sizeof(*mesh),available=linearSpaceFree();
    if(available<bytes || (u64)(available-bytes)*100<(u64)__ctru_linear_heap_size*15 ||
       vramSpaceFree()<3*4096+1024*1024)fh_platform_panic("PICA diffuse resident budget");
    mesh=linearMemAlign(bytes,128);
    if(!mesh)fh_platform_panic("PICA diffuse mesh allocation");
    fh_pica_specular_map_prepare(mesh);fh_gpu_cache_flush(mesh,bytes);
    for(unsigned i=0;i<3;i++){
        if(!C3D_TexInitVRAM(maps+i,32,32,GPU_RGBA8))fh_platform_panic("PICA diffuse map allocation");
        C3D_TexSetFilter(maps+i,GPU_LINEAR,GPU_LINEAR);
        C3D_TexSetWrap(maps+i,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
        if(!fh_pica_actor_sampler_prepare(map_commands+i,maps+i))fh_platform_panic("PICA light map commands");
        C3D_FrameBufTex(targets+i,maps+i,GPU_TEXFACE_2D,0);
        C3D_FrameBufDepth(targets+i,NULL,GPU_RB_DEPTH24);
    }
    program=fh_pica_material_shader_acquire(FH_MATERIAL_SHADER_DIFFUSE);
    if(shaderInstanceGetUniformLocation(program->vertexShader,"ambient")!=0 ||
       shaderInstanceGetUniformLocation(program->vertexShader,"light_position")!=1 ||
       shaderInstanceGetUniformLocation(program->vertexShader,"light_color")!=9)
        fh_platform_panic("PICA diffuse uniform ABI");
    for(unsigned i=0;i<8;i++){
        char name[24];snprintf(name,sizeof(name),"light%u_active",i);
        if(shaderInstanceGetUniformLocation(program->vertexShader,name)!=(int)(0x68+i))
            fh_platform_panic("PICA diffuse light boolean ABI");
    }
    AttrInfo_Init(&attributes);AttrInfo_AddLoader(&attributes,0,GPU_FLOAT,2);AttrInfo_AddLoader(&attributes,1,GPU_FLOAT,3);
    BufInfo_Init(&buffers);
    if(BufInfo_Add(&buffers,mesh->vertices,sizeof(mesh->vertices[0]),2,0x10)<0)fh_platform_panic("PICA diffuse mesh binding");
    memset(&cache,0,sizeof(cache));memset(light_histogram,0,sizeof(light_histogram));updates=reuses=shared=0;update_ticks=0;
}
void fh_pica_diffuse_map_update(unsigned mask,const FhPicaDiffuseMapState state[3]){
    if(mask&~7u || (mask&&!state))fh_platform_panic("PICA diffuse slot mask");
    for(unsigned i=0;i<3;i++)if(mask&(1u<<i)){
        if(state[i].count>8)fh_platform_panic("PICA diffuse light count");
        if((cache.valid&(1u<<i)) && !memcmp(cache.state+cache.slot[i],state+i,sizeof(*state)))reuses++;
    }
    FhPicaDiffuseAssignment assignment;fh_pica_diffuse_assign(&cache,mask,state,&assignment);
    for(unsigned i=0;i<3;i++)if(mask&(1u<<i))for(unsigned j=0;j<i;j++)
        if((assignment.valid&(1u<<j)) && assignment.slot[i]==assignment.slot[j]){shared++;break;}
    if(!assignment.writes){memcpy(cache.slot,assignment.slot,sizeof(cache.slot));cache.valid=assignment.valid;return;}
    if(!mesh)fh_platform_panic("PICA diffuse not prepared");
    u64 start=svcGetSystemTick();fh_minimal_auxiliary_begin();
    C3D_BindProgram(program);C3D_SetAttrInfo(&attributes);C3D_SetBufInfo(&buffers);
    C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    C3D_SetViewport(0,0,32,32);C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
    C3D_CullFace(GPU_CULL_NONE);C3D_DepthMap(true,-1,0);
    C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_PRIMARY_COLOR,0,0);
    for(unsigned i=0;i<3;i++)C3D_TexBind(i,NULL);
    unsigned count=0;
    for(unsigned i=0;i<3;i++)if(assignment.writes&(1u<<i)){
        const FhPicaDiffuseMapState *s=state+assignment.source[i];
        /* Original GX channel is disabled when the selected light mask is
         * empty: white material passes through; ambient is not applied. */
        C3D_FVUnifSet(GPU_VERTEX_SHADER,0,s->count?s->ambient[0]:1,s->count?s->ambient[1]:1,s->count?s->ambient[2]:1,1);
        /* No disabled-light normalization or stale-uniform dependency. The
         * shared SDK owns boolean publication; the auxiliary boundary restores
         * the next scene owner's full shader state. No raw register writes. */
        for(unsigned j=0;j<8;j++)C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68+j,j<s->count);
        for(unsigned j=0;j<s->count;j++){
            const float *p=s->position[j],*c=s->color[j];
            C3D_FVUnifSet(GPU_VERTEX_SHADER,1+j,p[0],p[1],p[2],0);
            C3D_FVUnifSet(GPU_VERTEX_SHADER,9+j,c[0],c[1],c[2],0);
        }
        light_histogram[s->count]++;
        C3D_SetFrameBuf(targets+i);
        C3D_DrawElements(GPU_TRIANGLES,FH_SPECULAR_MAP_INDICES,C3D_UNSIGNED_SHORT,mesh->indices);
        cache.state[i]=*s;count++;
    }
    /* Follow pinned Melee/SDK explicit publication and the verified main
     * auxiliary owner. All writes and later samples share one ordered queue. */
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    fh_minimal_auxiliary_end(count,count*FH_SPECULAR_MAP_INDICES);
    cache.resident|=assignment.writes;memcpy(cache.slot,assignment.slot,sizeof(cache.slot));cache.valid=assignment.valid;
    updates+=count;update_ticks+=svcGetSystemTick()-start;
}
const C3D_Tex *fh_pica_diffuse_map_sampler(unsigned slot){
    return slot<3 && (cache.valid&(1u<<slot))?maps+cache.slot[slot]:NULL;
}
void fh_pica_diffuse_map_shutdown(void){
    if(!mesh)return;
    char line[256];snprintf(line,sizeof(line),"PICA diffuse maps: updates=%u reuses=%u draws=%u submit_ticks=%llu linear_bytes=%u vram_bytes=%u CPU_vertices=0 readbacks=0\n",updates,reuses,updates,(unsigned long long)update_ticks,(unsigned)sizeof(*mesh),3*4096);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA diffuse slot sharing: exact_aliases=%u state_bytes=%u\n",shared,(unsigned)sizeof(cache));fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA diffuse light counts: %u %u %u %u %u %u %u %u %u\n",light_histogram[0],light_histogram[1],light_histogram[2],light_histogram[3],light_histogram[4],light_histogram[5],light_histogram[6],light_histogram[7],light_histogram[8]);fh_platform_log(line);
    for(unsigned i=0;i<3;i++)C3D_TexDelete(maps+i);
    linearFree(mesh);mesh=NULL;fh_pica_material_shader_release(FH_MATERIAL_SHADER_DIFFUSE);program=NULL;memset(&cache,0,sizeof(cache));
}

const FhPicaSamplerCommands *fh_pica_diffuse_commands(unsigned slot){return slot<3 && (cache.valid&(1u<<slot))?map_commands+cache.slot[slot]:NULL;}
