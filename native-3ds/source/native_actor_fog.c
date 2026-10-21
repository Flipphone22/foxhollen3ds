#include "fh_native_actor_fog.h"
#include <math.h>
#include <string.h>
_Static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"Actor fog packet byte order");
static unsigned half(const unsigned char*p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static unsigned word(const unsigned char*p){return half(p)|half(p+2)<<16;}
static unsigned aligned(unsigned n,unsigned alignment){return (n+alignment-1)&~(alignment-1);}
static int zeros(const unsigned char*p,unsigned n){for(unsigned i=0;i<n;i++)if(p[i])return 0;return 1;}
int fh_native_actor_fog_draw(const FhActorFogAsset*a,unsigned group,FhActorFogDraw*out){
    if(!a || !out || group>=a->groups)return 0;
    const unsigned char*p=a->meta+96+group*240;
    *out=(FhActorFogDraw){word(p),word(p+4),word(p+8),word(p+12),word(p+16),word(p+20),word(p+24),
        word(p+28),word(p+32),word(p+36),word(p+40),word(p+44),word(p+48),
        (const uint16_t*)(p+56),(const FhPicaActorFogMatrix*)(p+100),
        (const FhPicaActorFogRef*)(a->meta+word(p+52)),(const uint16_t*)(p+192)};
    return 1;
}
const FhPicaActorFogPosePlan *fh_native_actor_fog_pose_plan(const FhActorFogAsset*a,unsigned index){
    if(!a || index>=a->pose_plans)return NULL;
    return (const FhPicaActorFogPosePlan*)(a->meta+a->plans_at+index*36+4);
}
int fh_native_actor_fog_open(FhActorFogAsset*out,const void*metadata,unsigned metadata_bytes,
    const void*geometry,unsigned geometry_bytes,unsigned id,unsigned source_bytes,
    unsigned source_crc,unsigned variant){
    const unsigned char*p=metadata,*gpu=geometry;
    if(!out || !p || !gpu || ((uintptr_t)p&3) || ((uintptr_t)gpu&15) || id>>31 ||
       !source_bytes || source_bytes>4*1024*1024 || metadata_bytes<128 || metadata_bytes>FH_ACTOR_FOG_META_CAP || !geometry_bytes ||
       geometry_bytes>FH_ACTOR_FOG_GPU_CAP || memcmp(p,"FHAFG002",8) ||
       word(p+8)!=metadata_bytes || word(p+12)!=geometry_bytes || word(p+16)!=id ||
       word(p+20)!=source_bytes || word(p+24)!=source_crc || word(p+28)!=variant)return 0;
    unsigned groups=word(p+32),sections=word(p+36),section_at=word(p+44),refs=word(p+48);
    unsigned bones=word(p+52),joints=word(p+56),extra=word(p+60),positions=word(p+64),
        normals=word(p+68),colors=word(p+72),uvs=word(p+76),fixed=word(p+80),ref_bytes=word(p+84);
    unsigned plans=word(p+88),plan_at=word(p+92);
    if(!groups || groups>256 || sections<groups || sections>256 || word(p+40)!=96 ||
       section_at!=96+groups*240 || refs!=section_at+sections*32 || refs>metadata_bytes ||
       ref_bytes>metadata_bytes-refs || plan_at!=refs+ref_bytes || !plans || plans>512 ||
       plans>(metadata_bytes-plan_at)/36 || metadata_bytes!=aligned(plan_at+plans*36,128) ||
       joints>255 || extra>255 || joints+extra>256 || fixed>1 || (!joints&&(fixed||extra)) ||
       bones!=(joints?joints+fixed:1) || bones+extra>256 ||
       !positions || positions>65535 || normals>65535 || colors>65535 || uvs>65535)return 0;
    FhActorFogAsset a={p,gpu,metadata_bytes,geometry_bytes,id,variant,groups,sections,section_at,
        plans,plan_at,joints+extra>=2&&joints+extra<=100};
    unsigned gpu_end=0,ref_end=refs,section_end=0,source_poses=joints?joints+extra:1;
    unsigned char used[512]={0};
    for(unsigned i=0;i<plans;i++){
        const FhPicaActorFogMatrix*k=(const FhPicaActorFogMatrix*)(p+plan_at+i*36);
        const FhPicaActorFogPosePlan*q=fh_native_actor_fog_pose_plan(&a,i);
        unsigned world=k->kind==FH_ACTOR_FOG_WORLD_POSITION || k->kind==FH_ACTOR_FOG_WORLD_NORMAL;
        if(!fh_pica_actor_fog_matrix_valid(k,source_poses,fixed) || q->kind!=k->kind || q->a>=(joints?joints:1) ||
           q->b>=(joints?joints:1) || q->weight>4 || q->blend>1 ||
           !zeros(q->reserved,3) || q->blend!=(!world&&joints&&k->joint>=joints))return 0;
        if(!q->blend && (q->a!=k->joint || q->b!=q->a || q->weight!=4 ||
           !zeros((const unsigned char*)q->tail_a,24)))return 0;
        for(unsigned j=0;j<3;j++)if(!isfinite(q->tail_a[j]) || !isfinite(q->tail_b[j]))return 0;
        for(unsigned j=0;j<i;j++)if(!memcmp(k,p+plan_at+j*36,4))return 0;
    }
    for(unsigned i=0;i<groups;i++){
        const unsigned char*g=p+96+i*240;
        if(word(g+52)!=ref_end)return 0;
        FhActorFogDraw d;fh_native_actor_fog_draw(&a,i,&d);
        if(!d.vertices || d.vertices>8192 || !d.indices || d.indices>16384 || d.indices%3 ||
           !d.bones || d.bones>22 || !d.fog_matrices || d.fog_matrices>22 || d.bones+d.fog_matrices>22 ||
           d.first_section!=section_end || !d.sections || d.sections>sections-section_end ||
           (d.format!=3&&d.format!=5&&d.format!=6&&d.format!=7) || d.material>=64 || d.two_uv>1 ||
           word(g+188) || word(g+236) || word(g+52)!=ref_end || d.vertices>(plan_at-ref_end)/16 ||
           d.vertex_offset!=aligned(gpu_end,16) || d.vertex_offset>geometry_bytes ||
           d.vertices>(geometry_bytes-d.vertex_offset)/40)return 0;
        unsigned primary_end=d.vertex_offset+d.vertices*40;
        if(d.fog_offset!=aligned(primary_end,16) || d.fog_offset>geometry_bytes ||
           d.vertices>(geometry_bytes-d.fog_offset)/16)return 0;
        unsigned fog_end=d.fog_offset+d.vertices*16;
        if(d.index_offset!=fog_end || d.indices>(geometry_bytes-d.index_offset)/2 ||
           !zeros(gpu+gpu_end,d.vertex_offset-gpu_end) || !zeros(gpu+primary_end,d.fog_offset-primary_end))return 0;
        for(unsigned j=0;j<22;j++){
            if(j<d.bones){
                if(d.bone[j]>=bones)return 0;
                for(unsigned k=0;k<j;k++)if(d.bone[k]==d.bone[j])return 0;
            }else if(d.bone[j])return 0;
            if(j<d.fog_matrices){
                if(!fh_pica_actor_fog_matrix_valid(d.fog+j,source_poses,fixed))return 0;
                for(unsigned k=0;k<j;k++)if(!memcmp(d.fog+j,d.fog+k,4))return 0;
                unsigned slot=d.pose_plan[j];
                if(slot>=plans || memcmp(d.fog+j,p+plan_at+slot*36,4))return 0;
                used[slot]=1;
            }else if(!zeros((const unsigned char*)(d.fog+j),4) || d.pose_plan[j])return 0;
        }
        float scale=d.format==7?1.f:d.format==5?1.f/8:1.f/256;
        for(unsigned j=0;j<d.vertices;j++){
            const FhModelCompactVertex*v=(const FhModelCompactVertex*)(gpu+d.vertex_offset)+j;
            const FhPicaActorFogGpuVertex*f=(const FhPicaActorFogGpuVertex*)(gpu+d.fog_offset)+j;
            const FhPicaActorFogRef*r=d.refs+j;
            if(r->visible.position>=positions || (r->visible.normal!=65535&&r->visible.normal>=normals) ||
               (r->visible.color!=65535&&r->visible.color>=colors) || (r->visible.uv!=65535&&r->visible.uv>=uvs) ||
               r->visible.joint>=bones+extra || (d.two_uv?r->visible.material>=uvs:r->visible.material!=d.material) ||
               v->position[3]!=1 || v->normal[3] || v->skin[0]%3 || v->skin[1]%3 ||
               v->skin[0]>=d.bones*3 || v->skin[1]>=d.bones*3 ||
               (fixed?(unsigned)v->skin[2]+v->skin[3]!=128:v->skin[2]>4||v->skin[3]) ||
               f->skin[0]%3 || f->skin[0]<d.bones*3 || f->skin[0]>=(d.bones+d.fog_matrices)*3 ||
               f->skin[1]!=f->skin[0] || f->skin[2]!=4 || f->skin[3] ||
               memcmp(&r->fog,d.fog+f->skin[0]/3-d.bones,4))return 0;
            for(unsigned k=0;k<3;k++)if(!isfinite(v->tail[k]) || !isfinite(f->position[k]) ||
                f->position[k]!=(float)v->position[k]*scale)return 0;
            for(unsigned k=0;k<4;k++)if(v->color[k]>15)return 0;
            if(!d.two_uv&&(v->uv[0]!=v->uv[2]||v->uv[1]!=v->uv[3]))return 0;
        }
        for(unsigned j=0;j<d.indices;j++)if(half(gpu+d.index_offset+j*2)>=d.vertices)return 0;
        unsigned index_end=0;
        for(unsigned j=0;j<d.sections;j++){
            const unsigned char*s=p+section_at+(section_end+j)*32;unsigned count=word(s+20);
            if(word(s)>=256 || word(s+4)!=d.material || word(s+8) || word(s+12)>65535 ||
               word(s+16)!=index_end || !count || count%3 || count>d.indices-index_end ||
               word(s+24)!=section_end+j || word(s+28))return 0;
            index_end+=count;
        }
        if(index_end!=d.indices)return 0;
        gpu_end=d.index_offset+d.indices*2;ref_end+=d.vertices*16;section_end+=d.sections;
    }
    if(gpu_end!=geometry_bytes || section_end!=sections || ref_end!=refs+ref_bytes ||
       !zeros(p+plan_at+plans*36,metadata_bytes-plan_at-plans*36))return 0;
    for(unsigned i=0;i<plans;i++)if(!used[i])return 0;
    *out=a;return 1;
}
