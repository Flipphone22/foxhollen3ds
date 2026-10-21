#include "fh_install_effects.h"
#include "fh_actor_fog_collection.h"
#include <mbedtls/sha256.h>
#include <string.h>
static void put(unsigned char*p,unsigned value){for(unsigned i=0;i<4;i++)p[i]=value>>(i*8);}
int fh_install_effects_root(const char*root,char out[FH_INSTALL_TX_PATH]){
    if(!root||!*root||strlen(root)>=220||strstr(root,"..")||!out)return 0;
    int n=snprintf(out,FH_INSTALL_TX_PATH,"%s/actor-fog",root);return n>0&&n<240;
}
int fh_install_effects_key(const FhInstallBundleSpec*spec,const FhInstallBundle*base,unsigned char out[32]){
    if(!spec||!base||!out||!base->generation)return 0;
    unsigned nonzero=0;for(unsigned i=0;i<32;i++)nonzero|=spec->actor_fog_recipe[i];if(!nonzero)return 0;
    unsigned char identity[120]={0};memcpy(identity,"FHAFOG01",8);memcpy(identity+8,spec->actor_fog_recipe,32);
    for(unsigned i=0;i<2;i++){
        const FhInstallGeneration*g=base->stages+i;unsigned artifact=i?1:3;
        if(!g->generation||g->required!=15||g->complete!=15||g->partial!=FH_INSTALL_NO_ARTIFACT||
           memcmp(g->source,spec->source,32)||memcmp(g->recipe,spec->stage_recipe[i],32)||!g->bytes[artifact])return 0;
        memcpy(identity+40+i*40,g->recipe,32);put(identity+72+i*40,g->bytes[artifact]);put(identity+76+i*40,g->crc[artifact]);
    }
    return !mbedtls_sha256_ret(identity,sizeof(identity),out,0);
}
int fh_install_effects_open(const char*root,const FhInstallBundleSpec*spec,const FhInstallBundle*base,FhInstallGeneration*out){
    char directory[FH_INSTALL_TX_PATH];unsigned char key[32];FhInstallGeneration candidate;
    if(!out||!fh_install_effects_root(root,directory)||!fh_install_effects_key(spec,base,key)||
       !fh_install_active(directory,key,spec->source,3,&candidate))return 0;
    for(unsigned i=0;i<2;i++)if(candidate.bytes[i]<FH_ACTOR_FOG_COLLECTION_FOOTER||candidate.bytes[i]>FH_ACTOR_FOG_COLLECTION_LIMIT)return 0;
    *out=candidate;return 1;
}
