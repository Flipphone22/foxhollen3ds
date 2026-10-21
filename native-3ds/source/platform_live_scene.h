#include "fh_install_scene.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
enum {LIVE_META_CAP=64*1024,LIVE_GPU_CAP=128*1024};
typedef struct {
    const FhInstallVertexSource*s;FhInstallSkinWeights*pos,*nrm;FhInstallScene*out;
    unsigned char *gpu,*refs;unsigned gpu_bytes,ref_bytes,groups,draws,streams[3];
    unsigned char group[256][96],draw[256][8],section[256][32];
} LiveBuilder;
static void live_put(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static unsigned live_word(const unsigned char*p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static void live_half(unsigned char*p,unsigned n){p[0]=n;p[1]=n>>8;}
static unsigned live_be(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned live_align(unsigned n,unsigned alignment){return (n+alignment-1)&~(alignment-1);}
static int live_error(LiveBuilder*b,const char*why){snprintf(b->out->error,sizeof(b->out->error),"%s",why);return 0;}
static int live_group(void*context,const FhInstallGroup*g){
    LiveBuilder*b=context;unsigned stride=b->s->world?20:40,ref_stride=b->s->world?2:12;
    unsigned start=live_align(b->gpu_bytes,16),indices=start+g->vertices*stride,end=indices+g->indices*2;
    if(b->groups==256||g->sections>256-b->draws||end>LIVE_GPU_CAP||g->vertices>(LIVE_META_CAP-b->ref_bytes)/ref_stride)return live_error(b,"Native scene exceeds bounded packet storage.");
    /* Only alignment gaps survive into the published packet. */
    memset(b->gpu+b->gpu_bytes,0,start-b->gpu_bytes);
    unsigned char*m=b->group[b->groups];
    unsigned fields[]={0,g->vat,!!(g->flags&8),b->draws,g->sections,g->vertices,g->indices,start,indices,g->bone_count,b->draws,b->ref_bytes};
    for(unsigned i=0;i<12;i++)live_put(m+i*4,fields[i]);
    for(unsigned i=0;i<g->bone_count;i++)live_half(m+48+i*2,g->bones[i]);live_put(m+92,g->two_uv);
    for(unsigned i=0;i<g->vertices;i++){
        unsigned char*dest=b->gpu+start+i*stride;const FhModelRef*r=g->ref+i;
        if(b->s->world){
            FhWorldCompactVertex v;if(!fh_install_compact_world(b->s,r,g->two_uv,&v))return live_error(b,"World compact vertex encoding failed.");
            memcpy(dest,&v,sizeof(v));live_half(b->refs+b->ref_bytes+i*2,r->position);
        }else{
            FhModelCompactVertex v;if(!fh_install_compact_actor(b->s,r,g->two_uv,g->bones,g->bone_count,b->pos,b->nrm,&v))return live_error(b,"Actor compact vertex encoding failed.");
            memcpy(dest,&v,sizeof(v));memcpy(b->refs+b->ref_bytes+i*12,r,12);
        }
    }
    for(unsigned i=0;i<g->indices;i++)live_half(b->gpu+indices+i*2,g->index[i]);
    for(unsigned i=0;i<g->sections;i++){
        const FhInstallSection*s=g->section+i;
        if(s->ordinal!=b->draws||s->list>=256||s->shader>=64||s->stream>2||s->bit>65535)return live_error(b,"Native scene source mapping cannot be represented.");
        unsigned char*d=b->draw[b->draws],*section=b->section[b->draws];
        live_half(d,s->list);live_half(d+2,s->shader);live_half(d+4,b->groups);live_half(d+6,b->s->world?s->bit:0);
        unsigned values[]={s->list,s->shader,s->first,s->count,0,0,0,g->flags};
        for(unsigned j=0;j<8;j++)live_put(section+j*4,values[j]);
        if(b->s->world)b->streams[s->stream]++;b->draws++;
    }
    b->gpu_bytes=end;b->ref_bytes+=g->vertices*ref_stride;b->groups++;return 1;
}
static int fh_live_scene(FhInstallScene*out,const void*raw,unsigned bytes,unsigned id){
    if(!out)return 0;fh_install_scene_close(out);FhInstallVertexSource s;
    if(!fh_install_vertex_source(&s,raw,bytes,id>>31)){snprintf(out->error,sizeof(out->error),"Invalid source geometry arrays.");return 0;}
    unsigned draws=0;if(!fh_install_descriptors(raw,bytes,id>>31,NULL,NULL,&draws)){snprintf(out->error,sizeof(out->error),"Invalid source geometry description.");return 0;}
    if(!draws)return 1;
    unsigned bones=s.rigid?1:s.joints_count+s.fixed_world,bindings=s.rigid?1:bones+s.blends_count;
    if(bones>256||bindings>256){snprintf(out->error,sizeof(out->error),"Native source exceeds256 bone bindings.");return 0;}
    LiveBuilder*b=calloc(1,sizeof(*b));if(!b){snprintf(out->error,sizeof(out->error),"Cannot allocate startup scene owner.");return 0;}
    b->s=&s;b->out=out;b->gpu=malloc(LIVE_GPU_CAP);b->refs=malloc(LIVE_META_CAP);int ok=0;
    if(!b->gpu||!b->refs){live_error(b,"Cannot allocate bounded scene staging buffers.");goto done;}
    if(s.fixed_world){
        b->pos=calloc(s.positions_count?s.positions_count:1,sizeof(*b->pos));b->nrm=calloc(s.normals_count?s.normals_count:1,sizeof(*b->nrm));
        if(!b->pos||!b->nrm||!fh_install_skin_stream(&s,0,b->pos,s.positions_count)||!fh_install_skin_stream(&s,1,b->nrm,s.normals_count)){
            live_error(b,"Cannot prepare original skin stream maps.");goto done;
        }
    }
    if(fh_install_groups(&s,b->pos,b->nrm,live_group,b,&out->groups)!=1){if(!out->error[0])live_error(b,out->groups.error);goto done;}
    unsigned go=96,draw_at=go+b->groups*96,section_at=draw_at+b->draws*8,tails=section_at+b->draws*32;
    unsigned binds=tails+bones*12,refs=binds+bindings*4,split=live_align(refs+b->ref_bytes,128),total=split+b->gpu_bytes;
    if(split>LIVE_META_CAP){live_error(b,"Native scene metadata exceeds live bound.");goto done;}
    out->allocation=malloc(total+15);if(!out->allocation){live_error(b,"Cannot allocate completed native scene.");goto done;}
    unsigned char*p=(unsigned char*)(((uintptr_t)out->allocation+15)&~(uintptr_t)15);memset(p,0,split);
    memcpy(p,s.world?"FHNSCN08":s.fixed_world?"FHNSCN0B":"FHNSCN0A",8);
    unsigned fields[]={total,id,bytes,(unsigned)crc32(0,raw,bytes),s.world,b->groups,b->draws,bones,bindings,go,draw_at,section_at,b->draws,tails,binds,split,b->gpu_bytes,refs,0,0,0,0};
    for(unsigned i=0;i<22;i++)live_put(p+8+i*4,fields[i]);
    if(s.world){for(unsigned i=0;i<3;i++)live_put(p+80+i*4,b->streams[i]);live_put(p+92,s.positions_count);}
    memcpy(p+go,b->group,b->groups*96);memcpy(p+draw_at,b->draw,b->draws*8);memcpy(p+section_at,b->section,b->draws*32);
    for(unsigned i=0;i<b->groups;i++){unsigned char*g=p+go+i*96;live_put(g+44,refs+live_word(g+44));}
    if(!s.rigid)for(unsigned i=0;i<s.joints_count;i++)for(unsigned j=0;j<3;j++)live_put(p+tails+i*12+j*4,live_be(s.joints+i*28+16+j*4));
    for(unsigned i=0;i<bindings;i++){
        FhModelBinding binding;if(!fh_install_vertex_binding(&s,i,&binding)){live_error(b,"Invalid original bone binding.");goto done;}
        memcpy(p+binds+i*4,&binding,4);
    }
    memcpy(p+refs,b->refs,b->ref_bytes);memcpy(p+split,b->gpu,b->gpu_bytes);
    FhSceneAsset check;if(!fh_native_scene_open(&check,p,split,p+split,b->gpu_bytes,id,bytes,fields[3])){live_error(b,"Native runtime reader rejected generated geometry.");goto done;}
    out->data=p;out->bytes=total;out->metadata_bytes=split;ok=1;
done:
    free(b->pos);free(b->nrm);free(b->gpu);free(b->refs);free(b);
    if(!ok){free(out->allocation);out->allocation=NULL;out->data=NULL;out->bytes=out->metadata_bytes=0;}return ok;
}
#pragma GCC diagnostic pop
