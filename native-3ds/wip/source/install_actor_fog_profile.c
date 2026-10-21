#include "fh_install_actor_fog_profile.h"
static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const unsigned char*p){return (unsigned)p[0]<<8|p[1];}
int fh_install_actor_fog_profile(FhPicaActorFogMaterial*out,const void*source,
    unsigned bytes,unsigned index,unsigned bump,FhActorFogTextureWidth width,void*context){
    if(!out||!source||bytes<252||bytes>4*1024*1024||bump>1)return 0;
    const unsigned char*raw=source;unsigned materials=raw[0xf8],textures=raw[0xf2];
    unsigned shaders=word(raw+0x38),ids=word(raw+0x20);
    if(index>=materials||shaders>bytes||materials>(bytes-shaders)/68||
       ids>bytes||textures>(bytes-ids)/4)return 0;
    const unsigned char*p=raw+shaders+index*68;
    FhPicaActorFogMaterial m={0};unsigned flags=half(raw+0xe2);
    m.environment=(word(p+8)!=0 && (flags&0xc)!=0)|
        ((word(p+0x14)!=0 && (flags&0xe00)!=0)<<1);
    unsigned auxiliary=word(p+0x34);m.auxiliary=auxiliary!=0xffffffffu;
    if(m.auxiliary&&auxiliary>=textures)return 0;
    m.bump_enabled=bump;m.bump_parameters=p[0x42];m.layers=p[0x41];
    m.decal=(word(p+0x3c)&0x100000u)!=0;
    if(m.layers>2||(m.decal&&m.layers<2))return 0;
    for(unsigned i=0;i<m.layers;i++){
        unsigned texture=word(p+0x24+i*8);
        if(texture!=0xffffffffu){if(texture>=textures)return 0;m.textured_layers|=1u<<i;}
    }
    /* The original resolver never touches an absent layer-zero union. Match
     * the game adapter's orphan rejection instead of treating -1 as relocated
     * NULL here. This also avoids guessing bump inputs for malformed headers. */
    if(!m.layers&&word(p+0x24))return 0;
    if(m.environment&&m.auxiliary&&bump&&(m.textured_layers&1)){
        unsigned base=word(p+0x24);
        if(!width||!width(context,word(raw+ids+auxiliary*4),&m.auxiliary_width)||
           !width(context,word(raw+ids+base*4),&m.base_width)||
           !m.auxiliary_width||m.auxiliary_width>65535||!m.base_width||m.base_width>65535)return 0;
    }
    *out=m;return 1;
}
