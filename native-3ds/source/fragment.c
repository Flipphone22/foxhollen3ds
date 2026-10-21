#include "fh_fragment.h"
#include "fh_pica_texture.h"
#include <math.h>
#include <string.h>
#include "../vendor/melee/texture_decode.h"

static unsigned be16(const unsigned char *p) { return p[0]*256+p[1]; }
/* Coordinates are bounded below before conversion; texel components are
 * nonnegative bytes. Keep floor and half-away rounding exact without calling
 * libm for every component on ARM11. Adding .5f first would misround values
 * immediately below a half-integer, so compare the fractional part instead. */
static int sample_floor(float x) { int whole=(int)x;return whole-(x<(float)whole); }
static float sample_round(float x) { unsigned whole=(unsigned)x;return (float)(whole+(x-(float)whole>=.5f)); }
/* Equivalent to fmaxf(lo,fminf(hi,x)) for these ordered finite bounds,
 * including a NaN input and the positive-zero lower endpoint. */
static float sample_clamp(float x,float lo,float hi) { return !(x<hi)?hi:!(x>lo)?lo:x; }
static float indirect_scale(unsigned shift) { return shift<31?(float)(1u<<shift):ldexpf(1,shift); }
uint32_t fh_texture_pixel(const FhTexture *t,unsigned x,unsigned y) {
    if(t->format==FH_TEXTURE_PICA_ETC1)return fh_pica_etc1_pixel(t->pixels,fh_texture_extent(t->width),x,y);
    const unsigned char *p=t->pixels;unsigned xs=2,ys=2,bs=5;
    if(t->format==0x46) {p+=(y*t->width+x)*4;return mp_rgba(p[0],p[1],p[2],p[3]);}
    if(t->format==0 || t->format==8) xs=ys=3;
    else if(t->format==1 || t->format==2 || t->format==9) xs=3;
    else if(t->format==6) bs=6;
    else if(t->format==14) {
        p+=((y/8)*((t->width+7)/8)+x/8)*32+((y%8)/4*2+(x%8)/4)*8;
        uint32_t palette[4];mp_cmpr_palette(p,palette);return palette[(p[4+y%4]>>(6-2*(x%4)))&3];
    }
    unsigned bw=1u<<xs,bh=1u<<ys;
    p+=(((y>>ys)*((t->width+bw-1)>>xs)+(x>>xs))<<bs);unsigned k=((y&(bh-1))<<xs)+(x&(bw-1)),v=0;
    switch(t->format) {
    case 0:v=((p[k/2]>>((k&1)?0:4))&15)*17;return mp_rgba(v,v,v,v);
    case 1:v=p[k];return mp_rgba(v,v,v,v);
    case 2:v=(p[k]&15)*17;return mp_rgba(v,v,v,(p[k]>>4)*17);
    case 3:return mp_rgba(p[2*k+1],p[2*k+1],p[2*k+1],p[2*k]);
    case 4:return mp_rgb565(be16(p+2*k));
    case 5:return mp_rgb5a3(be16(p+2*k));
    case 6:return mp_rgba(p[2*k+1],p[32+2*k],p[33+2*k],p[2*k]);
    case 8:v=(p[k/2]>>((k&1)?0:4))&15;break;
    case 9:v=p[k];break;
    case 10:v=be16(p+2*k)&0x3fff;break;
    default:return 0;
    }
    if(v>=t->palette_count) return 0xffffffff;
    p=(const unsigned char*)t->palette+2*v;v=be16(p);
    return t->palette_format==1?mp_rgb565(v):t->palette_format==2?mp_rgb5a3(v):mp_rgba(p[1],p[1],p[1],p[0]);
}
static unsigned wrap(int x,unsigned size,unsigned mode) {
    if(!mode) return x<0?0:(unsigned)x>=size?size-1:(unsigned)x;
    int period=mode==2?2*(int)size:(int)size;
    if(!(period&(period-1)))x=(unsigned)x&(period-1);
    else {x%=period;if(x<0)x+=period;}
    return mode==2 && (unsigned)x>=size?2*size-1-x:(unsigned)x;
}
void fh_texture_sample(float out[4],const FhTexture *t,float s,float v) {
    float x=s*t->width,y=v*t->height;
    /* Bound enormous coordinates before float-to-int conversion. */
    if(t->wrap_s) {if(fabsf(x)>1048576)x=fmodf(x,t->width*(t->wrap_s==2?2.f:1.f));}else x=sample_clamp(x,-1,t->width+1);
    if(t->wrap_t) {if(fabsf(y)>1048576)y=fmodf(y,t->height*(t->wrap_t==2?2.f:1.f));}else y=sample_clamp(y,-1,t->height+1);
    if(!t->mag_filter) {
        uint32_t p=fh_texture_pixel(t,wrap(sample_floor(x),t->width,t->wrap_s),wrap(sample_floor(y),t->height,t->wrap_t));
        for(unsigned c=0;c<4;c++) out[c]=((p>>(24-8*c))&255)/255.f;
    } else {
        x-=.5f;y-=.5f;int ix=sample_floor(x),iy=sample_floor(y);float fx=x-ix,fy=y-iy;
        const unsigned char *previous_block=NULL;uint32_t palette[4];
        /* Each axis has only two bilinear coordinates. Resolve edge/repeat/
         * mirror wrapping once per axis, retaining the original tap order. */
        unsigned sx_pair[2]={wrap(ix,t->width,t->wrap_s),wrap(ix+1,t->width,t->wrap_s)};
        unsigned sy_pair[2]={wrap(iy,t->height,t->wrap_t),wrap(iy+1,t->height,t->wrap_t)};
        memset(out,0,16);
        for(unsigned dy=0;dy<2;dy++) for(unsigned dx=0;dx<2;dx++) {
            unsigned sx=sx_pair[dx],sy=sy_pair[dy];
            uint32_t p;
            if(t->format==14) {
                /* Four bilinear taps frequently share one CMPR sub-block.
                 * Reuse only within this sample, so mutable textures cannot
                 * retain stale palette data across calls or draws. */
                const unsigned char *block=(const unsigned char*)t->pixels
                    +((sy/8)*((t->width+7)/8)+sx/8)*32+((sy%8)/4*2+(sx%8)/4)*8;
                if(block!=previous_block){mp_cmpr_palette(block,palette);previous_block=block;}
                p=palette[(block[4+sy%4]>>(6-2*(sx%4)))&3];
            } else p=fh_texture_pixel(t,sx,sy);
            float weight=(dx?fx:1-fx)*(dy?fy:1-fy)/255.f;
            for(unsigned c=0;c<4;c++) out[c]+=((p>>(24-8*c))&255)*weight;
        }
    }
}
int fh_fragment_supported(const FhDrawState *s) {
    if(!s->stages || s->stages>16 || s->indirect_count>4) return 0;
    for(unsigned i=0;i<s->stages;i++) {
        const FhTevStage *t=&s->tev[i];
        unsigned matrix=t->indirect[3];
        if((t->color_op>1 && (t->color_op<8 || t->color_op>15)) || (t->alpha_op>1 && (t->alpha_op<8 || t->alpha_op>15)) || t->color_bias>2 || t->alpha_bias>2 || t->color_scale>3 || t->alpha_scale>3 || t->indirect[1]>3 || t->indirect[0]>3 || matrix>11 || (matrix && !(matrix&3)) || t->indirect[4]>6 || t->indirect[5]>6 || t->indirect[8]>3) return 0;
        if(t->map<8 && (!s->textures[t->map].pixels || !fh_texture_source_bytes(s->textures[t->map].format,s->textures[t->map].width,s->textures[t->map].height))) return 0;
    }
    for(unsigned i=0;i<s->indirect_count;i++) {
        unsigned map=s->indirect_order[i][1];if(map>=8 || s->indirect_order[i][0]>=8 || !s->textures[map].pixels || !fh_texture_source_bytes(s->textures[map].format,s->textures[map].width,s->textures[map].height)) return 0;
    }
    return 1;
}
int fh_texture_pack_fur(uint32_t *out,unsigned width,unsigned height,const FhTexture *base,const FhTexture *mask) {
    if(!out || !base->pixels || !mask->pixels || width<base->width || height<base->height || width%8 || height%8)return 0;
    /* Equal power-of-two dimensions make (texel+.5)/size an exact center.
     * Nearest and bilinear sampling both return that texel; rounding an
     * 8-bit alpha through normalized float recovers the same byte. */
    unsigned direct_mask=base->width && base->height && base->width==mask->width && base->height==mask->height &&
        base->width<=1024 && base->height<=1024 && !(base->width&(base->width-1)) && !(base->height&(base->height-1));
    /* Pinned Melee mp_cmpr_to_native: decode each 4x4 block's endpoints once,
     * retaining GameCube interpolation and repeated-edge padding. The SFA
     * mask is still sampled per texel before writing the native tile. */
    for(unsigned by=0;by<height;by+=4)for(unsigned bx=0;bx<width;bx+=4) {
      const unsigned char *block=NULL;uint32_t palette[4];
      if(base->format==14) {
        unsigned sx=bx<base->width?bx:base->width-1,sy=by<base->height?by:base->height-1;
        block=(const unsigned char*)base->pixels+((sy/8)*((base->width+7)/8)+sx/8)*32+((sy%8)/4*2+(sx%8)/4)*8;
        mp_cmpr_palette(block,palette);
      }
      for(unsigned y=by;y<by+4;y++)for(unsigned x=bx;x<bx+4;x++) {
        unsigned sx=x<base->width?x:base->width-1,sy=y<base->height?y:base->height-1;
        uint32_t p=block?palette[(block[4+sy%4]>>(6-2*(sx%4)))&3]:fh_texture_pixel(base,sx,sy);
        unsigned bump;
        if(direct_mask)bump=fh_texture_pixel(mask,sx,sy)&248;
        else {
            float sample[4];fh_texture_sample(sample,mask,(sx+.5f)/base->width,(sy+.5f)/base->height);
            bump=(unsigned)sample_round(sample[3]*255)&248;
        }
        unsigned alpha=(bump*255+124)/248;
        unsigned morton=(x&1)|((y&1)<<1)|((x&2)<<1)|((y&2)<<2)|((x&4)<<2)|((y&4)<<3);
        out[((y/8)*(width/8)+x/8)*64+morton]=(p&0xffffff00)|alpha;
      }
    }
    return 1;
}
static float konst(const FhDrawState *s,unsigned k,unsigned c) {
    return k<8?(8-k)/8.f:k>=12 && k<16?s->konst_colors[k-12][c]:k>=16 && k<32?s->konst_colors[k&3][(k-16)/4]:0;
}
void fh_fur_coordinate(float out[3],const FhDrawState *s,const FhVertex *v,const float matrix[2][2][3]) {
    float ind[2][3];fh_fur_samples(ind,s,v);fh_fur_coordinate_sampled(out,s,v,matrix,ind);
}
void fh_fur_samples(float ind[2][3],const FhDrawState *s,const FhVertex *v) {
    for(unsigned i=0;i<2;i++) {
        unsigned coord=s->indirect_order[i][0],map=s->indirect_order[i][1];float tex[4],q=v->uv[coord][2];if(fabsf(q)<1e-20f)q=1;
        fh_texture_sample(tex,&s->textures[map],v->uv[coord][0]/q/indirect_scale(s->indirect_scale[i][0]),v->uv[coord][1]/q/indirect_scale(s->indirect_scale[i][1]));
        ind[i][0]=sample_round(tex[3]*255);ind[i][1]=sample_round(tex[2]*255);ind[i][2]=sample_round(tex[1]*255);
    }
}
void fh_fur_offset(float offset[2],const float matrix[2][2][3],const float ind[2][3]) {
    offset[0]=offset[1]=0;
    for(unsigned i=0;i<2;i++)for(unsigned axis=0;axis<2;axis++)for(unsigned c=0;c<3;c++)offset[axis]+=(ind[i][c]-128)*matrix[i][axis][c];
}
void fh_fur_coordinate_sampled(float out[3],const FhDrawState *s,const FhVertex *v,const float matrix[2][2][3],const float ind[2][3]) {
    float offset[2],bump=((unsigned)ind[1][0]&248)/248.f;fh_fur_offset(offset,matrix,ind);
    float q=v->uv[4][2];if(fabsf(q)<1e-20f)q=1;
    out[0]=v->uv[4][0]/q+offset[0]/s->textures[3].width;out[1]=v->uv[4][1]/q+offset[1]/s->textures[3].height;out[2]=bump;
}
void fh_fur_noise(float out[4],const FhDrawState *s,const FhVertex *v,const float matrix[2][2][3]) {
    float coordinate[3],tex[4];fh_fur_coordinate(coordinate,s,v,matrix);float bump=coordinate[2];
    fh_texture_sample(tex,&s->textures[3],coordinate[0],coordinate[1]);
    out[0]=sample_round(tex[0]*255)/510.f;out[1]=out[2]=0;
    out[3]=sample_clamp(sample_round(tex[3]*255)/255.f*(s->tev[3].channel==255?1:bump)+s->tev_colors[3][3],0,1);
}
static float argument(unsigned arg,unsigned c,float reg[4][4],const float tex[4],const float raster[4],float k) {
    if(c==3) return arg<4?reg[arg][3]:arg==4?tex[3]:arg==5?raster[3]:arg==6?k:0;
    return arg<8?reg[arg/2][arg&1?3:c]:arg==8?tex[c]:arg==9?tex[3]:arg==10?raster[c]:arg==11?raster[3]:arg==12?1:arg==13?.5f:arg==14?k:0;
}
/* Resolve input components once per immutable raster job. Preserve every
 * arithmetic operation and intermediate clamp in the scalar evaluator. */
static unsigned argument_slot(unsigned arg,unsigned c) {
    if(c==3) return arg<4?arg*4+3:arg==4?19:arg==5?23:arg==6?27:30;
    return arg<8?(arg/2)*4+(arg&1?3:c):arg==8?16+c:arg==9?19:
        arg==10?20+c:arg==11?23:arg==12?28:arg==13?29:arg==14?24+c:30;
}
void fh_fragment_prepare(FhFragmentProgram *p,const FhDrawState *s) {
    memset(p,0,sizeof(*p));
    for(unsigned i=0;i<8;i++)p->scale[i][0]=p->scale[i][1]=1;
    for(unsigned i=0;i<3;i++)p->exponent[i]=ldexpf(1,s->indirect_exponent[i]);
    for(unsigned i=0;i<s->indirect_count;i++) {
        unsigned c=s->indirect_order[i][0],m=s->indirect_order[i][1];
        p->uv_mask|=1u<<c;p->scale[c][0]=s->textures[m].width;p->scale[c][1]=s->textures[m].height;
        for(unsigned j=0;j<2;j++)p->indirect_divisor[i][j]=indirect_scale(s->indirect_scale[i][j]);
    }
    /* GX_TEX_DISABLE suppresses sampling, not automatic SU scaling. */
    for(unsigned i=0;i<s->stages;i++) {
        const FhTevStage *t=&s->tev[i];unsigned c=t->coord,m=t->map&~256u;
        if(c<8){p->uv_mask|=1u<<c;if(m<8){p->scale[c][0]=s->textures[m].width;p->scale[c][1]=s->textures[m].height;}}
        /* An indirect stage can still carry previous coordinates even when
         * its matrix is zero. Omit only texture fetches with no offset/bump
         * consumer; the coordinate chain below remains unchanged. */
        unsigned selection=t->indirect[3],indirect_used=selection>=5;
        if(selection && selection<4){
            unsigned matrix=selection-1;
            for(unsigned a=0;a<2;a++)for(unsigned b=0;b<3;b++)
                indirect_used|=s->indirect_matrix[matrix][a][b]!=0;
        }
        indirect_used|=(t->channel==7 || t->channel==8) && t->indirect[8];
        if(indirect_used && t->indirect[0]<s->indirect_count)p->indirect_mask|=1u<<t->indirect[0];
        for(unsigned j=0;j<4;j++)p->konst[i][j]=konst(s,j==3?t->kalpha:t->kcolor,j);
        for(unsigned j=0;j<4;j++)for(unsigned k=0;k<4;k++) {
            unsigned slot=argument_slot((j==3?t->alpha:t->color)[k],j);
            p->arguments[i][j][k]=slot;
            if(slot>=16 && slot<20)p->sample_mask|=1u<<i;
        }
    }
}
int fh_fragment_evaluate(float out[4],const FhDrawState *s,const FhVertex *v) {
    FhFragmentProgram p;fh_fragment_prepare(&p,s);return fh_fragment_evaluate_prepared(out,s,v,&p);
}
/* Compare inputs use their low eight bits. Keep signed TEV-register wrap;
 * fmod bounds conversion even for an unclamped preceding float expression. */
static unsigned compare_byte(float value) {
    float scaled=value*255;
    if(fabsf(scaled)<8388608.f) {
        int whole=(int)scaled;float fraction=scaled-whole;
        return (unsigned)(whole+(fraction>=.5f)-(fraction<=-.5f))&255;
    }
    return (unsigned)(int)fmodf(roundf(scaled),256)&255;
}
int fh_fragment_evaluate_prepared(float out[4],const FhDrawState *s,const FhVertex *v,const FhFragmentProgram *p) {
    float values[31],uv[8][2]={{0}},ind[4][3]={{0}},prior[2]={0};
    float (*reg)[4]=(float(*)[4])values;
    memcpy(reg,s->tev_colors,16*sizeof(float));values[28]=1;values[29]=.5f;values[30]=0;
    const float (*scale)[2]=p->scale;
    for(unsigned i=0;i<8;i++)if(p->uv_mask&(1u<<i)){float q=v->uv[i][2];if(fabsf(q)<1e-20f)q=1;uv[i][0]=v->uv[i][0]/q;uv[i][1]=v->uv[i][1]/q;}
    for(unsigned i=0;i<s->indirect_count;i++)if(p->indirect_mask&(1u<<i)) {
        unsigned c=s->indirect_order[i][0],m=s->indirect_order[i][1];float sample[4];
        fh_texture_sample(sample,&s->textures[m],uv[c][0]*scale[c][0]/s->textures[m].width/p->indirect_divisor[i][0],uv[c][1]*scale[c][1]/s->textures[m].height/p->indirect_divisor[i][1]);
        ind[i][0]=sample_round(sample[3]*255);ind[i][1]=sample_round(sample[2]*255);ind[i][2]=sample_round(sample[1]*255);
    }
    for(unsigned i=0;i<s->stages;i++) {
        const FhTevStage *t=&s->tev[i];float *tex=values+16,*raster=values+20,result[4],coord[2]={0};
        for(unsigned j=0;j<4;j++){tex[j]=1;raster[j]=0;values[24+j]=p->konst[i][j];}
        unsigned c=t->coord,m=t->map&~256u;int base=c<8,sample=base && m<8 && !(t->map&256);
        if(base) {coord[0]=uv[c][0];coord[1]=uv[c][1];}
        int indirect=t->indirect[3] || t->indirect[4] || t->indirect[5] || t->indirect[6];
        if(indirect) {
            if(base) {coord[0]*=scale[c][0];coord[1]*=scale[c][1];}
            for(unsigned j=0;j<2;j++) {unsigned wrap=t->indirect[4+j];if(wrap)coord[j]=wrap==6?0:fmodf(coord[j],(float)(512u>>wrap));}
            if(t->indirect[3] && t->indirect[0]<s->indirect_count) {
                float delta[3];unsigned format=t->indirect[1],selection=t->indirect[3],matrix=(selection-1)&3;
                for(unsigned j=0;j<3;j++) delta[j]=sample_floor(ind[t->indirect[0]][j]/(format?1u<<(format+2):1))+(t->indirect[2]&(1u<<j)?format?1:-128:0);
                float exponent=p->exponent[matrix];
                if(selection<4) {
                    for(unsigned j=0;j<2;j++) for(unsigned k=0;k<3;k++) coord[j]+=delta[k]*s->indirect_matrix[matrix][j][k]*exponent;
                } else if(base) {
                    /* Dynamic S/T uses the unwrapped base vector and one
                     * biased indirect component; matrix coefficients are not
                     * used. Aurora's shader and Dolphin's Tev::Indirect are
                     * the arithmetic references; sampling enable is separate. */
                    float component=delta[selection<8?0:1];
                    for(unsigned j=0;j<2;j++)coord[j]+=uv[c][j]*scale[c][j]*component*exponent/256.f;
                }
            }
            for(unsigned j=0;j<2;j++) {if(t->indirect[6])coord[j]+=prior[j];prior[j]=coord[j];}
            if(sample) {coord[0]/=s->textures[m].width;coord[1]/=s->textures[m].height;}
        } else if(base) {prior[0]=coord[0]*scale[c][0];prior[1]=coord[1]*scale[c][1];}
        /* A configured map can have no TEV texture consumers. Coordinate
         * chaining above still runs, including indirect prior-stage offsets.
         * RGB arguments also cover packed alpha-compare dependencies. */
        if(sample && (p->sample_mask&(1u<<i))) {float sampled[4];fh_texture_sample(sampled,&s->textures[m],coord[0],coord[1]);for(unsigned j=0;j<4;j++)tex[j]=sample_round(sampled[s->swaps[t->tex_swap&3][j]&3]*255)/255.f;}
        if(t->channel<6) for(unsigned j=0;j<4;j++) raster[j]=v->color[t->channel&1][s->swaps[t->ras_swap&3][j]&3];
        else if((t->channel==7 || t->channel==8) && t->indirect[8] && t->indirect[0]<s->indirect_count) {
            unsigned mask=(unsigned[]){248,224,240,248}[t->indirect[1]];
            float alpha=((unsigned)ind[t->indirect[0]][t->indirect[8]-1]&mask)/(t->channel==8?248.f:255.f);
            for(unsigned j=0;j<4;j++)raster[j]=alpha;
        }
        for(unsigned j=0;j<4;j++) {
            float a[4];
            for(unsigned k=0;k<4;k++)a[k]=values[p->arguments[i][j][k]];
            unsigned op=j==3?t->alpha_op:t->color_op,bias=j==3?t->alpha_bias:t->color_bias,sc=j==3?t->alpha_scale:t->color_scale,clamp=j==3?t->alpha_clamp:t->color_clamp;
            if(op>=8) {
                unsigned ab[2]={0},kind=(op-8)/2;
                if(kind==3){ab[0]=compare_byte(a[0]);ab[1]=compare_byte(a[1]);}
                else for(unsigned input=0;input<2;input++)for(unsigned component=0;component<=kind;component++)
                    ab[input]|=compare_byte(argument(t->color[input],component,reg,tex,raster,p->konst[i][component]))<<(8*component);
                /* Alpha packed comparisons consume the stage's RGB inputs;
                 * mode 14/15 instead compares alpha. Bias/scale are ignored. */
                int pass=op&1?ab[0]==ab[1]:ab[0]>ab[1];
                result[j]=a[3]+(pass?compare_byte(a[2])/255.f:0);
                if(!clamp)result[j]=sample_clamp(result[j],-1024/255.f,1023/255.f);
            } else result[j]=(a[3]+(op?-1:1)*(a[0]*(1-a[2])+a[1]*a[2])+(bias==1?.5f:bias==2?-.5f:0))*(sc==3?.5f:(float)(1u<<sc));
            if(clamp)result[j]=sample_clamp(result[j],0,1);
        }
        memcpy(reg[t->color_dest&3],result,12);reg[t->alpha_dest&3][3]=result[3];
    }
    const FhTevStage *last=&s->tev[s->stages-1];
    memcpy(out,reg[last->color_dest&3],12);out[3]=reg[last->alpha_dest&3][3];return 1;
}
