# Shadow palette publication

11 October: implemented and checked locally, not deployed. V1 palette masking
alone gave no useful cave improvement. V2 below removes the blanket exclusion
of inactive morph-capable models; this reduces submissions without changing
topology. It does not add missing shadow fidelity.

Compact shadow plans now record a four-byte mask of bone slots changed from
the preceding draw. Submission composes/uploads only those live matrices.
The first draw of every object and every command-state reset republishes all
required slots. There is no cross-instance or cross-frame pose cache, geometry
change, worker, shader change or GPU allocation. This adapts the pinned Melee
immutable palette-mask and uniform-publication approach. Existing ownership
and retirement fences are unchanged.

All835 prepared model plans retain328,012 ordered triangles and319,752 vertices
with identical positions, bone weights and offsets. All2,505 negative cases
pass. Across that collection, a no-reset traversal needs5,954 instead of6,448
bone uploads (7.7% fewer); this is not a runtime FPS estimate.

The actual submission function passes5,120 synthetic draws across changing
poses/instances, palette slots/counts, the synthetic world bone, and every
command-reset position. Uniform storage is poisoned to reject stale values.
Removing the full-republication condition fails the test. ASan/UBSan and the
100-cycle delayed-reader retirement test pass.

The serial cave comparison passes state/save assertions, log drain and owned
exit. Image1410 was inspected: Fox, cave geometry/textures, mushrooms, HUD and
shadow remain. Both runs render the same5,005 casters,15,015 shadow draws and
8,790,210 shadow indices over the run.

| New 3DS Azahar measurement | Deployed baseline | Candidate |
|---|---:|---:|
| FPS | 59.831224 | 59.831225 |
| Scene preparation ms | 1.986860 | 2.010959 |
| Native submission ms | 5.747719 | 5.751384 |
| Heap peak bytes | 48,386,048 | 48,386,072 |
| Minimum free linear bytes | 18,804,224 | 18,804,224 |
| Minimum free VRAM bytes | 4,834,304 | 4,834,304 |
| Measured main stack bytes | 72,796 | 72,796 |

There is **no useful measured cave speedup**. All4,290 compact casters in this
run have one draw, so none benefits from skipping later palette uploads.
The remaining715 noncompact casters account for10,725 draws. A follow-up
corrected the initial attribution to active Fox morphs: this run records zero
morph updates, and the aggregate shadow counters do not identify the caster.
The loader excluded every morph-capable model even while its morph was inactive.
That blanket exclusion, rather than active deformation, is the next target.

The first baseline run overlapped compilation and is explicitly excluded from
timing comparison. Its replacement and the candidate run were serial without
builds or other emulators. No further OFF/Old/hardware repeats were requested
for an optimization that does not affect this scene's work.

Read-only morph-batch feasibility shows Fox465 has16 native groups, seven with
morphable positions. Keeping those groups separate while merging adjacent
static groups could reduce shadow draws to11. Krystal1 has12 morph groups out
of15 and gains nothing from that restriction. These are estimates, not an
implemented batch path, and do not establish which caster dominated this run.
A stronger active-morph implementation must batch live position
lanes while retaining their existing fenced ownership, source offsets and
bone mappings; replacing them with static geometry is incorrect. Sparse
morph writes must remain sparse, and extra resident storage must be budgeted.

Normal `ca2122c8c415` / CIA `8b8ec0b71e6b` and replay `55832803ca68` / CIA
`6218d0029115` share324 native sources, the unchanged game archive and unchanged
511/214/197-instruction shaders. Old80/New124 MiB CIA profiles are verified.
The only native changes are the shadow-plan header/compiler/submission helper.
Six unrelated VRAM WIP files were isolated and restored by the builders.

Evidence: `artifacts/research/pica-scene-rewrite/shadow-palette-test/`, especially
`result.json`, `compiler.json`, `lifetime.json`, `build-verification.json`,
`runtime-comparison.json` and `morph-batch-feasibility.json`.
Tools: `test_pica_shadow_palette.py`, `test_pica_shadow_plan.py`,
`test_pica_shadow_lifetime.py`, `audit_pica_morph_shadow_batches.py`.
Research helpers: `build-shadow-palette-v1.py`,
`build-shadow-palette-normal-v1.py`, `verify-shadow-palette-builds.py`,
`check-shadow-palette-v1.py` and `analyze-shadow-palette.py`.

The installed New3DS remains normal f50343cd2e56/replay e8eefd5944cf. No remote
files or saves changed. Full shadow softness, receiver coverage, self-shadow,
fur/lighting/visibility repairs, physical performance and goal completion remain
unproven or incomplete.
# Morph-capable base plans (v2, local)

The loader no longer rejects every model with a morph table. The existing draw
owner selects the compact base plan only when there is no live morph view;
active deformation continues through the fenced native position lanes. The
512 KiB linear and 64 KiB metadata caps, essential-geometry reserve, and joined
release remain unchanged. No base-pose substitution is permitted for live morphs.

An actual-router test covers 800 combinations and rejects a mutation that uses
the base mesh while a live morph is present. The owner test now uses a
morph-capable model through its 100 load/retire cycles, allocation failures and
GPU-reader lifetime checks. Neither test changes the original morph producer.

The completed New-profile cave check reduces shadow draws from 15,015 to 6,435,
with the same 5,005 casters and 8,790,210 indices. Scene preparation plus native
submission changes from 7.734579 to 7.668379 ms: a modest 0.0662 ms saving in
Azahar, not a physical FPS result. Animation phases differ; the candidate also
reports 13 morph updates while the baseline reports zero. The retrace1410
capture was inspected, with character, cave, mushrooms, shadow and HUD retained.

Minimum free linear memory changes from 18,804,224 to 18,579,712 B, a 224,512 B
increase in peak usage. Compact-plan peak is 479,920 B within its 524,288 B cap.
Optional plans remain allocated with shadows disabled; that cost is explicit.

Replay `c602175f7873` / CIA `96753f7d2dba` and normal `007333766cbd` /
CIA `0d312ad000e0` are built locally, not deployed. Their 324 native sources,
game archive and three shader programs match. Old80/New124 CIA profiles pass.
The subsequent New-OFF and Old-ON checks hit host disk exhaustion: captures
failed and replay results are incomplete. Those failed runs are excluded from
all timing/acceptance claims and retained. Replacement checks require fresh
labels after lossless archival/deduplication; no guest failure is inferred.

Both replacement checks completed: `shadow-palette-v2-storage-ok-cave-disabled-new`
and `shadow-palette-v2-storage-ok-cave-on-old`. State assertions, save hashes,
complete replay results, log drain and owned-process exit pass. Both retrace1410
captures were inspected. Original failed summaries and exclusion records remain.

| Azahar v2 cave | FPS | Scene CPU ms | Submission CPU ms |
|---|---:|---:|---:|
| New silhouettes ON |59.83122|2.018713|5.649666|
| New shadows disabled |59.83123|1.820735|5.385564|
| Old silhouettes ON |56.55045|1.973913|5.671174|

The same-binary New whole-shadow CPU addition is **0.462081 ms**, under the
provisional 0.5 ms gate for this scene. This is not a GPU-cost or physical-FPS
acceptance. No Old OFF comparison was repeated. Animation/particle phase differs;
the Old image includes a mist plume. Defaults remain OFF pending fidelity and
physical cost, not because this local CPU gate still fails.

Old peak heap is 40,849,992/50,331,648 B, minimum free linear is
3,899,648/10,485,760 B, VRAM free 4,834,304 B, measured stack
72,796/524,288 B, and process commitment 67,547,136/83,886,080 B.
New corresponding values are 48,386,632/71,303,168; 18,579,712/25,165,824;
4,834,304; 72,796/524,288; 103,198,720/130,023,424 B.

Host recovery retained every evidence path: 369 identical archived ELFs share
verified hardlinks (12,497,345,604 logical duplicate bytes reclaimed), and the
failed capture set was losslessly archived with member hashes. The runner now
checks available storage before starting an emulator. Record:
`artifacts/research/elf-dedup-20261010-205731.json`. No assets or saves were
deduplicated. The fresh read-only FTP collection at
`artifacts/console-visual/20261010-205223-223` contains the same native log as
before, so provides no new hardware measurement; no console files were changed.

See `shadow-palette-test/morph-admission.json`, `build-verification.json` and
`runtime-comparison.json`. The preceding section retains V1 history.
