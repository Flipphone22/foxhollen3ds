#ifndef FH_RENDER_FLOOR_H
#define FH_RENDER_FLOOR_H
/* Transitional names used by game adapters. There is only one backend.
 * Constants remove old rendering branches from the executable; they cannot
 * be changed by SD configuration or a replay-menu option. */
enum {
    fh_render_floor=1, fh_minimal_enabled=1, fh_pica_scene_enabled=1,
    fh_render_budget_strip=1, fh_render_budget_world=1,
    fh_render_budget_textures=0, fh_render_budget_models=0
};
extern unsigned fh_render_floor_lists,fh_render_floor_primitives,fh_render_floor_copies;
extern unsigned fh_render_floor_setters,fh_render_floor_matrix_ops;
/* The model instruction encodes a four-bit count followed by eight-bit IDs.
 * Consume precisely that instruction without constructing view/normal matrices.
 * Animated model matrices and hit volumes are updated outside this operation. */
static inline unsigned fh_render_floor_matrix_end(const unsigned char *data,unsigned pos){
    unsigned count=data[pos>>3]>>(pos&7);
    if((pos&7)>4)count|=(unsigned)data[(pos>>3)+1]<<(8-(pos&7));
    return pos+4+8*(count&15);
}
#endif
