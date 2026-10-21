#include "fh_native_scene.h"
#include <math.h>
#include <string.h>
_Static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"Scene packet byte order");
static unsigned h(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static unsigned w(const unsigned char *p){return h(p)|h(p+2)<<16;}
static unsigned align16(unsigned n){return (n+15)&~15u;}
unsigned fh_native_scene_compact_model(FhSceneAsset *a,const FhModelSource *source){
    if(!a || !source || !a->direct || a->block || a->compact)return 0;
    FhModelSource s=*source;
    s.bindings=(const FhModelBinding*)(a->meta+a->bindings_at);s.binding_count=a->bindings;
    s.tails=(const float(*)[3])(a->meta+a->tails_at);s.bone_count=a->bones;
    unsigned end=0;
    for(unsigned i=0;i<a->groups;i++){
        FhSceneGroup g;fh_native_scene_group(a,i,&g);s.format=g.format;s.secondary_uv=g.two_uv;
        if(!fh_native_model_compact((FhModelVertex*)g.vertex,g.refs,g.vertices,&s))return 0;
        unsigned at=align16(end),bytes=g.vertices*sizeof(FhModelCompactVertex),idx=at+bytes;
        /* Shrinking in ascending group order cannot overwrite an unread
         * following group. memmove handles overlap inside the current group. */
        unsigned char *gpu=(unsigned char*)a->gpu,*meta=(unsigned char*)a->meta+a->groups_at+i*96;
        memmove(gpu+at,g.vertex,bytes);memmove(gpu+idx,g.index,g.indices*2);
        memcpy(meta+28,&at,4);memcpy(meta+32,&idx,4);end=idx+g.indices*2;
    }
    a->compact=1;return end;
}
int fh_native_scene_group(const FhSceneAsset *a,unsigned index,FhSceneGroup *out){
    if(!a || !out || index>=a->groups)return 0;
    const unsigned char *p=a->meta+a->groups_at+index*96;
    *out=(FhSceneGroup){w(p),w(p+4),w(p+8),w(p+12),w(p+16),w(p+20),w(p+24),w(p+36),w(p+40),
        (const FhModelVertex*)(a->gpu+w(p+28)),(const uint16_t*)(a->gpu+w(p+32)),(const uint16_t*)(p+48),a->block?NULL:(const FhModelRef*)(a->meta+w(p+44)),w(p+92)};
    return 1;
}
int fh_native_scene_section(const FhSceneAsset *a,unsigned index,FhSceneSection *out){
    if(!a || !out || index>=a->sections)return 0;
    const unsigned char *p=a->meta+a->sections_at+index*32;
    *out=(FhSceneSection){w(p),w(p+4),w(p+8),w(p+12),w(p+16),w(p+20),w(p+24),w(p+28)};return 1;
}
int fh_native_scene_draw(const FhSceneAsset *a,unsigned ordinal,unsigned list,unsigned shader,unsigned *group){
    if(!a || !group || ordinal>=a->draws)return 0;
    const unsigned char *p=a->meta+a->draws_at+ordinal*8;
    if(h(p)!=list || h(p+2)!=shader)return 0;
    *group=h(p+4);return 1;
}
int fh_native_scene_stream_draw(const FhSceneAsset *a,unsigned stream,unsigned bit,unsigned list,unsigned shader,unsigned *ordinal){
    if(!a || !ordinal || !a->block || (a->meta[7]!='6' && a->meta[7]!='7' && a->meta[7]!='8' && a->meta[7]!='9') || stream>2)return 0;
    unsigned first=0;
    for(unsigned i=0;i<stream;i++)first+=w(a->meta+80+i*4);
    unsigned end=first+w(a->meta+80+stream*4);
    for(unsigned i=first;i<end;i++){
        const unsigned char *p=a->meta+a->draws_at+i*8;
        if(h(p+6)==bit && h(p)==list && h(p+2)==shader){*ordinal=i;return 1;}
    }
    return 0;
}
int fh_native_scene_open(FhSceneAsset *out,const void *metadata,unsigned metadata_bytes,
    const void *geometry,unsigned geometry_bytes,unsigned id,unsigned source_bytes,unsigned source_crc){
    const unsigned char *b=metadata;
    if(!out || !b || !geometry || ((uintptr_t)b&3) || ((uintptr_t)geometry&15) ||
       metadata_bytes<128 || metadata_bytes>256*1024 || geometry_bytes>2*1024*1024 ||
       (memcmp(b,"FHNSCN02",8) && memcmp(b,"FHNSCN03",8) && memcmp(b,"FHNSCN04",8) && memcmp(b,"FHNSCN05",8) && memcmp(b,"FHNSCN06",8) && memcmp(b,"FHNSCN07",8) && memcmp(b,"FHNSCN08",8) && memcmp(b,"FHNSCN09",8) && memcmp(b,"FHNSCN0A",8) && memcmp(b,"FHNSCN0B",8)) || w(b+8)!=metadata_bytes+geometry_bytes ||
       w(b+12)!=id || w(b+16)!=source_bytes || w(b+20)!=source_crc ||
       w(b+72)!=geometry_bytes || w(b+68)!=metadata_bytes)return 0;
    unsigned mixed_world=b[7]=='9',world_refs=b[7]=='8' || mixed_world,streams=b[7]=='6' || b[7]=='7' || world_refs,packed=b[7]=='7' || world_refs;
    unsigned stream_actor=b[7]=='B',packed_actor=b[7]=='A' || stream_actor;
    if(packed_actor && id>>31)return 0;
    if(streams){
        if(!(id>>31) || w(b+80)>256 || w(b+84)>256 || w(b+88)>256 ||
            w(b+80)+w(b+84)+w(b+88)!=w(b+32) ||
            (world_refs?(!w(b+92) || w(b+92)>65535):w(b+92)))return 0;
    }else for(unsigned i=80;i<96;i+=4)if(w(b+i))return 0;
    FhSceneAsset a={b,geometry,metadata_bytes,geometry_bytes,id,w(b+24),w(b+28),w(b+32),w(b+36),w(b+40),
        w(b+44),w(b+48),w(b+52),w(b+56),w(b+60),w(b+64),b[7]>='3',packed || packed_actor};
    if(a.block!=(id>>31) || !a.groups || a.groups>256 || !a.draws || a.draws>256 || a.groups>a.draws ||
       !a.bones || a.bones>256 || a.bindings<a.bones || a.bindings>256 || !a.sections || a.sections>a.draws ||
       a.groups_at!=96 || a.draws_at!=96+a.groups*96 || a.sections_at!=a.draws_at+a.draws*8 ||
       a.tails_at!=a.sections_at+a.sections*32 || a.bindings_at!=a.tails_at+a.bones*12 ||
       w(b+76)!=a.bindings_at+a.bindings*4 || w(b+76)>metadata_bytes || (a.block && (a.bones!=1 || a.bindings!=1)))return 0;
    for(unsigned i=0;i<a.bones*3;i++){float v;memcpy(&v,b+a.tails_at+i*4,4);if(!isfinite(v))return 0;}
    for(unsigned i=0;i<a.bindings;i++){
        const unsigned char *p=b+a.bindings_at+i*4;
        if(p[0]>=a.bones || p[1]>=a.bones || p[2]>4 || p[3]>1 ||
           (i<a.bones && (p[0]!=i || p[1]!=i || p[2]!=4 || p[3])) || (i>=a.bones && !p[3]))return 0;
    }
    for(unsigned i=0;i<a.draws;i++){
        const unsigned char *p=b+a.draws_at+i*8;unsigned g=h(p+4);
        if(h(p)>=256 || h(p+2)>=64 || (!streams && h(p+6)) || (g!=65535 && g>=a.groups))return 0;
        if(streams && i && i!=w(b+80) && i!=w(b+80)+w(b+84) &&
            h(p+6)<=h(p-8+6))return 0;
    }
    unsigned gpu_end=0,section_end=0,ordinal_end=0,refs_end=w(b+76);
    for(unsigned i=0;i<a.groups;i++){
        const unsigned char *p=b+a.groups_at+i*96;
        unsigned vertices=w(p+20),indices=w(p+24),at=w(p+28),idx=w(p+32),bones=w(p+36),first=w(p+12),draws=w(p+16);
        unsigned stride=packed?sizeof(FhWorldCompactVertex):packed_actor?sizeof(FhModelCompactVertex):sizeof(FhModelVertex);
        if((a.direct?w(p)!=0:w(p)>=64) || (w(p+4)!=3 && w(p+4)!=5 && w(p+4)!=6 && w(p+4)!=7) || w(p+8)>1 ||
           !vertices || vertices>FH_MODEL_VERTICES || !indices || indices>(a.block?FH_WORLD_INDICES:FH_MODEL_INDICES) || indices%3 ||
           !bones || bones>FH_MODEL_BONES || first<ordinal_end || first>=a.draws || !draws || draws>a.draws-first ||
           w(p+40)!=section_end || draws>a.sections-section_end || w(p+44)!=((a.block && !world_refs)?0:refs_end) ||
           w(p+92)>1 || (w(p+92) && b[7]!=(a.block?'4':'5') && !(a.block && streams) && !packed_actor) ||
           refs_end>metadata_bytes || ((!a.block || world_refs) && vertices*(world_refs?2:12)>metadata_bytes-refs_end) ||
           at!=align16(gpu_end) || at>geometry_bytes || vertices*stride>geometry_bytes-at ||
           idx!=at+vertices*stride || indices*2>geometry_bytes-idx)return 0;
        for(unsigned j=gpu_end;j<at;j++)if(a.gpu[j])return 0;
        gpu_end=idx+indices*2;ordinal_end=first+draws;
        if(!a.block || world_refs)refs_end+=vertices*(world_refs?2:12);
        FhSceneGroup g;fh_native_scene_group(&a,i,&g);
        for(unsigned j=0;j<22;j++){
            if(j>=bones){if(g.bone_ids[j])return 0;continue;}
            if(g.bone_ids[j]>=a.bones)return 0;
            for(unsigned k=0;k<j;k++)if(g.bone_ids[j]==g.bone_ids[k])return 0;
        }
        for(unsigned j=0;j<vertices;j++){
            if(packed){
                if(world_refs && h(b+w(p+44)+j*2)>=w(b+92))return 0;
                const FhWorldCompactVertex *v=(const FhWorldCompactVertex*)g.vertex+j;
                if(v->position[3]!=1 || (!g.two_uv && (v->second_uv[0] || v->second_uv[1])))return 0;
                for(unsigned k=0;k<4;k++)if(v->color[k]>15)return 0;
                continue;
            }
            if(!a.block){
                const FhModelRef *r=g.refs+j;
                if(r->position==65535 || (!a.direct && r->uv==65535) || r->joint>=a.bindings ||
                   (g.two_uv?r->material==65535:r->material>=64))return 0;
                const unsigned char *bind=b+a.bindings_at+r->joint*4;unsigned has[2]={0,0};
                for(unsigned k=0;k<bones;k++){has[0]|=g.bone_ids[k]==bind[0];has[1]|=g.bone_ids[k]==bind[1];}
                if(!stream_actor && ((bind[2] && !has[0]) || (bind[2]<4 && !has[1])))return 0;
            }
            if(packed_actor){
                const FhModelCompactVertex *v=(const FhModelCompactVertex*)((const unsigned char*)g.vertex+j*stride);
                const unsigned char *bind=b+a.bindings_at+g.refs[j].joint*4;
                if(v->position[3]!=1 || v->normal[3] || (!stream_actor && (v->skin[3] || v->skin[2]!=bind[2])) ||
                   (!g.two_uv && (v->uv[2]!=v->uv[0] || v->uv[3]!=v->uv[1])))return 0;
                if(stream_actor){
                    if((unsigned)v->skin[2]+v->skin[3]!=128)return 0;
                }
                for(unsigned k=0;k<4;k++)if(v->color[k]>15)return 0;
                for(unsigned k=0;k<2;k++){
                    unsigned slot=v->skin[k];
                    if(slot>=bones*3 || slot%3)return 0;
                    if(!stream_actor && (k?bind[2]<4:bind[2]>0) && g.bone_ids[slot/3]!=bind[k])return 0;
                }
                for(unsigned k=0;k<3;k++){
                    float tail=0;
                    if(!bind[3])memcpy(&tail,b+a.tails_at+bind[0]*12+k*4,4);
                    if(!isfinite(v->tail[k]) || v->tail[k]!=tail)return 0;
                }
                continue;
            }
            const FhModelVertex *v=g.vertex+j;const float *f=(const float*)v;
            for(unsigned k=0;k<16;k++)if(!isfinite(f[k]))return 0;
            if(a.block && g.two_uv && (fabsf(v->normal[0])>128 || fabsf(v->normal[1])>128 || v->normal[2]!=0))return 0;
            if(v->position[3]!=1 || (a.direct?(fabsf(v->uv[0])>128 || fabsf(v->uv[1])>128):
               (v->uv[0]<0 || v->uv[0]>1 || v->uv[1]<0 || v->uv[1]>1)) ||
               v->skin[2]<0 || v->skin[2]>1 || v->skin[2]*4!=(unsigned)(v->skin[2]*4))return 0;
            for(unsigned k=0;k<2;k++)if(v->skin[k]<0 || v->skin[k]>=bones*3 || v->skin[k]!=(unsigned)v->skin[k] || (unsigned)v->skin[k]%3)return 0;
            for(unsigned k=0;k<4;k++)if(v->color[k]<0 || v->color[k]>1)return 0;
        }
        for(unsigned j=0;j<indices;j++)if(g.index[j]>=vertices)return 0;
        unsigned index_end=0;
        for(unsigned j=0;j<draws;j++){
            FhSceneSection s;fh_native_scene_section(&a,section_end+j,&s);
            const unsigned char *d=b+a.draws_at+(first+j)*8;
            if(s.list!=h(d) || s.shader!=h(d+2) || h(d+4)!=i || s.first!=index_end ||
               !s.count || s.count%3 || s.count>indices-index_end)return 0;
            unsigned width=s.texture_extent&65535,height=s.texture_extent>>16;
            if(a.direct){
                FhSceneSection first;fh_native_scene_section(&a,section_end,&first);
                /* Version9 permits multiple original materials in one world storage
                 * group. Each section retains its own draw binding and flags;
                 * material execution remains per-section, never per-group. */
                if(s.texture_crc || s.texture_extent || s.texture_wrap ||
                    (!mixed_world && (s.shader!=first.shader || s.shader_flags!=first.shader_flags)))return 0;
            }else if(width<8 || width>1024 || height<8 || height>1024 || (width&(width-1)) || (height&(height-1)) ||
               (s.texture_wrap&255)>2 || (s.texture_wrap>>8)>2)return 0;
            if(!a.block && !g.two_uv)for(unsigned k=s.first;k<s.first+s.count;k++)if(g.refs[g.index[k]].material!=s.shader)return 0;
            index_end+=s.count;
        }
        if(index_end!=indices)return 0;
        section_end+=draws;
    }
    if(gpu_end!=geometry_bytes || section_end!=a.sections || metadata_bytes!=((refs_end+127)&~127u))return 0;
    for(unsigned i=refs_end;i<metadata_bytes;i++)if(b[i])return 0;
    for(unsigned i=0;i<a.draws;i++){
        unsigned g=h(b+a.draws_at+i*8+4);
        if(g!=65535){FhSceneGroup group;fh_native_scene_group(&a,g,&group);if(i<group.first || i>=group.first+group.draws)return 0;}
    }
    *out=a;return 1;
}

/* Caller owns the GPU completion/publication boundary. Version8 stores the
 * exact original position index for every compiled world vertex, including
 * duplicated UV seams. Never infer identity from equal initial coordinates. */
int fh_native_scene_refresh_world_positions(const FhSceneAsset *a,const void *positions,unsigned count){
    if(!a || !a->block || !a->compact || !positions || (a->meta[7]!='8' && a->meta[7]!='9') || count!=w(a->meta+92))return 0;
    const unsigned char *source=positions;
    for(unsigned i=0;i<a->groups;i++){
        FhSceneGroup g;fh_native_scene_group(a,i,&g);
        const unsigned char *refs=a->meta+w(a->meta+a->groups_at+i*96+44);
        FhWorldCompactVertex *v=(FhWorldCompactVertex*)g.vertex;
        for(unsigned j=0;j<g.vertices;j++)memcpy(v[j].position,source+h(refs+j*2)*6,6);
    }
    return 1;
}

/* Bounded bit reader: unlike the game reader, never fetch padding bytes. */
static int plan_bits(const unsigned char *p,unsigned bytes,unsigned *bit,unsigned n,unsigned *out){
    if(*bit>bytes*8u || n>bytes*8u-*bit)return 0;
    unsigned value=0;
    for(unsigned i=0;i<n;i++)value|=((p[(*bit+i)>>3]>>((*bit+i)&7))&1u)<<i;
    *bit+=n;*out=value;return 1;
}
unsigned fh_world_plan_compile(const FhSceneAsset *a,const void *instructions,unsigned bytes,
    FhWorldPlanOp *commands,unsigned capacity,FhSceneGroup *draws,unsigned draw_capacity){
    if(!a || !a->block || !a->direct || !instructions || !bytes || bytes>8191 ||
       (a->meta[7]!='6' && a->meta[7]!='7' && a->meta[7]!='8' && a->meta[7]!='9'))return 0;
    const unsigned char *data=instructions;
    unsigned bit=0,count=0,ordinal=0,shader=255,expected=w(a->meta+80);
    for(;;){
        unsigned kind,value=0;
        if(!plan_bits(data,bytes,&bit,4,&kind))return 0;
        if(kind==5)return ordinal==expected && count?count:0;
        if(kind==4){
            if(!plan_bits(data,bytes,&bit,4,&value) || value*8u>bytes*8u-bit)return 0;
            bit+=value*8u;continue;
        }
        if(kind<1 || kind>3 || count>=1024 || (commands && count>=capacity))return 0;
        unsigned payload=bit;
        if(!plan_bits(data,bytes,&bit,kind==1?6:kind==2?8:3,&value))return 0;
        if(kind==1)shader=value;
        FhWorldPlanOp op={(uint16_t)payload,(uint16_t)ordinal,(uint8_t)kind,(uint8_t)value,(uint8_t)shader,0};
        if(kind==2){
            unsigned group;FhSceneGroup g;FhSceneSection section;
            if(ordinal>=expected || !fh_native_scene_draw(a,ordinal,value,shader,&group) ||
               !fh_native_scene_group(a,group,&g) || ordinal<g.first || ordinal-g.first>=g.draws ||
               !fh_native_scene_section(a,g.first_section+ordinal-g.first,&section) ||
               section.list!=value || section.shader!=shader || section.first>g.indices ||
               section.count>g.indices-section.first || h(a->meta+a->draws_at+ordinal*8+6)!=payload)return 0;
            g.index+=section.first;g.indices=section.count;g.draws=1;
            if(draws){if(ordinal>=draw_capacity)return 0;draws[ordinal]=g;}
            ordinal++;
        }
        if(commands)commands[count]=op;
        count++;
    }
}
