#pragma once
#include "fh_pica_scene.h"
/* Layered secondary-light path. One existing SDK environment remains owner
 * of every fragment light; no parallel environment/cache can retain stale regs. */
unsigned fh_pica_environment_layered_available(unsigned slot,unsigned base,unsigned fog);
int fh_pica_environment_layered_bind(unsigned slot,unsigned base,unsigned fog,const float parameters[2]);
unsigned fh_pica_fog_layered_enabled(unsigned enabled);
unsigned fh_pica_fog_layered_bind(unsigned enabled,float parameters[2]);
unsigned fh_pica_material_layered_selected(const FhPicaMaterialSet*,unsigned index);
unsigned fh_pica_scene_layered_active(void);

/* Source-authored unlit UI models use direct UV GPU skinning. */
unsigned fh_pica_material_ui_selected(const FhPicaMaterialSet*,unsigned index);

/* Source cloud callback materials require ordinary UVs, never sphere/fur UVs. */
unsigned fh_pica_material_cloud_selected(const FhPicaMaterialSet*,unsigned index);

/* Prepared material admission for exact ambient-only GPU skinning. */
unsigned fh_pica_material_ambient_selected(const FhPicaMaterialSet*,unsigned index);
