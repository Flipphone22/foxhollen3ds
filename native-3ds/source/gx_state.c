#include "gx_internal.h"
#include "fh_geometry_cache.h"
#include "fh_retained_geometry.h"
#include "fh_compiled_geometry.h"
#include "fh_native_fog.h"
#include <math.h>
#include <string.h>
FhGx fh_gx;
#ifndef FH_GX_PRODUCER_IMPL
void fh_pica_live_viewport(float out[6]){memcpy(out,fh_gx.draw.viewport,6*sizeof(float));}
const FhDrawState *fh_gx_native_fog_state(void){return &fh_gx.draw;}
#endif
void fh_gx_require(int condition,const char *message) { if(!condition) fh_platform_panic(message); }
static void identity(float matrix[3][4]) { memset(matrix,0,48);matrix[0][0]=matrix[1][1]=matrix[2][2]=1; }
static void color(float *out,GXColor value) { out[0]=value.r/255.f;out[1]=value.g/255.f;out[2]=value.b/255.f;out[3]=value.a/255.f; }
static uint32_t rgba(GXColor value) { return (u32)value.r<<24|(u32)value.g<<16|(u32)value.b<<8|value.a; }
void fh_gx_reset(void) {
    fh_gx_bounds_reset();
    fh_geometry_reset();
    fh_retained_reset();
    fh_compiled_reset();
    memset(&fh_gx,0,sizeof(fh_gx));
    for(unsigned i=0;i<64;i++) { identity(fh_gx.position[i]);identity(fh_gx.normal[i]);identity(fh_gx.texture[i]);identity(fh_gx.post[i]); }
    for(unsigned i=0;i<4;i++) { fh_gx.projection[i][i]=1;for(unsigned j=0;j<4;j++) fh_gx.draw.swaps[i][j]=j; }
    for(unsigned i=0;i<2;i++) for(unsigned j=0;j<4;j++) fh_gx.material[i][j]=1;
    fh_gx.draw.depth_test=fh_gx.draw.depth_write=fh_gx.draw.color_write=fh_gx.draw.alpha_write=1;
    fh_gx.draw.depth_compare=GX_LEQUAL;fh_gx.draw.blend_src=GX_BL_ONE;fh_gx.draw.blend_dst=GX_BL_ZERO;
    fh_gx.draw.alpha_compare[0]=fh_gx.draw.alpha_compare[3]=GX_ALWAYS;
    fh_gx.draw.scissor[2]=640;fh_gx.draw.scissor[3]=480;
    fh_gx.draw.viewport[2]=640;fh_gx.draw.viewport[3]=480;fh_gx.draw.viewport[5]=1;
    fh_gx.clear_depth=0xffffff;fh_gx.line_width=fh_gx.point_size=6;
    fh_gx.draw.stages=1;
    for(unsigned i=0;i<8;i++) { fh_gx.texgens[i].matrix=GX_IDENTITY;fh_gx.texgens[i].postmatrix=GX_PTIDENTITY; }
    for(unsigned i=0;i<16;i++) { GXSetTevOrder(i,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);GXSetTevOp(i,GX_PASSCLR); }
}
void GXSetVtxDesc(GXAttr attribute,GXAttrType type) {
    fh_gx_require((unsigned)attribute<26 && (unsigned)type<4,"Invalid vertex descriptor");
    if(attribute==GX_VA_NRM || attribute==GX_VA_NBT) {
        if(attribute==GX_VA_NRM) fh_gx.normal_enabled=type!=GX_NONE;else fh_gx.binormal_enabled=type!=GX_NONE;
        if(type!=GX_NONE) fh_gx.normal_type=type;
        fh_gx.descriptors[GX_VA_NRM]=(fh_gx.normal_enabled || fh_gx.binormal_enabled)?fh_gx.normal_type:GX_NONE;
    } else fh_gx.descriptors[attribute]=type;
}
void GXClearVtxDesc(void) { memset(fh_gx.descriptors,0,sizeof(fh_gx.descriptors));fh_gx.normal_enabled=fh_gx.binormal_enabled=fh_gx.normal_type=0; }
void GXSetVtxAttrFmt(GXVtxFmt format,GXAttr attribute,GXCompCnt count,GXCompType type,u8 fraction) {
    if(attribute==GX_VA_NBT) attribute=GX_VA_NRM;
    fh_gx_require((unsigned)format<8 && (unsigned)attribute<26 && fraction<=31,"Invalid vertex format");
    fh_gx.formats[format][attribute]=(FhVtxFormat){count,type,fraction};
}
void GXSetArray(GXAttr attribute,const void *data,u32 bytes,u8 stride,bool little_endian) {
    if(attribute==GX_VA_NBT) attribute=GX_VA_NRM;
    fh_gx_require((unsigned)attribute<26,"Invalid vertex array");
    fh_gx.arrays[attribute]=(FhVtxArray){data,bytes,stride,little_endian};
}
void GXInvalidateVtxCache(void) {}
void GXLoadPosMtxImm(const void *matrix,u32 id) { fh_gx_require(id%3==0 && id/3<64,"Invalid position matrix");memcpy(fh_gx.position[id/3],matrix,48); }
void GXLoadNrmMtxImm(const void *matrix,u32 id) { fh_gx_require(id%3==0 && id/3<64,"Invalid normal matrix");memcpy(fh_gx.normal[id/3],matrix,48); }
void GXLoadTexMtxImm(const void *matrix,u32 id,GXTexMtxType type) {
    float (*destination)[4];
    if(id>=GX_PTTEXMTX0) { id-=GX_PTTEXMTX0;fh_gx_require(id%3==0 && id/3<64,"Invalid post texture matrix");destination=fh_gx.post[id/3]; }
    else { fh_gx_require(id%3==0 && id/3<64,"Invalid texture matrix");destination=id<GX_TEXMTX0?fh_gx.position[id/3]:fh_gx.texture[id/3]; }
    /* A 2x4 XF load writes eight words and preserves the third row. */
    memcpy(destination,matrix,type==GX_MTX2x4?32:48);
}
void GXSetCurrentMtx(u32 id) { fh_gx_require(id%3==0 && id/3<64,"Invalid current matrix");fh_gx.current_matrix=id; }
void GXSetProjection(const void *matrix,GXProjectionType type) { memcpy(fh_gx.projection,matrix,64);fh_gx.projection_type=type; }
void GXSetViewport(f32 left,f32 top,f32 width,f32 height,f32 near,f32 far) { float values[]={left,top,width,height,near,far};memcpy(fh_gx.draw.viewport,values,sizeof(values)); }
void GXSetViewportJitter(f32 left,f32 top,f32 width,f32 height,f32 near,f32 far,u32 field) { GXSetViewport(left,top-(field?0:.5f),width,height,near,far); }
void GXSetScissor(u32 left,u32 top,u32 width,u32 height) { u32 values[]={left,top,width,height};memcpy(fh_gx.draw.scissor,values,sizeof(values)); }
void GXGetScissor(u32 *left,u32 *top,u32 *width,u32 *height) { *left=fh_gx.draw.scissor[0];*top=fh_gx.draw.scissor[1];*width=fh_gx.draw.scissor[2];*height=fh_gx.draw.scissor[3]; }
void GXSetNumTexGens(u8 count) { fh_gx_require(count<=8,"Too many texture generators");fh_gx.draw.texgens=count; }
void GXSetTexCoordGen2(GXTexCoordID id,GXTexGenType type,GXTexGenSrc source,u32 matrix,GXBool normalize,u32 post) {
    fh_gx_require((unsigned)id<8,"Invalid texture generator");fh_gx.texgens[id]=(FhTexGen){type,source,matrix,normalize,post};
}
void fhGXSetCullMode(GXCullMode mode) { fh_gx.draw.cull=mode; }
void fhLoadIdentityPosMtx(void) { float matrix[3][4];identity(matrix);GXLoadPosMtxImm(matrix,GX_PNMTX9); }
void GXSetZMode(GXBool test,GXCompare compare,GXBool write) { fh_gx.draw.depth_test=test;fh_gx.draw.depth_compare=compare;fh_gx.draw.depth_write=write; }
void GXSetBlendMode(GXBlendMode mode,GXBlendFactor source,GXBlendFactor destination,GXLogicOp op) { fh_gx.draw.blend_mode=mode;fh_gx.draw.blend_src=source;fh_gx.draw.blend_dst=destination;fh_gx.draw.logic_op=op; }
void GXSetColorUpdate(GXBool value) { fh_gx.draw.color_write=value; }
void GXSetAlphaUpdate(GXBool value) { fh_gx.draw.alpha_write=value; }
void GXSetAlphaCompare(GXCompare first,u8 first_ref,GXAlphaOp op,GXCompare second,u8 second_ref) { u32 values[]={first,first_ref,op,second,second_ref};memcpy(fh_gx.draw.alpha_compare,values,sizeof(values)); }
void GXSetPixelFmt(GXPixelFmt pixel,GXZFmt16 depth) { fh_gx.draw.pixel_format=fh_gx.pixel_format=pixel;fh_gx.z_format=depth; }
void GXSetZCompLoc(GXBool before) { fh_gx.z_before=before; }
void GXSetDither(GXBool enabled) { fh_gx.dither=enabled; }
void GXSetFieldMode(GXBool field,GXBool aspect) { (void)field;(void)aspect; }
void GXSetFog(GXFogType type,f32 start,f32 end,f32 near,f32 far,GXColor value) {
    fh_gx.draw.fog_type=type;fh_gx.draw.fog_start=start;fh_gx.draw.fog_end=end;fh_gx.draw.fog_near=near;fh_gx.draw.fog_far=far;fh_gx.draw.fog_color=rgba(value);
}
void GXSetLineWidth(u8 width,GXTexOffset offset) { fh_gx.line_width=width;fh_gx.line_offset=offset; }
void GXSetPointSize(u8 size,GXTexOffset offset) { fh_gx.point_size=size;fh_gx.point_offset=offset; }
void GXEnableTexOffsets(GXTexCoordID coord,GXBool line,GXBool point) {
    fh_gx_require((unsigned)coord<8,"Invalid texture offset coordinate");
    unsigned bit=1u<<coord;
    fh_gx.line_offsets=(fh_gx.line_offsets&~bit)|(line?bit:0);
    fh_gx.point_offsets=(fh_gx.point_offsets&~bit)|(point?bit:0);
}
static FhTevStage *stage(unsigned index) { fh_gx_require(index<16,"Invalid TEV stage");return &fh_gx.draw.tev[index]; }
void GXSetNumTevStages(u8 count) { fh_gx_require(count>0 && count<=16,"Invalid TEV stage count");fh_gx.draw.stages=count; }
void GXSetTevColorIn(GXTevStageID id,GXTevColorArg a,GXTevColorArg b,GXTevColorArg c,GXTevColorArg d) { u32 values[]={a,b,c,d};memcpy(stage(id)->color,values,sizeof(values)); }
void GXSetTevAlphaIn(GXTevStageID id,GXTevAlphaArg a,GXTevAlphaArg b,GXTevAlphaArg c,GXTevAlphaArg d) { u32 values[]={a,b,c,d};memcpy(stage(id)->alpha,values,sizeof(values)); }
void GXSetTevColorOp(GXTevStageID id,GXTevOp op,GXTevBias bias,GXTevScale scale,GXBool clamp,GXTevRegID destination) {
    FhTevStage *s=stage(id);s->color_op=op;s->color_bias=bias;s->color_scale=scale;s->color_clamp=clamp;s->color_dest=destination;
}
void GXSetTevAlphaOp(GXTevStageID id,GXTevOp op,GXTevBias bias,GXTevScale scale,GXBool clamp,GXTevRegID destination) {
    FhTevStage *s=stage(id);s->alpha_op=op;s->alpha_bias=bias;s->alpha_scale=scale;s->alpha_clamp=clamp;s->alpha_dest=destination;
}
void GXSetTevOrder(GXTevStageID id,GXTexCoordID coord,GXTexMapID map,GXChannelID channel) { FhTevStage *s=stage(id);s->coord=coord;s->map=map;s->channel=channel; }
void GXSetTevColor(GXTevRegID id,GXColor value) { fh_gx_require((unsigned)id<4,"Invalid TEV color");color(fh_gx.draw.tev_colors[id],value); }
void GXSetTevColorS10(GXTevRegID id,GXColorS10 value) { fh_gx_require((unsigned)id<4,"Invalid TEV color");float *c=fh_gx.draw.tev_colors[id];c[0]=value.r/255.f;c[1]=value.g/255.f;c[2]=value.b/255.f;c[3]=value.a/255.f; }
void GXSetTevKColor(GXTevKColorID id,GXColor value) { fh_gx_require((unsigned)id<4,"Invalid konst color");color(fh_gx.draw.konst_colors[id],value); }
void GXSetTevKColorSel(GXTevStageID id,GXTevKColorSel value) { stage(id)->kcolor=value; }
void GXSetTevKAlphaSel(GXTevStageID id,GXTevKAlphaSel value) { stage(id)->kalpha=value; }
void GXSetTevSwapMode(GXTevStageID id,GXTevSwapSel raster,GXTevSwapSel texture) { stage(id)->ras_swap=raster;stage(id)->tex_swap=texture; }
void GXSetTevSwapModeTable(GXTevSwapSel id,GXTevColorChan r,GXTevColorChan g,GXTevColorChan b,GXTevColorChan a) {
    fh_gx_require((unsigned)id<4,"Invalid TEV swap table");u32 values[]={r,g,b,a};memcpy(fh_gx.draw.swaps[id],values,sizeof(values));
}
void GXSetTevOp(GXTevStageID id,GXTevMode mode) {
    GXTevColorArg raster=id?GX_CC_CPREV:GX_CC_RASC;GXTevAlphaArg alpha=id?GX_CA_APREV:GX_CA_RASA;
    switch(mode) {
    case GX_MODULATE:GXSetTevColorIn(id,GX_CC_ZERO,GX_CC_TEXC,raster,GX_CC_ZERO);GXSetTevAlphaIn(id,GX_CA_ZERO,GX_CA_TEXA,alpha,GX_CA_ZERO);break;
    case GX_DECAL:GXSetTevColorIn(id,raster,GX_CC_TEXC,GX_CC_TEXA,GX_CC_ZERO);GXSetTevAlphaIn(id,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,alpha);break;
    case GX_BLEND:GXSetTevColorIn(id,raster,GX_CC_ONE,GX_CC_TEXC,GX_CC_ZERO);GXSetTevAlphaIn(id,GX_CA_ZERO,GX_CA_TEXA,alpha,GX_CA_ZERO);break;
    case GX_REPLACE:GXSetTevColorIn(id,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,GX_CC_TEXC);GXSetTevAlphaIn(id,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,GX_CA_TEXA);break;
    case GX_PASSCLR:GXSetTevColorIn(id,GX_CC_ZERO,GX_CC_ZERO,GX_CC_ZERO,raster);GXSetTevAlphaIn(id,GX_CA_ZERO,GX_CA_ZERO,GX_CA_ZERO,alpha);break;
    default:fh_platform_panic("Invalid TEV operation");
    }
    GXSetTevColorOp(id,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
    GXSetTevAlphaOp(id,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
}
void GXSetTevDirect(GXTevStageID id) { memset(stage(id)->indirect,0,sizeof(stage(id)->indirect)); }
void GXSetNumIndStages(u8 count) { fh_gx_require(count<=4,"Invalid indirect stage count");fh_gx.draw.indirect_count=count; }
void GXSetIndTexOrder(GXIndTexStageID id,GXTexCoordID coord,GXTexMapID map) { fh_gx_require((unsigned)id<4,"Invalid indirect stage");fh_gx.draw.indirect_order[id][0]=coord;fh_gx.draw.indirect_order[id][1]=map; }
void GXSetIndTexCoordScale(GXIndTexStageID id,GXIndTexScale s,GXIndTexScale t) { fh_gx_require((unsigned)id<4,"Invalid indirect scale");fh_gx.draw.indirect_scale[id][0]=s;fh_gx.draw.indirect_scale[id][1]=t; }
void GXSetIndTexMtx(GXIndTexMtxID id,const void *matrix,s8 exponent) {
    unsigned index=((unsigned)id-1)&3;fh_gx_require(index<3,"Invalid indirect matrix");
    memcpy(fh_gx.draw.indirect_matrix[index],matrix,24);fh_gx.draw.indirect_exponent[index]=exponent;
}
void GXSetTevIndirect(GXTevStageID id,GXIndTexStageID indirect,GXIndTexFormat format,GXIndTexBiasSel bias,GXIndTexMtxID matrix,GXIndTexWrap s,GXIndTexWrap t,GXBool add,GXBool lod,GXIndTexAlphaSel alpha) {
    u32 values[]={indirect,format,bias,matrix,s,t,add,lod,alpha};memcpy(stage(id)->indirect,values,sizeof(values));
}
void GXSetTevIndRepeat(GXTevStageID id) { GXSetTevIndirect(id,GX_INDTEXSTAGE0,GX_ITF_8,GX_ITB_NONE,GX_ITM_OFF,GX_ITW_0,GX_ITW_0,GX_TRUE,GX_FALSE,GX_ITBA_OFF); }
