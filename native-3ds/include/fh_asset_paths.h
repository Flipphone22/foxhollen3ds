#ifndef FH_ASSET_PATHS_H
#define FH_ASSET_PATHS_H
enum { FH_ASSET_MODEL, FH_ASSET_SCENE, FH_ASSET_CACHE, FH_ASSET_TEXTURE, FH_ASSET_ACTOR_FOG };
const char* fh_asset_path(unsigned kind);
#endif
