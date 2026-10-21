## 12 October: direct source transforms (V7, experimental)

V7 consumes the original block transform directly instead of recovering it through the camera inverse. Matrix, publication, and serial New ON/OFF/Old ON coverage checks pass; GPU code and geometry counts are unchanged. Extra static storage is 6,208 bytes. New emulator submission improves only 0.0133 ms versus V5, and the same-binary shadow ON/OFF cost remains 0.7195 ms against a 0.5 ms budget. This is not a substantial performance win, physical validation, or deployment acceptance. See shadow-world-test, shadow-world-owner-test, and shadow-point-runtime-test/v7.json.

# Source collision receivers (local candidate, not deployed)

The original `trackBuildBlockTriangles` with query0x81 selects marked collision
groups; `collectShadowTrackTriangles` then selects source surface classes and
`cullVisibleShadowTriangles` rejects faces turned away from the light. The
installed native renderer instead projects onto visible-world triangles. This
is a parity gap separate from the unexplained missing hardware cave shadows.

`pica_shadow_track.c` prepares the original static-block receiver superset at
load time. It preserves source signed coordinate quantization, collision Y
offset, winding, source order, surface flags and normalized face normals. The
normal is part of each persistent vertex so a future native shader can perform
face selection without a CPU triangle walk or geometry rebuilding each frame.
No runtime shader, asset owner or game hook consumes this compiler yet.

The first representation used20bytes per corner. The indexed version shares
exact position+normal values while retaining ordered index/draw ranges. This is
exact sharing, not decimation. Caller-owned output receives nothing on invalid
input or insufficient capacity. Load-only scratch is capped at1MiB, allocation
failure is explicit, and no scratch is retained. No GPU synchronization is
implemented here; integration must use the existing scene retirement owner.

`test_shadow_track_source.py` compiles the **actual original function body**
with host platform/vector adapters and compares every prepared corner and flag
across1267 original blocks plus256 flag fixtures. Coordinates and normals match
byte-for-byte. The oracle uses one whole block and expanded output capacity:
it tests the prepared superset, not the original per-caster1200-triangle cap,
finite bounds, dynamic object receivers or physical float behavior.

The actual assets contain477273 admitted triangles. Indexed GPU storage totals
27,525,552bytes across the collection; largest block246,176bytes. Maximum
transient compiler scratch730,060bytes; largest draw metadata1120bytes. These
are static collection/block sizes, **not scene residency or frame costs**.
Original visible-world receiver batches total about13.8MB across a different
admission scope. Fewer triangles do not yet prove a memory/bandwidth win.
Do not integrate indiscriminately or claim a speedup from these counts.

ASan/UBSan covers1000 load/empty/negative-coordinate/capacity/index/group cycles,
including atomic rejection. Strict ARM compilation passes. The first oracle
attempt mistakenly extracted a forward declaration; corrected to the function
definition before obtaining results. No application build/deployment changed.

Integration still needs bounded resident ownership, a face-selecting native
shader, live source receiver policies/transforms, mutation retirement, collision
versus visible-depth handling and matched scene memory/GPU/CPU evidence. Existing
hardware missing-shadow investigation remains active independently.

Evidence: `artifacts/research/pica-scene-rewrite/shadow-track-source-test/result.json`
and `shadow-track-test/result.json`. Original game/assets/saves are unchanged.

12 October local additions (not integrated): a 26-instruction PICA program
performs the original face test on flat normals, rejecting all three corners
together. The actual original CPU function and Azahar interpreter agree for
65,536 face decisions and 5,505,024 output lanes; three deliberately incorrect
variants fail. This does not establish physical GPU correctness.

The bounded raw big-endian load adapter reproduces all 1,267 source blocks;
maximum combined scratch is 804,868 bytes. A separate load-time route compiler
combines source flag-8/flag-4 ordered subsets and computes bounds. Full subsets
alias existing indices. Sanitized capacity/ordering/atomic rejection checks and
strict ARM compilation pass; a whole-asset route audit is still outstanding.

Live ownership, finite and dynamic receiver policies, source-coordinate depth
alignment and runtime cost remain unimplemented. In particular, block transforms
already include collisionYOffset while prepared collision positions include it
too: integration must compensate the view translation to avoid double offset.
Evidence: shadow-track-{shader,raw,raw-source,routes}-test/result.json.

12 October load ownership is now integrated behind an inactive renderer flag.
`platform_scene_shadow_track.h` prepares one immutable PICA allocation per block
at registration, including both source surface selections. It publishes once
through the existing cache owner and releases through the parent retirement
boundary. Main scene accounting includes every resident byte. Caps are 1 MiB
linear, 64 KiB metadata and 2 MiB transient load scratch, in addition to existing
headroom guards. No geometry compilation or SD reads occur during drawing.
Mutation notifications invalidate the plan until an explicit update path exists.

The full route audit now passes all 1,267 blocks and 256 flag fixtures. It reduces
10,631 source groups to 1,063 nonempty block draws across the collection, retaining
all 477,273 ordered triangles. Alternate routes alias existing indices or are
empty in these assets; they add no index storage. Packed geometry totals
27,542,816 bytes across the collection, maximum block 246,176 bytes. These are
not per-scene measurements or a frame-rate gain.

Private build `38255f64f19f` activates load ownership only. Its 511-instruction
scene program and game archive match the preceding stable build. Serial New
3DSX and Old 80 MiB CIA headless cave checks completed with drained shutdown,
preserved saves and inspected screenshots. Both built 11 plans with zero
rejections, peaking at 663,232 linear bytes. The preceding visible-world receiver
batches remain resident (196,560 bytes) and do all drawing in this diagnostic.
This therefore establishes neither a rendering fix nor a memory saving.

Both runs invalidated two plans through original terrain-animation producers.
Before switching drawing, live source positions and corresponding face selection
must be handled without stale geometry or CPU mesh reconstruction. The source
YOffset compensation, finite queries, dynamic receivers and visual/depth checks
are also outstanding. No physical test or deployment was performed.

Memory: New normal 53,897,632/71,303,168 bytes, free linear 17,702,528/25,165,824;
Old normal 39,020,384/50,331,648, free linear 3,022,464/10,485,760. Both measured
main stack 73,008/524,288 and free VRAM 4,723,712. Normal checks reserve the full
stack rather than treating the measured sample as its required allocation.
Actual-owner lifetime tests pass 100 cycles and reject missing retirement/cache
publication. Runtime/build verification: `shadow-track-storage-test/runtime.json`.

Moving-terrain implementation (12 October, local): the live plan retains an
ordered source index triple per triangle and persistent 20-byte GPU corners.
Every corner receives the same three positions and a different selector. A
separate 61-instruction PICA shader chooses the corner, computes the face test,
and rejects degenerate triangles and faces wholly outside the block. Initially
collapsed or out-of-bounds faces remain in the topology so animation can restore
them. These are source positions, not CPU-transformed/projected vertices.

The original terrain producer already computes the packed positions for game
collision. The renderer scans its prepared references only on publication,
applies the original coordinate quantization/offset/wrap, and copies changed
position lanes. Indices, selectors and reference lists stay fixed. This still
has CPU and memory traffic that must be measured; it is not free. The updater
uses the existing world-position fence and flushes the changed span. Destruction
uses the parent fence and does not reopen a frame during shutdown.

The shared raw decoder remains bounded. Resident metadata allowance increases
from 64 to 256 KiB to include source references; all existing normal-memory
headroom guards remain. Linear cap remains 1 MiB and transient cap 2 MiB. Current
live bounds are conservatively the full signed16 range, pending an efficient
moving-bounds consumer. Current code is inactive in ordinary builds and no
application containing this updater has been run or deployed yet.

The actual-source comparison passes 7,615 pose cases across 1,267 blocks and
256 flag fixtures, including raw/native preparation and moving, collapsed,
outside and restored positions. The actual Azahar VM passes 65,536 sampled face
and bounds cases and all three corner outputs, with zero observed near-zero
classification differences and no split decisions. This is sampled precision,
not physical GPU validation. Lifetime tests reject missing write/free retirement
and missing cache publication. The actual stable scene translation unit and
changed compiler units pass strict ARM compilation in an isolated header tree.

Collection-wide live GPU storage is 31,507,116 bytes versus 27,525,552 for the
static indexed representation. The live shader uses 61 versus 26 instructions.
These costs require a measured implementation choice before enabling drawing;
static specialization remains an option. Neither figure is a per-scene memory
measurement or FPS prediction. The earlier `38255f64f19f` application only
validated the preceding static resource owner, not this updated format.

Evidence: `shadow-track-live-{source,shader,storage,arm}-test/result.json`.
Projector wiring, Y-offset correction, finite queries, dynamic object receivers,
GPU handoff, runtime timing and physical missing-shadow diagnosis remain open.

Integrated candidate, 12 October: `shadow-track-render-v1-builds` contains shared
normal `f974e260e4e8` / CIA `27e7d6fb0965`. This replaces visible-world receiver
storage and its mutation rebatching with the live collision plans. It publishes
the source alternate-surface flag and light, removes the duplicate collision Y
translation, and supplies the separate receiver shader with its own fog uniform.
The ordinary actor and compact caster loops republish all first-draw bone rows;
the receiver's overlapping uniform slots therefore do not retain pose state.

Bounds are now computed at preparation and refreshed during producer updates,
rather than retaining the full signed16 range described in the earlier entry.
Both routes receive conservative live bounds. The full source audit and actual
owner/lifetime checks pass again. Actual instance publication passes 49,248
affine coordinate checks and rejects a duplicate-Y-offset mutation.

New 3DSX and Old80 CIA cave runs both complete, drain and preserve saves. Their
1410 captures show Fox's shadow, the cave and HUD. Each admits all11 plans with
zero missing-plan draws: 737,088B linear and67,744B metadata. Two position hooks
execute but change zero triangles; actual moving-terrain GPU acceptance is open.
The New/Old free linear minimums are17MiB/3MiB. See
`artifacts/research/pica-scene-rewrite/shadow-track-render-test/runtime.json` for
all heap, VRAM, stack, grant and timing values and exact comparison scope.

The preceding load-only diagnostic retained both receiver representations. Its
Old linear minimum was about120KiB lower, but this is NOT a memory improvement
over the installed visible-only renderer. This candidate uses about528KiB more
receiver linear memory than the removed visible batches in this cave.

Logged cave cadence stays59.83FPS in New Azahar; Old changes53.90 to55.01FPS.
These runs have matching saves/settings/assets/input but different pose phases.
Auxiliary submission drops roughly0.37ms; it is not total renderer CPU time.
The shadow component (all masks plus capture, normalized by geometric casters)
drops1.07 to0.70ms, still above the initial0.5ms budget. GPU wait is essentially
unchanged. No physical speed, complete visual parity or hardware repair claim.

Static map receivers are connected. Finite source queries, dynamic object
receivers, physical missing-shadow diagnosis and broader visual acceptance remain
open. The build has not been deployed. Source game archive comparison finds only
the shadow adapter object changed; the main511-instruction program is identical.

12 October finite-query work (local candidate only): the hardware shadow repair
now uses separate UI43/clear9/projector21 entries. Preserve that tested 73-entry
instruction allocation when integrating collision receivers; the older candidate
above must not restore the failed combined boolean program. The latest accepted
normal/replay snapshots are `prepared-buffers-normal-v1-builds` and
`prepared-buffers-v1-builds`; neither includes the finite-query work below.

The original collision query rejects by group bounds and two cell-mask bytes as
well as triangle bounds. A read-only audit of all1,267 private source blocks finds
all477,273 admitted static triangles contained by their group bounds, covered by
their cell masks, and no noncontiguous mask bytes or signed16 position overflow.
An independent comparison executes the original `trackBuildBlockTriangles` over
60,816 finite boxes:5,613,837 admitted triangles match native static filtering in
coordinates, normals, flags and order. These are sampled queries; the separate
containment/mask audit establishes the static filter redundancy more broadly.

This does NOT justify omitting the original constraints after animation. A test
moves a triangle into a new cell while retaining its source mask: the original
rejects it while unconstrained triangle bounds admit it. The current WIP live
format loses those authored constraints. It must be redesigned before accepting
the finite path for animated terrain. Dynamic object receivers remain absent.

The candidate GPU finite-box filter now reduces six inclusive axis comparisons
in vector lanes, using55 instructions instead of the preceding67-instruction
draft. No extra vertex bytes, draw calls or CPU triangle work were added by this
shader change. Azahar's actual interpreter passes65,536 face/finite-box cases,
5,505,024 projected output lanes, and six deliberately broken shader variants are
rejected. This is sampled shader behavior, not physical precision or timing.
Whole-triangle rejection is identical across all three corners. The shader is
separate from the unchanged511-instruction scene program.

The instance owner now includes block origin in duplicate detection and retains
the single collision Y-offset correction. Its actual extracted functions pass
49,248 affine coordinate checks under sanitizers; missing-origin and duplicate-Y
mutants fail. No complete application was built from this WIP. In particular,
the old61-instruction integration helper must remain rejected until the new query
publisher and authored-filter policy are connected together.

Evidence: `shadow-query-filter-audit/result.json`,
`shadow-finite-source-test/result.json`, `shadow-track-finite-shader-test/result.json`,
and `shadow-finite-project-world-test/result.json` under the renderer research
directory. Melee's shader output verification, separate shader storage checks and
texture retirement were consulted; existing lifetime/cache ownership is unchanged.

12 October compact point implementation (not yet wired to gameplay):
`fh_pica_shadow_points.h` / `pica_shadow_points.c` prepare one32-byte record per
source triangle. It contains three quantized source positions and six immutable
limits derived from the group bounds and cell masks. Collision Y offset remains
a block parameter. This retains the distinction between unwrapped query bounds
and signed16 coordinates used for emitted geometry and face normals. Sparse,
noncontiguous masks are rejected atomically; all admitted masks in the actual
1,267 blocks are supported. Zero-mask triangles remain absent as in the source.

The new compiler/update function matches the actual original query over60,816
queries across six poses per block, including translation outside authored cells,
deformation, collapse and restoration. The previous moved-triangle counterexample
now correctly emits zero triangles. The test compares3,643,219 admitted triangles
in source order, including coordinates, normals and flags. The native update only
writes changed position lanes at the existing producer boundary; bounds, source
references and topology remain immutable. There is no per-caster CPU triangle
query, normal calculation, transformed-vertex upload or allocation in this kernel.
The gameplay owner still needs to connect it and enforce retirement/publication.

The paired `pica_shadow_points.v.pica` / `.g.pica` programs classify each triangle
once on the vertex processors, skip projection when rejected, and project all
three positions when admitted. The geometry stage unpacks the prepared outputs
and emits one triangle with original winding, or none. Shared storage is133
instructions, separate from the main511-instruction scene program. Seven output
registers pass from VS to GS. All output lanes are initialized on each emission.
The unused fragment-lighting view varying is explicitly zero; this path requires
the projector material to keep fragment lighting disabled, as it currently does.

The actual Azahar interpreter/emitter executes32,768 original query/face cases,
including signed16 Y wrap, authored lower/upper limits and moving triangles.
It compares325,584 emitted output lanes against the existing projector shader
(with the unused view varying intentionally zero). Six mutations are rejected:
missing authored lower/upper limits, missing signed wrap, changed winding,
missing fog, and disabled rejection. The first random-only test did not detect
the missing-lower-limit mutant; targeted disjoint-cell cases were added, and both
lower and upper constraints are now independently exercised. A first attempt
used VS CALLs unsupported by the conservative output validator; the three fixed
transforms are now inline, retaining the existing validator's requirements.

Sanitizer checks cover all65,536 masks (1,807 accepted including empty masks;
63,729 unsupported sparse masks rejected without writes), insufficient capacities,
invalid references and10,000 changed/unchanged producer updates. Strict ARMv6K
compilation passes. `check_pica_geometry` now accepts an explicit emission-count
contract; its existing default remains four, verified against the previous glow
shader. The point contract permits zero or three, not arbitrary emission counts.

Point records total15,272,736 bytes across the source collection, before index
routes, versus31,507,116 bytes for the preceding live vertex/index format.
These are not resident measurements. The intended existing1MiB receiver cap and
256KiB metadata cap must still be enforced by the new owner. Geometry shader
execution changes vertex-core allocation and can become its own bottleneck:
smaller storage and instruction counts are not evidence of a console FPS gain.
Queue handoffs, real rasterized images, per-scene RAM and ON/OFF timings remain
required before this path can replace the stable receiver renderer.

Evidence: `shadow-point-{source,shader,storage}-test/result.json`. Neither shared
application build nor either console installation includes this point path yet.
The separate finite-query WIP also now excludes a block when query minimum is
exactly its far640-unit edge, matching source grid-floor selection; clamping alone
would wrongly retain that block. That integration boundary still needs a runtime
check. The pending toggle/sky deployment remains independent and not ready while
the existing hardware sky-probe result is missing.


## Integrated point candidates (12 October)

12 October POINT RECEIVER INTEGRATION CANDIDATES; NOT ACCEPTED OR DEPLOYED.
Goal ACTIVE/INCOMPLETE. Current turn PROGRESS: bounded raw decoder, native point
owner, source routes, real application integration and New ON/OFF runtime.
Latest accepted stable remains prepared-buffers-normal-v1/replay-v1; BOTH normal
deployments still pending actual sky fix. Do not build unisolated WIP.
User authorized both-console normal deployment when toggle AND sky fixed. One
bounded read-only sky collection results-20261011-224444-223 found no probe.log;
no remote changes. Do not keep polling unchanged missing result. Installed
normal eb521d9e988c/b9b63688dbd6 and New sky probe41929343edf6 unchanged.

New point raw decoder/routes/owner: 1MiB linear,256KiB metadata,2MiB scratch caps;
32B per triangle, direct normal route, optional indexed alternate subset.
Separate unwrapped query and signed16 wrapped visible bounds. Producer changed
positions only, parent retirement before writes/free, flush before publication.
100 actual-owner lifecycles ASanUBSan incl delayed GPU readers/allocation failure
PASS; missing free/write fence and flush mutants fail. 2040 route cases PASS.
All actual assets supported by preceding source audit; runtime animated terrain
still not covered. OFF avoids shadow queries/draws, retains resident plans.

Isolated tools/build_shadow_points_v1.py rebuilds shadow_dolphin/tex_dolphin,
archives451 source files (444 stable+7new). Preserves hardware73 shadow entries
and main511. V1 failed missing angleToVec2 declaration; fixed actual trig header,
error preserved. V2 replay77b101d9980f/CIA480d097809de, V3 replay50ecc7901af7/
CIA7c4b903ec5e0 BUILT NOTDEPLOYED; no normal point build. Builder restores source
and archive finally; source snapshots are candidate-only, not new stable.

V2 New ON/OFF PASS50.24/47.72hostsec; save/drain/ownedexit PASS,1410 images
inspected. ON scene1.940253/submit5.884380ms; OFF1.876230/5.230059: added.718344ms
CPU exceeds.5ms gate. Both~59.831 AzaharFPS are pacing, NOT console throughput.
11 plans357248Blinear/68184Bmetadata,0declines/missing; old collision candidate
737088Blinear. ON715casters/5005receiverdraws/11688820 submitted POINTS; log field
indices counts points, auxiliary vertex workload counts points*3 upper bound.
OFF queries/draws zero. Two producer hooks,zero changed triangles in cave.

V3 prepares5SHORT3 attributes/32B buffer layout once and caches projector matrix,
no per-draw AttrInfo/BufInfo construction. Linked ARM SDK360962 differential
checks+5mutants PASS. New ON runtime PASS49.93sec,1410 image inspected; scene
1.944191/submit5.874632ms. Only projector source differs fromV2, all794 game
object payloads equal (archive headers differ), main shader and CIA80/124 same.
ReuseV2 OFF explicitly CROSS-BINARY: added.712534ms,stillFAIL .5ms; .005810ms
change vsV2 too small to claim gain. Do not repeat unchanged OFF/Old boots to
manufacture acceptance. Next redesign remaining shadow work before promotion.
V3 Newheap48465552/71303168,linearfree18439168/25165824,VRAMfree4723712,
mainstack73008/reserved524288,process103305216/130023424. No Old point runtime.
Evidence shadow-point-runtime-test/{new,v3-new}.json; owner/routes/ARM tests.
All owned processes TERMINAL incl v3 build(handle lost;scope/log/process proof),
New runtime37136. No deployment, no hardware or complete-shadow-parity claim.



## Immutable ordered ranges (12 October)

12 October IMMUTABLE SHADOW RANGES INTEGRATED LOCALLY; STILL EXPERIMENTAL.
Goal ACTIVE/INCOMPLETE. Previous goal turn PROGRESS(point owner+V3runtime).
Current turn PROGRESS(source-bound tree/compiler/actual source comparisons,
bounded owner integration/shared replay V4/runtime and measured cost). No FTP,
remote writes, new hardware requests, normal build or acceptance promotion.
Both requested normal deployments still await sky repair/existing probe result.
Accepted stable remains prepared-buffers-normal-v1/replay-v1. V4 is NOT stable.

New fh_pica_shadow_ranges.h/pica_shadow_ranges.c: immutable authored-bound
balanced tree,16B nodes/16-point leaves, one ordered contiguous span per route.
No reordered triangles/additional draws, CPU geometry, GPU resource or worker.
GPU retains exact per-point source query/face/position tests. Immutable tree
does NOT depend on moving vertex positions; signed collision Y subtracted in
query, never wrapped. Handles source inverted authored limits conservatively.
Actual source oracle1267blocks/60816queries/six poses gives3643219 ordered
triangles exact; moved-cell rejection retained. Broad audit22909105->12980246
submitted points(-43.34%); not a scene timing forecast. Largest single-route
tree16368B in actual assets. Pure-source oracle builds tree per query solely
for isolation; production owner builds once at load. All alternate/direct
ordering cases covered separately by sanitized C test.

ASanUBSan16356200 inclusion checks + exact leaf boundaries/signed offsets/
capacity/index/alias rejection PASS; three offset/boundary mutants rejected.
Strict ARMv6K compile PASS. Static stack build56B/edge48B/query72B; max13 tree
levels -> compiler-frame bound build760B/query696B, not runtime stack measure.
First unit test used unsigned i*80+y causing test-coordinate wrap; corrected
signed float expression, production unchanged. First oracle wrapper had two
Python quoting failures before executing C, fixed; original checks not weakened.

Actual owner now appends aligned trees after references, counts them under the
existing256KiB metadata cap; normal/alternate full routes share a tree. Empty
routes have no tree. 1MiB linear/2MiB scratch caps unchanged, retirement/flush
unchanged.100 lifecycle tests incl allocation failures/delayed GPU readers,
immutable tree after moved positions and missing fence/flush mutants PASS.
Updated old test metadata-size assertion to include actual tree/alignment.
Consulted pinned Melee renderer retained-state invalidation/publication; this
change leaves SDK target, bind, fence and cache ownership untouched.

Shared replay3c967d6a3090/CIA7657a85bc7a8 BUILT NOTDEPLOYED, shadow-points-v4-builds.
453 manifest files; V3->V4 changes exactly5 native files. All794 game objects
byte-identical, main511+point133+hardware73 shaders unchanged, CIAOld80/New124.
build_shadow_points_v1 now only accepts versionv4; old versions immutable.
Full baseline444 isolation/restoration retained, explicit9 additional native
files tracked. Other older builders must handle these new files explicitly.
Do not blindly build worktree containing unrelated VRAM/finite-shadow WIP.

New cave ON runtime PASS49.92hostsec/status1/saves/drain/ownedexit. 1410 image
inspected: world/Fox/HUD/shadow retained, fog phase differs.715 silhouette
casters+4290 authored masks/5005 projection draws unchanged fromV3. Point input
11688820->9493770(-18.779%),5005 range queries/117260 node visits. No missing
plans/declines. Resident linear357248B unchanged; metadata68184->104460B.
Scene1.944065/submit5.928618ms vsV3 1.944191/5.874632: CPU+.053860ms. Earlier
.5ms shadow gate remains failed; do NOT claim accepted or a physical speedup.
No fresh OFF/Old run: this failed CPU gate does not justify repeating unchanged
long boots. Cadence59.831224AzaharFPS is pacing, not physical performance.
Newheap48506200/71303168,linearfree18439168/25165824,process103309312/130023424;
see v4-new.json for exact VRAM/stack. Added memory is normal RAM only.

Initial V4 runtime preflight failed for disk space BEFORE emulator. Completed
height-fog-v2 ELF/map/archive and prior completed run ELFs losslessly NTFS
compressed with SHA readback; no paths/assets/saves deleted, guard kept.
Evidence shadow-range-{test,source-test},shadow-point-range-owner-test and
shadow-point-runtime-test/v4-new.json. All owned sessions TERMINAL21903(owner),
37919(build),78205(runtime),82487(source). No live emulator/build/FTP work.
Next reduce repeated per-caster preparation/binding and static-mask query work
without weakening invalidation/lifetimes; weigh GPU input reduction against CPU
cost using actual hardware only when a substantive candidate is ready. Sky
remains independently pending; do not repeat missing-result polls unprompted.



## Query reuse and Old 3DS admission (12 October)

12 October SHADOW QUERY REUSE + OLD RECEIVER ADMISSION REPAIR LOCAL.
Goal ACTIVE/INCOMPLETE. Previous turn PROGRESS(range tree/V4runtime); current
PROGRESS(exact bounded reuse/V5New, discovered Old missing receivers, repaired
scratch admission/V6Old and coverage gate). Not blocked; not full acceptance.
No FTP or console changes/requests this turn. Both authorized normal deployments
still wait actual sky fix/existing probe result. Stable release remains prepared-
buffers-normal-v1/replay-v1; installed normals eb521d9e988c/b9b63688dbd6 unchanged.

Range cache belongs to each load-prepared point plan: eight exact keys with
tree identity+six query float bit patterns+collision Y; hash is only a prefilter.
324B ARM cache per nonempty plan, counted in existing256KiB CPU cap; no new GPU
memory/worker/per-frame allocation. Zero at owner creation, free with owner;
immutable authored tree survives live position updates.111005 sanitized cases
cover real FNV collision(2,342 vs16,1), eight keys, evictions, offsets, alternate
trees and reused addresses. Missing full-key/tree comparison mutants fail.
100 actual-owner lifecycles use poisoned allocations; missing cache initialization,
free/write fences and publication mutants fail. ARM strict compile/cache stack88B.
Pinned Melee retained-state invalidation reviewed; SDK binding/fences untouched.

V5 shared replay4d137338a82e/CIA0e30e483f06d NOTDEPLOYED. New cavePASS49.47s/state/
save/drain/ownedexit,1410image inspected.4954/5005hits(98.981%),node visits117260->
1210, submitted9493770points/5005draws/715casters unchanged vsV4;18.779% less input
than untrimmed. Submit5.928618->5.881112ms (-.047506); scene1.944065->1.918220,
phase-dependent, do NOT attribute entire combined-.073350 to cache. Cadence
59.831477Azahar, not console FPS. FullshadowCPUgate .5ms remains unmet.
Newheap48509496/71303168,linearfree18439168/25165824,VRAMfree4723712,stack73008,
process103309312/130023424.5 native source deltas vsV4,794 game objects identical.

First Old point-path V5 run completed48.05s BUT VISUAL COVERAGE FAILED:3 plans
built/8declined/5376missing-plan requests, only672projectiondraws. Do NOT use its
56.322AzaharFPS or5.654381submit as complete-renderer performance. Image inspected.
New native_shadow_gate.py rejects this exact run, accepts NewV5, and now runs
after V6 ON checks. Gate covers admission/draw consistency, not image parity.

Root admission bug: prepare_shadow_track required room for full2MiB scratch cap
before decoding any block. Old normal_reserve also enforces15% headroom/4MiB
scene metadata limit, so this rejected valid plans despite much smaller needs.
New fh_pica_shadow_track_raw_scratch validates spans and measures exact decoder
storage without allocation; owner checks that size first. Existing512KiB decoder,
2MiB total scratch,1MiB linear,256KiB metadata and global headroom caps unchanged.
Decoder output remains exact against original across1267blocks/477273assettris;
212truncation/3072bad-span and atomic measurement checks PASS. Actual owner
now succeeds with64KiB available on the small fixture; original2MiB guard would
fail. Allocation/lifetime tests PASS. Added load-only rejection-stage telemetry.

V6 replay09dc767a2b0c/CIAd49b9f66b4e7 BUILT NOTDEPLOYED, no normal point build.
shadow-points-v6-builds has453 source files;4changed vsV5 (track header/raw decoder,
scene owner include, scene shutdown telemetry). All794 game objects unchanged;
main511/point133/hardware73 shader sources and CIAOld80/New124 preserved. Builder
only accepts currentv6, isolates full444stable sources plus9 explicit additions,
includes current track header/raw decoder and restores WIP/archive finally.
Never blindbuild native VRAM/finite WIP. Candidate snapshots NOT new stable.

V6 Old cavePASS48.59s/state/save/drain/ownedexit AND coverage gate.11plans built,
0declined/0missing draws,all7 rejection counters0. Largest decoder scratch51424B.
664casters+3984authored masks,4648draws,8816592submitted/10855072untrimmed points.
CPUmetadata107740B/linear357248B.1410image inspected, shadows/world/Fox/HUD intact;
fog phase differs, not pixel identity/full visual acceptance. Scene1.903605/
submit5.904542ms;55.645232AzaharFPS is not physical throughput. Restored work makes
V5 Old comparison invalid for speedup. No fresh V6New/OFF: reuse unchanged V5New
render code evidence, not claim exact V6New runtime. Oldheap40968480/50331648,
linearfree3759104/10485760,VRAMfree4723712,stack73008/524288,process67657728/83886080.

Evidence shadow-range-cache-test,shadow-point-{cache,scratch}-owner-test,
shadow-scratch-{test,source-test},shadow-point-runtime-test/{v5-new,v5-old,v6-old}.
V5Old JSON explicitly marks rendering failure. native_shadow_gate mandatory for
shadow ON cave integration; do not accept nonblank captures with missing plans.

Disk preflights twice refused before emulator; guard retained. Completed ELF/
map/archive evidence losslessly NTFS compressed with byte verification. New
compress_completed_run_elf.py selects at most8 older terminal/drained runs with
recorded ELF SHA and actual physical size, only compresses ELF, never deletes
paths/assets/saves.16 files reclaimed361349048B this turn. Initial legacy summaries
without ELF hash now skipped; incomplete historical build dirs rejected. All
owned handles TERMINAL28260(cache),30918/40751(owner),12084/46768(build),9773/
29835/7960(runtime),95828(rawsource). No live emulator/build/FTP process.

Next address remaining shadow CPU preparation/submission: source current
fh_pica_shadow_to_eye inverts the same affine camera once per caster, then
multiplies it back through each camera*block matrix. Investigate direct live block
transforms or frame-prepared inverse without assuming rigid cameras or omitting
source map origins. Preserve measured GPU point reduction and exact coverage.
Full shadow softening/visual parity, physical GPU behavior/performance and wider
port completion remain open; no shadow or sky deployment acceptance yet.

