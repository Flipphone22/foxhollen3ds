#include "fh_gpu_lighting.h"
#include "fh_light_lod.h"
#include "fh_compiled_geometry.h"
static void compiled_fixture_decode(FhVertex *v,const unsigned char *p){memcpy(v,p,sizeof(*v));}
#include "fh_gpu_texgen.h"
#include "fh_draw_capture.h"
#include <math.h>
/* Independent scalar GX oracle, evaluated per vertex before interpolation. */
static void lighting_reference(FhVertex *v,const FhDrawState *s,const FhGpuLighting *l) {
    float eye[3]={0},normal[3]={0},sum[3];
    for(unsigned r=0;r<3;r++) {
        for(unsigned c=0;c<4;c++)eye[r]+=s->model_palette[(unsigned)v->matrix_index+r][c]*v->position[c];
        const float *matrix=l->normal[r];
        if(l->mask&FH_LIGHT_MODEL_NORMAL)matrix=s->model_palette[(unsigned)v->matrix_index+r];
        if(l->mask&FH_LIGHT_NORMAL_PALETTE)matrix=(const float*)l->lights+40+((unsigned)v->matrix_index+r)*4;
        for(unsigned c=0;c<3;c++)normal[r]+=matrix[c]*v->uv[7][c];
        sum[r]=l->ambient[r];
    }
    float len=sqrtf(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
    if(len>1e-20f)for(unsigned c=0;c<3;c++)normal[c]/=len;
    if(l->mask&FH_LIGHT_DIFFUSE_FILL)for(unsigned c=0;c<3;c++){
        float fill=l->lights[1][c][3];
        for(unsigned axis=0;axis<3;axis++)fill+=normal[axis]*l->normal[c][axis];
        sum[c]+=fmaxf(0,fill);
    }
    for(unsigned i=0;i<8;i++)if(l->mask&(1u<<i)) {
        const float (*light)[4]=l->lights[i];float d[3];
        for(unsigned c=0;c<3;c++)d[c]=light[0][c]-eye[c];
        float distance=sqrtf(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
        if(distance>0)for(unsigned c=0;c<3;c++)d[c]/=distance;
        float diffuse=0,cosine=0;
        for(unsigned c=0;c<3;c++){diffuse+=normal[c]*d[c];cosine+=light[1][c]*d[c];}
        if(diffuse<0)diffuse=0;if(cosine<0)cosine=0;
        float num=light[3][0]+light[3][1]*cosine+light[3][2]*cosine*cosine;
        float den=light[4][0]+light[4][1]*distance+light[4][2]*distance*distance;
        float attenuation=num>0 && den>0?num/den:0;
        for(unsigned c=0;c<3;c++)sum[c]+=light[2][c]*diffuse*attenuation;
    }
    for(unsigned c=0;c<3;c++){
        if(l->mask&FH_LIGHT_LIVE_MATERIAL)v->color[0][c]*=l->normal[c][3];
        v->color[0][c]*=fmaxf(0,fminf(1,sum[c]));
    }
    if(l->mask&FH_LIGHT_LIVE_ALPHA)v->color[0][3]*=l->ambient[3];
}
static void reduced_lighting_device_test(void){
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;s.scissor[2]=s.scissor[3]=16;
    s.color_write=s.alpha_write=1;s.pixel_format=1;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=1;
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.channel=4,.coord=255,.map=255,.color_clamp=1,.alpha_clamp=1};
    for(unsigned c=0;c<4;c++){s.swaps[0][c]=c;s.position_matrix[c][c]=1;}
    for(unsigned m=0;m<10;m++)for(unsigned c=0;c<3;c++)s.model_palette[m*3+c][c]=1;
    unsigned checks=0,maximum=0;unsigned char reference[256],actual[256];
    for(unsigned test=0;test<16;test++){
        FhGpuLighting l={0};l.mask=255|FH_LIGHT_MODEL_NORMAL|FH_LIGHT_LIVE_MATERIAL;
        for(unsigned c=0;c<3;c++){l.ambient[c]=.1f;l.normal[c][3]=.4f+.15f*c;}
        for(unsigned i=0;i<8;i++){
            l.lights[i][0][0]=(i&1)?5:-5;l.lights[i][0][1]=(i&2)?3:-3;l.lights[i][0][2]=(i&4)?4:-4;
            l.lights[i][3][0]=l.lights[i][4][0]=1;
            for(unsigned c=0;c<3;c++)l.lights[i][2][c]=.04f*(1+(i+c+test)%5);
        }
        float center[3]={0};if(!fh_light_lod_reduce(&l,center))fh_platform_panic("Reduced lighting fixture admission");
        if(test&1){
            l.mask=(l.mask&~FH_LIGHT_MODEL_NORMAL)|FH_LIGHT_NORMAL_PALETTE;
            float (*n)[4]=(float(*)[4])l.lights[2];
            for(unsigned m=0;m<10;m++){
                float a=.1f*(m+test);
                n[m*3][0]=cosf(a);n[m*3][1]=-sinf(a);n[m*3+1][0]=sinf(a);n[m*3+1][1]=cosf(a);n[m*3+2][2]=1;
                n[m*3][2]=n[m*3+1][2]=n[m*3+2][0]=n[m*3+2][1]=0;
                n[m*3][3]=n[m*3+1][3]=n[m*3+2][3]=0;
            }
        }
        for(unsigned route=0;route<2;route++){
            FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
            for(unsigned i=0;i<6;i++){
                unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
                v[i].matrix_index=((i+test)%10)*3;
                for(unsigned c=0;c<4;c++)v[i].color[0][c]=c==3?.4f:1.f;
                v[i].uv[7][0]=.3f*(int)(test%3);v[i].uv[7][1]=.2f;v[i].uv[7][2]=(test&2)?-1:1;
                if(!route)lighting_reference(v+i,&s,&l);
            }
            s.raw_position=route?3:1;
            if(route && !fh_renderer_prepare_lighting(&s,&l))fh_platform_panic("Reduced lighting GPU rejection");
            fh_renderer_draw(v,6,&s);uint32_t rect[]={0,0,16,16};fh_renderer_copy(route?actual:reference,rect,8,8,6,1,0,0,0,&s);
            if(route)for(unsigned c=0;c<256;c++){
                unsigned error=abs((int)actual[c]-reference[c]);if(error>maximum)maximum=error;checks++;
                if(error>2)fh_platform_panic("Reduced lighting GPU/reference mismatch");
            }
        }
    }
    FILE *f=fopen("sdmc:/3ds/foxhollow/reduced-lighting.txt","wb");if(!f)fh_platform_panic("Reduced lighting report");
    fprintf(f,"%u %u\n",checks,maximum);fclose(f);
}
static void gpu_lighting_device_test(void) {
    FhDrawState s={0};s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;
    s.scissor[2]=s.scissor[3]=16;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=1;
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.channel=4,.coord=255,.map=255,.color_clamp=1,.alpha_clamp=1};
    for(unsigned c=0;c<4;c++)s.swaps[0][c]=c;
    FhVertex original[6]={0};const unsigned corners[]={0,1,2,0,2,3};
    const float xy[4][2]={{-1,1},{-1,-1},{1,-1},{1,1}};
    unsigned checks=0,maximum=0,submitted=0;unsigned char reference[256],actual[256];
    FILE *captures=fopen("sdmc:/3ds/foxhollow/gpu-lighting.bin","wb");
    if(!captures)fh_platform_panic("Lighting capture open failed");
    FILE *fur_config=fopen("sdmc:/3ds/foxhollow/fur_mesh.txt","rb");
    unsigned fur_cases=fur_config && fgetc(fur_config)=='v'?4:0;
    const float texgen_post[3][4]={{-.25f,0,0,.6f},{0,-.25f,0,.6f},{0,0,0,1}};
    if(fur_config)fclose(fur_config);
    for(unsigned fixture=0;fixture<85+fur_cases;fixture++) {
        unsigned unlit_normal=fixture>=81+fur_cases;
        unsigned test=fixture>=57+fur_cases?61+fixture-57-fur_cases:fixture;
        unsigned flat=fixture>=65+fur_cases && !unlit_normal,flat_case=flat?fixture-65-fur_cases:0;
        if(unlit_normal)test=65+fixture-81-fur_cases;
        if(flat)test=(flat_case>=8?32:0)+flat_case%4;
        FhGpuLighting lighting={0};lighting.mask=test<8?1u<<test:test%3==0?255:test%3==1?63:3;
        for(unsigned c=0;c<4;c++)s.position_matrix[c][c]=1;
        for(unsigned c=0;c<3;c++){s.model_palette[c][c]=1;lighting.normal[c][c]=1;lighting.ambient[c]=.04f*(c+1);}
        float translation=test&1?300:0;s.model_palette[0][3]=translation;s.position_matrix[0][3]=-translation;
        if(test==44 || test==45)for(unsigned c=0;c<3;c++) {
            s.model_palette[c][c]=test==44?25:64;s.position_matrix[c][c]=1/s.model_palette[c][c];
            s.position_matrix[0][3]=-translation/s.model_palette[0][0];
        }
        if(test&2){lighting.normal[0][0]=0;lighting.normal[0][2]=1;lighting.normal[2][0]=-1;lighting.normal[2][2]=0;}
        if(test==44)lighting.mask=63;
        for(unsigned i=0;i<8;i++) {
            float (*l)[4]=lighting.lights[i];
            l[0][0]=translation+(int)(i%3)-1;l[0][1]=(int)(i%4)-2;l[0][2]=3+i*.5f;
            l[1][0]=.1f;l[1][1]=.15f;l[1][2]=1;
            for(unsigned c=0;c<3;c++)l[2][c]=(.1f+.07f*((i+c)%4));
            l[3][0]=.4f;l[3][1]=.2f;l[3][2]=.3f;
            l[4][0]=1;l[4][1]=.05f;l[4][2]=.01f;
            if(test==44 && i<2) {
                l[0][0]=(i?-1:1)*60000;l[0][1]=(i?1:-1)*80000;l[0][2]=3000;
                l[1][0]=l[1][1]=l[1][2]=0;l[3][0]=l[4][0]=1;
                l[3][1]=l[3][2]=l[4][1]=l[4][2]=0;
            }
            if(test==16)l[3][0]=l[3][1]=l[3][2]=0;
            if(test==17){l[3][0]=-1;l[3][1]=l[3][2]=0;}
            if(test==18)l[4][0]=l[4][1]=l[4][2]=0;
            if(test==19){l[4][0]=-1;l[4][1]=l[4][2]=0;}
            if(test==20){l[0][0]=translation-1;l[0][1]=1;l[0][2]=-.5f;}
            if(test==21)for(unsigned c=0;c<3;c++)l[2][c]=1;
        }
        if(test%4==0){
            lighting.mask|=FH_LIGHT_LIVE_MATERIAL;lighting.lights[0][3][3]=1;
            lighting.normal[0][3]=test?0.f:.75f;lighting.normal[1][3]=.5f;lighting.normal[2][3]=1.f;
        }
        if(flat){
            lighting.mask=FH_LIGHT_LIVE_MATERIAL|FH_LIGHT_FLAT;lighting.lights[0][3][3]=1;
            if(flat_case&1){lighting.mask|=FH_LIGHT_LIVE_ALPHA;lighting.ambient[3]=(flat_case%5)*.25f;}
            for(unsigned c=0;c<3;c++){
                lighting.ambient[c]=((flat_case+c)%5)*.25f;
                lighting.normal[c][3]=((flat_case*3+c)%5)*.25f;
            }
        }
        s.stages=1;s.texgens=0;s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.channel=4,.coord=255,.map=255,.color_clamp=1,.alpha_clamp=1};
        for(unsigned c=0;c<4;c++)s.swaps[0][c]=(c+(test>=24 && test<32?test%4:0))%4;
        if(test>=32 && test<40) {
            s.texgens=1;s.textures[0]=(FhTexture){.pixels=pixels[0],.width=8,.height=8,.format=0x46};
            s.tev[0].coord=s.tev[0].map=0;
            memcpy(s.tev[0].color,(unsigned[]){15,8,10,15},16);memcpy(s.tev[0].alpha,(unsigned[]){7,4,5,7},16);
        }
        if(test>=49 && (test<57 || test>=61)){
            static unsigned char detail[3][8*8*4];
            s.stages=s.texgens=test&1?2:3;
            if(test>=65)s.stages=s.texgens=(test==66 || test==68)?3:2;
            for(unsigned unit=0;unit<s.stages;unit++){
                for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++)for(unsigned c=0;c<4;c++)
                    detail[unit][(y*8+x)*4+c]=c==3?255:32+((x*(unit+1)+y*(c+1)+test)%8)*28;
                s.textures[unit]=(FhTexture){.pixels=detail[unit],.width=8,.height=8,.format=0x46};
                s.tev[unit]=(FhTevStage){.color={15,8,unit?0:10,15},.alpha={7,4,unit?0:5,7},.channel=4,.coord=unit,.map=unit,.color_clamp=1,.alpha_clamp=1};
                if(test==66)s.tev[unit].coord=unit==2?1:0;
                if(test==67)s.tev[unit].coord=1-unit;
                if(test==68)s.tev[unit].coord=unit==0?1:0;
            }
            for(unsigned slot=1;slot<3;slot++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)s.model_palette[slot*3+r][c]=s.model_palette[r][c];
            if(test==66 || test==68)s.textures[test==68?1:0].width=6;
        }
        if(test>=57 && test<61){
            fur_fixture(&s);s.texgens=5;
            for(unsigned unit=0;unit<5;unit++)s.textures[unit]=(FhTexture){.pixels=pixels[unit],.width=8,.height=8,.format=0x46};
            s.textures[3].pixels=fur_noise;s.textures[3].format=3;
            for(unsigned c=0;c<3;c++)s.tev_colors[3][c]=.15f*(c+1);
            s.tev_colors[3][3]=-.1f;
            /* Deliberately force CPU material fallback in the second pair.
             * Deferred RGB must be resolved before rasterization. */
            if(test>=59)s.textures[2].wrap_s=1;
        }
        if(test>=61){
            lighting.mask|=FH_LIGHT_MODEL_NORMAL;
            for(unsigned slot=0;slot<3;slot++){
                float angle=.2f*(slot+test-61);
                s.model_palette[slot*3][0]=cosf(angle);s.model_palette[slot*3][1]=-sinf(angle);
                s.model_palette[slot*3+1][0]=sinf(angle);s.model_palette[slot*3+1][1]=cosf(angle);
            }
        }
        if((test>=49 && test<57) || test==66 || test==68){
            lighting.mask=(lighting.mask&FH_LIGHT_LIVE_MATERIAL)|FH_LIGHT_NORMAL_PALETTE|3;
            float (*normal)[4]=(float(*)[4])lighting.lights[2];
            for(unsigned slot=0;slot<10;slot++){
                float angle=.17f*(slot+test-49);
                normal[slot*3][0]=cosf(angle);normal[slot*3][1]=-sinf(angle);normal[slot*3][2]=0;normal[slot*3][3]=0;
                normal[slot*3+1][0]=sinf(angle);normal[slot*3+1][1]=cosf(angle);normal[slot*3+1][2]=0;normal[slot*3+1][3]=0;
                normal[slot*3+2][0]=0;normal[slot*3+2][1]=0;normal[slot*3+2][2]=1;normal[slot*3+2][3]=0;
            }
        }
        if(unlit_normal){
            lighting.mask=(lighting.mask&~255u)|FH_LIGHT_LIVE_MATERIAL;
            lighting.lights[0][3][3]=1;
            for(unsigned c=0;c<3;c++){lighting.ambient[c]=1;lighting.normal[c][3]=.25f+.25f*c;}
        }
        for(unsigned i=0;i<6;i++) {
            unsigned k=corners[i];FhVertex *v=&original[i];
            v->position[0]=xy[k][0];v->position[1]=xy[k][1];v->position[2]=-.5f;v->position[3]=1;v->matrix_index=0;v->uv[0][0]=v->uv[0][1]=.5f;v->uv[0][2]=1;
            v->uv[7][0]=test==22?0:.2f*xy[k][0];v->uv[7][1]=test==22?0:.3f*xy[k][1];v->uv[7][2]=test==22?0:1;
            for(unsigned c=0;c<4;c++)v->color[0][c]=(.25f+.15f*c)*(test&4?.5f+.1f*k:1);
            if(test>=49 && (test<57 || test>=61)){
                v->matrix_index=(i%3)*3;
                for(unsigned unit=0;unit<3;unit++){
                    v->uv[unit][0]=.125f+unit*.25f+(xy[k][0]+1)*.0625f;
                    v->uv[unit][1]=.75f-unit*.25f+(xy[k][1]+1)*.0625f;v->uv[unit][2]=1;
                }
                /* Eye-space normals prepared for distinct bone matrices;
                 * each texture coordinate must survive independently. */
                v->uv[7][0]=.1f*(i%3);v->uv[7][1]=-.2f*(i%3);v->uv[7][2]=1+.25f*(i%3);
            }
            if(test>=57 && test<61)for(unsigned unit=0;unit<5;unit++) {
                v->uv[unit][0]=.25f;v->uv[unit][1]=.25f;v->uv[unit][2]=1;
            }
            v->uv[6][0]=0;
            if(test>=65){
                float n[3]={0,0,1};unsigned row=(unsigned)v->matrix_index;
                for(unsigned r=0;r<2;r++)for(unsigned c=0;c<3;c++){
                    const float *matrix=(lighting.mask&FH_LIGHT_NORMAL_PALETTE)?(const float*)lighting.lights+40+(row+r)*4:s.model_palette[row+r];
                    n[r]+=matrix[c]*v->uv[7][c];
                }
                for(unsigned r=0;r<3;r++)v->uv[0][r]=texgen_post[r][0]*n[0]+texgen_post[r][1]*n[1]+texgen_post[r][2]*n[2]+texgen_post[r][3];
            }
        }
        for(unsigned mode=0;mode<2;mode++) {
            FhVertex v[6];memcpy(v,original,sizeof(v));s.raw_position=mode?3:1;
            if(mode){
                if(!fh_renderer_prepare_lighting(&s,&lighting))fh_platform_panic("Lighting fixture eligibility rejected");
                if(test>=65){
                    if(!fh_renderer_prepare_texgen(&s,texgen_post))fh_platform_panic("Texgen fixture eligibility rejected");
                    s.raw_position|=4;
                    for(unsigned i=0;i<6;i++)if(test!=67 || i!=1)v[i].uv[6][0]=1;
                }
                if(test==32 && !flat) {
                    FILE *config=fopen("sdmc:/3ds/foxhollow/capture_draw.txt","wb");
                    if(!config)fh_platform_panic("Lighting capture config failed");fputs("1 6 0\n",config);fclose(config);
                    fh_capture_draw_init();fh_capture_draw(1,v,6,&s,0);
                }
                if(test==46 || (flat && flat_case==7))for(unsigned i=0;i<6;i++) {
                    /* Already CPU-shaded/clip-space vertices mixed into a
                     * deferred draw must bypass lighting as well as projection. */
                    lighting_reference(v+i,&s,&lighting);v[i].matrix_index=-1;
                }
            }
            else for(unsigned i=0;i<6;i++)lighting_reference(v+i,&s,&lighting);
            /* Enough vertices to force rebind/re-upload across bounded storage. */
            unsigned repeat=test==47?5463:1;
            if(test==48) {
                /* Three disjoint regions, with no copy/sync between draws:
                 * changing uniforms then disabling lighting must flush the
                 * previous region before replacing its GPU state. */
                for(unsigned part=0;part<3;part++) {
                    FhGpuLighting next=lighting;next.mask=0;
                    next.ambient[0]=part==0?.8f:.1f;next.ambient[1]=part==1?.8f:.1f;next.ambient[2]=.2f;
                    memcpy(v,original,sizeof(v));s.raw_position=mode && part<2?3:1;
                    for(unsigned i=0;i<6;i++) {
                        if(part<2){v[i].position[0]=v[i].position[0]*.5f+(part?.5f:-.5f);v[i].position[1]=v[i].position[1]*.5f+.5f;}
                        else v[i].position[1]=v[i].position[1]*.5f-.5f;
                        if(!mode && part<2)lighting_reference(v+i,&s,&next);
                    }
                    if(mode && part<2 && !fh_renderer_prepare_lighting(&s,&next))fh_platform_panic("Pending lighting change rejected");
                    fh_renderer_draw(v,6,&s);submitted+=6;
                }
            } else for(unsigned n=0;n<repeat;n++){
                if((mode && test>=65) || test==50){
                    FhVertex shuffled[6];uint32_t offsets[6];uint16_t indices[6];
                    for(unsigned i=0;i<6;i++){shuffled[i]=v[5-i];offsets[i]=i*sizeof(FhVertex);indices[i]=5-i;}
                    FhCompiledMesh mesh={(const unsigned char*)shuffled,offsets,indices,6,6};
                    if(!fh_renderer_draw_compiled(&mesh,&s,compiled_fixture_decode,NULL))fh_platform_panic("Compiled GPU fixture declined");
                }else fh_renderer_draw(v,6,&s);
                submitted+=6;
            }
            uint32_t rect[]={0,0,16,16};fh_renderer_copy(mode?actual:reference,rect,8,8,6,1,0,0,0,&s);
            fwrite(mode?actual:reference,1,256,captures);
            if(mode)for(unsigned c=0;c<256;c++) {
                unsigned error=abs((int)actual[c]-reference[c]);if(error>maximum)maximum=error;
                if(error>2){char message[100];snprintf(message,sizeof(message),"GPU lighting mismatch test=%u byte=%u actual=%u reference=%u",test,c,actual[c],reference[c]);fh_platform_panic(message);}checks++;
            }
        }
    }
    /* Live ordinary texture matrices reuse the same shader bank without
     * lighting, including animated translation and non-unit texture padding. */
    FILE *uv_capture=fopen("sdmc:/3ds/foxhollow/gpu-uvgen.bin","wb");
    if(!uv_capture)fh_platform_panic("UV generator capture open failed");
    unsigned uv_checks=0,uv_error=0;
    memset(&s,0,sizeof(s));s.viewport[2]=s.viewport[3]=16;s.viewport[5]=1;
    s.scissor[2]=s.scissor[3]=16;s.color_write=s.alpha_write=1;s.pixel_format=1;
    s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=s.texgens=1;
    s.tev[0]=(FhTevStage){.color={15,8,10,15},.alpha={7,4,5,7},.channel=4,.color_clamp=1,.alpha_clamp=1};
    for(unsigned c=0;c<4;c++){s.swaps[0][c]=c;s.position_matrix[c][c]=1;}
    for(unsigned c=0;c<3;c++)s.model_palette[c][c]=1;
    for(unsigned test=0;test<32;test++){
        s.textures[0]=(FhTexture){.pixels=pixels[0],.width=test&1?5:8,.height=test&1?7:8,.format=0x46,.min_filter=1,.mag_filter=1};
        float matrix[3][4]={{.5f+(test%4)*.125f,-.125f,0,(test%5)*.0625f},{.125f,.5f,0,(test%7)*.03125f},{0,0,0,1}};
        for(unsigned mode=0;mode<2;mode++){
            FhVertex v[6]={0};s.raw_position=mode?21:1;
            FhGpuLighting flat_light={0};
            if(test>=16){
                flat_light.mask=FH_LIGHT_FLAT|FH_LIGHT_LIVE_MATERIAL|FH_LIGHT_LIVE_ALPHA;
                for(unsigned c=0;c<3;c++){flat_light.normal[c][3]=.5f+c*.25f;flat_light.ambient[c]=.25f+c*.25f;}
                flat_light.ambient[3]=(test%5)*.25f;
                if(mode){s.raw_position|=34;if(!fh_renderer_prepare_lighting(&s,&flat_light))fh_platform_panic("Flat UV lighting declined");}
            }
            for(unsigned i=0;i<6;i++){
                unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
                for(unsigned c=0;c<4;c++)v[i].color[0][c]=1-c*.125f;
                v[i].uv[0][0]=(xy[k][0]+1)*.5f;v[i].uv[0][1]=(xy[k][1]+1)*.5f;v[i].uv[0][2]=1;
                v[i].uv[6][0]=mode;
                if(!mode){float raw[3];memcpy(raw,v[i].uv[0],12);for(unsigned r=0;r<3;r++)
                    v[i].uv[0][r]=matrix[r][0]*raw[0]+matrix[r][1]*raw[1]+matrix[r][2]*raw[2]+matrix[r][3];}
                if(!mode && test>=16)lighting_reference(v+i,&s,&flat_light);
            }
            if(mode && !fh_renderer_prepare_uvgen(&s,matrix))fh_platform_panic("Raw UV fixture declined");
            if(mode && test==5){
                const char *failed="sdmc:/3ds/foxhollow/material-draw-capture.bin",*backup="sdmc:/3ds/foxhollow/fixture-material-capture.bin";
                FILE *existing=fopen(failed,"rb");unsigned restore=existing!=NULL;
                if(existing){fclose(existing);if(rename(failed,backup))fh_platform_panic("Fixture capture backup failed");}
                fh_capture_failed_draw(1,v,6,&s);
                if(rename(failed,"sdmc:/3ds/foxhollow/uvgen-draw-capture.bin") || (restore && rename(backup,failed)))fh_platform_panic("Fixture capture rename failed");
            }
            fh_renderer_draw(v,6,&s);
            uint32_t rect[]={0,0,16,16};fh_renderer_copy(mode?actual:reference,rect,8,8,6,1,0,0,0,&s);
            fwrite(mode?actual:reference,1,256,uv_capture);
            if(mode)for(unsigned c=0;c<256;c++){
                unsigned error=abs((int)actual[c]-reference[c]);if(error>uv_error)uv_error=error;
                if(error>2){char message[100];snprintf(message,sizeof(message),"Raw UV mismatch test=%u byte=%u actual=%u reference=%u",test,c,actual[c],reference[c]);fh_platform_panic(message);}uv_checks++;
            }
        }
    }
    fclose(uv_capture);FILE *uv_report=fopen("sdmc:/3ds/foxhollow/gpu-uvgen.txt","wb");
    if(!uv_report)fh_platform_panic("UV report open failed");fprintf(uv_report,"%u %u\n",uv_checks,uv_error);fclose(uv_report);
    fclose(captures);FILE *f=fopen("sdmc:/3ds/foxhollow/gpu-lighting.txt","wb");
    if(!f)fh_platform_panic("Lighting report open failed");fprintf(f,"%u %u %u\n",checks,maximum,submitted);fclose(f);
    reduced_lighting_device_test();fh_renderer_present();
}
