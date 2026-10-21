#include "fh_replay_timing.h"
#include "fh_native_model.h"
#include <math.h>
#include <string.h>
static unsigned be16(const unsigned char *p){return (unsigned)p[0]*256+p[1];}
static int signed16(const unsigned char *p){unsigned v=be16(p);return v<32768?(int)v:(int)v-65536;}
static int signed8(unsigned char v){return v<128?(int)v:(int)v-256;}
static int slot(const uint16_t *bones,unsigned n,unsigned bone){
    for(unsigned i=0;i<n;i++)if(bones[i]==bone)return (int)i;
    return -1;
}
static int valid_bones(const uint16_t *bones,unsigned n,unsigned total){
    if(!bones || !n || n>FH_MODEL_BONES || !total || total>256)return 0;
    for(unsigned i=0;i<n;i++){
        if(bones[i]>=total)return 0;
        for(unsigned j=0;j<i;j++)if(bones[i]==bones[j])return 0;
    }
    return 1;
}
int fh_native_model_vertices(FhModelVertex *out,unsigned capacity,
    const FhModelRef *refs,unsigned count,const FhModelSource *s,
    const uint16_t *bones,unsigned bone_count){
    if(!out || !refs || !s || !count || count>FH_MODEL_VERTICES || count>capacity ||
       !s->positions || !s->bindings || !s->tails || !s->binding_count || s->binding_count>256 ||
       !valid_bones(bones,bone_count,s->bone_count) ||
       (s->normal_stride!=3 && s->normal_stride!=9) ||
       (s->format!=3 && s->format!=5 && s->format!=6 && s->format!=7))return 0;
    for(unsigned i=0;i<bone_count;i++)for(unsigned c=0;c<3;c++)if(!isfinite(s->tails[bones[i]][c]))return 0;
    /* A rejected record must not publish a partly decoded mesh. */
    for(unsigned i=0;i<count;i++){
        const FhModelRef *r=refs+i;
        if(r->position>=s->position_count || r->joint>=s->binding_count ||
           (r->normal!=UINT16_MAX && (!s->normals || r->normal>=s->normal_count)) ||
           (r->color!=UINT16_MAX && (!s->colors || r->color>=s->color_count)) ||
           (r->uv!=UINT16_MAX && (!s->uvs || r->uv>=s->uv_count)))return 0;
        const FhModelBinding *b=s->bindings+r->joint;
        if(b->weight>4 || b->bind_space>1 || b->a>=s->bone_count || b->b>=s->bone_count ||
           (!b->bind_space && (b->a!=b->b || b->weight!=4)) ||
           (b->weight && slot(bones,bone_count,b->a)<0) ||
           (b->weight<4 && slot(bones,bone_count,b->b)<0))return 0;
    }
    float scale=s->format==7?1.f:s->format==5?1.f/8:1.f/256;
    float uv_scale=s->format==5?1.f/256:1.f/1024;
    for(unsigned i=0;i<count;i++){
        const FhModelRef *r=refs+i;const FhModelBinding *b=s->bindings+r->joint;
        FhModelVertex *v=out+i;
        for(unsigned c=0;c<3;c++){
            v->position[c]=signed16(s->positions+r->position*6+c*2)*scale;
            if(!b->bind_space)v->position[c]+=s->tails[b->a][c];
            v->normal[c]=r->normal==UINT16_MAX?0.f:signed8(s->normals[r->normal*s->normal_stride+c])*(1.f/64);
        }
        v->position[3]=1;
        unsigned color=r->color==UINT16_MAX?65535:be16(s->colors+r->color*2);
        for(unsigned c=0;c<4;c++)v->color[c]=((color>>(12-4*c))&15)*(1.f/15);
        for(unsigned c=0;c<2;c++)v->uv[c]=r->uv==UINT16_MAX?0.f:signed16(s->uvs+r->uv*4+c*2)*uv_scale;
        int a=slot(bones,bone_count,b->a),d=slot(bones,bone_count,b->b);
        if(!b->weight)a=d;
        if(b->weight==4)d=a;
        v->skin[0]=a*3;v->skin[1]=d*3;v->skin[2]=b->weight*.25f;
    }
    return 1;
}
int fh_native_model_refresh(FhModelVertex *out,unsigned capacity,
    const FhModelRef *refs,unsigned count,const FhModelSource *s,unsigned changed){
    if(!out || !refs || !s || !count || count>FH_MODEL_VERTICES || count>capacity ||
       !changed || (changed&~15u) || (s->format!=3 && s->format!=5 && s->format!=6 && s->format!=7) ||
       ((changed&FH_MODEL_POSITION) && (!s->positions || !s->bindings || !s->tails ||
        !s->binding_count || s->binding_count>256 || !s->bone_count || s->bone_count>256)) ||
       ((changed&FH_MODEL_NORMAL) && s->normal_stride!=3 && s->normal_stride!=9))return 0;
    /* The fixed packet already contains its skin palette. Resolve rigid bind
     * translations once per binding, not twice per vertex through bone scans. */
    unsigned char binding_valid[256]={0};
    float translation[256][3];
    if(changed&FH_MODEL_POSITION)for(unsigned i=0;i<s->binding_count;i++){
        const FhModelBinding *b=s->bindings+i;
        if(b->weight>4 || b->bind_space>1 || b->a>=s->bone_count || b->b>=s->bone_count ||
           (!b->bind_space && (b->a!=b->b || b->weight!=4)))continue;
        unsigned valid=1;
        for(unsigned c=0;c<3;c++){
            float t=b->bind_space?0:s->tails[b->a][c];
            if(!isfinite(t))valid=0;
            translation[i][c]=t;
        }
        binding_valid[i]=valid;
    }
    for(unsigned i=0;i<count;i++){
        const FhModelRef *r=refs+i;
        if(((changed&FH_MODEL_POSITION) && (r->position>=s->position_count ||
             r->joint>=s->binding_count || !binding_valid[r->joint])) ||
           ((changed&FH_MODEL_NORMAL) && r->normal!=UINT16_MAX && (!s->normals || r->normal>=s->normal_count)) ||
           ((changed&FH_MODEL_COLOR) && r->color!=UINT16_MAX && (!s->colors || r->color>=s->color_count)) ||
           ((changed&FH_MODEL_UV) && r->uv!=UINT16_MAX && (!s->uvs || r->uv>=s->uv_count)))return 0;
    }
    float scale=s->format==7?1.f:s->format==5?1.f/8:1.f/256;
    float uv_scale=s->format==5?1.f/256:1.f/1024;
    /* Separate loops allow the common position/normal refresh to avoid both
     * decode work and writes for immutable color, UV and skin attributes. */
    if(changed&FH_MODEL_POSITION)for(unsigned i=0;i<count;i++){
        const FhModelRef *r=refs+i;FhModelVertex *v=out+i;
        for(unsigned c=0;c<3;c++){
            float p=signed16(s->positions+r->position*6+c*2)*scale;
            if(!s->bindings[r->joint].bind_space)p+=translation[r->joint][c];
            v->position[c]=p;
        }
    }
    if(changed&FH_MODEL_NORMAL)for(unsigned i=0;i<count;i++){
        const FhModelRef *r=refs+i;
        for(unsigned c=0;c<3;c++)out[i].normal[c]=r->normal==UINT16_MAX?0.f:
            signed8(s->normals[r->normal*s->normal_stride+c])*(1.f/64);
    }
    if(changed&FH_MODEL_COLOR)for(unsigned i=0;i<count;i++){
        const FhModelRef *r=refs+i;unsigned color=r->color==UINT16_MAX?65535:be16(s->colors+r->color*2);
        for(unsigned c=0;c<4;c++)out[i].color[c]=((color>>(12-4*c))&15)*(1.f/15);
    }
    if(changed&FH_MODEL_UV)for(unsigned i=0;i<count;i++){
        const FhModelRef *r=refs+i;
        for(unsigned c=0;c<2;c++)out[i].uv[c]=r->uv==UINT16_MAX?0.f:
            signed16(s->uvs+r->uv*4+c*2)*uv_scale;
    }
    return 1;
}
static int compact_source_valid(const FhModelRef *refs,unsigned count,const FhModelSource *s,unsigned changed){
    if(!refs || !s || !count || count>FH_MODEL_VERTICES || !changed || (changed&~15u) ||
       (s->format!=3 && s->format!=5 && s->format!=6 && s->format!=7) ||
       ((changed&FH_MODEL_POSITION) && !s->positions) ||
       ((changed&FH_MODEL_NORMAL) && s->normal_stride!=3 && s->normal_stride!=9))return 0;
    for(unsigned i=0;i<count;i++){
        const FhModelRef *r=refs+i;
        if(((changed&FH_MODEL_POSITION) && r->position>=s->position_count) ||
           ((changed&FH_MODEL_NORMAL) && r->normal!=UINT16_MAX && (!s->normals || r->normal>=s->normal_count)) ||
           ((changed&FH_MODEL_COLOR) && r->color!=UINT16_MAX && (!s->colors || r->color>=s->color_count)) ||
           ((changed&FH_MODEL_UV) && r->uv!=UINT16_MAX && (!s->uvs || r->uv>=s->uv_count)) ||
           ((changed&FH_MODEL_UV) && s->secondary_uv && (!s->uvs || r->material>=s->uv_count)))return 0;
    }
    return 1;
}
static void compact_attributes(FhModelCompactVertex *v,const FhModelRef *r,const FhModelSource *s,unsigned changed){
    if(changed&FH_MODEL_POSITION){
        const unsigned char *p=s->positions+r->position*6;
        v->position[0]=signed16(p);v->position[1]=signed16(p+2);v->position[2]=signed16(p+4);
    }
    if(changed&FH_MODEL_NORMAL){
        for(unsigned c=0;c<3;c++)v->normal[c]=r->normal==UINT16_MAX?0:(int8_t)s->normals[r->normal*s->normal_stride+c];
    }
    if(changed&FH_MODEL_COLOR){
        unsigned color=r->color==UINT16_MAX?65535:be16(s->colors+r->color*2);
        for(unsigned c=0;c<4;c++)v->color[c]=(color>>(12-4*c))&15;
    }
    if(changed&FH_MODEL_UV){
        for(unsigned c=0;c<2;c++)v->uv[c]=r->uv==UINT16_MAX?0:signed16(s->uvs+r->uv*4+c*2);
        for(unsigned c=0;c<2;c++)v->uv[c+2]=s->secondary_uv?signed16(s->uvs+r->material*4+c*2):v->uv[c];
    }
}
int fh_native_model_compact(FhModelVertex *storage,const FhModelRef *refs,unsigned count,const FhModelSource *s){
    if(!storage || !compact_source_valid(refs,count,s,15) || !s->bindings || !s->tails ||
       !s->binding_count || s->binding_count>256 || !s->bone_count || s->bone_count>256)return 0;
    /* Preflight the complete packet before overwriting any float vertices. */
    for(unsigned i=0;i<count;i++){
        if(refs[i].joint>=s->binding_count)return 0;
        const FhModelBinding *b=s->bindings+refs[i].joint;
        if(b->weight>4 || b->bind_space>1 || b->a>=s->bone_count || b->b>=s->bone_count ||
           (!b->bind_space && (b->a!=b->b || b->weight!=4)))return 0;
        for(unsigned c=0;c<3;c++)if(!isfinite(s->tails[b->a][c]))return 0;
        for(unsigned c=0;c<2;c++){
            float n=storage[i].skin[c];
            if(!isfinite(n) || n<0 || n>=FH_MODEL_BONES*3 || n!=(unsigned)n || (unsigned)n%3)return 0;
        }
        if(storage[i].skin[2]!=b->weight*.25f)return 0;
    }
    /* Read each original vertex before the smaller output can overlap it. */
    for(unsigned i=0;i<count;i++){
        const FhModelBinding *b=s->bindings+refs[i].joint;
        FhModelCompactVertex v={.position={0,0,0,1},
            .skin={(uint8_t)storage[i].skin[0],(uint8_t)storage[i].skin[1],b->weight,0}};
        for(unsigned c=0;c<3;c++)v.tail[c]=b->bind_space?0:s->tails[b->a][c];
        compact_attributes(&v,refs+i,s,15);
        memcpy((unsigned char*)storage+i*sizeof(v),&v,sizeof(v));
    }
    return 1;
}
int fh_native_model_refresh_compact(FhModelCompactVertex *out,unsigned capacity,
    const FhModelRef *refs,unsigned count,const FhModelSource *s,unsigned changed){
    if(!out || count>capacity || !compact_source_valid(refs,count,s,changed))return 0;
    for(unsigned i=0;i<count;i++)compact_attributes(out+i,refs+i,s,changed);
    return 1;
}
int fh_native_model_palette(float out[FH_MODEL_BONES][3][4],const uint16_t *bones,
    unsigned count,const float *poses,unsigned pose_count,const float (*tails)[3],const float view[3][4]){
    FH_REPLAY_SCOPE(MODEL_PALETTE);
    if(!out || !poses || !tails || !view || !valid_bones(bones,count,pose_count))return 0;
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)if(!isfinite(view[r][c]))return 0;
    for(unsigned i=0;i<count;i++){
        const float *m=poses+bones[i]*16;
        for(unsigned j=0;j<12;j++)if(!isfinite(m[j]))return 0;
        for(unsigned j=0;j<3;j++)if(!isfinite(tails[bones[i]][j]))return 0;
    }
    for(unsigned i=0;i<count;i++){
        const float *m=poses+bones[i]*16,*t=tails[bones[i]];
        float moved[3];
        for(unsigned r=0;r<3;r++)moved[r]=m[r*4+3]-m[r*4]*t[0]-m[r*4+1]*t[1]-m[r*4+2]*t[2];
        for(unsigned r=0;r<3;r++){
            for(unsigned c=0;c<3;c++)out[i][r][c]=view[r][0]*m[c]+view[r][1]*m[4+c]+view[r][2]*m[8+c];
            out[i][r][3]=view[r][0]*moved[0]+view[r][1]*moved[1]+view[r][2]*moved[2]+view[r][3];
        }
    }
    return 1;
}
