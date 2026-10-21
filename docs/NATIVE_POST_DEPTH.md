# Native post-effect depth access prototype

14 October latest: real source-triggered dialogue coverage passes on both
emulated hardware profiles. A combined colour/depth capture candidate passes
pixel, temporal and resource-denial checks, plus both dialogue replay profiles.
Neither local blur implementation is deployed. Original-image parity, physical
PICA acceptance and console performance remain incomplete. Heat distortion is
still missing. Earlier records below describe their original scope.

The original `track/intersect_render.c` effects use the high byte of the scene
depth capture for their alpha masks. The native renderer already owns a tiled
D24S8 depth attachment. `fh_pica_depth_view.h` constructs a checked, borrowed
RGBA8 texture descriptor for that storage: G is the high depth byte, while R
is stencil. It does not allocate, copy, read back, or own the underlying memory.
The source depth direction is reversed from the native attachment, so the
prototype samples one minus G. Original half-resolution capture/filtering and
boundary quantization still need comparisons before this becomes a game effect.

The caller must publish attachment writes, switch to an independent destination
without the sampled depth attached, and retire GPU use before the target owner
reuses or frees storage. Never delete, load into, or flush this borrowed texture
as if it were independently allocated. The helper rejects unsupported formats,
dimensions, alignment, memory ranges and overlapping colour/depth storage.
Pinned Melee EFB lifetime handling and the SDK frame-split rules were consulted.

`tests/native_depth_view_device.c` renders known depth/stencil values, changes
targets, and samples the depth view using the exact linked game UI shader.
`tools/test_motion_blur_device.py --depth-view --elf ...` runs it unattended.
The two local runs are under `artifacts/native-motion-blur-device/`:

- `depth-view-v1`: eight raw-G cases, zero sampled-byte error.
- `depth-view-v2`: eight inverted-G cases, including both clip endpoints,
  zero sampled-byte error. Case6 image inspected: expected black/white halves.

These are Azahar/llvmpipe correctness checks, not physical PICA acceptance or
performance measurements. The emulator explicitly implements D24S8-to-RGBA8
reinterpretation. Its existing general depth-UNORM precision issue is separately
documented by `test_depth_unorm.py`; these samples do not prove every depth code.
There is no CPU depth readback and the former raw-depth-copy path stays retired.

## Native blur material, 13 October

`platform_pica_blur_material.c` now prepares a fixed two-pass implementation of the
original `doBlurFilter` equations. The source uses five samples, but repeats
the negative vertical offset twice. Pass one uses three samplers for the
up/left/right contributions (weights .4/.2/.2); pass two adds the centre at .2
and computes the source-selected depth mask using the borrowed depth texture.
The first sum is at most .8, avoiding the original unclamped intermediate sum
that would exceed the PICA combiner's range. This is a fixed material compiler,
not per-draw interpretation of GX calls.

The alpha path keeps the normal symmetric, normal one-sided and stronger
symmetric modes. Its delayed combiner-buffer handoff preserves both signs of
the depth difference. The 480-byte prepared state table only needs the live
threshold byte updated. A separate 11-instruction affine UV shader supplies
three independent coordinate sets; the actor shader is unchanged. There are
no CPU image reads, per-frame expanded vertices, geometry shaders or workers.

`test_pica_blur_material.py` extracts the original function's actual TEV calls
and compares them with the actual native SDK state across 262,144 cases.
ASan/UBSan pass; maximum floating-equation difference is 1.43e-14. Mutants that
remove the delayed buffer or halve the upward sample weight both fail. Estimated
RGB rounding through 8-bit native stages differs by at most 1.6/255 in this
corpus. The desktop Foxhollow Aurora renderer explicitly implements the GX
divide-by-two operation as floating `/ 2.0`; GameCube intermediate fixed-point
rounding remains a separate fidelity question.

`native_blur_filter_device.c` executes the actual material and new shader with
the linked game's UI/capture program. The eight Azahar cases exercise both
depth signs, all modes, changed thresholds, RGB565 history, the two passes and
target handoffs. All pass, with maximum sampled RGB errors 2.624/255 for normal
modes and 6/255 for strong modes; case0 was visually inspected. These samples
are inside constant-colour regions. Full spatial-filter/edge comparisons are
still required; the test does not prove them or source-image parity. Evidence:
`artifacts/native-motion-blur-device/blur-filter-v1/result.json` and
`artifacts/research/pica-scene-rewrite/blur-material-test/`.

This remains a local component, not a gameplay feature or deployment. The
fixture allocates a 512x256 RGBA8 partial result (512 KiB VRAM) in addition to
the existing RGB565 capture (256 KiB); those are fixture resources, not a
validated production budget. Before integration, preserve the original
conditional capture timing—including stale depth when capture is suppressed—
and establish bounded capture/partial ownership, OFF work elimination, frame
cost and visual acceptance. Sampling current depth unconditionally would
change that source behavior. Heat's indirect distortion is still missing.


## Capture ownership and packed target, 13 October

The native game adapter consumes the original focus point and mode flags. It
projects one point only when BLUR FILTER is enabled; it does not call the retired
GX renderer. The original source triggers include combat targeting, NPC dialogue
and placed cutscene focus points. The option is OFF in all four presets.
Original simulation, focus selection and conditional capture timing are unchanged.

Production capture shares the existing RGB565 motion-history texture. A separate
11-instruction shader captures the native depth byte through a borrowed D24S8
view, with ordered SDK frame splits and no CPU depth or colour reads. Suppressed
capture retains both prior colour and prior depth; switching OFF invalidates depth
history. Enabling without a new capture skips an explicitly counted unseeded draw.
Neither option ON captures the same colour twice. All readers retire through the
existing parent fence before textures/programs/linear quads are freed. Pinned
Melee early_queue.c completion/resource rules and Citro3D renderqueue/effect
publication were consulted; no register bypass or new worker is introduced.

The v3 owner reduced two RGBA8 targets to RGB8 (768 KiB). V4 packs the captured
depth byte in A and the partial blur colour in RGB of one 512x256 RGBA8 target
(512 KiB). The depth capture writes A only. The partial pass writes RGB only,
preserving depth, and samples only the distinct colour-history texture. The final
pass switches to the scene target before sampling the packed result. Two stable
sampler descriptors share its storage: linear for colour, nearest for depth.
Only one owns the allocation. This keeps resolution and all consumed bytes;
there is no extra pass or feedback sampling. Combined with the shared 256 KiB
colour history, total post-effect storage is 768 KiB VRAM and 1024 bytes linear.
This is 512 KiB less VRAM than the initial two-RGBA8 proposal including history.

Actual owner fixtures `blur-owner-v2`, `blur-owner-v3-rgb8`, and
`blur-owner-v4-packed` all pass eight cases: OFF, capture, suppressed capture with
changed current depth, new capture, OFF invalidation, unseeded re-enable, seeded
re-enable, and both toggles sharing capture. Every pixel in all eight images is
identical between all three designs. V4 records exactly four depth/colour
captures, six requests, ten filter draws and one unseeded skip. Its case3 image
was inspected. The packed and borrowed-depth material modes pass 524,288 actual
source/native equation comparisons under ASan/UBSan (max 1.43e-14).

`check_blur_spatial_capture.py` independently models capture sampling and the
source's repeated-upward five-tap kernel across 3,216 edge pixels in case3.
Maximum channel error is 2.712/255; reversing the vertical tap yields 68.686/255
and is rejected. This strengthens the former flat-colour-only test; it excludes
the discontinuous depth-mask crossing and does not establish original-game image
parity or physical sampler behaviour. Evidence lives under
`artifacts/native-motion-blur-device/blur-owner-v4-packed/`.

The source adapter's actual threshold helper passes 100,000 comparisons including
world-map offsets and OFF zero-projection checks. Malformed projections are
rejected before undefined float-to-integer conversion. The previous fixture's
copied threshold formula was replaced with extraction of the actual helper.

## Runtime evidence and remaining gates

V3 shared replay source `2c1258110e9b`, 3dsx `0fc089d47d8b`, preserves the deployed
shadow-fetch correction. All prior embedded shaders are unchanged; post_sample
is the only added shader. Only lightmap.o changes in the game archive. Shared
Old80/New124 CIA profiles are verified. V4 packed replay e5e9e4aebebf/CIA053a917ceb6e is built; source8b90fe89c5df.
There is no new normal or console deployment at this checkpoint. V4 retains
the same game archive as V3 and all existing shaders. Component owners match
the device fixture exactly apart from documented CRLF/LF source line endings;
the initial byte-only provenance check rejected those before normalization.

The full moving New route hit its 150-second host bound before finishing;
`blur-filter-v3-on-new` is a failure, not performance evidence. The first shortened
ON/OFF runs completed and retained visible world/Fox/HUD, but their HOLD step
accidentally disabled timing collection. These are correctness-only evidence.
The runner now explicitly validates a measured short window and uses distinct
`short-timed` labels so earlier failed/incomplete evidence remains intact.

Corrected serial New ON/OFF runs both complete with owned emulator exit, drained
logs and saves preserved. Across matching 480 simulation-tick idle windows:
ON 425 frames/53.1125 submitted FPS; OFF 414/51.6122. Rendering CPU delta is
+0.188145 ms (scene +0.003735, object render -0.016701, submit +0.201110).
Audio differs substantially, so higher ON cadence is NOT a speedup. These runs
contain captures but ZERO source-requested filter draws. They establish neither
full-effect cost nor visual acceptance. The complete wall partition and sparse
memory samples are retained in `blur-filter-runtime/v3-new-comparison.json`.
GPU pending-command spans overlap CPU and include service latency; they are not
exclusive PICA execution times. Unrelated host workload also limits interpretation.

Next validation budgets, set before packed game runs: aim for <=0.30 ms added
render CPU for capture alone on New and <=0.90 ms on Old; complete requested blur
must be measured separately within the overall 16.67/50 ms targets. These are
investigation gates, not permission to exceed total-frame targets. Optional
allocation retains a 1 MiB VRAM reserve and 15% linear reserve. Expensive combined
settings may fail current admission; graceful user-visible rejection or a shared
resource policy remains necessary before broad deployment. Do not lower reserves,
silently disable effects, claim unmeasured peaks, or call the feature complete.


Packed V4 serial New/Old palace checks both pass: 103.90/99.20 host seconds,
private saves unchanged, logs drained, owned emulators exited. Captures1350
inspected on both: world, Krystal, purple enemies, flames and HUD present.
The held targeting input did not select an eligible target: counters remain
ZERO filter requests/draws. Treat these as capture/regression and memory checks,
not source-requested blur acceptance. Do not repeat the same route expecting
coverage without identifying an actual dialogue/combat/cutscene trigger.

New/Old captured807/808 times. Measured windows360/359 frames at16.71368/
16.72410 ms per submission (emulator only). New scene1.82186/object1.80675/
submit3.57498 ms; Old1.79861/1.79957/3.57099. There is no same-binary OFF palace
comparison; these are not incremental cost gates or physical performance claims.
Direct capture CPU totals are nested in the existing submission bucket, never
added to it a second time. Whole-run depth-capture CPU means0.07335/0.07396 ms,
shared colour+depth means0.19832/0.19924 ms (include initial allocation).

Last memory rows: New heap48,597,272/71,303,168, linear free18,035,200/25,165,824;
Old heap41,060,608/50,331,648, linear free3,355,136/10,485,760. Both VRAM free
3,937,280. Process commitment103,342,080/130,023,424 and67,690,496/83,886,080.
Measured main stack72,996/73,008 of524,288. These remain sparse observations,
not continuous peaks. Per-run analyses are `blur-filter-runtime/v4-{new,old}-
palace-analysis.json` (remove the line break). All builds restore the worktree
and game archive in finally; all current owned runners are terminal. No FTP,
assets, installed executables, preferences or saves changed this turn.


## Optional post-effect admission, 13 October

The motion and depth-blur owners now reject insufficient memory without stopping
gameplay. Requested option bits are preserved; the lower-screen row displays MEM,
and the closed button displays GFX MEMORY. Turning that option OFF clears its
session rejection; turning it ON retries. The failure is logged once and retained
separately for replay honesty. Preferences do not acquire a silently changed mask.
This applies to these two post effects, not every optional owner in the renderer.

Allocation keeps the existing 1 MiB VRAM and 15% linear reserves. Buffers and the
shader are allocated before creating the render target. Target creation is last:
pinned Citro3D rejects RenderTargetDelete inside an open frame, so error unwinding
must never depend on deleting a newly created target there. No failed allocation
has been published to GPU. Partial textures/programs/linear buffers are released;
texture descriptors are explicitly zeroed because SDK TexDelete does not do so.
Previously resident resources retain the parent retirement fence. Neither a GPU
reset nor unsafe timeout-based resource reuse is added.

`test_post_admission.py` exercises the actual two owners with 12 allocation/reserve
failures, 12,000 rejected retries, then successful retries and complete cleanup
under ASan/UBSan. A mutant leaving a freed texture descriptor populated fails.
The existing 35,600 temporal/OFF cases, 100 lifecycles and source alpha checks pass.
The graphics menu test checks visible rejection, unchanged requested settings,
clearing/retry behaviour and 10,000 repeated denials without additional LCD flushes;
its admission-menu.png was inspected. The replay completion functions are tested
in 108 combinations; a matching rejected effect changes running/success status to
12, while preserving an earlier failure. The first test harness lacked the existing
input-stop stub and did not compile; corrected harness passes. result.json records
graphics_unavailable, and analysis also rejects that field even with status1.

The actual ARM allocator/device fixture `post-admission-v1` completes in16.21 host
seconds. It creates real VRAM and linear pressure, checks shared-colour denial and
then independent depth-owner denial with motion already resident. It renders through
each rejection, frees the pressure buffers and validates unchanged available storage.
After recovery all eight temporal images are pixel-identical to packed V4. These
are headless Azahar component checks, not physical console pressure acceptance.
No request to run a separate hardware probe has been made.

Shared V5 replay addd538d0894/CIA7348b09de657 is built locally with the exact
pressure-tested owners, the same game archive and prior GPU shaders. Its snapshots
restore the worktree/archive. A source-guided Thorntail conversation route is being
checked; source-requested filter acceptance, costs and physical GPU behaviour remain
open. No new normal build or deployment at this checkpoint.


V5 New game run `blur-filter-v5-on-new-dialogue` completes88.47 host seconds,
status1, graphics_unavailable0, unchanged saves, drained logs and owned exit.
Capture1350 inspected: Fox/world/HUD remain visible, but movement hit the outer
wall at approximately(-5994.4,-743.9,-2212.3), before the intended Thorntail.
The A press drew the staff rather than starting a conversation. Zero filter
requests is failed dialogue coverage, not proof of a broken source callback or
a reason to count the effect as accepted. Source bEnableBlurFilter and camera
triggers remain intact. Do not repeatedly rerun this route unchanged.

Its287 measured frames average20.979 ms in that different wall-facing scene;
scene2.42575/object6.04909/submit5.58713 ms. These are NOT comparable to the palace
workload or a speedup/regression against it. Whole-run captures808, no filter
draws. Last sampled heap49,194,328, linear free16,756,480, VRAM free3,937,280,
process103,342,080 and measured main stack73,068. The complete report is
`blur-filter-runtime/v5-new-dialogue-analysis.json`.

Successful resident post resources are retained until renderer shutdown, even
when their toggle is OFF, to avoid mid-frame destruction/fence stalls. OFF stops
the associated capture/draw work; it is not a promise of immediate VRAM release.
If a rejected effect cannot be retried within the reserve, restart with fewer
optional effects enabled. The renderer preserves the requested settings and shows
MEM rather than silently changing their visual meaning. Replay status12 prevents
such an omitted workload being accepted as a passing performance run.


The normal V5 setup app is now built (source69cb54b9394c, 3dsx7bfbcfd406ab,
CIA3beda0187483), not deployed. The first normal/replay verification rejected
four renderer source differences: replay inherited the older ETC-only movie and
format14-only texture loader, while setup already supported RGB8 movies and typed
texture packs. This is a real comparison mismatch, not a newline discrepancy.
The builder now selects the same setup snapshot for both application variants;
startup and replay behavior remain selected by their compile flags. A V6 replay
is being built; exact loader/renderer/shader verification must pass before treating
its timing as representative of the normal renderer. The original V5 replay and
failed comparison remain preserved. This does not retroactively invalidate its
owner-component tests, nor make its old-format asset runs new-format validation.


Shared-source correction is complete: V6 replay 19a96af57a89/CIA47a55a1ba14e
and V5 normal 7bfbcfd406ab/CIA3beda0187483 pass exact verification of111
renderer/loader sources, all16 linked shaders and game archive3001dd7502b2.
Both retain the tested shadow-fetch correction and Old80/New124 CIA profiles.
The allocation-pressure-tested owners match byte-for-byte. Builds and source
restoration are complete; neither app has been deployed. Evidence:
`post-admission-build-verification.json`. Startup-menu-only functionality is
excluded from the renderer equivalence claim. The source inventory records
build helpers too; it does not imply every inventoried file is linked.


The complete source inventories also match:588/588 files, with sixteen exact
linked shaders; verifier now requires that full equality, not merely the selected
renderer subset. Both build snapshots restored successfully. The current normal
startup menu has not been rerun; this is renderer/loader equivalence evidence.

V6 dialogue-v2 hit the150-second host limit (155.56 seconds including cleanup),
near the NPC tail, with no completed benchmark. V3 removes270 ticks of movement
against the earlier wall, extends the approach, settles targeting and completes
179.46 host seconds with status1, zero graphics admission failures, unchanged
saves and drained shutdown. Its explicit blur coverage assertion FAILS:1482
captures, zero requests/draws. Four read-only snapshots show currentTarget0;
closest Thorntail root is56.65 world units away. The source tests transformed
interaction-volume centres and radii, height, flags and visibility, not simple
root proximity. Do not infer a rendering bug or successful conversation from
these observations. Inspect the actual target volume before another route.
Capture1950 was inspected: world/NPC/HUD remain, with foreground reeds occluding
Fox. Both timed-out2100/final captures were inspected too.

This run included a read-only object observer and is explicitly excluded from
performance comparisons. `analyze_blur_filter.compare` now rejects that marker;
`post-admission-runtime.json` records the regression-only scope. Last memory
sample: heap49,194,280/71,303,168; linear free16,767,744/25,165,824;
VRAM free3,937,280; process103,350,272/130,023,424; main stack73,044/524,288.
These remain sparse samples, not continuous peaks. No physical run, FTP changes,
new deployment, gameplay blur acceptance or speedup is claimed.


13 October interaction-volume diagnostic follow-up (paused for physical shadow flicker):
`blur-filter-v6-on-new-short-timed` timed out after 155.91 host seconds and is
not a passing timing run. The read-only observer initially reused its outer
executable-base variable as a hit-volume pointer, causing host EIO; renamed
the inner variable to volume_base. No guest state was changed.
`blur-filter-v6-on-new-target-audit` then completed in 132.13 host seconds,
status1, saves preserved, logs drained and owned process exited. Read-only
observations1009/1113 captured actual authored/runtime interaction volumes.
The Thorntail interaction centre uses local z=-45 and radius48, not the body
origin. The previously approached root being56.65 units away therefore did
not establish interaction eligibility. The near-target live centre itself is
not yet observed; a revised ordinary-input route v4 is written but NOT RUN.
No blur activation or performance acceptance follows from this diagnostic.
Both diagnostics and observers are terminal. New physical entity-shadow
flicker takes priority; no console files were modified by these tests.

## Real dialogue coverage and bounded waypoint input, 13 October

The new replay command `WALK_TO x y z radius strength timeout_seconds` uses live
player position and camera yaw to issue ordinary on-foot stick input. It does
not teleport, write game/camera state, change collision, or alter simulation
timing. It gates input during loading/cutscenes and outside gameplay UI, checks
3D arrival, and times out on blocked/wrong-floor routes. Strength is 24..56 and
radius 5..100; arrival input is neutral. Waypoint approach is excluded from the
measured frame window. Recorded input-tape formats and their playback are intact.
This is an on-foot route controller, not a general vehicle/platform navigator.

Actual original angle/matrix functions validate 4,608 directions, changing
camera/cadence, invalid state and deadline cases under ASan/UBSan. The root-motion
plant is idealized and does not claim to emulate animation/collision. Existing
replay interpreter tests pass. V1 incorrectly gated on the title-ready flag,
which the original game can leave set during gameplay. V2 reduced stick input
too far near the target and stopped short. Both failed runs are retained; V3
uses gameplay state and constant walking strength until arrival.

Local replay build d6a95f83d4f7 (3dsx bb5dab2a9092, CIA9a6323e00e16) carries this
controller and the already-tested shadow depth header. It retains all sixteen
linked shaders and game archive3001dd7502b2 from the shared blur build, with
Old80/New124 CIA profiles. Snapshot/archive restoration passed. NOT DEPLOYED.

`replay-walk-v3-new-dialogue` and its `-off-new-new-dialogue` counterpart complete
the same normal approach and A-button conversation in64.22/61.58 host seconds.
The ON run issues163 source blur requests,326 filter draws and zero unseeded
samples. OFF issues zero capture/filter draws. ON1800/1920 and OFF1800 images
were inspected: the conversation starts, foreground characters remain sharp and
the ON background is blurred. Animation phases differ; original-desktop image
parity and physical PICA acceptance remain open.

The 161-frame measured dialogue windows average18.7146/18.7159ms in Azahar.
Scene/object/submission costs ON2.0842/3.9947/4.8297ms versus
OFF2.0453/3.9787/4.4468ms add0.43775ms rendering CPU. Audio work differs and
explains why equal total cadence is not zero effect cost. GPU pending observations
0.7129/0.3760ms overlap CPU and service latency; they are not hardware timings.
Report: `replay-walk-test/dialogue-comparison.json` under pica-scene-rewrite.

Same-binary Old run `replay-walk-v3-on-old-new-dialogue` passes in60.03 host
seconds,140 requests/280 draws/zero unseeded;1800 image inspected. Its measured
137 frames average21.9161ms in Azahar, not an Old console prediction. No Old OFF
pair was run. All three runs preserve saves, drain logs and exit their owned
emulators; no live object observer was attached during these measurements.

Last ON memory samples (bytes): New heap49,264,056/71,303,168, linear free
16,740,096/25,165,824; Old heap41,727,440/50,331,648, linear free2,051,840/
10,485,760. Both VRAM free3,937,280 and measured main stack73,044/524,288.
Process commits103,350,272/130,023,424 and67,698,688/83,886,080. These are sparse
samples, not continuous peaks. The effect owns524,288VRAM+512linear and shares
262,144VRAM+512linear colour history; no CPU pixel processing. Default remains
OFF in all four presets. No shadow hardware resolution or full parity claim.

## Combined colour/depth capture, 14 October

The previous owner used separate colour and depth capture draws. The new owner
stores colour RGB and depth A in one RGBA8 history image using the existing
affine post shader and one TEV stage. The filter's independent RGBA8 scratch
keeps its full 8-bit intermediate precision. Its nearest depth descriptor now
borrows history, while colour uses linear sampling. No pass samples its active
destination; source capture timing, suppressed-capture stale history and
motion-blend-before-capture ordering remain unchanged. Toggles do not reallocate
live history. The fixed RGBA8 format also increases motion-only storage256KiB.
Colour capture now retains8-bit channels rather than RGB565; it is not claimed
bit-identical to the earlier capture or original console quantization.

This removes one fullscreen capture draw and two target transitions per captured
frame. History grows262,144bytes; combined storage is1MiB VRAM and1KiB linear.
Normal/linear/process allocations are unchanged. The1MiB VRAM and15% linear
admission reserves remain enforced. Both effects still default OFF in all presets.
Pinned Melee renderer.c::gpu_efb_copy and citro3d_fix.c were consulted for source
ownership, descriptor identity and cache publication. No SDK bypass, CPU pixel
conversion/readback, added shader instructions or worker is involved.

The first device candidate missed the old UI shader's final1-v transform and
failed the pixel oracle; its images/results are retained as
`combined-post-capture-v1`. V2 corrects the affine coordinates. Eight actual GPU
owner cases pass with maximum flat-region channel error1/255. Independent
3,216edge-pixel comparison gives max1.999/255; reversing the source asymmetric
kernel fails at68.1/255. V3 admission fixture additionally asserts exactly one
capture draw and exercises VRAM/linear denial, toggles and stale-depth history.
All owned device emulators exit. ASan/UBSan owner35,600cases/100lifecycles and
12allocation failures/12,000denied retries pass; the missing-reset mutant fails.
These are emulator/resource model checks, not physical raster acceptance.

Local replay79da13900cc0,3dsxb94636c124e8/CIA5459467841ed builds from the tested
waypoint replay with only the two capture-owner headers changed. All16linked
shaders and gamearchive3001dd7502b2 are identical; Old80/New124 preserved and
worktree/archive restoration verified. Build/run/analyze helpers are
`tools/{build,run,analyze}_combined_post.py`. NOT DEPLOYED.

New/Old ordinary dialogue runs pass in63.09/59.35hostseconds, preserve saves,
drain logs and exit; both1800images inspected with sharp foreground and blurred
background. New171requests/342filterdraws and Old142/284; zero unseeded samples.
The completed routes eliminate1,280/1,156capture
draws. No live observer was attached. New measured frame time18.7146->17.9156ms;
Old21.9161->21.5881ms (`combined-post-test/old-comparison.json`).
These are Azahar numbers only. New submission4.82965->4.42079ms; Old
4.61677->4.41067ms. Scene/animation/audio work differs, so the total deltas cannot
all be attributed to this change. Direct capture-owner CPU per capture falls
0.198754->0.120888ms New and0.199105->0.121197ms Old. Those nested spans must
not be added to submission time. GPU pending observations are overlapping spans,
not console GPU execution timings.

New last heap49,264,056/71,303,168, linear free16,740,096/25,165,824;
VRAM free3,675,136; process103,350,272/130,023,424, main stack73,044/524,288.
Old heap41,727,440/50,331,648, linear free2,051,840/10,485,760;
VRAM free3,675,136; process67,698,688/83,886,080, same main stack.
Full windows are retained in the reports. Sampling is not a
continuous peak measurement. Whole-game performance targets, original-image
parity and physical shadow stability remain unmet. No fresh console test or
additional probe was requested for this local change.

Matching normal setup9d99a0d60bb2 (3dsx7a60cd736c5b/CIA7c8e47eca438) is also
built, not deployed. All588source files and the game archive match the tested
replay; both builders verify identical linked shader hashes against the same
base, Old80/New124 memory profiles and successful restoration. Normal conversion
recipes are unchanged from the archived blur setup; a fresh whole-disc conversion
and normal menu boot were not repeated for capture-only changes. Evidence:
`combined-post-test/build-verification.json` and both build scope/restoration files.

The actual motion-only owner with the normal app's linked UI shader also passes
eight temporal pixel cases (maximum channel difference0.3765/255), including
OFF and suppressed capture. `native-motion-blur-device/combined-post-motion-only`
records the independent run and owned exit. It is not a normal gameplay boot.
