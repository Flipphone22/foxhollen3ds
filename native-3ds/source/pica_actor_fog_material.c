#include "fh_pica_actor_fog_material.h"
int fh_pica_actor_fog_material_alternative(FhPicaActorFogAlternative*out,unsigned joints,
    unsigned texture_matrices,const FhPicaActorFogMaterial*m,const FhPicaActorFogSelection*s){
    if(!out || !m || !s || m->environment>3 || m->auxiliary>1 || m->bump_enabled>1 ||
       m->bump_parameters>255 || m->layers>2 || m->textured_layers>=(1u<<m->layers) ||
       m->decal>1 || (m->decal&&m->layers<2) || s->selected_lights>8 ||
       s->projection_images>=(1u<<s->selected_lights) || s->shadow_near>1 ||
       s->render_flags>255 || s->cast_shadow>1)return 0;
    unsigned bump=0,projected=0;
    if(m->environment && m->auxiliary && m->bump_enabled){
        unsigned extra=1;
        if(m->textured_layers&1){
            if(!m->base_width || m->base_width>65535 || !m->auxiliary_width || m->auxiliary_width>65535)return 0;
            unsigned repeat=(m->bump_parameters&15)*4+1;
            extra=m->base_width/(m->auxiliary_width*repeat)==0;
        }
        bump=2+extra;
    }
    if(s->shadow_near || ((s->render_flags&4) && s->cast_shadow))projected=2;
    else if(s->render_flags&16)projected=1;
    else if(!(s->render_flags&4)){
        unsigned bits=s->projection_images;
        while(bits){projected+=bits&1;bits>>=1;}
    }
    FhPicaActorFogAlternative a={0};
    if(!fh_pica_actor_fog_coordinate(&a.coordinate,m->environment!=0,bump,projected,
            m->layers,m->textured_layers,m->decal) ||
       !fh_pica_actor_fog_routing(&a.route,joints,m->environment!=0,m->auxiliary,
            texture_matrices,s->selected_lights,0,0))return 0;
    *out=a;return 1;
}
