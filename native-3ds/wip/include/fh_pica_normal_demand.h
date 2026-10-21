#ifndef FH_PICA_NORMAL_DEMAND_H
#define FH_PICA_NORMAL_DEMAND_H
/* These slots are checked against all three consuming program tables at init.
 * Read the SDK's authoritative pending mask after material publication. */
static inline unsigned fh_pica_normal_needed(unsigned mask){
    return ((mask&(1u<<2)) && (mask&0xff0u)) || (mask&((1u<<12)|(1u<<15)));
}
/* A negative colour scale selects constant RGB material. Alpha always keeps
 * its positive decode; the shader separately retains alpha-light semantics. */
static inline float fh_pica_color_decode(unsigned compact,unsigned native_normals,
    unsigned lighting,unsigned vertex_material){
    float scale=compact?1.f/15:1.f;
    return native_normals && lighting && !vertex_material?-scale:scale;
}
#endif
