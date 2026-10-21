# Immutable native image placement in VRAM

11 October 2026. Experimental replay `8e4bbc1c9563`, source tag
`ad2f22e49522`; matching local normal `d128b33be3e8`. VRAM placement defaults
OFF. This is a hardware bandwidth experiment, not an established speedup or a
fix for intermittent missing surfaces. The normal candidate is not deployed.

The native material image pool can now choose VRAM at initial image loading,
before publishing a sampler. GPU-ready bytes are copied once through the
existing SDK queue. The original linear allocation remains immutable and
owned for the image's entire lifetime. CPU content comparisons read that
linear copy. Existing parent fences retire copies and draws before freeing
either allocation. There is no per-frame migration, eviction, reverse copy,
CPU VRAM read, sampler pointer rebinding or additional worker.

The pinned Melee `texture_vram.h` documents its disabled runtime promotion
path and hardware failures. Its texture/cache/queue owners and the pinned
Citro3D texture/renderqueue code were consulted. This implementation deliberately
uses initial immutable placement and retains source lifetime; those differences
do not establish hardware safety. L4/A4 stay linear because of the known
hardware hazard. Placement is capped at 3 MiB with at least 1 MiB free VRAM.
Pressure leaves an image in its original linear allocation. The extra image
pointer costs 2 KiB for 512 entries; process commitment grows by one 4 KiB page.
Existing linear texture pixels are retained, not duplicated in linear RAM.

Upload validates main-owner execution, actual linear grant addresses, VRAM
addresses, alignment and size. It finishes preceding native/HUD preparation,
then calls `C3D_SyncTextureCopy`. The SDK splits the prior list and orders the
copy before subsequent draws. It adds no explicit CPU wait for copy completion.
The preexisting queue can still drain when full; this is not a claim of zero
blocking. Existing source cache publication and SDK command publication remain
in force. No raw-register or cache-coherence shortcut was added.

Validation:

- Actual image owner under ASan/UBSan: deferred source/target lifetime, exact
  bytes, CPU dedup source, format exclusions, capacity/reserve/allocation failure
  and retirement. Actual upload function plus pinned SDK copy function: 226
  checks, both supported linear mappings, rejected worker/address/layout errors,
  and queue ordering. Missing join and invalid VRAM-domain mutants fail.
- Headless runner requires explicit `--native-image-vram`, a linked upload
  function and checked shutdown accounting. Four invalid link cases and nine
  invalid telemetry cases are rejected. Default native checks still reject
  unintended VRAM enablement.
- Shared normal/replay: 334 matching native sources; unchanged game archive;
  exact tested 511/214/270-instruction shader programs; Old80/New124 CIA grants.
  Only three audited integration files differ from the no-light stable build.
  Other shelved renderer changes were isolated and restored by the builders.

Serial original-geometry/texture ThornTail Arwing checks:

| Azahar profile | FPS | Scene CPU ms | Submission CPU ms | Copies | Peak image VRAM |
| --- | ---: | ---: | ---: | ---: | ---: |
| New, ON | 54.0865 | 2.07919 | 4.00090 | 410 | 2,684,832 B |
| New, OFF, same binary | 54.6623 | 2.06408 | 3.98536 | 0 | 0 |
| Old, ON | 46.9546 | 2.04936 | 3.95142 | 410 | 2,688,928 B |

All pass state assertions, save preservation, drained logs and owned-process
exit. All ON images are retired by shutdown. Both ON runs leave 113 excluded
image loads in linear RAM, with zero placement-pressure rejections. Captures
at frame1650 were inspected: tree, panelled Arwing, Fox, world and HUD remain
visible. Animation/workloads differ, so these are not pixel-identical pairs.
New combined preparation/submission differs by +0.03066 ms; neither a speedup
nor a material regression is established. No Old OFF comparison was run.
Azahar does not establish physical shared-memory bandwidth or GPU throughput.

New ON heap peak is 56,433,184/71,303,168 B, minimum linear free
16,473,216/25,165,824 B, minimum VRAM free 2,137,088 B. Old ON heap is
41,551,736/50,331,648 B, linear free 1,815,680/10,485,760 B, VRAM free
2,159,616 B. Old linear headroom is only 17.32%, above the 15% reserve but
narrow. Stack measured 72,868 B against 524,288 B reserved. Process commitment
is 103,211,008/130,023,424 B New and 67,559,424/83,886,080 B Old. These are
route-specific measurements, not worst-case whole-game bounds.

Evidence is under `artifacts/research/pica-scene-rewrite/native-image-vram/`
(storage, upload-owner and evidence tests), `native-image-vram-v2-runtime-test/`
(build proof and runtime comparison), and `artifacts/native/native-image-vram-v2-*`.

For a physical comparison, use the same replay case, PICA ACTORS+HUD, MAIN,
GPU profiling OFF, first VRAM OFF and then VRAM ON (R toggles it). Keep the
graphics mask fixed. A successful short run does not prove long-run resource
safety. Report corruption, hangs or missing images as failures regardless of
FPS. The physical question is whether static VRAM residency reduces GPU wait
without corrupting textures. Filesystem stalls, remaining fur/wind, lighting,
shadows and original-image parity remain separate open work.
