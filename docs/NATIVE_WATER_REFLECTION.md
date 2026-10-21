# Native water reflection replacement

13 October — **v6 prepares the three splash materials at asset loading**.
Untextured, ordinary textured and reflected rings now publish immutable SDK
combiner descriptions, then update only their live ambient/reflection colours.
The reflected route no longer constructs and binds the ordinary material first.
All six stages are still republished after other draw owners; fog and texture
ownership/invalidation remain intact. Pinned Melee raster invalidation and the
pinned SDK `C3D_SetTexEnv` copy/dirty-state implementation were consulted.

This adds 360 bytes of fixed CPU storage, prepared even when splashes are off;
it adds no GPU resources, vertices, instructions, passes, worker or file I/O.
OFF still skips per-frame effect work and reflection capture admission. Existing
capture resources remain resident after the first opt-in use until shutdown.
All four presets retain SPLASH REFL OFF. Incomplete effects may be connected
as opt-ins under the user's authorization; this does not imply visual parity.

Actual archived-v5/current-v6 SDK publication matches in 3,072 cases covering
all intensity-format combinations, three modes, live colours, and poisoned
previous-owner state. Templates remain immutable; omitting the last-stage reset
is rejected. Original/native post-sample equations still pass 524,288 channels
at zero distortion. The actual ring owner passes 128 load/retire cycles with
delayed readers and no per-draw allocation or geometry writes. The first expanded
host test rejected its own reflection-only non-null texture stub; allowing the
existing ordinary route's legitimate disabled samplers repaired that harness.

Shared v6 normal3dsx `bef4c5151310` /CIA `f09a3904a8e4` and replay3dsx
`108888816502` /CIA `1f7e7a603d62` are built, **not deployed**. Exactly three
native material-owner files change from v5. All shader bytes, the game archive,
presets and converter/install recipes remain unchanged; Old80/New124 profiles
are verified. Windows CRLF versus archived LF is recorded by normalized source
text checks; raw archive hashes are separately verified. The accepted 62m58s
conversion evidence is reused, with no new full conversion or normal-startup run.
Evidence: `splash-material-publication-test/{result,build-verification}.json`.

Serial v6 New/Old reflected-route replays both pass state, save, log-drain and
owned-exit checks. Their 1740 captures retain world/Fox/HUD and Old touch controls;
they do not isolate the small transient splash or establish original-image parity.
New reports 57.4515 Azahar FPS, scene1.80850/submit3.23541ms; Old50.5784,
scene1.81928/submit3.25728ms. Compared with v5, combined rendering CPU changes
+0.10781/+0.00512ms. These runs establish **no speedup**: splash counts/poses
differ (New102 to71 rings, Old95 to96). All142/192 reflected draws retain fog.
Both changes stay within the existing0.20/0.50ms short regression tolerances.

Heap peaks remain49,210,640/71,303,168 New and41,674,000/50,331,648 Old;
minimum free linear memory16,756,224/25,165,824 and2,073,088/10,485,760.
Both keep4,690,944VRAM free and73,068/524,288 measured stack. Process commitments
are103,333,888/130,023,424 and67,682,304/83,886,080. Linear variations reflect
different workloads; the fixed material storage does not reduce GPU allocations.
Existing default-OFF capture evidence is reused because admission is unchanged;
all three material modes have exact state comparisons. No further console run
is requested for this small architectural cleanup. See the same directory's
`runtime.json` for complete records.

An archive-patch repeat check caught a duplicate include introduced only by
reapplying the helper; fixed before further builds. Reapplying now reproduces
the exact checked v6 source text. The successfully built/tested binaries were
unaffected. `patch-check.json` binds this repair to the existing snapshots.

Source investigation also confirms that the three reflection constants select
separate red, green and blue contributions, and their indirect matrices differ.
A single averaged UV offset would not reproduce that sampling. v6 deliberately
leaves distortion unimplemented; this material-publication change makes no
additional visual-fidelity claim.

13 October 09:30 UTC — **v5 restores per-fragment distance fog on reflected
splashes**. The v3/v4 limitation below is historical. The new shader publishes
`view=(0,0,1+eyeDepth/normalization,0)` and the identity normal quaternion to the
existing fog-only fragment-light owner. Its distance-attenuation LUT yields the
source exponential transmission; combiner stage5 blends fog RGB after foam and
preserves material opacity. All three water texture coordinates remain available.
No extra pass, image, geometry buffer, worker or CPU vertex operation is added.
The shader grows from28 to32 instructions and uses the existing c94 fog ABI
(96 float registers including padding/constants). The shared environment owner
restores ordinary scene lighting after this fog-only use. DISTANCE FOG OFF
disables the fragment-light contribution; SPLASH REFL remains off in all four
presets and its OFF admission/resource behaviour is unchanged.

The actual linked shader passes32,768 float24 vertex cases: geometry, opacity,
mask/foam coordinates unchanged; projected relative error1.91e-6; encoded fog
view and quaternion correct. The original/native post-sample material comparison
still passes524,288 channels with zero sampled error at zero distortion. The
shared fog owner passes200,020 exponential comparisons and cached/toggle state
checks; the shared environment owner passes300 fog/ordinary/fog-only changes,
LUT caching and eight-light rejection. These are component proofs; exact original
water-image fidelity and physical execution remain unvalidated.

Serial `splash-reflection-v5-on-new` and `splash-reflection-v5-on-old` replays
complete with state assertions, saves, log drain and owned-process exit intact.
All204 New /190 Old reflected draws report fog enabled. Inspected1740 images
retain world/Fox/HUD and Old touch controls; the small transient ring and differing
animation phases do not establish pixel parity. New reports57.8111AzaharFPS,
scene1.77872/submit3.15738ms; Old50.4971, scene1.81760/submit3.25384ms. Versus v3
reflected runs, combined rendering CPU changes -0.01986/+0.05981ms; within the
0.20/0.50ms gates, but varying splash counts mean no speedup claim. No additional
default-OFF boot was repeated for unchanged admission/control/capture code.

New heap49,210,640/71,303,168, linear free16,749,056/25,165,824; Old
41,674,000/50,331,648, linear free2,067,968/10,485,760. Both retain4,690,944VRAM
free and73,068/524,288 measured stack. Process commitments stay103,333,888 /
67,682,304 within124/80MiB grants. Fog reuses already-resident LUT/light storage.

Latest local normal3dsx `4f473946578a` /CIA `ea9a79faf9aa` and replay3dsx
`5d39b1e5e544` /CIA `272e97910ea6` are built, **not deployed**. Exact snapshots:
`splash-reflection-v5-{console-setup,console-replay}-builds`. The only changes
from v4 are the reflection shader and reflection owner header. Both applications
share these files and the exact shader bytes; game archive, prior shader programs,
installer recipes and capture ownership are unchanged. Existing normal-specific
setup integration remains unchanged; there was no new normal startup or hour-long
conversion run. Evidence: `splash-reflection-fog-test/{runtime,build-verification}.json`
and `splash-reflection-fog-shader-test/result.json`.

Indirect water distortion, five-bit filtered bump opacity, original capture
boundary/image acceptance, ripples/wakes and general water surfaces remain open.
The full port/performance goal is incomplete.

## Initial v3/v4 implementation and preserved evidence

13 October: the user's explicit authorization permits incomplete native effects
to be integrated behind graphics controls, **off in all four presets**. A failed
default-on performance gate does not prohibit an explicit experimental opt-in.
Safety, resource ownership, truthful scope and OFF skipping work still apply.

The first connected consumer is **SPLASH REFL**, graphics page 11, alongside
WEATHER and FUR GROOM. It requires SPLASHES and particle textures enabled. It
uses the existing immutable splash ring, its live source band matrices and alpha,
and a separate 28-instruction PICA program. The game archive, simulation and
every prior linked shader are unchanged. This is not the retired expanded-vertex
renderer. Shared Old 80/New 124 MiB CIA and 3dsx builds are produced with
`tools/build_splash_reflection.py` from checked source archives.

The first active ring in a frame requests the native GPU-owned 128x128 RGB565
scene-colour snapshot. Later rings reuse it. Ordered command splits publish the
framebuffer before sampling and the capture before returning to the main target;
existing joined shutdown owns retirement. No raw depth transfer, CPU pixel copy,
per-frame vertex generation, new worker or second world render is introduced.
The optional target costs 32 KiB VRAM and its immutable quad 240 bytes of linear
storage, plus small SDK descriptors/shader state. Allocation preserves a 1 MiB
VRAM floor and 15% linear headroom. After first use these bounded resources stay
resident until shutdown, even if disabled; disabling stops capture/draw work.

The material retains the source's live three reflection RGB constants, ambient
term, band/mask opacity and foam blending. The three original indirect samples
currently share **one undistorted projective sample**. Normalized bump alpha
still uses continuous mask alpha rather than the original filtered five-bit
quantization. All three coordinate channels are occupied, so **distance fog on
this optional reflected ring is not implemented**; foam coordinates are never
misrouted into the fog LUT. The original non-reflected ring retains its existing
fog. Snapshot timing is immediately before the first splash, not yet validated
against the original reflection-copy boundary. Ripples, wakes, general water
surfaces and complete distortion remain separate incomplete work.

The menu bit previously collided with an inactive placeholder; that obsolete
check was removed. Actual UI taps/locks, replay mask admission, persistence and
all four presets pass. Custom retains an explicit opt-in. The capture admission
test executes the real owner function: 4,000 OFF cases do no capture or owner
submission, 1,000 frames capture once, 10,000 repeated requests reuse it; deleting
the OFF guard fails. Existing ring retirement passes 128 load/draw/unload cycles
and rejects missing-fence and shutdown-reopening mutants.

Actual original `setupWaterReflectionTev` versus the native SDK descriptors:
524,288 continuous post-sample comparisons match with zero sampled error when
indirect offsets are zero. A lost-ambient mutant fails. This does not validate
texture filtering or GPU fixed-point arithmetic. Exact linked Azahar float24
execution checks 32,768 ring vertices: position, alpha, mask and foam coordinates
are bit-identical; projective-coordinate relative error is at most 1.91e-6.

Serial v3 New ON/OFF and Old ON runs complete the same isolated Thorntail movement
recipe, preserve saves, drain logs and exit their owned emulator. New ON/OFF
reports 57.5455/57.4425 Azahar FPS, scene 1.78967/1.79224 ms, submission
3.16629/3.17124 ms. Different animation/splash counts (72/97 rings) make the
-0.00752 ms combined difference noise, not a speedup. New ON uses 50 captures /
144 reflected draws; OFF uses zero captures/draws and no extra capture allocation.
Old ON reports 51.0617 Azahar FPS, scene 1.80889 / submission 3.20274 ms, 72
captures / 196 reflected draws. There is no same-binary Old OFF measurement.
These are emulator observations, not console throughput or physical acceptance.

New ON heap peak 49,210,640/71,303,168; linear free 16,745,984/25,165,824. Old ON
heap 41,674,000/50,331,648; linear free 2,071,040/10,485,760. Both have 4,690,944
VRAM free and measured main-stack use 73,068/524,288. Process commitments
103,333,888/130,023,424 and 67,682,304/83,886,080 stay within grants. All measured
headroom gates pass. Captures retain world/actors/HUD and Old 1740 shows a ring
at Fox's feet. New rings are small/partly hidden by reeds; original-image
reflection fidelity is **not accepted** merely from draw counts.

Evidence is under `artifacts/research/pica-scene-rewrite/`:
`splash-reflection-{capture,material,shader}-test`,
`splash-reflection-runtime/comparison.json`, and `splash-native-test/lifetime.json`.
Runs are `artifacts/native/splash-reflection-v3-{on-new,off-new,on-old}`.
V1 failed shader-output packing; V2 failed an SDK enum name; V3 builds/runs pass.
V4 only corrects an obsolete log field that said `reflection=0`; v3's separate
`PICA splash reflection` row is authoritative. No console upload is claimed.

The following ripple/wake compiler is an earlier, separately checked component;
it is **not** connected by the splash option yet.

The actual original `track/intersect_render.c` functions
`setupReflectionDistortTev` and `setupReflectionBumpDistortTev` both combine two
projected samples of `gNewShadowReflectionTexture`. Their first stage adds the
ambient tint (ambient RGB shifted right three bits), or averages the sample with
heavy-fog RGB. Their second stage blends that result with a second scene sample
using the per-channel reflection K color. Opacity is source A1 multiplied by the
normalized indirect bump alpha. Wake distortion uses the authored wake texture;
ripples use the separately generated bump texture. These are not merely colored
ring sprites. The original reflection source is a captured scene color image;
the two source texture matrices include an alternate Y mapping.

`pica_water_material.c` now prepares the two native Citro3D combiner descriptors.
They consume TEX0/TEX1 and a primary color whose RGB contains original C2 and
whose alpha contains the source opacity product. This contract deliberately does
not pretend that producing the texture coordinates or opacity is implemented.
The compiler allocates nothing, publishes no GPU state, and creates no geometry.
It can prepare the normal and heavy-fog variants outside the draw loop; dynamic
K color can subsequently update the second descriptor's constant.

For heavy fog, PICA interpolation with128/255 replaces the source half-sum.
Adding and clamping first would destroy bright values before the division.
The interpolation's mathematical error is at most0.5/255 before target rounding;
the sampled comparison peaks at0.001899749 normalized units. Normal-mode RGB
and both modes' opacity match the continuous source equations within1e-6 in
the test. This is **not** an assertion of bit-identical GX/PICA fixed-point
fragment results or of image parity.

`test_pica_water_material.py` compiles the actual two original material setup
functions and the actual native compiler against pinned Citro3D texenv helpers.
It records the original GX inputs/operations and interprets both post-sample
equations across1,048,576 channel comparisons, both material families and fog
modes. Clamp-before-half and lost-opacity mutants fail. ASan/UBSan and strict ARM
compilation pass. Evidence: `water-material-test/result.json`. The test starts
after texture sampling; it cannot validate distortion, projected interpolation,
mask quantization, filtering, frame capture, resource ownership or GPU throughput.

The existing native frame owner already has a128x128 RGB565 color snapshot:
`platform_native_surface.h:native_surface_capture`. It samples the GPU-owned
power-of-two VRAM framebuffer view with an immutable quad. Its command splits
separate framebuffer writes and texture reads, and it restores the main target.
This is the candidate capture service to adapt, not the old CPU-visible EFB
conversion. Pinned Melee's `renderer.c` EFB path and `command_cache.c` were read:
reuse their explicit publication/lifetime discipline, but do not reintroduce
the raw transfer/readback path that previously failed on this console.

Remaining ripple/wake integration requirements:

- Capture at the original scene boundary, only for admitted live consumers and
  an enabled effect. Retain GPU ownership through all later samples and shutdown.
- Supply live camera/pose and the original two projection matrices. PICA's
  projective2D sampling is available on texture unit0; unit1 cannot simply be
  assumed to perform the same homogeneous division. The checked Azahar software
  rasterizer and fragment generator reflect that distinction. A single-pass
  vertex approximation needs quantified interpolation error, or an explicitly
  budgeted native alternative. No shader is accepted yet.
- Preserve source distortion and normalized bump-alpha semantics. Prequantizing
  a mask before bilinear filtering is not equivalent to quantizing the filtered
  sample; do not silently substitute one. Ripples' changing bump image requires
  a different ownership strategy from wakes' immutable authored texture.
- Keep original pools, spawn/update/RNG, active/fade state and draw order. Use
  persistent geometry; do not revive `lightmapDrawTriangleList` CPU expansion.
- Preserve distance fog, blending and depth without exceeding texture/coordinate
  slots. Do not silently use the fog coordinate for the third water sampler.
- Add a persistent graphics control, default off until measured. Initial added
  CPU budget is0.20ms New/0.50ms Old; GPU work, capture cost, normal/linear/VRAM
  storage and stack must be measured separately inside whole-frame targets.
  Reuse the existing32KiB color snapshot and240B quad only if their lifetime and
  admission are proven; those bytes are not free when first activated.

Other water families, dynamic reflections, complete rendering parity and the
physical frame-rate goals remain incomplete. No additional hardware probe is
requested for this unintegrated component.
