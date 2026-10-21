#ifndef FH_PICA_WORLD_STREAM_H
#define FH_PICA_WORLD_STREAM_H
#include "fh_pica_scene.h"
#include "fh_pica_draw_owner.h"
typedef struct {
    const FhPicaResidentDraw *draw;
    unsigned fog,buffer_mask;
} FhPicaWorldSelection;
/* Load-only native geometry description, with no SDK context mutation. */
int fh_pica_world_stream_geometry(const FhSceneAsset *,const FhPicaWorldDraw *,FhPicaDrawDescription *);
/* 0 belongs to another native family, 1 prepared draw, 2 source-hidden. */
unsigned fh_pica_world_stream_select(const FhPicaMaterialSet *,unsigned,
                                     FhPicaDrawLive *,FhPicaWorldSelection *);
void fh_pica_world_stream_common(unsigned fog);
unsigned fh_pica_world_packet_count(const FhPicaMaterialSet *,unsigned);
unsigned fh_pica_world_packet_submit(const FhPicaMaterialSet *,unsigned,const FhPicaDrawLive *,unsigned);
#endif
