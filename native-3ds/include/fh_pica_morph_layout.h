#ifndef FH_PICA_MORPH_LAYOUT_H
#define FH_PICA_MORPH_LAYOUT_H
#include <citro3d.h>
/* Physical differential 20261010-145208: leading padding in the 40-byte
 * second loader corrupts 12/12 images, although Azahar accepts it. The packed
 * 32-byte static lane matches the interleaved reference in all 12 cases.
 * Unlike Melee's different matrix-UV layout, this loader has no padding.
 * Attributes are packed ONCE at admission, never rebuilt per frame. */
static inline int fh_pica_morph_bind(C3D_BufInfo *buffers,const void *positions,const void *attributes){
    return BufInfo_Add(buffers,positions,8,1,0)>=0 &&
           BufInfo_Add(buffers,attributes,32,5,0x54321)>=0;
}
#endif
