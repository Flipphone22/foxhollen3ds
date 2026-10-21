#include "fh_install_bindings.h"
#include "fh_asset_paths.h"
#include <string.h>

typedef struct {char assets[8][FH_INSTALL_TX_PATH],disc[FH_INSTALL_TX_PATH];} Paths;
static Paths selected;
static unsigned active,locked;
int fh_install_bindings_select(const char*root,const FhInstallBundleSpec*spec,const char*disc){
    if(locked||!disc||!*disc||strlen(disc)>=FH_INSTALL_TX_PATH)return 0;
    FhInstallBundle bundle={0};Paths candidate={0};
    if(!fh_install_bundle_open(root,spec,&bundle))return 0;
    for(unsigned i=0;i<8;i++)if(!fh_install_bundle_path(&bundle,i,candidate.assets[i]))return 0;
    /* Do not retain pointers into the temporary task or its selected manifest. */
    memset(candidate.disc,0,sizeof(candidate.disc));strcpy(candidate.disc,disc);
    selected=candidate;active=1;return 1;
}
void fh_install_bindings_lock(void){locked=1;}
const char*fh_install_bindings_asset(unsigned kind,unsigned original){
    if(!active||kind>FH_ASSET_TEXTURE)return NULL;
    return selected.assets[kind==FH_ASSET_TEXTURE?FH_BUNDLE_TEXTURE_PACK:(original?FH_BUNDLE_ORIGINAL_MODEL:FH_BUNDLE_PERFORMANCE_MODEL)+kind];
}
const char*fh_install_bindings_movie(void){return active?selected.assets[FH_BUNDLE_PREPARED_MOVIE]:NULL;}
const char*fh_install_bindings_disc(void){return active?selected.disc:NULL;}
