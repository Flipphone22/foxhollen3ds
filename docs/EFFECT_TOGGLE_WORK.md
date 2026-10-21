# Rendering toggle CPU work

12 October 2026. Latest local candidate `ec2541368491` / CIA `68fa73f35d89`
(`effect-work-v10-builds`). V10 removes redundant fog-image generation;
V8/V9 add the HUD and movie work gates described below;
the earlier V6 Old CIA-content check covers the unchanged particle/morph paths.
Not deployed: the requested combined update also requires the outstanding sky repair.

Disabled visual consumers now reject work earlier:

| Option | CPU work skipped | Work retained |
| --- | --- | --- |
| LIGHTING | Object light selection, conversion and parameter assembly | Pipeline identity, object colour-fade publication |
| ENVIRONMENT | Environment light snapshots and map preparation | Source distortion-group boundary reset; independently enabled specular |
| ACTORS | Camera, light and material preparation for ordinary native actors | Source pose, animation, collision and hit-volume updates |
| WORLD | Camera and live material preparation | World simulation, streaming and visibility state |
| WORLD/ACTOR TEXTURES | Animation-table and texture-slot searches, UV scroll extraction | Material colour, opacity and raster parameters; resident images for immediate re-enabling |
| HUD / HOLOGRAM | Native model UI or hologram parameter preparation | Game menu/input state |
| SKY | Cloud and moon actor preparation | Source sky/light state and cloud placement; glare RNG preserved |
| MORPHS | Repeated GPU slot retirement and cache publication | Latest positions retained in one existing CPU-owned slot before collision overwrites the source stream |
| PARTICLES | Native begin/end, scissor, texture-page lookup, draw parameters and dispatch | Original lifetime, alpha, motion and RNG updates; resource retirement barrier |
| WEATHER | Native begin/end, texture-page lookup, visual height/shape extraction and dispatch | Original flake motion, drift, colour and RNG updates |
| SKY glow | Lightning-glow camera matrices, texture lookup and draw preparation | Original lightning state and the same random-size update, including while hidden |

Morphs are not free while disabled: retaining the latest expression is necessary
for immediate re-enabling, including when the game does not produce another
morph update. No additional position buffer is allocated. The first hidden
update retires its chosen slot safely; subsequent hidden updates reuse it.
Re-enabling flushes the latest data before GPU consumption. The record adds one
word per instance, 256 bytes across the fixed 64-instance table.

This follows the pinned Melee renderer's explicit publication and queue ownership
rules (`reference/melee-3ds/native-3ds/port/3ds/renderer.c`, `flush_dynamic` and
`end_command_frame`) and the pinned citro3d `renderqueue.c` explicit-flush contract.
The adaptation defers publication while there is no consumer, rather than
removing required publication or retirement. No shader, render target, geometry
format or worker changed.

Validation:

- Actual lighting producer: 9,792 original-source comparisons and 1,000 OFF/ON
  cycles; disabled selection count zero. Missing-alpha and late-gate mutants rejected.
- Actual environment producer: 12,300 comparisons and disabled snapshot checks.
- Actual material producers: 1,000 OFF/ON cycles with poisoned animation readers
  while disabled; current animation restored on re-enable, colour/alpha retained.
- Morph owner: 192 retirements, 8,017 updates, delayed readers, slot reuse,
  toggles, source reset, and shutdown. 4,800 successive disabled updates perform
  no additional GPU retirement or flush. Ownership mutants rejected. ASan/UBSan.
- Corrected Old 80 MiB CIA-content run `effect-work-v5-off-old`: requested mask
  278727 asserted in the real startup log, 126 morph updates with zero GPU flush
  bytes, zero selected lights, zero diffuse/specular map updates. Isolated save
  preserved, Select exit and log drain completed. Capture 1410 inspected: geometry
  and HUD remain; world/actor textures are visibly disabled as requested.
- Those retained morph copies cost 8.688 ms across the whole emulator run,
  not per frame. This is not console timing.
- `verify_effect_work.py`: exactly three changed game archive members, seven
  adapter files and the morph owner; active shader unchanged; shared CIA retains
  Old 80 / New 124 MiB profiles.

The earlier `effect-work-v3-old` and `effect-work-v5-old` settings fixtures used
CRC32 instead of the file format's FNV checksum; the latter also wrote the mask
to the serial field. Both loaded defaults. Their boot/correctness evidence is
retained, but neither validates disabled effects or a speedup. The corrected
fixture explicitly asserts the loaded mask and disabled morph flush count.

The v6 Old check `effect-work-v6-off-old` completed at retrace 1741 with Select,
drained logs, unchanged saves and owned process exit (66.24 host seconds).
The startup log asserts mask 278726: particles, lighting, environment, world and
actor textures, and morph presentation disabled. Capture 1410 was inspected:
untextured world/actors and the HUD remain. It records 25,089 source particle-pool
visits but zero native glow pools, payloads, draws or parameter publications.
There were 167 retained morph updates and zero GPU flush bytes. Weather was not
active in this cave, so this run does not validate a live rain transition.

The actual weather adapter passes 90,000 source-coordinate comparisons and
10,000 OFF/ON motion comparisons, with visual data poisoned while OFF. The glow
adapter preserves the source state/RNG statements and gates the entire native
payload. `test_pica_cloud_work.py` executes the actual adapted glare body in
10,250 cases: hidden visual calls are zero and original random-call counts are
unchanged. Cloud placement/order, 41,984 glare size/alpha cases and 65,536 raster
cases also pass. C harnesses use ASan/UBSan.

`verify_effect_work_v6.py` verifies exactly five changed game objects relative
to the installed build. `verify_effect_work_v7.py` verifies that v7 changes only
the cloud adapter and its `9.o` game object relative to v6; all native code and
the active shader are unchanged. Shared CIA profiles remain Old 80/New 124 MiB.
The v6 cave evidence is reused for unchanged paths; v7 does not have a separate
runtime or physical performance claim. These checks establish skipped work,
not a measured console FPS gain.

V8/V9 address HUD lookup and prepared movie decode, as detailed below. Resident optional resources are not released on toggles.
No universal zero-cost claim, physical speedup, or complete toggle audit is made.

Sky investigation: `sky-wrap-original-new` explicitly selects original geometry
and textures with the hardware's effect mask. Captures 5040/6240/7440 show clouds
around Krystal, the ship and the fight. The run was deliberately stopped at 8400
before the recording ended, giving replay status 6, with drained logs and owned
process exit; it is diagnostic image evidence, not a completed replay. It does
not yet match the hardware's nearly flat-blue ship-front view. No sky fix has
been established. Four present cloud packets were compared across original and
performance packs: 1195/1198/1199 identical, 1196 differs by 312 bytes; 1197 absent
from both. This comparison alone does not establish the visual cause.

Further targeted captures: `sky-wrap-front-new` covers 5400–6000;
`sky-wrap-head-new` covers 5100–5400. Captures 5100/5160/5220/5280 and
5400/5520/5640/5880 were inspected. In particular, 5280 approaches the hardware
ship-front angle and still shows cloud detail, while the physical last-frame
capture is nearly flat blue. Lighting/scroll phases are not identical, so this
does not by itself prove a hardware-only cause. Both runs deliberately stop an
unfinished recording (status 6) and cleanly exit their owned processes; they are
diagnostic image evidence, not completed replay passes. The latter run overlaps
host compilation and must not be used for timing comparisons.

The archived replay has the same game archive and sky/main shader as the normal
deployment; differences are in shadow code. Hardware enables the original-geometry
7 MiB RAM cache, unlike these diagnostic runs. `check_sky_cache_bytes.py` checks
all 51,010,084 original geometry bytes plus 4,000 random reads through the actual
7 MiB cache reader, with ASan/UBSan: exact match. This rules out byte corruption
in that tested cache file/reader, not every runtime lifetime or hardware issue.
No sky appearance change has been accepted and neither console was updated.


12 October V8/V9 follow-up: HUD helpers now gate presentation before page lookup
and payload construction. Font parsing and caller-owned alpha are retained.
Actual adapted C checks cover 34,816 HUD transitions and 8,192 title glyph cases.
Prepared movie decoding now skips pixel decompression/copy and GPU publication
when MOVIES, ACTORS or RENDERING is off. Player/audio progression remains live.
Movie materials explicitly distinguish an unpublished movie frame from ordinary
material data, avoiding a placeholder texture while hidden. Re-enabling publishes
a newly decoded frame. The JPEG fallback remains outside this optimization;
normal prepared assets use FHPICA01. No new worker, buffer or GPU lifetime model.

Shared build a9e5f3dfaa1d (CIA fa74bf4ff0ad) passed serial New/Old normal touch
checks: hidden HUD/video restored, images inspected, Select/drain/owned exit.
384 actual decoder hide/resume transitions produced zero hidden GPU acquisitions,
writes or publications, with unchanged player state. Headless runs each resume
241 movie frames and 1,440 draws. These are correctness/omitted-work checks,
not a physical speedup measurement. Source/build verification and RAM evidence
are in effect-work-test/verification-v8.json, verification-v9.json and runtime-v9.json.

Sky diagnosis now captures full native draw inputs. check_sky_draw_sources.py
matches all ordered positions and UVs for original cloud models 1199 and1196.
The float24 interpreter emits finite positions and expected UVs. Two other
count-filter hits are unrelated models and explicitly excluded. The 5280 image
still shows clouds in Azahar, unlike the earlier console image. Matching source
inputs in this emulator run does not establish the hardware output or identical
weather/scroll phase. No sky repair is established. Both requested deployments
remain pending; saves and console graphics preferences are unchanged.


12 October physical sky isolation: a bounded FTP REST read fetched exactly
57,024 bytes containing both original warlock cloud texture records from the
console ISO. They match the local source exactly (remote-textures.json); no
remote asset changed. Source model1196 uses I8 texture0x88d; PICA format7 is L8,
not LA4. The sampled geometry covers the viewport without needing W-crossing
triangles. This is one captured angle, not every sky pose.

The separate sky GPU probe41929343edf6 uses the exact captured actor shader and
cloud inputs. All16,384 repacked pixels independently match the source; the
linked ARM NULL-sampler guard passes18 cases. Its9 isolated cases pass a3.45s
headless run; all captured images inspected. Four image pairs are byte-identical;
depth-off/passthrough differ at only2/4 pixels by at most2 byte levels. No console
fix is inferred from these emulator results. See SKY_GPU_PROBE.md.

Probe only installed to New223, staged and final byte-readback verified:
console-sky-probe/20261011-200027-223/transfer.json. No normal application, save,
setting or Old229 file changed. The requested two normal deployments remain
pending the sky repair. A single automatic hardware probe run has been requested.


12 October V10: removed the original CPU height-fog image generator, whose
4,096 generated bytes and subsequent 4,096-byte copy had no native consumer.
The immutable native atlas already contains all 65 exact source images. The
original byte intensity selector remains current even when HEIGHT FOG is off;
source texture allocation remains unchanged. Disabled block snapshots now avoid
texture, fog colour and inverse-camera lookups and clear the prior block state.

Actual generated-C tests cover 256 byte selectors, 266,240 original/atlas texels
and 2,000 off/on cycles with forbidden disabled reads and stale-state rejection.
Linked ARM frame-function execution reduces changed-intensity work from roughly
46,000 instructions to 93?106, with matching scroll/phase/selector state and
zero source-image writes/copies. External functions are stubbed; these counts
are neither console cycles nor a whole-frame speedup estimate.

The Old 80 MiB CIA-content normal cave run completed 1,861 frames with actual
touch toggles, clean shutdown, preserved saves and inspected captures. The fog
atlas produced 26,780 draws. Only the initial source selector update occurred,
so the run does not measure the benefit during changing camera height. Sampled
heap peak39,035,688/50,331,648 B; linear minimum free3,761,792/10,485,760 B;
VRAM free4,723,712 B; measured stack73,008/reserved524,288 B; process67,612,672/
83,886,080 B. Late-window Azahar cadence59.275 FPS is a changing-toggle
correctness observation, not a matched comparison or physical performance claim.

V10 changes exactly two game archive objects and three semantic source files;
active shader and shared CIA grants are unchanged. The first post-check assumed
a startup option-change log even though the default produces none; corrected
validation reused the completed run. There was no unnecessary second boot.
Both console gameplay deployments remain pending the independent sky repair.

The matching replay V10 is now built and checked: 3DSX `e50725868ec6`, CIA
`342551128e0b`. All 439 manifest sources, the game archive and active shader
match normal V10. Shared CIA profiles remain Old 80 / New 124 MiB.
The New cave replay completed normally with save preservation and an inspected
1410 capture showing the world, Fox, textures and HUD. Its 598 measured frames
averaged 59.831 Azahar FPS, scene preparation 1.877 ms and submission 5.677 ms;
this is a correctness observation, not a matched speedup or console prediction.
Heap peak was 48,404,624 / 71,303,168 B, linear minimum free 18,441,856 /
25,165,824 B, VRAM free 4,723,712 B, measured stack 73,008 / 524,288 B and
process commit 103,297,024 / 130,023,424 B. Only the initial source fog selector
changed, so this run does not measure the removed source rasterization cost.
Evidence: `effect-work-test/verification-replay-v10.json` and
`effect-work-test/runtime-replay-v10.json`. Both console normal builds remain
unchanged; the latest exact sky probe collection found no result.


14 October Old 3DS physical toggle audit (`22e57bb425dd`): collected logs and
both FHGRAPH5 settings slots read-only in
`artifacts/console-visual/20261014-110258-229/`. The startup receipt confirms
that the corrected CIA actually ran. Latest valid serial 5 selects mask
35667969: particles, weather and specular enabled, performance geometry/textures;
FPS/performance/GPU overlays all enabled. Ordinary lighting and morphs remain on.
Fur, ground shadows, height fog, diffuse maps and optional water/blur effects
are off. These are the user's saved choices, not an automatically forced preset.

Physical counters show zero fur draws, shadow draws/receiver queries, height-fog
preparation/draws, and diffuse-map updates/draws. Archived producers gate fur,
shadow queries, light selection, environment/specular snapshots, particle/weather
payloads and water capture before their visual preparation. Existing transition
tests remain relevant historical evidence; no new transition-test pass is claimed.
The run briefly toggled lighting/particles and later restored them, so total
counters alone cannot establish costs during those OFF windows.

OFF is not zero memory or zero load work: 30 shadow plans were built (18 declined),
with 521,648 linear bytes resident; shadow images retain 98,304 VRAM bytes. Two
fur groom builds retain 2,560 linear bytes, and three fur plans were built.
Simulation, collision, RNG and latest morph positions are intentionally retained.
No console settings/save/build changed, no extra probe requested, no FPS gain
claimed. Exact reviewed source hashes and observations: `effect-toggle-audit.json`.
