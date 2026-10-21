#ifndef FH_SURFACE_DETAIL_H
#define FH_SURFACE_DETAIL_H
/* Explicit 3DS visual tier; independent of prepared atlas or model IDs. */
extern unsigned fh_surface_detail;
extern unsigned fh_surface_detail_models,fh_surface_detail_fur,fh_surface_detail_world;
/* Indexed texture matrices override GX_IDENTITY. Preserve the serialized
 * descriptor/packet layout and use only generators without matrix indices.
 * Object setup (no projected light) reserves a forward normal-map prefix and
 * texMtxCount slots from TEX7 downward; jointless objects have neither. */
static inline int fh_surface_uv_base(unsigned joints,unsigned tail,unsigned refs,unsigned aux,unsigned needed){
    unsigned first=joints>1 && refs?(aux?3:1):0;
    if(joints<=1)tail=0;
    return tail<=8 && needed<=8-tail && first<=8-tail-needed?(int)first:-1;
}
#endif
