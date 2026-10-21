/* Indexed native particle distortion. Load-compiled geometry is borrowed from
 * HUD pages; live records contain only source parameters, never CPU vertices. */
#include "pica_glow_adaptive_shader.h"
static DVLB_s *glow_adaptive_binary;
static shaderProgram_s glow_adaptive_program[2];
static unsigned glow_adaptive_draws,glow_adaptive_triangles,glow_adaptive_declines;
static void glow_adaptive_init(void)
{
    glow_adaptive_binary=DVLB_ParseFile((u32*)pica_glow_adaptive_shader,sizeof(pica_glow_adaptive_shader));
    if(!glow_adaptive_binary||glow_adaptive_binary->numDVLE!=1||glow_adaptive_binary->DVLP.codeSize>512)
        fh_platform_panic("Adaptive particle shader");
    for(unsigned i=0;i<2;i++){
        shaderProgramInit(glow_adaptive_program+i);
        shaderProgramSetVsh(glow_adaptive_program+i,glow_adaptive_binary->DVLE);
        shaderInstance_s *v=glow_adaptive_program[i].vertexShader;
        const char *names[]={"view","projection","templates","shapes","fade","uv_scale","reflected","instance","distortion_scale"};
        const int locations[]={0,3,7,15,19,20,21,24,31};
        for(unsigned n=0;n<9;n++)if(shaderInstanceGetUniformLocation(v,names[n])!=locations[n])
            fh_platform_panic("Adaptive particle uniform ABI");
        shaderInstanceSetBool(v,0,i!=0);
    }
}
static const Particle *glow_adaptive_particle(unsigned first,unsigned i)
{
    return batch_dynamic?&dynamic_instances[first+i].particle:instances+first+i;
}
static int glow_adaptive_flush(const C3D_Tex *mask,const C3D_Tex *capture)
{
    unsigned first=batch_dynamic?dynamic_segment*DYNAMIC_CAPACITY+dynamic_used-count:
                                  segment*CAPACITY+used-count;
    /* Admission before submitting anything preserves order on a batch decline.
     * The projected-only path records that indirect displacement was unavailable. */
    for(unsigned i=0;i<count;i++)if(!fh_pica_hud_glow_mesh(page,glow_adaptive_particle(first,i)->uv)){
        glow_adaptive_declines+=count;return 0;
    }
    for(unsigned i=0;i<count;i++){
        unsigned reset=fh_pica_scene_reserve_changed();
        if(!i||reset){
            fh_pica_scene_invalidate();
            C3D_BindProgram(glow_adaptive_program+!!batch_dynamic);
            C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
            C3D_TexEnvBufUpdate(C3D_Both,0);C3D_CullFace(GPU_CULL_NONE);
            glow_reflected_bind(raster,mask,capture);fh_pica_fog_bind(1);
            C3D_SetScissor(GPU_SCISSOR_NORMAL,scissor[0],scissor[1],scissor[2],scissor[3]);
            float depth[2];fh_pica_viewport_depth(depth,near_z,far_z);C3D_DepthMap(true,depth[0],depth[1]);
            unsigned mode=raster&3;
            C3D_DepthTest(true,GPU_GEQUAL,mode==0?GPU_WRITE_ALL:GPU_WRITE_COLOR);
            C3D_AlphaTest(mode==0,GPU_GREATER,254);
            C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,mode==0?GPU_ONE:GPU_SRC_ALPHA,
                mode==0?GPU_ZERO:mode==1?GPU_ONE:GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
            C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddLoader(a,0,GPU_FLOAT,4);
            for(unsigned n=0;n<3;n++)C3D_FVUnifSet(GPU_VERTEX_SHADER,n,view[n][0],view[n][1],view[n][2],view[n][3]);
            for(unsigned n=0;n<4;n++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+n,projection[n][0],projection[n][1],projection[n][2],projection[n][3]);
            for(unsigned n=0;n<8;n++)C3D_FVUnifSet(GPU_VERTEX_SHADER,7+n,templates[n][0],templates[n][1],templates[n][2],0);
            for(unsigned n=0;n<4;n++)C3D_FVUnifSet(GPU_VERTEX_SHADER,15+n,shapes[n][0],shapes[n][1],shapes[n][2],shapes[n][3]);
            C3D_FVUnifSet(GPU_VERTEX_SHADER,19,fade,1.f/(-fade-2.5f),1.f/255,1.f/256);
            C3D_FVUnifSet(GPU_VERTEX_SHADER,20,1,1,fh_pica_fog_distance_scale(),0);
            for(unsigned n=0;n<3;n++)C3D_FVUnifSet(GPU_VERTEX_SHADER,21+n,
                glow_reflected_matrix[n][0],glow_reflected_matrix[n][1],glow_reflected_matrix[n][2],glow_reflected_matrix[n][3]);
            C3D_FVUnifSet(GPU_VERTEX_SHADER,31,1.f/320,1.f/240,0,0);
        }
        const Particle *p=glow_adaptive_particle(first,i);
        const FhGlowMeshView *mesh=fh_pica_hud_glow_mesh(page,p->uv);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,24,p->position[0],p->position[1],p->position[2],p->position[3]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,25,p->angles[0]/65536.f,p->angles[1]/65536.f,p->angles[2]/65536.f,p->angles[3]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,26,p->color[0],p->color[1],p->color[2],p->color[3]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,27,p->uv[0]/128.f,p->uv[1]/128.f,p->uv[2]/128.f,p->uv[3]/128.f);
        if(batch_dynamic){
            const int16_t *corners=&dynamic_instances[first+i].corners[0][0];
            for(unsigned n=0;n<3;n++)C3D_FVUnifSet(GPU_VERTEX_SHADER,28+n,corners[n*4],corners[n*4+1],corners[n*4+2],corners[n*4+3]);
        }
        C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
        if(BufInfo_Add(b,mesh->vertex,sizeof(FhGlowMeshVertex),1,0)<0)fh_platform_panic("Adaptive particle buffer");
        C3D_DrawElements(GPU_TRIANGLES,mesh->triangles*3,C3D_UNSIGNED_SHORT,mesh->index);
        fh_pica_hud_account(mesh->triangles*3);glow_adaptive_draws++;glow_adaptive_triangles+=mesh->triangles;
    }
    /* VSH uniform writes can mirror into GS hardware. Restore its complete
     * source parameters on the next ordinary particle submission. */
    bound=0;camera_dirty=1;template_publications=0;clip_dirty=1;glow_reflected_matrix_dirty=1;
    last_page=last_raster=~0u;last_fog_scale=-1;
    fh_pica_scene_invalidate();count=0;draws++;
    return 1;
}
static void glow_adaptive_shutdown(void)
{
    for(unsigned i=0;i<2;i++)shaderProgramFree(glow_adaptive_program+i);
    if(glow_adaptive_binary)DVLB_Free(glow_adaptive_binary);
    glow_adaptive_binary=NULL;
    char line[160];snprintf(line,sizeof line,"PICA adaptive particles: draws=%u triangles=%u declined_particles=%u\n",
        glow_adaptive_draws,glow_adaptive_triangles,glow_adaptive_declines);fh_platform_log(line);
}
