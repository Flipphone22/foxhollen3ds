#include "fh_pica_distortion_geometry.h"
#include <string.h>
_Static_assert(FH_DISTORTION_INDEX_OFFSET%128==0,"Resident index alignment");
int fh_pica_distortion_geometry_prepare(FhPicaDistortionGeometry *owner,
    const uint8_t *field,size_t field_bytes,unsigned tiled,size_t reserve_bytes)
{
    if(!owner||owner->allocation||!field||field_bytes!=FH_DISTORTION_FIELD_BYTES||tiled>1)
        return 0;
    size_t available=linearSpaceFree();
    if(available<FH_DISTORTION_GEOMETRY_BYTES||
       reserve_bytes>available-FH_DISTORTION_GEOMETRY_BYTES)return 0;
    FhPicaDistortionGeometry candidate={0};
    candidate.allocation=linearMemAlign(FH_DISTORTION_GEOMETRY_BYTES,128);
    if(!candidate.allocation)return 0;
    uint16_t *indices=(void*)((char*)candidate.allocation+FH_DISTORTION_INDEX_OFFSET);
    if(!fh_pica_distortion_mesh(field,field_bytes,tiled,candidate.allocation,
        FH_DISTORTION_VERTICES,indices,FH_DISTORTION_INDICES))goto failed;
    memset((char*)candidate.allocation+FH_DISTORTION_VERTEX_BYTES,0,
           FH_DISTORTION_INDEX_OFFSET-FH_DISTORTION_VERTEX_BYTES);
    AttrInfo_Init(&candidate.attributes);
    AttrInfo_AddLoader(&candidate.attributes,0,GPU_FLOAT,2);
    AttrInfo_AddLoader(&candidate.attributes,1,GPU_SHORT,2);
    BufInfo_Init(&candidate.buffers);
    if(BufInfo_Add(&candidate.buffers,candidate.allocation,sizeof(FhDistortionVertex),2,0x10)<0)
        goto failed;
    if(R_FAILED(GSPGPU_FlushDataCache(candidate.allocation,FH_DISTORTION_GEOMETRY_BYTES)))
        goto failed;
    *owner=candidate;return 1;
failed:
    linearFree(candidate.allocation);return 0;
}
void fh_pica_distortion_geometry_destroy_after_fence(FhPicaDistortionGeometry *owner)
{
    if(!owner)return;
    linearFree(owner->allocation);
    memset(owner,0,sizeof(*owner));
}
