#include "fh_material.h"
#include "fh_fog.h"
#include <string.h>
#include <math.h>

/* Factor the SFA lighting/base/detail/movie expression without dropping a
 * texture term. Intermediate products are halved to avoid premature clamps. */
static FhCombinerChannel channel(unsigned op,unsigned a,unsigned ao,unsigned b,unsigned bo,unsigned c,unsigned co,unsigned scale) {
    return (FhCombinerChannel){.source={a,b,c},.operand={ao,bo,co},.operation=op,.scale=scale};
}
static int layered_opacity(const FhDrawState *s,float *opacity){
    unsigned k=s->tev[3].kalpha;
    if(k<8)*opacity=(8-k)/8.f;
    else if(k>=16 && k<32)*opacity=s->konst_colors[k&3][(k-16)/4];
    else return 0;
    return *opacity>=0 && *opacity<=1;
}
/* The scalar oracle remains available to captured-workload comparisons. */
unsigned fh_reflective_disabled;
unsigned fh_reflective_triangles,fh_reflective_fallbacks;
unsigned fh_reflective_lod_enabled,fh_reflective_lod_plans;
int fh_reflective_material(const FhDrawState *s){
    /* Opaque base plus two positive reflection products. Ordered draws
     * per triangle preserve intersections and equal-depth overlap. Pinned
     * Melee renderer.c is the reference for depth/write-mask restoration. */
    if(fh_reflective_disabled || s->stages!=5 || s->indirect_count || s->blend_mode ||
       !s->depth_test || !s->depth_write || s->depth_compare!=3 ||
       s->alpha_compare[0]!=7 || s->alpha_compare[2] || s->alpha_compare[3]!=7)return 0;
    static const unsigned color[5][4]={{15,8,10,15},{15,14,8,15},{15,2,8,0},{15,14,8,15},{15,2,8,0}};
    static const unsigned alpha[5][4]={{7,4,5,7},{7,7,7,0},{7,7,7,0},{7,7,7,0},{7,7,7,0}};
    for(unsigned i=0;i<5;i++){
        const FhTevStage *t=s->tev+i;
        if(memcmp(t->color,color[i],16) || memcmp(t->alpha,alpha[i],16) ||
           t->color_op || t->alpha_op || t->color_bias || t->alpha_bias || t->color_scale || t->alpha_scale ||
           !t->color_clamp || !t->alpha_clamp || t->color_dest!=((i==1 || i==3)?1u:0u) || t->alpha_dest ||
           t->map!=(i+1)/2 || t->coord!=i || (i==0 && t->channel!=4) ||
           (i==1 && t->kcolor!=12) || (i==3 && t->kcolor!=13))return 0;
        for(unsigned j=0;j<9;j++)if(t->indirect[j])return 0;
        for(unsigned c=0;c<4;c++)if(s->swaps[t->tex_swap&3][c]!=c || (!i && s->swaps[t->ras_swap&3][c]!=c))return 0;
    }
    for(unsigned i=0;i<2;i++)for(unsigned c=0;c<3;c++)
        if(!(s->konst_colors[i][c]>=0 && s->konst_colors[i][c]<=1))return 0;
    return 1;
}
int fh_reflective_triangle(const FhVertex *v,const FhDrawState *s){
    if(s->raw_position || !(s->viewport[4]>=0 && s->viewport[4]<s->viewport[5] && s->viewport[5]<=1))return 0;
    for(unsigned i=0;i<3;i++){
        if(!(v[i].position[3]>0))return 0;
        for(unsigned c=0;c<4;c++)if(!(v[i].color[0][c]>=0 && v[i].color[0][c]<=1))return 0;
        for(unsigned t=0;t<5;t++)if(!isfinite(v[i].uv[t][2]) || fabsf(v[i].uv[t][2])<1e-10f ||
           (t && v[i].uv[t][2]!=v[0].uv[t][2]))return 0;
        float depth=1-s->viewport[5]-(s->viewport[5]-s->viewport[4])*v[i].position[2]/v[i].position[3];
        if(!isfinite(depth) || depth<0 || depth>1)return 0;
    }
    if(s->fog_type&7){
        unsigned type=s->fog_type&7;
        if((type!=2 && type<4) || !(s->fog_far>s->fog_near && s->fog_near>0 && s->fog_end>s->fog_start) ||
           !isfinite(s->fog_far) || !isfinite(s->fog_end) || !isfinite(s->fog_start))return 0;
    }
    return 1;
}
void fh_reflective_pipeline(FhMaterial *plan,const FhDrawState *s,unsigned pass){
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;FhPipeline *p=&plan->pipeline;
    p->count=pass>=3?1:pass?2:1;p->textures=pass>=3?0:pass?2:1;
    for(unsigned c=0;c<4;c++)p->primary[c]=c;
    if(pass>=3){
        /* White with black fog supplies F; black with the original fog color
         * supplies (1-F)*fog. Apply both after clamping the full lighting sum.
         * The same PICA LUT/depth interpolation as an ordinary draw is used. */
        for(unsigned c=0;c<4;c++)p->stages[0].constant[c]=pass==3?1:0;
        p->stages[0].channel[0]=channel(FH_REPLACE,FH_CONSTANT,0,0,0,0,0,1);
        p->stages[0].channel[1]=channel(FH_REPLACE,FH_CONSTANT,1,0,0,0,0,1);
    }else if(!pass){
        p->map[0]=p->coord[0]=0;
        p->stages[0].channel[0]=channel(FH_MODULATE,FH_TEXTURE0,0,FH_PRIMARY,0,0,0,1);
        p->stages[0].channel[1]=channel(FH_MODULATE,FH_TEXTURE0,1,FH_PRIMARY,1,0,0,1);
    }else{
        p->map[0]=p->map[1]=pass;p->coord[0]=pass*2-1;p->coord[1]=pass*2;
        for(unsigned c=0;c<3;c++)p->stages[0].constant[c]=s->konst_colors[pass-1][c];
        p->stages[0].channel[0]=channel(FH_MODULATE,FH_CONSTANT,0,FH_TEXTURE0,0,0,0,1);
        p->stages[1].channel[0]=channel(FH_MODULATE,FH_PREVIOUS,0,FH_TEXTURE1,0,0,0,1);
        for(unsigned i=0;i<2;i++)p->stages[i].channel[1]=channel(FH_REPLACE,FH_PRIMARY,1,0,0,0,0,1);
    }
}
/* Reduced-detail reflection: sample each reflection layer once, using its
 * first original coordinate, and square that sample instead of multiplying
 * two differently positioned samples. Base color/alpha and both live tint
 * colors survive. Five combiners fit one PICA draw; ordinary fog follows the
 * final saturated sum. Only the audited opaque reflection family qualifies.
 * Melee's three-sampler/six-combiner limit and ordered depth semantics apply;
 * this approximation is specific to SFA and must be explicitly enabled. */
int fh_reflective_lod_pipeline(FhMaterial *plan,const FhDrawState *s){
    if(!fh_reflective_material(s))return 0;
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;
    FhPipeline *p=&plan->pipeline;p->count=5;p->textures=3;
    p->buffer_updates[0]=(1u<<0)|(1u<<2);
    for(unsigned c=0;c<4;c++)p->primary[c]=c;
    for(unsigned i=0;i<3;i++){p->map[i]=i;p->coord[i]=i?2*i-1:0;}
    p->stages[0].channel[0]=channel(FH_MODULATE,FH_TEXTURE0,0,FH_PRIMARY,0,0,0,1);
    p->stages[0].channel[1]=channel(FH_MODULATE,FH_TEXTURE0,1,FH_PRIMARY,1,0,0,1);
    for(unsigned layer=0;layer<2;layer++){
        unsigned a=1+2*layer,b=a+1,tex=FH_TEXTURE1+layer;
        p->stages[a].channel[0]=channel(FH_MODULATE,tex,0,tex,0,0,0,1);
        p->stages[b].channel[0]=channel(FH_MULTIPLY_ADD,FH_PREVIOUS,0,FH_CONSTANT,0,FH_BUFFER,0,1);
        memcpy(p->stages[b].constant,s->konst_colors[layer],sizeof(p->stages[b].constant));
        p->stages[a].channel[1]=p->stages[b].channel[1]=channel(FH_REPLACE,FH_PREVIOUS,1,0,0,0,0,1);
    }
    return 1;
}
/* A second interpolated RGBA color travels in two 16x16 identity lookups.
 * Texel steps are exactly 17/255, so bilinear sampling reconstructs each
 * component up to PICA's sampling precision. This keeps the two positive
 * lighting products in the fragment combiners, without subdividing geometry.
 * Sources are ordinary immutable CPU bytes; the normal texture cache owns
 * conversion, linear storage, flushes and GPU lifetime. */
int fh_sphere_prepare(FhMaterial *plan,FhDrawState *draw,FhVertex *out,const FhDrawState *s,const FhVertex *v,unsigned count) {
    static unsigned char lookup[2][16*16*4];static unsigned initialized;
    if(!count || count%3 || s->tev[0].map>=6 || s->tev[0].coord>=6)return 0;
    for(unsigned i=0;i<count;i++)for(unsigned c=0;c<8;c++)
        if(!(((const float*)v[i].color)[c]>=0 && ((const float*)v[i].color)[c]<=1))return 0;
    if(!initialized){
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<16;x++){
            unsigned k=(y*16+x)*4;lookup[0][k]=x*17;lookup[0][k+1]=y*17;
            lookup[1][k+2]=x*17;lookup[1][k+3]=y*17;
        }
        initialized=1;
    }
    *draw=*s;memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;
    FhPipeline *p=&plan->pipeline;p->count=3;p->textures=3;p->buffer_updates[0]=1;
    p->map[0]=s->tev[0].map;p->coord[0]=s->tev[0].coord;
    for(unsigned i=0;i<2;i++){
        p->map[i+1]=p->coord[i+1]=6+i;
        draw->textures[6+i]=(FhTexture){.pixels=lookup[i],.width=16,.height=16,.format=0x46,.min_filter=1,.mag_filter=1};
    }
    for(unsigned c=0;c<4;c++)p->primary[c]=c;
    p->stages[0].channel[0]=channel(FH_MULTIPLY_ADD,FH_PRIMARY,0,FH_PRIMARY,1,FH_TEXTURE0,0,1);
    p->stages[1].channel[0]=channel(FH_ADD,FH_TEXTURE1,0,FH_TEXTURE2,0,0,0,1);
    p->stages[2].channel[0]=channel(FH_MULTIPLY_ADD,FH_PREVIOUS,0,FH_TEXTURE2,1,FH_BUFFER,0,1);
    for(unsigned i=0;i<3;i++)p->stages[i].channel[1]=channel(FH_REPLACE,FH_CONSTANT,1,0,0,0,0,1);
    for(unsigned i=0;i<count;i++){
        out[i]=v[i];
        if((s->raw_position&64) && v[i].matrix_index>=0)continue;
        for(unsigned j=0;j<2;j++){
            out[i].uv[6+j][0]=(.5f+15*v[i].color[1][j*2])/16;
            out[i].uv[6+j][1]=(.5f+15*v[i].color[1][j*2+1])/16;
            out[i].uv[6+j][2]=1;
        }
    }
    return 1;
}
/* Five-tap masked EFB blur. Fold the alpha arithmetic into x4 stages:
 * clamp(64*(mask-threshold)), or absolute difference scaled by 64/256.
 * PICA's 8-bit intermediates cannot retain the
 * original half of a one-byte difference. Five weighted additive draws keep
 * all taps and need no extra render target. Restrict weights so that the final
 * RGB sum cannot overflow before blending. */
int fh_blur_material(const FhDrawState *s) {
    if((s->stages!=6 && s->stages!=7) || s->indirect_count || s->depth_test || s->depth_write || s->cull ||
       s->blend_mode!=1 || s->blend_src!=4 || s->blend_dst!=5 ||
       s->alpha_compare[0]!=7 || s->alpha_compare[2]!=0 || s->alpha_compare[3]!=7)return 0;
    /* Orthographic screen effects retain the world's fog registers. Validate
     * the parameters here, then prove the actual quad's depth interval clear
     * below. Requiring every possible depth to be clear rejects NPC dialogue. */
    if((s->fog_type&7) && !(s->fog_far>s->fog_near && s->fog_near>0 && s->fog_end>s->fog_start &&
       isfinite(s->fog_far) && isfinite(s->fog_end) && isfinite(s->fog_start)))return 0;
    for(unsigned c=0;c<3;c++)if(!(s->konst_colors[1][c]>=0 && s->konst_colors[1][c]<=.4f))return 0;
    if(!(s->konst_colors[0][3]>=0 && s->konst_colors[0][3]<=1))return 0;
    const unsigned first_color[]={15,15,15,15},first_alpha[]={6,7,7,4},alpha[]={7,7,7,0};
    unsigned absolute=s->stages==7,bigger=absolute && s->tev[0].alpha_scale==0;
    if(absolute){
        const FhTevStage *t=&s->tev[0];const unsigned a[]={4,7,7,6};
        if(memcmp(t->color,first_color,16) || memcmp(t->alpha,a,16) || t->color_op || t->alpha_op!=1 ||
           t->color_bias || t->alpha_bias || t->color_scale || t->alpha_scale!=(bigger?0u:3u) ||
           !t->color_clamp || !t->alpha_clamp || t->color_dest!=3 || t->alpha_dest!=3 ||
           t->map!=1 || t->coord!=1 || t->kalpha!=28)return 0;
        for(unsigned c=0;c<4;c++)if(s->swaps[t->tex_swap&3][c]!=c)return 0;
        for(unsigned j=0;j<9;j++)if(t->indirect[j])return 0;
    }
    for(unsigned i=0;i<6;i++){
        const FhTevStage *t=&s->tev[i+absolute];unsigned color[]={15,8,14,i==1?15:0},sum_alpha[]={0,7,7,3};
        const unsigned *expected_alpha=absolute && i==1?sum_alpha:i?alpha:first_alpha;
        if(memcmp(t->color,i?color:first_color,16) || memcmp(t->alpha,expected_alpha,16) ||
           t->color_op || t->alpha_op!=(i==0) || t->color_bias || t->alpha_bias ||
             t->color_scale!=(i==5?3u:0u) || t->alpha_scale!=(bigger?(i==0 || i==5?0u:2u):(i<2?3u:2u)) ||
           t->color_clamp!=(i==0 || i==5) || !t->alpha_clamp || t->color_dest || t->alpha_dest ||
           t->map!=(i==0?1u:0u) || t->coord!=(i==0?1u:i==1?0u:i) ||
           (i && t->kcolor!=13) || (!i && t->kalpha!=28))return 0;
        for(unsigned c=0;c<4;c++)if(s->swaps[t->tex_swap&3][c]!=c)return 0;
        for(unsigned j=0;j<9;j++)if(t->indirect[j])return 0;
    }
    return 1;
}
int fh_blur_quad(const FhVertex *v,unsigned count,const FhDrawState *s) {
    if(count!=6 || memcmp(v,v+3,sizeof(*v)) || memcmp(v+2,v+4,sizeof(*v)))return 0;
    if(s->fog_type&7){
        if(s->raw_position || !(s->viewport[4]>=0 && s->viewport[4]<s->viewport[5] && s->viewport[5]<=1))return 0;
        float low=1,high=0;
        for(unsigned i=0;i<6;i++){
            if(!(v[i].position[3]>0))return 0;
            float depth=1-s->viewport[5]-(s->viewport[5]-s->viewport[4])*v[i].position[2]/v[i].position[3];
            if(!isfinite(depth) || depth<0 || depth>1)return 0;
            if(depth<low)low=depth;if(depth>high)high=depth;
        }
        /* PICA fog interpolates a 128-entry LUT. Include two extra intervals
         * on either side for clip/depth quantization, and require a completely
         * flat clear plateau. Fogged/threshold-crossing geometry stays scalar,
         * avoiding repeated fog color contributions in the additive taps. */
        low=fmaxf(0,(floorf(low*128)-2)/128);high=fminf(1,(ceilf(high*128)+2)/128);
        if(fh_fog_transmission(s,low)!=1 || fh_fog_transmission(s,high)!=1)return 0;
    }
    const unsigned corner[]={0,1,2,5};float p[4][2],sign=0;
    for(unsigned i=0;i<6;i++){
        if(!(v[i].position[3]>0) || !(v[i].uv[1][2]>0) || v[i].uv[1][2]!=v[0].uv[1][2])return 0;
        /* A filtered mask is safe at native pixel centers when its coordinate
         * maps one texel to one destination pixel. Otherwise 8-bit sampling
         * differences would be amplified 64-fold near the threshold. */
        if(s->textures[1].min_filter || s->textures[1].mag_filter)for(unsigned c=0;c<2;c++){
            if(v[i].position[3]!=v[0].position[3])return 0;
            float screen=(s->viewport[c]+(1+(c?-1:1)*v[i].position[c]/v[i].position[3])*s->viewport[c+2]*.5f)*.5f;
            float texel=v[i].uv[1][c]/v[i].uv[1][2]*(c?s->textures[1].height:s->textures[1].width);
            if(fabsf(screen-texel)>1e-4f || !isfinite(texel))return 0;
        }
    }
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<2;c++){
        p[i][c]=v[corner[i]].position[c]/v[corner[i]].position[3];
        if(!isfinite(p[i][c]))return 0;
    }
    for(unsigned i=0;i<4;i++){
        unsigned j=(i+1)%4,k=(i+2)%4;
        float cross=(p[j][0]-p[i][0])*(p[k][1]-p[j][1])-(p[j][1]-p[i][1])*(p[k][0]-p[j][0]);
        if(!isfinite(cross) || cross==0 || (i && (cross>0)!=(sign>0)))return 0;
        sign=cross;
    }
    return 1;
}
void fh_blur_pipeline(FhMaterial *plan,const FhDrawState *state,unsigned tap) {
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;
    FhPipeline *p=&plan->pipeline;p->count=4;p->textures=2;
    p->map[0]=0;p->map[1]=1;p->coord[0]=tap?tap+1:0;p->coord[1]=1;
    for(unsigned c=0;c<4;c++)p->primary[c]=c;
    p->stages[0].channel[0]=channel(FH_MODULATE,FH_TEXTURE0,0,FH_CONSTANT,0,0,0,1);
    for(unsigned c=0;c<3;c++)p->stages[0].constant[c]=state->konst_colors[1][c]*.5f;
    p->stages[0].constant[3]=state->konst_colors[0][3];
    p->stages[0].channel[1]=channel(FH_SUBTRACT,FH_TEXTURE1,1,FH_CONSTANT,1,0,0,1);
    for(unsigned i=1;i<4;i++){
        p->stages[i].channel[0]=channel(FH_REPLACE,FH_PREVIOUS,0,0,0,0,0,1);
        p->stages[i].channel[1]=channel(FH_REPLACE,FH_PREVIOUS,1,0,0,0,0,4);
    }
    if(state->stages==7){
        p->count=5;p->buffer_updates[1]=1;
        p->stages[1].constant[3]=state->konst_colors[0][3];
        p->stages[1].channel[1]=channel(FH_SUBTRACT,FH_CONSTANT,1,FH_TEXTURE1,1,0,0,1);
        p->stages[2].channel[1]=channel(FH_ADD,FH_PREVIOUS,1,FH_BUFFER,1,0,0,4);
        p->stages[4]=p->stages[3];
        if(state->tev[0].alpha_scale==0){p->count=6;p->stages[5]=p->stages[4];}
    }
}
unsigned fh_unmasked_fur_disabled;
int fh_fur_material(const FhDrawState *s) {
    const unsigned colors[4][4]={{15,15,15,8},{15,8,6,10},{15,0,4,15},{8,14,0,0}};
    const unsigned alpha[4][4]={{7,7,7,7},{7,7,7,3},{7,7,7,0},{7,4,5,0}};
    const unsigned maps[]={0,1,255,3},coords[]={2,0,255,4};
    const unsigned indirect[4][9]={{0},{0},{0,0,7,1,6,6,0,0,0},{1,0,7,2,0,0,1,0,1}};
    if(s->stages!=4 || s->indirect_count!=2 || s->textures[3].format!=3 || s->tev[3].kcolor!=4 || s->tev[1].channel!=4)return 0;
    /* The unmasked family adds noise alpha directly. It uses the same two
     * indirect coordinate samples, but never consumes the bump alpha. */
    unsigned unmasked=s->tev[3].channel==255;
    if(unmasked && fh_unmasked_fur_disabled)return 0;
    if(!unmasked && s->tev[3].channel!=8)return 0;
    const unsigned order[4]={1,4,3,2};if(memcmp(s->indirect_order,order,sizeof(order)))return 0;
    for(unsigned i=0;i<4;i++) {
        const FhTevStage *t=&s->tev[i];
        const unsigned unmasked_alpha[4]={4,7,7,0},unmasked_indirect[9]={1,0,7,2,0,0,1,0,0};
        if(memcmp(t->color,colors[i],16)||memcmp(t->alpha,i==3 && unmasked?unmasked_alpha:alpha[i],16)||memcmp(t->indirect,i==3 && unmasked?unmasked_indirect:indirect[i],36)||t->map!=maps[i]||t->coord!=coords[i]||t->color_dest!=(i==1?2u:0u)||t->alpha_dest||t->color_op!=(i==3)||t->alpha_op||t->color_bias!=(i==3)||t->alpha_bias||t->color_scale||t->alpha_scale)return 0;
        if(i && !t->color_clamp)return 0;
        if(i==3 && !t->alpha_clamp)return 0;
        if((i==1 || i==2) && t->alpha_clamp && (s->tev_colors[3][3]<0 || s->tev_colors[3][3]>1))return 0;
        for(unsigned c=0;c<4;c++)if(s->swaps[t->tex_swap&3][c]!=c || s->swaps[t->ras_swap&3][c]!=c)return 0;
    }
    for(unsigned c=0;c<3;c++)if(s->tev_colors[3][c]<0 || s->tev_colors[3][c]>1)return 0;
    return 1;
}
void fh_fur_pipeline(FhMaterial *plan,const FhDrawState *state) {
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;
    FhPipeline *p=&plan->pipeline;p->count=5;p->textures=3;p->buffer_updates[0]=2;
    p->map[0]=0;p->map[1]=1;p->map[2]=2;p->coord[0]=7;p->coord[1]=2;p->coord[2]=0;
    for(unsigned c=0;c<4;c++)p->primary[c]=c;
    p->stages[0].channel[0]=channel(FH_MULTIPLY_ADD,FH_TEXTURE2,0,FH_CONSTANT,0,FH_PRIMARY,0,1);
    memcpy(p->stages[0].constant,state->tev_colors[3],12);
    p->stages[0].channel[1]=channel(FH_REPLACE,FH_TEXTURE0,1,0,0,0,0,1);
    p->stages[1].channel[0]=channel(FH_MODULATE,FH_PREVIOUS,0,FH_TEXTURE1,0,0,0,1);
    p->stages[2].channel[0]=channel(FH_ADD,FH_TEXTURE0,2,FH_CONSTANT,0,0,0,1);
    p->stages[3].channel[0]=channel(FH_MULTIPLY_ADD,FH_PREVIOUS,0,FH_BUFFER,0,FH_CONSTANT,0,1);
    p->stages[4].channel[0]=channel(FH_SUBTRACT,FH_PREVIOUS,0,FH_TEXTURE0,2,0,0,2);
    for(unsigned c=0;c<3;c++)p->stages[2].constant[c]=p->stages[3].constant[c]=.25f;
    for(unsigned i=1;i<5;i++)p->stages[i].channel[1]=channel(FH_REPLACE,FH_PREVIOUS,1,0,0,0,0,1);
}
void fh_fur_mesh_pipeline(FhMaterial *plan,const FhDrawState *state) {
    fh_fur_pipeline(plan,state);FhPipeline *p=&plan->pipeline;p->count=4;
    unsigned unmasked=state->tev[3].channel==255;
    if(!unmasked){p->texture_mode[1]=3;p->secondary_map[1]=3;}
    p->stages[0].channel[1]=channel(unmasked?FH_REPLACE:FH_MODULATE,FH_TEXTURE0,1,FH_TEXTURE1,1,0,0,1);
    p->stages[1].channel[1]=channel(state->tev_colors[3][3]<0?FH_SUBTRACT:FH_ADD,FH_PREVIOUS,1,FH_CONSTANT,1,0,0,1);
    p->stages[1].constant[3]=fabsf(state->tev_colors[3][3]);
    p->stages[2].channel[0]=channel(FH_INTERPOLATE,FH_TEXTURE0,0,FH_CONSTANT,0,FH_PREVIOUS,8,1);
    for(unsigned c=0;c<3;c++)p->stages[2].constant[c]=.5f;
    p->stages[3].channel[0]=channel(FH_ADD_SIGNED,FH_BUFFER,0,FH_PREVIOUS,8,0,0,1);
}
int fh_fur_surface_pipeline(FhMaterial *plan,const FhDrawState *state) {
    /* Explicit 3DS visual reduction, restricted to the recognized fur family.
     * Retain its color/lighting/opacity combiner and original noise texture,
     * but use the live ordinary noise coordinate without indirect distortion.
     * No CPU texture samples or generated per-vertex coordinates are needed.
     * Shell animation, silhouettes, raster/depth state and gameplay are owned
     * by the unchanged caller. This is not an exact GX rendering replacement. */
    if(!fh_fur_material(state))return 0;
    fh_fur_mesh_pipeline(plan,state);
    FhPipeline *p=&plan->pipeline;
    p->map[0]=3;p->map[1]=0;p->map[2]=1;
    p->coord[0]=4;
    if(p->texture_mode[1]==3)p->secondary_map[1]=2;
    return 1;
}
static int layered_five(FhMaterial *plan,const FhDrawState *s) {
    /* L=clamp(T0+R), B=T1.blue. GX produces clamp(T2*L+T2*(B*L)),
     * then blends that with T3 using T2.alpha. Pack T0.rgb/T1.blue into
     * one sampler. All product factors are bounded, so their intermediate
     * clamps are identities; the lighting sum and final sum still clamp.
     * This is the ordinary five-stage layered family, not an asset match. */
    static const unsigned colors[5][4]={{15,8,14,10},{15,8,4,15},{15,6,8,15},{15,8,4,6},{8,0,1,15}};
    static const unsigned alphas[5][4]={{7,7,7,7},{7,4,3,7},{7,7,7,7},{7,4,6,7},{7,4,0,7}};
    static const unsigned maps[]={0,1,2,2,3},coords[]={0,0,1,1,2},dest[]={2,3,3,0,0};
    float opacity;
    if(s->stages!=5 || s->tev[0].channel!=4 || s->tev[0].kcolor || !layered_opacity(s,&opacity))return 0;
    for(unsigned i=0;i<5;i++) {
        const FhTevStage *t=&s->tev[i];
        if(memcmp(t->color,colors[i],16) || memcmp(t->alpha,alphas[i],16) || t->color_op || t->alpha_op ||
           t->color_bias || t->alpha_bias || t->color_scale || t->alpha_scale || !t->color_clamp || !t->alpha_clamp ||
           t->color_dest!=dest[i] || t->alpha_dest || t->map!=maps[i] || t->coord!=coords[i])return 0;
        for(unsigned j=0;j<9;j++)if(t->indirect[j]!=(i==1?(j==4 || j==5?6u:j==6?1u:0u):0u))return 0;
        for(unsigned c=0;c<4;c++) {
            if(s->swaps[t->tex_swap&3][c]!=(i==1?(c==3?1u:2u):c))return 0;
            if(i==0 && s->swaps[t->ras_swap&3][c]!=c)return 0;
        }
    }
    const FhTexture *a=&s->textures[0],*b=&s->textures[1];
    if(a->format!=6 || b->format!=6 || a->width!=b->width || a->height!=b->height ||
       a->wrap_s!=b->wrap_s || a->wrap_t!=b->wrap_t || a->min_filter!=b->min_filter || a->mag_filter!=b->mag_filter)return 0;
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;
    FhPipeline *p=&plan->pipeline;p->count=4;p->textures=3;
    p->map[0]=0;p->map[1]=2;p->map[2]=3;p->coord[0]=0;p->coord[1]=1;p->coord[2]=2;
    p->texture_mode[0]=1;p->secondary_map[0]=1;
    for(unsigned c=0;c<4;c++)p->primary[c]=c;
    p->stages[0].channel[0]=channel(FH_ADD,FH_TEXTURE0,0,FH_PRIMARY,0,0,0,1);
    p->stages[1].channel[0]=channel(FH_MODULATE,FH_PREVIOUS,0,FH_TEXTURE1,0,0,0,1);
    p->stages[2].channel[0]=channel(FH_MULTIPLY_ADD,FH_PREVIOUS,0,FH_TEXTURE0,1,FH_PREVIOUS,0,1);
    p->stages[3].channel[0]=channel(FH_INTERPOLATE,FH_PREVIOUS,0,FH_TEXTURE2,0,FH_PREVIOUS,1,1);
    for(unsigned i=0;i<3;i++){
        p->stages[i].constant[3]=opacity;
        p->stages[i].channel[1]=channel(FH_MODULATE,FH_TEXTURE1,1,FH_CONSTANT,1,0,0,1);
    }
    p->stages[3].channel[1]=channel(FH_MODULATE,FH_PREVIOUS,1,FH_TEXTURE2,1,0,0,1);
    return 1;
}
/* SFA depth/opacity mask: T2.rgb*K0, T2.a*clamp(1-T0.a*T1.a).
 * PICA cannot subtract a product in one stage. Materialize the product,
 * subtract it, then apply the last texture alpha, keeping all three samples.
 * Pinned Melee's layered combiner is the state/operand reference. */
unsigned fh_product_mask_disabled;
static int product_mask(FhMaterial *plan,const FhDrawState *s){
    if(fh_product_mask_disabled || s->stages!=3 || s->indirect_count)return 0;
    static const unsigned colors[3][4]={{15,15,15,15},{15,15,15,0},{15,8,14,15}};
    static const unsigned alphas[3][4]={{7,7,7,4},{7,2,4,6},{7,4,0,7}};
    for(unsigned i=0;i<3;i++){
        const FhTevStage *t=s->tev+i;
        if(memcmp(t->color,colors[i],16) || memcmp(t->alpha,alphas[i],16) ||
           t->color_op || t->alpha_op!=(i==1) || t->color_bias || t->alpha_bias || t->color_scale || t->alpha_scale ||
           !t->color_clamp || !t->alpha_clamp || t->color_dest || t->alpha_dest!=(i?0u:2u) ||
           t->coord!=i || t->map!=i || t->channel!=255)return 0;
        for(unsigned c=0;c<4;c++)if(s->swaps[t->tex_swap&3][c]!=c)return 0;
        for(unsigned j=0;j<9;j++)if(t->indirect[j])return 0;
    }
    if(s->tev[1].kalpha || s->tev[2].kcolor!=12)return 0;
    for(unsigned c=0;c<3;c++)if(!(s->konst_colors[0][c]>=0 && s->konst_colors[0][c]<=1))return 0;
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;FhPipeline *p=&plan->pipeline;p->count=p->textures=3;
    for(unsigned i=0;i<3;i++){p->map[i]=p->coord[i]=i;p->stages[0].constant[i]=s->konst_colors[0][i];}
    for(unsigned c=0;c<4;c++)p->primary[c]=c;
    p->stages[0].channel[0]=channel(FH_MODULATE,FH_TEXTURE2,0,FH_CONSTANT,0,0,0,1);
    p->stages[0].channel[1]=channel(FH_MODULATE,FH_TEXTURE0,1,FH_TEXTURE1,1,0,0,1);
    p->stages[1].constant[3]=1;
    p->stages[1].channel[0]=channel(FH_REPLACE,FH_PREVIOUS,0,0,0,0,0,1);
    p->stages[1].channel[1]=channel(FH_SUBTRACT,FH_CONSTANT,1,FH_PREVIOUS,1,0,0,1);
    p->stages[2].channel[0]=channel(FH_REPLACE,FH_PREVIOUS,0,0,0,0,0,1);
    p->stages[2].channel[1]=channel(FH_MODULATE,FH_TEXTURE2,1,FH_PREVIOUS,1,0,0,1);
    return 1;
}
unsigned fh_layered_five_tint_disabled;
int fh_layered_material_compile(FhMaterial *plan,const FhDrawState *state) {
    if(state->stages==3 && product_mask(plan,state))return 1;
    if(state->stages==5)return layered_five(plan,state);
    if((state->stages==7 || (state->stages==6 && !fh_layered_five_tint_disabled)) &&
       state->tev[state->stages-1].map==255 && state->tev[state->stages-1].color[0]==0) {
        /* Either layered family can end in an ordinary constant tint, including
         * damage flashes. Initial register-0 alpha is untouched by that
         * family, so it and the tint share one PICA constant RGBA vector. */
        const FhTevStage *t=&state->tev[state->stages-1];
        const unsigned color[]={0,14,3,15},alpha[]={7,7,7,0};
        if(memcmp(t->color,color,16) || memcmp(t->alpha,alpha,16) || t->coord!=255 ||
           t->color_op || t->alpha_op || t->color_bias || t->alpha_bias || t->color_scale || t->alpha_scale ||
           !t->color_clamp || !t->alpha_clamp || t->color_dest || t->alpha_dest)return 0;
        for(unsigned i=0;i<9;i++)if(t->indirect[i])return 0;
        unsigned k=t->kcolor;float tint[4];
        if(!(k<8 || (k>=12 && k<32)))return 0;
        for(unsigned c=0;c<3;c++)tint[c]=k<8?(8-k)/8.f:k<16?state->konst_colors[k-12][c]:state->konst_colors[k&3][(k-16)/4];
        tint[3]=state->tev_colors[1][3];
        for(unsigned c=0;c<4;c++)if(!(tint[c]>=0 && tint[c]<=1))return 0;
        FhDrawState base=*state;base.stages--;
        if(!fh_layered_material_compile(plan,&base) || plan->pipeline.count!=base.stages-1)return 0;
        FhCombiner *last=&plan->pipeline.stages[plan->pipeline.count];
        last->channel[0]=channel(FH_INTERPOLATE,FH_CONSTANT,0,FH_PREVIOUS,0,FH_CONSTANT,1,1);
        last->channel[1]=channel(FH_REPLACE,FH_PREVIOUS,1,0,0,0,0,1);
        memcpy(last->constant,tint,sizeof(tint));plan->pipeline.count++;return 1;
    }
    static const unsigned colors[7][4]={{15,8,14,10},{15,8,4,15},{15,6,8,15},{15,8,14,15},{8,0,1,15},{15,0,4,6},{0,8,14,15}};
    static const unsigned alphas[7][4]={{7,7,7,7},{7,4,3,7},{7,7,7,7},{7,4,6,7},{7,4,0,7},{7,7,7,0},{7,7,7,0}};
    const unsigned maps[]={0,1,2,2,3,255,4},coords[]={0,0,1,1,2,255,3},dest[]={2,3,3,0,0,0,0};
    if(state->stages!=6 && state->stages!=7) return 0;
    unsigned movie=state->stages==7;
    if(movie && (state->blend_mode || state->alpha_write || !state->color_write || state->alpha_compare[0]!=7 || state->alpha_compare[2] || state->alpha_compare[3]!=7)) return 0;
    if(movie && state->depth_test && state->depth_write && state->depth_compare!=3 && state->depth_compare!=6 && state->depth_compare!=7) return 0;
    for(unsigned i=0;i<state->stages;i++) {
        const FhTevStage *s=&state->tev[i];
        if(memcmp(s->color,colors[i],16) || memcmp(s->alpha,alphas[i],16) || s->color_op || s->alpha_op || s->color_bias || s->alpha_bias || s->color_scale || s->alpha_scale || !s->color_clamp || !s->alpha_clamp || s->color_dest!=dest[i] || s->alpha_dest || s->map!=maps[i] || s->coord!=coords[i]) return 0;
        if((i==0 || i==3) && s->kcolor!=0) return 0;
        for(unsigned j=0;j<9;j++) if(s->indirect[j]!=(i==1?(j==4 || j==5?6u:j==6?1u:0u):0u)) return 0;
        for(unsigned j=0;j<4;j++) {
            unsigned desired=i==1?(j==3?1:2):j;
            if(state->swaps[s->tex_swap&3][j]!=desired) return 0;
            if(i==0 && state->swaps[s->ras_swap&3][j]!=j) return 0;
        }
    }
    float opacity;
    if(state->tev[0].channel!=4 || (movie && state->tev[6].kcolor>=8) || !layered_opacity(state,&opacity)) return 0;
    const FhTexture *a=&state->textures[0],*b=&state->textures[1],*detail=&state->textures[3];
    if(a->format!=6 || b->format!=6 || a->width!=b->width || a->height!=b->height || a->wrap_s!=b->wrap_s || a->wrap_t!=b->wrap_t || a->min_filter!=b->min_filter || a->mag_filter!=b->mag_filter) return 0;
    if(detail->format!=5 && detail->format!=6 && detail->format!=14 && detail->format!=0x46) return 0;
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;plan->movie_layer=movie;plan->movie_mix=movie?(8-state->tev[6].kcolor)/8.f:0;
    FhPipeline *p=&plan->pipeline;p->count=5;p->textures=3;p->buffer_updates[0]=1;
    p->map[0]=0;p->map[1]=2;p->map[2]=3;p->coord[0]=0;p->coord[1]=1;p->coord[2]=2;
    p->texture_mode[0]=1;p->secondary_map[0]=1;p->texture_mode[2]=2;
    for(unsigned c=0;c<4;c++) p->primary[c]=c;
    p->stages[0].channel[0]=channel(FH_ADD,FH_TEXTURE0,0,FH_PRIMARY,0,0,0,1);
    p->stages[0].channel[1]=channel(FH_MODULATE,FH_TEXTURE0,1,FH_CONSTANT,1,0,0,1);p->stages[0].constant[3]=.5f;
    p->stages[1].channel[0]=channel(FH_MODULATE,FH_TEXTURE1,0,FH_PREVIOUS,1,0,0,1);
    p->stages[1].channel[1]=channel(FH_MODULATE,FH_TEXTURE1,1,FH_CONSTANT,1,0,0,1);p->stages[1].constant[3]=.5f*opacity;
    p->stages[2].channel[0]=channel(FH_MULTIPLY_ADD,FH_TEXTURE1,0,FH_PREVIOUS,1,FH_PREVIOUS,0,1);
    p->stages[2].channel[1]=channel(FH_REPLACE,FH_PREVIOUS,1,0,0,0,0,2);
    p->stages[3].channel[0]=channel(FH_MULTIPLY_ADD,FH_TEXTURE2,0,FH_PREVIOUS,9,FH_PREVIOUS,0,1);
    p->stages[3].channel[1]=channel(FH_MODULATE,FH_PREVIOUS,1,FH_TEXTURE2,1,0,0,1);
    p->stages[4].channel[0]=channel(FH_MODULATE,FH_PREVIOUS,0,FH_BUFFER,0,0,0,2);
    p->stages[4].channel[1]=channel(FH_REPLACE,FH_PREVIOUS,1,0,0,0,0,1);
    return 1;
}

/* Zero indirect matrices still add the previous stage's coordinates. Preserve
 * that sum as a rational coordinate, with bounded subdivision of its quadratic
 * numerator, then evaluate both texture/color/alpha terms on PICA. */
int fh_chained_material(const FhDrawState *s) {
    static const unsigned color[3][4]={{15,15,15,10},{15,15,15,8},{15,15,15,0}};
    static const unsigned alpha[3][4]={{7,7,7,5},{7,7,7,0},{7,4,6,0}};
    static const unsigned indirect[3][9]={{0,0,7,1,0,0,0,0,0},{1,0,7,1,0,0,1,0,0},{0}};
    if(s->raw_position || s->stages!=3 || s->texgens!=3 || s->indirect_count!=2)return 0;
    for(unsigned r=0;r<2;r++)for(unsigned c=0;c<3;c++)if(s->indirect_matrix[0][r][c]!=0)return 0;
    if(s->indirect_exponent[0]<-20 || s->indirect_exponent[0]>20)return 0;
    for(unsigned i=0;i<3;i++){
        const FhTevStage *t=s->tev+i;
        if(memcmp(t->color,color[i],16) || memcmp(t->alpha,alpha[i],16) || memcmp(t->indirect,indirect[i],36) ||
           t->color_op || t->alpha_op!=(i==2) || t->color_bias || t->alpha_bias || t->color_scale || t->alpha_scale ||
           !t->color_clamp || !t->alpha_clamp || t->color_dest || t->alpha_dest || t->coord!=i ||
           t->map!=(i==2?1u:0u) || t->channel!=(i?255u:4u))return 0;
        for(unsigned c=0;c<4;c++)if(s->swaps[t->tex_swap&3][c]!=c || s->swaps[t->ras_swap&3][c]!=c)return 0;
    }
    if(s->tev[2].kalpha!=28 || !(s->konst_colors[0][3]>=0 && s->konst_colors[0][3]<=1))return 0;
    for(unsigned i=0;i<2;i++)if(!s->textures[i].pixels || !s->textures[i].width || !s->textures[i].height ||
        !s->textures[i].min_filter || !s->textures[i].mag_filter)return 0;
    /* SU scale for coordinates zero/one comes from their last direct map,
     * both map zero; the independent alpha mask remains coordinate two. */
    return 1;
}
void fh_chained_pipeline(FhMaterial *plan,const FhDrawState *s) {
    memset(plan,0,sizeof(*plan));plan->map=plan->coord=-1;FhPipeline*p=&plan->pipeline;
    p->count=2;p->textures=2;p->map[0]=0;p->map[1]=1;p->coord[0]=7;p->coord[1]=2;
    for(unsigned c=0;c<4;c++)p->primary[c]=c;
    p->stages[0].channel[0]=channel(FH_REPLACE,FH_TEXTURE0,0,0,0,0,0,1);
    p->stages[0].channel[1]=channel(FH_MODULATE,FH_TEXTURE1,1,FH_CONSTANT,1,0,0,1);
    p->stages[0].constant[3]=s->konst_colors[0][3];
    p->stages[1].channel[0]=channel(FH_REPLACE,FH_PREVIOUS,0,0,0,0,0,1);
    p->stages[1].channel[1]=channel(FH_SUBTRACT,FH_PRIMARY,1,FH_PREVIOUS,1,0,0,1);
}
unsigned fh_chained_disabled,fh_chained_reference_bounds;
#ifdef FH_EMULATOR_TEST
static float chained_error_reference(const FhVertex v[3],const FhDrawState *s) {
    float qmin[2]={16,16},qmax[2]={0},d_error=0;
    for(unsigned i=0;i<3;i++)for(unsigned k=0;k<2;k++){
        float q=v[i].uv[k][2];if(q<qmin[k])qmin[k]=q;if(q>qmax[k])qmax[k]=q;
    }
    for(unsigned i=0;i<3;i++){unsigned j=(i+1)%3;d_error+=fabsf((v[i].uv[0][2]-v[j].uv[0][2])*(v[i].uv[1][2]-v[j].uv[1][2]))*.25f;}
    float denominator=qmin[0]*qmin[1],bound=0;
    for(unsigned c=0;c<2;c++){
        float n_error=0,nmax[2]={0};
        for(unsigned i=0;i<3;i++){
            unsigned j=(i+1)%3;
            n_error+=fabsf((v[i].uv[0][c]-v[j].uv[0][c])*(v[i].uv[1][2]-v[j].uv[1][2])+
                          (v[i].uv[1][c]-v[j].uv[1][c])*(v[i].uv[0][2]-v[j].uv[0][2]))*.25f;
            for(unsigned k=0;k<2;k++){float a=fabsf(v[i].uv[k][c]);if(a>nmax[k])nmax[k]=a;}
        }
        float error=(n_error+(nmax[0]*qmax[1]+nmax[1]*qmax[0])/denominator*d_error)/denominator;
        error*=c?s->textures[0].height:s->textures[0].width;if(error>bound)bound=error;
    }
    return bound;
}
#endif
/* A product of interpolants differs from the interpolated product by
 * sum(e_ij*w_i*w_j). Summing three independent edge maxima overestimates
 * its error: those maxima cannot all occur at once. Evaluate the boundary
 * extrema and the possible interior stationary point instead. Near-singular
 * systems use max(abs(e))/3, since sum(w_i*w_j)<=1/3. The float guard is
 * additional to the existing 1/256-texel arithmetic reserve below. */
static float chained_quadratic_bound(const float e[3]) {
    float bound=.25f*fmaxf(fabsf(e[0]),fmaxf(fabsf(e[1]),fabsf(e[2])));
    float a=e[0],b=e[1],c=e[2],d=a-b-c,det=4*b*c-d*d;
    float scale=fmaxf(fabsf(4*b*c),d*d);
    if(fabsf(det)<=scale*1e-4f)return fmaxf(bound,fmaxf(fabsf(a),fmaxf(fabsf(b),fabsf(c)))/3.f);
    if(det!=0){
        float x=b*(2*c+d)/det,y=c*(2*b+d)/det,z=1-x-y;
        if(x>0 && y>0 && z>0){float value=fabsf(a*x*y+b*y*z+c*z*x);if(value>bound)bound=value;}
    }
    return bound*(1+1e-5f);
}
static float chained_error_tight(const FhVertex v[3],const FhDrawState *s) {
    float qmin[2]={16,16},qmax[2]={0},d_edges[3];
    for(unsigned i=0;i<3;i++)for(unsigned k=0;k<2;k++){
        float q=v[i].uv[k][2];if(q<qmin[k])qmin[k]=q;if(q>qmax[k])qmax[k]=q;
    }
    for(unsigned i=0;i<3;i++){unsigned j=(i+1)%3;d_edges[i]=(v[i].uv[0][2]-v[j].uv[0][2])*(v[i].uv[1][2]-v[j].uv[1][2]);}
    float d_error=chained_quadratic_bound(d_edges),denominator=qmin[0]*qmin[1],bound=0;
    for(unsigned c=0;c<2;c++){
        float n_edges[3],nmax[2]={0};
        for(unsigned i=0;i<3;i++){
            unsigned j=(i+1)%3;
            n_edges[i]=(v[i].uv[0][c]-v[j].uv[0][c])*(v[i].uv[1][2]-v[j].uv[1][2])+
                       (v[i].uv[1][c]-v[j].uv[1][c])*(v[i].uv[0][2]-v[j].uv[0][2]);
            for(unsigned k=0;k<2;k++){float a=fabsf(v[i].uv[k][c]);if(a>nmax[k])nmax[k]=a;}
        }
        float error=(chained_quadratic_bound(n_edges)+(nmax[0]*qmax[1]+nmax[1]*qmax[0])/denominator*d_error)/denominator;
        error*=c?s->textures[0].height:s->textures[0].width;if(error>bound)bound=error;
    }
    return bound;
}
static float chained_error(const FhVertex v[3],const FhDrawState *s) {
#ifdef FH_EMULATOR_TEST
    if(fh_chained_reference_bounds)return chained_error_reference(v,s);
#endif
    return chained_error_tight(v,s);
}
static unsigned chained_split(FhVertex*out,unsigned count,const FhVertex v[3],const FhDrawState*s,unsigned depth) {
    /* Native-screen effect detail: bound coordinate error to 1/8 texture
     * texel, with 1/256 reserved for arithmetic. This deliberately reduces
     * UV subdivision versus the previous 1/32-texel policy, not effect
     * geometry, alpha, timing or simulation. Only recognized linearly
     * filtered chained effects qualify. Unproven bounds still fall back. */
    if(chained_error(v,s)<=31.f/256){
        if(count>381)return 385;
        for(unsigned i=0;i<3;i++){
            out[count+i]=v[i];float q0=v[i].uv[0][2],q1=v[i].uv[1][2];
            for(unsigned c=0;c<2;c++)out[count+i].uv[7][c]=v[i].uv[0][c]*q1+v[i].uv[1][c]*q0;
            out[count+i].uv[7][2]=q0*q1;
        }
        return count+3;
    }
    if(depth==12)return 385;
    unsigned edge=0;float longest=-1;
    /* Split the edge contributing the largest product error. A large UV
     * distance alone need not cause any error when its Q is constant. */
    for(unsigned i=0;i<3;i++){
        unsigned j=(i+1)%3;float length=0;
        float dq0=v[i].uv[0][2]-v[j].uv[0][2],dq1=v[i].uv[1][2]-v[j].uv[1][2];
        for(unsigned c=0;c<2;c++)length+=fabsf((v[i].uv[0][c]-v[j].uv[0][c])*dq1+(v[i].uv[1][c]-v[j].uv[1][c])*dq0);
        length+=fabsf(dq0*dq1);
#ifdef FH_EMULATOR_TEST
        if(fh_chained_reference_bounds){length=0;for(unsigned k=0;k<2;k++)for(unsigned c=0;c<2;c++){
            float d=v[i].uv[k][c]/v[i].uv[k][2]-v[j].uv[k][c]/v[j].uv[k][2];length+=d*d;
        }}
#endif
        if(length>longest){edge=i;longest=length;}
    }
    unsigned j=(edge+1)%3,k=(edge+2)%3;FhVertex child[3],mid;
    for(unsigned c=0;c<sizeof(mid)/sizeof(float);c++)((float*)&mid)[c]=(((const float*)&v[edge])[c]+((const float*)&v[j])[c])*.5f;
    child[0]=v[edge];child[1]=mid;child[2]=v[k];count=chained_split(out,count,child,s,depth+1);
    if(count>384)return count;
    child[0]=mid;child[1]=v[j];return chained_split(out,count,child,s,depth+1);
}
unsigned fh_chained_triangle_append(FhVertex out[384],unsigned prefix,const FhVertex v[3],const FhDrawState*s) {
    if(prefix>384 || prefix%3)return 0;
    if(s->textures[0].width>64 || s->textures[0].height>64)return 0;
    for(unsigned i=0;i<3;i++){
        if(v[i].uv[2][2]!=1 || !(v[i].color[0][3]>=0 && v[i].color[0][3]<=1))return 0;
        for(unsigned k=0;k<2;k++)for(unsigned c=0;c<3;c++){
            float value=v[i].uv[k][c];if(!isfinite(value) || fabsf(value)>16 || (c==2 && value<1))return 0;
        }
    }
    return chained_split(out,prefix,v,s,0);
}
unsigned fh_chained_triangle_capacity(FhVertex out[384],const FhVertex v[3],const FhDrawState*s) {
    return fh_chained_triangle_append(out,0,v,s);
}
unsigned fh_chained_triangle(FhVertex out[384],const FhVertex v[3],const FhDrawState*s) {
    unsigned count=fh_chained_triangle_capacity(out,v,s);return count<=384?count:0;
}
