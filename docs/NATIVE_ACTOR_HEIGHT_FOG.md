# Actor height fog: source inputs and native preparation

14 October: the prepared-file loader and native actor-height-fog consumer are
integrated in isolated replay builds. The first cave run loaded packets but
recorded zero fog draws; that build failed coverage and was not deployed.
Subsequent rejection counters found an unconditional optional-state rejection
and no prepared material programs. Corrections now produce 52,195 fog draws in
the New-profile cave replay, with normal shutdown and preserved saves.
Full actor-fog parity and performance are not accepted. Deployed world height
fog is a separate path. Earlier dated sections below record the development
stages and retain their original validation scope.

**Source-routing correction:** `GX_PNMTX0` is the default selection, not an
unconditional per-vertex choice. Aurora `gx/shader.cpp:1093` first checks the
corresponding `GX_VA_TEXnMTXIDX` attribute and uses its indexed matrix when
present. `modelRenderFn_setVtxDescr` enables these attributes according to the
material and projection path. The audit below does not yet resolve that
routing. Its counts describe differences from the default matrix, not proven
incorrect native transforms or the number of models needing repair. Do not
integrate a fixed-PNMTX0 consumer based on these counts.

The original `renderHeavyFog` generates coordinates from `GX_TG_POS` using
`GX_PNMTX0`, then applies its two fog planes. A vertex's position-matrix index
does not by itself select this texture matrix; a texture-matrix index can
override the default as described above. `renderOpMatrix` changes PNMTX0 while
walking source model lists. Reusing the final native skinned position would
therefore requires checking the actual source texture-matrix routing. Native preparation also
rebases some positions for skeletal tails; those bytes cannot silently stand
in for the original source position.

The read-only inventory covers 821 of 829 unique original models; eight
instruction-overflow streams remain explicit exclusions. Of 801 models whose
header permits fog, 177 have position bindings that differ from PNMTX0. This
affects 428,455 of 944,670 source triangle corners in that collection. It is a
structural mismatch count, not observed UV error or visible corruption.

The current source-ordinal geometry grouping has 1,445 groups for those models.
168 groups cross PNMTX0 changes. Splitting at every such boundary would add 394
draws across the collection, before runtime visibility or native large-list
splits. This is not accepted as a free solution; a live design must budget
submission and palette storage. The independent cursor respects source matrix
ID aliases for slots 10/11 and does not invent an uninitialized PNMTX0 binding.
Original, not reduced/installed, assets are the scope of this inventory.

`fh_pica_actor_fog.h` / `pica_actor_fog.c` provide two preparation primitives:

- Copy referenced original signed-short positions into eight-byte PICA fetch
  records at load time, with deterministic padding and complete validation
  before publication. No source pointers or allocations are retained.
- Compose the two source fog planes with the actual PNMTX0 camera matrix into
  two GPU uniform rows. This is draw-constant work, with no per-vertex CPU loop.
  The caller must preserve source VAT scaling and validate atlas orientation;
  do not assume a texture-V inversion (the world path required removing it).

The corpus estimate is 1,402,784 bytes for eight-byte records deduplicated by
original position index across all eligible models. It is neither an eager
allocation nor the final live working set: palette splits, per-group streams,
CPU deformation and morph ownership can change storage requirements. Do not
allocate the collection eagerly on Old 3DS or attach these records to models
that do not consume them. Dynamic source positions still require their actual
producer; the immutable helper alone does not handle animated vertex streams.

Pinned Melee's raster-state and cache-publication ownership remains the model
for integration. These pure functions add no GPU owner, fence, queue, SDK call,
worker, shader or game-runtime allocation. An eventual resident owner must use
existing retirement/cache publication, preserve material/fade/fog ordering,
and prove its resource budget before connecting it to gameplay.

`tools/test_pica_actor_fog.py` captures the actual original source fog-plane
constructor and compares 524,288 original position/coordinate cases with the
native composed rows. Maximum relative error is 3.251e-7. It covers signed-short
extrema, three source VAT scales, alias-safe row output, invalid plans and NaN
rejection without partial output. Missing-translation and wrong-position
mutants fail. ASan/UBSan and the ARM11 hard-float warning-clean compile pass.
The intentionally different position matrix produces 524,288 counterexamples
to substituting the skinned position in this synthetic sample set.

These are CPU source/setup checks, not PICA float24, rendered-image, gameplay
or physical-performance proof. Next integration must retain original position
provenance through prepared groups, resolve default/indexed source matrices, provide
bounded native shader/material ownership, and cover actual fogged actors.
No additional hardware probe is requested.

Evidence: `artifacts/research/pica-scene-rewrite/actor-height-fog-audit/report.json`
and `actor-height-fog-test/result.json`. Original assets and saves are unchanged.

14 October routing compiler follow-up: `fh_pica_actor_fog_routing` now prepares
an immutable nine-byte descriptor: eight texture-coordinate-to-packet-byte
lanes and the total direct matrix prefix length. A lane of -1 means the texgen
uses its default matrix; other lanes select original GX matrix IDs, which must
still be resolved into the native palette. This handles forward/backward
attribute allocation, environment/auxiliary attributes, projected-light count,
pass-mask 4's special first projection, and rigid models. Invalid configurations
leave the destination untouched. No allocation, GPU work, or frame consumer was
added. Required configurations should be compiled at asset load, not interpreted
per frame.

`test_pica_actor_fog_routing.py` executes the descriptor prefix extracted from
original `modelRenderFn_setVtxDescr`: 44,800 accepted configurations, 358,400
indexed/default selections, 6,400 oversized prefixes rejected. Both missing
indexed routing and treating coordinate number as packet offset are rejected
by deliberate mutants. ASan/UBSan pass. The 524,288 original coordinate checks
were rerun successfully; the combined module passes warning-clean ARM11 compile.
Evidence: `actor-height-fog-routing-test/result.json` and updated
`actor-height-fog-test/result.json`.

This does not yet determine each material's generated fog coordinate number,
compile its live projection alternatives into the resident actor plan, extend
native bone palettes, or provide the GPU/material consumer. In particular, do
not interpret the earlier default-PNMTX0 inventory as resolved by this helper.
No new application build, hardware request, performance claim or deployment.

14 October coordinate compiler follow-up: `fh_pica_actor_fog_coordinate`
prepares the ordinary source fog coordinate index before route selection.
Diffuse sphere and secondary-light maps share one coordinate; bump contributes
two plus an optional UV coordinate, source projection families contribute their
own count, and a decal replaces the second ordinary layer. Validation rejects
invalid flags/masks and more than eight total coordinates without publishing an
output. This remains load-time metadata preparation, not a TEV interpreter or
GPU material admission rule. The caller must choose the source projection
family and actual live texture alternatives; callbacks remain separate.

`test_pica_actor_fog_coordinate.py` executes 13 original stage constructors,
the original layer-ordering helper for decal cases, and the original descriptor
prefix. It passes 61,040 constructor sequences, rejects 9,360 coordinate-capacity
cases, and covers 21,120 shared sphere/specular cases. Selected fog coordinates
then exercise 180,960 default and 2,160 indexed descriptor routes. These are
controlled configurations, not counts of visible in-game materials. Missing
bump, double-counted environment and double-counted decal mutants fail under
ASan/UBSan. The existing 524,288 coordinate checks and strict ARM11 compile pass.

The current installed `pica_scene_actor_hud_shader` is 511/512 stored
instructions; actor entry zero uses all 96 float and 16 boolean registers.
Older 486-instruction/minimal reports in the build record are not the active
program. Adding actor fog requires an explicit program/uniform layout solution,
not writing beyond this budget. Existing native shader SHA starts 6820952d3f07.
Evidence is `actor-fog-coordinate-test/{result,installed-shader-budget}.json`.
No application was rebuilt or deployed. Remaining integration includes live
material alternatives, original position/morph ownership, resident palette and
vertex plans, native fog sampling/composition and measured memory/submission
cost. No emulator or hardware performance claim accompanies these host checks.

14 October native GPU consumer development: a separate actor vertex program
computes height-fog coordinates from an immutable companion stream. It retains
the ordinary actor's 22-bone, eight-light ABI: 213 stored instructions, 96 float
registers, 16 booleans. It is separate from the existing 511-instruction shared
bank and is not yet selected by gameplay.

`pica_actor_height_fog_shader.py` consumes the stable archived actor shader.
Inputs 6/7 hold independent fog bind-space position, palette-row selectors and
quarter weight. Fixed attributes 8/9 hold source fog planes, requiring no extra
uniform registers or per-frame geometry rebuilding. Ordinary position, colour,
texture and normal outputs remain unchanged. Texture coordinate 2 carries height
fog; distance fog uses the existing directional-diffuse/secondary-light view
arrangement. The separate groom-stream shader and material admission are not
covered. `fh_pica_actor_fog_gpu_vertices` validates all source references,
positions, offsets and bindings before publishing 16-byte companion records.
Cost is 16 bytes per prepared vertex before alignment, deduplication changes and
palette expansion; there is no gameplay allocation yet. Dynamic morph positions
must come from their real publisher, not a new per-frame geometry reconstruction.

Float24 checks cover 65,536 independent bindings and all 22 bones: 1,310,720
retained output lanes are bit-identical to the linked parent, maximum fog UV
error 5.96046448e-08. Using the original shader as a mutant fails. The actual
companion compiler passes 1,024 records, whole-output nonpublication guards,
existing 524,288 source-plane cases, ASan/UBSan and strict ARM11 compilation.
These controlled inputs do not establish correct live model metadata selection.

The asset-free GPU fetch test found a pinned SDK trap: `AttrInfo_AddFixed`
computes `regId << (id*4)` as a 32-bit expression. Actual ARM library disassembly
confirms attributes 8/9 lose their intended permutation. The new native layout
constructs those final entries and the 64-bit permutation explicitly, retaining
SDK dirty-state/publication ownership. No SDK/vendor file was changed. This is
a new-layout repair, not a diagnosis of a previously deployed visual defect.

`build_actor_height_fog_gpu_probe.py` snapshots exact sources and parent identity;
`run_actor_height_fog_gpu_probe.py` verifies them, runs silently with a 30-second
bound, and reads only retired colour storage. V1 failed on SDK permutation.
V2 exposed fixture texture orientation: native Morton rows run opposite sampler
V. Corrected fixture packing follows the decoder; shader source UVs were not
flipped. Both failures remain archived. V3 passes full-format fetch; V4 alternates
compact/full formats and changes planes across 64 draws, passing every sampled
RGBA channel exactly. Captured gradients were inspected. V4's 3.05 host seconds
include startup/cleanup and do not measure gameplay performance. All processes
are terminal. No console probe or application deployment was requested.

Evidence: `actor-height-fog-shader-test/result.json`,
`actor-height-fog-test/result.json`, `actor-height-fog-fetch-test/sdk.json`, and
`artifacts/diagnostic/actor-height-fog-fetch-v1` through `v4`.

Remaining integration: resolve live material alternatives and position/normal
texture-matrix provenance, extend resident groups without incorrect deduplication
or palette overflow, publish morph changes, own shader/material/third-sampler
handoffs, and measure actor scenes and Old/New memory. Default PNMTX0 and indexed
texture matrices must remain distinct. No actor-fog gameplay parity or frame-rate
acceptance is claimed.

14 October source-binding follow-up: native symbolic matrix-bank loading now
retains position versus normal-texture provenance and the source table's shared
slot-zero aliases. Indexed texture coordinates select this bank; unknown,
unsupported and malformed bindings fail without replacing the result. These
are load-time descriptions, not live GX state or an added game-frame interpreter.

The original `renderOpMatrix` cached branch was executed as the load-order oracle:
3,328 sequences, 528,800 resolved selectors and 1,175,136 unsupported/unknown
rejections pass ASan/UBSan. Aliasing, skipped texture loads and indexed routing
mutants fail; ARM11 compile passes. This proves symbolic load order, not the
floating-point matrix producer, live material selection or palette publication.
Evidence: `actor-fog-matrix-test/result.json`. Wiring the metadata into resident
groups remains unfinished; work was steered to the reported Old 3DS startup
rejection before doing so.

14 October asset-loader integration: `install_descriptors` now exposes authored
descriptor and matrix-load events after validating the entire source. Its
ordinary consumer retains byte-identical descriptions for 2,110 assets and
13,559 draws. A material selection without a descriptor opcode retains the old
descriptor, including its texture-load decision.

`install_actor_fog.c` consumes those events at load time for a caller-selected
pass/light alternative. It emits the current symbolic matrix bank and routing
alongside each source draw. An explicit initial bank represents known pre-stream
loads; unknown entries remain unknown. Routing includes the original
`usesTextureMatrix` decision: backward TEX attributes do not necessarily enable
normal-texture matrix loads. The original descriptor oracle verifies this over
44,800 configurations.

The new 16-byte source key combines the existing 12-byte visible vertex reference
with four bytes of independent fog-matrix provenance. This prevents native group
deduplication from merging vertices that need different fog transforms. It keeps
the original position index; the GPU companion stream must use that position,
not the rebased visible vertex. Reference failure leaves the output unchanged.

An independent stream parser matches all banks and routes for 764 ordinary
models under five fixed alternatives, plus five adversarial synthetic cases:
3,825 alternatives and 9,205 draws. ASan/UBSan and ARM11 compilation pass.
Deliberately losing retained matrices, republishing descriptors on shader change,
and forcing texture loads each fail the state comparison. The 65 alternate or
nonrendering source models are outside this fog audit, though included in the
ordinary parser compatibility test. No assertion is made that these alternatives
match live selected lights or that every actual fog-coordinate binding is known.

Compiler entry stack is 176 bytes on ARM; the descriptor walker uses 216 bytes
and its wrapper 56 bytes. These are per-function compiler reports, not a measured
whole-call-chain peak. There are no allocations, GPU calls, or per-frame work in
this compiler. The extended key is not yet stored in the shipped asset format.

Remaining: bounded resident group/palette integration, live material alternatives,
pre-stream world/normal matrix publication, morph-source updates and GPU ownership,
followed by image and performance checks. No application build, deployment,
visual acceptance or performance claim accompanies this stage. Sources and scoped
records are in `actor-fog-loader-test/source-snapshot.zip`, `result.json` and `completion.json`. Old/New installations remain unchanged.

14 October bounded group and packet compiler: `install_actor_fog_groups.c` now
combines the new source keys into native groups. It reuses reviewed startup
triangulation/batching through a hash-pinned generator, not the retired game-frame
renderer. Visible rebased bone entries and independent source-fog matrices share
the existing 22-entry shader palette. Groups split only at triangle boundaries;
source section order, materials and all visible vertex references are retained.

`install_actor_fog_pack.c` emits a 40-byte compact primary stream plus a 16-byte
companion stream. Companion positions retain original VAT scaling and contain no
visible bind-tail adjustment. Their matrix IDs address the source-matrix suffix
of the group palette. This requires the eventual publisher to supply actual source
matrices, including their position versus normal semantics. The existing visible
world-space palette is not interchangeable with the source camera-space matrix.
Both output buffers remain untouched if complete preflight fails.

The original-asset audit exercises 764 ordinary models with selected default or
indexed coordinates. 1,477 alternatives pass, preserving 1,502,916 ordered
corners and visible attributes. Fifty-one indexed stress alternatives reject
because the ungrouped source selector cannot resolve their inputs. These are
not live fog-material selections and do not establish missing game geometry.
Alternate/nonrendering models remain outside this fog audit.

The accepted alternatives require 3,024 groups versus 2,728 without fog, and
500,376 unique vertices versus 490,049. These increases (10.85% and 2.11%) are
inventory costs, not measured scene submission cost or FPS. The companion adds
16 GPU bytes per vertex. Maximum single-model combined streams and indices are
801,598 bytes; proposed metadata reaches 212,910 bytes. ARM compiler scratch is
408,128 CPU bytes during loading. Individual stack reports peak at 344 bytes in
the group compiler and 120 in the packer; these are not total call-chain peaks.
Scene residency and Old/New reserve gates are still required.

ASan/UBSan and ARM compilation pass. Deliberate visible-only merging, ignored fog
palette capacity and incorrect position scale fail. A later oversized-count
rejection guard passes 61 focused cases; a hash proof shows valid packing code is
otherwise identical to the corpus-tested version. No redundant full rerun was
used for that guard. The generation tool reproduces the tested group source bytes.
Evidence: `actor-fog-groups-test/result.json`, its `completion.json` and source ZIP,
and `actor-fog-pack-bounds-test/result.json`.

The compiled groups are not yet serialized into shipped assets or admitted by
the live owner. Source matrix/pose and morph publication, actual material choices,
fixed-plane coordinate space, shader/material handoff, visual verification and
performance gates remain. No application build or deployment occurred. The
Melee deferred-queue retirement rules were reviewed; future stream owners must
retain buffers until the actual GPU join, not just command submission.

14 October source-pose preparation: the new `pica_actor_fog_pose.c` calls the
existing optimized `C_MTXConcat` implementation rather than introducing another
matrix arithmetic backend. Position entries use the original unrebased pose.
Normal entries preserve a distinction in the original code: cached construction
clears translation after adjustment, while uncached construction clears it before
adjustment. Translated adjustment matrices make these results different.

The caller must supply the original cache-mode semantics. `modelInitMtxs` chooses
its cached path for total joint counts from 2 through 100. This is independent of
native uniform caching. The new actor path returns before the old bulk cache;
that work must not be restored merely to obtain the mode or extra-joint matrices.
A separate helper prepares only a required extra-joint matrix, matching the
original negative-bind-tail translations and quarter-weight blend. World preloads
still need explicit keys, distinct from original extra-joint indices.

Actual original `modelBuildPosNrmMtxs`, `renderOpMatrix`, `C_MTXConcat` and
`modelCalcVtxGroupMtxs` are the host oracle. Controlled tests pass 12,288 matrix
cases, 36,864 input/output aliases and 4,096 extra-joint blend/alias cases under
ASan/UBSan. Translated adjustments produce 2,048 genuine cached/uncached result
differences; mutations of translation order, adjustment and cached translation
are rejected. ARM11 compilation passes, reporting individual wrapper stack use
of 112 and 160 bytes. This is not a whole-call-chain high-water measurement or
new ARM execution test of the wrappers. Earlier ARM proof of the reused affine
kernel remains separate evidence.

Per changed palette entry, POSITION needs one affine concat and NORMAL two;
a selected extra-joint blend needs two tail-translation concats and twelve
weighted sums. No allocation, source-array write, vertex loop, GPU operation or
worker was added. Existing native changed-slot publication must own caching.
These helpers still need resident-owner and actual game-state wiring; no timing,
image, live matrix capture, app build or deployment acceptance is claimed.
Evidence and source archive: `actor-fog-pose-test/result.json`, `completion.json`
and `source-snapshot.zip`.

14 October resident packet and selected source-pose recipes: FHAFG002 stores
bounded CPU metadata separately from 16-byte-aligned GPU streams. Its identity
includes original model CRC/length/ID and the explicitly selected material variant.
The reader validates all extents, palettes, sections, original references and
pose-recipe indices once at load. It does not interpret source commands per draw.

Each distinct source matrix key has a 32-byte pose recipe. It preserves raw source
extra-joint numbering even when the visible renderer inserts a world slot. Blends
at weights zero and four still perform the original bind-tail translations.
The selected live helper reads only original 16-float-stride bone matrices and
calls the existing affine and extra-joint helpers. It does not restore the original
all-joint matrix cache. Actual source cache mode remains an adapter responsibility;
source_cache_eligible only records the count-based admission predicate.

Original source producers match 32,768 selected matrix results under ASan/UBSan.
Three deliberate numbering/endpoint/stride faults reject. ARM compilation passes.
The serialized packet audit preserves 1,502,916 corners over 1,477 alternatives;
source recipe tail/bone/weight bytes match the original model, and 60 identity/
extent corruptions reject. GPU extents are unchanged from v1; maximum metadata
is 220,544 bytes and GPU storage 801,846 bytes. Maximum metadata increase for any
packet is 4,608 bytes. These inventory numbers do not establish scene frame cost.

Measured requested allocation peak inside the host packet compiler is 3,469,973
bytes, with all tracked allocations released. Input model/harness, allocator
overhead, application services and live GPU allocations are excluded. This is not
physical Old/New RAM headroom. ARM individual stack reports are 848 bytes for
reader validation, 328 for the builder, and 72 for the selected pose helper;
whole-call-chain stack and scene residency still need measurement.

No app build or conversion recipe has been changed. Live material selection,
world-preload keys, source/morph publication, resident/fence ownership and actual
shader/material dispatch remain necessary before visual or timing acceptance.
Test scope, source snapshots and the cosmetic cache-field rename are recorded in
actor-fog-asset-pose-v2-test/completion.json. The original v1 packet sources and
results remain archived separately.

14 October world preloads: explicit WORLD_POSITION and WORLD_NORMAL source
keys now preserve the original position slot 9 / texture slot 19 entries. They
are not joint zero, nor an extra-joint index. Later source matrix loads overwrite
these slots normally. Ordinary world normal construction clears translation
before adjustment regardless of the skeletal cache mode. The world helper accepts
the actual object-world matrix; no per-vertex CPU transformation is introduced.

The pose compiler, compact packet packer and native reader admit these distinct
keys only for fixed-world models. The existing 32-byte pose recipe and per-group
indices are reused. Fur-transformed preloads are a separate unsupported variant.

The actual original ordinary preload block matches 16,384 generated matrix
results under ASan/UBSan. Source overwrite order and a controlled two-key packet
through the production packer/reader pass; malformed world aliases reject.
Three deliberately incorrect variants fail. ARM compile passes; individual world
wrapper stack is 16 bytes. The existing 32,768-case bone/extra-joint source oracle
also passes after the shared key-validation change. This is not live game,
GPU image, console performance or whole-scene resource acceptance.

Actual material selection remains necessary: source fog coordinates come after
environment/bump, projection or shadow/caustic branches, then layers/decal. Selected
light count alone is insufficient; null projection textures and shadow precedence
change coordinate counts, as do auxiliary/base texture widths in the bump path.
The corpus's default/last-indexed stress selections must not substitute for that
source behavior. No app build or deployment accompanied this change.
Evidence: actor-fog-world-test/{result,completion,source-snapshot.zip}.

14 October source material alternatives: `pica_actor_fog_material.c` selects the
ordinary fog coordinate and matrix routing from compact source facts. Projection
images and selected lights are distinct inputs: a null image consumes no coordinate
but its selected-light position still affects the source descriptor. Near-shadow,
cast-shadow and caustic branches retain original precedence. Bump coordinate
selection uses original image widths, authored repeat and the original enable bit;
it does not substitute downscaled native texture dimensions. Descriptor auxiliary
attributes remain present when bump evaluation is disabled.

`install_actor_fog_material_asset` now joins that selector to the real native
packet compiler and reader. It lazily compiles only referenced material alternatives
at load and seeds original world entries for fixed-world models. The original
stream's descriptor retention still belongs to the existing event compiler.
No per-frame GX interpretation or geometry reconstruction is introduced. ARM
load context is 1,108 bytes, released after compilation; individual stack reports
are 216 bytes for the load wrapper and 88 for the selector. These are not combined
scene RAM or total stack measurements.

The original ordinary material setup prefix through fog entry, its coordinate
constructors and descriptor producer match 32,344 valid controlled configurations;
424 exceed coordinate capacity and reject. This includes 27,345 default and 4,999
indexed fog routes. ASan/UBSan pass and four wrong-selection mutants reject.
GX calls are stubbed, so this proves branch/coordinate behavior, not colour or
rendering. A real two-material model (214, source hash recorded) passes the new
load entry in a controlled no-environment/no-projection state: two groups, 112
triangles, 2,304 metadata and 6,296 GPU bytes. ARM compilation passes.

Actual game-state capture, material admission, variant identity, resident/morph
ownership and shader dispatch remain unconnected. The ordinary selector does not
claim support for custom renderer callbacks or fur preloads. No new app build,
deployment, visual result or FPS claim accompanies this stage. Evidence lives in
actor-fog-material-test and actor-fog-material-packet-test; source snapshots and
initial test failures are recorded in the former completion.json.

14 October actual game-state adapter: `pica_actor_fog_game.c` now captures the
compact ordinary material/light facts using the real ObjModel, GameObject,
Shader and Texture ABI. It is not yet called by the production app. The native
entry precedes original objSetupLightChannels, so reading its projected-light
globals would use stale state. The adapter invokes only the original type-8
selection and retains shadow/caustic/override precedence; it does not restore GX
channels or diffuse selection. The original selection function changes light
scores and intensity, so this belongs on the game thread before publication,
once per admitted object, never on the render worker or when the effect is off.

A source flag can force a light slot even when no fresh selection exists. The
original reads retained global state in that case. The adapter explicitly rejects
that unowned case; matching retained-slot ownership remains work. Custom render
callbacks and inconsistent orphan layer-zero textures are also not admitted.
Original source image widths are captured, not native reduced texture sizes.

Actual original light policy with controlled selection results matches 22,896
states; 144 unowned-slot cases reject without changing output. Material capture
covers 176 controlled fixtures. Captured facts feed the original material/texgen/
descriptor comparison: 32,344 valid configurations match; 424 exceeding coordinate
capacity reject. Four deliberate adapter faults fail. ASan/UBSan pass, and the
adapter compiles against real ARM game headers. Individual stack is 48 bytes for
material capture and 88 bytes for selection; called light routines and complete
scene costs are excluded. No allocation, GX setup or GPU work was added.

This does not prove live resource ownership, actual light ranking/intersection,
shader dispatch, visible actor fog, or frame cost. No app build/deployment was
performed. Evidence: actor-fog-game-test/{result,completion,source-snapshot.zip}.

14 October resident storage: `platform_actor_fog_resident.c` owns immutable
prepared metadata and 128-byte-aligned linear geometry. Publication validates
and copies once. Direct lookup and draw access do not validate/copy again.
A failed replacement retains its prior generation. A successful replacement
flushes the new geometry, then uses the existing scene-storage join before
freeing old storage. Melee's early-queue completion rule was consulted: VBlank
alone is not a release fence. The existing owner handles queued-worker/GPU drain;
no new completion mechanism or timeout recovery policy is introduced here.

Global retained-packet caps are 1 MiB CPU and 2 MiB linear, subject to the normal
85% ceiling (including full main stack/margin) and existing Old/New linear
reserves of 1/4 MiB. Both generations count during replacement. Linear storage
rounds to 128 bytes and the rounded allocation is measured. Conversion scratch,
source models and allocator overhead are not packet-byte counters; normal-room
admission uses actual heap usage. The record costs 68 bytes on ARM, static stats
48 bytes. Publication individual stack is 160 bytes, direct lookup zero.

The real model214 fixture passes compiler, reader and this owner under ASan/UBSan:
112 triangles, 2,304 metadata and 6,296 GPU payload bytes (6,400 allocated).
327 resident replicas exercise the global GPU cap, reaching 753,408 CPU and
2,092,800 linear bytes. These are synthetic capacity-stress measurements, not a
game scene. Ten thousand lookups perform no allocation, flush or join. Allocation
failures, invalid replacement packets, memory reserves and identity mismatch are
checked; four fault variants reject. SDK ARM compilation passes. Host join/cache
stubs prove ordering only, not hardware completion or timing.

The owner is not yet attached to scene load/unload or actor drawing. No app build,
asset update, deployment, image acceptance or performance claim accompanies this
component. Do not put the source packet compiler on a per-frame path. Remaining
work includes material admission, bounded variant loading, dynamic source/morph
publication, shader dispatch and scene resource/performance gates. Evidence:
actor-fog-resident-test/{result,completion,source-snapshot.zip}.

14 October material owner: `pica_actor_fog_tev.c` compiles height fog into a
passthrough material stage 3, preserving stages 0..2 and alpha. This retains
source height-fog-before-colour-fade ordering: actor fade remains at 4 and distance
fog at 5. Occupied stage 3 rejects without modifying output; it is not overwritten.
`actor_fog_material_owner_patch.py` adapts the archived real material owner. Each
base Program reference-owns its compiled child variant. Explicit admission
prepares once; binding never compiles or allocates. Ordinary and fog bindings use
distinct program handles, so normal cache publication restores stage 3 on exit.
No separate overlay invalidation state is needed.

The new wrapper binds the existing secondary-light distance-fog route and avoids
replacing it with ordinary environment state. The prepared atlas uses sampler2/
UV2; its live colour updates the compiled stage. Other source texture stages,
raster state and actor colour fade remain under existing owners. Initial admission
excludes sphere/specular/diffuse-map, swizzle, special callbacks and materials with
occupied stage3. This is incomplete family coverage, not permission to drop their
geometry. Scene shader/geometry selection is not yet connected.

Actual owner cache/admission/release/wrapper tests pass ASan/UBSan: 10,000 retained
preparations, 1,000 lifecycles, shared variant references, capacity failure, 13
family exclusions, eight occupied-stage rejects and three deliberate faults.
The host wrapper stubs the full material binder; the complete patched TU compiles
against real ARM SDK headers separately. Static cost is 520 bytes for child
handles and binding state, within the existing 128-program table. Individual
stack: preparation 96 bytes, bind wrapper 32, TEV compiler 96; total chains and
scene cost remain unmeasured.

A headless asset-free GPU run uses the actual actor-fog shader and new compiler.
64 tiles cover compact/full fetch, independent fog-coordinate alpha gradient,
occupied base stages, fog ON/OFF, then colour fade and a synthetic distance post
stage. Maximum pixel error is one byte; actual and reference captures inspected.
This does not exercise the live distance-fog LUT, scene shader handoffs, gameplay
or physical-console performance. No app build/deployment accompanied this stage.
Evidence: actor-fog-material-owner-test/{result,completion,source-snapshot.zip}
and artifacts/diagnostic/actor-fog-material-pixels-v1/summary.json.

14 October native scene consumer: `actor_fog_scene_patch.py` adds an explicit
actor-fog shader program and draw entry to the archived native scene owner.
`platform_pica_actor_fog_draw.h` consumes the immutable 40-byte primary and
16-byte companion streams, visible bind-tail matrices and independent original
fog poses. Source planes use fixed attributes8/9. Ordinary lighting publication
is shared byte-identically through an always-inline helper. Existing material,
colour-fade, command-reserve and completion owners remain authoritative. Leaving
the entry resets its mode/binding so the next shader must publish its own state.

All material readiness and packet compatibility checks precede the first draw.
A rejected actor returns zero for the caller's ordinary native path, rather than
partly drawing it. No geometry conversion, source GX interpretation, SD access
or allocation occurs in submission. Source deformation/morph readiness and
original normal-cache mode still require actual game admission. Shader setup is
explicit at admission, not performed during a draw. Original source matrices
and visible-pose numbering are separate inputs; the caller must retain their
snapshots until the render worker finishes.

The actual authored model214 is compiled through both native scene and actor-fog
packet builders/readers, then the actual draw entry executes under ASan/UBSan
with captured SDK calls. All112 triangles/two groups submit; independent pose
arrays, bind-tail transforms, companion/index addresses, command-split layout/
plane republication, mode cleanup and four early rejection paths pass. This is
not a gameplay or GPU skinning image test. The complete patched scene/material
TUs compile against the real SDK; 14 existing shader blobs come from the exact
parent ELF. The actor-fog shader matches the already pixel-tested blob exactly,
so that GPU evidence is reused rather than rerunning unchanged tests.

New scene static state is56 bytes and the shader blob1,888 bytes; DVLB/runtime
allocation and scene headroom remain unmeasured. Individual stack is112 bytes
for shader initialization and216 for draw; full call chains are not established.
All fog palette entries are currently requested each group; no scene timing or
FPS benefit is claimed. Actual asset/game admission, bounded variant loading,
queued source-pose lifetime, morph publication and runtime image/performance gates
remain unconnected. No app build, deployment or hardware probe accompanied this
stage. Evidence: actor-fog-scene-test/{compile,completion,source-snapshot.zip}
and actor-fog-draw-test/result.json.


## 14 October: synchronous source publication and main graphics bridge

The actual main actor entry submits synchronously. It can borrow the original
pose array through the call; GPU uniforms are copied into command storage. A
future queued actor implementation would need an explicit snapshot or retained
source ownership, but adding a full-skeleton copy now would be unnecessary.
Pinned Melee early_queue.c ownership rules were consulted; resource retirement
and command submission remain with the existing native owners.

`pica_actor_fog_live.c` captures the current original fog source, camera planes,
colour and inverse root scale. The original ordinary main pass unconditionally
calls modelInitMtxs. Executing that original producer over 65,280 joint/extra
count combinations verifies the cached-normal choice without reading the stale
native global. The adapter keeps output unchanged on rejection, avoids source
queries when disabled, and rejects dynamic vertex/morph, callback, model-disabled
and indirect-fog cases that the immutable input does not implement. Four mutants
cover stale cache state, escaped local planes, wrong source flag and wrong pose
count. These are controlled source/host cases, not gameplay observations.

`actor_fog_live_patch.py` patches the archived actual main graphics owner and
composes the scene/material patch. The optional actor-fog entry uses the same
begin, native projection, scissor and accounting as ordinary actors. Rejected
fog restores ordinary native drawing. Texture-atlas selection was moved inside
the draw entry after complete material admission, eliminating unused preparation
on a rejected actor. No additional GPU shader instructions or pass were added.

The authored model214 integration check compiles ordinary and fog packets, calls
the actual live adapter and main graphics bridge, and executes the actual draw
entry with captured SDK calls: all 112 triangles/two groups, correct palettes,
forced command split, native projection, accounting, ordinary fallback, OFF and
clipped cases pass. Full actual SDK scene/material/graphics translation units
compile. The first full graphics compile lacked post_sample_shader.h; its failure
log is retained, and all required old DVLB banks were then extracted from the
exact parent ELF. Existing GPU pixel evidence applies to unchanged shader math;
this check adds no new game or physical acceptance.

ARM live snapshot is120B (including80B local matrices); adapter stack184B, main
helper176B and public bridge8B are individual compiler reports, not total stack
usage. No new heap allocation, skeleton copies, or worker is introduced by live
capture/bridge. Existing atlas admission remains bounded and conditional.

**Still unconnected in gameplay:** automatic packet/material variant admission,
real game call/build wiring, and source mutation ownership at that admission.
The adapter/bridge are not an app build or deployment. Next work must connect
load-time variants and actual game calls, then inspect scene images and measure
resources/cost; more isolated shader repeats do not fill this coverage gap.

Evidence: actor-fog-live-test/{result,graphics-compile,completion}.json and
actor-fog-live-bridge-test/result.json. No emulator/physical FPS claim.


14 October scene storage: candidate scene owner now attaches bounded resident fog
packets to their original scene identity, invalidates on source mutation, and
retires them before parent geometry on forget/shutdown. Actual owner/lifecycle
ASan/UBSan and full SDK ARM compilation pass; three ownership mutants fail.
Static scene-record growth is 2,048 bytes. No game caller or automatic converter
admission is connected yet; no app build or visual/performance claim. Evidence:
`actor-fog-storage-test/{result,compile,completion}.json`.


14 October compiler scratch reduction: the asset compiler now grows unpublished
geometry/reference buffers with emitted groups rather than reserving 2 MiB plus
256 KiB for every model. Growth is bounded, preserves prior bytes, and leaves
cleanup ownership intact on allocation failure. GPU-resident ownership is
unchanged. There is no per-frame compilation or new app integration.

Across 764 source models, all 1,477 admitted packets match the parent byte for
byte. The 51 unsupported cases are unchanged. Forced-moving realloc accounting
includes old and new allocations during growth: median requested compiler heap
falls from 2,855,616 to 509,640 bytes; maximum falls from 3,469,973 to 2,725,426.
These are host compiler allocations, excluding raw source, libc overhead and
live scene resources?not measured console commitment or a loading-time/FPS win.
Sixty-eight injected allocation failures leave no live allocations or published
output. The source-corner/pose oracle covers 1,502,916 corners, with 60 reader
identity/extent mutations rejected. ASan/UBSan and strict SDK ARM compilation
pass. Individual compiler stack is 328 bytes; group callback 160 bytes, excluding
call chains. Prior source/reader results and the parent are preserved.

Evidence: `actor-fog-staging-test/{result,completion,source-snapshot.zip}` and
`actor-fog-asset-staging-v3-test/result.json`. Actual packet/converter admission,
live game calls, visible parity and scene resource/performance gates remain.


14 October prepared material alternatives: a 164-byte immutable lookup maps the
original selected-light count and effective projected-coordinate count to an
existing packet variant. Preparing its 81 cells happens at conversion/loading,
using resolved original material facts. Deduplication compares full coordinate
and matrix routing for every referenced material; a hash match alone never
permits reuse. Frame lookup scans no materials and compiles no geometry. Actual
source light selection and its simulation-side effects remain with the game
adapter; the table does not replace them.

The source helper collects descriptor-setting materials as well as drawing
materials. A source shader can change without replacing the previous descriptor,
so omitting a descriptor-only material would select incompatible packets. The
load wrapper compiles a selected representative through the existing compiler.
Source-material-generation ownership, bounded residency and converter/game calls
remain unconnected; the 81 cells are not permission to allocate 81 packets.

Validation: 523,264 multi-material lookup states; 4,038 valid original-source
material/descriptor configurations and 58 coordinate-capacity rejects. Forced
hash collisions preserve exact behavior; three lookup mutants reject. All seven
prepared variants of authored model214 match explicit-selection packet bytes.
An inherited-descriptor fixture and its missing-event mutant validate both kinds
of source material dependency. ASan/UBSan and actual SDK ARM compilation pass.
Individual stack reports: plan preparation640B, source collector152B, packet
wrapper64B, live lookup12B; no total-chain or gameplay timing claim. Evidence:
`actor-fog-variants-test/{result,completion,source-snapshot.zip}`.

The user requested both Old and New console updates when ready. Those deployments
remain pending integrated application, visual/resource and shared-build checks.


14 October scene variant residency: the actual patched scene owner now retains
multiple prepared packets for one immutable source-material generation. Prepare
allocates CPU slot records only. Failed allocation retains the previous generation;
success retires it before publishing the new bank. Generation-qualified publication
and lookup reject stale alternatives. Source mutation and switched model views
still reject immutable fog input. There is no compilation, eviction, validation
scan or allocation in frame lookup. A lookup miss must remain an explicit native
fog omission until a packet is prepared outside submission; ordinary geometry
continues through the existing native caller.

All variants retire together using one existing scene-storage join, including
queued GPU consumers, before their allocations or parent model are freed. The
per-packet global CPU/linear caps and normal/linear headroom checks are unchanged;
bank CPU memory also goes through normal scene admission and accounting. ARM
bank storage is 172 bytes plus 68 per potential resident slot (648 bytes for the
seven-variant fixture), in addition to packet payloads. The previously added
2,048 bytes of static scene-record storage do not grow further.

Actual model214/compiler/resident/scene lifecycle tests exercise seven resident
variants, 10,000 selections with no allocations/flushes/joins, generation changes,
allocation and normal-budget failures, source CRC mismatch, source mutation,
model-view rejection and parent forget/shutdown. ASan/UBSan pass; four mutants
reject, including a missing batch join caught by pending-resource ownership.
Full actual SDK scene and resident translation units compile. Evidence:
`actor-fog-bank-test/{result,compile,completion,source-snapshot.zip}`.

This is not an application build or runtime performance result. Registration of
resolved model material generations, automatic converter/load admission, actual
game calls and scene visual/resource checks remain before the requested Old/New
deployments. No private assets or console files changed.


14 October automatic source facts: `fh_install_actor_fog_profile` mirrors the
original source resolver, shader initialization and live material adapter without
retaining relocated model textures. The width callback receives authored IDs and
must return original image dimensions. It is needed only for environment bump
routing; it must not substitute reduced native image dimensions. Successful game
texture loads and initialized distortion slots remain explicit prerequisites.

`fh_install_actor_fog_prepare` collects descriptor and drawing dependencies,
resolves only used records, and publishes the material facts and variant plan
together after success. It allocates nothing and belongs at conversion/loading,
never submission. Its ARM stack frame is 2,624 bytes; the reader uses 72 bytes.
These are individual static stack reports, not live scene memory measurements.

The unchanged original resolver/shaderInit/live adapter execute in the host
oracle: 5,632 resolved states, 1,536 rejected source configurations and 17 failure
cases pass ASan/UBSan. Two incorrect-reader mutants fail. Actual model214 yields
seven GPU packets byte-identical to the former explicit material inputs. A
descriptor-only malformed auxiliary reference catches an omitted collection
callback; invalid layer counts were unsuitable because the descriptor decoder
already rejected them. Evidence is in actor-fog-profile-test and
actor-fog-prepare-test. No application/visual/FPS acceptance is claimed: the
texture resolver, packet loading/generation wiring and game dispatch still remain.


14 October game dispatch: the isolated archive patch now connects the actual
native actor call to the prepared bank. Fog OFF bypasses the helper; zero,
unpublished or invalid generations avoid source fog and light queries. The
current object's culling result is produced earlier in the same generated
function. Successful dispatch consumes a stack-owned snapshot synchronously;
its graphics bridge owns any ordinary native fallback, preventing a second draw
at the game call site. Lookup misses compile nothing and read no files.

At original material-load completion, the candidate prepares child material
programs only if fog packets are already resident, and initializes the separate
shader only after a material is admitted. Packet loading must therefore precede
that hook. This load boundary remains unconnected; no application was linked or
booted, and this does not yet make actor fog visible. Source/material-generation
admission also needs live integration verification.

The bank's published flag adds four normal-RAM bytes per allocated bank. It keeps
an empty new generation or failed first publication out of frame work. Existing
GPU resource fences and caps remain unchanged. The dispatcher has a 200-byte
individual ARM stack frame. Host sanitizer tests cover 40 handoff states, and
the authored seven-variant bank retains the 10,000 allocation-free selections
and four failure-mutant checks. Generated game draw and model-load translation
units plus the SDK scene owner compile for ARM from the pinned archive. These
checks do not establish application linking, visual parity or performance.
Evidence: actor-fog-dispatch-test, actor-fog-dispatch-bank-test and
actor-fog-dispatch-compile. Existing accepted shadow stability is untouched.


14 October direct prepared loading: `fh_actor_fog_resident_load` accepts a
bounded reader over `[metadata][geometry]` and writes into final owned CPU and
linear allocations. It verifies the complete payload CRC and semantic/source
identity before publishing. Failed loads leave the prior resident and its GPU
consumers intact; successful replacement uses the existing scene completion
join. The scene entry additionally enforces the current registered generation.
No full packet staging allocation, extra geometry copy, frame-time file read or
geometry rebuilding is added. Budget checks still include both generations.
The successful-load byte counter adds eight static CPU bytes; actual SD timing
must come from filesystem telemetry, not this counter.

Actual authored packets pass host sanitizer short-read/CRC/identity/allocation/
budget and scene-lifetime tests, including nine deliberately incorrect variants
across the two suites. Real SDK ARM compilation retains 68-byte resident and
176-byte bank-header layouts. This does not yet supply automatic file discovery
or converter integration, nor an application build, visual acceptance or measured
SD/frame-time gain. Those remain required before the authorized deployments.
Evidence: actor-fog-stream-test and actor-fog-stream-bank-test.

14 October prepared bundle integration: `FHFGB001` files have a fixed 1,504-byte
checksummed directory, the exact source ID/length/CRC, the original bump-routing
bit, an 81-cell selection plan and bounded metadata/geometry entries. Payloads
are contiguous and independently checksummed. The writer uses the existing
on-device compiler, one variant at a time, and publishes the header last into an
empty temporary file. Final installation still needs the install transaction;
the writer deliberately does not overwrite or rename live assets.

The scene file loader validates the directory before any resident allocation,
then reads payloads into final normal/linear buffers. Failed variants never
publish; the returned resident count exposes partial admission. Registered
source identity, generation, global caps and GPU completion ownership remain
authoritative. Source changes retire through the existing scene lifecycle.
Directory validation is bounded; neither directory scans nor file reads occur
in draw lookup. File generations encode the original bump-routing bit; draw
dispatch rejects a generation if that bit changed since loading.

The isolated game patch discovers files in `actor-fog/` beside the selected
native scene pack, named `ID-LENGTH-CRC-BUMP.fhb` (eight-digit lowercase hex for
the first three fields). This preserves separate normal/replay and original/perf
asset roots. Loading happens before child material compilation, only with height
fog and actors enabled. The latest patch also avoids loading models rejected by
the live path's static source flags, missing joints, morphs or vertex animation.
Dynamic callbacks and dirty vertices remain draw-time admission checks.

Host source-to-file-to-scene tests retain seven byte-identical authored variants,
reject thirteen directory corruptions and mismatched source settings, and isolate
a corrupt GPU payload while retaining valid variants. GPU completion is a host
fixture here. SDK compilation measures individual stack frames of 4,312 bytes
for the writer, 3,032 for directory validation and 1,528 for scene file loading;
these are not total live stack measurements.

Private cave preparation found all 33 identities in the selected native scene
pack and produced 32 bundles (6,140,400 bytes total on disk). Model212 was rejected;
the preparation tool still lacks the external original-texture-width resolver.
This is a partial route fixture, not the complete automatic ISO conversion or
full actor-fog parity. Static admission also excludes many produced files from
current gameplay use; residency remains separately bounded at 2 MiB linear RAM.
Enabling height fog after a model has already loaded does not load its packets
from drawing; reload/restart behavior still needs UI integration before release.
Evidence: actor-fog-bundle-test, actor-fog-bundle-compile and actor-fog-cave-files-v2.

14 October live-admission corrections: two diagnostic cave runs preserved the
ordinary native scene but produced no actor-fog draws. Of 25,025 calls with a
published bank, 17,160 were rejected solely because `modelState` was null. The
other 7,865 reached graphics admission with no initialized actor-fog shader;
all eleven resident model families had prepared zero fog material programs.
Identity, active special-pass and morph-view rejection counters were zero for
those graphics calls. These are instrumented call counts, not performance data.

Original material setup dereferences `shadowCastSlot` only for projected-shadow
flag 4 after near-shadow precedence. The corrected adapter admits absent state
on other paths, retaining rejection when the source actually requires it. It
passes 256 new absent-state/precedence cases, 22,896 original light-policy cases,
144 unowned-light rejections and actual ARM ABI compilation.

Fog material preparation now supports ordinary texture-channel programs rather
than excluding any material capable of channel selection. Each compiled parent
owns its fog child; live identity and swizzled selectors choose the corresponding
prepared child. The first three combiner stages, constants and buffer masks are
preserved. Occupied final stages and unsupported sphere/environment combinations
remain explicit exclusions. Binding does not compile, allocate or add a pass.
Host owner tests cover 10,000 selector switches, retained preparation, release,
capacity failure and invalid family rejection under sanitizers; full SDK material
compilation passes. These tests do not establish visible scene parity.

Evidence: `actor-fog-bundle-v5-trace-new`, `actor-fog-bundle-v6-trace-new`,
`actor-fog-optional-state-test` and `actor-fog-channel-owner-test`. The first two
test wrappers accidentally wrote their results into prior component directories;
their new outputs were copied to the correctly named directories and the mistake
is recorded in `provenance.json`. Prior completion records and source snapshots
remain intact; prior generated result bytes were overwritten, not preserved.

The first channel-aware application (v7) still produced zero fog draws. Static
inspection found that constant-opacity materials own a diffuse alternative even
without environment lighting. Ordinary binding cannot select it when both
environment and layered mode are off. The corrected admission allows that
inactive alternative while explicitly rejecting layered fog binding. It does
not disable an active diffuse effect to force admission. This passes the v2
owner test, including retained selectors and incompatible-binding rejection.

Replay v8-trace, source `c9e82b1678de`, now records 52,195 fog groups in the New
cave test. Of 25,025 eligible calls, 2,860 retain ordinary native rendering due
to unprepared environment/specular materials; the prior optional-state and
shader-absence rejections are gone. Captures 1200 and 1410 were inspected:
Fox, cave geometry, textures, HUD and animated mist remain. This establishes
live submission without missing scene geometry, not an original-renderer pixel
match or full fog parity. The instrumentation and foreign host compilation
exclude timing attribution. No console performance gain is claimed.

New-profile sampled normal peak is 49,147,456/71,303,168 bytes, minimum linear
free 16,368,256/25,165,824, initialized VRAM free 4,723,712 and measured main
stack 73,120/524,288. The first analyzer incorrectly included an unmeasured
full-stack sentinel; it was corrected from the existing logs without a rerun,
and the initial report is retained. Process commitment is 103,448,576/130,023,424.
These are guest observations, not physical memory/performance acceptance.

Remaining work includes environment/specular composition, dynamic geometry,
full converter integration and late-toggle/reload behavior, a build without
diagnostic instrumentation, and measured cost/visual gates before deployment.
The file loader currently spends residency on families later rejected by
material admission; move that admission ahead of file loading before accepting
its Old 3DS resource cost. Do not bypass source checks or restore the old GX
renderer to make coverage pass.

The same v8-trace binary also completes the Old80 cave replay with 48,472 fog
groups, preserved saves, normal shutdown and inspected 1200/1410 captures.
Sampled normal peak is 41,610,808/50,331,648 bytes, linear free minimum
1,688,192/10,485,760, VRAM free 4,723,712 and main stack 73,120/524,288.
Process commitment is 67,796,992/83,886,080. The 2,656 material-family declines
retain ordinary native actor rendering. Timing is explicitly excluded; this is
not evidence of meeting the Old 20 FPS or New 60 FPS targets. Both consoles still
have normal build `22e57bb425dd`; no files were transferred during this work.

14 October admission before residency: clean replay v9, source `a42a1a8ff34d`,
tests material capability without acquiring a program, before opening the fog
file or allocating packets. It checks both ordinary channel alternatives; live
binding still selects the source-requested channel. All-incompatible models
50, 56 and 1257 no longer consume fog storage. A model with mixed compatible and
incompatible materials can still load: this change does not invent partial
object rendering or remove the live whole-object admission checks.

The v3 material-owner sanitizer test covers 10,000 allocation-free capability
queries and existing selector/refcount/lifetime failures. The generated game
load-gate test covers 896 combinations, including disabled actors/rendering,
unused materials, absent files and failed publication. Child programs are never
created before a published bank. The ignored-capability mutant is rejected.

Actual v9 New/Old cave runs complete with unchanged saves and owned exits.
Captures 1200/1410 were inspected for Fox, world geometry, textures and HUD;
animated haze differs by phase. Fog draws are 52,195/48,618. New matches v8's
count exactly; Old differs by 146 draws with a different frame count, not a
claim of newly supported materials. Both runs report peak fog metadata 282,368
bytes and geometry 910,464 bytes, 58 publications, zero allocation failures,
1,189,964 streamed bytes, and zero resident bytes/records after shutdown.

Old minimum free linear memory improves from 1,688,192 to 2,871,040 bytes;
normal peak falls from 41,610,808 to 41,281,400. New minimum free linear memory
is 17,551,104 and normal peak 48,818,064. Reservations remain Old 48 MiB normal /
10 MiB linear and New 68 MiB / 24 MiB. Both measured main-stack peaks are 73,120
of 524,288 bytes and initialized VRAM free is 4,723,712. These are sparse guest
observations for this route, not hardware bounds. Both timing comparisons are
excluded by observed foreign compilation. No redundant control run was started
against those already-contended measurements. The prepared same-binary
`--baseline-without-actor-fog` control and cost comparison remain unexecuted.

Startup output now also supports a streaming sink. The existing FILE compiler
uses it, and `fh_install_actor_fog_artifact` connects it to the existing
generation transaction. Packets go directly to staging, one variant at a time;
the header is finalized last. Only the outer transaction owner may activate the
generation. Partial writes resume from the zero checkpoint. An interrupted
finish with a complete checkpoint is validated and finished without rewriting
checkpointed bytes. Incomplete nonzero checkpoints are rejected.

The actual C converter and transaction pass ASan/UBSan with byte equality to
the archived seven-variant source fixture, sixteen partial sink-write faults,
interruptions before and after the finish checkpoint, active-generation
preservation, identity/bump mismatch and incomplete-checkpoint rejection.
Strict ARM11 compilation passes. Individual stack frames: compiler 4,312 bytes,
transaction validator 1,544, wrapper 376; these are not full call-chain maxima.
The validator uses a temporary 16 KiB CPU buffer, no linear/VRAM allocation and
no game-frame work. This does not measure conversion duration or SD durability.
The initial ARM compile rejected misleading indentation; corrected evidence is
in `actor-fog-transaction-v2-test`, with the original failure retained.

This transaction adapter handles one artifact, not the entire model collection.
The installer has eight artifact slots: do not allocate one slot per model or
create thousands of per-model generation directories. Collection packing,
source texture-width lookup, original/performance identities, startup recipe
and progress integration are still required. The writer changes postdate v9
and have not been linked into a new normal application. No new console files,
saves or settings were changed. Full visual/cost acceptance, remaining native
effect families and the original performance/playability goal remain open.

14 October collection conversion: `fh_install_actor_fog_collection_*` now writes
many model/bump records into one recipe-owned artifact. Each record contains
packet payloads followed by the unchanged bundle header and a 64-byte checked
footer. The footer links to the preceding complete boundary. No earlier bytes
are patched or copied; transaction checkpoints advance only after a complete
record. Batching uses the existing 32-record/1 MiB cursor mechanism. A failed
append is truncated to the last checkpoint on resume, and previously completed
sources are reused rather than recompiled. Caller cancellation must flush the
cursor before closing if it intends to preserve the current batch.

The collection is bounded at 4,096 records and 512 MiB. It costs 64 extra disk
bytes per logical bundle. Startup indexing keeps 56 bytes per entry, allocated
in powers of two (maximum 229,376 bytes normal RAM, with allocator transient
overlap possible during growth). It is not a linear-RAM or VRAM allocation.
The verifier uses a temporary 16 KiB buffer. These are explicit design bounds,
not measured gameplay headroom; global load ownership/budget admission remains
to be integrated before using the index in an application.

Indexing validates the footer chain, the shared bundle decoder, bounds and
duplicate runtime identities. Lookup is a binary search over the already
prepared index. Actual load uses `fh_native_scene_actor_fog_collection` and the
existing direct resident reader, CRC, semantic checks, cache publication and GPU
retirement. It retains neither the collection index nor its file handle after
loading. No collection work runs in drawing. The standalone file reader remains
available for existing fixtures and uses the same immutable header decoder.

Duplicate reuse includes a SHA-256 of the raw model and canonical resolved
material facts, including original image widths. Raw CRC/SHA alone is not enough
when another texture bank resolves the same model differently. Ambiguous inputs
are rejected, not assigned the first matching packet. This currently adds one
source/material preparation before the bundle compiler's own preparation;
neither is game-frame work. Resolver and collection integration still need a
whole-conversion cost measurement; no conversion speedup is claimed.

The v6 collection test executes the real compiler/transaction/reader under
ASan/UBSan with 24 records in one artifact, 12 resumed duplicate skips and 4,704
byte-exact packet chunks. It rejects 20 damaged footers, two checksum-valid
structural mutations, damaged payloads, truncation, identity and read bounds.
The prior active generation remains intact. A separate source-facts check
proves changed original texture widths change the input digest, and a missing
resolver fails without publishing a digest. Its first attempt tried to compile
a synthetic environment mutation of an incompatible geometry stream; that v5
failure is retained. The corrected scope tests the material identity directly,
without weakening the geometry compiler to admit the synthetic model.

The scene-bank test executes the actual archived native owner: seven variants
load, closing the index/file does not invalidate resident packets, a damaged
payload leaves six valid variants, and parent retirement joins and frees them.
Host GPU fences are fixtures. The subsequent input-digest change is verified by
the v6 collection test; the scene admission/lifetime body is unchanged. Full SDK
ARM compilation of that owner and strict ARM compilation of the collection
units pass. Representative individual frames are 2,696 bytes for input hashing,
1,688 for footer reading, 1,568 for index opening and 1,544 for scene admission;
these are not full call-chain maxima. No application build, emulator rerun or
console deployment was performed for this converter-only work.

Evidence: `actor-fog-collection-v6-test`, `actor-fog-collection-bank-test` and
`actor-fog-collection-integration-test`. Remaining integration is the selected
scene's index/file lifetime and RAM admission, ISO scan/texture-width resolution,
original/performance collection recipes, progress and late-toggle behavior.
All original visual, performance, gameplay and console-validation requirements
remain in force; this does not make actor fog or the full renderer complete.

### Application collection loading, 14 October

`actor-fog-collection-v1-console-replay-builds` now calls the collection reader
from actual model/material loading. A selected scene owns one index and file
for the renderer session. Repeated absent lookups do not reopen the file; a
changed path is rejected until shutdown (geometry selection already requires
restart). Closing the index never frees GPU packets: their existing joined
resident owner remains responsible. Pinned Melee `early_queue.c` lifetime rules
were reviewed; no fence, cache publication or GPU command changes were made.

Index admission uses the actual scene owner's 85% normal-heap/stack guard.
The maximum live index is 229,376 bytes. Admission reserves 360,448 bytes for
the largest index plus realloc's old allocation and 16 KiB for stdio/nested
load use. The actual allocated index is charged to scene CPU storage and
removed on close. The cache has no linear/VRAM allocation or draw-time work.
Its ARM loader has a 200-byte individual stack frame; that is not a whole
call-chain bound. The host test executes source conversion, transaction,
indexing, resident loading, failed/missing opens, budget refusal, fixed-path
ownership, repeated lookup, close-before-consume and shutdown-loop cleanup.
The full archived scene owner compiles with the real SDK.

The private cave fixture contains exactly the same 32 supported source
identities and packet bytes as the previous standalone files, plus 2,048 bytes
of collection footers. Its total is 6,142,448 bytes. The first preparation
attempt included previously rejected model 212 and correctly aborted with a
packet compilation failure. The accepted fixture explicitly excludes that
known unsupported identity; it does not prove a complete source collection.
Profile preparation succeeded for that failed input, so missing texture-width
resolution alone must not be asserted as its cause.

Shared replay source `ba9905f293f3` completes the actual New/Old cave runs with
52,195/48,472 actor-fog draw groups, eight resident model identities and one
index open. New count equals v9; Old count differs with submitted frame count.
Both runs use 1,792 index bytes, 58 packet publications, peak resident CPU/GPU
282,368/910,464 bytes, zero allocation failures and zero resident bytes after
shutdown. Captures at retraces 1200/1410 were inspected: Fox, cave textures,
ladder, mushrooms, tunnel and HUD remain present. Animated foreground mist
varies; this is not a new haze repair or full visual-parity acceptance.

Sparse guest memory samples: New normal 48,821,416/71,303,168 bytes, minimum
linear free 17,551,104/25,165,824; Old normal 41,284,792/50,331,648, minimum
linear free 2,871,040/10,485,760. Both retain 4,723,712 bytes VRAM and measured
stack upper bound 73,112/524,288 bytes. Process commitments remain New
103,448,576/130,023,424 and Old 67,796,992/83,886,080. These are route samples,
not exhaustive maxima. All prior GPU programs are unchanged. Both runs are
excluded from timing comparisons because foreign compilation was active.
No console FPS, physical memory acceptance or speedup is claimed.

The cache test's first wrapper failed to insert the cache code; the second
incorrectly expected the full shutdown counter reset from only the extracted
release loop. Those failures are retained. V3 tests the loop's actual scope;
the separately compiled full owner retains its final counter reset. No
production checks were weakened. Evidence is in `actor-fog-archive-v3-test`,
`actor-fog-archive-test/compile.json`, `actor-fog-cave-collection-v2`, the two
`actor-fog-collection-v1-*` runs and `actor-fog-archive-integration-test`.

No normal build or deployment was made. Whole-ISO source/texture discovery,
original/performance recipes, progress and generation-owned asset paths still
need integration; the current application hook uses the isolated fixture's
scene-relative collection. Late-toggle reload and clean cost/source-image
gates also remain open. Do not deploy this cave fixture as complete conversion.

### Source preload repair and original texture widths, 14 October

Model 212's collection failure was traced to a validation mismatch, not missing
image dimensions. The source marks it as using vertex animation, and the
material compiler correctly seeds WORLD_POSITION/WORLD_NORMAL in slots 9/19.
`fh_install_actor_fog` still rejected all initial kinds greater than NORMAL,
before its first descriptor callback. It now uses the same symbolic-key
validator as matrix selection, with explicit zero-only unknown entries.
World keys retain their distinct meaning; they are not mapped to joint zero.
The original preload is visible in `objprint_dolphin.c` at the ordinary
vertex-animation branch (position, normal and texture matrix slot 9).

The actual original and performance source bytes for this model are identical.
The repaired compiler produces all five variants, each retaining 1,632 source
triangles. ASan/UBSan tests follow all 35 source draws, verify the preloaded
world entries survive the authored matrix loads and reject 100 malformed
preloads before callbacks. Reinstating the old validator fails this test.
The first test wrongly required a world key to be selected by these particular
fog coordinates. The source preload is valid but remains unused by the fog
materials; the corrected test checks that distinction. No runtime admission
guard was relaxed, and this does not enable static fog on deformed actors.

`actor-fog-cave-collection-v3` now prepares all 33 requested identities. V4 adds
the original-width reader and verifies 355 repeated raw identities through the
C owner's resolved-material digest, rather than skipping them in Python before
checking texture-bank context. Its 6,676,076-byte collection is byte-identical
to V3; the original 32 standalone bundles remain byte-identical, with one new
five-variant model-212 record (533,564 logical bundle bytes). These are private
conversion fixtures, not shipped game data or complete-game conversion proof.

`install_actor_fog_width.c` resolves original model texture widths from explicit
ISO TEX1 table/data FST entries. It follows the model loader's 15-bit bank index
and out-of-range-to-zero behavior, handles base-level indirection in mip tables,
and caches 64 entries within one selected bank. Compressed reads stop after the
96-byte source header; they neither allocate nor convert full pixel images.
This is metadata access, not complete compressed-stream/checksum validation.
Whole-disc fingerprinting and full texture conversion remain necessary.

Zlib workspace has a 65,536-byte cap including its aligned allocation headers,
excluding libc bookkeeping. The measured host allocation peak is 39,992 bytes;
this is not a measured console allocator peak. ARM compilation passes, with
1,272 bytes for the individual reader frame and 176 for the matrix compiler,
not whole-call-chain stack maxima. There is no new draw-time, linear-RAM or
VRAM work. The explicit ISO reader preserves the existing startup disc owner;
GPU publication/fences and the pinned Melee lifetime adaptations are unchanged.

The reader matches 23 independently decoded original textures from actual ISO
banks. Synthetic tests cover 76 successful widths/cache hits and nine rejected
records, stored/compressed/mip containers, malformed extents, output retention,
index aliases and cache replacement. A damaged trailing checksum intentionally
does not invalidate an already-read metadata prefix; this limitation is explicit.
The actual source-width-dependent model 764 also prepares three collection
variants with the ISO resolver. Its profile fails without that resolver.

Evidence: `actor-fog-world-compile-v2-test`, `actor-fog-width-v2-test`,
`actor-fog-cave-collection-v4`, `actor-fog-width-collection-v1` and
`actor-fog-source-width-integration-test`. No app build, emulator rerun, FTP,
save/settings changes or deployment occurred for this converter work. Latest
local replay remains `ba9905f293f3`, installed normal apps `22e57bb425dd`.
Full ISO original/performance stage wiring, model-bank selection, recipes,
progress/activation and late toggles remain unfinished, alongside renderer
parity and performance targets. No new visual or FPS improvement is claimed.


14 October ? complete ISO actor-fog preparation, local only

`install_actor_fog_iso.c` now feeds the real ISO geometry scanner into the
transaction collection writer. It produces both bump states, supports orderly
cancellation/resume, and reuses prepared topology through a startup-owned
reader. It does not rerun mesh reduction, extract files or add gameplay IO.
The caller must bind the source fingerprint, converter recipe and topology
identity to its generation; startup task/bundle activation is not connected yet.

The source loader merges live-area TEX1 tables. Nonzero entries without the
on-disc presence flag can be placeholders, so selecting a model-directory
texture blindly is not sufficient. The new width catalog enumerates the
53 paired ISO TEX1 banks and accepts a requested width only if all physically
present definitions agree. Ambiguity/malformed definitions fail explicitly.
It keeps 64 resolved IDs, reads metadata prefixes using the existing bounded
zlib reader, and never substitutes a reduced texture's dimensions. The selected
fixture preparation tool now uses this resolver too. Original source references:
`pi_dolphin.c:mergeTableFiles/tex1GetFrame`, `texture.c:textureLoad` and
`model.c:modelSyncResourceTables`. Pinned Melee `port/3ds/file_io.c` startup
inventory/owned-reader discipline was consulted; no OS/GPU owner changed.

Actual ISO audit: 3,325 model occurrences, 26 with no main render lists/materials,
6,598 nonempty model/bump profiles accepted, 1,670 unique identities. All required
widths agree; 19 uncached width requests checked 80 physical definitions. The
independent width fixture checks 23 requests, including aliases/repeated IDs.
Synthetic tests reject conflicting dimensions, malformed images and missing
paired banks, and ignore a nonresident placeholder with a different width.

Both full native collections pass ASan/UBSan, packet verification, cancellation,
resume and completed-stage no-IO checks. Original: 1,670 records / 339,143,292
bytes, SHA4976e4f7be9b33b4. Performance: 1,670 records / 338,279,244 bytes,
SHAa264c055e8f27124. Every expected original and post-topology identity is present
in both bump states. The existing 33 cave bundles are byte-identical, and the
width-dependent model764 fixture is unchanged. These are private generated
assets. The combined ~646 MiB disk cost still needs inclusion in installation
storage/space estimates; no console or full-conversion time claim follows.

Earlier valid topology packs use discovery order. The new reader validates
contiguous extents and directory CRC, sorts its owned index, rejects duplicates,
and validates each selected patch before modifying a model. All 3,325 model
occurrences match the independent existing Python converter byte-for-byte,
including 184 changed occurrences; nine corrupt cases reject, and a rejected
patch preserves its source. Empty original-topology packs are supported.

Sparse ISO table slots no longer cause journal commits. Both complete outputs
remain byte-identical: original journal serial4031->448, performance4035->447.
These are transaction publications, not literal SD syscall counts. Latest host
sanitizer runs took71.81/66.16seconds; those durations are not console estimates
or controlled performance comparisons. Compilation/host load was not isolated.
An interrupted first bump-state checkpoint points to the same source model;
the cursor only advances past the model after the second state is complete.

Strict ARM compilation passes: individual stack frames ISO stage288B, topology
open80B/apply72B, width-catalog enumeration1616B/query600B. These are not complete
stack-chain or console heap measurements. Topology directory cap96KiB and
transient patch cap1MiB; zlib workspace cap64KiB (host observed39,992B) remain
startup-only. No new shader, GPU/linear buffer, draw work, worker or emulator run.

Evidence: `actor-fog-iso-integration-test/{result,source-snapshot.zip}`, source
audit-v2, width-catalog-v1-test, topology-v2-test, ISO-v5 original and ISO-v6
performance. Earlier failures are retained: a real unsorted-pack rejection and
incorrect test assumptions about nonzero cursors/payload before the first actor;
no production guard was weakened to accept invalid input. Audit-v1's diagnostic
flags column used the wrong header word; v2 corrects it, unchanged admission.

Next connect recipe-owned generations and original/performance asset selection
without invalidating unrelated texture/movie/geometry preparation, then shared
normal/replay builds and deployment checks. Static runtime admission still
excludes dynamic/environment families; compiling their source packets does not
establish live visual parity. Late toggles, remaining rendering families,
physical performance and complete gameplay/save/endurance goals remain open.
Both deployed normal apps remain22e57bb425dd; latest local replayba9905f293f3.

The focused `actor-fog-iso-cursor-v1-test` interrupts the actual stage immediately
after its first bump-zero publication, then resumes and validates both packet
states. A mutant that advances to the next model too early is rejected. This
checks the actual producer's cursor rather than only seeding a synthetic correct
checkpoint. No additional full ISO conversion was needed for this assertion.

15 October: lossless collection storage and bounded loading

New FHFCL002 collections store independently compressed 16 KiB LZ4 blocks,
followed by a bounded block directory and the original logical bundle header.
The converter writes these blocks directly to its existing transaction; it does
not first write a raw collection or keep a second SD copy. Record checkpoints,
source identities and decoded packet CRCs remain authoritative. The runtime
also accepts existing FHFCL001 collections. GPU geometry and shaders are not
changed by compression.

Both complete private collections were repacked by the actual C writer and
checked through the actual C reader under ASan/UBSan:

| Geometry | Previous bytes | Compressed bytes | Exact packets |
| --- | ---: | ---: | ---: |
| Original | 339,143,292 | 164,784,780 | 11,093 |
| Performance | 338,279,244 | 164,312,156 | 11,093 |

Combined storage drops from 677,422,536 to 329,096,936 bytes (51.4%). These are
private, derived assets, not redistributable seeds. All decoded bytes and bundle
plans match. Both compressed and incompressible raw-block routes are covered:
80/96 raw blocks and 21,268/21,201 compressed blocks respectively. Directory
offset/length/mode damage rejects; packet CRC failure prevents publication.
Actual converter cancellation/resume and source-identity checks still pass.

The final load callback reads one relevant directory slice and streams payload
blocks forward. Each metadata/geometry request uses three libc seeks regardless
of block count, instead of the prototype's repeated directory/payload seeks.
These are libc-call counts, not SD IPC counts. Both complete collections passed
the resident owner's two-request pattern: 22,186 requests per collection and
672,185,416 decoded bytes compared in total. No decompression occurs in draws.

The host writer used 114,720 bytes of transient normal RAM; the production
writer bounds its queried LZ4 state and total workspace to 163,840 bytes.
The final reader used at most
33,176 transient bytes on these actual packets, with a format-wide 98,304-byte
limit explicitly reserved by resident admission. The buffer is freed before
GPU publication; existing resident counters exclude it. No added linear RAM,
VRAM, worker, draw, shader instruction or GPU transfer. Original packet CRC,
range flush and joined retirement remain with their existing owners, following
the pinned Melee early_queue ownership discipline.

Evidence: actor-fog-compression-v1-test (initial reader),
actor-fog-compressed-packets-v1-test (final streaming reader),
actor-fog-collection-compressed-v4-test and
actor-fog-collection-bank-compressed-v1-test (actual resident owner, failed CRC,
file/index lifetime and retirement). Individual ARM stack frames remain small;
read1640B/footer1696B are separate frames, not a complete stack-chain bound.
The first compression report incorrectly sampled source hashes after a reader
edit; its preserved pre-edit compiled-sources proof corrects that attribution.

All four existing base conversion keys remain unchanged. The new dependent
actor-fog recipe includes the actual LZ4 library identity. The isolated effects
builder also rebuilds both weather and particle game adapters so it retains the
deployed Fast-preset CPU gates. These storage tests do not establish physical
loading time, whole-conversion time, gameplay FPS, or full actor-fog parity.

15 October: normal dependent-generation activation is now exercised with the
actual completed ARM base bundle and matching original/performance fog files.
The performance collection uses committed topology SHA7aa59eb2d4cd (1,193,256
bytes), not the previous legacy mesh pack. Full compressed performance output
is164,035,624bytes; original164,784,780bytes unchanged. Private integration
publication uses the actual archived transaction, dependency key, bindings and
collection reader under ASan/UBSan; incomplete/wrong-key generations reject.

Normal2200189c7ceb retains Old48/12 heap repair and passes Old80 startup from
this prepared fixture through intro/cockpit/Select. Ready-menu and game images
inspected; normal prepared-asset paths accepted, no texture errors. This run's
Custom default has height fog off, so it proves installer/game handoff rather
than new fog visual parity. Actual process71,053,312/83,886,080bytes;
heap39,051,680/50,331,648, linear min free5,930,368/12,582,912.

The runner's final fixture.files KeyError lost summary serialization after clean
shutdown; recovered checks are in actor-fog-normal-ready-v1-old/post-validation.json.
No repeat boot, invented elapsed time or pre-run filesystem timestamp. The
fixture inventory was repaired after source/runtime hash comparisons; future
runs reject missing inventories before boot. This is an explicitly synthetic
publication of real converted files, not a new full on-console conversion.
Full added-phase ARM timing and the total70minute limit remain unvalidated.
The latest preset policy (Lighting/Environment/Specular ON in all four profiles)
must be included before promoting this feature candidate;2200189 predates it.

15 October: measured ARM preparation prefix and append-only startup index

`actor-fog-arm-cost-v3-new` finally reached the new converter stage after source
verification325.322s. The baseline ARM app spent75.504s in original-fog before
ordinary HID cancellation and clean exit (414.892hostsec overall). It retained
63,734,506bytes of actual checkpoints. The before-cancel image shows18% of
120,800 table slots; slot counts are not a time-weighted whole-conversion ETA.
No physical timing or complete70minute acceptance follows from this prefix.
Original/base/save fixtures are unchanged; peak startup arena2,392,064bytes.

The local startup collection now appends entries and uses a bounded16KiB
index rather than sorting/moving the full entry vector for each insertion.
Actual performance collection order1670records formerly moved41,303,584bytes
and made657,647linear insertion comparisons. Those sorted insertion moves
are removed; ordinary vector-capacity reallocations remain. The lookup compares
all four fields and retains input SHA validation, compressed bytes, checkpoint
ordering and the normal runtime reader. No draw-time allocation or GPU change.
Existing24-record actual transaction/4704packet chunks, resumed duplicate skips,
malformed payload/footer/identity tests and strictARM compilation pass. Additional
actual-index tests cover captured order,200400repeated lookups,4096capacity,
64deliberate collisions, corrupt indices and bounded full-table rejection.
Full real-ISO performance conversion passes in actor-fog-iso-append-index-v1-test:
all164,035,624outputbytes match the preserved parent;1670records/4928resumed
duplicate skips. Original variant was not rerun; no timing improvement claimed.
No new application build or deployment of this converter change yet.
15 October: retain the installed actor-fog pipeline across compatible actors.

The `actor-fog-retention-v1` pair changes only the fog draw owner and its
shutdown counter. Ending an actor no longer discards the actual installed
program. The next actor can retain it; ordinary/HUD/GS/target/command-buffer
handoffs still invalidate or select a different owner. The first draw continues
to publish live lighting, palettes, material, scissor, attributes and fog planes.
Camera rows use the existing validated scene-camera owner. No geometry, GPU
program, game simulation, asset, pass, worker or resource lifetime changes.
The only new storage is a four-byte diagnostic counter. Pinned Melee's
`uniform_upload.h` actual-installed-state discipline was consulted.

The actual authored model214 entry passed 800 retained/split draws and four
preflight rejects under ASan/UBSan, with exact geometry/palette output; three
incorrect-state mutants failed. The actual pipeline dispatcher passed 10,000
owner cycles and four invalidation mutants. SDK calls are modeled in these
tests; this is not physical GPU validation.

Old80 cave candidate passed status1, save preservation, log drain and owned
exit. Both1200/1410 captures retain Fox, cave, ladder, mushrooms, tunnel and HUD;
animated mist differs, so full image parity is not claimed. First baseline was
contaminated by a foreign build and excluded. Replacement baseline had no build
activity observed at boundary samples. For the same step3,60..540-simulation-tick
window, native submission was6.0547 ->5.8648ms and exclusive main-thread total
17.7582 ->17.5673ms. These are single-run Azahar guest timings, not physical
throughput or a universal hardware multiplier. Draws/frame271.840 ->272.533,
indices/frame82144.749 ->82132.868: animation/phase differences remain.

Full-run pipeline publications29060 ->13961, actor-fog draws48399 ->48910,
with15410 retained fog entries. The roughly52% reduction in pipeline setup
does not mean52% less rendering time. Uniform uploads actually increase
1468379 ->1484455 across the differing-length runs. The benefit is modest
in this emulator scene and applies to height-fog actors, not the whole physical
69ms preparation/submission bottleneck. No new hardware test requested.

Sampled Old normal heap42003728/50331648, linear min free4188800/12582912,
VRAM min free4723712, measured stack72984/524288, process69898240/83886080
are unchanged. This is a cave fixture, not worst-case gameplay headroom.
Normal1a732a666be0 and replaya8821d9c76ff contain685 matching source records,
18 unchanged GPU programs and identical complete gamearchive2f219edec50e.
Both retain Old80/New124 CIA grants and unchanged setup recipes; root/archive
restore checks pass. Exact normal startup/conversion was not repeated because
the shared renderer is the only changed runtime code. Neither build is deployed.

Reproduce with `build_actor_fog_retention.py`, `test_actor_fog_retention.py`,
`test_actor_fog_retention_owner.py`, `run_actor_fog_retention.py`, and
`analyze_actor_fog_retention.py --baseline actor-fog-retention-baseline-v2-old`.
Pair identities are in `actor-fog-retention-v1-pair.json`; test/runtime evidence
is in `actor-fog-retention-test/` and `actor-fog-retention-owner-test/`.
`verify_actor_fog_retention.py` retains the full renderer feature-loss guard.
Future actor-fog install builds include this owner change. Full renderer parity,
physical performance, sustained progression/save/endurance remain incomplete.
