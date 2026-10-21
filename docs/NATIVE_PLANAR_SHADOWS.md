11 October: the receiver/pair path is deployed in New normal86b336c4bfa5 and has a confirmed cave regression. See [current investigation](SHADOW_REGRESSION_20261011.md); historical not-deployed statements below are superseded. No repair deployed yet.

11 October local receiver-depth candidate: see [bounded receiver depth](NATIVE_SHADOW_RECEIVER_DEPTH.md). Adds a measured extra native geometry pass; not deployed or physically accepted.

11 October: a local, not deployed, correction restores original per-object
light direction/smoothing and projected-caster alpha state. Read
[NATIVE_SHADOW_DIRECTION.md](NATIVE_SHADOW_DIRECTION.md) for scope and measured
cost. Softness, finite/multiple receivers and full visual acceptance remain open.

10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native planar silhouette candidate

10 October source shadow-mesh audit: all 829 unique original models were read
without modifying assets. Only 24 contain a separate shadow display-list group.
Across those 24, all 56,557 oriented position/bone-binding triangles are identical
to their visible counterparts; none supplies a cheaper topology. The shadow
packets strip unused attributes (direct RGBA4 plus position/matrix binding), not
triangles. Replacing current geometry with those lists alone therefore cannot
claim a triangle reduction. The source specialized shadow path also disables
alpha testing and does not sample caster textures; adding generic cutout handling
would not by itself reproduce that source path. Other shadow callback families
still need separate analysis. Tool: `tools/audit_original_shadow_meshes.py`;
private report: `artifacts/research/pica-scene-rewrite/original-shadow-mesh-audit/report.json`.
The compact, load-prepared shadow plan is now implemented locally and checked
as described below. It combines material boundaries without changing topology.
Morphing models retain their existing native live geometry, not the compact copy.

Local prototype, not deployed or accepted. Existing soft circular contact
shadows remain the installed approximation. Original projected silhouettes,
soft filtering, clipping across receivers and self-shadowing are not complete.

The candidate projects the already animated, resident PICA model onto a live
collision plane. CPU work is one receiver query and a 3x4 camera composition;
PICA performs the existing skinning and vertex transformation. It does not call
the game's actor renderer again, build vertices, scan GX commands, allocate a
render target or copy a generated image. The original directional shadow
offsets supply projection direction. Grazing/invalid directions use the contact
footprint instead of producing an unbounded projection.

The existing D24S8 frame target provides one stencil bit for the silhouette
union. A passing shadow fragment writes that bit; further projected triangles
at the pixel cannot darken it repeatedly. The frame's existing clear resets it.
Depth is tested, not written. Stencil state is disabled on return, and the
native material/shader caches are invalidated at the actual handoff. A command
buffer split republishes the shadow pipeline and live palette. Native resource
use accounting and retirement remain with the existing owner, following the
pinned Melee lifetime model. This stencil adaptation is new and requires
physical validation; the Melee reference does not prove it works on hardware.

This is a hard, single-plane silhouette, not the original shadow algorithm.
It does not clip to the finite ground polygon, project across walls or multiple
ground planes, handle moving receivers, honour texture cutouts in the caster,
filter soft edges, or self-shadow. Overlapping casters share a
screen-space stencil union and do not accumulate opacity. These limitations
must be judged in captures rather than reported as full shadow parity.

GROUND SHADOW remains the master switch on page 6. SHADOW SHAPE on page 7
selects this candidate instead of circles; DISTANCE FOG stays on that page.
SHADOW SHAPE is opt-in on both models, and existing recipes/settings retain
their exact meaning. No new worker exists: the audio worker uses New core 2,
and the candidate avoids software raster work rather than assuming two idle
cores. Additional CPU jobs require measured scheduling and immutable source
lifetimes before moving them off the main thread.

The new mode adds no shader instructions, uniforms, GPU vertex allocation,
linear allocation or render target. It does add a second submission and
skinning/raster pass for each caster. Test that cost before enabling by default.
Host ASan/UBSan: 46,381 double-precision ray/plane and camera comparisons pass,
plus invalid/grazing rejection. Those tests do not prove rendered correctness.
Exact console replay build 27cb88b94462 is archived in planar-shadow-v1-builds.
Four serial exact-CIA-content Azahar checks passed state assertions, log drain,
save preservation and owned-process exit. Captures at retrace 1410 were inspected:
Fox/mushrooms in the cave, Krystal and particles in the palace. These exercise
animated casters, but do not establish all poses, transitions or physical safety.

| Azahar scene/profile | Shadow mode | FPS | Scene preparation ms | Native submission ms |
|---|---|---:|---:|---:|
| Cave/New | Circles | 59.8312 | 3.570744 | 5.221922 |
| Cave/New | Silhouettes | 59.8313 | 3.578201 | 5.506571 |
| Cave/Old | Silhouettes | 52.0030 | 3.532577 | 5.527953 |
| Palace/New | Silhouettes | 59.8312 | 2.006853 | 4.254688 |

The matched New cave adds 0.284649 ms submission work, 25.8 draws and 12,264
indices per frame versus circles. The earlier Old receiver-v2 circles check
was 52.9104 FPS, scene 3.475290 ms, submission 5.239354 ms; this is a prior
candidate comparison, not a same-binary toggle pair. No measured observation
interval exceeds 250 ms in these four checks. Do not infer hardware GPU time
from Azahar: it does not emulate the physical memory/vertex/raster bottlenecks.

Peak cave normal heap: New 48,078,912/71,303,168 bytes, Old
40,738,880/50,331,648. Minimum free linear: New 19,474,048/25,165,824,
Old 4,793,984/10,485,760; initialized VRAM free 4,858,880 on both.
Process commitment New 103,120,896/130,023,424, Old 67,469,312/83,886,080;
measured main stack 72,836/524,288. Palace New heap peaks at 48,149,560.
Memory headroom is retained. The original cave receiver feature budget still
fails (roughly 1.8 ms scene addition versus shadows disabled), so this is an
opt-in partial effect, not a default-on or overall performance acceptance.

Evidence: `artifacts/research/pica-scene-rewrite/planar-shadow-test/` contains
the host check and four replay analyses including memory. Runtime folders start
`artifacts/native/planar-shadow-v1-`. No console was updated for this candidate.

V2 replay5466c3a87fb2 applies the existing distance-fog stage to the black shadow
colour, preserving opacity. This retains the receiver's fog contribution after
alpha blending. It adds no shader, target or allocation. The actual fog owner/LUT
host check passes, as do analytic blend invariants. Final New cave exact-CIA
check passes: 59.83184 FPS, scene 3.592915 ms, submission 5.524021 ms, unchanged
memory; capture1410 inspected. Evidence planar-shadow-v2-cave-on-new. v1's four
checks remain evidence for unchanged projection/stencil behavior; v2 has not
been retested on Old or physical hardware. The whole shadow budget remains open.

Latest local replay6b4ec0a84e3a includes the load-time mask index and exact
stationary-query reuse documented in NATIVE_SHADOW_RECEIVERS.md. The shadow
entry/exit now retains the shared resident actor program. Only material stages
0..3, buffer selection and shadow raster state change; fog-owned stage5 remains
with its owner. Material identity is invalidated before/after the pass so the
next actor/world draw cannot reuse the overwritten combiner. Actual HUD/GS,
target and command-buffer handoffs retain their normal full invalidation.
This follows the pinned Melee state-ownership principle; no Melee GX translation
path or CPU vertex processing was added. The shadow still has an additional
PICA geometry pass and is not free GPU work.

The actual publication block and entry/exit functions pass96 ASan/UBSan owner
cases. Mutants that clear the fog stage or fail to invalidate material identity
are rejected. Serial exact-CIA checks on New and Old both pass replay state,
save preservation, drained logs and owned exit;1410 captures inspected on both.
Scene/submission timings are1.986869/5.477235ms New and1.958688/5.494077ms Old.
Azahar cadence is59.83122/56.36915FPS. New full-pipeline publications fell from
6943 to1940 in the complete run; submission saved only0.040ms in the measured
window. Do not describe that small change as a physical FPS fix.

Against the preceding same-architecture shadowsOFF check, added New CPU work is
about0.58ms, still above the0.5ms feature budget. Normal/linear/VRAM use is
unchanged from result-cache v1, with memory headroom retained. Shadows remain
OFF by default on both models; full fidelity and physical GPU acceptance remain
open. Evidence: planar-handoff-v1-builds, planar-handoff-test/result.json and
planar-shadow-test/planar-handoff-v1-cave-on-{new,old}.json. Not deployed.

## Compact persistent shadow inputs (local, 10 October)

Replay `f06f317f3d25` prepares shadow-only geometry at the asset load boundary.
The 24-byte vertex retains exact position, skin weights/bindings and bind offset;
normal/UV/colour input streams are unnecessary for this constant-colour pass.
Consecutive material sections combine only within the same position format,
22-bone palette and bounded vertex/index counts. Triangle order and winding are
unchanged. Source morph targets exclude an asset from compact preparation; their
shadows keep the live native geometry. Modified source geometry invalidates its
compact plan. None of these cases reconnects the retired GX/CPU renderer.

Only plans that reduce draw count allocate storage. They share a 512 KiB linear
and 64 KiB CPU cap, the existing total scene cap and allocation reserves. The
owner flushes each immutable upload once and uses its existing GPU retirement
fence before freeing it. Optional plans yield to essential scene geometry under
memory pressure; shadows continue through the ordinary native geometry. Plans
are prepared even with shadows disabled, so their memory cost does not disappear
when the graphics toggle is OFF. Texture pressure is not a separate eviction
trigger. This bounded cache is a performance/memory tradeoff, not free storage.

The implementation follows the pinned Melee renderer/async-queue ownership and
cache-publication rules; compact palette compilation is a Foxhollow adaptation.
Each frame still composes live bone matrices and submits the shadow pass. Fixed
attribute/combiner descriptions are initialized once. PICA still processes all
shadow triangles; no CPU rasterization, extra worker, target or shader instruction
was added. Audio already uses New core 2; this does not assume two idle cores.

Actual-compiler equivalence checks cover all 835 prepared actor assets: 1,894
source sections become 1,040 compact sections, retaining all 328,012 triangles
and exact ordered position/bone/weight/offset bytes. All 2,505 malformed/capacity
cases pass. The 9,651,104-byte whole-collection statistic is not a resident
allocation. Actual owner code passes 100 ASan/UBSan load/retire cycles, delayed
GPU-reader checks, allocation pressure and morph exclusion. Removing the retire
fence makes the test fail. Shadow material/handoff checks pass 96 cases; fog and
material-invalidation mutants fail as required.

| Serial Azahar cave | FPS | Scene ms | Submission ms |
|---|---:|---:|---:|
| New, previous fur-v8 build, silhouettes ON | 59.83122 | 1.979301 | 5.566235 |
| New, compact v2, silhouettes ON | 59.83122 | 1.989988 | 5.507212 |
| New, same compact v2, ground shadows OFF | 59.83122 | 1.800208 | 5.143694 |
| Old, compact v2, silhouettes ON | 56.26082 | 1.954206 | 5.521505 |

The matched New ON/OFF CPU increment is **0.553298 ms**, still above the
0.5 ms feature budget. Do not enable by default or stack more shadow features
without addressing that budget. The compact-vs-prior submission reduction is
only 0.059023 ms. Complete-run New shadow draws fall 23,595 to 15,015 while
5,005 casters and 8,790,210 indices remain identical; measured timings above use
the replay window, not whole-run counters. That is a draw-count reduction,
not proof of a physical GPU/FPS improvement. Old Azahar timing is particularly
unrepresentative of its physical CPU throughput.

Both profiles pass state/save assertions, log drain and owned-process exit.
Both ON captures at retrace 1410 were inspected: Fox, ladder, mushrooms, world,
HUD and silhouettes remain visible. These do not establish every pose/receiver
or eliminate the documented finite-receiver/softness/moving-receiver limitations.

Peak normal heap: New 48,122,048/71,303,168 B; Old 40,777,856/50,331,648 B.
Minimum free linear: New 18,970,496/25,165,824 B; Old 4,290,432/10,485,760 B.
Initialized free VRAM is 4,858,880 B; measured main stack 72,836/524,288 B.
Process commitments are 103,165,952/130,023,424 and 67,514,368/83,886,080 B.
Shadow plans peak at 255,552 B linear, with 195,024 B linear and 1,800 B CPU
still resident at shutdown. No allocations were declined in this scene.

Evidence: `compact-shadow-test/{compiler,lifetime,lifetime-mutant,runtime-comparison}.json`,
`planar-handoff-test/result.json`, and `artifacts/native/compact-shadow-v2-*`.
Builds are local only; no remote files or saves changed. Existing GROUND SHADOW
and SHADOW SHAPE controls apply, default OFF on both hardware models.
