#pragma once
#include <citro3d.h>
#include "../vendor/citro3d/internal.h"
/* No public API in the pinned SDK. Change its cached register, preserving
 * procedural fog, sampler enables and cache publication. Never write around
 * the SDK cache. Texture2 may share UV1; otherwise it uses UV2. */
static inline void fh_pica_texture2_coord1(unsigned shared) {
    C3D_Context *ctx=C3Di_GetContext();
    u32 config=(ctx->texConfig&~BIT(13))|(shared?BIT(13):0);
    if(config!=ctx->texConfig){ctx->texConfig=config;ctx->flags|=C3DiF_TexStatus;}
}
