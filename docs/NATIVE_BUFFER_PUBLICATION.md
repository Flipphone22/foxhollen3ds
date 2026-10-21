# Native buffer publication candidate

Superseded locally by [prepared buffer layouts](NATIVE_PREPARED_BUFFERS.md),
which remove the per-draw descriptor scan. This document and its archived
candidate remain evidence of the rejected implementation's CPU cost.

12 October 2026. Local replay candidate only; **not promoted to the normal
build and not deployed**. The toggle/sky release remains pending sky repair.

The pinned SDK emits the base address and all 36 descriptor words whenever
`C3DiF_BufInfo` is dirty, even for one-stream draws. The candidate in
`tools/retained_raster.py` stores the previously published register values and
emits the changed span through the SDK command functions. It retains no vertex
data pointers and does not change draws, shaders, geometry, simulation, fences,
cache flushes or command-list ownership.

The common publisher covers ordinary meshes, morph positions, fur streams,
particles and HUD. Comparing all descriptor words deliberately preserves
clearing a retired secondary stream; trusting only `bufCount` would be wrong.
Initial context, framebuffer invalidation and APT restoration force the full
SDK publication. Other SDK clients and program/geometry boundaries retain
their existing paths. The local source audit found no application owner writing
these registers outside the SDK publication path. Pinned Melee's
`native-3ds/port/3ds/citro3d_fix.c` binding snapshots and checked draw-packet
discipline were consulted. This extends retention to buffer descriptors;
Melee's own file is not evidence that this extension is hardware-validated.

Additional state is 148 bytes of saved register values, a 4-byte enable flag,
and four 64-bit counters (184 bytes before any surrounding structure padding).
There are no heap, linear-RAM, VRAM or vertex allocations. The CPU still scans
descriptor values per publication: this is the remaining cost, not a completed
load-prepared buffer architecture.

## Validation

`tools/test_retained_buffers.py` compiles the actual common SDK publisher and
the pinned `buffers.c` for ARM11. Across 1,800 mixed context updates, it compares
every modeled GPU register against the full-publication path. All 330,493
checks pass, including 0–12 streams, changing physical bases/offsets, in-place
mutations, retired streams, context loss and geometry-stage handoffs. Four
deliberately broken variants are rejected: lost invalidation, lost tail clears,
lost base updates and lost first-stream offset updates. Non-buffer side-effect
counts match. This is command-state equivalence, not physical GPU acceptance.

Replay `0d3546767837` / CIA `f5c17af05334` was built from the verified
`vertex-layouts-normal-v1-builds` snapshot, changing only the SDK publisher and
shutdown telemetry. The game archive and exact 511-instruction scene shader
remain unchanged. Old 80 / New 124 MiB CIA profiles are retained. The builder
restores all 440 prior source files and the game archive in `finally`.

Serial cave replays completed with status 1, unchanged saves, drained logs and
owned emulator exit. Both frame-1410 images were inspected: cave geometry,
Fox, ladder, mushrooms, torch, particles and HUD remain visible. The Old touch
stick is present. Particle phases differ, so these are not pixel-identical
comparisons or broad rendering-parity acceptance.

| Azahar profile | Previous scene + submit | Candidate scene + submit | Change | Buffer command words |
| --- | ---: | ---: | ---: | ---: |
| New | 7.365252 ms | 7.471827 ms | +0.106575 ms | 8,599,120 → 504,614 |
| Old | 7.366142 ms | 7.468543 ms | +0.102401 ms | 8,195,520 → 480,182 |

Descriptor traffic falls about 94.1%, but both CPU measurements rise. Same
recipe/settings/assets were used; animated particle workloads differ. New
guest cadence remains 59.831 FPS, Old changes 57.140 → 56.906 FPS. These are
emulator measurements, not console FPS forecasts. Faster host execution alone
is not a guest CPU performance win.

Measured RAM is unchanged: New heap 48,404,624 / 71,303,168 bytes, linear free
18,441,856 / 25,165,824; Old heap 40,867,984 / 50,331,648, linear free
3,761,792 / 10,485,760. Both VRAM free 4,723,712; measured main stack 73,008 /
524,288. Process commitment remains 103,301,120 / 130,023,424 on New and
67,649,536 / 83,886,080 on Old.

The initial Old run was refused by the unchanged storage preflight before any
emulator launched. Four completed, hash-checked ELF files were NTFS-compressed,
preserving paths and bytes, then the run proceeded. No evidence or assets were
deleted. The failed preflight is recorded in the decision report.

## Decision and next step

Keep this as an isolated candidate, not an accepted CPU optimization. Avoid
stacking new effects or deploying this as a speedup. The next implementation
should consume prepared compact buffer layouts and live offsets directly, so
unchanged descriptors need neither rebuilding nor per-draw scanning. It must
still support mutable morph/groom streams and preserve common SDK state and
retirement rules. Hardware command-processing benefits remain unmeasured.

Evidence: `artifacts/arm-buffer-reuse-tests/result.json`,
`artifacts/research/pica-scene-rewrite/buffer-reuse-runtime-test/`, and
`artifacts/native/buffer-reuse-v1-cave-on-{old,new}/`.
