# Synchronous CPU skin input views

This is an unintegrated CPU-consumer experiment, not a rendering backend or a
replacement for PICA skinning. Gameplay and morph consumers still use CPU vertex
outputs. The deployed New build remains 7b980b145315; Old remains 4f58d475c8f7.

The candidate borrows live source vertices and weights for the duration of one
synchronous skin call. It uses the existing two scratch slots only where an
output write would overwrite a prefetched next input or current weights. It
retains no pointers after return and introduces no owner directory or allocation.
Disjoint output uses memcpy; partial overlap uses memmove; identical source and
destination needs neither. The original transform and publication order remain.

Pinned Melee's source_snapshot_cache.h requires immutable borrowed inputs and
acknowledged reader release. This adaptation is synchronous, not permission to
modify renderer-owned data. Reconstructed 7b980b145315 model.c joins readers for
all output chunks before any skin writes, when model resource tracking is active.
That prefix must stay intact. mm.c's conditional DCFlushRange publication policy
must also remain. The experiment's callback does not establish those guarantees.

Validation so far:

- Exact source transform and port saturation: 1,920 full-buffer and ordered
  publication comparisons, six alias cases and sixteen scales, ASan/UBSan pass.
- Removing any of three required snapshots fails the actual-kernel tests.
- ARM11: 102 comparisons using 51 actual asset chunk layouts, synthetic matrices
  and weights, identical outputs. Inclusive copy/transform instructions fall
  3,992,528 to 3,717,748 (6.88%). Disjoint cases improve 4.19%; in-place 9.58%.
- First version used memmove for all output copies and regressed disjoint cases
  8.96%; its report is retained as arm-v1-result.json. Do not promote that version.

These are not hardware cycles or FPS. Cache publication and reader fences are
modeled as no-ops; live descriptor decoding, metadata/output separation, caller
ownership, whole-frame costs and code-size effects need integration validation.
The ARM fixture consumes an already decoded chunk table, so its instruction
reduction must not be presented as the cost of the final game adapter.

Reproduction: tools/test_cpu_skin_live_views.py --actual-kernel,
tools/test_cpu_skin_live_views_guards.py, tools/test_cpu_skin_live_views_arm.py.
Evidence: artifacts/research/pica-scene-rewrite/cpu-skin-live-views-test/.
tools/reconstruct_live_skin_source.py verifies the archived adapter inputs and
reconstructs model.c, mm.c and objprint_dolphin.c without modifying root sources.
Its generated sources have not yet been matched to linked DWARF checksums.

## Complete game adapter gate

`cpu_skin_live_game.py` now composes an isolated adapter into the reconstructed
model TU. It decodes the relocated live descriptors, rejects output overlap with
descriptors, offsets, model metadata or the matrix span before any mutation,
preserves the existing reader-join prefix and retains the original cache-flush
condition. Exceptional layouts use the original CPU staging function. This is
CPU consumer compatibility, not restoration of a retired graphics backend.

The complete TU compiles. The skin entry grows from 688 to 2,340 code bytes
(including compiler-inlined handling). `test_cpu_skin_live_game_arm.py` executes
both actual compiled entry points with the original mm.c copy helper bodies,
51 asset layouts, two source/destination modes and resource tracking off/on.
All 204 comparisons match complete output buffers and join/flush/wait counts.
This fixture uses movie state 1; it does not validate all movie states, physical
cache behavior, live game ownership, exceptional metadata aliases or rendering.

Inclusive instruction counts: 8,194,258 to 8,055,140, a 1.70% reduction in this
fixture. Separate buffers regress 0.93%; in-place improves 4.32%. The earlier
6.88% result excluded descriptor decoding and admission and must not be used
as the game-path gain. The adapter is **not integrated into an app or deployed**.
Next resolve the disjoint regression without dropping ownership protection,
then assess actual whole-frame cost; no hardware probe is justified yet.

Evidence: `cpu-skin-live-game-test/{compile,arm-result}.json`. The first compile
failed on a missing attract_movie declaration; the adapter now includes its
original header. Root game archives and deployed app files were not changed.

## Direct destination experiment

The next candidate transforms disjoint source vertices directly into destination
vertices and copies only untouched prefix/suffix padding. Identical buffers stay
in place; partial overlap still uses protected memmove. No ownership guard was
removed. The 1,920 actual-kernel sanitizer comparisons pass, and five mutants
(three missing snapshots, missing prefix, missing suffix) are rejected.

Complete compiled entry: 2,440 bytes. All 204 ARM comparisons still match outputs
and notification counts. Instructions total 8,065,416 versus 8,194,258 original:
1.57% fewer, slightly worse than the preceding adapter despite fewer copied bytes.
This is not enough evidence to select the new path or claim a hardware gain.
The preceding full-TU source/report are retained as candidate-v1.c and
arm-v1-result.json; views-v1.h preserves its helper.

The existing deployed-build palace profile contains only eight blend calls and
192 transform calls (324,576 transform encounters). This work cannot plausibly
close the overall performance gap on its own. Stop tuning this copy mechanism
without captured memory-cost evidence or a materially different ownership design.
Keep the candidates local; prioritize the larger submission/particle costs using
the existing complete-frame profile instead of another unchanged boot.
