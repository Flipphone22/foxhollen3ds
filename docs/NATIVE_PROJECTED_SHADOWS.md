# Native projected shadows — integrated candidate

The New 3DS normal app now contains candidate `1a1a16f3e6cd`, CIA `5456de168563`.
It was staged, byte-readback verified and installed without changing saves or
preferences. Matching replay `43b0d378396f` / CIA `eb61e6c44e0f` is also installed
on New 3DS; Old installation is unchanged. Both variants use identical checked
renderer sources, game archive and linked shadow/scene programs. Hardware acceptance FAILED: the 11 October run with SHADOW SHAPE ON has missing cave shadows. See the depth-state candidate below; prior Azahar images did not establish hardware correctness.

The native path projects a GPU-produced silhouette onto the actual visible
world geometry rather than flattening it onto one plane. The first candidate used world-depth equality; the hardware report now requires revisiting that choice. Original BIG_BOX shadows instead consume
the original authored mask, with the source XZ projection and opacity; generating
a mesh silhouette for that family was unnecessary work and a fidelity departure.
Model shapes retain GPU skinning/morph positions. Source finite-volume policies,
receiver flags/backfaces, position overrides, self-shadowing and filtering are
not all reproduced yet.

Load-prepared position-only receiver batches preserve every ordered source
triangle. They refresh at world-position mutation, and retire with their scene.
The cave uses 196,560 additional linear bytes and 3,180 CPU bytes for batches,
plus 98,304 VRAM bytes for images, 2,048 linear bytes for the ramp and 18,432 BSS
bytes for frame references. Global batch caps are 512 KiB linear/64 KiB CPU with
headroom guards; a declined batch uses the existing resident native world mesh.

Short serial Azahar cave checks: first integration 51.88 FPS, batching 54.57,
source-mask correction 59.83 on emulated New; deployed baseline also 59.83. The
latest path still adds about 1.02 ms of auxiliary submission and 0.17 ms of GPU
telemetry in these windows. Those are neither whole-frame CPU costs nor console
predictions. Old candidate measured 54.11 FPS without a matched Old baseline.
All exited normally and drained logs. A separate movement check crosses the
cave ledge and shows the shadow following the lower surface; it has performance
dips and is not a claim of sustained physical 60 FPS or complete edge acceptance.

Reproducible helpers and measurements are in
`artifacts/research/pica-scene-rewrite/{build-shadow-project-v1.py,`
`check-shadow-project-v1.py,check-shadow-project-walk-v1.py,`
`shadow-project-integration-result.json}`. Exact source/archive snapshot:
`shadow-project-v5-builds`. Original asset audit checks 2,932,998 indices across
1,259 packets; an ASan/UBSan test checks 1,000 batch allocation/mutation cycles.
No assets were decimated or edited for this repair. The earlier prototype
records below remain historical evidence, not the current integration status.

## Component-stage history

# Native projected shadows — integration in progress

This replaces the faulty single-plane receiver band isolated in
[the cave regression](SHADOW_REGRESSION_20261011.md). The components below are
implemented and locally checked but **not connected to gameplay, linked into
a replacement application, deployed or physically validated**. New normal
`86b336c4bfa5` remains installed with the reported regression.

## Implemented components

- `platform_pica_shadow_image.c`: three persistent128×128 RGB565 images,
 98,304bytes total VRAM and no depth attachments, CPU image buffers, texture
 copies or per-capture allocation. Requires at least1MiB VRAM headroom beyond
 these images. SDK framebuffer descriptors share the main auxiliary queue owner.
 Ordered command-list splits precede reuse and publication; the sampler is
 unavailable during capture. Explicit rebind restores the attachment and viewport
 after a command-capacity join. The parent must retire all readers before shutdown.
- `pica_shadow_receivers.c`: load-time bounds over the actual resident world
 triangles, borrowing existing vertex/index storage. No extra GPU geometry bytes.
 Bounds use decoded local coordinates; GPU input scaling is a separate uniform
 operation. Four conservative footprint planes reject unrelated draw groups.
 Validation precedes publication, including index/range/finite-position checks.
 World-position mutation must rebuild these bounds before reuse; that owner hook
 is still to be integrated.
- `fh_pica_shadow_projection.h`: bounded orthographic capture and receiver UV/depth
 matrices. Retains the original400-unit light-eye offset,1..1025 capture depth
 range and `(alpha>>1)+(alpha>>2)` intensity. Native capture and lookup share one
 orthonormal basis. This preserves their projection relationship without changing
 the game's camera. The basis is not an assertion of bit-identical original
 quantized camera angles. Affine inverse/composition runs per caster/receiver,
 never per vertex. Source fade/ramp sampling, receiver admission and blending still
 need the live material implementation.
- `pica_shadow_project_pair_shader.py`: combines the established caster/quad
 program and finite-receiver projection in one65-instruction program. Existing
 ordinary scene programs are unchanged. The shared ABI uses96float registers and
16booleans; bool14 selects receiver projection and bool15 selects the quad.
- `pica_shadow_project_owner_patch.py`: pure, **unapplied** integration patch for
 the shared program, explicit mode selection and GPU quad clear. Clear rebinds
 the active image after reserving command space. It uses SDK state, no raw
 register writes. Both selectors explicitly reset the other mode, so a previous
 ordinary material's boolean state cannot silently choose the wrong shader path.

Pinned Melee renderer/texture-VRAM ownership and citro3d buffer/framebuffer/queue
implementations were consulted. A ring is not treated as proof of completion;
all images remain resident and CPU ownership is returned only by the main fence.
There is no new worker or assumption that New3DS audio's core is idle.

## Completed checks

- Actual image-owner C under ASan/UBSan:100lifecycles,100,000captures, zero
 per-capture allocations. Missing publication and premature retirement mutants
 are rejected. This host queue model does not establish actual GPU cache behavior.
- Current Original geometry source hash`c5ab2aaa1cfd`:1,259world packets,
10,912draw groups and2,932,998indices preserved;3,777negative cases;
43,648footprint cases with no observed false rejection. Current Performance
 source`09eb9bf70d26` preserves1,775,196indices with the same group/packet counts.
 Maximum metadata for one packet on ARM is1,764bytes. Whole live-scene residency
 and cost are not yet measured. Earlier hallway-pack evidence is kept separately.
- Projector math under ASan/UBSan:32,768capture/receiver/affine-camera cases,
256alpha values; maximum observed coordinate error0.000044783units in the tested
 output coordinates. Invalid inputs are rejected. This is not image acceptance.
- Actual assembled PICA program in pinned Azahar's interpreter:32,768old caster/
 quad interleavings retain exact position/view/quaternion/fog outputs; another
32,768nonplanar receiver vertices match917,504output lanes with zero observed
 error. Flattened-height mutant fails. Azahar arithmetic is not physical PICA
 precision, and stored instruction count is not a throughput measurement.
- Actual proposed mode selectors:65,536inherited flag states/196,608handoffs;
 missing mode-reset mutant fails. The GPU clear's full raster behavior still
 requires the integrated run.
- Four strict ARM objects compile: image owner, receiver compiler, proposed shared
 program owner and projector math. The proposed build hook parses. No full
 application link or runtime claim follows from this check.

Evidence directories under`artifacts/research/pica-scene-rewrite`:
`shadow-image-owner-test`, `shadow-receivers-{original,performance}-current-test`,
`shadow-projection-test`, `shadow-project-pair-{skin,receiver}-test`,
`shadow-project-mode-test`, `shadow-project-arm-check`.

The first ARM compile caught a nonexistent SDK border-colour setter; it now sets
the SDK texture descriptor's`border`field for normal bind publication. The first
combined shader assembly caught an output-register naming mismatch; it was fixed.
A test-helper output-path mistake was also corrected. These were local failures,
not console tests or deployed changes.

## Next integration

1. Attach bounded receiver metadata at scene load and refresh it only on the
 existing world-position mutation path. Account allocations and remove borrowed
 frame references before scene retirement/free. Record live world transforms
 with a fixed frame-list limit and no per-frame geometry construction.
2. Replace the SHADOW SHAPE single-plane branch with native capture of the current
 resident caster/pose into the bounded image. Use explicit shader modes and
 rebind the image after any command-capacity join. Remove the narrow plane test
 from this path; preserve the separate optional contact footprint.
3. Project the image onto admitted finite receiver geometry, with original ramp,
 intensity and multiplicative blending semantics. Preserve depth, fog, texture
 routing and subsequent material state. Explicitly account remaining source
 receiver/admission differences; do not silently claim them complete.
4. Build from`lightning-v1-builds/source-snapshot`and its archive, preserving the
 unrelated VRAM WIP. Test the exact cave scene, captures and workload/cost before
 deployment. Verify ledges and uneven floors as well as restored mushroom/Fox
 shadows. Optional effects still need performance gates and physical validation.

Do not deploy the earlier band-disabled diagnostic as this repair. No new
hardware replay is requested until there is an integrated candidate worth testing.


## 11 October hardware rejection and depth-state candidate

The read-only collection `artifacts/console-visual/20261011-140404-223` matches
the new projected-shadow path and a normal Select exit at retrace 3805. Its
last LCD image shows Fox standing in the lower cave with no clear player
shadow. The final settings mask 133890049 enables ground/shape shadows.
Counters show 1409 generated silhouettes, 3497 authored masks and 46301
receiver draws: submitted work did not establish visible output.
The last 320 rendered frames average 20.545 FPS. GPU service wall telemetry
averages 35.151 ms and auxiliary submission 12.366 ms; these overlap and are
not an additive frame breakdown or pure GPU utilization.

Local `shadow-depth-v1-builds` changes receiver depth state from EQUAL to
reverse-Z GEQUAL with a +2^-22 depth-map offset (approximately four D24 units).
This keeps actual receiver geometry and foreground occlusion, writes color
only, and adds no shader instructions, geometry passes or allocations. More
accepted fragments can still cost GPU time. Exact equality rejecting the
replayed surface is a hypothesis, not a proven hardware diagnosis. The offset
is a native adaptation, not a claim of identical original shadow rasterization.

`test_shadow_project_depth.py` runs 4,934,475 bounded roundoff/occlusion cases
under ASan/UBSan and rejects the former equality rule. It does not simulate
PICA raster precision. All original world vertices were also checked to have
W=1, ruling out the receiver shader's explicit W=1 as a data mismatch.
`verify-shadow-depth-v1.py` verifies only two source identities changed, identical
game archive and actual linked shaders (65 shadow / 511 scene instructions),
and shared Old80/New124 CIA profiles. Normal/replay have 430 matching sources.
Runtime/deployment outcomes are recorded separately in
`shadow-depth-hardware-result.json` and `shadow-depth-validation.json`.

Pinned Melee raster-state verification and depth comparison mapping, its VRAM
resource-lifetime warnings, and pinned citro3d framebuffer/effect/publication
code were consulted. Existing queue ownership, cache publication and retirement
are unchanged. No raw depth readback or private GPU queue was introduced.

A separate load-time exact-overlap audit found 15,853 repeated oriented shadow
receiver triangles among 977,666 (5,287 across material groups). No mesh changes
or deduplication have been applied; this is separate from the missing-shadow
failure. Finite-volume/receiver policies, softness and full parity remain open.

Normal `4ff085bfe304` / CIA `a34c23158ede` and replay `b9c3cfa271d3` / CIA `de7476ffba35` deployed to New223, staged byte-readback verified with saves/preferences retained. Old installation unchanged. Main/replay .3dsx ready; CIA requires FBI installation. Headless New cave walk passed with images1440/1760/1840 inspected: 58.665 versus58.800 Azahar FPS, memory peaks identical. Hardware acceptance FAILED again in collection20261011-142505-223: the user still sees no shadow. Deployment proof: `console-settings-update/20261011-141803-223/manifest.json`.

## 11 October repeated hardware failure and isolation probe

The depth candidate also fails on New 3DS. Collection
`console-visual/20261011-142505-223` ends normally at retrace2461, confirms the
installed candidate and enabled shadow settings, and captures missing cave
shadows despite1161 images/4533 authored masks/53925 receiver draws.
The automatic [shadow probe](SHADOW_GPU_PROBE.md) is deployed and its hardware
results are pending. It separates geometry, known textures, generated images,
depth tests and texture-coordinate Q without another game replay.

Separately, the load compiler now removes exact oriented duplicates locally.
All1259 packets retain every unique triangle;15853 duplicates and8 empty
batches are removed/skipped. CPU and linear allocation capacities are unchanged.
Mutation rebuilds restore triangles that cease to overlap. ASan/UBSan checks
pass, but this change has not been ARM-built, timed or deployed. It does not
explain the missing hardware shadows. Earlier no-dedup statements above describe
the installed build. Evidence: `shadow-receiver-unique-assets-test/result.json`.

## Stable receiver order (local candidate)

Sorting actual triangle draws was rejected after cache-locality modelling found
regressions. The compiler now sorts a bounded ordinal scratch table, marks only
exact duplicates, and compacts the original draw order. The64KiB scratch cap
applies at load and actual source-position refresh; normal frames do not allocate
it. Allocation failure retains the original geometry. The full asset oracle
checks original order as well as exact unique coverage. Generic8/16/32/64-entry
LRU models improve modestly; these are not PICA cache sizes or timing estimates.

Local normal5d5373d101c7 passes the same New cave route and inspected captures.
58.614 vs58.665 emulated FPS is unchanged within run variation. Measured normal,
linear and VRAM peaks are unchanged; process commitment adds one4KiB code page.
Temporary scratch is bounded explicitly because sampled memory logs need not
capture its peak. Actual shaders and game archive are unchanged. No deployment
or Old runtime is claimed. Hardware missing shadows remains a separate failure.
See `shadow-unique-v2-validation.json` and `shadow-unique-v2-build-verification.json`.

## V2 hardware controls pass; actual fog probe pending

All nine V2 controls pass on New3DS, including generated images and both depth
comparisons. Capture validity is established with the solid control. This does
not reproduce or fix missing cave shadows. V3 links the deployed procedural fog
owner and checks near/half/far transmission for both image sources. All15 cases
pass Azahar with inspected images; V3 is installed at the existing probe path.
Hardware results are pending. Main/replay installations remain unchanged.
See [probe evidence](SHADOW_GPU_PROBE.md).

The source receiver-policy inventory also identifies a remaining architectural
gap: original shadows select marked collision triangles and light-facing faces,
whereas the native implementation currently projects onto visible-world meshes.
Static counts are in shadow-track-inventory/result.json. They are not a measured
speedup or evidence that collision geometry can replace visible geometry without
checking transforms, raster depth, dynamic receivers and source semantics.
