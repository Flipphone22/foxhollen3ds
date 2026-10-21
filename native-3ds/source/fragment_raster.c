#include "fh_fragment_raster.h"
#include <math.h>
static float edge(float ax,float ay,float bx,float by,float x,float y) { return (bx-ax)*(y-ay)-(by-ay)*(x-ax); }
static unsigned byte(float value) { return value<0?0:value>1?255:(unsigned)(value*255+.5f); }
void fh_fragment_raster_rows(FhFragmentRaster *job,unsigned first,unsigned end) {
    const FhVertex *input=job->input;const FhDrawState *state=job->state;
    const float (*screen)[2]=job->screen;float area=job->area;
    unsigned trim=job->width>=8 && job->height>=8;
    float margin[3]={0};
    for(unsigned i=0;trim && i<3;i++) {
        unsigned a=(i+1)%3,b=(i+2)%3;
        /* The atlas uses nearest sampling. Keep a two-pixel exterior band for
         * PICA vertex/subpixel rounding and interpolated atlas coordinates.
         * Only discard texels beyond the expanded triangle, never its edge.
         * No depth/alpha visibility assumption is made here. */
        margin[i]=2*(fabsf(screen[b][0]-screen[a][0])+fabsf(screen[b][1]-screen[a][1]))+1;
    }
    /* Preserve every interpolant read by the scalar evaluator. Unused colors
     * and coordinates have no consumers; their zero-initialized values remain
     * private to this fragment. Keep per-component summation order unchanged. */
    unsigned fields[32],field_count=0,used=0;FhFragmentProgram local_program;
    const FhFragmentProgram *program=job->program;
    if(job->mode!=2) {
        if(!program){fh_fragment_prepare(&local_program,state);program=&local_program;}
        for(unsigned i=0;i<state->stages;i++){
            const FhTevStage *t=&state->tev[i];
            if(t->channel<6)used|=15u<<((t->channel&1)*4);
            /* Disabled texture sampling still consumes coordinates for GX
             * indirect S/T matrices, wrapping and the previous-stage offset. */
            if(t->coord<8)used|=7u<<(8+t->coord*3);
        }
        for(unsigned i=0;i<state->indirect_count;i++){
            unsigned coord=state->indirect_order[i][0];
            if(coord<8)used|=7u<<(8+coord*3);
        }
        for(unsigned c=0;c<32;c++)if(used&(1u<<c))fields[field_count++]=c+4;
    }
    for(unsigned y=first;y<end;y++)for(unsigned x=0;x<job->width;x++) {
        float px=job->left+x+.5f,py=job->top+y+.5f;
        float edges[3]={edge(screen[1][0],screen[1][1],screen[2][0],screen[2][1],px,py),edge(screen[2][0],screen[2][1],screen[0][0],screen[0][1],px,py),0};
        if(trim) {
            edges[2]=edge(screen[0][0],screen[0][1],screen[1][0],screen[1][1],px,py);
            if(area>0?(edges[0]<-margin[0] || edges[1]<-margin[1] || edges[2]<-margin[2]):
                      (edges[0]>margin[0] || edges[1]>margin[1] || edges[2]>margin[2])) {
                job->pixels[y*job->width+x]=0;continue;
            }
        }
        float weights[3]={edges[0]/area,edges[1]/area,0};weights[2]=1-weights[0]-weights[1];
        float total=0;for(unsigned i=0;i<3;i++){weights[i]/=input[i].position[3];total+=weights[i];}
        FhVertex fragment={0};float result[4]={0};
        if(fabsf(total)>1e-20f) {
            for(unsigned i=0;i<3;i++)weights[i]/=total;
            if(job->mode==2) {
                for(unsigned i=0;i<3;i++)for(unsigned t=0;t<3;t++)for(unsigned c=0;c<3;c++) {
                    unsigned coord=(unsigned[]){1,3,4}[t];fragment.uv[coord][c]+=input[i].uv[coord][c]*weights[i];
                }
                fh_fur_noise(result,state,&fragment,job->matrix);
            } else {
                for(unsigned i=0;i<3;i++)for(unsigned j=0;j<field_count;j++){
                    unsigned c=fields[j];((float*)&fragment)[c]+=((const float*)&input[i])[c]*weights[i];
                }
                fh_fragment_evaluate_prepared(result,state,&fragment,program);
            }
            job->evaluated++;
        }
        job->pixels[y*job->width+x]=(byte(result[0])<<24)|(byte(result[1])<<16)|(byte(result[2])<<8)|byte(result[3]);
    }
}
