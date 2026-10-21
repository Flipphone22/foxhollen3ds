#ifndef FH_ASSET_STAGING_H
#define FH_ASSET_STAGING_H
#include <stdio.h>
/* Main-thread CPU staging for prepared asset reads. Preserve the hardware
 * rule that filesystem IPC never targets unpublished GPU/linear storage.
 * Larger bounded reads amortize SD IPC after resident caches are released.
 * This is stack storage, not an enlarged persistent texture/scene pool. */
#ifndef FH_ASSET_STAGE_BYTES
#define FH_ASSET_STAGE_BYTES (32*1024)
#endif
#if FH_ASSET_STAGE_BYTES < 4096 || FH_ASSET_STAGE_BYTES > 32768
#error Invalid prepared asset staging bound
#endif
/* These binary pack consumers already stage bounded bulk reads explicitly.
 * devkitARM newlib buffered fseek/fread can issue a prefetch and trailing
 * refill for each chunk. Use that existing staging as the only read buffer.
 * This is deliberately NOT a global stdio policy (config/audio/log readers
 * have different access patterns). Call before any operation on the stream. */
static inline FILE *fh_asset_open_staged(const char *path){
    FILE *f=fopen(path,"rb");
    if(f && setvbuf(f,NULL,_IONBF,0)){fclose(f);return NULL;}
    return f;
}
#endif
