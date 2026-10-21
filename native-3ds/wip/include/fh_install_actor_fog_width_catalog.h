#ifndef FH_INSTALL_ACTOR_FOG_WIDTH_CATALOG_H
#define FH_INSTALL_ACTOR_FOG_WIDTH_CATALOG_H
#include "fh_install_actor_fog_width.h"
/* Source tables merge two live areas. A directory-local texture lookup is not
 * sufficient. This startup resolver accepts a width only when every physically
 * present TEX1 definition of that ID agrees. Conflicting dimensions require
 * contextual variants; they are never guessed from the first bank encountered.
 * No pixels or game-derived lookup tables are embedded in the application. */
enum {FH_INSTALL_FOG_TEXTURE_BANKS=256};
typedef struct {
    FhInstallDisc*disc;
    struct {unsigned table,data,entries;} banks[FH_INSTALL_FOG_TEXTURE_BANKS];
    unsigned count,cursor,requests,hits,definitions,conflicts,workspace_peak;
    struct {unsigned index_plus_one,width;} cached[64];
    char error[112];
} FhInstallActorFogWidthCatalog;
/* Zero initialize. Resolves paired TEX1.tab/bin from the validated FST;
 * rejects orphan tables, duplicate paths and excessive bank counts. */
int fh_install_actor_fog_width_catalog_begin(FhInstallActorFogWidthCatalog*,FhInstallDisc*);
/* Compatible with FhActorFogTextureWidth. On failure, output is unchanged. */
int fh_install_actor_fog_width_catalog(void*,uint32_t,unsigned*);
#endif
