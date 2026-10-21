#include "fh_pica_bump.h"
#include <limits.h>
#include <string.h>

int fh_pica_bump_basis(FhPicaBumpBasis *out,unsigned capacity,
    const FhModelRef *refs,unsigned count,const FhModelSource *source){
    if(!source || !refs || !count || count>FH_MODEL_VERTICES ||
       !source->normals || source->normal_stride!=9 || !source->normal_count ||
       source->normal_count>UINT_MAX/9 || (out && capacity<count))return 0;
    /* Validate every reference before writing. A rejected group cannot leave
     * a partially initialized stream that a later draw could accidentally use. */
    for(unsigned i=0;i<count;i++)
        if(refs[i].normal==UINT16_MAX || refs[i].normal>=source->normal_count)return 0;
    if(out)for(unsigned i=0;i<count;i++){
        const unsigned char *basis=source->normals+9u*refs[i].normal;
        memcpy(out[i].binormal,basis+3,3);out[i].binormal[3]=0;
        memcpy(out[i].tangent,basis+6,3);out[i].tangent[3]=0;
    }
    return 1;
}

int fh_pica_bump_parameters(FhPicaBumpParameters *out,unsigned enabled,
    unsigned aux_width,unsigned base_width,unsigned packed){
    if(!(enabled&1))return 0;
    if(!out || !aux_width || aux_width>1024 || base_width>1024 || packed>255)return -1;
    FhPicaBumpParameters next={0};
    unsigned repeat=(packed&15)*4+1;
    if(base_width){
        unsigned div=base_width/(aux_width*repeat);
        /* Source table has nine entries. Reject malformed inputs rather than
         * inventing a clamped result for an out-of-bounds source lookup. */
        if(div>9)return -1;
        if(div){next.writes_indirect_scale=1;next.indirect_scale_log2=div-1;}
        else next.extra_texgen_scale=repeat;
    }else next.extra_texgen_scale=1;
    next.basis_scale=.5f*(3.f*((float)(packed>>4)/7.f-1.f));
    *out=next;return 1;
}

static uint8_t field_byte(float component){
    /* All input lanes have already been bounded to [-1,1]. The explicit
     * clamp absorbs rounding at those endpoints before float-to-int conversion. */
    float value=(component*.5f+.5f)*255.f;
    if(value<=0)return 0;
    if(value>=255)return 255;
    return (uint8_t)(unsigned)(value+.5f);
}
int fh_pica_bump_field(uint8_t *out,size_t capacity,float *decode_scale,
    const uint8_t *rgba,unsigned pixels,unsigned packed){
    if(!out || !decode_scale || !rgba || !pixels || pixels>1024u*1024u ||
       packed>255 || capacity<(size_t)pixels*4)return 0;
    float strength=.5f*(3.f*((float)(packed>>4)/7.f-1.f));
    float amplitude=strength<0?-strength:strength;
    float scale=amplitude>1?amplitude:1;
    float multiplier=(2*strength/256)/scale;
    uint8_t z=field_byte(1/scale);
    for(unsigned i=0;i<pixels;i++){
        const uint8_t *p=rgba+4u*i;
        out[4u*i]=field_byte(multiplier*((int)p[3]-128));
        out[4u*i+1]=field_byte(multiplier*((int)p[2]-128));
        out[4u*i+2]=z;out[4u*i+3]=255;
    }
    *decode_scale=scale;return 1;
}

int fh_pica_bump_texture(uint8_t *out,size_t capacity,float *decode_scale,
    const uint8_t *rgba,unsigned width,unsigned height,unsigned packed){
    if(!out || !decode_scale || !rgba || width<8 || height<8 ||
       width>1024 || height>1024 || (width&(width-1)) || (height&(height-1)) ||
       packed>255 || capacity<(size_t)width*height*3)return 0;
    float strength=.5f*(3.f*((float)(packed>>4)/7.f-1.f));
    float amplitude=strength<0?-strength:strength;
    float scale=amplitude>1?amplitude:1;
    float multiplier=(2*strength/256)/scale;
    uint8_t z=field_byte(1/scale);
    /* PICA RGB8 stores B,G,R in each 8x8 Morton tile. As in the pinned Melee
     * texture repacker, source Y orientation is retained; no implicit V flip. */
    static const unsigned char mx[8]={0,1,4,5,16,17,20,21};
    for(unsigned by=0;by<height;by+=8)for(unsigned bx=0;bx<width;bx+=8){
        uint8_t *tile=out+((by/8)*(width/8)+bx/8)*192;
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
            const uint8_t *p=rgba+((by+y)*width+bx+x)*4;
            uint8_t *d=tile+3*(mx[x]+2*mx[y]);
            d[0]=z;d[1]=field_byte(multiplier*((int)p[2]-128));
            d[2]=field_byte(multiplier*((int)p[3]-128));
        }
    }
    *decode_scale=scale;return 1;
}
