# Demand-driven specular experiment, 14 October

Status: implemented, built and tested; **not adopted into the normal renderer or
deployed**. It reduces redundant map copies but does not establish a speedup.
The stable local base remains `native-heat-v3`. Physical rapid whole-shadow
dropout remains open; this experiment does not repair shadows.

The physical run collected in `console-visual/20261013-163615-223` reports 7,709
full-map updates and 7,709 half-intensity copies. Its full-map owner's elapsed
submission span is about 44.74 seconds over the entire run. That span can include
queue/reservation waits and overlaps other rendering measurements: it is neither
pure CPU computation nor additive GPU time. This motivated checking unused work,
not claiming the maps explain the entire physical frame budget.

`tools/pica_specular_demand_patch.py` separates original source-group snapshots
from GPU production. Successful material compilation records a six-bit aggregate
of full/half map slots in the first material. Actor entry requests that mask in
constant time, before lighting, camera, pose or material publication. Sampler
getters remain side-effect-free. Unconsumed source states do not render; a consumed
full rewrite invalidates its half map. A change reverted before consumption can
reuse the existing completed map. Source simulation, light equations, shaders,
geometry and clamps are unchanged.

The original ordered `FrameSplit(GX_CMDLIST_FLUSH)` and joined retirement remain.
Pinned Melee `renderer.c::gpu_efb_copy` and `citro3d_fix.c` were consulted: retain
borrowed targets until completion, publish framebuffer writes before sampling,
and preserve SDK texture-cache invalidation. There is no CPU rasterization,
readback, private queue, new worker, or frame allocation. Both target sets remain
resident: **no VRAM saving**. Added normal storage is 312 bytes of source snapshots,
small counters/masks, and four bytes per material. Geometry/linear storage is
unchanged.

Validation:

- Actual C owner under ASan/UBSan: 100 lifecycles and 100,000 invisible snapshots;
  latest source state, OFF controls, full/half demand, stale-half invalidation,
  change/revert, ordered targets and retirement pass. Missing publication,
  stale-half, eager-half and stale-full mutants are rejected.
- Actual material compiler: 36 aggregate publications, multiple material indices,
  full/half and compact selection; rejected materials add no demand. Missing
  demand mutant rejected. Source ordering verifies preflight precedes uniforms.
- Replay build `3fab2370a7e7`, 3dsx `2cd1bdfb0bd3`, CIA `2d770793de6c`. Exact game
  archive `9d521d51f66e` and all 17 linked shaders match native-heat-v3. Only six
  native source/header files differ. Old80/New124 profiles retained. Snapshot
  builder restored worktree/archive; unrelated VRAM work was not incorporated.
- Four serial palace runs (candidate/baseline, New/Old) passed state assertions,
  save preservation, log drain and owned exit. New candidate/baseline and Old
  candidate 1410 images inspected: world, Krystal, enemies, particles and HUD
  present. Source phases and asynchronous captures differ; not pixel-identical
  or comprehensive visual acceptance.

New-mode measured 300-frame interval: baseline 16.713682 ms, candidate 16.711522
ms; this is effectively the same paced cadence. Exclusive scene/object/submission
CPU sum **increased 0.054360 ms/frame**. Whole-run full updates 796 -> 790 and
half copies 796 -> 574; these totals cover startup too, not just the measured
window. Specular consumer draws 23,137 -> 23,270; both have zero unready resources.
Removing copies did not yield a measured CPU speedup. Late materialization can
split full/half production across different actor calls and repeat target/state
publication; further grouping is needed. Do not label fewer draws a frame win.

Old-mode candidate also has zero unready resources (23,137 consumer draws).
Unrelated SMG compilation was observed at both Old-run boundaries. Those runs
are correctness/memory coverage only; no Old timing attribution or physical FPS
forecast is accepted. There is no reason to repeat unchanged boots while that
external workload persists.

Candidate sampled memory (bytes, not continuous high-water proof):

| | New | Old |
|---|---:|---:|
| Heap used / reserved | 48,599,424 / 71,303,168 | 41,061,792 / 50,331,648 |
| Linear free / reserved | 18,008,192 / 25,165,824 | 3,356,160 / 10,485,760 |
| VRAM free | 4,723,712 | 4,723,712 |
| Measured main stack / reserved | 72,972 / 524,288 | 72,984 / 524,288 |
| Process commit / grant | 103,354,368 / 130,023,424 | 67,702,784 / 83,886,080 |

Evidence: `artifacts/research/pica-scene-rewrite/specular-demand-test/` and
`specular-demand-v1-console-replay-builds/`. Replay routes are
`artifacts/native/specular-demand-v1-{baseline-on,on}-{new,old}`.
`analyze_specular_demand.py` preserves the Old exclusion instead of accepting
those timings. A first host material fixture lacked the SDK ADD_MULTIPLY enum;
the harness was corrected, not production behaviour.

The experiment is retained as an idempotent patch, immutable executable/source
snapshot, and `experimental-worktree.zip`. `retain_specular_demand_experiment.py`
reversed only its checked hunks in the shared worktree and verified that applying
them again reproduces the experimental text. Existing production tests were
restored; experiment tests apply the patch in memory. No normal build, remote
files, saves, preferences or prepared assets were changed. No hardware probe is
requested. Next investigate batching demand before scene submission; retain the
unchanged lighting equations and resource-ordering guarantees.
