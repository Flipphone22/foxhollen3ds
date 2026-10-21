11 October New223 deployment COMPLETE: normal `f50343cd2e56` / CIA
`82d3285f6677`, replay `e8eefd5944cf` / CIA `72ee1193db8b`. Layered
secondary-light materials and reduced native memory-log traffic are installed.
Eight changed application/identity files received full staged byte readback,
rename and final-size verification. Ten unchanged original asset/cache files
reuse prior completed proof plus current descriptors/topology/sizes; they were
not bulk-reread. Saves/preferences preserved. Old229 unchanged. Matching3dsx
ready; CIA requires FBI installation. SPECULAR remains opt-in; existing saved
options are preserved. No physical stall, speed or whole-parity claim.
Evidence: `artifacts/console-settings-update/20261010-202815-223/manifest.json`.

# Native memory telemetry

11 October local build, not deployed. This reduces diagnostic SD traffic; it
does not establish that physical filesystem stalls are fixed.

The old memory serializer still emitted retired renderer arenas, vertex/GX
caches and counters. Each sample was approximately 6.3 KB. The retained
physical `sd-slow.csv` contains 64 KiB `memory.jsonl` writes lasting roughly
100–334 ms, some overlapping asset reads. Those overlapping spans cannot be
added or treated as proof of the cause of every stall.

`platform_3ds.c::metrics()` now emits schema 2, explicitly identifying the
native PICA backend. Sampling call sites and cadence are unchanged. It retains
normal/linear/VRAM and process grants, game heaps, measured stacks/canaries,
audio and prefetch workers, disc I/O, diagnostic queue waits/errors, model LOD,
CPU status and the depth-readback regression counters. Native renderer resource
and timing records remain in their existing logs. No new allocation, worker,
queue policy or rendering change is included.

The actual serializer passes ASan/UBSan checks for startup, active play,
resource-query failure and shutdown. The test compares the whole source to the
previous snapshot to verify unchanged sampling call sites and code outside
the serializer. The historical retired-renderer analyzer explicitly rejects
the new schema instead of silently interpreting absent counters as zero.

The same palace recipe/assets/options passed a headless New 3DS Azahar replay,
state/save assertions, shutdown drain and owned-process exit. Image 1410 was
inspected: Krystal, world geometry/textures, purple enemies/glow and HUD remain.

| Measurement | Previous | New |
|---|---:|---:|
| Memory samples | 53 | 53 |
| Memory file bytes | 331,461 | 105,846 |
| Average sample bytes | 6,254 | 1,997 |
| Azahar FPS | 59.831223 | 59.831224 |
| Scene preparation ms | 1.824176 | 1.845799 |
| Native submission ms | 4.217762 | 4.226733 |
| Heap peak bytes | 48,470,104 | 48,470,104 |
| Minimum free linear bytes | 18,703,104 | 18,703,104 |
| Minimum free VRAM bytes | 4,834,304 | 4,834,304 |
| Measured main stack bytes | 72,796 | 72,796 |

The 68% reduction applies to this memory log, not all diagnostic traffic.
Both runs drained without logging errors/backpressure, and both recorded zero
depth transfers. Source animation phases can differ. These are emulator
measurements, not a physical speedup claim.

Normal `f50343cd2e56` / CIA `82d3285f6677` and replay `e8eefd5944cf` / CIA
`72ee1193db8b` share the same game archive and 324 native sources. The prior
511/214/197-instruction shader binaries are unchanged; Old 80/New 124 MiB CIA
profiles are verified. Prior layered-material rendering evidence remains
applicable. Six unrelated VRAM WIP files were isolated and restored by the
builders. Physical builds, saves and preferences were not changed.

Reproduce with `tools/test_native_memory_metrics.py` and research helpers
`build-native-metrics-v1.py`, `build-native-metrics-normal-v1.py`,
`verify-native-metrics-builds.py`, `check-native-metrics-v1.py new on`, and
`analyze-native-metrics.py` under `artifacts/research/pica-scene-rewrite/`.
Reports live in that directory's `native-memory-metrics-test/`.

The disappearing-tree and white-Arwing reports remain unresolved. A source
audit found the sampled fern's alpha-test policy already matches the original;
forcing it visible is not justified. Landed Arwing model 1152 uses auxiliary
environment-bump lighting that the native renderer does not yet implement.
That missing family is relevant, but does not establish the all-white symptom's
cause. Fur indirect sampling and broader shadow/lighting parity also remain.
