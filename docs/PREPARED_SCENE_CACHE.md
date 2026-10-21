# Optional New 3DS geometry RAM cache

The current scene package includes version8 world-position references. These
are required to render terrain changed by gameplay (for example Ice Mountain's
XYZAnimator); a static version7 package cannot refresh those vertices. After
rebuilding static geometry/topology, preserve the source identity and add the
references before auditing coverage and making the matching cache:

```sh
python3 /work/tools/prepare_world_position_refs.py \
  --source /work/artifacts/research/lod-features/all-maps/native-scene.bin \
  --topology /work/assets/GSAP01-rev1/prepared/model-lod-feature-coverage.bin \
  --output /work/artifacts/research/texture-pressure/dynamic-world
python3 /work/tools/audit_native_scene_coverage.py \
  /work/artifacts/research/texture-pressure/dynamic-world/native-scene.bin \
  --topology /work/assets/GSAP01-rev1/prepared/model-lod-feature-coverage.bin \
  --output /work/artifacts/research/texture-pressure/dynamic-world/coverage.json
python3 /work/tools/prepare_block_cache.py \
  /work/artifacts/research/texture-pressure/dynamic-world/native-scene.bin \
  /work/artifacts/research/texture-pressure/dynamic-world/native-scene.cache --level 12
```

Use a new output directory when reproducing preparation; recorded assets are
not overwritten. The reference step verifies byte-identical static GPU data
before adding metadata. Install the matching executable, scene and cache
together. Runtime residency remains a bounded7 MiB window, not the15.8 MiB
complete cache file.

This development optimization retains a bounded working set of prepared geometry.
Small packages fit entirely; complete-bank packages need demand reads outside
the resident window. It preserves prepared geometry, colors and UVs. It does not cache
the original disc, movie or prepared texture pack, or establish steady 30/60 FPS.

Prepare locally, through the existing Linux SDK environment:

```sh
python3 /work/tools/extend_native_scene.py \
  --source /work/artifacts/research/native-all-maps-v1/native-scene.bin \
  --all-maps --output /work/artifacts/research/native-all-maps-v2 \
  --topology /work/assets/GSAP01-rev1/prepared/model-lod-alpha-coverage.bin
python3 /work/tools/audit_native_scene_coverage.py \
  /work/artifacts/research/native-all-maps-v2/native-scene.bin \
  --topology /work/assets/GSAP01-rev1/prepared/model-lod-alpha-coverage.bin \
  --output /work/artifacts/research/native-all-maps-v2/coverage.json
python3 /work/tools/prepare_block_cache.py \
  /work/artifacts/research/native-all-maps-v2/native-scene.bin \
  /work/artifacts/research/native-all-maps-v2/native-scene.cache --level 12
```

Keep both `native-scene.cache` and `native-scene.cache.json` private. Copy them
beside the matching `native-scene.bin` in `/3ds/foxhollow/`, or use the verified
installer's `--native-scene-cache PATH` with `--install --package PATH`. The
installer checks the source identity against the original pack supplied in the
same operation or the existing remote scene manifest. Saves are not involved.
Do not copy the emulator checkpoint to the console.

The complete-bank package contains 2,102 unique packets, 36,364,096 bytes raw and
13,554,527 bytes compressed on SD. Its working set uses exactly 7,340,032 bytes
of normal RAM including directory, compressed-block residency, input and decode
scratch. It does not allocate the whole compressed file. Startup verifies every
compressed block, then matches the original header and directory. Original
packet CRC and live asset checks remain in place during admission.

New 3DS admits at most 7 MiB within the normal-heap 15% headroom guard. Compressed
cache misses replace overlapping blocks in a byte arena using one private file.
`Native scene window IO` separately reports misses, bytes and evictions. Under
scene/texture memory pressure it releases this optional cache and resumes
original file reads. Old 3DS builds retain original file reads and do not need
the cache. The original prepared scene pack remains necessary for fallback.

`native.log` reports startup verification, cache reads/bytes/decodes and
`Native scene IO` startup/cache/SD ticks, SD calls/bytes and pressure releases.
The scene SD counters cover runtime packet reads through the scene adapter;
they exclude original header/directory reads during initialization and all
other files. Global SD logs continue to cover those other operations.

Startup and initial loading may take longer. Gameplay must be checked for
remaining stalls, especially disc/texture loading and area changes. Host and
emulator timings cannot predict the console's SD latency or CPU/GPU timings.

The subsequent prepared-texture residency implementation uses the existing
`texture-pack.bin`, without a new file format. New 3DS loads exact original
source bytes and native PICA images at startup, sharing identical source bytes
between surface/UI variants only after comparing them in full. The current
482-entry pack occupies10,617,152 bytes of CPU-readable linear RAM instead of
14,306,464 bytes on SD. This optional staging cache is capped at11MiB and yields
to live allocations before violating the4MiB linear reserve. GPU textures have
independent storage, so releasing the staging cache does not invalidate draws.
Old 3DS retains ordinary reads. `Native texture cache IO` records startup SD
bytes, resident reads, runtime prepared-texture SD bytes and pressure releases.

The v14 scene extension adds the separate `shop` and `swapholbot` areas, preserving
all689 earlier packets and appending26. Its original pack is15,389,348 bytes;
its cache is7,215,830 bytes on SD /7,281,366 normal RAM including scratch. Use the
matching v14 cache when installing the v14 original pack; do not mix revisions.

The complete-bank compiler discovers root/common assets and every model/world
bank with `--all-maps`. Its audit checks exact post-topology source identities,
not merely map names. Twenty-six source entries have no render stream and are
explicitly recorded as non-rendering. Two inherited partial model materials
remain recorded as unsupported layered/decal combiners. Inventory completeness
does not assert complete material compatibility or gameplay validation.

Both installers require a current all-bank coverage report. Large cache upload
also requires an explicit verified executable package with window-cache support.
Coverage and preparation manifests are installed alongside the private data.
