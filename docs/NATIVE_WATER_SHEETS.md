# Native ripple/wake sheets

14 October: native wake sheets are now connected to the original water DLL and
built into shared replay candidate `9904bce048d7`. Both emulated hardware
profiles complete a real Thorntail movement route with active wake draws and
clean shutdown. **Not deployed or accepted for visual parity/performance**.
Game simulation and existing splash rendering are preserved. This does not
promote the unaccepted camera-palette experiment.

The source `waterfx_render` draws ripples before splashes/drops and wakes after
them. Both use immutable quads, live translation/rotation/scale and opacity.
Wake quads are asymmetric (-200..400 on Z); ripple UV endpoints are127/128,
where wake endpoints reach1. Original `GX_VTXFMT0` texture coordinates have
seven fractional bits. The native kernel preserves these inputs without CPU
vertex rebuilding. Native owners must retain active/f18 checks and original
simulation, spawning and RNG.

The source material samples two independently projected scene coordinates.
PICA texture0 supports projective sampling; simply moving the second sample to
texture1 and dropping its denominator would distort it. Two ordered draws can
instead use texture0 for each projection. Each draw retains ordinary mask UV
on texture1 and fog through the existing fragment-light encoding.

## Material split

Let C0 be the first source stage after its clamp (or heavy-fog half-sum), C1
the second reflected sample, K the source per-channel mix, A the original
opacity product, and D the existing framebuffer. Original output is:

`(C0*(1-K) + C1*K)*A + D*(1-A)`.

`fh_pica_water_split_compile` prepares two pairs of SDK combiner descriptors:

1. C0*(1-K), blended with SRC_ALPHA / ONE_MINUS_SRC_ALPHA.
2. C1*K, blended with SRC_ALPHA / ONE.

Neither pass writes depth. Identical depth, culling, clipping and geometry are
required, and the second pass preserves destination alpha. Fog color is normal
in the first pass and black in the second, so fog is added exactly once. Both
passes borrow the same captured scene and same source mask/opacity; the first
pass must not cause a recapture or become the second pass's source image.
The source first-stage clamp remains before the split. Since K and A are
bounded, intermediate framebuffer saturation does not lose valid contributions
in the continuous model. This does not establish exact physical fixed-point
or framebuffer rounding.

Pinned Melee `citro3d_fix.c` sampler descriptor/dirty-state ownership and pinned
SDK texenv constructors were consulted. Compilation prepares immutable SDK
records; it does not publish GPU state or bypass cache handling. The module is
80bytes of descriptor output per prepared pair instead of the original40bytes.
There is currently no live allocation or added gameplay draw. Integration adds
two draws per active sheet and must measure that GPU/CPU cost before admission.

## Checks

`test_pica_water_split.py` executes actual original ripple/wake TEV setup and
the actual native compiler under ASan/UBSan. It compares1,572,864 continuous
RGB composites and as many illustrative nearest-rounded RGBA8 composites,
including heavy fog, independent K channels, opacity endpoints, backdrop and
distance-fog transmission. Maximum continuous difference0.001121342 comes from
the documented128/255 half-sum approximation; illustrative framebuffer error
is at most one8-bit level. Wrong first weight, lost second opacity, repeated
backdrop attenuation and repeated fog mutants fail. Strict ARM compilation
passes. The existing one-pass1,048,576channel test still passes unchanged.

`pica_water_sheet.v.pica` assembles to23instructions,96declared float registers
(82are padding to the shared fog ABI), no booleans, six output registers. Complete-output/control-flow
checks pass. The pinned Azahar shader interpreter executes65,536 ripple/wake
vertices with independent projective matrices and varying transforms. Maximum
relative arithmetic error1.90498e-5; geometry, color, mask and fog lanes are
bit-identical across the two passes. Dropped Q, wrong UV fraction and lost fog
mutants fail. The first invocation used an incorrect Picasso path; corrected
to `/sdk/tools/bin/picasso` before assembly. These are standalone programs,
not evidence of an application link, raster sampling or physical precision.

Evidence: `water-split-test/result.json`, `water-sheet-shader-test/result.json`,
and the retained original `water-material-test/result.json`.

## Persistent native owner

`platform_pica_water_sheet.c` now owns one128-byte aligned immutable allocation:
four16-byte vertices, six16-bit indices and padding. Each vertex fetch fills its
16-byte stride with two short4attributes, retaining the hardware fetch-layout
restriction learned from the shadow failure. The source quad and indices are
copied once at loading and flushed at that write site. One source mask image is
acquired through the existing shared image pool. Its scale and intensity/alpha
selection remain live in the prepared sampler. No draw allocates, reads files,
rebuilds geometry or samples mask pixels on the CPU.

Four six-stage SDK material templates cover heavy-fog/pass choices. Every draw
pass restores the program, all material stages, samplers, fragment-fog owner,
viewport projection, depth/blend/scissor state, attributes and buffer after
command reservation. The second pass uses its own reflection matrix and black
fog contribution. Both borrow the same captured image; this component contains
no capture operation. Live tint/opacity/mix/matrices are uniform updates.

Mid-game release joins queued readers before releasing the image and geometry.
Shutdown uses the caller's prior retirement and cannot reopen a command frame.
Insufficient headroom, geometry allocation failure or image admission failure
returns0with no retained resource. The future caller must mark the optional
effect unavailable and skip draws, without retrying every frame. Corrupt input
indices or invalid call order remain explicit contract failures.

The owner compiles with strict ARM warnings. At `-O2`, the standalone object has
592bytes BSS; GCC reports192bytes local stack for draw and280for shutdown.
These are component figures, not application-linked memory peaks or total
nested call-stack costs. The mask image pool, program allocation and optional
shared capture add their own costs; the128-byte geometry figure is not the
whole feature's memory budget. Current native capture is128x128RGB565
(32KiB VRAM) plus240bytes linear copy geometry if it is not already resident.

`test_pica_water_sheet_owner.py` executes the actual owner and actual SDK texenv
constructors under ASan/UBSan with delayed reader ownership and deliberately
poisoned state at every reserve, including between the two passes. It covers
128load/draw/unload lifecycles,2,562draws, clipped targets, both mask formats,
heavy/distance fog changes, immutable geometry/materials, exact draw accounting,
and three memory-pressure rollback cases. Missing retirement, missing second
material publication, wrong second blend, doubled fog and shutdown reopening
mutants fail. The first host harness compilation had mismatched SDK declaration
linkage and incomplete symbolic enums; corrected before test execution. This
models owner boundaries, not physical cache/raster behavior. No application was
built or run for an unconnected owner.

The shader now uses shared fog uniform94, matching the existing fog publisher;
its arithmetic/output tests still pass with23instructions. The initial low-slot
prototype could not directly consume that shared publisher. No instruction or
GPU pass was added by aligning this ABI.

## Distortion representation remains open

Read-only audit `audit_water_wake_grid.py` resolves the original32x64 RGB5A3
wake texture through TEXTABLE/TEXPRE. It writes hashes/statistics only, no
private pixels or converted geometry. A simple regular vertex-offset grid is
not acceptable on this evidence: at256triangles its sampled95th-percentile
source reflection-coordinate error is30.17texels; even4096triangles leaves
13.41texels. That may be worse than omitting the displacement. These are
base-mip sampled error statistics, not image differences, an exhaustive bound
or GPU timing. They reject blindly adding this particular costly grid.

Normalized indirect bump alpha is also still missing. Quantizing before
filtering is not equivalent to source quantization after filtering. An
illustrative5-bit alpha comparison reaches roughly0.0314error; this is not
accepted as exact GX behavior. Ripple bump images are dynamic and require a
different ownership design from the authored wake map.

The integration below connects source live matrices to the ordered capture
and sampler lifetime. Actual source image comparison remains necessary.
A zero-distortion prototype may be explicitly opt-in/defaultOFF under
the user's authorization; it cannot count as complete water parity. Keep the
initial added CPU budget0.20ms New/0.50ms Old, account for both draws and capture
cost, and measure GPU work and resident memory separately. Component checks
alone are insufficient for accepting this effect.

## Wake integration and real-game checks

`pica_water_sheet_override.py` prepares the original asymmetric quad once during
water initialization, using the existing authored wake mask. Source spawning,
pool updates, fade, RNG and collision code remain unchanged. Native render
admission retains `active != 0 && f18 == 0`; live translation/rotation/scale,
opacity, reflection mix, source projection matrices and ambient/heavy-fog tint
are consumed directly. CPU work is a bounded 30-entry admission scan, original
object-matrix composition per active wake, uniform publication and SDK command
submission. There is no CPU vertex rebuilding or retired GX draw interpretation.

An active wake captures the scene before water drawing. Both projected passes
and all other wakes in that frame borrow the same immutable snapshot; splash
reflection can share it. OFF, disabled particle textures, empty pools and
unavailable resources do not perform wake capture/matrix/draw work. Load-time
residency is still paid when OFF: 128 bytes of linear geometry, an 8 KiB shared
mask in this fixture, the small program and fixed material records. The optional
shared capture adds 32 KiB VRAM and a 240-byte copy quad when first used.
Budget admission failure is handled without retrying each frame. The existing
underlying capture allocator still panics on unexpected allocation failure;
the free-space guard is not a proof against fragmentation. Full transactional
rollback for that shared allocator remains a separate follow-up.

Hardware integration follows the pinned Melee `native-3ds/port/3ds/renderer.c`
write-site cache flush/queue retirement discipline and `citro3d_fix.c` descriptor
dirty-state handling. The adaptation uses Foxhollow's existing shared native
capture and SDK command queue. It adds no private queue, direct register writes,
readback or worker. Source GX's two projective samples become two PICA passes;
that is an explicit architectural departure, with the algebra and remaining
quantization/distortion differences documented above.

WAKES is an opt-in graphics control, OFF in all four presets. It uses the final
32-bit graphics-mask bit. Settings persistence and replay lock are retained;
the text replay parser now explicitly rejects negative/overflowing masks
instead of relying on an unused bit. Touch tests locate the actual option row
(the stable snapshot and unrelated worktree differ in WEATHER ordering).

Actual source spawn/adapter checks compare all 120 pool corners, source state
preservation, active/f18 selection, tint, fog, matrices and OFF/empty admission.
These, UI/settings and shared capture tests pass ASan/UBSan. Capture checks cover
4,000 OFF admissions, 1,000 captures and 20,000 cross-consumer reuses; optional
memory rejection balances ownership and suppresses repeated attempts.

The first app build failed because the SDK material compiler was sent to the
game's Clang path. It now uses the actual SDK GCC path. The first runtime (v2)
reached gameplay but **failed** at shutdown with an 8 KiB material reference:
the new owner was released after the image pool teardown. V3 releases it after
queue retirement and before scene/material shutdown. A build-order assertion
preserves that contract; both subsequent real-game runs pass. Earlier failures
remain recorded and are not counted as successful validation.

`water-sheet-v3-on-new` and `water-sheet-v3-on-old` pass replay state checks,
preserved saves, log drain and owned-process exit. New: 220 wake instances,
440 draws; Old: 188 instances, 376 draws. Neither has unavailable resources.
The separate 23-instruction shader leaves all 17 previous programs unchanged;
only `19.o` differs in the game archive. Shared CIA profiles remain Old80/New124.
The Old and New 1740 images were inspected: terrain, Fox and HUD remain intact,
including the Old touch item pad. Small water features are not isolated enough
to claim original-image acceptance or correct distortion (which is absent).

Observed resource samples, not exhaustive high-water proofs:

| Resource | New | Old |
|---|---:|---:|
| Heap peak / reserve | 49,210,992 / 71,303,168 B | 41,674,352 / 50,331,648 B |
| Linear minimum free / reserve | 16,747,904 / 25,165,824 B | 2,062,720 / 10,485,760 B |
| VRAM minimum free | 4,690,944 B | 4,690,944 B |
| Process commit / grant | 103,358,464 / 130,023,424 B | 67,706,880 / 83,886,080 B |
| Measured main stack / reserve | 73,044 / 524,288 B | 73,044 / 524,288 B |

The resource gates retain at least 15% linear reserve and 1 MiB VRAM headroom
in observed samples. Additional effects must account for the much smaller Old
linear reserve.

Matching normal setup build `00889a33880e` is also built, not deployed. Its
603 recorded source hashes, 18 linked shader programs and exact game archive
match the tested replay. Existing setup/installation recipe and Old80/New124
profiles are preserved. This is build identity evidence, not a new normal-app
runtime test; unchanged startup behavior was not booted again.

Guest cadence was 57.28/51.28 FPS, but **both timings are excluded from performance
comparison** because another project was compiling on the host. These are not
console FPS, host display speed or evidence of meeting the 0.20/0.50 ms added-CPU
gate. Recorded owner spans of 0.0775/0.0849 ms per wake exclude game-side matrix
preparation and shared capture, include scheduling, and must not be presented
as the complete effect cost. No redundant OFF performance run was started
under that known contamination.

Evidence: `water-sheet-runtime/result.json`, per-run `host-timing-context.json`,
`water-sheet-adapter-test/result.json`, `splash-reflection-capture-test/result.json`
and `water-sheet-v3-console-replay-builds/scope.json` under the usual research
and native artifact roots. Next: isolate affected pixels against original
rendering, obtain a clean matched toggle pair including capture costs, and
design indirect distortion/dynamic ripple ownership without a dense CPU mesh.

The existing physical normal-run tail still predates the deployed shadow depth
correction: whole windows5602..5861 cover260frames at39.557FPS, mean interval
25.280ms, submission11.089ms and GPU tail16.771ms. Those spans overlap; they
are not additive or an exact scene-matched baseline. It has seven intervals
over33.5ms, none over100ms in that tail. The overall hardware FPS, progression,
visual parity and rapid whole-shadow stability requirements remain incomplete.

## Captured GPU pixels and allocation recovery, 14 October

`tests/native_water_sheet_device.c` runs the production owner/material/shader
against a four-quadrant scene capture and an explicit mask. Eight cases cover
K endpoints and independent RGB channels, opacity, heavy-fog material colour,
projective Q and foreground occlusion. Every pass forces a real FrameSplit and
poisons prior program/material bindings. Hidden X11 captures observe the LCD;
there is no GPU depth readback or desktop input. The exact game UI entry comes
from the archived replay ELF. All SDK/tool sources are extracted and checked
against the immutable snapshot, keeping unrelated live SDK work out of the test.

`artifacts/native-water-sheet-device/linked-v1` passes: worst sampled error
3.75 RGB byte levels against a six-level tolerance for RGB565 capture. Captures
were inspected. Replacing projective Q with one makes case6 fail at73.22 levels
(`linked-drop-q-v1`); the test therefore detects that projection error. This is
a controlled material/projection test, not original-game image acceptance. Its
fragment-fog stub does not establish distance-fog correctness. Distortion and
dynamic ripples remain absent.

The shared optional reflection allocator previously panicked if texture,
target or copy-quad allocation failed after budget admission. It now constructs
the resource transactionally and publishes it only after all allocations succeed.
Each failure cleans its partial resources and returns NULL. The requesting
optional effect is marked unavailable and does not retry every frame. Successful
capture draw commands are unchanged. This only repairs the optional capture
allocator, not every essential render-target failure path.

`water-capture-allocation-test/result.json` exercises the actual factory under
ASan/UBSan:3,000 failures,1,000 successes and100,000 resident reuses. A missing
texture rollback fails. Shared consumer checks additionally verify balanced
ownership and1,000 suppressed retries after failure. The `rollback-v1` GPU
fixture has identical sampled pixels to `linked-v1`.

Shared `water-capture-v1` builds are **not deployed**: replay `a7a1605be736`
(CIA `e84e93db90b1`), normal setup `14dd9962313d` (CIA `cbca13e69837`). Only two
capture-owner sources differ from v3. Game archive `3bea7fb6f1ba`, all18 shader
programs and installation recipes are unchanged;603 source hashes match
normal/replay and Old80/New124 profiles pass. The capture-build-verification.json
report in water-sheet-runtime records identity. Existing v3 gameplay plus actual candidate
GPU/failure tests are reused; no additional normal-game boot or physical test is
claimed. Worktree/archive restoration is verified and all owned processes ended.

A clean timing attempt began with no competing compiler, but unrelated
compilation started during `water-sheet-v3-clean1-on-new`. Both that run and
the following OFF run are excluded from timing comparison. Both pass replay,
save preservation, log drain and owned exit; OFF reports zero wake objects,
draws and submission ticks. All four New/Old evidence runs remain in
`water-sheet-runtime/result.json`. No effect CPU budget or FPS gain is accepted.

`tools/audit_water_wake_knots.py` corrects the earlier grid experiment to use
texel centres, where bilinear slopes change, rather than texel edges. Across
three projective fields,4290 triangles reduce sampled95th-percentile coordinate
error from roughly14 to4.74 source texels;17160 triangles give1.18 and68640
give0.293. These are coordinate samples, not displayed-pixel quality or cost.
No dense mesh was adopted. The next design needs screen-footprint and actual
image comparisons before adding geometry. WAKES stays opt-in and default OFF.
