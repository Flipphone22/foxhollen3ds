#include "fh_fragment_raster.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static float edge(float ax,float ay,float bx,float by,float x,float y){return (bx-ax)*(y-ay)-(by-ay)*(x-ax);}
static unsigned byte(float v){return v<0?0:v>1?255:(unsigned)(v*255+.5f);}
static int inside(const float s[3][2],double x,double y){
    double ax=s[1][0]-s[0][0],ay=s[1][1]-s[0][1],bx=s[2][0]-s[0][0],by=s[2][1]-s[0][1];
    double determinant=ax*by-ay*bx;
    if(fabs(determinant)<1e-10)return 0;
    double u=((x-s[0][0])*by-(y-s[0][1])*bx)/determinant;
    double v=(ax*(y-s[0][1])-ay*(x-s[0][0]))/determinant;
    return u>=0 && v>=0 && u+v<=1;
}
static void check_pixel(const FhFragmentRaster *j,unsigned x,unsigned y,uint32_t expected){
    uint32_t actual=j->pixels[y*j->width+x];
    if(actual==expected)return;
    assert(actual==0);
    double px=j->left+x+.5,py=j->top+y+.5;
    assert(!inside(j->screen,px,py));
    /* An omitted texel must lie more than two pixels from every segment.
     * This independent distance test also covers corners and both windings. */
    for(unsigned i=0;i<3;i++){
        unsigned k=(i+1)%3;double dx=j->screen[k][0]-j->screen[i][0],dy=j->screen[k][1]-j->screen[i][1];
        double q=dx*dx+dy*dy,t=q?((px-j->screen[i][0])*dx+(py-j->screen[i][1])*dy)/q:0;
        if(t<0)t=0;if(t>1)t=1;
        double ex=px-j->screen[i][0]-t*dx,ey=py-j->screen[i][1]-t*dy;
        assert(ex*ex+ey*ey>4);
    }
}
/* Original complete interpolation, independent of the optimized field mask. */
static uint32_t reference(const FhFragmentRaster *j,unsigned x,unsigned y){
    const float (*s)[2]=j->screen;float px=j->left+x+.5f,py=j->top+y+.5f;
    float w[3]={edge(s[1][0],s[1][1],s[2][0],s[2][1],px,py)/j->area,edge(s[2][0],s[2][1],s[0][0],s[0][1],px,py)/j->area,0};w[2]=1-w[0]-w[1];
    float total=0;for(unsigned i=0;i<3;i++){w[i]/=j->input[i].position[3];total+=w[i];}
    float result[4]={0};FhVertex v={0};
    if(fabsf(total)>1e-20f){
        for(unsigned i=0;i<3;i++)w[i]/=total;
        if(j->mode==2){
            for(unsigned i=0;i<3;i++)for(unsigned t=0;t<3;t++)for(unsigned c=0;c<3;c++){
                unsigned coord=(unsigned[]){1,3,4}[t];v.uv[coord][c]+=j->input[i].uv[coord][c]*w[i];
            }
            fh_fur_noise(result,j->state,&v,j->matrix);
        }else{
            for(unsigned i=0;i<3;i++)for(unsigned c=4;c<36;c++)((float*)&v)[c]+=((const float*)&j->input[i])[c]*w[i];
            assert(fh_fragment_evaluate(result,j->state,&v));
        }
    }
    return byte(result[0])<<24|byte(result[1])<<16|byte(result[2])<<8|byte(result[3]);
}
int main(void){
    unsigned char texture[8*8*4];for(unsigned i=0;i<sizeof(texture);i++)texture[i]=i*57+13;
    FhDrawState s={0};FhVertex v[3]={0};uint32_t output[24*16];
    float screen[3][2]={{0,0},{24,0},{0,16}},matrix[2][2][3]={{{.01f,0,0},{0,.02f,0}},{{0,.01f,0},{0,0,.02f}}};
    for(unsigned i=0;i<8;i++)s.textures[i]=(FhTexture){.pixels=texture,.width=8,.height=8,.format=0x46,.wrap_s=i%3,.wrap_t=(i+1)%3,.mag_filter=i&1};
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s.swaps[i][c]=(c+i)%4;
    for(unsigned i=0;i<3;i++){
        v[i].position[3]=1+i*.25f;
        for(unsigned c=0;c<8;c++){v[i].uv[c][0]=i*.2f+c*.07f;v[i].uv[c][1]=i*.13f-c*.02f;v[i].uv[c][2]=1+c*.05f;}
        for(unsigned ch=0;ch<2;ch++)for(unsigned c=0;c<4;c++)v[i].color[ch][c]=.2f+(i+ch+c)*.08f;
    }
    unsigned checks=0;
    for(unsigned coord=0;coord<8;coord++)for(unsigned channel=0;channel<9;channel++)for(unsigned indirect=0;indirect<3;indirect++){
        s.stages=2;s.indirect_count=indirect;
        for(unsigned i=0;i<indirect;i++){s.indirect_order[i][0]=(coord+i+2)%8;s.indirect_order[i][1]=i;}
        for(unsigned t=0;t<2;t++)s.tev[t]=(FhTevStage){.coord=(coord+t)%8,.map=coord,.channel=channel,
            .color={15,8,10,t?0:15},.alpha={7,4,5,7},.color_clamp=1,.alpha_clamp=1,.tex_swap=t,.ras_swap=t,
            .indirect={0,0,0,0,0,0,0,0,indirect?1:0}};
        FhFragmentRaster j={.input=v,.state=&s,.matrix=matrix,.screen=screen,.area=384,.width=24,.height=16,.mode=1,.pixels=output};
        fh_fragment_raster_rows(&j,0,5);fh_fragment_raster_rows(&j,5,16);
        uint32_t baseline[24*16];memcpy(baseline,output,sizeof(baseline));
        unsigned evaluated=j.evaluated;FhFragmentProgram program;fh_fragment_prepare(&program,&s);
        j.program=&program;j.evaluated=0;
        /* Multiple separately consumed row jobs share only immutable prepared
         * input selection; their output and coverage must remain identical. */
        for(unsigned first=0;first<16;first+=4)fh_fragment_raster_rows(&j,first,first+4);
        assert(j.evaluated==evaluated && !memcmp(baseline,output,sizeof(baseline)));checks+=384;
        assert(j.evaluated<384 && j.evaluated>192);
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<24;x++){check_pixel(&j,x,y,reference(&j,x,y));checks++;}
    }
    /* Disabled-sample stages still feed dynamic S/T offsets to a later stage. */
    for(unsigned coord=0;coord<8;coord++)for(unsigned selection=5;selection<=11;selection++) {
        if(selection==8)continue;
        s.stages=2;s.indirect_count=1;s.indirect_order[0][0]=(coord+2)%8;s.indirect_order[0][1]=0;
        s.indirect_exponent[(selection-1)&3]=7;
        s.tev[0]=(FhTevStage){.coord=coord,.map=256,.channel=255,.color={15,15,15,15},.alpha={7,7,7,7},.indirect={0,0,3,selection,6,6,0,0,0}};
        s.tev[1]=(FhTevStage){.coord=(coord+1)%8,.map=1,.channel=255,.color={15,15,15,8},.alpha={7,7,7,4},.color_clamp=1,.alpha_clamp=1,.indirect={0,0,0,0,0,0,1,0,0}};
        FhFragmentRaster j={.input=v,.state=&s,.matrix=matrix,.screen=screen,.area=384,.width=24,.height=16,.mode=1,.pixels=output};
        fh_fragment_raster_rows(&j,0,16);
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<24;x++){check_pixel(&j,x,y,reference(&j,x,y));checks++;}
    }
    FhFragmentRaster j={.input=v,.state=&s,.matrix=matrix,.screen=screen,.area=384,.width=24,.height=16,.mode=2,.pixels=output};
    fh_fragment_raster_rows(&j,0,16);
    for(unsigned y=0;y<16;y++)for(unsigned x=0;x<24;x++){check_pixel(&j,x,y,reference(&j,x,y));checks++;}
    s.stages=1;s.indirect_count=0;s.tev[0]=(FhTevStage){.map=255,.coord=255,.channel=255,.color={15,15,15,12},.alpha={7,7,7,6},.color_clamp=1,.alpha_clamp=1};
    for(unsigned i=0;i<3;i++)v[i].position[3]=1;
    j.mode=1;unsigned seed=39173,covered=0,skipped=0;
    for(unsigned trial=0;trial<256;trial++){
        for(unsigned i=0;i<3;i++)for(unsigned c=0;c<2;c++){
            seed=seed*1664525u+1013904223u;screen[i][c]=(seed>>16)/65535.f*(c?16:24);
        }
        j.area=edge(screen[0][0],screen[0][1],screen[1][0],screen[1][1],screen[2][0],screen[2][1]);
        if(fabsf(j.area)<1e-4f)continue;
        j.evaluated=0;fh_fragment_raster_rows(&j,0,16);skipped+=384-j.evaluated;
        for(unsigned y=0;y<16;y++)for(unsigned x=0;x<24;x++){check_pixel(&j,x,y,0xffffffff);checks++;}
        /* Perturb every vertex corner by a quarter pixel, then quantize to
         * PICA's 1/16 pixel grid. All resulting covered texels must survive. */
        for(unsigned corners=0;corners<64;corners++){
            float snapped[3][2];
            for(unsigned i=0;i<3;i++)for(unsigned c=0;c<2;c++)snapped[i][c]=roundf((screen[i][c]+((corners>>(2*i+c))&1?.25f:-.25f))*16)/16;
            for(unsigned y=0;y<16;y++)for(unsigned x=0;x<24;x++)if(inside(snapped,x+.5,y+.5)){
                assert(output[y*24+x]==0xffffffff);covered++;
            }
        }
    }
    assert(covered && skipped);
    printf("PASS %u exact retained raster/coverage checks, %u perturbed covered texels, %u excluded texels\n",checks,covered,skipped);
}
