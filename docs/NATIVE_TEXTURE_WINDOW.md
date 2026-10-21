11 October New223 deployment COMPLETE: normal `4ad2bfc39ced` / CIA
`b93ef5c89545`, replay `01d268cc7891` / CIA `d5fcb77b8a07`. New texture
page caching plus the accumulated environment-normal/unlit-material corrections
are installed. Eight changed application/identity files received full staged
byte readback, rename and final-size checks. Ten original asset/cache files
reuse prior completed byte proof plus current descriptor/topology bytes and
sizes; large asset bytes were not freshly reread. Saves/preferences preserved.
Old229 is unchanged. Matching3dsx ready; CIA requires FBI installation.
Physical stalls, fur and reported visibility/lighting acceptance remain open.
Evidence: `artifacts/console-settings-update/20261010-190758-223/manifest.json`.

# Native texture read cache

11 October 2026. Local builds only; physical SD stalls remain unvalidated.

The native material owner opened the prepared texture pack without attaching
the reader's existing page cache. The `128*1024` open argument limits the
directory allocation; it is not a data cache. Source verification and GPU
image reads consequently reread small chunks even when their contents overlap
or are reused by another material.

`platform_pica_texture_window.h` now owns at most 256 KiB of ordinary heap on
New 3DS, or 64 KiB on Old 3DS. It attaches the existing 32 KiB staged page
reader once at renderer initialization. The cache yields before scene or
material allocation would exceed the existing 85% heap admission threshold.
It is optional when initial memory is insufficient. No per-draw allocation,
worker, GPU allocation, geometry change or shader change is introduced.
Dropping the cache detaches the reader before freeing storage; GPU images
have independent ownership. Source byte verification and image CRCs remain.

Pinned melee-3ds `native-3ds/port/3ds/file_io.c` uses bounded 64 KiB reader
buffers and separates owned I/O from consumers. This adaptation reuses our
already staged pack reader instead of importing its worker or menu cache.
Filesystem reads still land in CPU staging, never in published GPU storage.

The new log reports cache allocation, hits/misses, returned bytes, file bytes,
and `read_calls`. These are calls to the pack reader's `fread`, not a claim of
one-to-one FS IPC calls or physical SD transactions. Existing FS telemetry
retains that separate scope.

## Evidence

`texture-window-test-v2/result.json` exercises the actual owner and reader
under ASan/UBSan: absent stream, memory admission, failed allocation, repeated
initialization/release, reader closure before owner cleanup, and all 482
private pack entries. Every exact GPU encoding and the reader-selected
encoding match file bytes, with and without the cache. Synthetic repeated
entry reads fall from 2,065 calls to 759 on Old and 625 on New. These are not
gameplay forecasts. The initial test incorrectly assumed the selected encoding
must equal an arbitrary duplicate source entry; its failure is preserved in
`texture-window-test`. The corrected test checks each independently.

The existing reader corruption/bounds suite passes 2,054 checks and 647 rejects
in `texture-window-reader-test/summary.json`.

Serial `texture-window-v1-arwing-new` and `-old` headless runs both pass replay
state assertions, save preservation, shutdown/log drain and owned-process
exit. Both frame1650 captures were inspected: tree, textured Arwing, Fox, HUD
and Old touch pad are present. This scene still does not reproduce the user's
physical disappearing-tree/white-Arwing report.

| Arwing route | Prior New | Cached New | Prior Old | Cached Old |
|---|---:|---:|---:|---:|
| Pack file bytes | 15,102,080 | 6,877,792 | 15,102,080 | 13,316,704 |
| New reader calls | unrecorded | 311 | unrecorded | 530 |
| Scene CPU ms/frame | 2.00052 | 2.01579 | 1.97389 | 1.97651 |
| Submission CPU ms/frame | 3.74339 | 3.75527 | 3.71915 | 3.73476 |
| Azahar FPS | 54.3117 | 55.0395 | 47.5878 | 47.3702 |

Prior runs are `fur-gpu-original-arwing-{new,old}`. They use the same recipe,
assets and original-geometry/texture/effect settings, but precede the local
hemisphere and unlit material repairs. Animation phase and draw counts differ;
these timings do not isolate a rendering speedup. Cache returned bytes are
15,082,784 on both devices, exactly the prior pack file bytes less its 19,296
byte header/directory. Less file traffic is not missing texture work. The
cache remains resident throughout these runs. No physical timing is inferred.

New heap peak is 56,418,504/71,303,168 bytes; Old 41,541,280/50,331,648.
Linear minimum free is 16,652,032/25,165,824 New and 1,980,160/10,485,760 Old.
VRAM minimum free remains 4,846,592. Measured main stack is 72,860/524,288.
Process peaks are 103,202,816/130,023,424 New and 67,551,232/83,886,080 Old.
Old linear headroom remains tight; this cache uses ordinary heap only.

`texture-window-runtime-test/runtime-comparison.json` contains timing, memory
and counter records. Build helpers isolate the six unrelated VRAM work files
and restore them in `finally`. This work does not establish that physical
multi-second pauses, visibility bugs, shadow/day-night errors or fur fidelity
are fixed.

Shared local builds: normal `4ad2bfc39ced` / CIA `b93ef5c89545`, replay
`01d268cc7891` / CIA `d5fcb77b8a07`. All320 native source files, the game
archive and both actual shader blobs match normal/replay. Main511/fur214
instruction budgets and Old80/New124 profiles are unchanged. See
`texture-window-runtime-test/build-verification.json`. No new normal UI run
is claimed; unchanged UI evidence is reused. These builds are not deployed.

The final normal binary also passed its own headless startup, touch-menu,
SOURCE FUR/FUR toggle, persistence and Select shutdown check in25.08hostseconds.
Frame540 was inspected: movie, readable option and record control remain.
The isolated FHGRAPH4 mask5259265 and checksum match; console preferences were
not changed by that test. New223 deployment is recorded above.
