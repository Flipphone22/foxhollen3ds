#include "fh_pica_distortion_draw.h"
#include "fh_pica_distortion_composite.h"
#include "fh_pica_distortion_submit.h"
#include "fh_pica_texture_coordinates.h"
unsigned fh_pica_distortion_draw(FhPicaDistortionGeometry *geometry,
    shaderProgram_s *program,C3D_TexEnv material[6],
    const FhPicaDistortionPlan *plan,const float viewport[4],
    C3D_Tex *color,C3D_Tex *depth,C3D_Tex *radial)
{
    FhPicaDistortionUniforms uniforms;
    if(!geometry||!geometry->allocation||!program||!material||
       !color||!depth||!radial||!color->data||!depth->data||!radial->data||
       !fh_pica_distortion_uniforms(&uniforms,plan,FH_DISTORTION_SOURCE_WIDTH,
                                  FH_DISTORTION_SOURCE_HEIGHT,viewport))return 0;
    C3D_BindProgram(program);C3D_LightEnvBind(NULL);
    C3D_SetAttrInfo(&geometry->attributes);C3D_SetBufInfo(&geometry->buffers);
    for(unsigned i=0;i<7;i++){
        const float *row=uniforms.row[i];
        C3D_FVUnifSet(GPU_VERTEX_SHADER,i,row[0],row[1],row[2],row[3]);
    }
    C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE_MINUS_SRC_ALPHA,GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA,GPU_SRC_ALPHA);
    fh_pica_distortion_composite_parameters(material,plan->color);
    for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,material+i);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    C3D_TexBind(0,color);C3D_TexBind(1,depth);C3D_TexBind(2,radial);
    fh_pica_texture2_coord1(0);
    unsigned batches=fh_pica_distortion_submit(fh_pica_distortion_geometry_indices(geometry));
    C3D_TexEnvBufUpdate(C3D_Both,0);
    return batches;
}
