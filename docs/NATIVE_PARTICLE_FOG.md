# Native particle distance fog

The original `drawGlow` in engine 10 (`expgfx.c`) calls `_gxSetFogParams` before
its particle pools. That publishes the game's exponential distance fog. The
native particle adapter removed the GX setup, but its replacement reset all six
texture-combiner stages without restoring native distance fog. Particles therefore
missed an effect already applied to ordinary geometry.

The replacement consumes the existing native fog range and colour, published
directly by `fogSetRange` and the colour setter. It does not call the GX renderer.
The geometry shader exports normalized linear eye depth on texture coordinate 2;
the existing procedural fog LUT and final combiner stage consume it. Particle
opacity, blending, depth tests, simulation, random calls and generated corners
are unchanged. The existing DISTANCE FOG option controls it. Disabling that option
avoids LUT generation and disables the fog combiner.

The shader remains 511 instructions: an exact power-of-two alpha-wrap multiply
and subtract are folded into MAD, making room for the depth export. The actual
linked interpreter tests compare all prior position, colour and UV outputs
bit-for-bit, including negative alpha wrap, dynamic quads and poisoned scratch
registers. Both template and dynamic cases pass 27,648 emitted vertices each.
The added coordinate is independently checked against linear eye depth.

The owner restores fog after its combiner reset and republishes the scale in
geometry uniform 20 when the texture extent or fog scale changes. It uses the
same existing shader rebind/invalidation discipline as the verified particle
handoff. The pinned Melee SDK binding reset and the pinned Citro3D uniform owner
were consulted; no direct register writes, extra worker, vertex rebuilding,
additional geometry pass, allocation or new texture are introduced. There is
one additional four-byte cached scale. The added fragment fog lookup and varying
export still have GPU cost; no physical performance claim follows from instruction
count alone.

`test_pica_glow_fog_owner.py` exercises 600 water/particle ON/OFF handoffs against
the actual native fog owner and SDK LUT encoding, plus 200,020 exponential-fog
comparisons. Removing particle invalidation is detected. The first build failed
because the stable snapshot lacked the fog-scale accessor present in unrelated
work-in-progress; V2 explicitly includes that accessor and declaration. It imports
none of that unrelated rendering work. An initial assembly attempt had an invalid
MAD operand order; the accepted shader passes assembly and complete-output checks.

Evidence: `artifacts/research/pica-scene-rewrite/glow-fog-shader-test/`,
`glow-fog-owner-test/`, and `glow-fog-v2-builds/`. The isolated builder preserves
all worktree files and the game archive. Runtime cost/image acceptance is recorded
separately; this is not yet hardware acceptance or the pending skybox repair.

The exact replay build `6ee2e2f05bec` completed serial New/Old cave checks with
status 1, save preservation, drained logs and owned-process exit. Both frame-1410
images were inspected: cave geometry/textures, Fox, torch particles, ladder and
HUD remain visible. Haze still varies with particle phase. These images do not
prove original-renderer parity, and this nearby scene does not isolate distant
particle attenuation visually.

| Azahar cave check | New | Old |
| --- | ---: | ---: |
| Scene preparation, ms | 1.875733 | 1.850909 |
| Native submission, ms | 5.560080 | 5.578334 |
| Change in their sum vs sampler V2, ms | +0.022380 | +0.023191 |
| Incremental CPU budget, ms | 0.20 | 0.50 |
| Guest measured FPS | 59.831223 | 57.003025 |
| Heap peak / reserved, bytes | 48,409,000 / 71,303,168 | 40,867,984 / 50,331,648 |
| Linear minimum free / reserved, bytes | 18,441,856 / 25,165,824 | 3,761,792 / 10,485,760 |
| VRAM minimum free, bytes | 4,723,712 | 4,723,712 |
| Measured main stack / reserved, bytes | 73,008 / 524,288 | 73,008 / 524,288 |
| Process commitment / limit, bytes | 103,301,120 / 130,023,424 | 67,649,536 / 83,886,080 |

These are guest/emulator measurements, not physical-console FPS or host playback
rate. Animation and particle workloads differ between runs: New quads
268,581→273,131; Old 243,581→247,173. They are short incremental-cost checks, not
instruction-identical A/B comparisons. There are no new allocations in this
change; the New run's observed heap peak is 4,376 bytes higher. Linear/VRAM,
stack and process measurements match the preceding sampler checks. Eleven fog
curve rebuilds per complete run remain unchanged. Evidence and reproduction:
`check_glow_fog_v2.py`, `summarize_glow_fog_v2.py`, and `glow-fog-runtime-test/`.

## Cave haze investigation

In `artifacts/native/cave-haze-particle-toggle-new`, a single ordinary-game session
switched particles ON/OFF/ON/OFF/ON through actual touch input, leaving both fog
options enabled. Both OFF captures were clear. Haze appeared in the later ON
captures; the first ON capture was clear. Read-only sampling found unchanged
height-fog intensity, height bounds, and distance normalization. This identifies
particle-dependent animated haze, not proof that mist is incorrectly drawn.
Do not remove it or change particle alpha to manufacture matching images.

The game exited through Select, drained logs, preserved its isolated save and
terminated its owned emulator. The wrapper nevertheless failed a requested
original-geometry cache check because this fixture uses performance geometry.
Its failed summary is preserved. `validate_cave_haze_evidence.py` validates the
narrow visibility experiment separately without claiming a full runner pass;
the unused cache argument is removed from future runs. This experiment is not
performance evidence and does not establish that particle distance fog explains
all haze differences.


## Ordinary CPU corner preparation retired, 14 October

The native particle consumer already chooses a resident quad template and shape
ID on the GPU. Engine 10 nevertheless kept rebuilding ordinary quad XYZ bytes
in every particle update. `pica_glow_override.retire_static_quads` removes these
five static branches, retaining their eight UV writes. Dynamic trail and locked
rotation branches remain because the current native dynamic consumer uses their
corners. This is an explicit remaining CPU geometry cost, not full retirement of
all particle geometry preparation.

The source slot's first 64 bytes overlap simulation fields. The change leaves
those lifetime, sequence, impact, colour and alpha bytes intact; it neither
clears the block nor reuses the retired XYZ lanes. Original spawn initialization
remains. Static-to-dynamic transitions fully prepare their dynamic corners in
the unchanged update branches before the native draw consumes them. Source
physics, random calls, callbacks, source-vector integration and pool snapshot
rules remain unchanged. No GPU commands, shader changes, memory, workers or
cache-coherence changes were added. Pinned Melee's explicit resource/command
publication rules remain in the existing owners.

The existing eight-frame profile attributes 217,258 instruction encounters to
the former static branch: 15.62% of this function and 0.894% of main-core
encounters. This includes UV work retained by the replacement, excludes called
functions, and is not a time saving or FPS estimate. `analyze_arm_source.py`
uses the exact linked ELF and DWARF5 source MD5, preserves core identity and
reconciles its totals with the original block analysis. Initial DWARF5 file
indexing was corrected before using the attribution. `verify_particle_generated.py`
reconstructs the checksum-matching profiled translation unit by reversing only
this branch change; all other generated source is identical.

`test_particle_static_retirement.py` passes 65,536 actual-source cases under
ASan/UBSan, verifies all 136 retained slot bytes and both authored templates,
and rejects a mutation corrupting the aliased alpha field. The host oracle
expresses negative signed shifts as multiplication, within the authored signed16
range. Production source outside the retired branch is unmodified.

Local shared builds: normal setup c1ee1a7c39ad (3dsx bc298414ca4f, CIA
e7d0cb9d45b4); replay dbb073db35e7 (3dsx 8f303f85fde4, CIA fe5349bd1669).
They are **not deployed**. Their 619 source hashes match and all 17 GPU programs
match the parent. Only the expgfx object changes. Old80/New124 CIA profiles and
setup recipes remain. The archive payloads match between variants, but SDK ar
inserted different modification timestamps in five header bytes. The verifier
checks every other byte including symbol indices and object order; it does not
claim identical archive SHA256 or full byte reproducibility.

The New Thorntail route and Old palace route finish in 90.89/53.31 host seconds,
with status1, unchanged saves, drained logs and owned emulator exit. Inspected
New1770 and Old1410 captures retain terrain, actors, HUD and particle effects;
87,529/228,600 particle quads were submitted. Both runs observed concurrent
foreign compiler activity, so their timings are excluded from comparisons.
The build adds no allocations; exact sampled budgets and evidence are recorded
in `particle-static-retirement-test/runtime.json`. Normal setup shares the
checked game payload but was not freshly booted or converted this turn.

These changes do not resolve the user's whole-shadow flicker, establish hardware
performance, or complete missing particle/reflection and broader renderer parity.


## Live cave particle comparison, 14 October

The changing foreground haze is not established as a renderer error. A bounded
read-only observation of the exact height-fog-handoff-v1 replay captured eight
last retained particles with coherent camera/material state. The live shared
HUD opacity was 255 in every sample. The source and native blend/depth setup
agree; the original Aurora quad conversion uses the same diagonal.

The captured page-145 particles use large world-plane templates, with some
corners behind the near plane. The original C vertex loop and trigonometric
function were checked against vendor source, then compared with the exact linked
vertex and geometry programs through Azahar's float24 interpreter. All 48
emitted corners pass: maximum relative component error 3.11459e-8, alpha error
0.0000152 byte levels, and zero projected-pixel error at reported precision.
Removing near fade in the interpreter test correctly fails the comparison.
The original near-camera alpha wraps; the native shader reproduces it. Changing
that alpha to make one screenshot clearer would depart from the source.

The inspected frame-1410 image has clear Fox/foreground with particles ON,
while low tunnel/ground mist remains. Earlier ON images were hazier. These are
different animated phases, not a controlled full-image comparison. Texture
sampling, clipping, blending and occlusion are outside the vertex proof. No
hardware fidelity or performance improvement is claimed.

The observer initially failed because linear memory needed separate verification,
pending batches were too brief, and specular/diffuse use identical sphere meshes.
Its final mapping proof checks independent immutable HUD geometry as well as
both executable anchors, heap identity, process ownership and live retrace.
It never chooses an arbitrary first match. Failed observers v1-v4 are preserved;
v5 succeeds. All five game runs exit normally with state/save/drain checks, but
all timings are explicitly excluded because the observer perturbs execution.

Evidence: `artifacts/native/particle-cave-state-v5-new/particle-validation.json`,
`particle-analysis-v2/result.json`, and
`artifacts/research/pica-scene-rewrite/particle-state-test/mapping.json`.
Tools: `capture_native_particle_state.py`, `analyze_native_particle_state.py`,
`test_linear_hud_mapping.py`. No production renderer or console installation
changed during this investigation. Whole-shadow flicker is separately accepted
as fixed by the user; older unresolved statements in this document are history.


17 October: packed lifetime shader candidate

17 October PARTICLE LIFETIME VSH candidate; goal ACTIVE/INCOMPLETE, no blocker.
Previous turn PROGRESS(full/half lifetime endpoint arithmetic repairs); this turn
PROGRESS(packed native VSH generator, linked actor/HUD/particle bank, execution
checks). No app build/deployment/FTP or private asset/save/preference changes.
Old SD remainsc8acfac0d348; latest physical played6d78487cce32 at23.0011FPS.

New tools/particle_lifetime_shader.py generates static/dynamic packed-input VSH
from exact archived original inputs. Age/limit/mode/source-alpha use one extra
short4 attribute; initial alpha reuses colour.w. CPU simulation/physics/RNG not
changed. Native vertex/UV/RGB outputs preserved; dynamic corner pass-through
checked. Original GS source unchanged; link verifies relocated operations and
entry tables. VSH static63/dynamic66 instructions. Particle-only bank287;
actor+HUD+static+GS+dynamic common bank509/512,95 descriptors,3instruction spare.
This CAN avoid code uploads among those five entries if integrated; uncommon
hologram/stars/splash still require existing bank and cost must be measured.
Pinned Melee/linker shared DVLP rationale and citro3d C3D_BindProgram inspected.
No raw GPU cache/state lifetime changes; not a GPU execution/hardware claim.

Original C vs float24 standalone + linked static/dynamic entries:1478350 cases
per path PASS max1 alpha byte,1628 differences,0opaque/discard mismatches.
Includes1027lifetime lengths/5boundaries/5modes/6initial/7source values plus
400k signed/random cases. Earlier random-only gate missed148 half-life peak
255->254 errors: repaired exact half-life equality, preserved failed result
full-endpoint-only.json. Earlier full-endpoint reciprocal wrap failure also
retained. No arbitrary epsilon or broad clamping introduced. This is not
full GS near-fade/texture/blend/physical fidelity acceptance. Main game remains
unchanged; arithmetic prototype only, no FPS estimate or new full-frame timing.

Proposed strides36->44/60->68; existing capacities imply+30720linear bytes and
8additional fetched/published bytes per particle. NO live allocation yet. Both
branches need proper ABI/producer/lifetime/culling integration before app run.
Evidence particle-lifetime-shader-test/linked-result.json and
particle-alpha-candidate-test/result.json; generator/tests executable locally.
All owned sessions terminal20416(prior),39481,72972,5295. Initial inline file
creation failed Python parsing before writes; apply_patch version succeeded.
Next integrate bounded owner/producer paths with archived builder and evaluate
actual full-frame cost; reject if added uploads/traffic erase CPU savings.
Full rendering parity/performance/gameplay/save/endurance remain incomplete.



17 October PARTICLE LIFETIME INTEGRATED LOCAL; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(packed VSH bank); this turn PROGRESS(native producer/owner
integration, actual host checks, exact shared build and headless palace execution).
No FTP/hardware request/deployment/assets/save/preferences changes; Old SD remains
c8acfac0d348. Physical baseline remains6d78487cce32,23.0011FPS/43.4762ms palace.

New LOCAL shared normal a38edcd5bb02 in unified-particle-lifetime-v1-console-setup-builds.
Pure particle_lifetime_patch.py: seven archived sources, direct lifetime short4
packing into unpublished ring; removes CPU lifetime envelope/modulation only.
Original source update/physics/RNG unchanged. Static44/dynamic68B, lifetimes
at36; dynamic corners at44 map to v4..6, lifetime v7. SDK attribute permutation
reviewed. Existing publish/retire fences retained; no extra resource copy/thread.
New common actor/HUD/particle bank509instructions, oldbank retained byte-identical
for hologram/stars/drops/splash. Every parent embedded shader unchanged plusone
newbank. Old80/New124/setuprecipe unchanged. Root/gamearchive restored PASS.
Onlyexpgfx.o changed among794 ordered archive objects (duplicate basenames are
legal; initial dictionary audit rejected them, corrected ordered comparison).
Generated fullgame TU exactly parent with ONLY drawGlow lifetime_adapter applied.
Host30000actual producer comparisons ASanUBSanPASS: RNG/slots/drawmetadata/cleanup
exact; original-C decoding of newpacked parameters yields prior ordered values.
This is CPU producer proof; float24 error/gates remain from prior shader tests.

Exactcandidate Old palace107.44hostsec executionPASS/status1,106496auditeddraws,
5277geometrydraws,0layoutfault/attemptedmissingactor draws, saves/settings same,
GPUdrain/ownedexitPASS. Captured1600 inspected: Krystal/world/textures/purple
particle effects/HUD/bottomcontrols present. Not completevisualparity acceptance.
Rawrunnerstatusfailed-audio-core-placement preserved: Azahar SVC11 reports0;
requested/granted/selected1/30/1. This run has no blockprofile independent core
proof, don't markplacementverified. TimingEXCLUDED DSP-LLE+diagnostic capture.
Heap41365120/49283072,linearminfree3560704/12582912,initializedVRAM4723712,
stack72984/524288,process70651904/83886080. Exactly30720Bextra linear vsparent.
Dynamicparticle runtime/handoffs outsidepalace remain to cover. No FPSgainclaim.

Attempted paired cost runner rejected DSP-LLE attribution BEFORE emulatorlaunch;
DO NOTweaken that guard to manufacture timings. Need valid remaining full-frame
work/performance evidence. RUNTIME_REJECTED.json keeps candidate unpromoted.
Initial correctness harness rejected obsolete all_shaders_unchanged field before
boot; updated localrunner to all_parent_shaders_unchanged+named additivebank.
No redundant emulator restart; oneactualrun. Owned81490host,92603build,72791run
all TERMINAL. tools/run_particle_lifetime_cost.py remains guarded/unexecuted
pastargumentcheck; don't claim a pairedtiming result. No external blocker.
Next quantify full-frame work against currentcore1 baseline and validate actual
GPU lifetime/GS handoff/corner cases before promotion; continue fullgoal.


17 October MAIN-CORE MEMORY CALL TRACE; goal ACTIVE/INCOMPLETE, no blocker.
Previous goal turn NO PROGRESS(status answer only); this turn PROGRESS(exact
candidate bounded memory-call trace, inspected capture, source attribution).
Full objective read. No build/deployment/assets/save/preferences changes.
Old installed SD c8acfac0d348 unchanged; latest physical6d78487 remains23.0011FPS
/43.4762ms short palace sample, not sustained. No new FPS improvement claim.

Candidate a38edcd5bb02 still NOTPROMOTED. Existing eight-frame comparison now
reviewed: main18206685->18035926 encounters(-.93789%), different particle/audio/
lightning activity. Not cycles/retired-instructions or attributable FPS gain.
Its independent exact-ELF profile proves mix_once on core1; RUNTIME_REJECTED
now references that proof, preserving raw SVC11 telemetry failure and remaining
performance/dynamic-handoff/visual limitations.

run_particle_lifetime --memory-calls via profile wrapper captures memcpy/memcmp
LR+R2 on host only, frames1300..1307. Exact same candidate/ELF, no guest changes.
Run unified-particle-lifetime-v1-benchmark-old-dsp-core1-memory-profile-v1
executionPASS146.52hostsec,benchmark1,save/settings unchanged,drain/ownedexitPASS.
Palace1600 image inspected: Krystal, purple creature, world/textures/HUD retained.
Raw status still failed-audio-core-placement due SVC11 stub; independent
execution audit proves mix_once core1 only4308 encounters. No timing claim.
Analyzer now recognizes strict unified execution/save/render/drain guard and
checks exact ELF SHA; earlier legacy run schema retains original guard.
Zero dropped memory records. Main memcpy1381400B/8frames=172675B/frame.
Largest: programConfigure369216B/184calls; DSPaux280800B/135; packetleaf262656B/
144. Together66.07% copy bytes. Main memcpy1365615/18062261 encounters7.56%;
byte shares NOT per-caller time. Comparison requests are upper bounds, not
completed reads. LR may identify enclosing caller of tailcall; no cycle claim.

Read exact archived platform_program_plan.c and platform_pica_packet_owner.c:
program packets already precompiled, current cost copies immutable commands;
packet_leaf copies safe SDK segments excluding channel1 return. Read prior
PROGRAM_RESIDENCY/SHARED_LEAVES evidence: do NOT repeat existing rejected
shared-leaf experiment or restore command chaining casually. Pinned Melee
command_cache.c and uniform_upload.h publication/cache ownership consulted.
Next inspect actual shader-bank transitions and remaining render preparation;
copy volume alone doesn't justify skipping necessary GPU program uploads.
No removed simulation/effect; main performance and full goal still incomplete.
Owned31752run and88046audit TERMINAL, analysis terminal; no live owned process.
Evidence memory-call-analysis.json/audio-core-execution.json in exact run;
particle-lifetime-integration-test/arm-comparison.json. No hardware request.


## 18 October: reflected-particle material preparation

The reflected particle family remains unconnected. A new load-time compiler,
`fh_pica_glow_reflection_material_prepare`, prepares two immutable six-stage
Citro3D descriptions: RGB modulation and the source override's saturated addition.
Both multiply vertex alpha by the independently supplied normalized bump mask.
All unused stages are initialized to pass through; a future owner must publish
all stages and restore native distance fog after publication. This follows the
pinned Melee raster invalidation discipline without restoring GX interpretation.

`tools/test_glow_reflection_material.py` executes the original C
`setupReflectionIndirectTev` against recorded TEV state, then compares its
post-sample equations with the actual new compiler and pinned SDK helpers.
262,144 continuous normalized-input comparisons pass with zero float error,
including colour saturation, alpha independence and both modes. Sanitizers pass;
three mutations fail (lost additive override, wrong opacity sampler, stale final
stage). Strict SDK ARM11 compilation passes with no external calls. The first
standalone compile exposed a missing 3ds.h prerequisite; the header now includes it.

Storage would be 240 CPU bytes per pair; no game allocation or frame work has
been introduced because this header is not connected to a production owner yet.
This proves neither PICA rounding nor source mask production, distortion, capture
orientation/lifetime, image parity or performance. The test records which original
setup calls are excluded rather than treating their no-op stubs as validation.
The next integration needs a source-compatible projected reflection and filtered
normalized mask. A plain particle-alpha sample is not equivalent. The existing
splash material also has different colour equations and cannot be reused as-is.
Evidence: `artifacts/research/pica-scene-rewrite/glow-reflection-material-test/`.

## 18 October: reflected-particle projective shader

`pica_glow_reflection_shader.py` derives a reflected variant from the exact
verified particle source in the 842f archive. This lives in a separate three-entry
program (static VSH, shared GS, dynamic VSH), not in the nearly-full actor/HUD
bundle. It uses the existing eye-space position, applies the source camera-light
flip-Y rows, and exports homogeneous reflection coordinates for projected sampler
0. Ordinary particle UVs move to sampler 1; fog retains coordinate 2. The source
`loadReflectionTexMtxs` composes flip-Y * current view, so using the already
transformed position avoids another CPU or GPU view transform.

The assembled linked program stores 179 of 512 instructions and fully writes all
five outputs before every emit. Four additional instructions execute per corner.
`test_glow_reflection_shader.py` checks 27,648 static and 27,648 dynamic emitted
vertices through actual packed attributes and Azahar's float24 VSH/GS interpreter.
All prior position/colour/UV/fog lanes remain bit-identical. This includes the
near-camera alpha cases excluded by the older float-C oracle's loose comparison.
The new coordinates match source matrix composition with maximum relative error
3.8147e-5 (static) and 3.43323e-5 (dynamic). Wrong-axis mutants fail both fixtures.
All scratch registers are poisoned before execution; source order and winding
checks remain active. An initial fixture insertion failed on whitespace before
execution; corrected before obtaining these results.

This is not connected to the game yet. It adds no current runtime allocations or
frame work. Capture lifetime, filtered normalized mask, indirect distortion,
texture sampling, physical GPU behaviour and whole-frame cost remain unvalidated.
Source and test artifacts are under `glow-reflection-shader-test/`. Do not infer
an FPS gain or visual completion from this instruction budget or numeric proof.

## 18 October: optional prefiltered particle mask

`pica_glow_mask.c` is a load-time compiler for an explicitly approximate A8 mask.
It reads decoded RGBA8 alpha, applies source 5-bit mask normalization, and emits
PICA Morton tiles with the native row orientation. Dimensions and buffer extents,
stride arithmetic and overlap are checked before any writes. It allocates and
publishes nothing; callers must own, budget and flush storage before GPU use.

The original Aurora generator samples `.abg`, rounds the filtered indirect
sample, masks alpha with 0xF8, then normalizes by 248. Doing the quantization
before filtering changes the result. `test_glow_mask.py` binds its equation to
those source-generator expressions and compares 4,456,448 filtered endpoint pairs
and 4,161,600 tiled texels. Maximum normalized opacity error is 0.031697301 for
these synthetic filtered cases, below the conservative bound 8/248 + 0.5/255.
Exact texels differ by at most 0.00189753321. Odd weight denominators avoid source
round-half ambiguity. ASan/UBSan and strict ARM11 compilation pass; wrong-channel,
row-orientation and missing-normalization mutations fail.

This is not a claim of exact filtering, visual acceptance or physical GPU
validation. It is not connected to the game. Future residency costs one byte per
texel; a maximum-size mask is 1 MiB, which must not be blindly allocated on Old
3DS. Actual affected texture quality and bounded residency remain acceptance
requirements, along with capture ownership and indirect distortion. Ordinary
particle alpha is also an approximation, not an exact normalized mask substitute.
Evidence: `artifacts/research/pica-scene-rewrite/glow-mask-test/result.json`.

## 18 October: actual reflected-particle mask inventory

The eight literal reflection cases in Effect20 use five texture IDs. Across 53
bank directories, the source resolver identifies five unique eligible images
(16x32 or 32x32), totaling 4,608 bytes of A8 masks. There are 154 resolved bindings;
111 candidate bindings need the runtime paired-bank selection and remain
unresolved. This is not evidence of 111 missing textures. The initial audit
incorrectly treated the table high bit as validity; inspection of `tex0GetFrame`
shows bank selection. The corrected report preserves the rejected assumption
separately. Dynamic flag changes, aliases and full runtime residency are not
proved by this literal-case inventory.

The actual native compiler was executed on all five decoded images. Across
163,840 filtered samples, preconverted-mask RMS opacity error ranges from
0.003819 to 0.008103; maximum error is 0.029806. Raw alpha has higher RMS error
on every image, although preconversion does not always improve worst-case error.
The private five-row source/mask/raw-alpha/error contact sheet was inspected:
mask shapes remain intact, with differences along filtered boundaries. This is
mask-level acceptance for further integration, not final reflected-image or
physical GPU acceptance. No source pixels are shipped or modified.

Evidence: `glow-mask-asset-audit/result.json`. Proceed with bounded residency
based on this small inventory, while preserving actual bank ownership and
retirement; do not use the compiler's maximum extent as a blanket allocation.
Capture, distortion and gameplay integration still remain incomplete.

## 18 October: mask orientation correction before integration

The earlier mask compiler's extra Y flip was wrong for this consumer. Ordinary
and reflected particles share the same UVs, and the existing native texture
converter stores rows directly. An added regression executing actual
`fh_texture_convert` failed against the initial mask compiler. Removing the flip
makes all mask bytes agree with normalized alpha in the native texture storage.
The old untile check shared the compiler's mistaken flip assumption; it proved
internal consistency rather than compatibility with the actual consumer.

The expanded synthetic tests pass with ASan/UBSan and ARM11 compilation. A new
actual-asset comparison also executes the existing converter on all five source
RGB5A3 images and compares every corresponding mask byte. Converter and pinned
Melee decoder/repacker/storage headers match the ready 842f archive. A wrong-row
mutation is rejected. The logical masks, filtering error and 4.5 KiB inventory
are unchanged. Evidence: `glow-mask-test/orientation-repair.json` and updated
`glow-mask-asset-audit/result.json`.

No deployed or gameplay code used this experimental compiler yet. This is not
a repair of existing console visuals. Resident lifetime, capture, distortion,
physical filtering and complete effect integration remain unfinished.

## 18 October: bounded image-pool mask owner

`platform_pica_glow_mask_images.h` adds load-time mask acquisition to the existing
native image pool. `glow_mask_residency_patch.py` applies the helper and final
release accounting to the checked 842f source snapshot without importing root
renderer work in progress. The game consumer is not connected or built yet.

Masks deduplicate by dimensions and exact converted bytes, with no retained
source pointer. Sixteen mask handles and a 16 KiB mask ceiling sit within the
existing 4 MiB image budget, preserving 15% of the actual linear grant. Temporary
normal-RAM staging is bounded by the same 16 KiB ceiling. A direct RGB5A3-to-A8
compiler avoids an intermediate RGBA image; all 65,536 RGB5A3 words match the
actual native texture converter's normalized alpha. The existing image reference
count and final-release path remain authoritative. As with existing HUD images,
the caller must flush pending batches and drain GPU readers before final release.

Actual helper and patched release tests cover 128 shared-reference lifecycle
cycles, moved-source deduplication, changed bytes at a reused address, allocation
failure, linear reserve, mask and global budgets. ASan/UBSan and strict SDK ARM11
compilation pass. Mutations omitting retirement accounting, reserve enforcement
or cache publication fail. These use mocked GPU readers and SDK allocation;
they do not establish physical cache coherence or scheduling behaviour.

Evidence: `glow-mask-residency-test/result.json` and `glow-mask-test/result.json`.
No current gameplay allocations or rendering costs have changed. Consumer load/
retire hooks, capture and indirect distortion still require integration.

## 18 October: proposed spawn/page consumer integration

`glow_mask_consumer_patch.py` composes the image-owner patch with source spawn
hooks, a sparse page owner and a new effect-word bit (`PART REFL`, off in all
unchanged presets). Masks are requested at reflected-particle spawn boundaries,
never from draw callbacks. The game helper and native owner both skip preparation
when the option, particles, rendering or particle textures are disabled. Resource
failure marks the optional effect unavailable for the session.

`platform_pica_glow_mask_pages.h` stores at most sixteen immutable samplers and
a 256-byte page-index map. Source relocation preserves the existing page index.
The three existing retirement paths release masks under their established GPU
reader-drain policy. Borrowing a sampler uses the page index directly and does
no conversion, allocation or source validation.

The actual patched release/forget/relocation functions and shutdown loop pass
sanitizer tests, including 10,000 allocation-free borrows, cached acquisition,
OFF gates and failed preparation. A missing release mutation is rejected. The
HUD generator and added glow source hook are checked separately: the complete
glow generator requires upstream pool rewrites, so a failed standalone harness
attempt was corrected to test only the added hook. This is not full generated
ARM game or runtime validation. Evidence: `glow-mask-consumer-test/result.json`.

This remains a proposed archive patch, not a built gameplay update. Reflected
draw and capture integration, live-toggle admission and indirect distortion are
still pending. Builders must import the explicit new helper/compiler files into
the isolated snapshot; do not build unrelated root renderer work in progress.

## 18 October: complete mask owner integration check

`tools/check_glow_mask_integration.py` compiles the complete patched archived HUD
and material owners, plus the mask compiler, with the ARM11 SDK. It also executes
the archived `arm_overrides.prepare` pipeline for texture.c and expgfx.c, including
the required upstream pool rewrites. All pass. The earlier hook-only source test
is no longer the only integration evidence. Inputs are isolated from root WIP.

This is object compilation/source generation, not an app link or GPU execution.
The reflected draw/capture/indirect consumer remains missing. No deployment or
performance claim. Evidence: `glow-mask-integration-test/result.json`.

## 18 October: source-boundary reflection snapshot owner

Original particles sample gNewShadowReflectionTexture. Water's on-demand capture
can occur at a different scene point and must not alias this image. The proposed
archive patch adds a separate 128x128 RGB565 GPU target (32 KiB VRAM), sharing the
existing immutable 240-byte quad and ordered split/draw mechanism. It captures at
the two lightmap reflection-update boundaries. OFF/disabled particles, rendering,
or textures perform no capture/allocation. Other screen-effect update boundaries
remain to be integrated; this is not full capture fidelity yet.

Actual helper/surface ARM SDK compilation and host lifecycle/gate tests pass.
The full archived game source pipeline retains both scene capture hooks. No game
binary is linked or deployed, and the reflected draw/indirect consumer is still
missing. Hardware cost and image fidelity remain unmeasured. Evidence lives in
`glow-reflection-capture-test` and `glow-mask-integration-test`.

## 18 October: reflected stream consumer connected in candidate source

The native particle owner now selects a separate static/dynamic VSH+GS program
for prepared reflection descriptors. Packed instances and original simulation
remain unchanged. The material uses projected snapshot RGB and prepared A8 mask;
indirect distortion remains absent, so this is an opt-in approximation.

Program transitions dirty all 21 ordinary or 24 reflected uniform rows. Material
publication precedes fog restoration; fog scale is read after restoration because
that owner may rebuild its normalization. Missing masks/snapshots are counted and
omitted, never routed through the legacy renderer. Enabling after particle spawn
still needs a preparation/admission solution.

The complete native owner compiles for ARM11 without warnings. The full archived
game generator emits the new descriptor and scene capture hooks. Build scripts
now generate the separate shader and track its generator identity. No complete
app is linked or run yet, and GPU handoff/image correctness is unverified. A new
game archive must be built: the parent library does not contain these hooks.

## 18 October: enable preparation and full linked candidate

The update-boundary helper prepares existing live reflected particles on an
effective OFF-to-ON transition. It does not rescan on steady enabled updates.
Source simulation memory is unchanged; cached masks bypass source texture-view
validation. Focused tests cover 10,000 steady updates and failure/disable cases.

`glow-reflection-v1-console-setup-builds` contains the linked shared normal CIA
and 3dsx, with rebuilt texture/lightmap/expgfx game objects. Existing shaders
remain byte-identical; the new shader occupies 179/512 instruction slots. CIA
Old80/New124 and restoration checks pass. No runtime image, GPU handoff, memory
measurement or performance acceptance yet; not deployed. Indirect distortion
and additional screen-effect capture boundaries remain incomplete.

## 18 October: exact normal binary exercised

`glow-reflection-v1-save-new` runs source 7b669f530721 with PART REFL enabled,
real DSP LLE, the copied New save, and ordinary scripted controls. It completed
634 reflected draws with zero missing masks/snapshots, then drained and exited.
The periodic layout audit reported 131072 draws and no fault; it remained at
`checking`, so this is not a final exhaustive audit. Two cave captures were
inspected; Fox/Tricky/world/torch/HUD appear coherent. No matched source image
comparison or physical-console acceptance is claimed.

Normal heap peak 54174792/71303168 B; linear minimum free 17141632/25165824 B;
initialized VRAM free 4690944 B; main stack 72972/524288 B. DSP LLE and auditing
exclude performance comparisons. The rescue progression was not exercised.
Saves, original disc and prepared assets remained unchanged. Nothing deployed.

## 18 October: capture cost attribution (unbuilt)

The capture owner now counts requests, disabled requests, completions and failures,
and separates preparation/retirement wall ticks from draw-submission wall ticks.
Neither is GPU execution time; these intervals may overlap other frame buckets.
The disabled route performs no timer reads or GPU work. Actual helper tests and
SDK ARM compilation pass. No new application build or timing measurement yet.

A proposed no-resident-mask gate was not applied. Current residency does not prove
that a particle spawned later in the frame will not need the earlier snapshot.
Preserve the original capture timing until that temporal condition is established.
