#ifndef FH_PICA_SPLASH_H
#define FH_PICA_SPLASH_H
#include <stdint.h>
#include "fh_graphics.h"
typedef struct {float position[4],uv[2];uint8_t rows,band,pad[2];} FhPicaSplashVertex;
typedef struct {float x,y,z,size,life;} FhPicaSplashState;
enum {FH_SPLASH_VERTICES=128,FH_SPLASH_INDICES=630};
/* Source ring order and strip winding, prepared once at the resource boundary. */
static inline void fh_pica_splash_template(FhPicaSplashVertex *v,uint16_t *indices,
    const float pos[16][3],const float uv[128][2]){
    for(unsigned m=0;m<8;m++)for(unsigned k=0;k<16;k++){
        unsigned n=m*16+k;
        v[n]=(FhPicaSplashVertex){{pos[k][0],pos[k][1],pos[k][2],1},
            {uv[n][0],uv[n][1]},m*3,m,{0,0}};
    }
    unsigned at=0;
    for(unsigned k=0;k<15;k++)for(unsigned t=0;t<14;t++)for(unsigned j=0;j<3;j++){
        unsigned n=t+((t&1)&&j<2?1-j:j);
        indices[at++]=(7-n/2)*16+k+(n&1);
    }
}
/* Eight affine band descriptions, never transformed/expanded CPU vertices. */
static inline void fh_pica_splash_bands(const FhPicaSplashState *s,float rows[8][3][4],float alpha[8]){
    for(unsigned i=0;i<8;i++){
        float ph=(0.100000024f+0.9f*((float)i/7.f))*s->life;
        float dd=ph-0.5f,fade=-(4.f*(dd*dd)-1.f);
        float lim=0.05f+0.9f*((float)i/7.f);
        float a=s->life<lim?1.f:(1.f-s->life)/(1.f-lim);
        float sc=2.f*ph+1.f;
        rows[i][0][0]=s->size*sc;rows[i][0][1]=rows[i][0][2]=0;rows[i][0][3]=s->x;
        rows[i][1][0]=rows[i][1][2]=0;rows[i][1][1]=s->size;rows[i][1][3]=s->y+s->size*(2.f*fade);
        rows[i][2][0]=rows[i][2][1]=0;rows[i][2][2]=s->size*sc;rows[i][2][3]=s->z;
        alpha[i]=(uint8_t)(int)(255.f*a)/255.f;
    }
}
void fh_pica_splash_init(void *shader_entry);
void fh_pica_splash_load(const float pos[16][3],const float uv[128][2],const FhTexture *mask,const FhTexture *foam);
void fh_pica_splash_release(void);
void fh_pica_splash_draw(const FhPicaSplashState*,const float view[3][4],const float projection[4][4],
                         const unsigned clip[4],const uint8_t ambient[3]);
void fh_pica_splash_shutdown(void);
#endif
