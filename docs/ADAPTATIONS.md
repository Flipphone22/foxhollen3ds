9 October native geometry-stage transition candidate (hardware confirmation pending).

Old3DS probe v2 d4d9fd4b78a7 hangs at the first particle GS -> native HUD VSH
switch in one command list. The same captured particles, original stream offset
and texture pass separately. This is reproduced with stock Citro3D and immutable
published input, without game logic, saves or our command-writer adaptations.
See console-particle-results/20261009-115333 and the confirming115807 collection.

Pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e point.g.pica,
citro3d_fix.c and its queue/cache ownership were consulted. Its point shader
shares the vertex output layout; our native HUD and particle pipelines differ.
Upstream Citro3D issue56 reports a similar GS/nonGS hardware hang, but this
does not establish the exact register fault or validate our candidate.
https://github.com/devkitPro/citro3d/issues/56

tools/geometry_stage_boundary.py adapts pinned Citro3D9f21cf7 context publication.
After actual GS drawing, a following non-GS program finalizes/queues the current
list before framebuffer or shader state changes. Multiple CPU bindings without
drawing do not cause splits; already-separated lists do not split again. Existing
GX_ProcessCommandList wrapper publishes owned world indices and particle records
before command memory, using Melee's explicit cache ownership discipline. No
simulation changes, CPU-generated particle geometry, GPU resource retirement,
presentation or explicit CPU wait is added. The existing bounded queue may drain
at capacity; added splits and those drains must be measured, not hidden.

The common native publisher is the deliberate departure from per-renderer fixes.
Counts distinguish edges, actual splits and already-separated transitions. Source
identity is included in both native and private-SDK build manifests. ARM tests
exercise GS->VSH, pending binds, repeated edges, existing boundaries and coincident
framebuffer invalidation, with a missing-boundary negative control. Cache test
now verifies native-only world/particle/cache/command publication order. Probe v3
isolates the boundary using stock SDK; do not call this a hardware fix until
that probe and a full game replay validate it. No replacement game deployed yet.

7 October local performance continuation: audited world texture reuse.

Consulted Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
port/3ds/renderer.c texture generations/fenced retirement and
texture_visibility.h image/palette overlap. Foxhollow pinned33f3079 synchronous
renderMapBlock main pass has instruction decode, visibility/light selection and
existing-frame texture getters; it does not modify image/palette bytes. Its
AttractMovie_DrawTextureCallback ends the scope BEFORE callback execution, and
reuse stays disabled for the remainder. Deferred calls and other game producers
still compare every byte. End at block exit. Explicit source/palette writes
invalidate affected entries. Narrow transaction trust is a deliberate departure
from adopting Melee global-generation trust for arbitrary Foxhollow writers.
See audit.json source identities and reviewed boundaries. No geometry, timing,
callbacks, effects, shader instructions, GPU ownership or simulation removed.

Host New/Old actual-C ASAN/UBSAN texture tests pass alignment/mutation/palette/
eviction/fences plus scope boundaries, explicit writes, unknown writes between
calls, serial wrap, disabled mode, and an unannounced in-scope write negative
control. Added check_world_texture_scope.py: inspect linked world begin/end and
scope close preceding movie callback. Archived4628 is correctly rejected. First
matrix retained old game archive; zero-hit run exposed it. Corrected by building
only main/tex_dolphin.c --update-archive before matrix. Complete game archive
e18311de4ca2ce6c39d3b935ba973047e1f4466659c7309b289ad6ba8ca00817.
CLI --texture-scope now enforces the linked guard before launching. Never count
the earlier zero-hit runs as validation of this optimization.

Current New696d771a3d57 / Old7867d2d59509; emulator a096ab2c39c5 /
Old emulator0e7b75e938aa. BSS +544bytes, text +832 versus4628; no additional
normal/linear/VRAM allocation or texture storage. Existing headroom guards and
raw-depth-readback rejection untouched. Exact console binary ran modes2/1/0
with same originalv11 pack/save/HID: all1790 rendered frames/2101retraces,
clean Select exit and balanced GPU observer28432/28424/28440commands respectively.
Mode2 verified13845 skipped comparisons, zero mismatches. Mode1 reused13871.
Images1440 in all three modes and prior4628 inspected: room floor/character/
yellow ornaments intact; asynchronous pose differences remain. No Pepper repro
or physical stability/performance acceptance is claimed.

Room renderframes1300..1499 reference/reuse: draws21566/21559, references
5414394/5432820, submit15.213930/14.899875ms (~2.06%). Whole-run texture
comparison2,834,472,064->2,726,627,264bytes (-3.80%); texture work6.649->6.432
emulator ms/rendered frame. Room interval42.623->39.108ms is one asynchronous
emulator sample, not a console speedup estimate. This is a small measured
contribution, insufficient to solve remaining performance. No repeated local
benchmarks or console request justified for this slice alone.

One first linked mode1 run hit host disk full while xwd wrote hid540; stopped
with owned cleanup. Six completed runs' raw XWDs were ZIP archived, read back
and SHA256 verified before raw duplicate retirement. Failed partial evidence
retained. Automatic verified archival now follows successful console headless
runs; PNGs/logs/builds/saves stay accessible. No console files changed this turn.
Installed4628 Pepper candidate awaits user result. Goal remains incomplete:
stable normal progression and measured physical60/steady30 not established.
All owned emulator/build sessions terminal. This turn PROGRESS (integrated
tested CPU reduction), not completion or a claimed substantial speedup.

7 October: alternate local joints and world-map package extension. Reviewed
pinned Melee c2ebe7 native-3ds/port/3ds/renderer.c raster/cull/uniform ownership
and citro3d_fix.c normal-memory descriptor staging. Foxhollow's alternate
modelDoAltRenderInstrs uses local animation matrices; modelLoadMtxsToGx composes
view*object*joint. Native correction composes object*joint in its existing pose
buffer only for this path. Ordinary animated world-space poses and rigid/fixed
slots are unchanged.4800 actual-generated-C cases under ASAN/UBSAN pass;1600
missing-world negative controls fail reference comparison. No shader changes,
extra GPU allocations, relaxed fences or depth-readback regression changes.

World-map omissions were an asset-set preparation gap, independently fixed by
appending38 source-validated packets while retaining558 existing packets byte
for byte. v12 private package32ed57156235 restores planet/islands/paths. Exact
New3cf16 reached map/Select clean with measured RAM headroom. ExactOld56cb
rendered map but exceeded180s bound before Select: NOT a passed run or playable.
Sky gaps/hologram remain wrong. No physical gain or Pepper hang fix claimed.
InstalledNew4628/Old9793 unchanged. See IMPLEMENTATION/native-alt-pose/audit.json.


7 October native texture residency: consulted pinned Melee renderer.c texture_load,
texture_evict and texture_barrier (c2ebe7c769b3287f2b3748173e8fe5ec24ef408e).
Melee explicitly raises its directory capacity for many small menu textures while
preserving its byte cap. Adapted that separation: native New slots128->512, Old
128->256, existing GPU4/2MiB and CPU2/1MiB caps unchanged. Use64-bit per-access
recency rather than frame-only ties. No Melee16MiB budget transplanted. Keep
Foxhollow's per-content snapshot comparison and existing GPU fence before every
retirement; no uninstrumented-source generation assumption and no pending-resource
reuse. Added cumulative change/directory/budget retirement counters and live byte
counts to existing buffered metrics. Host actual-C tests cover same-frame200-small
texture reuse and full-directory oldest eviction as well as mutation/pinning/fences.
Physical performance and integration remain to validate; not a Pepper crash fix.

Texture residency follow-up: the512-entry linear lookup made the matched idle Fox
window slower than the prior128-entry build. Adopted pinned Melee texture_index.h
verbatim (1024buckets/slots,4096bytes), already covered by the Melee GPL notices.
Hash only narrows candidates; full pointer/format/palette/dimension/surface key
and source-byte validation remain. Remove entries after the GPU fence, before
freeing; publish only fully initialized uploads. Eviction selection remains LRU
on misses. Host fixture compares every indexed lookup with the old linear lookup
through mutations, surface/full variants, capacity, palette changes and pinning.
No pending-resource reuse, shader or submission protocol change.


7 October model texture validation: reviewed pinned Melee c2ebe7's
port/engine/geometry_dirty.h, port/3ds/uniform_upload.h, renderer.c and
citro3d_fix.c. Melee allows reuse only within verified dependency lifetimes;
Foxhollow uses a narrower synchronous instruction-loop lifetime, not global
immutability or Melee memory budgets. Existing world texture scope is extended
only to callback-free model loops after all animation/morph/after-bones work.
Original callbacks stay on the fully compared route. Every object boundary
closes reuse; source/palette write overlap immediately invalidates entries.
Checked mode re-compares every hit;49332 live comparisons pass. No GPU fence,
shader layout, fixed attributes, cache flush or resource-retirement change.
Scene handles use validated offset/stride bounds in their fixed directory;
unknown pointers/empty slots are rejected before dereference. No new allocations.
Rejected world span scope had no live reuse and was removed. Full evidence and
limitations in IMPLEMENTATION and research/native-fox-render/audit.json.


7 October constant lighting: reviewed pinned Melee c2ebe7 port/3ds/vertex.v.pica,
renderer.c and uniform_upload.h. Adapt Melee's unlit/constant/full path separation
to Foxhollow packed light channels without changing the shared palette/output
layout. Foxhollow's constant path preserves vertex ambient/material, uses one
uniform marker and uploads only four needed lighting vectors; sphere mapping
keeps normal calculations. Melee hardware protections (complete outputs, MOVA
separation, initialized fixed attrs, bounded IF stack, fences) retained. No new
fixed-function model fragment lighting, GPU readback or resource lifetime change.
Float32 accumulation order differs from the generic per-vertex path; host math
and actual-console-binary headless A/B images checked, physical validation pending.
No substantial emulator cadence gain or Pepper hang fix established.


7 October mutable model views: inspected pinned Melee c2ebe7 renderer.c immutable
queued geometry, stream-capacity drains, texture retirement and geometry_dirty.h
exact-source validation. Foxhollow adapts that ownership contract to model-file
geometry shared by distinct animated source arrays: separately allocated views
retain exact snapshots and last-reader tokens. No speculative write generations,
no early GPU buffer reuse or changed libctru queue protocol. At most7extra views
per owner,2MiBNew/512KiBOld within existing scene/global budgets. Failed admission
uses existing fenced in-place updates. Actual-C queued-reader negative control
and same-console-binary on/off tests pass; physical benefit remains unmeasured.

## File-loaded CMPR lifetime reuse (7 October)

Pinned Melee c2ebe7 renderer.c uses explicit source visibility generations for
texture reuse and optional smoke hashing. Foxhollow now follows that contract
only for audited file-loaded CMPR images; loader/relocation registration and
actual allocation release hooks bound identity. Unknown/dynamic images keep
exact comparisons. All overlap/broad invalidations and GPU lifetime fences stay.
1024slot20KiB registry has eviction and serial-wrap fallback; checked mode rejects
unreported mutations. New/Old host tests plus exactNew97815checked reuses pass.
Full evidence/limits: IMPLEMENTATION and research/native-texture-assets/audit.json.
No physical performance or Pepper hang resolution is claimed by this adaptation.

## Subpixel collapsed-model LOD and early crash-log persistence (7 October)

Pinned Melee geometry_bounds.h supplies the conservative bounds/error-margin
pattern; this port's existing multi-bone bound now distinguishes exact empty
coverage from authorized low-detail omission. Bounds under1/4pixel in bothaxes,
with collapsed transforms only, can omit a pixel speck near a pixelcenter. This
is explicitly LOD, not a proof of invisibility or of fixing the Pepper GPUhang.
No gameplay/progression changes. Freshpartial4628fixture now selectsLOD; 6000
boundscases pass. Queue log is flushed before large failure-onlySDcaptures,
following bounded diagnostic separation; GPUresource waits remain intact.

## Complete asset inventory and bounded scene cache (8 October)

Consulted pinned Melee c2ebe7 file_io.c: bounded buffered file ownership, hot
cache reads, separate SD counters and worker publication. Foxhollow uses an
immutable LZ4 block window for its complete prepared scene file instead of
Melee's selected resident files. One private CPU-only FILE and fixed 7 MiB
normal allocation replace the assumption that the whole package fits. Whole
file CRC verification happens at startup; misses are counted and decoded CRCs
checked. This adaptation adds no threads, GPU storage or source lifetime changes.
Memory pressure releases the optional window and restores the original file
cursor; actual stream-adapter fault and pressure tests cover both paths.

Game assets are discovered from every model/world bank, including common root
assets. An exact post-topology inventory and deployment gate replace the manual
map list. Renderless source records and partial material compatibility stay
explicit. Source assets, progression, GPU geometry budgets and timing unchanged.

## Virtual-card inventory retention (8 October)

Pinned Melee c2ebe7 port/engine/card.c retains its loaded virtual card across SDK
unmounts; port/3ds/card_storage.c additionally queues persistence. Foxhollow now
adopts the former, with a128KiB retention cap including GCI headers. The physical
SD-backed virtual card is process-owned; imports/FTP happen between launches.
Cold launch/shutdown reset inventory. Oversized inventories and failed writes
force a cold reload after unmount. Existing synchronous temporary/backup writes
remain unchanged: success is still returned only after the write completes.
This avoids changing this game's synchronous save/error contract to Melee's
asynchronous completion contract without a corresponding game integration.
No new thread, GPU allocation, simulation or save-format changes. Actual saved
payload retention is24,640B for the supplied card. Host filesystem-interposition
tests and actual console-binary pause/save/cold-reload checks are recorded in
research/card-residency and IMPLEMENTATION. Physical latency benefit unmeasured.


## Effect 0x714 byte argument (8 October)

Physical 7c444 dump 191 identifies an ARM alignment data abort in
Effect18_spawnObject, PC 0x279804, vldr s0,[r7], argument object+0x36.
KaldachomSpit_update passes &ObjAnimComponent.alpha (u8); the recovered game
consumer incorrectly treated it as int*. Generated arm_overrides now consumes
const u8*, preserving alpha scaling, random calls and particle generation.
No graphics, GPU, allocator or OS behavior changed. The pinned Melee renderer's
state/lifetime rules were reviewed for the ongoing FPS work; this game-specific
producer/consumer correction has no Melee counterpart. Pinned vendor untouched.
Actual ARM function regression rejects the baseline four-byte access and checks
all 256 alpha values, two adjacent-byte patterns, the actual object member and
null/default behavior. No suppression of effect or broad unaligned-access mode.


## Replay selection and silent DSP inspection (8 October)

The combined native draw owner follows the pinned Melee render_worker.c and
render_snapshot.h lifetime rules already recorded in the implementation notes.
The replay app now selects MAIN or SHARED per launch, preserving ordinary game
configuration and saves. Requested and actually executed modes are recorded.
The Old 3DS replay retains MAIN. This adds no guest audio capture or filesystem
work during playback. A separate Azahar build captures DSP output before host
mute/stretch into a bounded, exclusively created host file. MAIN/SHARED runs of
fe10ec27c4c1 complete the same palace recipe; signal/image evidence and its limits
are in artifacts/research/native-prepared-draws/audio-integration.json. This does
not establish physical FPS, audio deadlines, complete fidelity or APT behavior.

## World hierarchy plane propagation (8 October)

Pinned Melee port/engine/geometry_bounds.h uses conservative homogeneous side
planes and a .002 relative margin; its cached planes retain exact camera keys.
Foxhollow's existing world hierarchy uses an additional .0001 parent margin.
The new implementation carries a four-bit set of unresolved parent planes into
child tests. It retains the same bounds, triangle order and conservative margins;
invalid parents keep all child planes. No temporal cache, asset mutation rule,
shader, GPU fence or allocation changes. Exact ordered-range tests include all
11,589 world groups of the current prepared pack. Performance acceptance is
separate from this correctness evidence; see world-plane-masks/ records.

## Retained raster publication (8 October)

Adapted pinned Melee renderer.c STATE_CULL/SCISSOR/TEXENV/DEPTH/BLEND/ALPHA retained
normal-state approach into private Citro3D base.c generation. Shared publication
comparison covers all native passes instead of duplicating state in each draw
helper. Exact values only. Init/APT/target invalidate; shader configuration also
invalidates vertex AttrInfo because it independently writes INPUTBUFFER_CONFIG.
No GPU texture/cache/fence/draw side effects filtered.348Bstatic,0dynamic bytes.
Actual ARM differential/mutation tests plus samebinary silentheadless game pair
prove register equivalence in tested transitions and ~28%less gamecommandbytes;
not physicalFPS. Detailed evidence latestIMPLEMENTATION. Vendor trees untouched.

8 October GPU bandwidth diagnostic: consulted pinned Melee renderer.c renderer_begin
for completion-before-reuse and no additional VBlank pacing, plus locked Citro3D
texture.c/renderqueue.c. Independent tools/gpu_bandwidth_bench/main.c uses SDK
Citro3D to preserve its automatic-flush control; game continues explicit flushing.
CPU VRAM stores and raw depth reads absent. Read-only GSP external busy/hotspot
register snapshots are not configured performance counters. No thread introduced.
See docs/GPU_BANDWIDTH_BENCH.md and artifacts/diagnostic/gpu-bandwidth-final/.

## Completed VRAM texture promotion candidate (8 October)

Consulted pinned Melee `port/3ds/texture_vram.h`, `renderer.c` and pinned
Citro3D `texture.c`/`renderqueue.c` before changing texture storage. Melee's
hardware default DISABLES deferred promotion after hangs/corruption. This is
not an endorsement to copy that path unchanged.

Foxhollow now promotes stable textures only at a real frame boundary after
previous GPU work and CPU descriptors retire. It fills a fresh queue with at
most eight contiguous uploads / 256 KiB, completes a non-presenting frame,
joins GPU completion/callbacks, then replaces pointers and frees old linear
copies. Both allocations remain owned through a stall. Format, dimensions,
texels and sampler parameters are preserved. No CPU VRAM stores or depth reads.
A 3 MiB resident limit and 1 MiB VRAM reserve leave room for existing targets.
Normal content invalidation and fenced eviction still govern these textures.
No new worker or game simulation shortcut. Resident images currently leave by
normal texture cache eviction; a separate cold-VRAM replacement policy is not
implemented, so later scenes may fall back to linear until ordinary eviction.

Thirteen production-body host cases cover delayed completion, byte copies,
allocation failure, bounds/reserve and both linear-memory mappings. Existing
29 readback / 3843 texture-copy checks retain physical-crash negative controls.
An initial mapping assumption failed in Azahar and was corrected before any
hardware deployment. Headless actual-game cave on/off and palace shared-worker
runs complete. Separate GPU tests compare 288 exact rendered images across
eight texture formats/six sizes/three allocation rounds, before and after a
source has been sampled. Cave mist varies between runs even before delayed
promotion; it remains after the transition. See native-vram-textures records.
These tests are not physical sustained stability or FPS evidence.

8 October isolated WarpStone asset repair: reviewed pinned Melee port/engine/geometry_bounds.h, which deliberately retains the normal path for mixed-matrix skinning. No bind-pose culling rule was transplanted onto the animated WarpStone. This change restores original topology and recompiles one native packet; GPU services, retirement, shaders and allocation policy stay as previously validated. Two neutral-shaded scientific plots use real decoded geometry with common bounds, not synthetic artwork or game captures. Animated visual/performance acceptance remains required.

Animated model bounds candidate (8 October): reviewed pinned Melee port/engine/geometry_bounds.h before implementation. Melee retains mixed-matrix models; Foxhollow extends the conservative homogeneous-side-plane approach using per-bone boxes of actual compact positions plus bind tails, transformed by each current pose. Both legal convex influences are included. Input, matrix and projection error margins inflate bounds; nonfinite/invalid inputs retain geometry. No perspective division, near-plane LOD, callback suppression or simulation changes. Special callbacks, movie and cloud paths retain previous handling.

Bounds belong separately to each base/alternate mutable model backing. Original byte comparisons and upload fences still apply; position refresh invalidates only that backing, and lifetime release frees its optional normal-heap bounds. The existing 4MiB scene CPU budget and normal-heap reserve gate allocation; allocation failure retains draws. This adds no linear or VRAM allocation. Same-binary model_bounds.txt toggles permit controlled tests; ordinary default is OFF after the emulator candidate increased frame time; explicit ON remains a local candidate. Native model capture now compiles for replay builds only as well as existing emulator-test builds; ordinary console builds omit it. Explicit opt-in snapshots are bounded to64groups and invalidate timing comparisons.

Replay camera control (8 October): consulted pinned Melee native-3ds/port/engine/
video_pad.c and port/3ds/main.c for the separation between pad publication and
VI pacing. No Melee pacing rule or input mapping was changed. Foxhollow's
engine/68 firstPersonDoControls has inertial yaw and stick-position-controlled
pitch; a timed sweep is therefore not a reliable camera checkpoint. AIM_VIEW
uses normal GC-level stick input, inverse PADClamp axis scaling, typed read-only
viewfinder state and continuous settling. CHECK_VIEW rejects drift and wrong
modes. Loading/cutscene gates retain neutral input, and deadlines remain bounded.
No camera/player/game-state writes or simulation changes. Replay CSV gains
camera observations, increasing its fixed normal-RAM allocation by128KiB to
480KiB; no linear/VRAM/worker changes and ordinary builds exclude the hooks.

8 October VRAM mask correction: re-read pinned Melee texture_vram.h including
its hardware disable, and pinned Citro3D texture.c sampler/allocator behavior.
Physical capture20261008-141810 shows eight L4/VRAM/TEXUNIT1 draws51..58 before
GPU command-list freeze at frame380. This matches the first-person reproducer
at https://www.3dbrew.org/wiki/GPU/Pitfalls#TEXUNIT1_Corruption_and_Instability .
The report itself qualifies reproduction consistency; matching state is strong
evidence for a hazard, not proof this candidate fixes all observed hangs.

Instead of disabling all VRAM textures, leave every uncompressed L4/A4 cache
entry in linear memory, regardless of first sampler. Future unit1 use is then
safe from this placement hazard without a per-draw conversion or removal of
masks. Other formats remain eligible; ETC1 is block-compressed and distinct.
A4 is precautionary. Host production-body tests cover all14formats, plus earlier
ownership/pressure cases. Offline sampler analysis rejects all eight physical
negative controls. The prior pixel-copy fixture only bound unit0, so its passes
never tested this hardware restriction. Azahar acceptance is insufficient.

Retained-source residency also departs from Melee's cold-entry deletion: keep
immutable original linear images for pressure-driven pointer demotion after
all GPU/CPU readers retire. No reverse copies or SD rereads;3MiB additionalVRAM,
1MiB reserve,120coldframes,8demotions/frame maximum. Existing4MiB/2MiB converted
image limits bound retained linear images. Both allocations survive stalls;
retirement frees both. Exact-source rebinding and pressured reuse tested locally;
actual cave run did not exert pressure. Hardware acceptance remains open.

8 October replay accounting: reread pinned Melee c2ebe7c port/3ds/gpu_busy.c.
Retain its submission/IRQ observation, including our failure/unknown-event guards;
snapshot at actual replay boundaries instead of sparse120-frame reports. GPU
outstanding time overlaps CPU and includes service latency. New exclusive
main-thread scope clock is project-specific, borrowing existing wrapper
signatures without changing game calls, return values, scheduling or GPU fences.
Replay stores fixed-size RAM rows and writes only after measurement. Main-thread
identity prevents worker scopes from mutating the partition. See
artifacts/research/replay-frame-accounting/integration.json and host.json.

World original-list admission: reread pinned Melee port/engine/geometry_bounds.h
and Citro3D9f21cf7 source/drawElements.c. Keep conservative culling overflow as a
full draw. A world-only32766index limit admits original19182index/4284vertex list;
16-bit vertex addressing,8192vertices,64KiB gather,model limits and merge thresholds
stay unchanged. No Melee code copied. Original world assets restored offline
require separate workload/memory/performance validation before deployment.

8 October capacity-only native queue backpressure. Reviewed pinned Melee
port/3ds/render_worker.c dispatch/wait_until/mp_renderer_end. Melee drains its
full job ring on capacity. Our32slot immutable prepared queue now waits for one
free slot for allocation only; all resource barriers still require zero pending
CPU readers before existing GPU retirement. Unsigned outstanding counts preserve
wrap behavior. Host actual-code sanitizer tests include blockedconsumer releasing
one slot,12000orderedjobs,wrap,refusal and suspendbarriers. Integrated palace
passes; no whole-frame FPS gain established. Evidence native-worker-backpressure.

Replay native stage timing (8 October): extend the existing main-owner wall
clock; GPU observer still follows pinned Melee gpu_busy.c without changing
submission. Cleanup scopes restore every normal/early return; sourcevalidation
reuses existing timestamp reads. No GPU/OS ownership or simulation changes.
See native-stage-timing/integration.json; instrumentation is not a speedup.


## 9 October: persistent main-world draw plans (prototype)

Consulted pinned Melee c2ebe7c769b3287f2b3748173e8fe5ec24ef408e
native-3ds/port/3ds/render_snapshot.h and render_worker.c. Retain their distinction
between CPU descriptor consumption and borrowed geometry's separate GPU lifetime.
This port compiles the Foxhollow main-world bitstream at asset admission, binds
immutable packet geometry, and retires the CPU plan with its owning scene after
the existing storage join. It does not persist uniforms, texture pointers or live
shader state and does not change queue/OS/GPU barriers. Movie callback blocks take
the complete prior interpreter; dynamic source snapshots must still match before
plan execution. 256 KiB aggregate optional CPU plan cap, existing scene/global
headroom guards, allocation failure falls back without dropping game draws.
No equivalent Foxhollow instruction compiler is claimed in Melee. This prototype
has no demonstrated whole-frame performance gain; see native-draw-plan evidence.

For the user-requested rendering reset, reuse the existing rendering-off diagnostic
instead of removing arbitrary game callbacks. Retain the original simulation clock,
audio and input cadence. Replay counters explicitly label game iterations when no
renderer is active; this is not rendered FPS or proof of console playability.

9 October radical visual stripping and audio slicing: pinnedMelee render_worker.c
and render_snapshot.h reviewed again. No GPUmemory/fence changes. Foxhollow-specific
visual-preparation diagnostic retains simulation and audited render-side mutations;
mode selection is startup/per-replay only. MusyX worker now releases recursive lock
after one160-sample synth slice instead of holding it for a640-frame packet. Existing
CPU-owned partial PCM packet persists across locks; publish only completepacket,
finishunderoldcallbackathandoff,stopclears. Actualproduction-C sanitizer comparisons
verifyorderedstereo output andhandoffs. This is not attributed to copiedMeleeaudio.

9 October gradual world reconstruction (mode4): reviewed pinned Melee
render_snapshot.h/render_worker.c CPU-consumption versus GPU-lifetime contracts.
Reuse Foxhollow's bounded predecoded immutable world plans and prepared native
world submissions, including source validation, use marking, resource retirement,
scissor/frustum checks and GPU fences. No new GPU allocation or pointer borrowing.
New direct executor deliberately bypasses original GX material/light construction
for callback-free main-world blocks; preserves hidden flag2, view-frustum and
main-pass player-relative visibility. Blocks with movie callbacks retain original
interpreter. Flat vertex-colour world plus HUD/movie is a reconstruction stage,
not gameplay fidelity acceptance. Models/effects/water emission suppressed only
in this opt-in tier after existing callbacks/state updates. Normal FULL unchanged.
No equivalent game bytecode executor is claimed in Melee.

9 October flat-world block state and hardware profiling: block-local common PICA
setup is reused only during the callback-free WORLD ONLY executor in MAIN mode.
UI flush precedes scope entry; scope exit and GPU retirement invalidate reuse.
Vertex format changes force setup; buffer/raster/scissor and every indexed draw
still execute. Worker mode retains complete immutable prepared-draw setup.
Initial adjacent index merging admitted no palace work; it is not an FPS gain.

Replay-only optional hotspot sampling uses GSP IPC, not direct MMIO. Consulted
https://www.3dbrew.org/wiki/GPU/External_Registers#Hotspot_Profiling and existing
tools/gpu_bandwidth_bench/main.c. Fixed4096x1024GPU-cycle windows, four FIFO reads
only after duration plus1ms, at most2windows/sec and32samples. Saves disabled
control/configuration and restores after completion/failure; refuses an existing
active profiler. No framebuffer/reset registers touched. Logs buffered until exit.
Eight counter labels remain tentative; sum must equal4096 or sample is invalid.
Emulator zero/unsupported results must never establish GPU performance. Profiling
runs explicitly marked timing-perturbed, IPC cost recorded separately.

9 October staged world textures: reviewed pinned Melee renderer.c material-state
reset, independent sampler copies for aliased images, texture_visibility.h and
render_snapshot.h before changes. The new WORLD TEXTURES mode retains the existing
Foxhollow fenced texture resolver, live texture animation/scroll, material order,
independent sampler snapshots and explicit unused sampler/TEV resets. Block-local
program/layout/view/projection reuse is separated from live material uniforms;
only mode4's constant flat material can reuse its complete state. GPU retirement
invalidates block state, and worker setup remains complete. No shader, GPU fence,
VRAM allocation or index-order changes. This stage restores base/layered world
textures and ambient colour; dynamic lights, deferred falloff, models and effects
remain absent, so it is not parity acceptance. New texture stage will be checked
with ORIGINAL installed assets to isolate renderer cost; coalesced09 assets stay
local until separately installed in an isolated replay asset path.

Lossless FHNSCN09 coalescing is a Foxhollow-specific storage extension, not copied
Melee behavior. It keeps per-list/source/material identity, draw order and live
position references while allowing adjacent compatible vertex storage to span
material boundaries. CPU plan snapshots still borrow fenced GPU-owned geometry;
no weaker ownership lifetime. Actual C reader/position refresh verifies1267worlds,
11589draw expansions,2534negative cases; saved original bytecode compiles1259plans
and10912draw slices identically. It does not simplify or omit geometry.


9 October unified Old/New replay: reviewed pinned Melee render_snapshot.h,
render_worker.c, renderer.c material/sampler reset rules and texture_vram.h /
texture_visibility.h. The renderer's queue ownership and texture fences remain.
Runtime hardware selection is a departure from Melee's New-only target. The
libctru startup heap call is wrapped to choose the existing 68+24 MiB or 46+6 MiB
reservations using kernel process entitlement before SDK allocation. The SDK
allocator itself remains unchanged; linked disassembly confirms __libctru_init
calls the wrapper and the wrapper calls the original allocator. After services
initialize, APT_CheckNew3DS selects CPU/core/control behavior. Old hardware cannot
enable the New shared core-2 executor or prepared-worker defaults, even from its
configuration file. The Old audio worker retains core 0 and a 96 KiB stack.
The game arena, linear reserve, texture caps, scene views and RAM cache policy
select the smaller memory profile at runtime. Texture metadata has a shared
512-slot table; actual texture byte budgets remain target-specific.
Replay-only private asset paths permit exact fixture assets without replacing
ordinary game assets. These paths use immutable strings, including worker reads.
Candidate 4a71a863b600 was rejected headlessly: a later New worker default
re-enabled shared audio on Old, increasing stack allocation and triggering the
normal-RAM texture budget. Corrected 965f6695d701 completes the Old textured palace
replay with zero native-world failures; physical validation is still pending.


9 October world pass state: pinned Melee renderer.c resets all combiner stages
and explicitly unbinds unused texture samplers on material transitions. Retain
those full setups at block/callback/worker/GPU ownership boundaries. Within the
existing callback-free unlit world scope, an exact TEV key now avoids repeating
the same six-stage setup; image pointers, UV scale/scroll, colours, fog and all
samplers remain live. Layer/intensity/constant changes update the combiner key.
Depth/alpha/blend/cull/scissor reuse requires identical live raster inputs;
foreground/light/movie paths keep complete setup. Actual material equations and
4096 exact-state comparisons pass. Local0021c0ead841 New headless passes, submit
5.682->5.320ms, main20.391->19.878ms, both~30AzaharFPS. Not physical evidence.
Stationary captures inspected; camera endpoint differs from baseline by16 angle
units, so no cross-build pixel equality claim. No console update for this change.

Old snapshot allocation experiment: current replay spends235.55ms/frame resolving
textures in Azahar,5032 budget evictions/44.68MB runtime texture SD reads for the
whole replay. Keep image/CPU byte caps and15%normal/1MiBlinear headroom; charge
CPU-only comparison snapshots to spare Old linear space instead. No CPU access
toVRAM, no stdio tolinear, no new GPU input or cache-coherence contract. Reviewed
pinned Melee command_cache.c: explicit GPU range publication unchanged. Every
snapshot failure/retirement uses its matching allocator. New retains malloc.
This is a departure from New-only Melee allocation; local evaluation pending.


9 October unified formats: build_native now produces one 3DSX and one CIA from
one ELF per application variant. Platform ctr, Old96/New124 exheader requests,
visible/usage SMDH without New-only flag. package_native_cia verifies exact ELF
segments, CIA hashes/title IDs, icon/banner/audio and pinned Melee launch logo.
Pinned makerom0.19/bannertool1.2 downloads are checksum-verified. The ordinary
and replay applications have distinct stable title IDs; neither is hardware-specific.
Build matrix, packagers, FTP staging and headless runners carry both formats.
CIA headless mode boots exact matching CXI content, not a HOME installation.
Runtime heap selection uses actual kernel entitlement including current code
commit and15%expanded process reserve; Old96 chooses52+16MiB, New12468+24,
80MiB intermediate48+10, legacy64 fallback46+6. Model detection still gates cores.
Melee's New-only snake/SystemMode64 profile is deliberately not copied. Its
service permissions, stack, SD paths and verified Homebrew launch-logo approach
are adapted. No private assets are embedded; see docs/UNIFIED_BUILDS.md.

Physical Old661 collected console-test-app-results/20261008-185552: same palace
fixture3971085882/recipee139cbf06f40, WORLD TEXTURES, profilingOFF, MAIN/VRAMOFF.
9.187FPS/108.848ms versus1.040FPS/961.713ms (~8.84x). Texture809.35->2.307ms;
GPUwait76.81->0.028ms. Native submit58.807ms is now the dominant measured bucket;
GPU pending7.024ms overlaps CPU and is not added. 51measuredframes/5.55s;
max122.137ms,p95118.192ms,zero>250ms. Still unplayably slow; no full-renderer60claim.
Run source0ce528e7 matches installed661. Process grant remains64MiB underHBL.
This is not CIA/extended-memory performance evidence. CIA deployment pending.


9 October correction: 96 MiB is a documented Old3DS SystemMode (2).
Do not claim it is impossible or invalid based on the pre-menu crash. Cause is
unconfirmed; collected crash dumps are dated 2017, not evidence of this run.
User challenged blanket rejection; deployment is PAUSED, no 80 MiB upload.
Current local RSF remains an 80/124 candidate, not a validated fix. Installed
replay remains cabc9c16dcc9 / CIA40141b391cd5 (Old96/New124), both consoles.
Validator now decodes all defined Old memory modes and checks the requested
build profile. Regression passes: intact archived96 accepted for96, rejected
for mismatched80. Artwork supplied from current build matched archived CIA.
Smash example header shows Old80/New124; that does not disprove96 support.
Local 80MiB CIA-content run completed cleanly at
artifacts/native/palace-vram-off-main-unified-cia80-old; no HOME transition or
physical validation. No live tool sessions, no new FTP writes this turn.
Tools changed after current binaries: rebuild before future packaging/deploy.
Continue diagnosing96 launch instead of presenting80 as established fix.
Goal remains active/incomplete. This supersedes earlier FINAL deployment notes.



9 October: user now explicitly selected Old80/New124 CIA after96 probe failed.
Replay candidate bc811b261bf0 (CIA f41f58a3b577, source910b35d73029) built;
package261bb05177c7. Old.229 deployment IN PROGRESS session72167, installer
replay-only. Do not claim installed before terminal transfer success. New.223
still cabc/40141b; ordinary game unchanged. Both headless CIA-content replays
passed: palace-vram-off-main-index-publication-old/new. Saves unchanged, clean
shutdown, images inspected. Old48+10 heaps,69,844,992 process/83,886,080 grant,
normalpeak40,720,296; minlinear5,380,992; VRAMfree4,858,880; stack75,548/524,288.
New68+24,105,496,576/130,023,424,normal55,996,352,linearfree10,004,096,
stack43,680/524,288. Process/pool reserves >15%, includesfullmainstack reserve.
No physical80 launch or FPS improvement yet established.

Isolated memory probe67a1aa6f27e7 / CIA2e145a2f9ba5 was deployed toOld at
/3ds/foxhollow-memory-probe/foxhollow-memory-probe.cia (transfer193602).
User:3DSX passed,CIA96crashed. Collected193? actual directory
artifacts/console-memory-probe-results/20261008-194035. HBL64MiB,54.4MiB
commit sample tested and freed, cleanexit. Fresh crash_dump_00000003 identifies
HOME Menu(menu),title0004003000008f02,dataabort PC12c718,strh r3,[r1],r1=0.
CIA produced noSDcheckpoint. This newly identifies HOME crash; precise cause
ofNULL remainsunproven. Previous2017dumpsnotattributable. RunningLuma info
151060480; SDboot.firm string9.1. No firmware changes.96remainsdefinedvalidmode.

Renderer change: append-only world indices now cleaned once at command-list
publication (Melee GX_ProcessCommandList hook), before GPU admission including
internal splits. Owner-only cursor,128byte line separation, unchangedfences;
reset used+published onlyafterFrameBegin success. Extra persistentstate16bytes,
noextra linear/VRAM. 100,000gather/20,000retirement tests plusnoncoherentGPU
shadowpublication checks passed;1,700productionspan capacity cases passed,
unsafecontrolrejected. Stale test stub renamedfh_minimal_sync->owner_gpu_sync.
Old80emulator ~28.81->29.52FPS,nativeSubmit14.47->13.04ms, gatheredpublication
perdraw7832 becomes2696 commandboundaries. Wholecachecalls19289->14344, ticks
184837729->137563165 but runs differ in duration/camera withinrecipe tolerances;
NOT pixelidentical or exactcapturedworkload comparison,NOTphysicalforecast.
Baseline/candidate images visually retainworld/HUD, camera differs ~1worldunit;
recorded rawpixel differences are not a correctness pass. Worldmodels/effects
remainincomplete. FullplayabilitygoalACTIVE. Both headlessprocessesterminal.


9 October PICA renderer REPLACEMENT implemented as a separate backend, per user mandate. Current replay3dsx5aeb1aef3727, CIA314a16e6663f, source0f4ca55d54a5; Old80/New124 unifiedpair. Package build/console-replay/foxhollow-console-replay-5aeb1aef3727-private.zip SHA5a0e980f8211. Old.229 FTP deployment IN PROGRESS owned session89566; do not claim uploaded yet. New.223 unchanged.
New files platform_pica_scene.c, pica_world_plan.c, fh_pica_scene.h, pica_scene.v.pica. Own10instruction shader and simple native submission, persistent load-time geometry descriptors from prepared assets. Does NOT call old world plan/GX material interpreter/texture resolver/prepared queue/index gathering/per-draw source checks. Reuses verified target,fences,cache publication,asset readers. Diagnostic mode7 PICA GEOMETRY explicitly has no textures/models/HUD/effects/alpha or fine visibility. Simulation/audio untouched. Old legacyWORLD TEXTURES still available only as comparison. Do not re-add old CPU-heavy renderer into new mode.
Tests: differential1259world packets10912draws1775196indices;1259short-output rejections,8empty-main packets. Shader full-output/capacity pass. Isolationgate rejects oldrenderer draw counters; coveragegate rejects formerly false-positive black-modelrun. Final Old exactCIA-content headless palace-vram-off-main-pica-scene-resident-old PASS, imagesinspected (untexturedpalace),59.6497FPS/16.765ms, measured299frames,135draws33585indices/frame. This is emulator timing, NOT physical speedup. Normal40709312/50331648,linearfree6735488/10485760,VRAMfree4858880,process69849088/83886080,mainstack43836/524288; >15% headroom. Initial prototype f28db1851455 testedOld/New headlessnear60; finalresidentplan onlyOldtestedsofar. Processesclosed. Hardware question whether nativecostreduction carriesover; no physicalreplacementrunyet.
Baselinebc811preserved artifacts/research/pica-scene-rewrite/baseline and originalprivatezip. PhysicalOldbaseline9.91FPS/101ms/51.1submit. Docs/PICA_RENDERER_REPLACEMENT.md defines architecture, budgets and explicit omissions. Next: finishOldupload/readbackandgivefullremotepaths; continue nativepreparedmaterial/texture pipeline (notlegacyresolver) while awaitinghardware. Goalactive/incomplete, progress notblocked. Ordinarygame/assets/saves unchanged.



9 October native PICA material replacement (4cd922fe5a6b): follows pinned
melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e command_cache.c
publication and render_snapshot.h GPU-lifetime rules. It does not reuse the
snapshot/interpreter submission architecture. Load-time native image ownership,
compiled TEV/samplers and GPU UV conversion replace per-draw GX material/texture
resolution. Citro3D texture.c/renderqueue.c examined for binding and retirement;
scene owners fence before deleting retained images/materials. Shader writes all
four declared output registers;20instructions/11floats. Geometry and texture
programs selected once at startup, never alternated during a running tier.

Prepared image sharing uses verified pack offset identity. Missing pack entries
convert at load to bounded temporary staging and use exact native-byte comparison
before acquiring shared storage; no hash-only/source-pointer aliasing. Cave
regression caught missing dedup (2MiB cap exhaustion); corrected residency peaks
479264bytes with no missing materials/leaks. Originalprepared/privateassets unchanged.
No claims for characters,HUD,fog/deferred effects or physical console performance.
Repeated source position checking remains temporary compatibility to replace
with producer-driven updates before calling the architecture complete.

9 October native PICA world position publication: explicit notifications overlay Foxhollow pinned GroundAnima.c (animation update plus free-time restoration) and XYZAnimator.c (update), leaving game/collision writes intact. The separate PICA world plan attaches source vertices at load and validates/publishes only after producer notification. Reuses verified GPU retirement and range-flush ordering from the pinned Melee command-cache/snapshot model; does not reuse Melee game data or alter simulation. CPU work retained: dirty-block comparison, scatter into prepared geometry, bounds update, cache publication after GPU retirement. Immutable draws only test dirty state. Tests/evidence: artifacts/research/pica-scene-rewrite/producer-position-tests and position-producer-cave-old.json. No new physical timing claim.

10 October native graphics options adaptation: consulted pinned melee-3ds/native-3ds/port/3ds/renderer.c constant-colour combiner bindings and explicit GX_CMDLIST_FLUSH submission. Foxhollow retains its precompiled material plans, existing fences and retirement order; a single immutable 8x8 RGBA8 neutral sampler is cache-published once and retained until renderer shutdown. Rendering controls gate native submissions only, without unloading asset owners or changing simulation. HUD/particle sampler caches invalidate on option-mask changes. No GX interpretation, CPU vertex conversion, extra pass, or full-heap flush introduced. Untextured source KColor materials now compile their solid-colour branch at load and publish live packed colour/alpha in the existing material constant; shared source helper must be emitted into both model.c and objprint_dolphin.c. Host source-oracle checks pass; runtime/physical evidence must be recorded separately.
