#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_movie.h"
#include "fh_pica_texture.h"
#include "fh_pica_scene.h"
#include "fh_minimal_graphics.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"

typedef struct {C3D_Tex texture;unsigned bytes,valid,last_used,format;float scale[2];} Movie;
static Movie movie[3];
static unsigned resident,peak,commits,draws,drains;
static C3D_TexEnv stages[4];
static unsigned stages_ready;
void *fh_pica_movie_begin(unsigned slot,unsigned width,unsigned height){
    return fh_pica_movie_begin_format(slot,width,height,FH_TEXTURE_PICA_ETC1);
}
void *fh_pica_movie_begin_format(unsigned slot,unsigned width,unsigned height,unsigned format){
    /* At most three 192 KiB RGB8 half-frame slots (576 KiB total), or smaller
     * legacy ETC slots. Preserve the independent 15% linear-heap reserve.
     * RGB8 full-resolution images are deliberately outside this memory budget. */
    if(slot>=3 || !((width==256 && height==192)||(width==512 && height==384)) ||
       (format!=FH_TEXTURE_PICA_ETC1&&format!=FH_TEXTURE_PICA_RGB8) ||
       (format==FH_TEXTURE_PICA_RGB8&&width!=256))return NULL;
    Movie *m=movie+slot;unsigned extent=width,bytes=format==FH_TEXTURE_PICA_RGB8?extent*extent*3:extent*extent/2;
    if(m->last_used){
        if(m->last_used==fh_minimal_frames+1)drains++;
        fh_minimal_prepare_write(m->last_used);m->last_used=0;
    }
    m->valid=0;
    if(m->texture.data && (m->texture.width!=extent||m->format!=format)){
        C3D_TexDelete(&m->texture);resident-=m->bytes;memset(m,0,sizeof(*m));
    }
    if(!m->texture.data){
        extern u32 __ctru_linear_heap_size;
        unsigned available=linearSpaceFree();
        if(bytes>576*1024-resident || bytes>available ||
           (u64)(available-bytes)*100<(u64)__ctru_linear_heap_size*15)return NULL;
        if(!C3D_TexInit(&m->texture,extent,extent,format==FH_TEXTURE_PICA_RGB8?GPU_RGB8:GPU_ETC1))return NULL;
        C3D_TexSetWrap(&m->texture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
        C3D_TexSetFilter(&m->texture,GPU_NEAREST,GPU_NEAREST);
        m->bytes=bytes;m->format=format;resident+=bytes;if(resident>peak)peak=resident;
    }
    m->scale[0]=1;m->scale[1]=(float)height/extent;
    return m->texture.data;
}
void fh_pica_movie_commit(void *pixels){
    for(unsigned i=0;i<3;i++)if(pixels && movie[i].texture.data==pixels){
        fh_gpu_cache_flush(pixels,movie[i].bytes);movie[i].valid=1;commits++;return;
    }
    fh_platform_panic("PICA movie commit owner");
}
int fh_pica_movie_bind(const void *pixels,unsigned format,unsigned compact){
    Movie *m=NULL;for(unsigned i=0;i<3;i++)if(pixels && movie[i].texture.data==pixels)m=movie+i;
    if(!m || !m->valid)fh_platform_panic("PICA movie unpublished frame");
    if(!stages_ready){
        for(unsigned i=0;i<4;i++)C3D_TexEnvInit(stages+i);
        C3D_TexEnvSrc(stages,C3D_Both,GPU_TEXTURE0,0,0);
        C3D_TexEnvFunc(stages,C3D_Both,GPU_REPLACE);stages_ready=1;
    }
    fh_pica_scene_actor_second_uv(0);fh_pica_environment_bind(0,0);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<4;i++)C3D_SetTexEnv(i,stages+i);
    C3D_TexBind(0,&m->texture);C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
    float us=compact?(format==5?1.f/256:1.f/1024):1.f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,1,1,1,1);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,8,m->scale[0]*us,m->scale[1]*us,0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,9,us,us,0,0);
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_ALL);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    C3D_CullFace(GPU_CULL_FRONT_CCW);
    m->last_used=fh_minimal_frames+1;draws++;return 1;
}
void fh_pica_movie_shutdown(void){
    /* Called after the host's shutdown GPU drain, before C3D_Fini. */
    for(unsigned i=0;i<3;i++)if(movie[i].texture.data){C3D_TexDelete(&movie[i].texture);memset(movie+i,0,sizeof(Movie));}
    char line[192];snprintf(line,sizeof(line),"PICA movie: commits=%u draws=%u gpu_peak=%u overwrite_drains=%u copy_bytes=0\n",commits,draws,peak,drains);fh_platform_log(line);resident=0;
}
