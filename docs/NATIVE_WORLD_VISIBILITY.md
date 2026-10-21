# Native world draw visibility

Local candidate, not deployed. The accepted renderer submits every prepared
group in a world block selected by the game. This candidate uses the existing
per-group receiver bounds to reject a draw only when its entire box is outside
one of the four homogeneous camera side planes. It retains draw/index order,
materials, game visibility decisions, simulation and all in-view geometry.
It does not use perspective division, near/far rejection or temporal caching.

The bounds belong to the original world plan, not the separately merged shadow
plan. They already exist with shadows disabled. The accepted scene owner
rebuilds them after world-position mutation, before publishing the changed GPU
storage, under its existing reader retirement fence. No additional bounds,
vertices, indices, allocations, transfers, workers or shaders are introduced.
An unavailable or mismatched bounds owner keeps the draw visible. Nonfinite
plane arithmetic also keeps it visible. Camera planes are prepared once per
block; the per-group work is a conservative box/plane check. The margin and
homogeneous tests follow the existing pinned-Melee-derived world bounds helper.

`world-visibility-v3-builds` changes only `platform_pica_scene.c` relative to
the accepted `postlight-specular-v1-builds`. The game archive, draw-record ABI
and scene shader remain identical. Additional persistent state is two 32-bit
counters and one 64-bit counter. There is no added resident bounds cost.

`test_world_visibility_reuse.py` checks the actual receiver compiler on 1,259
private world packets, 10,912 draws and 2,932,998 indexed corners. Receiver
bounds exactly match an independent corner scan; every stored homogeneous W
is one. Across 349,184 random plane sets, all 218,158 rejections have a matching
double-precision outside-plane witness over every indexed vertex. Random
planes are correctness coverage, not a gameplay savings estimate.

Failure history is preserved: V1 changed the draw-record stride and failed a
runtime material assertion, because the game archive retained the original ABI.
V2 stopped at a patch assertion before building, revealing the existing trailing
receiver storage. V3 reuses that storage and leaves the ABI/owner unchanged.
The initial V1 replay also stopped once at the disk-space guard before launch;
old completed ELF evidence was losslessly compressed with hashes preserved.
An early host reuse test ran while the isolated builder had temporarily shelved
its oracle header; it was rerun successfully after builder completion.

Hardware acceptance, runtime savings, image acceptance and the full game
performance targets are separate requirements. Do not infer console FPS from
the source audit or emulator timing. The pending cloud UV repair is not included
in this isolated candidate.

The exact shared V3 replay is `879849d1eecf` / CIA `0383442d8955`.
All 446 source identities, the game archive and eleven linked shader hashes
were checked; Old80/New124 CIA profiles remain intact. The source patch is
isolated in `pica_world_visibility_reuse_patch.py`, not applied over global WIP.

Matched New-profile cave runs both complete with state assertions, unchanged
saves, drained logs and owned exit. Frame-1410 captures were inspected in both:
Fox, ladder, tunnel, ground, mushrooms and HUD remain visible. Particle phase
differs, so these are not pixel-identical fixed-state observations.

| New Azahar cave, 598 measured frames | Accepted baseline | V3 |
|---|---:|---:|
| Scene preparation | 1.876618 ms | 1.886592 ms |
| Native submission | 5.107182 ms | 4.693293 ms |
| Emulated guest cadence | 59.831224 FPS | 59.831223 FPS |
| Heap peak | 48,404,624 B | 48,404,624 B |
| Minimum free linear RAM | 18,441,856 B | 18,441,856 B |
| Free VRAM | 4,723,712 B | 4,723,712 B |
| Process commitment | 103,301,120 B | 103,305,216 B |

The net scene/submission reduction is 0.403916ms, about 5.8% of that combined
CPU cost. V3 rejects 32,152 of 122,134 whole-run group checks and 3,125,412
indices. Whole-run animation/frame counts differ slightly; these totals are
not exact workload equivalence. The test used sequential silent emulators with
no concurrent build or compression. Host duration was 58.99 seconds for V3 and
52.92 for baseline; neither host duration nor paced guest FPS establishes console
throughput. No physical speedup claim is made.

The Old-profile V3 cave also completes with saves/state/drain/owned-exit checks
and an inspected1410 capture including its touch item pad. No new Old baseline
was run, so do not claim a matched Old speedup. Detailed component timings and
memory are in `world-visibility-reuse-test/runtime.json`.

The original-geometry/textures Thorntail movement route also completes normally
in 79.04 host seconds, retaining input/state checks, save hashes and log drain.
Captures1500/1650 show Fox approaching the textured Arwing, with trees, cliff
walls, ground and HUD present at both camera positions. V3 rejects32,498 of
90,805 world-group checks and3,302,742 indices. There is no matched timing
baseline for this route; it is additional visibility coverage, not a speedup
claim. `world-visibility-reuse-test/moving-route.json` records its measurements.

The pinned source consulted is
`reference/melee-3ds/native-3ds/port/engine/geometry_bounds.h`: four homogeneous
half-spaces, finite checks and0.002 relative margin. Foxhollow uses the live
rotated top-screen projection and prepared world positions. It does not add
Melee's stereo plane cache or perform actor culling with static world bounds.

The matching normal application is now built locally as `0ca1364b4af2` /
CIA `2f713979a04a`. `verify_world_visibility_normal.py` checks all446 source
identities against replay879849d1eecf, the unchanged archive, active shader and
Old80/New124 profiles. No additional normal boot was needed for unchanged UI
code; prior normal startup/toggle evidence and shared renderer checks are reused.
Neither app is deployed. This normal build contains the accepted effect-OFF
work gates but does not contain the hardware-pending cloud shader candidate.
