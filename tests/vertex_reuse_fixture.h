#include "fh_vertex_reuse.h"
static void vertex_reuse_device_test(void) {
    extern unsigned fh_vertex_reference,fh_vertex_unique,fh_vertex_reused;
    enum { QUADS=6000,COUNT=QUADS*6,BYTES=320*240*4 };
    FhVertex *v=calloc(COUNT,sizeof(*v));unsigned char *reference=malloc(BYTES),*actual=malloc(BYTES);
    uint32_t *identities=malloc(COUNT*sizeof(*identities));
    if(!v || !reference || !actual || !identities)fh_platform_panic("Vertex reuse fixture allocation failed");
    FhDrawState s={0};s.viewport[2]=640;s.viewport[3]=480;s.viewport[5]=1;s.scissor[2]=640;s.scissor[3]=480;
    s.color_write=s.alpha_write=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.blend_src=1;s.stages=1;
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.color_clamp=1,.alpha_clamp=1,.channel=4};
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)s.swaps[i][c]=c;
    const unsigned corners[]={0,1,2,2,1,3};
    for(unsigned q=0;q<QUADS;q++)for(unsigned i=0;i<6;i++){
        FhVertex *p=&v[q*6+i];unsigned corner=corners[i];
        identities[q*6+i]=q*4+corner+1;
        p->position[0]=-1+((q%100)+(corner&1))*.02f;
        p->position[1]=1-((q/100)+(corner>>1))/30.f;p->position[2]=-.5f;p->position[3]=1;p->matrix_index=-1;
        for(unsigned c=0;c<3;c++)p->color[0][c]=((q*(13+2*c))%255)/255.f;
        p->color[0][3]=1;
    }
    uint32_t rect[]={0,0,640,480};
    unsigned first=fh_vertex_reused;
    for(unsigned pass=0;pass<2;pass++){
        fh_vertex_reference=pass==0;fh_renderer_clear(0x203040ff,0xffffff);
        /* Each submission respects the API bound. The second cached draw
         * crosses the index capacity while still fitting the vertex buffer. */
        fh_renderer_draw_identified(v,COUNT/2,&s,identities);fh_renderer_draw_identified(v+COUNT/2,COUNT/2,&s,identities+COUNT/2);
        fh_renderer_copy(pass?actual:reference,rect,320,240,6,0,0,0,0,&s);
        fh_renderer_present();
    }
    fh_vertex_reference=0;
    unsigned hits=fh_vertex_reused-first;
    if(hits<10000 || memcmp(actual,reference,BYTES))fh_platform_panic("Indexed vertex reuse changed rollover image");
    FhVertexReuse cache={0};unsigned checks=BYTES;
    if(fh_vertex_reuse(&cache,1,7)!=7 || fh_vertex_reuse(&cache,1,8)!=7)fh_platform_panic("Vertex identity key failed");checks+=2;
    if(fh_vertex_reuse(&cache,17,8)!=8)fh_platform_panic("Vertex identity collision failed");checks++;
    if(fh_vertex_reuse(&cache,0,9)!=9)fh_platform_panic("Uncacheable vertex failed");checks++;
    memset(&cache,0,sizeof(cache));if(fh_vertex_reuse(&cache,1,0)!=0)fh_platform_panic("Vertex buffer retirement failed");checks++;
    FILE *f=fopen("sdmc:/3ds/foxhollow/vertex-reuse.txt","wb");if(!f)fh_platform_panic("Vertex report open failed");
    fprintf(f,"%u %u %u %u\n",checks,hits,fh_vertex_unique,(unsigned)sizeof(FhVertexReuse));fclose(f);
    free(v);free(reference);free(actual);free(identities);
}
