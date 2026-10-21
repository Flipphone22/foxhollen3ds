#include "fh_pica_shadow_points.h"
typedef struct {
    FhShadowPoint*v;unsigned vc;FhShadowTrackDraw*d;unsigned dc;
    FhShadowTrackLiveRef*refs;unsigned rc;FhShadowPointSize size;
} PointContext;
static int compile(const FhShadowTrackSource*s,void*arg){
    PointContext*c=arg;
    return fh_pica_shadow_points_compile(s,c->v,c->vc,c->d,c->dc,c->refs,c->rc,&c->size);
}
int fh_pica_shadow_points_raw_compile(const void*raw,unsigned bytes,FhShadowPoint*v,unsigned vc,
    FhShadowTrackDraw*d,unsigned dc,FhShadowTrackLiveRef*refs,unsigned rc,FhShadowPointSize*size,unsigned*scratch,int16_t*y){
    if(!size || !scratch || (v?(!d || !refs):(d!=0 || refs!=0)))return 0;
    PointContext context={.v=v,.vc=vc,.d=d,.dc=dc,.refs=refs,.rc=rc};unsigned used;int16_t offset;
    if(!fh_pica_shadow_track_raw_source(raw,bytes,compile,&context,&used,&offset))return 0;
    *size=context.size;*scratch=used;if(y)*y=offset;return 1;
}
