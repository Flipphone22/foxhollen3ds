11 October: the receiver/pair path is deployed in New normal86b336c4bfa5 and has a confirmed cave regression. See [current investigation](SHADOW_REGRESSION_20261011.md); historical not-deployed statements below are superseded. No repair deployed yet.

# Native shadow receiver checks

Latest local candidate: **fae427b9ed3d**, not deployed. Receiver classification
and silhouette skinning now share one 43-instruction PICA program. Switching
their boolean mode preserves the code allocation; ordinary actors, UI and fur
keep their previous programs. SDK C3D_BindProgram recognizes the same program
and DVLP, avoiding the previous full scene-code replacement for the silhouette.

New and Old isolated cave runs passed. Geometry counts match the deployed quad.
New-profile cumulative actor submission fell 2,169.880 to 2,105.404 ms across
the whole run; this is not a per-frame cost or a physical speedup. Both profiles'
captures retain world, Fox, silhouettes, HUD and mist. Hardware edge acceptance,
physical cost, softness and multiple receivers remain open. Defaults unchanged.

Build: build-shadow-pair-v1.py. Evidence: shadow-pair-v1-builds/verification.json,
shadow-pair-shader-test and shadow-pair-owner-test. The working shadow owners
were migrated with exact reversal/reapplication checks; unrelated VRAM/weather
work still requires isolated builds. The prior quad remains installed on New223.

## Deployed receiver quad and earlier experiments

# Native shadow receiver checks

Latest candidate: **97a57fc89e33**, persistent receiver quad. This replaces the
earlier duplicate-mesh experiment documented below. Deployment status is in
AGENTS.md; this document does not itself prove transfer or hardware acceptance.

One resident 28-byte quad and a nine-instruction program classify visible depth
against the caster's receiver plane. The CPU supplies an affine depth equation,
not transformed vertices. Normalizing clip depth and undoing it through SDK
DepthMap preserves coverage at near/far crossings. The same stencil predicate
then admits the ordinarily skinned shadow once. No new target or texture.

The cave run reduces receiver geometry from 6,036,354 to 20,622 indices, and
receiver draws from 4,419 to 3,437. Fullscreen coverage and program switching cost
more CPU: cumulative actor submission 2,169.880 versus 2,014.123 ms for the
duplicate-mesh candidate over the whole run. This is not a per-frame figure or
a speedup claim. The full shadow budget remains open. Physical performance,
depth precision and edge clipping need acceptance; optional shadows stay opt-in.

Ray/plane math, actual owner/state/lifetime tests and exact shader interpreter
checks passed. Cave captures retain world, actors, shadows, HUD and moving mist.
Azahar's f24 class actually performs float32 arithmetic. Interpreter success
therefore does not prove hardware precision. An additional depth-map mantissa
truncation estimate is recorded separately, not called a hardware oracle.

Evidence: receiver-plane-test, receiver-owner-test, receiver-shader-test,
receiver-quad-v2-builds/verification.json and artifacts/native/receiver-quad-v2-new.
Build with build-receiver-quad-v1.py; its v2 snapshot includes deployed fur groom
and C-menu fixes, excludes unrelated weather/VRAM work, and preserves game code.
Single-plane depth proximity still cannot wrap shadows onto lower floors or
multiple receivers. Soft filtering and self-shadowing remain incomplete.

## Earlier duplicate-mesh experiment

# Bounded receiver depth for native silhouettes

11 October local candidate, **not deployed or physically accepted**. The user
confirmed the deployed C-stick UI repair and reports floating shadows at edges
with SHADOW SHAPE enabled. Fur appearance is separately rejected by the user;
neither defect is closed by successful startup or shader arithmetic tests.

The original `shadow_dolphin.c` collects and culls finite track triangles before
drawing projected shadows. The native approximation instead skins its resident
caster onto a single infinite collision plane. A front-facing depth test admits
that projection even when the ground beyond a ledge is much farther away.

The candidate uses the existing D24S8 target and two stencil bits. For each
resident shadow range, a colour/depth-write-disabled pass tests a receiver plane
0.25 world units farther from the camera. It sets bit 1 when scene depth lies in
front of that plane and clears bit 1 on depth failure. The colour pass uses its
existing near-side projection and requires bit 1 set and shadow-union bit 0
clear. A passing colour fragment increments the stencil with write mask 1,
preserving the receiver and other bits. Overlapping triangles darken once.
Strict LESS in the receiver pass rejects cleared far depth, including clamping
at zero. Every colour draw has an identical-coverage prepass; no old receiver
bit can escape into another range. Ordinary exit disables stencil as before.

Both passes use the **same shader, clip coordinates, live bone uniforms,
attributes, resident vertices and indices**. Only SDK depth-map/raster state and
the draw command change. Moving a perspective plane along its normal scales
all eye-ray intersections equally; the corresponding affine window-depth
transformation avoids a fixed depth epsilon growing into huge gaps far away.
CPU work is three cross products, the plane distance and two coefficients per
caster. Unsupported/degenerate camera planes are explicitly counted and omitted;
the tested run has zero such cases. There is no CPU mesh clipping, extra target,
readback, allocation, worker, shader instruction or source simulation change.

This adapts pinned Melee `renderer.c::draw_with_alpha`'s masked stencil predicate
and write-disabled preliminary passes. Its immutable source/GPU lifetime rules
and the pinned citro3d `effect.c` SDK publication are retained. Melee's code does
not validate this new depth-band algorithm on hardware.

This remains a **hard single-plane approximation**. It clips against visible
depth near that plane; it does not wrap onto lower floors, walls, multiple or
moving collision receivers, filter softness, or reconstruct self-shadowing.
Visual-vs-collision mesh offsets exceeding the band can reject legitimate
shadows. Floating-point/depth quantization, grazing views and physical stencil
behavior need dedicated acceptance. Circular contact shadows are unchanged.

## Evidence and costs

- Actual helper: 79,477 random perspective/tilted-plane comparisons with an
  independently moved-plane oracle, receiver and lower-floor exclusions.
- Actual SDK submission function: 256,256 stale-bit, foreground, ground,
  overlapping-shadow and far-depth cases under ASan/UBSan. A mutant retaining
  stale receiver bits fails. Existing 96 fog/material/HUD handoff checks pass.
- Shared normal candidate `aadd9ce1924f`, CIA `709dd11fcfff`, retains the exact
  deployed game archive and all four native shader binaries, including the
  physically confirmed 43-instruction UI specialization. Two native owner
  sources and one new math header change. CIA remains Old80/New124 MiB.
- Serial New-profile normal checks, same save/assets/settings/script, both
  complete Select exit, drained logs, unchanged isolated saves and owned exit.
  Before/after captures1500 inspected: world, Fox, HUD and ground silhouette
  remain. The cave comparison is not every ledge or the user's exact location.

Whole-run caster counts match: 3,437 objects, 4,419 colour draws, 6,036,354
colour indices. Candidate adds 4,419 receiver draws with identical geometry.
Whole-run actor submission grows 1,992.295 to 2,014.123 ms, +21.828 ms over the
complete run; world submission changes 1,204.364 to 1,204.095 ms. These totals
include different scenes during startup and are not per-gameplay-frame values.

In the matched final300-frame window total draws rise326.83 to335.63/frame;
indices107,362.68 to119,698.52/frame. The small normal-app queue counters remain
submit0.165873/0.165660ms and GPU0.677130/0.677507ms. **Those counters do not
measure total renderer CPU time or physical GPU cost.** Reported interval stays
16.713ms, but wall run time grows77.625 to84.717seconds. The added GPU geometry
is a real cost, not excused by paced emulator FPS. No performance acceptance or
physical FPS improvement is claimed; the previous overall shadow budget is
still open and defaults stay opt-in.

Final RAM observations identical before/after: normal53,887,088/71,303,168 B,
linear free18,656,128/25,165,824 B, VRAM free4,822,016 B, process102,694,912/
130,023,424 B, main stack73,008/524,288 B. Sixteen static bytes of counters and
coefficients fit existing alignment. Old physical cost remains unmeasured.

Reproduce with `tools/test_shadow_receiver_depth.py`,
`tools/test_planar_handoff.py`, and the build/check/verify-shadow-receiver-v1
helpers in `artifacts/research/pica-scene-rewrite`. Evidence is
`shadow-receiver-depth-test/result.json`, `shadow-receiver-v1-builds/verification.json`
and `artifacts/native/shadow-receiver-v1-{before,after}-new`.
The builder isolates the exact deployed `cmenu-ui-v3-builds` snapshot and archive,
then restores WIP in `finally`. `pica_shadow_receiver_patch.py` is already applied
to current WIP; do not apply twice. No remote files or saves were changed.
