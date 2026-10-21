#include "fh_material.h"
#include "fh_fragment.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
/* GXSetTevOrder's c2r register table maps all COLOR/ALPHA/COLORA aliases
 * onto the same complete raster channel. TEV swaps precede RASC/RASA. */
static void evaluate(float out[4],const FhPipeline *p,const float color[2][4]) {
    assert(p->count==1 && p->textures==0);
    const FhCombiner *s=&p->stages[0];float primary[4];
    for(unsigned c=0;c<4;c++)primary[c]=p->primary[c]<0?0:((const float*)color)[p->primary[c]];
    for(unsigned c=0;c<4;c++) {
        const FhCombinerChannel *ch=&s->channel[c==3];
        assert(ch->operation==FH_REPLACE && ch->scale==1);
        const float *source=ch->source[0]==FH_PRIMARY?primary:s->constant;
        assert(ch->source[0]==FH_PRIMARY || ch->source[0]==FH_CONSTANT);
        unsigned operand=ch->operand[0];assert(!(operand&8));
        out[c]=source[operand==0?c:operand==1?3:operand-2];
    }
}
int main(void) {
    unsigned checks=0;
    for(unsigned channel=0;channel<6;channel++)for(unsigned swap=0;swap<256;swap++)
    for(unsigned alpha=0;alpha<2;alpha++)for(unsigned known=0;known<2;known++) {
        FhDrawState s={0};FhVertex v={0};s.stages=1;
        for(unsigned c=0;c<4;c++)s.swaps[0][c]=(swap>>(c*2))&3;
        for(unsigned ch=0;ch<2;ch++)for(unsigned c=0;c<4;c++)v.color[ch][c]=(1+ch*4+c)/16.f;
        s.tev[0]=(FhTevStage){.color={15,15,15,alpha?11:10},.alpha={7,7,7,5},
            .channel=channel,.coord=255,.map=255,.color_clamp=1,.alpha_clamp=1};
        s.constant_raster_mask=known?255:0;memcpy(s.constant_raster,v.color,sizeof(v.color));
        /* Fixed c2r table is independent of all three compiler/evaluator paths. */
        const unsigned c2r[]={0,1,0,1,0,1};float expected[4],actual[4];
        for(unsigned c=0;c<4;c++)expected[c]=v.color[c2r[channel]][s.swaps[0][c==3 || alpha?3:c]];
        assert(fh_fragment_evaluate(actual,&s,&v));
        for(unsigned c=0;c<4;c++){assert(actual[c]==expected[c]);checks++;}
        FhMaterial material;assert(fh_material_compile(&material,&s));
        fh_material_color(actual,&material,v.color);
        for(unsigned c=0;c<4;c++){assert(actual[c]==expected[c]);checks++;}
        assert(fh_pipeline_compile(&material,&s));
        evaluate(actual,&material.pipeline,v.color);
        for(unsigned c=0;c<4;c++){assert(actual[c]==expected[c]);checks++;}
    }
    for(unsigned ch=0;ch<6;ch++)for(unsigned alpha=0;alpha<2;alpha++) {
        FhDrawState s={0};FhMaterial p;s.stages=2;
        for(unsigned c=0;c<4;c++)s.swaps[0][c]=c;
        s.tev[0]=(FhTevStage){.color={15,alpha?11:10,12,alpha?11:10},.alpha={7,5,6,5},.channel=ch,.map=255,.coord=255,.color_clamp=1,.alpha_clamp=1};
        s.tev[1]=(FhTevStage){.color={15,0,13,15},.alpha={7,0,6,7},.channel=255,.map=255,.coord=255,.color_clamp=1,.alpha_clamp=1,.kalpha=4};
        assert(!fh_material_compile(&p,&s) && fh_pipeline_compile(&p,&s));
    }
    printf("%u raster alias/swap checks passed across fragment, affine and PICA pipeline paths\n",checks);
}
