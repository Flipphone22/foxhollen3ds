/* Included after the original decoder, renamed to ...Unshared. */
typedef struct FhAudioAssetOwner {
  struct FhAudioAssetOwner *next;
  MusyPCGroupData data;
  SND_PC_GROUP_ASSETS input;
  size_t references;
} FhAudioAssetOwner;
static FhAudioAssetOwner *assetOwners, *assetBatch;
static bool assetBatchActive;

static bool sameSpan(SND_PC_SPAN a, SND_PC_SPAN b) {
  return a.data == b.data && a.size == b.size;
}
static void releaseAssetOwner(FhAudioAssetOwner *owner) {
  if (--owner->references) return;
  FhAudioAssetOwner **link = &assetOwners;
  while (*link != owner) link = &(*link)->next;
  *link = owner->next;
  salPCFreeGroupDataUnshared(&owner->data);
  salFree(owner);
}
void fh_audio_asset_batch_end(void) {
  FhAudioAssetOwner *owner = assetBatch;
  assetBatch = NULL; assetBatchActive = false;
  if (owner) releaseAssetOwner(owner);
}
void fh_audio_asset_batch_begin(void) {
  /* The sole caller has no nesting; end also makes accidental restart safe. */
  fh_audio_asset_batch_end(); assetBatchActive = true;
}
bool salPCDecodeGroup(const SND_PC_GROUP_ASSETS *input, MusyPCGroupData *result,
                      SND_PC_ASSET_ERROR *error) {
  FhAudioAssetOwner *owner = assetBatch;
  if (owner && input && sameSpan(input->project, owner->input.project) &&
      sameSpan(input->pool, owner->input.pool) &&
      sameSpan(input->directory, owner->input.directory) &&
      sameSpan(input->samples, owner->input.samples)) {
    *result = owner->data; ++owner->references;
    if (error) memset(error, 0, sizeof(*error));
    return true;
  }
  if (!salPCDecodeGroupUnshared(input, result, error)) return false;
  if (assetBatchActive && !owner) {
    owner = salMalloc(sizeof(*owner));
    /* Cache allocation failure may fall back to independent ownership. */
    if (owner) {
      *owner = (FhAudioAssetOwner){assetOwners, *result, *input, 2};
      assetOwners = assetBatch = owner;
    }
  }
  return true;
}
void salPCFreeGroupData(MusyPCGroupData *data) {
  for (FhAudioAssetOwner *owner = assetOwners; owner; owner = owner->next) {
    if (data->project == owner->data.project && data->pool == owner->data.pool &&
        data->directory == owner->data.directory) {
      memset(data, 0, sizeof(*data)); releaseAssetOwner(owner); return;
    }
  }
  salPCFreeGroupDataUnshared(data);
}
