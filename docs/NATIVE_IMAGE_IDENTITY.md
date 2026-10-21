# Resident image source identity

11 October experiment **shelved**, not deployed. The additional RAM did not
produce a useful SD-read reduction in the tested route. Active native sources
were restored; frozen candidate sources, binaries and tests remain available.
Physical SD stall reduction is unverified. This investigated repeated load-time
reads, not missing material families, fur, shadows, or rendering throughput.

The prepared-image owner previously asked the texture pack to identify every
source before checking whether that prepared image was already resident. The
pack correctly verifies source bytes, rather than trusting CRC alone, but that
verification can reread the SD card after its ordinary file pages are evicted.

The native owner now retains exact compressed source snapshots for up to32
already-resident prepared images. Lookup requires the same dimensions, CMPR
format, original/reduced texture policy, and every source byte. A hit only
increments the existing image's reference count. It changes no sampler,
filter, UV, GPU allocation, publication, fence or image contents. A miss uses
the unchanged source verification, prepared payload CRC and staged upload.
Snapshots are inserted only after successful validation. Unprepared/converted
images keep their existing path. No borrowed game pointer or hash-only match
can become an accepted identity.

Snapshot storage is capped at128 KiB New /32 KiB Old within the existing512 KiB
material allocation ceiling, not an increased material budget. ARM metadata
is1024 bytes plus counters. The owner uses LRU eviction and the existing85%
normal-heap admission threshold. A final image reference releases its snapshot;
scene-load pressure can release all snapshots before sacrificing file pages or
the larger geometry cache. These CPU copies never feed PICA and require no
GPU synchronization to discard. No worker or per-draw operation is added.
Lookup scans at most32 metadata entries and compares at most the bounded
resident source bytes. This CPU work is necessary to prove exact identity and
occurs only during image acquisition.

The pinned Melee `native-3ds/port/3ds/file_io.c` and `citro3d_fix.c` were
consulted. Melee's bounded reader retention and descriptor lifetime rules are
preserved; this port's extra cache retains verification bytes for native
prepared images, rather than changing Melee's stream or GPU ownership design.

`tools/test_pica_image_identity.py` compiles the actual acquisition/release
functions and real texture-pack reader with ASan/UBSan. GPU allocations are a
host sink.936 real prepared-entry checks across both cache sizes verify the
uploaded bytes and8,424 repeated acquisitions with no additional file reads.
The test deliberately evicts the generic file pages to exercise this specific
cache; it is not a captured gameplay access pattern or a latency measurement.
Changed source bytes, dimensions, formats and quality policy miss correctly;
LRU pressure, allocation refusal, timestamp wrap and final-reference retirement
are checked. Missing-byte-comparison and missing-retirement mutants fail.

The first integrated New v2 run completed with state/save/drain/owned-exit
checks passing, but is rejected: it prematurely dropped the256 KiB file-page
cache when only the material allocation ceiling was tight. It recorded270
identity hits yet344 texture-pack reads versus311 in the prior same-recipe
run. Bytes were6,811,232 versus6,877,792; differing animation/loading work
prevents strict frame-for-frame attribution. The source fix now rechecks
normal-heap pressure after releasing snapshots; a dedicated assertion ensures
material-only pressure preserves the existing file pages. v2 evidence remains
under `artifacts/native/image-identity-v2-new/`; it is not deployment evidence.

The v2 frame1650 image was inspected: Fox, textured Arwing, palm tree, world
and HUD remain visible. This single scene does not resolve reported physical
disappearing objects or white textures. The revised v3 runtime measurements
are below. No claim that multi-second SD stalls are fixed follows from fewer
cache lookups or a nonblank capture.

## Revised runtime result and rejection

Serial v3 New and Old Arwing-approach replays both passed state assertions,
save preservation, log drain and owned-process exit. Both frame1650 images
were inspected and retain Fox, Arwing texture, tree, world and HUD. The same
recipe, options and original assets were used as the retained environment-unlit
baseline. Animation/workloads differ; these are not instruction-identical runs.

| Profile | Baseline pack reads / bytes | v3 reads / bytes | Cache hits / peak bytes |
| --- | ---: | ---: | ---: |
| New | 311 / 6,877,792 | 304 / 6,836,832 | 270 / 131,072 |
| Old | 530 / 13,316,704 | 530 / 13,341,280 | 67 / 32,768 |

The retained file pages already served most repeat source verification.
Snapshot hits therefore are **not** an equal count of saved filesystem calls.
New saves seven calls and40,960 bytes in this route; Old saves no calls and
reads24,576 more bytes with slightly different loaded animation work. This is
not sufficient justification for another128/32 KiB working set. v3 preserves
the original file pages on both profiles and reports zero image errors and
zero live source-snapshot bytes at shutdown.319/535 identity evictions show
that this small cache also churns substantially.

New baseline/v3 Azahar FPS54.535927/53.922056; scene CPU2.058731/2.064617 ms,
submission3.958374/3.978523 ms. Old47.335735/47.155792; scene2.036988/2.040847,
submission3.957117/3.940015 ms. These differing workloads establish neither a
physical FPS change nor the cause of small cadence differences. No steady
rendering operation or GPU shader was changed.

v3 New heap peak56,552,208/71,303,168 bytes; Old41,579,312/50,331,648.
Linear minimum free16,495,744/25,165,824 New and1,793,152/10,485,760 Old.
Both retain4,822,016 free VRAM and measured stack72,868/524,288. Process
commitments103,211,008/130,023,424 and67,559,424/83,886,080. Old's17.10%
linear margin is narrow and remains relevant even though this cache is CPU-only.

The tested replay is `e590099d5acb` / CIA `85e05e369698`, with unchanged game
archive and exact511-main/214-fur/270-material programs. The shared CIA retains
Old80/New124 profiles. A normal build was deliberately not made for this
rejected experiment. `image-identity-runtime-test/experiment-build-verification.json`
records the four changed native sources; `shelved.json` records their checked
restoration. The reverse patch preserves unrelated VRAM work. Latest stable
sources remain `diffuse-slots-v1-builds`; installed console files are unchanged.

The frozen v3 snapshot under `artifacts/research/pica-scene-rewrite/` retains
the implementation. `tools/test_pica_image_identity.py` now tests that archived
candidate, not active native code. Its host checks can be rerun independently;
the archived replay and existing check recipe reproduce execution. Historical
build helpers require the frozen candidate sources staged in an isolated build;
do not blindly invoke them against the restored worktree.

Next investigate the synchronous source/prepared loading sequence and per-file
bursts that fall below the individual50 ms trace threshold. This experiment
does not establish that those are the sole causes of physical stalls. Ordinary
actor/world raster source checks found no new confirmed defect; missing native
material/effect families and physical visibility acceptance remain open.
