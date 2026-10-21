# Load-prepared rigid actor visibility

This is a local candidate on `collision-current-v2-builds`, not a deployed or
physically accepted optimization. It follows pinned Melee's homogeneous side-plane
tests and conservative margin. It does not bring back the retired draw interpreter.

The model owner prepares a 28-byte bound for each draw at load. Admission requires
all stored vertices to use the same bone slot for both skin influences. Compact
position scale and bind-tail offsets are included. Mixed bone sections remain
visible. At draw time the bound consumes the current bone/tail transform and camera
planes; only a box wholly outside a side plane can reject the draw. There is no
near/far rejection, distance LOD, position rebuilding, extra shader or geometry copy.

Bounds live after the existing immutable draw array without changing its ABI.
They share its aggregate 256 KiB plan budget, accounting and retirement. Essential
geometry still loads if optional bounds do not fit. Position mutation invalidates
the optional bounds rather than rebuilding them during animation. Alternate
instance backing, live morphs, fur and shadow passes bypass these static bounds.
After a rejected section the next visible section republishes its full palette;
the existing palette-change masks describe uninterrupted source draw order and
cannot safely bridge skipped sections.

The original prepared collection contains 835 models and 1,609 sections. 1,136
sections, representing 398,187 of 988,530 index references, qualify. This is
eligibility, not a forecast of culled work. Source checks compare every stored
position and 32 affine/plane combinations per admitted section; all 29,308
rejections have a double-precision vertex witness. Sanitizers cover invalid skin
slots, mixed influences, nonfinite data, optional allocation absence, backing
switches and 1,000 actual owner accessor/invalidation cycles. Removing invalidation
fails the test. Strict ARM compilation passes.

V2 shared replay `69103bad0d8e` / CIA `6883ef8ad7bc` changes four tracked sources,
leaving the game archive and all thirteen linked GPU programs byte-identical.
Old80/New124 profiles remain. The original-geometry moving Thorntail replay passes
state checks, save preservation, log drain and owned exit. Captures 1500/1650 were
inspected; camera/actor phase differences prevent fixed-image equivalence claims.
It performs 24,932 tests, rejecting 1,900 draws and 269,040 index references.
Rendering CPU **increases 0.082268 ms** against the previous same-recipe run. This
is not accepted as a performance win; the slightly higher overall emulated FPS
does not override the component regression.

V3 first checks whether the box center is inside every side plane. That permits
an immediate conservative keep; outside decisions still require the original
full-box test. All source rejection witnesses remain unchanged. A separate test
executes the exact linked actor vertex program in Azahar's float24 interpreter:
65,536 rigid poses, with all 49,724 CPU rejections having an actual GPU clip-plane
witness. This does not establish physical PICA correctness or performance.
V3 runtime results are recorded separately before any promotion.

Evidence lives in `rigid-bounds-test`, `rigid-visibility-owner-test`,
`rigid-bounds-shader-test` and `rigid-visibility-test` under
`artifacts/research/pica-scene-rewrite`. Snapshot builders isolate other WIP and
restore every replaced source and archive in `finally`. The first V1 builder
stopped before compilation because a snapshot-only cloud source was absent in
the working tree; optional prior-file restoration fixed that in V2. A V3 runtime
attempt stopped before emulator launch at the evidence-storage guard. Completed
old evidence was losslessly compressed with hashes retained, rather than removing
the guard or deleting assets.

No hardware test or deployment is requested while the user's consoles are
unavailable. Complete renderer parity, sustained target performance and physical
sky/shadow acceptance remain open.


V3 runtime completed in 79.55 host seconds with saves, state checks, log drain and
owned exit intact. It rejected 1,848 of 24,426 tested draws, avoiding 259,023 actor
index references (roughly 0.35% of this route's actor work). Scene plus submission
CPU is still **0.042404 ms above the baseline**, versus V2's +0.082268 ms. Different
animation/work counts prevent exact cost attribution. Emulated cadence is 52.425
versus baseline 53.332 FPS; no physical forecast or net speedup is established.
The candidate is not promoted, and no Old/console repetition is requested for it.

V3 New heap peak is 56,586,696/71,303,168 bytes; linear free 16,041,472/25,165,824;
VRAM free 4,723,712; measured stack 73,068; process 103,325,696/130,023,424. The
owner accounts optional metadata inside the existing plan cap. Differences in
heap and linear use include different texture/animation phases and must not be
presented as savings from culling. All thirteen linked shader programs and the
game archive are identical to the baseline. The shared CIA keeps Old80/New124.
