9 October LOCAL native time-of-day backdrop milestone. Ordinary2dac1b602577 / CIA9d168de15d60; replayfc215fa6d019 / CIA21af3ce158b2 / source200aa28747cd; game9d88c336cc00. Unified Old80/New124. All build/test/emulator sessions terminal. No FTP/deployment while hardware unavailable. Goal ACTIVE/INCOMPLETE. Previous goal turn and this turn made implementation/verification progress, not a wait/blocker.

Time-of-day backdrop now uses resident phase A/B textures and a frozen previous transition image, one persistent HUD quad, three fixed native PICA combiners. Source phase selection, six light/ambient table channels, camera pitch/roll/FOV texture coordinates and Sky2 tint calculation preserved. Restores gSkyCurrentLightColor/gSkyCurrentAmbientColor updates previously omitted with the whole draw. Original per-frame blendTextures calls are removed. No GX interpreter, source-image revalidation, per-frame pixel blend/upload or geometry rebuilding. Existing source textures are registered at original loads; only live phase/fade/tint/UV values change. Texture2 uses one additional HUD output/move:30 HUD instructions,256/512 shared actor/HUD/hologram instructions, four vertex processors/no GS. Linked shader5c0eb98a45ab; no extra shader uniforms or vertex buffer. Opaque depth-tested/no-depth-write raster policy preserves original backdrop behavior, and ordinary HUD state is invalidated/restored afterward.

At an environment change, fh_sky_freeze_transition materializes only the last displayed blend using original blendTextures, including a prior fade if present. This is a bounded transition snapshot, not recurring frame work. It uses original game-owned scratch images/slot swaps and stores only last factors/validity, not pointers that could become stale after relocation. Reset clears validity before free. The frozen image is registered once; existing HUD resource retirement precedes replacement. One observed4096-byte snapshot costs0.10084ms total in Azahar. This CPU work remains to preserve exact frozen-transition pixels without per-frame GPU readback or retaining an unbounded chain of old image blends. Future larger/repeated transitions need measurement. Pinned Melee c2ebe7c769b3 renderer.c texture_barrier and constant-alpha GPU_INTERPOLATE patterns consulted; native host retirement/exact cache publication reused, no new OS/worker behavior.

First replay8b8d7fac885c/native-opening-sky-old is REJECTED: native sky draws0 despite snapshot1. An older minimal_render_override wrapper left new quad in unreachable else, so that was not native visual coverage. pica_sky_override now removes the entire old wrapper; test rejects fh_minimal_/fh_gx_ calls and original GX/pixel-blend entrypoints in generated sky body. native_sky_gate.py rejects zero-draw runs and demonstrably rejects that first candidate. Do not reuse its timing or images as accepted native sky evidence.

Accepted native-opening-sky-fixed-old exactOld80 CIA-content recorded normal profile/opening replay completes77rows/status1/maxinputlateness0.987911075ticks, normal/source saves unchanged, owned emulator exits and drains logs.2190 native sky draws,1snapshot/4096bytes,8798cloud/266glare draws; no material errors/leaks. Image3990 inspected: gradient visible behind cloud layers/Krystal/mount/dialogue. This proves the opening path, not sun/moon/day-cycle parity or transition pixel identity. GPU fade branch is scalar-tested; separate visual crossfade coverage is not yet established. Result GCI remains fb8a389b1e75... identical to original New recording/prior Old playback. cloud-results.json, sky-timing-results.json, perrun sky-coverage.json contain evidence.

Matched opening renderframes3006..4386 (1380frames), versus accepted cloud+glare baab0: submission1.544859->1.580138ms (+0.035279), objectrender1.280173->1.289419, scenerender0.485440->0.486413, nonVBlank5.959923->6.016246ms (+0.056322). Cadence16.714071->16.713543ms,~59.8FPS Azahar ONLY. Report added work even though pacing hides it in FPS; no physical estimate. Normalpeak40670896/50331648, linearminfree4165248/10485760 (16384 fewer free vs cloud+glare), process67305472/83886080, activeVRAMfree4858880, stack72788/524288. Imagepoolpeak2973568/4194304, materialCPU283388/524288, movie98304separate. Native gradient retains headroom for this scene, not proof of fullgame residency/performance. Last matched palace/cave drift evidence is previousbaab0, not this sky build; physical still1a433 Old20.002palace/18.081caveFPS, New unmeasured.

Tests: test_pica_sky.py actual freeze helper/order/slot reuse plus unchanged phase/light/UV source checks and16777216 scalar one-channel blend cases. PICA ideal interpolation differs from original separately truncated /256 products by at most2/255 per blend; multi-stage rounding/filtering can accumulate differences, not bit-exact pixels. test_pica_sky_material.py9216 actual C combiner/raster publications. test_pica_hud.py actual gradient ordering/page ownership/ordinary-state restoration. sky-shader/run.py2304 standalone+2304 linked shader numerical cases, all5 outputs, maxerror5.96e-8. Earlier hud-shader fixture is stale (pre-projection; missing current registers), rejected before runtime; new fixture based on current hud-projection layout also fixes unsigned negative test-coordinate arithmetic. Do not treat historical fixture as current shader gate. build-native-sky.py reproduces both variants; run-opening-sky-fixed-old.py requires nonzero draws/snapshot.

NEXT: sun/moon and their light-direction state are still omitted: fh_pica_game_sky_update currently calls shadow/light updates but not original renderSunAndMoon. The latter computes original camera-relative sun/moon pose/opacity and gSkyTimeOfDayLighting directions before two objRender calls. Restore that game state and direct native actor dispatch (no GX geometry path). Moon has moonFxRenderCallback in track/intersect_render.c: base RGB; alpha times op/object alpha, attenuated by same texture sampled at U+mainGetBit(0x2ba)/30 and scaled62*4, standard alpha blend, depth test/no write. Requires semantic native material support before visual parity; do not just accept unknown callback as absent. General stars/morphs/shadows/secondary-world/particles/specular/essential UI and unknown callbacks, full normal opening->Fox and meaningful gameplay saves, extended stability/final physical performance remain unmet. Keep performance cost gates and current matched baselines; hardware absence is not a blocker.


9 October LOCAL native clouds/glare + matched hardware-build drift checkpoint. Ordinary648dc34e54db / CIA8e716721d008; replaybaab0c824adf / CIA8acf7495812f / source5497ee1f416a; game7265445ccdd1. Shared Old80/New124. All builds/tests/emulators terminal. No deployment/FTP; hardware unavailable. Goal ACTIVE/INCOMPLETE.

Cloud objects now consume the existing native actor geometry/material plans with original camera-relative placement, object order, visibility, tint and texture scrolling. Load-time prepared geometry is unchanged. Scoped native raster policy reproduces original blend/depth/color-write/alpha-test semantics, including the sun-occlusion depth-only pass; adds4 bytes/material. Original sky shadow-light direction and time-of-day light-state updates are restored separately from omitted sun/moon geometry. No GX visual interpreter or per-frame vertex reconstruction reintroduced.

Lightning glow uses one persistent projected native HUD quad, sky texture registered once at its original load boundary, original camera-space translation, size RNG call, fade alpha, blue tint and additive SRC_ALPHA/ONE. Existing texture-alpha combiner consumes the resident page; no shader additions or recurring upload. One CPU point transform per active billboard supplies live object position, not vertices. Flushes the quad before lower clouds to preserve ordering (command submission, not GPU wait). +4096 linear bytes. Pinned Melee c2ebe7c769b3 renderer.c texture_barrier consulted: reuse native HUD load/free retirement and exact upload flush; do not free or overwrite pending GPU resources. No worker/service/lifetime departure introduced.

tools/test_pica_cloud.py passes65536 actual original/native raster comparisons under ASan/UBSan,41984 glare size/alpha cases and strict source equivalence for original cloud placement/order; original lightning RNG guard/call retained. tools/test_pica_hud.py passes actual batching/ownership SDK-boundary checks. Runtime native-opening-cloud-glare-old exactOld80 CIA-content replay passes77rows/status1, maxinputlateness1.138900548ticks,55packets/peak56of512. Cloud8792color/0depth draws,280glare draws. Depth-only branch is scalar-tested but has NO runtime image coverage. Images2700/3990 show restored blue clouds;3810 comparison shows bright lightning glow behind actors. Capture camera poses vary slightly: image comparisons prove feature visibility, not pixel-identical deterministic playback or full fidelity. General sky gradient, sun/moon/stars and other effects remain absent. Character lighting parity remains unproven.

Opening matched1380-frame window3006..4386: beforeclouds/withclouds/withglare submission1.480411/1.540349/1.544859ms; cadence16.713681/16.713430/16.714071ms. Latest objectrender1.280173ms, objectupdate0.293428ms; observedGPUbusy0.303038ms is Azahar only, NOT physical GPU time. Latest normalpeak40670824/50331648; linearminfree4181632/10485760; process67305472/83886080; VRAMfree4858880; stack72788/524288. Native image peak2965376/4194304, materialCPU283388/524288, movie98304separate. Zero material errors, resources return0 at shutdown. Result GCI identical fb8a389b1e75... to original New recording and prior Old replay. Source/normal saves unchanged. Evidence cloud-results.json; build-native-clouds.py and run-cloud-milestone.py reproduce. No physical performance claim.

User asked accumulated Azahar drift since last hardware test. Fresh matching Old80 palace-facing/cave-idle runs compare exact last physical1a433cbe6d29/sourcee47d4f0443e4 with latestbaab0. Same saves, case/input scripts, firmware, movie, main worker and VRAMOFF; captures1290 inspected. Palace cadence17.289781->16.715833ms, nonVBlank14.059282->13.817256ms (-0.242026), submit3.491576->3.346803ms. Cave17.563772->16.715614ms, nonVBlank14.876724->15.148011ms (+0.271288), submit5.217821->5.134927ms. Restored lighting/HUD/menu/movie/clouds and frame-pacing change means NOT equal-fidelity pure optimization. Cave added text0.1298/objectrender0.2419ms partly offset by submission/UI. Do not hide this regression behind~59.8AzaharFPS; overall work roughly neutral, NOT a substantial speedup. Physical Old baseline remains20.002palace/18.081caveFPS, not inferred current FPS. New physical performance still unknown. Latest palace/cave normal40712488/40650936, linearfree5551360both, stack72836; process67305472. hardware-baseline-drift.json and measure-hardware-baseline-drift.py retain exact exclusive accounting. These are next stage comparison baselines; watch CPU/RAM as features return, request physical milestone only when user has hardware.

NEXT: replace time-of-day backdrop CPU blendTextures with native resident source/GPU blending, preserving phase/light/ambient color and transition snapshot semantics. Current pure light updates do not restore backdrop-owned gSkyCurrentAmbientColor/gSkyCurrentLightColor updates; must restore with original phase table, not guess a brighter tint. Engine5 skyRenderTimeOfDayBackdrop has phase A/B and previous blended snapshot(slot4); skyUpdateEnvfxAct swaps slot4 and current destination when environment changes. Do not re-enable per-frame CPU texture blends. Native general sky, morphs, shadows, secondary world, particles/specular/unknown callbacks, essential UI coverage, normal opening->first Fox progression, meaningful gameplay save/reload and final sustained/hardware validation remain unmet. Keep legacy renderer rejected. Continue autonomously; no hardware absence blocker.


9 October LOCAL opening-recording / residency follow-up (supersedes build IDs below). Ordinaryd60c135cbfe0 / CIA7bc5cfcf9fb4; replayd2fee9e977f5 / CIA4056c11f1931 / source3cea7a133151. Shared Old80/New124; gameff2d76c2b03e. All build/test/emulator sessions terminal. No deployment/FTP, hardware unavailable. Goal ACTIVE/INCOMPLETE.

Native cockpit movie is implemented and validated in prior checkpoint below. New ordinary native-opening-movie-record-new (6ac3c02625b0, prior identical New behavior) now creates profile AAA, selects Play Game and reaches Krystal's opening flight through normal inputs. Touch recording55packets/4562.51949simulationticks (~76s), fixture plus tape under opening-record-cases/record-0000000110a1fe66. Image3990 inspected: Krystal/mount/subtitles visible, SKY/CLOUD BACKDROP BLACK. This is real opening cutscene coverage, NOT complete opening gameplay/parity. Source GCI SHAab4698fe962f unchanged. Runtime ended via Select normally, owned emulator exited.

Old playback first failed at asset151 loading source256x384 CMPR: resident1863040 +262144 exceeds obsolete2MiB native image cap, while linearfree5745536. Preserved native-opening-record-playback-old rejection. Full New opening image peak2961280 (~2.83MiB) showed actual needed live set. Replaced Old-only2MiB image ceiling with shared4MiB ceiling, still enforces15% actual linear grant reserve on every allocation and existing512KiB material CPU cap. No texture omission, shader change, extra draw work, per-frame allocations or copies. A budget correction based on residency, not a performance gain. Latest ordinary New runtime behavior unchanged; prior New evidence reused. Both ordinary/replay rebuilt and unified profile/build gates passed.

Corrected Old80 native-opening-record-playback-old-resident PASS,77rows/status1/maxinputlateness0.958824ticks;55packets readerpeak56/512. Image3990 inspected same opening line/actors and same missing backdrop. Resulting isolated GCI byte-identical to New recording fb8a389b1e75...; original/normal saves preserved. Runtime autoexit4568 with logs drained, owned process exited. Imagepoolpeak2961280/4194304 (29.4% headroom), movie98304 separate, normalpeak40670504/50331648 (fullstackreserve18.16%), linearminfree4185728/10485760 (39.92%), process67305472/83886080 (19.77%), activeVRAMfree4858880, stack72788/524288. Movie983commits/1984draws/0overwrite drains/0copy bytes. MaterialCPUpeak283140/524288. No resource leaks/errors at shutdown.

Opening window renderframes3006..4386:1380frames,16.71368ms cadence,1.48041ms native submission,1.24617ms objectrender,0.29244ms objectupdate,0.30303ms observedGPUbusy (Azahar ONLY, incomplete sky/effects; not physical target achievement). opening-record-results.json contains exact counters, memory, identities and save equality. Native movie owner3000cycle/actual prepared decoder tests and12288post-state tests pass; no additional unrelated long boots. Reproduce build-native-movie.py, run-opening-record-new.py and run-opening-record-playback-old-resident.py.

NEXT: native sky/cloud restoration, now observed in normal opening rather than isolated palace. Root cause and boundaries in opening-sky-investigation.json: minimal_render_override.py/main/lightmap.c still gates renderTimeOfDayBackdrop/renderSky/renderClouds/stars under !fh_render_budget_strip (compile-timefalse). Engine5 skyRenderTimeOfDayBackdrop currently uses CPU blendTextures and mutable scratch images: replace with resident source textures, GPU blending and persistent quad, preserve time/phase/color/visibility semantics. Engine9 renderClouds uses camera-relative cloud objects plus sun-occlusion depth-only pass and glare quad; consume prepared native actors and explicit native material policies. Do not simply re-enable original GX/glare/minimal paths. Current code has NOT implemented sky restoration. Callback/morph/shadow/secondary-world/particles/lighting parity and full normal opening->Fox gameplay, meaningful gameplay save/reload, stability, physical60/min30New/~20Old remain incomplete. Continue autonomously; no hardware absence blocker.

9 October LOCAL native cockpit movie restoration. Ordinary6ac3c02625b0 / CIA53e8a163cfd5; replay8376bb274d77 / CIA9428168d23d9 / sourcece3f13193446; gameff2d76c2b03e. Unified Old80/New124. Not deployed; hardware unavailable. Goal ACTIVE/INCOMPLETE.

AttractMovie_DrawTextureCallback is now a native semantic material override: only original materialId1 while movie state2 borrows the current prepared frame. Other ops retain normal materials, live overrides reset per object. No original GX callback execution, old texture resolver, per-frame geometry building or extra shader instructions. Screen-body geometry previously omitted with that callback is restored. Four bytes per material hold the transient image pointer. Prepared ARM THP decode writes directly into three C3D ETC1 images (96KiB half-resolution;384KiB cap full), commits only after decompression/CRC, flushes exact written range, and binds existing actor geometry/UV through fixed replace-texture stages. Legacy cache notification bypassed for these committed images. Mixed prepared/JPEG streams reject before realloc could touch GPU-owned storage; host desktop JPEG path unchanged. Current shipped asset remains complete PAL movie, half256x192 padded256x256.

Pinned Melee c2ebe7c769b3 texture_barrier reviewed: CPU worker completion alone cannot retire GPU reads. Native movie uses the existing host begin/prepare_write retirement before overwrites or resizes, and shutdown's completed GPU drain before deletion. Game movie decoder runs under existing cooperative scheduler on the host owner thread; it is not an SDK worker doing GPU operations. Keep those ownership rules if moving decode later. Existing prepared ring semantics, decode/audio timing and game progression are preserved. No recurring allocation or frame-copy upload. Memory checks retain15% linear reserve. Extra mandatory CPU work is prepared LZ4/CRC plus exact cache flush, already performed at decode frequency; future optimization must preserve verified publication.

Tests: actual owner3000 reuse cycles under ASan/UBSan, unpublished-frame rejection, resize retirement, reserve checks; actual adapted native decoder tests raw/zlib/LZ4 payloads, checksum/malformed input, seven successful direct commits and mixed JPEG rejection. Original desktop prepared/JPEG test still passes. Post-material12288 state regression passes with new live-field reset. tools/test_pica_movie.py, test_pica_movie_decode.py, test_pica_post_material.py. native_movie_gate rejects absent/zero-draw/copying movie paths; both runtime logs pass. SDK stubs do not establish physical GPU correctness.

Native-cockpit-movie-new ordinary run and native-cockpit-movie-old exactCIA-content replay PASS, images390 inspected against earlier native-title-hud-new blank screen: Arwing video and physical screen now visible; image660 also shows handheld screen with Fox. New447commits/1124draws, Old393/1038;96KiB peak,0same-frame overwrite drains,0extra copy bytes. Old replay20rows/status1, max inputlateness0.75337ticks; normal saves unchanged, shutdown/log drain clean, owned emulators terminal. Runners run-native-movie-new/old.py require real movie counters. movie-results.json / movie-coverage.json hold evidence. No complete rendering/progression or physical timing claim.

Same Old tape/save/assets/render-frame windows versus prior430293: closeup391..511 submission1.2674->1.6130ms (+0.3456); cockpit628..808 1.3505->1.4671 (+0.1166); profile1048..1079 1.4862->1.6029. Cadences16.6946/16.7104/16.5907ms; Azahar only. GPU busy~0.302-0.304ms emulated, not a hardware estimate. Baseline lacks movie geometry. Old normalpeak40495312->40397824/50331648, linearfree5653760->5555456/10485760, process67301376->67305472/83886080, activeVRAMfree4858880, stack72788. Normal reserve including fullstack18.695%. New normal45903384/71303168, linearfree20235520/25165824, process102916096/130023424, normalreserve34.887%. Storage moved from normal to linear, not additional96KiB total memory; process rises4KiB. Native image poolpeak1874272 unchanged, movie separate98304. Shared native actor/HUD/hologram shader255/512 unchanged (build metadata also reports an older unused486instruction container; do not confuse that with bound native scene shader).

Opening progression investigation is continuing through a longer real touch-recorded new-profile sequence. Existing older recording stopped on Play Game panel; new opening-record-input adds A2220, then neutral until STOP4500/Select4600. Check its actual result and images before claiming opening coverage. Remaining native normal-disk/caustic/projected effects, secondary world/shadows/morphs/particles, contextual A/lock and opaque fuel/sparkle visual coverage remain incomplete. Full opening->Fox/save reload/final stability and physical New60/min30/Old~20 remain unmet. No hardware access is not a blocker.

# PICA renderer replacement

## Actor post-material states

The native path recognizes three original post-material policies without calling the GX callbacks. Shield/vortex additive rendering and fuel-cell/shop-sparkle opaque-or-additive rendering use persistent actor geometry, normal live pose/material data, and direct PICA depth/blend state. The original object alpha selects the latter policy, independently of texture or shader alpha. Both override depth writes and alpha rejection after material selection. Ordinary material updates clear the override; unsupported callbacks remain explicit omissions.

This adds four CPU bytes per resident material and three counters, no shader instructions or GPU buffers. Original/native raster comparison covers12,288 cases. A normal R-trigger cave replay restores four visible blue shield segments absent from the prior native build. Its Old-profile Azahar submission cost is approximately0.112ms/frame and peak normal RAM rises848bytes; linear/process/image allocations are unchanged. Physical cost and the fuel/sparkle-specific visual cases remain unvalidated. See `post-material-results.json` and `post-material-test/result.json` in `artifacts/research/pica-scene-rewrite`.

## Native title and profile text

The title/profile and name-entry callbacks now emit native HUD glyph batches directly. They use the existing resident font pages, persistent quad buffer and 29-instruction HUD entry. PICA applies the live title view and orthographic projection, including the original signed-short coordinates and depth. The CPU retains original text layout, selection/scroll state, clipping, color and projection-state restoration; it no longer composes a matrix per glyph or calls the retired quad renderer. No new shader instructions, buffers or texture allocations were added. Resource ownership follows the existing native font load/retirement boundaries; the pinned Melee texture barrier was consulted, without adopting its whole-heap frame flush.

`tools/test_pica_title_hud.py` compares 8,192 original/native glyph emissions under ASan/UBSan. Profile text and name-entry character input have inspected emulator captures. An Old80 replay shows native submission 1.259 to 1.486 ms over the same 31-frame profile window; UI-draw scope 1.583 to 1.803 ms; cadence remains 16.790 ms. These are short Azahar measurements, not console timing. Peak normal/linear/process memory is unchanged. Evidence: `artifacts/research/pica-scene-rewrite/title-hud-results.json` and `artifacts/native/native-title-name-entry-full-new/coverage.json`. Full renderer parity and physical targets remain unmet.

## Vertex/geometry processor arrangement

The replacement attaches only `shaderProgramSetVsh`; no geometry shader is
attached. The actor/world entry has180stored instructions,
95float uniform registers and13booleans. The HUD entry has29instructions,
81float registers and no booleans; the hologram entry has46instructions and82float
registers. All three share one255instruction DVLP so switching
entries does not require uploading shader code again. Uniform register ranges
are reused between entries, not added together. libctru's no-GSH configuration writes
`GPUREG_GEOSTAGE_CONFIG` mode0 and `GPUREG_VSH_COM_MODE`0, leaving the fourth
processor available for vertex work. Indexed-triangle bit8 writes by
`C3D_DrawElements` are not geometry-shader enable bit1. This is a configuration
claim, not proof of four-unit occupancy or throughput.

CPU game simulation/animation produces the live camera, pose and material
parameters. Prepared indexed world/character vertices stay in linear RAM.
PICA vertex processing performs transforms, two-bone skinning, UV transforms,
object diffuse lighting and the normal quaternion for fixed-function fragment
lighting. Fragment hardware samples/combines textures, adds diffuse environment
lighting, blends and depth/alpha-tests into the400x240top-screen target. No
programmable fragment shader or geometry amplification is used. Orthographic
font text, loaded image widgets, solid rectangles and projected menu text now use
a native batched HUD path. The pause hologram uses persistent native actor geometry
with GPU projected color and normal-derived opacity. Full HUD,
specular and effect parity remains missing. The deployed Old build predates
this local lighting stage.

References checked9October: 3dbrew GPU/Primitive_Engine and devkitPro libctru
`libctru/source/gpu/shaderProgram.c` initialization/configuration. Native source
of record is `platform_pica_scene.c` and `pica_scene_actor.v.pica`. Citro3D's
GSH-named bookkeeping and `GPU_GEOMETRY_PRIM` indexed-draw enum do not imply an
attached geometry program.


## Native font HUD, 9 October (offline implementation)

`platform_pica_hud.c` owns persistent quad vertices and bounded resident font
pages. `pica_hud_override.py` attaches page preparation after the original
font loader finishes writing its images; replacement retires GPU readers before
release. Original layout, reveal, color and text state remain live. Native glyph
draws do not decode texture descriptors or reconstruct GX stages. The native
image pool is shared with scene materials and keeps its existing limits.

Each draw consumes up to24 glyph/rectangle records (rectangle, UV, RGBA) in
uniforms. The1728byte vertex buffer is immutable after initialization; the
1152byte CPU parameter buffer holds only live records. PICA transforms corners,
selects UVs and emits color. Batching preserves order and splits at page, matrix,
scissor, alpha-mode, capacity and scene/present boundaries. There is no legacy
texture resolver, transient vertex construction, CPU clipping loop or prepared
GX draw queue in this path. CPU retains game text layout and bounded parameter
packing, which supplies live user-visible state rather than rebuilding geometry.

Pinned Melee's shared-DVLP arrangement and Citro3D `base.c:C3D_BindProgram`
inform actor/HUD code sharing. The existing audited linker independently checks
relocated operations and unchanged entry tables. Existing owner fences and
explicit cache flushes remain in charge; no full-heap flush or readback added.
HUD sampling scale is derived from each source rectangle, even when identical
GPU bytes are deduplicated with another page.

First separate-program candidate `c5c21ae618f4` / CIA `02f6221dfbd2` passed
Old80 palace/cave CIA-content replays. Captures1290 inspected: cave now shows
Staff/No Map Data/inventory text; geometry and shaded actors remain. Native
submission3.147/4.666ms versus3.106/4.566ms without HUD; about59.831AzaharFPS.
Normal reserve17.90/18.03%, linear59.70/58.97%, process17.20%. Native texture
peaks939424/1204896bytes; zero resident image bytes at shutdown. Evidence:
`hud-font-results.json`. This is not console performance validation.

Final shared-program candidate `0b57ddd77341` / CIA `5164d45d8f2b` /
source `cb3cd390b47e` passed exact-CIA Old80 cave and New124 palace checks.
Both captures1290 inspected; saves unchanged, logs drained and owned processes
exited. Cave submission4.661ms/text0.438ms versus pre-HUD4.566/0.422ms:
about0.110ms added across both exclusive buckets, not just submission.
Cave59.834 and Newpalace59.832 are emulated submission cadences only.
Oldcave normal/linear/process reserves18.03/58.97/17.20%; Newpalace
31.75/83.00/19.16%. VRAMfree4858880bytes, mainstack72812of524288.
Native image peaks1204896/939424bytes; all pages/images released at shutdown.
Evidence `hud-shared-results.json`, reproduced with `run-hud-shared-old.py`
and `run-hud-shared-new.py` (off --pacing1 --case cave/palace respectively;
CLI spelling is `--pacing 1`). No physical performance claim or deployment.

Actual assembled standalone and linked HUD entries
pass1152 numeric shader cases each (maximum error1.49e-8); full outputs and
instruction/uniform capacity checks pass. `test_pica_hud.py` compiles the actual
batching C against an observed SDK boundary and verifies ordering, capacity,
page/matrix/scissor/alpha changes, retirement and page-slot reuse. This test is
not GPU execution. HUD icons, custom projected menu text, other image widgets,
essential effects, specular and morphs remain missing. No complete HUD or
playability claim, and no new deployment while hardware is unavailable.

## Geometry-stage candidates (investigation, not implemented or measured)

The pinned Melee renderer already has a concrete point-to-quad geometry shader:
`reference/melee-3ds/native-3ds/port/3ds/point.g.pica`, attached by
`shaderProgramSetGsh` in `renderer.c`. It accepts transformed position, color
and UV, clips unsuitable points and emits four corners/two triangles. This is
a relevant starting point for native sprite/particle expansion, not evidence
of its cost in Foxhollow. Batch compatible effects and enable this stage only
for their draws; keep normal world/actor draws vertex-only.

Candidates include sparks, rain/snow billboards and simple glow sprites where
GPU expansion could avoid CPU-built quads and redundant uploads. Compare against
persistent indexed quads with vertex-shader expansion, not only a CPU-heavy
baseline. Preserve simulation and effect ordering; measure command/state-switch
cost, vertex work, pixel overdraw and memory. A geometry stage uses one of the
four vertex processors, so it is not a free extra processor.

Fur shell amplification is possible in principle but remains unproven for this
backend. It could reduce repeated setup/skinning only if the supported shader
dataflow permits useful reuse; generated geometry still incurs emission,
rasterization, texture sampling and overlapping fragments. Start any fur
comparison with native textured base shading plus sparse prepared silhouette
tufts and screen-size LOD, preserving character readability. A few shells are
an experiment, not a promise of performance-neutral fur. Missing HUD state,
material transparency and lighting correctness are separate problems that a
geometry shader does not automatically solve.

No geometry stage has been added by this investigation. Hardware unavailable;
neither effect cost nor a console speedup has been established.

## Persistent pipeline and light-slot correction, 9 October (offline only)

Local replay `374ef9149d48`, CIA `881cc26e5fa5`, source `81fc1a8806ad`
keeps the native pipeline bound across actors. World and actor entry points
publish their own camera/layout; the GPU-buffer owner still invalidates the
whole pipeline after retirement. World entry explicitly disables fragment
lighting after an actor. Draw ordering, poses and material state are preserved.

A source and linked-ARM disassembly check found that `C3D_LightEnvBind` only
changes the context's selected pointer/enable flag. It does not dirty all light
registers when switching between clean environments. The earlier three-slot
implementation could therefore retain the previous slot's lighting. The native
backend now uses ONE SDK environment and three small game snapshots. Switching
slots explicitly republishes all light parameters, including disabled lights;
unchanged repeated binds do no republishing. This uses 1,920 static bytes,
down from 4,020. It is a correctness repair as well as simpler ownership.
No reference/SDK code, cache-coherence rule or GPU fence was changed.

`tools/test_pica_environment.py` exercises 300 slot switches, live updates,
repeated binds and lighting disable/re-enable at the SDK-call boundary.
Linked SDK disassembly is retained in `environment-slot-test/sdk-lightenv-bind.txt`.
This test does not execute PICA. Both Old80 CIA headless `pica-native-pipeline-old`
replays also pass, captures1290 inspected, geometry counts unchanged, saves
unchanged, logs drained, owned processes exited. Palace/cave submission is
3.585/5.146 ms versus 3.614/5.283 ms before this change; cadence is still about
59.8 FPS. No substantial new FPS improvement or hardware prediction is claimed.
Memory headroom remains unchanged at allocator granularity. Cave whole-run
pipeline publication occurs970 times for37,301 actor calls, instead of resetting
after every actor. Evidence: `native-pipeline-results.json`.

The current renderer still omits HUD, callbacks, morphs, specular and essential
effects. Physical targets and complete visual/playable acceptance remain unmet.


## Native environment lighting, 9 October (offline only)

Hardware is unavailable while the user is away. Continue implementation and
bounded unattended checks; do not request replays or infer console FPS from Azahar.
The deployed Old build remains `1a433cbe6d29` (source `e47d4f0443e4`). Its physical
palace/cave results are 20.002/18.081 FPS with incomplete rendering.

Local replay `34414ac64142`, CIA `dc54c470b9a9`, source `f4b60c63c524` restores
diffuse actor lighting. `pica_environment_override.py` snapshots the original
mode-0 light-map slots at their original update boundary, including slot/group
selection, ambient and selected directional lights. `platform_pica_environment.c`
owns three bounded Citro3D lighting environments. This replaces three rendered
and copied spherical textures with an analytic hemisphere evaluated by the
vertex shader and PICA fragment-lighting hardware. Point/directional object
lighting is evaluated by the vertex shader. There is no CPU vertex-lighting
loop, texture readback, extra pass, transient geometry or additional draw.

This is an approximation of the original 32x32 diffuse-map raster/filtering,
not pixel-equivalent lighting. Specular maps, spot/distance attenuation in
some light configurations, full layered-material semantics, morph rendering,
callbacks, HUD and essential effect parity are still incomplete. The palace
and cave contain no logged unsupported light attenuation. Do not extrapolate
that coverage to other areas. The first point-only prototype was rejected
because Krystal was black; this candidate's inspected images show both Krystal
and Fox shaded and retain the world geometry and cave's left tunnel.

Materials compile four plain/lit combiner stages at load. Binding selects the
compiled stages and live intensity; it does not reconstruct the expression.
Light snapshots update only when changed. Three environments use 4,020 bytes
of static CPU state; no new GPU image allocation is used for the light maps.
Melee reference: `native-3ds/port/3ds/lighting_uniforms.h` boolean-light ABI and
`command_cache.c` publication rules. Citro3D `light.c`, `lightenv.c`, `texenv.c`
provide the actual native register encoding. Existing fences, explicit cache
flushes, immutable image lifetimes and command-segment publication are retained.

Old80 exact-CIA headless runs `*-pica-environment-compiled-old` completed with
unchanged saves, drained logs and owned emulator exits. `hid-1290.png` was
inspected in each. Results in
`artifacts/research/pica-scene-rewrite/environment-lighting-results.json`:

| Scene | Azahar FPS | Native submission ms | Change from unlit/pacing ms | Draws / indices per frame |
| --- | ---: | ---: | ---: | ---: |
| Palace | 59.833 | 3.788 | +0.296 | 197 / 56,970 |
| Cave | 59.838 | 5.611 | +0.390 | 279 / 72,468 |

Normal reserves including the full main stack are 17.90/18.03%; linear
60.66/59.62%; process 17.22%. Textures peak at 841,120/1,106,592 bytes, materials
at 97,784/167,444 bytes, below their 2 MiB/512 KiB Old limits. The shader has
180/512 instructions, 95/96 float registers, 13/16 booleans and complete,
single-write outputs. Its assembled-code oracle passes 216 compact/float
lighting cases and 1,000 hemisphere-normal cases (maximum normal error
0.000346). These checks do not validate PICA hardware throughput or full
fragment/material fidelity. The near-60 emulator cadence is not a console
performance claim, and this renderer is not yet playable parity.

The physical cave's 12.145 ms submission still exceeds the 8 ms Old budget.
Do not treat these offline checks as closing that hardware budget.

## Actor publication plan, 9 October (offline only)

Local replay `8f25eb39f7e4`, CIA `e3c3cdd1a917`, source `76593379ebc6`
compiles inter-draw bone-slot changes at load and shares camera, projection,
scissor and vertex-layout publication across an actor's material draws.
Every actor starts with a full publication of its live pose. Command-buffer
retirement also forces all rows and state to be republished. No pose survives
across actors or frames; no live matrix comparison or pointer-identity cache
is used. The prepared plan adds four CPU bytes per draw, no GPU storage.
Simulation, animation and original CPU buffers for collision/morph consumers
are unchanged. Draws and their ordering remain unchanged.

`test_pica_model_plan.py` checks all 835 models, 1,880 groups, 586,623 indices,
7,515 rejected malformed cases and 4,000 numeric pose cases. It interprets the
compiled publication masks against original bone IDs, including forced
invalidation every third group: all 8,435 bone slots match. Across the pack,
7,238 instead of 8,435 bone publications are needed without extra invalidations.
This is a static workload reduction, not a hardware FPS estimate.

The two `*-pica-actor-publication-old` CIA headless runs pass; captures1290
were inspected, draw/index counts match the preceding lighting candidate,
saves are unchanged, all log queues drained and owned emulators exited.
Palace submission drops 3.788 to 3.614 ms; cave 5.611 to 5.283 ms. Cadence
remains about 59.8 FPS under Azahar. The whole cave run writes 783,624 of
978,225 requested bone rows and publishes shared state 37,301 times for
97,386 actor draws. These counters include startup/unmeasured phases and
must not be presented as measured-window counts. The runs did not force a
mid-actor command-buffer restart; that branch has the static ownership check,
not a new hardware validation.

Normal reserves are 17.90/18.03%, linear60.66/59.62%, process17.22%; texture,
material, shader and stack budgets are unchanged within allocator alignment.
Evidence: `actor-publication-results.json` and
`palette-publication-plan-test/summary.json` under the same research directory.
Neither new build is deployed. Full lighting/HUD/effect parity, normal opening
progression and sustained physical performance remain unmet.

User mandate (9 October): replace the rendering architecture; do not rebuild the CPU-heavy path under new names. Targets: 60 FPS New (steady 30 minimum), about 20 FPS Old. Neither is established for complete rendering.

## Baseline and prototype

Preserved deployed replay bc811b261bf0/CIAf41f58a3b577 in artifacts/research/pica-scene-rewrite/baseline and its original private deployment archive. Physical Old80 palace evidence: console-test-app-results/20261008-194953, 9.905 FPS, 100.958 ms/frame, 51.120 ms native submission. This is world textures plus HUD, not parity.

The new platform_pica_scene.c owns its shader and command submission. Game world traversal hands it a resident asset and camera/block transform. It consumes resident section descriptors, joins contiguous geometry in source order and lets PICA transform/color vertices. It does not invoke the old material resolver, prepared-world queue, per-section byte validation, CPU index gathering, or old submit_prepared_world. The first shader contains 10 instructions. The current section plan comes from the validated asset loader; its descriptor compilation will move into the new native asset format as live materials return.

This initial geometry-only experiment intentionally omits textures, characters, HUD, lighting, transparency, effects, and fine visibility rejection. It draws the main-stream resident sections of game-visible blocks. It is not a visual correctness substitute for the full game. Animation, audio, input and simulation are unchanged. Dynamic world positions retain their existing producer update. Dynamic visibility/material selection is still outstanding.

The existing frame target, retirement fences, cache publication and resident asset lifetime code are reused as infrastructure. The initial host still allocates some unused prior renderer resources; remove those after the integrated correctness baseline. Never infer a memory saving until measured. Separate shader code must not interleave with old draw paths; this experiment suppresses old world/model/UI emissions. Own shader freed only after host GPU retirement.

## Initial budgets (requirements, not measurements)

| CPU wall-work component | New ms | Old ms |
| --- | ---: | ---: |
| Simulation, animation and audio | 8 | 30 |
| Scene selection and live parameter collection | 1 | 3 |
| PICA preparation and submission | 3 | 8 |
| OS, input and steady-state I/O | 1 | 3 |
| Unallocated frame headroom | 3.67 | 6 |

GPU work overlaps CPU: target <=14 ms New and <=40 ms Old, not added to those CPU totals. Treat all timing buckets as wall time including scheduling, not CPU utilization. A feature that breaks budget requires redesign before adding another. Stable30New remains minimum completion, not permission to abandon60target. No steady-state multi-second stalls.

Keep >=15% reserve in each tested pool and total process grant, including committed code/stacks. Validate Old80/New124 kernel grants; record normal, linear, VRAM, main/worker stack, vertex/index/texture and command-buffer consumption at each stage. GPU storage stays bounded; no borrowed buffer is overwritten/freed on timeout. Record SD reads/writes and timing separately.

## Melee adaptations

Pinned melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e: command_cache.c range cleaning before every internal/final command publication, explicit GPU fences before resource recycling, and shader output initialization. We retain these rules; we do not reuse its game-specific render interpreter. Citro3D's fixed SDK draw emission is permitted. The dedicated shader writes every declared output; assembled instruction/uniform capacity and output checks run during build. Raw depth-readback regression remains unchanged.

## Gates and next stages

Run the same saved palace recipe through the new geometry route on Old/New headless CIA content. Inspect images, draw coverage, memory and timing. No console speed claims from Azahar. Then precompile native material/texture bindings and add texture sampling without reconnecting generic GX preparation. Follow with GPU-skinned characters, native HUD, lighting and essential alpha/effects. Maintain explicit omissions and matching workload comparisons throughout. Clean replay exit alone does not prove rendering correctness.

9 October integrated prototype f28db1851455: Old/New CIA content headless replays completed; captures inspected, no coverage failures, saves unchanged, processes exited. New58.989FPS,16.952ms/frame,0.997ms native submission; Old59.831FPS,16.714ms/frame,0.996ms submission. These are Azahar timing only, not forecasts. 135 draws and33585indices per measured frame, vs earlier world-textured rendering with different selection/work. Geometry-only omissions mean this is not an apples-to-apples visual speedup. Physical measurement remains outstanding. New process105500672/130023424, normal55565344/71303168, linearfree10924416/25165824, VRAMfree4858880, stack43680/524288; Old process69849088/83886080, normal40715984/50331648, linearfree6735488/10485760, sameVRAMfree, stack43828/524288.

Subsequent resident-plan implementation eliminates the old instruction-plan dependency entirely: pica_world_plan.c compiles compact native descriptors once at asset load from validated prepared groups. Source-order indices/material IDs are preserved, no runtime merge/interpretation. Same allocation owner/fence as resident asset; all accounting included in scene CPU plan budget. Differential host test covers1259nonempty world packets,10912draws,1775196indices;1259undersized-output cases rejected;8empty main streams identified. Initial geometric prototype evidence must not be claimed as final resident-plan runtime evidence until its replay finishes.

9 October replacement replay DEPLOYED to Old.229:5000, transfer artifacts/console-test-apps/20261008-201145 COMPLETE:5files replaced/readbackverified,UNIFIED_BUILDS unchangedverified,8fixtures verified,normalGCI+backup identicalbefore/after. 3dsx5aeb1aef3727 / CIA314a16e6663f, source0f4ca55d54a5. Remote /3ds/foxhollow-replay/foxhollow-replay.cia and .3dsx. User instructed FBIinstallCIA, HOMEFoxhollowReplay, palace-facing / PICA GEOMETRY / MAIN / VRAMOFF / GPUprofilingOFF / AautoexitthenFTP. Expected no textures/models/HUD. OldWORLD TEXTURES comparison remains available; no need rerunbaseline unlessmatchingquestion. No normalassets/game changes. New.223 NOTupdated.
Both final exactCIAcontent tests palace-vram-off-main-pica-scene-resident-old/new completed, imagesinspected,no oldrenderer draws/coverage failures,savesunchanged, >15%pool/processreserve. Timing/memory records artifacts/research/pica-scene-rewrite/resident-{old,new}-*.json. Geometry-only timing is not full-render/physicalspeedup. ALL build,emulator,FTP sessions terminal. No live tools. Goal active/incomplete. Await hardware evidence but continue implementing native material/texture stage; do not restore oldrenderer. See docs/PICA_RENDERER_REPLACEMENT.md for budgets/architecture. This supersedes deploymentpending notes.


9 October latest Old console collection completed (read-only) at artifacts/console-test-app-results/20261008-202239 after one reset/retry. New run palace-facing-000000015b466084-0 uses current source0f4ca55d54a5/build5aeb BUT selected legacy WORLD TEXTURES: result pica_scene_geometry=false, world_textures=true; native.log confirms old world/UI draws. Do NOT attribute to replacement renderer. 51 measured frames/5.129688s=9.942126FPS,100.582ms/frame,p95106.517,max109.610; no >250ms measured observations; clean completion, normal saves unchanged. Exclusive native_submit50.552ms,object_update8.743,VBlank8.609,scene_render6.626,object_render5.215,world4.808,visibility4.689,source4.328,texture_resolve2.290. GPU pending6.949ms overlaps CPU, not hardware utilization. Actual80MiB grant, process69849088, normalpeak40720296/50331648,linearminfree5380992/10485760,VRAMfree4858880,measured mainstack75572/524288. Analysis physical-old-legacy-comparison-{timing,memory}.json under pica-scene-rewrite. Physical replacement performance is STILL UNMEASURED. For hardware comparison use existing installed CIA: palace-facing / L until PICA GEOMETRY / MAIN / VRAM OFF / GPU profiling OFF / A. No reinstall needed. Mode7 deliberately lacks textures/models/HUD. No new deployment or remote changes this collection.
Local native material/texture stage is UNBUILT, unfinished: platform_pica_material.c and fh_pica_scene.h/native_scene edits. Do not deploy or claim tested; fh_pica_scene_textured still needs runtime definition/integration, shader/load hooks/harness gating outstanding. Existing preserved5aeb package unaffected. Goal active/incomplete; next implementation native load-time material binding, not legacy resolver.

## Native material stage (local candidate, 9 October)

`platform_pica_material.c` owns bounded native texture allocations and compiled
material descriptors. `pica_material_override.py` prepares only materials used
by the resident world plan, after the game's block texture loading resolves
its pointers. Prepared image identity is verified at load; retained handles
never depend on source pointer reuse. Animated frame chains are uploaded at
load. Draws select a retained frame and update small tint/UV constants. Sampler
wrap/filter and UV scale are compiled once into immutable frame descriptors.
No legacy texture resolver, per-draw source comparison or per-frame image copy.

CPU live work still collects ambient tint, scroll and animation selection once
per material/block. The game owns their producers. This remains necessary
until parameter publication can be attached directly to those producers; it
must be measured as part of scene preparation. No simulation changes.

Melee adaptation: explicitly flush native texture storage before GPU visibility;
retain it until scene ownership is retired by the existing GPU fence; fully
initialize shader outputs. Unlike Melee's general snapshot route, this stage
owns persistent immutable images and does not copy source snapshots every draw.
No asynchronous VRAM texture promotion is introduced. The new dedicated shader
uses 20/512 instructions and 11/96 float registers; geometry-only keeps its
own 10-instruction shader. The two never interleave within a running tier.

Bounds: 512 image slots; 512 KiB material heap; 2 MiB Old/4 MiB New image cap,
plus 15% normal/linear reserve checks. Material storage is released with its
scene plan after GPU retirement. Valid empty main streams now have an empty
plan rather than a false missing-draw error. Unhandled material types fail
coverage; they are not silently interpreted by the old backend.

Initial f8743e92e115 Old/New palace tests passed, captures inspected. Mode8
PICA TEXTURES shows textured palace walls/floors/columns but NO characters,
HUD, fog/lighting parity, deferred water/transparency or essential effects.
Old Azahar 58.8429 FPS, 16.9944 ms/frame, native submit1.8417ms, 134 draws and
33549 indices/frame; New58.6346FPS. This is emulator evidence only. The changed
visible scope differs from both geometry-only and old textured/HUD baseline.
No claim of physical 60 FPS or completed rendering. Initial image peak159776
bytes, material heap30928 bytes; no material errors/leaks. Revised immutable
samplers are under final validation; previous timings do not validate revisions.

Cave expansion check on2a89fe547737 failed before measurement with PICA scene
missing material, despite palace Old/New passing. Do not package/deploy the
textured candidate as validated yet. Archive cave-vram-off-main-pica-textures-resident-old
retains the failure; owned process exited. A bounded diagnostic adds asset,
material, slot/frame attribution. No console changes.

Architecture audit: current fh_native_scene_prepare_world_positions still calls
fh_native_scene_source each visible block/frame (0.225ms in initial Old-profile
Azahar palace). This is temporary compatibility, NOT the intended final route.
Pinned source search identifies world-position producers GroundAnima.c and
XYZAnimator.c; both already publish their modified block positions via DCStoreRange.
Next remove repeated source comparisons using explicit producer dirtiness plus
load-time source attachment, preserving their CPU simulation/collision updates
and existing GPU retirement before genuine geometry changes. Do not simply
assume unchanged pointer means unchanged vertices. Shader compilation/loading
and material image resolution are already outside draw submission.

Cave failure diagnosed by c95843201435: unprepared 256x256 CMPR images expanded
to131072byte RGB5A1 allocations. Separate materials requested duplicates;
unprepared images were not deduplicated. Pool reached2093088/2097152bytes,
rejecting subsequent images despite ample physical linear reserve. Current
fix converts unprepared images into bounded CPU staging at LOAD and compares
canonical GPU bytes exactly against matching resident dimensions/format before
allocating. Shares refcounted images; no hash-only/pointer identity; no draw-time
comparison. Prepared images retain verified pack identity lookup. CPU staging
is included in512KiB material heap budget and15% normal reserve. No increased
image cap and no silently missing textures. Validation pending.

Cave fix4cd922fe5a6b passed the exact CIA-content Old-profile replay (cave-vram-off-main-pica-textures-dedup-old).
Native images peak479264bytes vs previous rejection at2MiB; 37 uploads,
56 load-time conversions, zero errors, all references released. Material CPU
peak147248 includes131072byte temporary conversion staging. Captured cave ladder,
floor, walls and left tunnel are textured; no models/HUD/effects by design.
Old Azahar58.529FPS, >=15% normal/linear/process reserve includingfullmainstack.
Native gate confirms no legacy world/UI/model submissions. Normal saves unchanged,
clean automatic exit; owned emulator exited. New-profile palace regression inprogress.
No physical performance claim, no remote files changed. Mode8 remains diagnostic.

9 October native PICA TEXTURES stage BUILT and DEPLOYED to Old.229:5000. Unified3dsx4cd922fe5a6b7499007468320fce895254af2b75ae61a1587e2971c5c55657f0 / CIAcd628c7ab0ed34c2e89717432619082fb652d374b504bb8bb46bf2b60033108a, source5f981d45d3fdbf41e29c9f30736e4cdb29776db94682c83704588374504fc073. Currentgamearchiveec43427decada05e2ed314cd89ffe8f94eb0ee78df76f9770be9c12cf0af6b24. Private package build/console-replay/foxhollow-console-replay-4cd922fe5a6b-private.zip SHAbf024e61ac69ec20eae49bb10bb8d589088a9ebbc57a064f5f8df1acef036098. Transfer artifacts/console-test-apps/20261008-204707 COMPLETE:5files installed/readback,8fixturesverified,savesGCI+backup identicalbeforeafter. Documentation-only followup updated remote README atomically/readbackverified; documentation-transfer.json records hashes. Remote /3ds/foxhollow-replay/foxhollow-replay.cia and .3dsx. FBIinstallCIA needed to updateHOMEicon (not remotely installed). New.223 unchanged. Old80/New124 onepair. No ordinarygame/assets/save changes.
Replacement material architecture: platform_pica_material.c, pica_material_override.py, pica_scene_texture.v.pica. Native images/TEV/samplers compiled at load, live tint/scroll/frame selectors, GPU UV transform. No legacytexture resolver/drawsourcecompare, oldworld/UI/model draws remainzero. Mode8 PICA TEXTURES followsmode7 PICA GEOMETRY withL. Shader20instructions,11floats,completeoutputs. Explicitly nocharacters/HUD/fog/deferredworld/effectparity. Fullplayability NOT achieved. Imagebudget2MiBOld/4MiBNew,CPUmaterial512KiB and15%normal/linearreserve. Cavecaught absent dedupforunpreparedCMPR exhausting2MiB; fixed exactcanonicalnativebytecomparison atLOADbeforeallocation, neverperdraw. Cavepeak479264GPU,147248CPUincltemporary131072staging;37uploads/56conversions,zeroerrors/leaks. Palace159776GPU/24432CPU,42uploads,zeroerrors/leaks. Oldtexturepackresidentcopy disabled innewbackend, Newlinearreserve improved. Geometryonly mode remains available.
Current exactCIA-content tests cave-vram-off-main-pica-textures-dedup-old and palace-vram-off-main-pica-textures-dedup-new PASS, captureshid1290inspected, nooldrenderer/missingmaterialdraws,savesunchanged,ownedemulators exited. Oldcave58.529FPS/17.0856ms/2.2053mssubmit;Newpalace59.632FPS (fulltimings in textures-dedup-*.json). EMULATOR ONLY, nophysicalreplacementresult yet. Oldpalace earlier2a89candidate58.983FPS/1.791mssubmit; initialf874Old/Newpalacealso passed. Hardware targetperf stillunproven. Differential worldplan1259packets10912draws1775196indices passes,1259capacityrejects;8empty main streams now represented byemptyplans.
Hardware instructions: afterFBIinstall4cd CIA, palace-facing, PICA GEOMETRY, MAIN,VRAMOFF,GPUprofilingOFF,Aautoexit; relaunchsamecase/settingsPICA TEXTURES,Aautoexit;ftpd. WORLD TEXTURES andFULL arelegacy comparison, doNOTselectforrewrite. Lastphysicalrun current5aebsource butlegacyWORLD TEXTURES9.94FPS, notnewbackend. Evidence20261008-202239 preserved.
NEXT required architectural work: remove remaining per-frame world-position source comparisons (0.688msOldcaveAzahar) via explicit producer dirtiness/load-time attachment. Pinned source writers GroundAnima.c and XYZAnimator.c both publishblockpositionsviaDCStoreRange; preserveCPUcollision/simulation and GPUretirement onactualchanges. Do NOT assumeunchangedpointerimmutable. Then nativeGPUskinnedcharacters andHUD withownpreparedmaterials; doNOTreconnect oldmodelresolver/preparedqueue. Keep measuringperfeaturebudget. Allbuild/emulator/FTP sessions terminal, no livehandles. Goal ACTIVE/incomplete. This supersedes earlier unbuilttexture/pendingdeploy notes.


9 October continuation: producer-driven world-position refresh built/tested locally; NOT DEPLOYED. Replay3dsx7242c887fc3356fd8e698d8fb32cb28331fad4fb04992480176693f4da8dc6a6, CIA5199d0b68559bbe24fc4f4568fa1a8760b68e9df578059928b75b6ff12163e90, sourceba1af77c043b68c82b5e6aa7b069f388e57cdf9ab0f203b76f0781be16e97bc9, gamearchive1ca9616a5588c9c1b16b8fb7e5d4783bc29916348a8892cfc387f6f15c64cbaa. Load-time attachment plus GroundAnima/XYZAnimator producer notifications replace repeated unchanged world-position comparisons. Includes GroundAnimator_free restoration, not just DCStoreRange writers. Exact removal of overlay calls/header restores pinned source; game writes unchanged. Dirty publications retain GPU join -> positions -> bounds -> flush -> snapshot; failed updates retain dirty. Twelve extra ARM bytes per world plan, no extra linear/VRAM. Host ASan/UBSan actual-production fixture verifies10000 clean reuses, coalesced notifications, failure retry and owner reset;1267preparedpackets/808656vertices/2534negative cases. Evidence producer-position-tests.
Headless exactCIA-content cave-vram-off-main-pica-position-producer-old PASS:59.2319FPS/16.8828ms versus prior58.5289/17.0856, not hardware forecast;156draws40524indices/frame both. Scene source0.68838->0.48024ms (remaining model checks; NOT all source comparisons removed), submission2.20616ms unchanged. Actual counters11attached/10notifications/6updates/7064reuses. Capture1290 inspected: cave ladder/floor/walls/left exit retained, not pixel-equivalent screenshot. Image peak479264/CPU147248, errors/leaks0; normal18.15%,linear62.84%,process16.69% reserve. Normal saves unchanged; shutdown clean; owned emulator exited. Record position-producer-cave-old.json. Models/HUD/effect parity still absent. Next architecture stage remains own native GPU-skinned model plans/materials, not old resolver.
Latest user reports Old FTP back after run. Read-only collection artifacts/console-test-app-results/20261008-210019 COMPLETE but ONLY the four previously archived runs exist. Fresh current/native.log bytes exactly match earlier palace-facing-000000015b466084-0; no new replay measurement. Remote manifest still confirms staged4cd922fe5a6b/CIAcd628c7ab0ed. Do NOT blame user or attribute9.94FPS to new test. Asked once asynchronously how far test got (autoexit/manual exit/ordinary game/crash); response pending. No remote writes. Old4cd remains deployed; New unchanged. All local build/emulator/FTP processes terminal. Goal ACTIVE, incomplete, not stalled.


## Native actor geometry stage in progress (9 October)

The replacement now has its own load-time actor draw compiler (`pica_model_plan.c`) and direct GPU submission (`fh_pica_scene_actor_draw`). Plans contain resident buffer offsets, ordered material IDs and bone lists; they do not execute the legacy GX interpreter or source-validation/model-refresh path. Plans share the existing scene owner and GPU retirement fence, use the same 256 KiB plan pool and normal-RAM reserve guard, and are released with that owner. Fixed bind-tail translation is applied to live matrix rows on the CPU; camera transformation and two-bone vertex skinning execute in the new shader. This avoids rebuilding view*joint matrices separately for every material. CPU work remaining here is uniform publication and the three-row bind-tail translation; its cost must be measured after the visual stage is valid.

One 47-instruction program serves both world and actors, with 78 float registers including constants and 18 free. It preserves all four complete output registers on every branch. There is no per-draw switch to a separate shader-code image. This follows pinned Melee complete-output and command publication/lifetime practice while replacing its game-specific rendering machinery.

This development mode (config 9, not offered in the hardware replay menu) currently uses the validated 64-byte float actor packets. Compact actor packets, actor textures, vertex deformation/morphs, callback materials, lighting, HUD and essential effects remain required. Simulation/animation still execute, but this intermediate rendering is explicitly incomplete. Do not deploy or call this playable. World geometry/textures remain the existing new backend.

Compiler differential check covers all 835 models / 1,847 draw groups / 586,623 indices in corrected private scene pack 73cb9f7530cf7d3c7a9ac28bf03ae3106687720ed551d62f4c425ad5f98c3707. No omitted draw ranges; 1,670 negative checks reject partial/invalid plans without publication. 4,000 random world-space matrix comparisons include nonuniform scale and shear (max error 0.000031412). See model-plan-test/summary.json.

Initial c906 actor replay completed with draw counters but was rejected on image review: Krystal was absent. A CPU projection oracle confirmed 2,885 vertices in the settled camera volume. The assembled shader also passed standalone Azahar-interpreter rigid and blended transform tests. Root cause was the integration's use of bit index 0 in C3D_BoolUnifSet; this API requires a location in 0x68..0x77. Querying and checking the exported actor uniform location fixes the invalid ABI call. The corrected ea4ec174 geometry replay shows Krystal; see actor-development.json and the later material-stage record. No hardware claim. The rejection record is actor-geometry-rejection.json. A separate software vertex-trace emulator was built while diagnosing; baseline emulator/source restored. Its timings are explicitly invalid for performance comparisons.


9 October latest continuation: native actor base-texture stage tested LOCALLY; NOT DEPLOYED. Current shared replay3dsx5fa0014ca1d5a535eaba0c186c7ca2dc4b546b785c6ea35c56e388426e13a15e, CIA6d8d70fa998175189829f857cd968609fda7189777283cc75290f5b923bca7d5, source86ecef874a07719ac1ad3903151d9fcfc985bba713460c0777a3e8ced254306c. Gamearchive44bbc9c05c50c956e1a5fd7ef94539a4f8665e3e68694c4c47473eae93d97290. Build command tools/build_native.py --replay-console (GX dispatcher/shared worker build flags FALSE). Old80/New124 unified pair. Ordinary game and both consoles unchanged; Old still4cd922fe5a6b/CIAcd628c7ab0ed.
Architecture: own pica_model_plan.c validates/compiles all835 models/1847groups/586623indices once;1670negativecases,4000palette numeric cases. Own47instruction world+actor shader,78floatregisters,18free,completeoutputs, GPU two-bone skinning. CPU only live bind-tail matrix translation/publication, no source validation/model rebuilding. Current actors float64; compact load/prepared storage NEXT. Actor images/native TEV compiled at load, separate opacity images, bounded shared native image/material pools; live frame/scroll/alpha once per material. pica_actor_material_override.py plus pica_actor_override.py, shared FhPicaMaterialSet for world/actors, retire resources after existing owner GPU join. No old GX/TEV interpreter or legacy native draw path. Original game animation/morphs/simulation run; rendering of morphs/callbacks/layered lighting/HUD/effects still absent. Config9 PICA ACTOR BASE(dev) remains hidden from L cycle, NOT a public playable tier. Need expose deliberately before future hardware package; current deployed private scene lacks model465 correction, so actor test requires corrected private native scene73cb9f... below.
Fixed two integration errors: C3D_BoolUnifSet requires queried location0x68 (not bit0); fixedgeometryea4ec174 capture shows Krystal silhouette. Base-material5345c13fb034 completed but ALL BLACK, rejected visually: projection had only been published through GX dispatcher. New entry points now take Camera_GetProjectionMatrix() directly and use verified LCD rotation. Both final tests run with dispatcher OFF and render real images. Temporary CPU vertex oracle and admission logging removed. Replay menu now displays 12-character executable source tag. Gate accepts explicit actor-native declaration, still rejects old draws/coverage failures/leaks.
Tests: palace-vram-off-main-pica-actor-live-camera-old PASS57.880FPS/17.277ms, nativeSubmit3.4415ms; cave-vram-off-main-pica-actor-live-camera-old PASS56.937FPS/17.563ms, nativeSubmit5.3531ms. Captures1290 inspected: textured Krystal/palace and Fox/cave ladder/mushrooms/left tunnel. Emulator ONLY; different compile flags versus prior geometry stage, not isolated texture-cost comparison or hardware forecast. Source/model-refresh counters0. TextureGPUpeaks832928palace/1103520cave (<2MiBOld); CPUmaterials72860/153756inclstaging (<512KiB). Normalreserves17.96/18.09%,linear57.98/56.81%,process17.24%; VRAMfree4858880. Saves unchanged, log queues clean, owned emulators exited. Evidence actor-base-{palace,cave}-old.json and actor-development.json. All tool sessions terminal.
Known coverage audit: model801 loads without resident plan in palace (1shader,1list,8vertices). It is NOT proven non-rendering. Pack contains801 fromclouddungeon, so check source variant/size admission. Load helper logs missing plan and retains game resource; attempted native draw still explicit coverage failure. Neither replay attempts native draw of801. Do not silently claim complete asset coverage.
Latest user Old FTP run collected earlier at artifacts/console-test-app-results/20261008-210019; ONLY old4runs/currentnative.log exactlymatches prior15b466084 log. No new physical replacement measurement. Asked once async how far run got; no reply. Do not blame user or recycle9.94FPS as new data. No new remote writes. Keep working; goal ACTIVE/incomplete. Next compact PICA actor storage and layered/deformation/HUD restoration within budgets; hardware native baseline still needed, no need repeat long boots without a specific change.


9 October deployment preparation: candidate dd1771fc4f6f / CIA63ced419303b0c675bd84454a5b7e1b44322de6b818646c704d8f8f2cdf554e5, source6d5ed388699fe1f3d5096afb1bdef2ac68299134a0f89001080c8c9f61dada28. Game/renderer unchanged from5fa0014; replay-only visuals.txt selects native stage9, L now cycles7/8/9, menu shows source ID. Exact packaged Old80 palace test pica-actor-package-old PASS, image1290 inspected;57.637FPS emulator only, reservesnormal17.96%/linear57.98%/process17.24%. Private package build/console-replay/foxhollow-console-replay-dd1771fc4f6f-private.zip SHA3771b4b63088addfcd01c3eebe578f214e5da57ec3d5e85cdbfea4b30460b586 includes corrected73cb9f scene, original-world topology, existing texturepack and4isolatedcases. Private startup config preservesordinary render_disabled.txt.
Old.229 FTP transfer IN PROGRESS owned session70337, artifacts/console-test-apps/20261008-220421/transfer.json. Do not claim deployed until terminal success/readback. New.223 unchanged. Read-only preinstall collection220211 found ONLY SAME four archivedruns; no freshphysicalresult. User instructions aftersuccess: FBIinstall /3ds/foxhollow-replay/foxhollow-replay.cia; HOME menu source6d5ed388699f; palace-facing then cave-idle, PICA ACTOR BASE default,MAIN/VRAMOFF/profilingOFF,AautoexitthenFTP. No reboot. Diagnostic incompleteactorbase,notplayable.
Actor masks code is implemented but neither palace/cave logged nonzero masked materials; do NOT claim that branch visuallyvalidated. Model801 staticaudit actor801.json:onlyclouddungeon indexedsource896bytes/16verts exists andiscompiled; runtimepalaceheader801has8verts, suggestsdifferent/fallbacksourceidentity, causeNOT established. getTableFileEntry returns tablevalueeven0, loadModelsBin fallsback to loadedbank; mustauditactualruntimebytes/CRC beforefix. No gameplay changes or sourcealiases guessed.


9 October compact actor continuation: LOCAL ONLY, shared replay457d68ef1fd9 / CIA058c92410e683629639c6981fdee30f60cacef7607887a44cffa898fe2268a94, sourcef60af90849fc795f2111499f74c9943de0052fb36b3d7611a1ab52bf4f43461e. Prepared FHNSCN0A actor packets use40bytes instead64; original indices and all world packets unchanged. All835models/1847groups/191922vertices checked against source; maximumdecodederror5.96e-8, UV1 independently checked. Actor vertex/index storage13462668 ->8856668bytes across complete pack (NOT resident RAM saving). Private pack01c0fbc804c394b111e7be62844557055d20b7cbec57e25f3a2874f7a58b8cec under actor-compact-prepared. Reader rejects malformedcompactfields beforepublication;7515negativecases,4000posecases. Shader unchanged, direct40byteattribute loading and decode uniforms; no frame conversion/old modelrefresh. Legacy renderer disallowed from admitting A packets. Pure old converter reused ONLY as offline data conversion, not perframe path.
Old80 exactCIA-content palace/cave pica-actor-compact-old passed; captures1290 inspected withKrystal/Fox and world/leftcavetunnel retained. Emulator57.847/57.037FPS, no establishedperformancewin; linearreserve60.87/59.64% versus57.98/56.81% floatstage, normal17.96/18.09%,process17.24%. No old submissions/sourcechecks/materialerrors/leaks; save unchanged; owned emulators exited. Full details actor-compact-state.json. Masks remain visuallyunexercised; layeredlighting/HUD/morph/callback/effect parity absent. Next actualhardware baseline and further measured restoration; compact not deployed.
DEPLOYMENT IN PROGRESS remains immutable dd1771fc4f6f package (float actors), NOT current457d workspacebuild. Old.229 transfer20261008-220421/session70337 has staged binaries but is still pre-reading large existing native-scene after oneconnectionreset; active files not yet replaced. Do not claim success without transfer.json complete and finalreadbacks. New.223 unchanged. All other build/test sessions terminal. No freshphysicalrun from latest collection.


9 October OLD DEPLOYMENT COMPLETE: immutable dd1771fc4f6f5fe287789be2015d808cb4be3ad83a3a09d1d3573814599b6dbf / CIA63ced419303b0c675bd84454a5b7e1b44322de6b818646c704d8f8f2cdf554e5, menu source6d5ed388699f, uploaded toold229:5000. Transfer artifacts/console-test-apps/20261008-224009/transfer.json COMPLETE;7files activated after full staged-byte comparison, remote dated backups retained, finalnames/sizes checked; rename did not rewrite payload bytes. Eight casefiles verified; model-lod priortransaction byte verification+freshsize, texturepack freshlybyteverified unchanged. NormalGCI andbackup hashes equal before/after. Session81743 TERMINAL success; old70337/17534 andalllocalbuild/emulator sessions terminal. No livebackgroundwork.
User must FBIinstall SD:/3ds/foxhollow-replay/foxhollow-replay.cia to updateHOME app, verify menu6d5ed388699f. Run palace-facing then cave-idle, defaultPICA ACTOR BASE/MAIN/VRAMOFF/profilingOFF, A andallowautoexit;ftpdafter. No reboot. UnifiedOld80/New124 pair. Correctedfloatnative scene73cb9f uploaded. New.223 andordinarygame unchanged. Lastread-onlycollection220211 stillonlyfourhistoricalruns; NO NEW physical renderer/FPS result. Do NOT attribute old9.94FPSto newbackend. Newcandidate is incomplete native world+GPUskinnedactors/baseimages; no layeredlighting/morphs/callbacks/HUD/effect parity; notplayable.
Further LOCAL compact candidate457d68ef1fd9 passed palace/cave with~300KB morelinearfree andnoFPSclaim; newestworkspacec1f9ede099a7/source411be83f79ba onlyaddsload-time missing-source identitylog andpassedkrystal-eyes. Thesecompactbinaries/assets are NOT deployed. Staticcompactcoverage53bankscomplete; reader+plan/UV/indices checks andcaptures recorded inactor-compact-state.json. Missing801 identity is640/f4223cdd matchingfirstmodel0 across51banks; indexedclouddungeon801896bytevariant isdifferent. No guessedalias added. Goal remainsACTIVE/incomplete; next obtain firstnative hardwaremeasurement and continue measured rendering restoration.


## Supported actor second layers (local, 9 October)

9 October current checkpoint: read-only Old FTP collection 20261008-230025 still contains only four historical replays. Current log probe 20261008-230059 SHA ebc268e451cbe135b228c07c15d1244eace0a52d63655a926fbc21552e8bf3d9 exactly matches palace-facing-000000015b466084-0. No new physical replacement timing and no remote writes. Asked for replay-menu source ID to distinguish installed CIA/launch issue; do not repeatedly ask whether user ran it. Deployed dd1771fc4f6f (source6d5ed388699f) remains unchanged, CIA at SD:/3ds/foxhollow-replay/foxhollow-replay.cia requires FBI installation.

LOCAL candidate f0d4be1a1cfa, source c5d46ba15024, now restores supported dual-UV actor texture layers with load-compiled combiners and independent live frames/scroll. No extra draw pass, per-frame image conversion or geometry copy. Shader50instructions/78floatregs; compact40byte actor stream retained. Old80 exact-CIA palace/cave and Krystal-front tests completed; captures inspected, saves unchanged, owned processes exited. Palace57.842 vs57.847 baseline emulator FPS; cave56.940 vs57.037. Same195/279draws and56970/72468indices per frame. No speedup or hardware claim. Normal reserve17.96/18.09%, linear60.79/59.62%, process17.23%. Details actor-layer-state.json and actor-layers-{palace,cave}-old.json. Lighting/morph/callback/HUD/effect parity and projected/decal/opacity-plus-layer combinations remain absent. All sessions terminal. Goal ACTIVE/incomplete; continue measured native restoration, preserve deployed baseline.

The load compiler follows the pinned Foxhollow addShaderLayerStages ordering and addTexLayerStage color equations. It supports replace/modulate, accumulated-alpha interpolation, add and modulate for the applicable source paths. Two native images use independently prepared UV0/UV1 and live animation/scroll parameters. A PICA boolean selects UV1 for the second layer; opacity masks continue to use UV0. Existing material pools, range flushes and GPU-owner retirement remain unchanged, following the pinned Melee command_cache.c/render_snapshot.h ownership. Citro3D texenv.h supplies the direct combiner encoding. CPU work is bounded live material-state extraction and uniform binding; no general-purpose per-frame GX/TEV interpretation was added. Lighting-dependent source semantics remain incomplete until the native lighting stage is restored.

The shader grew from 47 to 50 instructions and retains 18 free float registers. No new draw/index workload was added. This is a fidelity restoration with approximately unchanged emulator cadence, not a performance improvement. The deployed hardware test remains the preceding base-texture stage.

## GPU restoration of CPU-stream skinning

9 October latest local native deformation repair: shared replay1a433cbe6d29 / CIAfdbfaa84c5c2, sourcee47d4f0443e4. NOT DEPLOYED. Lighting investigation found53model packets/18938vertices with CPU-stream skinning incorrectly represented by the object-world matrix. prepare_pica_actor_deformation.py creates FHNSCN0B40byte vertices with original byte/128 weights and offline bounded palettes. All normal-stream weights independently checked equal for these vertices; mismatch is rejected. Runtime uses existing GPU skinning shader (50instructions/78floatregs), no per-frame vertex conversion. Original CPU game/morph buffers remain updated; audit consumers before removing duplicate rendering work.
Private packet actor-deformation-compact/native-scene.bin SHA b51886a70344a4cf799f23e7382a4af020411b15fb8db8339bb0af3f6854a33d. Previous actor-deformation-prepared44byte intermediate is SUPERSEDED/incompatible, do not deploy. Affected groups333->366; overall1880groups/586623indices; affected GPU storage3434410->3580538bytes across full pack (not peak RAM). Static ordered260145corners/source attribute and independent weight comparisons pass;835models/7515negative cases/4000palette cases pass. GPU oracle matches original prequantization, not CPU short-output rounding. No simulation changes.
Old80 exactCIA headless palace/cave pica-actor-deformation-old pass; captures1290 inspected, saves unchanged, owned emulators exited. Palace57.838FPS vs57.842,197draws vs195, same56970indices/frame, submit3.492 vs3.389ms. Cave56.935FPS vs56.940,279draws/72468indices unchanged,submit5.218 vs5.216ms. Emulator ONLY; no performance improvement. Normal reserves17.95/18.09%,linear60.66/59.62%,process17.23%. Full records actor-deformation-state.json. Lighting, morph rendering, callbacks, HUD/effects remain incomplete. Next restore lighting after verifying original normal-matrix semantics, and audit duplicate CPU stream consumers. All sessions terminal; goal ACTIVE/incomplete.
Hardware unchanged: Old deployeddd1771fc4f6f/source6d5ed388699f. Latest collection230025/probe230059 only old4runs/currentlog byte-identical to prior15b466084. Source-ID clarification pending; do not repeatedly ask user whether they ran. No console writes this turn.

The existing Melee-derived command-list cache publication and GPU lifetime fences are unchanged. The departure is Foxhollow-specific prepared byte-weight skinning and offline palette splitting; no Melee gameplay/deformation semantics were transplanted. The new reader validates weight sums and every palette address before publishing an asset and forbids the legacy backend from consuming this format.


## Native frame pacing, 9 October (local candidate)

Physical Old80 actor-stage palace measured 49.994 ms/frame, including 15.771 ms
in the unconditional VBlank wait, versus 34.224 ms elsewhere. Cave measured
55.308 ms including 9.168 ms VBlank. These exclusive wall buckets include
scheduling; they are not CPU cycles. This motivates a pacing change, not a
claim that the full wait can be converted into performance.

`fh_frame_pacing.h` consumes a top-screen Citro3D retrace counter once. If it
advanced during game work, native rendering does not force a second edge before
starting the next iteration. If unchanged, it waits for VBlank as before.
Elapsed ticks do not become a catch-up backlog. Startup still waits, counter
wrap is covered, and non-native rendering retains the previous behavior.

Adaptation: pinned Melee `port/engine/video_pad.c` uses the actual display count
for pacing, and `gx_fifo.c:frame_pace` waits only until its deadline. Foxhollow
retains its original wall-stopwatch/timeDelta calculation and existing VI
callback count instead of importing Melee's fixed-step pad queue. No simulation
or audio work is skipped. The existing C3D_FrameBegin completion/ownership join,
explicit command publication, and render-target lifetimes are unchanged.

Replay-only `frame_pacing.txt` selects 0/1 for same-binary comparison; the headless
harness records the corresponding command argument. A host fixture exercises
100,000 jitter/late-frame schedules, unsigned wrap and no catch-up credits.
Hardware benefit remains unvalidated. Do not equate a higher submission rate
with scanout or full game performance.

The rejected diffuse-lighting experiment remains compiled out by
FH_PICA_ACTOR_LIGHTING_EXPERIMENT=0. Its source and numerical shader oracle remain
available while native procedural light-map support is implemented. This is an
explicit incomplete rendering stage, not a lighting repair.


## Native HUD images and ownership (local, 9 October)

9 October LATEST offline HUD-image checkpoint: unified replay c65386693357 / CIA 5df3374ed9f6 / source 0a223352cb36 / game archive 64079d0b2c89. All builds/tests/emulators terminal. No FTP/deployment while hardware unavailable; Old deployment1a433/sourcee47d4 and physical palace20.002/cave18.081FPS remain unchanged. Goal ACTIVE/INCOMPLETE.

Native orthographic HUD now includes loaded image widgets, masks, additive/color-only tinted sprites, health/staff/compass/inventory icons, plus prior fonts/rectangles. Producer registrations cover GameUI, minimap, save/options/WarpStone UI, object704 and text boxes; loading logos register from their separate arena and retire at boot-loop completion. Animation frames register at LOAD; logical texture unload, allocator frees and relocation manage native ownership. Batch retirement drains once per chain. No per-draw uploads/resolver/vertex building or discarded GX setup in native image helpers. Shared actor/HUD shader205instructions (180+25), HUD79floatregs; fixed1728byte vertices+1152byte parameters. Image pool remains2MiBOld/4MiBNew; max256page owners.

Final exactCIA-content cave Old80 and palace New124 pica-hud-image-assets runs pass; images1290 inspected, normal saves unchanged, owned processes exited. Cave submission5.046ms versus4.661font-only; palace3.255ms versus3.139. Both59.832AzaharFPS approximately, NOT console performance. GPU image peak1873248bytes,224KB remaining underOld2MiBcap. Old reservesnormal18.02%,linear53.93%,process17.17%; New31.75/80.80/19.14%; VRAMfree4858880 andmainstack72812/524288. Evidence hud-image-results.json,hud-image-state-test/,hud-batch-test/,hud-image-shader/ under artifacts/research/pica-scene-rewrite. Actual-C lifetime/batching/combiner-state checks and1152standalone+linked shader cases pass; linked shaderhash matchesfinalbuild.

Scope: NOT fullHUD/parity. General hudDrawColored also draws generated framebuffer/shadow textures; these are NOT loaded UI assets. First broad hook caught cardShowLoadingMsg's320x240RGB565captured backdrop. Final hook is explicitly at colored UI producers(engine0/50), leaving native captures/shadows still absent; no stale pixels uploaded, no claim that backdrop is fixed. Missing UI pages remain hard failures. Projected pause/item-menu images and text still omitted; next implement native projection over persistent quads using live camera/view/depth, without old fh_minimal_projected_image CPU vertex construction. Specular/morph/callbacks/otheressentialeffects, fullprogression/save validation andphysicalperformance remain unmet. Previous question ongeometry-stage particles/fur remains investigation only.

Pinned Melee reference reviewed: renderer.c texture_barrier() retires queued readers before texture reuse while retaining the target color. This stage uses the already verified Foxhollow owner fence and explicit GX_CMDLIST_FLUSH/cache publication, rather than copying Melee's whole-heap-flush behavior. Source relocation changes only the owner key because GPU images are owned copies. Logical cached-texture unload releases keys even when the allocator retains the original bytes; animation-chain release joins once, not once per frame image.

The first image candidate f1946f6f6a81 failed at retrace303 because raw loading logos bypass textureLoadAsset. Registration now follows GX descriptor initialization, and release follows the loading loop, rather than assuming the logo counter reaches600. d945eaf52879 and82d50bb3ba2d then exposed the generated card backdrop through the broad colored helper; preserved failed runs document this distinction. The accepted c65386693357 restores asset-backed HUD draws, not GPU capture effects. Image shader mask UV scaling, original alpha-times-four tinting, additive blending, and transition back to ordinary/font state have focused checks. No blanket global texture preload or per-draw source validation was introduced.

Projected menu source investigation: tools/native_menu_override.py currently calls fh_minimal_projected_image(), which returns in the replacement backend. Its CPU per-corner projection/vertex upload must not be reused. Engine0 GameUiMatrixWorkspace carries view/object transforms; pauseMenuSetHoloTransform changes camera FOV through Camera_RebuildProjectionMatrix, so consume live Camera_GetProjectionMatrix rather than assuming its separately calculated workspace projection is the active projection. Original GX positions are quarter units; preserve casts, depth, UV crops/flips, alpha and ordering. Keep this work independent of generated captured backdrops and shadow masks.

## Projected menus and hologram, 9 October

9 October latest LOCAL native hologram checkpoint: unified replay9012cd6bb545 / CIA262a89cceaed / source95896312e84d / game36bd41917c9f. All builds/tests/emulators terminal; no FTP/deployments while hardware unavailable. Goal ACTIVE/INCOMPLETE. Physical Old deployment1a433/sourcee47d4 and palace20.002/cave18.081FPS remain unchanged, no new physical gain established.

Native projected HUD is now runtime-validated: prior cave-ui only opened carousel (zero projected draws), new cave-projected enters Save via normal Start/A and leaves via B/Start. Initial748bf produced3767projected batches/39388glyphs and readable save choices. New9012 adds native pause hologram with existing persistent actor plans, own46instruction GPU shader, load-prepared projected samplers and load-uploaded32x32 opacity disk. No GX callback interpretation, old texture resolver, CPU UV/vertex generation, geometry rebuilding or extra render target/readback. Original zero indirect matrix collapses to two precompiled PICA combiner stages; RGB projected base, alpha clamp(channelAlpha-mask*konstAlpha). CPU retains only live matrices/alpha and bounded palette/uniform publication. Shared actor/HUD/hologram255/512instructions, individual float95/81/82, no geometry shader. Melee c2ebe7c769b3 texture_barrier/ownership and prior shared-DVLP integration consulted; retain verified host fences and explicit cache publication.

Old80 exactCIA cave-vram-off-main-pica-hologram-old passes, image1380 inspected with transparent blue hologram/readable choices.504hologram draws/120960indices,3773projected HUD batches/39512glyphs. Normal saves unchanged, drain/cleanup complete. Matching submenu native submission4.894->4.965ms (~0.071ms added, ~2draws/frame); emulated59.832->59.826FPS, not physical throughput. Idle5.097->5.099ms. GPU image peak1874272bytes (+1024); materialCPUpeak168412, both resident0 atshutdown. Normalpeak40823248/50331648; minlinearfree5565440/10485760; process69488640/83886080; stack72836/524288. Reserve includingfullstack17.85%, linear53.08%, process17.16%; active VRAMfree4858880. No measured>250ms windows. Evidence hologram-{results,timing}.json and hud-projected-before-hologram.json.

Standalone+linked2048hologram shader cases each pass (zero error), static limits/entry relocation pass, HUD batch ownership and material program tests pass. tools/test_native_menu_gate.py rejects actual carousel-only log as projected coverage and requires actual native hologram submissions. Headless runner now has --expect-projected-hud/--expect-hologram. Existing checked run predates these CLI flags but artifact runner performed same nonzero counters and new gate revalidates archived logs; no redundant boot.

Old touch C-stick retained and locally tested (not physical HID validated). Still missing native C-menu ring/icon callbacks and contextual A/enemy status callback states, generated shadows/backdrops, full morph/specular/essential effect fidelity; fullplaythrough/save/reload/stability and hardwareFPS targets unproven. Next native semantic callback stage should consume live selection/alpha/health and prepared textures/geometry directly, not reconnect the old callback/GX machinery. Hologram projected sampler added only at menu-object load, retired with original model ownership; one4byte pointer per native material plus small per-hologram sampler arrays, no texture duplication. Repro build-hologram.py and run-hologram-old.py under artifacts/research/pica-scene-rewrite. Latest ordinary game and both consoles are unchanged.


## Native item menu and contextual material state, 9 October

9 October latest LOCAL native UI-model checkpoint: unified replay67b61b1a3cb2 / CIA05c498bcd41b / source99df43943dd3 / gameca10ab1af2d3. All builds/tests/emulators terminal. No FTP or deployments while user lacks hardware access. Goal ACTIVE/INCOMPLETE. Physical Old1a433/sourcee47d4 and palace20.002/cave18.081FPS remain unchanged; no physical speedup established.

Native C-menu rings/icons now restored with prepared actor meshes, original live item pages/availability/fades, and fixed load-interned PICA combiner templates. No original render callbacks executed, GX/TEV interpretation, per-frame texture upload, per-vertex CPU work or new GPU allocations. Direct semantic adapters also implement A-prompt and lock/status alpha/depth/blend decisions, but those paths have NO runtime draw coverage yet. Reticle KAlpha uses raster RGB, not unused callback color RGB. Rings use constant RGB and secondary opacity replaces base alpha. Hidden original alpha-zero draws omitted with explicit counters. Standard native materials clear transient UI state; original resident owners/fences retained. Pinned Melee renderer.c texture_barrier at c2ebe7c769b3 consulted; reuse verified Foxhollow owner retirement and explicit cache publication rather than its whole-heap flush.

UI actor wrapper now folds live shifted viewport into camera projection before GPU vertex transformation/LCD rotation. Original C-menu right-side viewport had been ignored by the native actor wrapper. Uses 640x480 UI canvas on native400x240 target, no geometry rebuild. UI viewport depth is currently checked for original0..1; arbitrary UI depth-range support remains unimplemented. World/ordinary actor projection unchanged. UI states add24bytes per resident material, fixed16 program handles plus counters; no new shader instructions (shared native255/512, actor180/HUD29/hologram46; float95/81/82).

tools/test_pica_ui_model.py passes216064 actual original/native state comparisons including all byte alphas, item presence/availability, A-prompt flags and threshold boundaries;1024 viewport screen-point comparisons. Actual HUD batching/ownership test extended for borrowed base/mask samplers; passed with no allocation/wait while borrowing. Program interning/refcounts/capacity tests pass. HUD harness initially failed because its stripped-header stub omitted the new sampler prototype; stub updated and rerun, production build was already correct.

Old80 exactCIA headless cave-vram-off-main-pica-ui-model-old PASS, captures1500/1590/1650 inspected. Earlier1650 had floating labels/arrows with no ring/icons; new1590/1650 show frame and aligned icons.813ring draws,813icon draws,1084originally invisible icon groups skipped; A/lock draws0, so NO contextual visual-parity claim. Equal cave-ui recipe/save: idle native submission5.095->5.107ms, item-menu5.155->5.325ms (+0.170ms),6restored draws/frame; ~59.85AzaharFPS, not hardware timing. No measured>250ms observations. GPU image peak1874272bytes unchanged from hologram stage, materialCPUpeak171244, both resident0 atshutdown. Normalpeak40829480/50331648, linearminfree5590016/10485760, VRAMfree4858880, process69492736/83886080, stack72836/524288. Reserves including fullstack17.84%normal/53.31%linear/17.16%process.

New124 palace-vram-off-main-pica-reticle-new completes with59.831emulatedFPS/3.266mssubmission; normalpeak48148976/71303168, linearminfree20333824/25165824, process105144320/130023424, stack72836/524288; reserves31.74/80.80/19.13%. Palace idle and additional normal walking/lock-on replay (palace-vram-off-main-pica-reticle-walk-new) BOTH have zero A/lock draws; not coverage passes. Walking inputs and player movement/health change observed, no injected positions or mutated save. Stop blindly repeating these fixtures; next locate a real target/interaction with read-only state observation or an established interaction fixture, then verify dynamic reticle images. Both runs clean shutdown, saves unchanged, owned emulators exited.

Evidence ui-model-{results,timing}.json, ui-model-palace-timing.json, ui-model-test/result.json, each run ui-coverage.json. Reproduce build-ui-model.py, run-ui-model-old.py, run-reticle-new.py, run-reticle-walk-new.py in artifacts/research/pica-scene-rewrite. Latest binary includes all production changes; subsequent test-only HUD harness edit changes future source tag but not production behavior. Ordinary game and consoles unchanged. Still missing generated shadows/backdrops, remaining callbacks/morph/specular/effects, full progression/save-reload and physical sustained60/30New/~20Old validation. Old touch C-stick remains locally tested but physical HID unverified. Continue implementation; hardware unavailable is not a blocked goal.


9 October LOCAL native-only renderer and input recording checkpoint (not deployed; user hardware unavailable). Goal ACTIVE/INCOMPLETE.

User explicitly rejected retaining the old rendering fallback. fh_render_floor/minimal/scene and legacy budget selectors are now compile-time constants, not mutable SD/menu switches. Ordinary app always uses the new native actor/HUD backend; replay menu only offers new PICA stages7/8/9. Generated ARM ObjModel_BlendNormalStream is replaced completely (shared GQR/scratch setup retained; position streams/morph/animation/hit volumes untouched). Generated modelDoRenderInstrs retains its game-state prefix and native UI/hologram/actor dispatch, removes its old visual interpreter tail; renderMapBlock now calls the native plan directly. Unimplemented callbacks and secondary world passes are counted, not routed into the old renderer. Pinned original sources remain unmodified as references. Build link gate rejects retired renderer entry points/interpreters and mutable backend-selector symbols; it also requires the actual simulation-clock hook. Initial linked replay9c443 removes344744 text bytes and1856400 BSS bytes versus6f9ed (roughly337KiB code/1.77MiB static storage), not an FPS claim. Shared OS/fences/asset ownership still reused. Removing their old callers is not removing those required services.

Old80 CIA-content cave native-only check completes, screenshot1650 inspected with world/Fox/ring/icons/touch pad retained, saves unchanged.647 CPU normal calls/3933 blend groups avoided.526 unsupported actor-callback calls and17294 secondary-world-pass calls remain explicit omissions (counts are calls, not necessarily nonempty geometry). Native C-menu813 ring+813icon draws preserved. Actual process commitment67293184/83886080; normalpeak40829480/50331648, linearminfree5590016/10485760. This was a correctness run concurrent with a host build: do NOT use its wall timing as matched speedup evidence. A/lock runtime coverage and complete rendering/progression remain absent.

New ordinary app now has RECORD/STOP/SAVED/LOCKED/ERROR bottom-screen button. It can arm only before first gameplay input, uses first game PAD poll as origin and includes preceding neutral time, snapshots the retained save or read-only cold GCI/backup choice before input. Empty card supported; multiple/foreign card files rejected explicitly, no guessed selection. One private case folder contains fixture.gci, recording.fht and last-published scenario.fhr. Original card files are never overwritten by recording; normal gameplay saves still behave normally. Simulation clock observes original timeDelta, does not set it. Playback uses absolute timestamps, no hold-duration accumulation. State checks cover UI/player/loading/sequence/position; 160-unit position tolerance, checked every120 simulation ticks. This is divergence detection, NOT guaranteed deterministic simulation across hardware or camera/animation parity.

Streaming uses release/acquire SPSC512 records (28696byte ring),32KiB owned worker stack and8KiB stdio buffer; writer/reader owns the FILE, standard inherited caller affinity, priority0x31. Compared pinned Melee file_io.c/card_storage.c owned-worker/join discipline; no synchronous gameplay fallback. Main enqueue/consume has no SD I/O/waits. Snapshot/prepare and explicit stop/exit may perform IO/wait; overflow/error marks invalid without joining from the gameplay capture failure path. Reader prefetches before starting. Full-stream CRC, packet/clock bounds, missed complete button taps, starvation and state divergence are failures, never passing timing samples. No output recipe is published for failed recording. Completed files only are replayable. Initial replay log sampling is once per60 simulation ticks for long tapes; cumulative component counters retained. Physical SD/CPU cost and long gameplay remain unvalidated.

Regression evidence:100000 threaded ring packets/wrap/full/empty/serialization under ASan+UBSan; absolute60/30/20/irregular cadence and missed-tap/starvation/divergence rejection; actual background FILE owner10000 packets plus CRC corruption; actual recorder touch/first-input lock/snapshot/stop/raster tests; actual card snapshot CRC/data plus existing rollback/cold-remount tests pass. tools/test_input_{tape,stream,record}.py, tools/test_card.py, tools/test_replay.py. Custom Azahar HID now supports optional9-column rows (retrace/buttons/mainXY/CXY/touchXY/held); actual touch publication only, no game-memory writes/clocks changed. Baseline emulator/source restored after isolated build; helper parser/touch/release tests pass.

Actual New124 ordinary app3398fd01dc6b / CIA319fe0f59c81 records an11-packet title/menu sequence through real HID touch, snapshot24640 bytes same SHA as starting GCI, completes/Select exits. Captures420/690/1140 inspected: green RECORD/red STOP/gray SAVED. First run caught missing bottom-buffer presentation; corrected. Runner then hit an ordinary-vs-replay metadata variable error AFTER successful guest shutdown. Fixed gate and revalidated existing captures/logs/saves via post-validation.json instead of rerunning another boot; original failed summary preserved. Evidence artifacts/native/input-record-native-touch-visible-new/. Peak normal46000872/71303168, linearfree20333824/25165824, process102912000/130023424. Stream peak2/512. Headless OLD80 replay of the earlier same-behavior recording completes all state checks (status1), normal saves unchanged, input max lateness1.04387ticks (~17.4ms), stream peak12/512, no starvation. Evidence artifacts/native/input-record-stream-playback-old/; replayf36f8b3368f4/CIA1ad2d980301a source653468859f76 (prior final source-tag rebuild). No physical performance or long-playthrough claim. User's hardware unavailable: no FTP or deployment.

General UI depth range support from the preceding in-progress work is now included: fh_pica_viewport_depth maps near/far to C3D_DepthMap scale/offset; cached SDK state restored for world draws, no shader/buffer increase.12221 numeric depth cases plus200000 state-cache repetitions passed; unusual depth ranges still lack in-game coverage. No legacy raw depth readback reintroduced.

Continue renderer parity/performance work and longer recorded-gameplay validation. Current intro/profile menu still lacks some native visual callback details; C-menu/A/lock/effects/morph/shadows/secondary-world parity and sustained60/min30New/~20Old physical targets remain unmet. Do not call this a playable-complete port or mark the goal complete/blocked. Console deployments and last hardware20.002palace/18.081cave Old evidence unchanged.

