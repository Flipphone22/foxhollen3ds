#ifndef FH_PICA_DISTORTION_SUBMIT_H
#define FH_PICA_DISTORTION_SUBMIT_H
#include <citro3d.h>
#include "fh_pica_distortion_mesh.h"

/* Match the established native heat owner: complete triangles, <=65535 indices
 * per submission, and eight-byte aligned index offsets. The load owner must
 * publish/fence the immutable allocation; this function never copies it. */
enum { FH_DISTORTION_INDEX_RUN=65532 };
_Static_assert(FH_DISTORTION_INDEX_RUN%3==0,"Whole distortion triangles");
_Static_assert((FH_DISTORTION_INDEX_RUN*sizeof(uint16_t))%8==0,
               "Aligned distortion index run");
static inline unsigned fh_pica_distortion_submit(const uint16_t *indices)
{
    unsigned batches=0;
    for(unsigned first=0;first<FH_DISTORTION_INDICES;){
        unsigned count=FH_DISTORTION_INDICES-first;
        if(count>FH_DISTORTION_INDEX_RUN)count=FH_DISTORTION_INDEX_RUN;
        C3D_DrawElements(GPU_TRIANGLES,count,C3D_UNSIGNED_SHORT,indices+first);
        first+=count;batches++;
    }
    return batches;
}
#endif
