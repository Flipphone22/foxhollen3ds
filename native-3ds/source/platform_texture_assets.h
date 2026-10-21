/* File-loaded non-paletted textures obey explicit DCStore/DCFlush/GX visibility and
 * allocation lifetime notifications. Dynamic/unregistered images retain exact
 * comparisons. Pinned Melee renderer.c uses the same visibility-generation
 * contract; dynamic textureAlloc producers are not registered by the file loader.
 * Unlike Melee's global generation, an explicit GX hint retains these tokens:
 * only registered file images qualify. Actual CPU writes, including unknown
 * ranges, still invalidate; mode5 checks each retained source byte-for-byte.
 * Palette lifetimes remain separately unproven and are not admitted here. */
enum { TEXTURE_ASSET_SLOTS=1024 };
typedef struct {const void *owner,*pixels;unsigned width,height,format,generation;} TextureAsset;
static TextureAsset texture_assets[TEXTURE_ASSET_SLOTS];
static unsigned texture_asset_mode=4,texture_asset_serial,texture_asset_cursor,texture_asset_exhausted;
static unsigned texture_asset_registered,texture_asset_forgotten,texture_asset_hits,texture_asset_checks;
static unsigned texture_asset_searches,texture_asset_negative_hits;
static unsigned texture_asset_hint_retained;
static int texture_asset_current(const BaseTexture *t){
    return t->asset_slot && t->asset_slot<=TEXTURE_ASSET_SLOTS &&
        texture_assets[t->asset_slot-1].generation==t->asset_generation && t->asset_generation;
}
static void texture_asset_attach(BaseTexture *t){
    if(texture_asset_current(t))return;
    t->asset_slot=t->asset_generation=0;
    if(t->source.palette || t->source.palette_count || texture_asset_exhausted)return;
    /* Cache only the absence of an ownership token, never pixel equality.
     * New registrations advance serial; removals cannot turn a miss into a
     * match. Source writes still take the exact snapshot path. serial+1
     * reserves zero for an unsearched slot; exhaustion above forbids wrap. */
    if(t->asset_miss_epoch==texture_asset_serial+1){texture_asset_negative_hits++;return;}
    texture_asset_searches++;
    for(unsigned i=0;i<TEXTURE_ASSET_SLOTS;i++){
        TextureAsset *a=texture_assets+i;
        if(a->generation && a->pixels==t->source.pixels && a->width==t->source.width && a->height==t->source.height && a->format==t->source.format){
            t->asset_slot=i+1;t->asset_generation=a->generation;t->asset_miss_epoch=0;return;
        }
    }
    t->asset_miss_epoch=texture_asset_serial+1;
}
static int texture_asset_preserve_hint(const BaseTexture *t,int hint){
    return hint && texture_asset_mode>=4 && t->valid && texture_asset_current(t);
}
void fh_minimal_texture_asset(const void *owner,const FhTexture *s){
    if(!fh_minimal_enabled || !owner || !s || !s->pixels || (s->format>6 && s->format!=14) || s->palette || s->palette_count ||
       !s->width || !s->height || !fh_texture_source_bytes(s->format,s->width,s->height) || texture_asset_exhausted)return;
    if(texture_asset_mode==3 && s->format!=14)return; /* Same-binary former policy. */
    if(texture_asset_serial==UINT32_MAX){
        memset(texture_assets,0,sizeof(texture_assets));texture_asset_exhausted=1;return;
    }
    unsigned slot=TEXTURE_ASSET_SLOTS;
    for(unsigned i=0;i<TEXTURE_ASSET_SLOTS;i++)if(texture_assets[i].pixels==s->pixels){slot=i;break;}
    if(slot==TEXTURE_ASSET_SLOTS)for(unsigned i=0;i<TEXTURE_ASSET_SLOTS;i++)if(!texture_assets[i].generation){slot=i;break;}
    if(slot==TEXTURE_ASSET_SLOTS)slot=texture_asset_cursor++%TEXTURE_ASSET_SLOTS;
    fh_minimal_texture_written(s->pixels,fh_texture_source_bytes(s->format,s->width,s->height));
    texture_assets[slot]=(TextureAsset){owner,s->pixels,s->width,s->height,s->format,++texture_asset_serial};
    texture_asset_registered++;
}
void fh_minimal_texture_asset_forget(const void *allocation,size_t bytes){
    uintptr_t base=(uintptr_t)allocation;
    if(!allocation || !bytes)return;
    for(unsigned i=0;i<TEXTURE_ASSET_SLOTS;i++){
        TextureAsset *a=texture_assets+i;uintptr_t owner=(uintptr_t)a->owner;
        if(a->generation && owner>=base && owner-base<bytes){memset(a,0,sizeof(*a));texture_asset_forgotten++;}
    }
    /* Address reuse can occur before the next draw. Invalidate scoped reuse as
     * well as lifetime tokens; never free an allocation still read by PICA. */
    fh_minimal_texture_written(allocation,bytes);
}
