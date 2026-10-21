#ifndef FH_PICA_GLOW_QUALITY_H
#define FH_PICA_GLOW_QUALITY_H
#include "fh_pica_glow_field.h"
#include "fh_pica_glow_quality_keys.h"
#include <mbedtls/sha256.h>
#include <string.h>
/* Load-time only. Exact tiled payload plus dimensions/wrap identify fields;
 * serial numbers and bank-local texture IDs are deliberately not identities.
 * Returns zero on invalid input or hash failure, never guesses a lower budget.
 * Unknown valid fields retain the established default, not a fidelity claim. */
static unsigned fh_glow_field_quality_budget(const FhGlowField *field)
{
    size_t bytes=fh_glow_field_bytes(field);
    if(!bytes||field->format!=1)return 0;
    int candidate=0;
    for(unsigned i=0;i<sizeof(glow_quality_keys)/sizeof(glow_quality_keys[0]);i++)
        if(field->width==glow_quality_keys[i].width&&field->height==glow_quality_keys[i].height&&
           field->wrap_s==glow_quality_keys[i].wrap_s&&field->wrap_t==glow_quality_keys[i].wrap_t)
            candidate=1;
    if(!candidate)return FH_GLOW_MESH_DEFAULT_TRIANGLES;
    unsigned char digest[32];
    if(mbedtls_sha256_ret(field->rgba,bytes,digest,0))return 0;
    for(unsigned i=0;i<sizeof(glow_quality_keys)/sizeof(glow_quality_keys[0]);i++)
        if(field->width==glow_quality_keys[i].width&&field->height==glow_quality_keys[i].height&&
           field->wrap_s==glow_quality_keys[i].wrap_s&&field->wrap_t==glow_quality_keys[i].wrap_t&&
           !memcmp(digest,glow_quality_keys[i].sha256,sizeof digest))return 256;
    return FH_GLOW_MESH_DEFAULT_TRIANGLES;
}
#endif
