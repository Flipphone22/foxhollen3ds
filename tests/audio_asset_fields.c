/* Appended to the actual decoder: both pinned and native versions run exactly
 * the same differential corpus, under ASan/UBSan and allocation fault injection. */
#include <assert.h>
#include <stdio.h>
static size_t liveBytes, peakBytes, allocationCount;
static long failAt = -1;
void *salMalloc(size_t size) {
  if (failAt == 0) return NULL;
  if (failAt > 0) --failAt;
  size_t *p = malloc(size + 16); if (!p) return NULL;
  p[0] = size; liveBytes += size; ++allocationCount;
  if (liveBytes > peakBytes) peakBytes = liveBytes;
  return (u8 *)p + 16;
}
void salFree(void *data) {
  if (!data) return;
  size_t *p = (size_t *)((u8 *)data - 16);
  assert(liveBytes >= p[0]); liveBytes -= p[0]; free(p);
}
#ifndef FH_COMPACT_ASSET_TEST
static void fh_audio_asset_batch_begin(void) {}
static void fh_audio_asset_batch_end(void) {}
static void freeFields(PCReader *r) { salFree(r->fields); r->fields = NULL; }
#endif
static uint32_t digest = 2166136261u, cases, successes;
static size_t sampleUploads, sampleRemovals;
static size_t sampleBytes(const SAMPLE_HEADER *header) {
  unsigned length = header->length & 0xffffff, type = header->length >> 24;
  return type == SAMPLE_TYPE_PCM16 ? length * 2 : type == SAMPLE_TYPE_PCM8 ? length :
         ((length + 13) / 14) * 8;
}
void hwSaveSample(void *header, void *address) {
  void **data = address; size_t size = sampleBytes(*(SAMPLE_HEADER **)header);
  void *copy = salMalloc(size); assert(copy); memcpy(copy, *data, size);
  *data = copy; ++sampleUploads;
}
void hwRemoveSample(void *header, void *data) {
  (void)header; salFree(data); ++sampleRemovals;
}
static void consume(const void *ptr, size_t size) {
  const u8 *p = ptr; while (size--) digest = (digest ^ *p++) * 16777619u;
}
static void groupDigest(const MusyPCGroupData *g) {
  consume(g->project, g->projectBytes); consume(g->pool, g->poolBytes);
  consume(g->directory, g->directoryBytes);
}
static SND_PC_SPAN load(const char *base, const char *name) {
  char path[1024]; snprintf(path, sizeof(path), "%s.%s", base, name);
  FILE *file = fopen(path, "rb"); assert(file);
  assert(!fseek(file, 0, SEEK_END)); long size = ftell(file); assert(size > 0);
  rewind(file); u8 *data = malloc(size); assert(data);
  assert(fread(data, 1, size, file) == (size_t)size); fclose(file);
  return (SND_PC_SPAN){data, (size_t)size};
}
static void checkGroup(const SND_PC_GROUP_ASSETS *input) {
  MusyPCGroupData out; SND_PC_ASSET_ERROR error;
  bool ok = salPCDecodeGroup(input, &out, &error);
  consume(&ok, sizeof(ok)); ++cases; successes += ok;
  if (ok) groupDigest(&out);
  salPCFreeGroupData(&out); assert(!liveBytes);
}
static uint32_t randomState = 12345;
static uint32_t randomValue(void) {
  randomState ^= randomState << 13; randomState ^= randomState >> 17;
  randomState ^= randomState << 5; return randomState;
}
static void fieldsCorpus(void) {
  u8 source[32768]; for (unsigned i = 0; i < sizeof(source); ++i) source[i] = i * 37;
  for (unsigned test = 0; test < 600; ++test) {
    PCReader r = {.span = {source, sizeof(source)}};
    for (unsigned i = 0; i < 160; ++i) {
      size_t offset = randomValue() % (sizeof(source) - 4);
      unsigned kind = randomValue() % 3;
      if (test % 3 == 0) offset = (offset / 4) * 4;
      if (kind == 0) raw(&r, offset, 1);
      else if (kind == 1) word(&r, offset);
      else dword(&r, offset);
    }
    /* Repeated exact fields and fields straddling bitmap page boundaries. */
    dword(&r, 8190); dword(&r, 8190);
    void *out = materialize(&r); bool ok = out != NULL;
    consume(&ok, sizeof(ok)); ++cases; successes += ok;
    if (out) { consume(out, r.extent); salFree(out); }
    freeFields(&r); assert(!liveBytes);
  }
}
int main(int argc, char **argv) {
  assert(argc == 3); fieldsCorpus();
  for (unsigned bank = 1; bank < 3; ++bank) {
    SND_PC_GROUP_ASSETS input = {load(argv[bank], "pro"), load(argv[bank], "poo"),
                               load(argv[bank], "sdi"), load(argv[bank], "sam")};
    checkGroup(&input);
    for (unsigned section = 0; section < 4; ++section) {
      for (unsigned i = 0; i < 32; ++i) {
        SND_PC_GROUP_ASSETS cut = input;
        SND_PC_SPAN *span = section == 0 ? &cut.project : section == 1 ? &cut.pool :
                            section == 2 ? &cut.directory : &cut.samples;
        span->size = i < 8 ? i : (i * span->size) / 32;
        checkGroup(&cut);
      }
    }
    for (unsigned section = 0; section < 3; ++section) {
      SND_PC_SPAN span = section == 0 ? input.project : section == 1 ? input.pool : input.directory;
      for (unsigned i = 0; i < 64; ++i) {
        size_t at = randomValue() % span.size; u8 *p = (u8 *)span.data + at;
        u8 prior = *p; *p ^= 1u << (i % 8); checkGroup(&input); *p = prior;
      }
    }
    /* Failure must not retain any partial ownership. Retry every allocation
     * boundary, including conversion and batch-owner allocation. */
    allocationCount = 0; checkGroup(&input); size_t allocations = allocationCount;
    for (size_t i = 0; i <= allocations; ++i) {
      failAt = i; MusyPCGroupData out;
      fh_audio_asset_batch_begin(); salPCDecodeGroup(&input, &out, NULL);
      fh_audio_asset_batch_end(); salPCFreeGroupData(&out);
      assert(!liveBytes); failAt = -1;
    }
    peakBytes = allocationCount = 0;
    MusyPCGroupData groups[55]; fh_audio_asset_batch_begin();
    for (unsigned i = 0; i < 55; ++i) {
      assert(salPCDecodeGroup(&input, &groups[i], NULL)); groupDigest(&groups[i]);
    }
    fh_audio_asset_batch_end();
    printf("bank%u_retained=%zu bank%u_peak=%zu bank%u_allocations=%zu\n",
           bank, liveBytes, bank, peakBytes, bank, allocationCount);
    /* A new scope at the SAME addresses must not reuse
     * the old conversion. The input can change between scopes. */
    fh_audio_asset_batch_begin(); MusyPCGroupData independent;
    assert(salPCDecodeGroup(&input, &independent, NULL));
    assert(independent.project != groups[0].project); fh_audio_asset_batch_end();
    salPCFreeGroupData(&independent);
    /* Actual pinned sample registration code: two overlapping references per
     * group, one upload per live sample, balanced teardown after batch closure.
     * The shared directory must neither reset live refcounts nor free early. */
    salPCResetSampleData(); sampleUploads = sampleRemovals = 0;
    u16 ids[55][2];
    for (unsigned i = 0; i < 55; ++i) {
      assert(dataInsertSDir(groups[i].directory, (void *)input.samples.data));
      ids[i][0] = groups[i].directory[0].id;
      ids[i][1] = groups[i].directory[1 + i % (groups[i].sampleCount - 1)].id;
      assert(dataAddSampleReference(ids[i][0])); assert(dataAddSampleReference(ids[i][1]));
      salPCFinishSampleUpload();
    }
    assert(sampleUploads <= 56);
    for (unsigned i = 55; i--;) {
      for (unsigned j = 0; j < 2; ++j) {
        SAMPLE_INFO sample; assert(dataGetSample(ids[i][j], &sample) == 0);
        SDIR_DATA *entry = groups[i].directory;
        while (entry->id != ids[i][j]) ++entry;
        size_t size = sampleBytes(&entry->header);
        assert(sample.length == (entry->header.length & 0xffffff));
        assert(sample.addr != (const u8 *)input.samples.data + entry->offset);
        assert(!memcmp(sample.addr, (const u8 *)input.samples.data + entry->offset, size));
        consume(sample.addr, size); assert(dataRemoveSampleReference(ids[i][j]));
      }
      dataRemoveSDir(groups[i].directory); salPCFreeGroupData(&groups[i]);
    }
    assert(sampleUploads == sampleRemovals); salPCResetSampleData();
    assert(!liveBytes);
    free((void *)input.project.data); free((void *)input.pool.data);
    free((void *)input.directory.data); free((void *)input.samples.data);
  }
  printf("cases=%u successes=%u digest=%u\n", cases, successes, digest);
  return 0;
}
