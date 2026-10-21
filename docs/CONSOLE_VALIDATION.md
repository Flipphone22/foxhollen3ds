16 October unified app ee35300a85c5: DSP audio now defaults ON for Play and
Benchmarks. Select Audio: DSP in startup to choose CPU for this launch. The
benchmark picker also supports Start to change audio. Selection locks when
audio starts; relaunch resets to DSP. One shared Old80/New124 CIA and 3dsx.
Remote paths remain SD:/3ds/foxhollow/foxhollow.cia (install with FBI) and
SD:/3ds/foxhollow/foxhollow.3dsx. No assets/saves/settings need replacement.
Deployment and hardware performance are separate from local build status.
See NATIVE_OLD_FAST.md for the same-app CPU/DSP palace comparison.

Current native-fog candidate: normal e783636ad1e8 and replay b49219fa4c26, one shared .3dsx and CIA each for Old80/New124. Installation completion is recorded in artifacts/console-settings-update. Normal CIA remote path SD:/3ds/foxhollow/foxhollow.cia (install with FBI for the HOME icon), or launch SD:/3ds/foxhollow/foxhollow.3dsx through HBL on New3DS. Replay CIA is SD:/3ds/foxhollow-replay/foxhollow-replay.cia.

This adds native distance fog and optional soft contact shadows. GRAPHICS page6 GROUND SHADOW defaultsOFF because collision receiver queries still exceed the cave CPU budget; page7 DISTANCE FOG defaultsON; page8 ORIGINAL GEO/TEX remains independent and restart-required. FPS/PERF STATS/GPU COUNTERS remain on page2. FHGRAPH4 preserves preferences on clean Select exit and migrates previous choices. Replay recipes select their explicit mask separately. Correctness and memory checks passed in headless Azahar; 59.83FPS in the New cave default-mask window is NOT a physical-console performance claim. Full visual parity, hardware effects validation and performance targets remain unmet. Saves and private asset data are preserved.

Current candidate697b6b1d9794: graphics preferences now persist when exiting with Select. A shutdown/reboot without clean exit may lose that session's preference changes. The first five GRAPHICS pages retain the existing switches; page6 adds ORIGINAL GEO. ON selects source geometry for actors and environments; OFF selects the performance pack. Change it, press Select, then relaunch. RESTART REQUIRED indicates a pending geometry choice; active geometry is never swapped beneath the GPU. Prepared texture resolution and incomplete native effects remain unchanged. FPS still defaults off on a fresh installation and remains idle when hidden. Replay recipes select their own graphics/geometry profile and do not overwrite normal preferences. Both variants use one shared3dsx/CIA acrossOld/New; CIA installation through FBI updates the HOME icon, while copying it toSD alone does not. Deployment completion is recorded separately in console-settings-update.

The synth worker refills without a forced timed wait after every ready slice; its idle wait, buffer counts and linear allocation are unchanged. This is not yet a physically established underrun fix or speedup. The larger-buffer experiment was explicitly rejected by the user and is not included.

Granular graphics 6cd59125022e (four local console-binary checks passed; deployment recorded separately): bottom GRAPHICS has five pages, using PREV/NEXT. Page 1: PARTICLES, LIGHTING, ENV LIGHT. Page 2: FPS DISPLAY, RENDERING (master top-screen draw switch), WORLD. Page 3: ACTORS, WORLD TEX, ACTOR TEX. Page 4: HUD, HOLOGRAM, SKY (gradient and stars). Page 5: HUD TEX, PARTICLE TEX, MOVIES. All visuals default on; FPS off. Settings apply for the current launch. Recording/replay locks graphics changes; FPS remains available.

These are explicit rendering omissions for tuning/diagnosis, not evidence of complete rendering or passing the performance target. Game simulation, collisions, progression, audio and saves continue; disabled geometry can conceal hazards. Texture-off modes retain asset residency and replace sampling with an immutable 8x8 white texture (256 linear bytes plus SDK allocation overhead), so they measure sampling/render differences rather than texture-memory savings. FPS measures rendered submission cadence. Bottom item controls remain available even when top rendering is off; hidden FPS has no periodic redraw/flush. No console performance improvement established for this candidate.

10 October regular app candidate 2ee8d97eb3f9 / CIA 55ea97a765df: includes the shader-handoff repair reported successful on New 3DS palace particles through replay 3dsx dd579215456e. New bottom-screen GRAPHICS controls: PARTICLES, LIGHTING (actor diffuse and environment), ENV LIGHT (environment contribution only), then NEXT > for FPS DISPLAY. All graphical features default on; FPS defaults off each launch. LIGHTING OFF also suppresses environment illumination. These switches change rendering only, not simulation/collision/progression. Other lighting, texture and effect parity issues remain.

FPS measures actual render-submission cadence using the existing presentation clock over at least half-second windows; it is not simulation FPS or GPU utilization. When visible, only a changed value repaints its small LCD rectangle, at most twice per second per back buffer. When hidden, the two buffers are cleared once and subsequent frames perform no FPS rasterization/cache flush/swap or additional clock read. Menu and recording status can still update on changes. FPS is usable during recording/replay; graphics settings remain locked during capture/replay. Old replay masks 0/1 keep their existing meaning. New optional disable bits 2/4 select no actor lighting/no environment lighting.

Same .3dsx and CIA support both models (Old 80 MiB/New 124 MiB). Install SD:/3ds/foxhollow/foxhollow.cia using FBI for HOME Menu, or launch SD:/3ds/foxhollow/foxhollow.3dsx through HBL. Transfer completion is recorded separately under graphics-menu-builds; this document alone does not prove upload. No asset or save replacement is required for this update.

10 October systemic actor asset repair: normal c0f0cffe5c74 / CIA 8920b091dbe1, topology 9a795b4ddda2, native scene f94831959008. Same shared Old80MiB/New124MiB executable pair. Transfer completion must be confirmed in artifacts/research/pica-scene-rewrite/systemic-actor-deployment; files prepared locally are not necessarily installed.

This repairs all 37 audited bone-blend-boundary losses and the audited actor shape/UV reductions, across the asset collection rather than one enemy. Restores original sections where independently checked reduction failed; retains 70 passing reductions. No runtime CPU skinning or per-frame geometry reconstruction was added. Large original sections are partitioned offline into bounded native draws. Original world packets and texture image bytes are preserved. This also retains the previous dual-layer eye-lighting fix; it does not claim every lighting/effect issue fixed.

The fidelity repair adds GPU geometry. Palace/cave emulator CPU submission remains essentially unchanged, but hardware FPS with these assets is not yet established. Full performance targets and the separate long-record replay divergence remain unresolved. Do not make another long recording for this update.

Install SD:/3ds/foxhollow/foxhollow.cia with FBI to update the HOME app, or launch SD:/3ds/foxhollow/foxhollow.3dsx from Homebrew Launcher. If c0f0cffe5c74 is already installed, replacing these assets requires no CIA reinstall. Saves are preserved. For reproducible performance checks use the separate replay app's palace-facing and cave-idle cases as described in CONSOLE_REPLAY.md.

Historical records follow.

9 October recording repair candidate: normal 75d0ec992dc1

This shared Old/New application adds the FHTAPE5 recorder on New 3DS. Deployment status must be confirmed in the transfer record. The separate replay app b508e3536324 has passed a short empty-save title/name-entry/Game Select trace on Old/New Azahar and a deliberately slower CPU configuration. Full opening gameplay replay is not yet validated; no FPS improvement is claimed.

RECORD now arms the NEXT launch and displays RESTART. Before entering gameplay, tap RECORD, exit with Select and relaunch the ordinary game. Recording starts automatically from startup, with a private copy of the starting save. STOP or Select commits the recording. Do not delete normal saves to record; the initial snapshot preserves them. For the present hardware check, use the separate Foxhollow Replay app and 00-clock-check instead of making another long recording. See docs/CONSOLE_REPLAY.md.

9 October normal validation candidate b82909f65c0d / CIA87d36eb544c8.
One shared executable pair supports Old80MiB/New124MiB. Install
SD:/3ds/foxhollow/foxhollow.cia with FBI for HOME launch, or launch the matching
foxhollow.3dsx from Homebrew Launcher. FTP copying does not update HOME.
Replacement PICA stage9 with prepared world/actor textures, native HUD and
particles; MAIN, VRAMOFF, GPU profilingOFF. Includes hardware-tested GS-to-HUD
command-boundary fix and corrected particle static-data layout. Rendering
parity and target FPS remain incomplete; this is a development build.
Physical replay averages: New54.25 palace/32.38 cave; Old18.41/16.02FPS.
Measured windows5/10seconds, no >250ms intervals. Not extended-game proof.

New 3DS recording: tap bottom-screen RECORD before any game buttons/sticks;
then play. Tap STOP and wait for SAVED, then Select to exit and reopen ftpd.
Inputs use simulation time, a bounded SD streaming ring, starting save fixture,
and state checkpoints. Empty-save starts supported; existing saves are copied,
not removed by RECORD. Playback checks divergence; determinism is not assumed.
Recordings live in SD:/3ds/foxhollow/replays/cases/record-*/.
C-stick is the item menu. Old 3DS uses the bottom-screen Items pad.
GRAPHICS exposes session visual options; particles default ON and options are
locked while recording to preserve the benchmark's graphics configuration.

Deployment status lives in transfer records; the following entries are history.

Current packaging: one shared Old/New 3DSX plus one shared Old/New CIA.
See [UNIFIED_BUILDS.md](UNIFIED_BUILDS.md) for memory requests, installation
and validation scope. FTP stages CIAs; install with FBI to launch from HOME.

8 October deployment candidate: New 3DS ordinary build 346ffa86eadd.
Includes accumulated texture-publication, culling, collision and command-submission
changes. MAIN rendering remains selected for ordinary play; the separate
foxhollow-replay app supports controlled MAIN/SHARED comparisons. No physical
speedup is established. The bounded exact-binary headless opening/movie test
completed 701 retraces with all 5940 GPU requests retired, no GPU errors, and
Select shutdown. Capture690 inspected; not a full progression validation.
Installed private assets and saves are preserved by this binary-only update.
See docs/GPU_BANDWIDTH_BENCH.md and docs/CONSOLE_REPLAY.md for test apps.

8 October local builds Newfb1fc8a560ab/Oldcabe83636cb7 are not deployed.
Bounded texture fallback passed isolated exact-console emulator runs; no new
physical FPS/stall result. Installed New73e/Old9793 remain. Full required visual
and performance acceptance: [RENDERING_ACCEPTANCE.md](RENDERING_ACCEPTANCE.md).

8 October continued goal ACTIVE. Local New84999e1cc69d /Old06876800ca02
built; NOT DEPLOYED. InstalledNew73e4838ca819 with repaired cave assets,
installedOld9793 remain. File-texture GX hints now preserve audited ownership
tokens; generic CPU writes including NULL still invalidate. Modes4(default)/5
(byte-check) added, mode1 retained for comparison. New cave+Ice checked replays
186188 token uses passed; fast cave compare traffic397124384->127001856B.
This is component work reduction, NOT physicalFPS or whole-frame acceptance.
All4targets compile. FinalNew790frames/13699GPUbalanced0errors, repaired arch
inspected; normal+stack55,758,912/71,303,168B,linearfree8,751,232/25,165,824B.
FinalOld790frames/13663GPUbalanced0errors,125038 byte-checked texture uses;
normal+stack40,549,072/48,234,496B,linearfree1,347,072/6,291,456B. Old harness
misconfigured to require New-only scene RAM cache: original failure retained;
guest Select/log/GPU evidence independently revalidated without another boot.
Harness now rejects that target/argument combination before emulator launch.
SD rename/delete/create/directory calls added; named counters SUBSETS of metadata.
Saves/durability unchanged. New package includes texture_assets=4; installer
validates policy/capability and restores mode1 for legacy-package rollback.
No FTP or console test requested. Source/cache/shader budgets unchanged aside
from160B metadata counters and hint counter. No raw-depth path change.
Evidence research/file-texture-hints/integration.json, implementation tail.
Next: address texture-cache all-or-nothing pressure fallback and save stalls;
broader world/model/material fidelity and physical30/60FPS remain incomplete.
Full-cache zlib sizing saves only~17%; not a sufficient standalone solution.
Keep goal ACTIVE and pursue implementation; no external input blocks this work.

## Local staging-reader candidate (not deployed)

8 October goal resumed ACTIVE; prior status-only turn no progress, this turn progress.
Local New a8ce7e68ef05 / Old ab967d27fb7c built and validated, NOT DEPLOYED.
Installed New remains73e4838ca819 with cave topologydac4ac/scene5bb0/cache7b64;
installedOld9793 unchanged. No FTP/save changes this turn; no new console probe.

Prepared asset staging now32KiB instead of4KiB in texture reader and scene GPU
loader. CPU stack staging retained; FS never targets linear/GPU destinations.
Melee pinned renderer native_geometry ownership and texture_decode separation
consulted. Foxhollow-specific bounded staging change preserves CRC/source checks,
existing cache release policy, GPU fences and pool limits. Extra stack28,672B;
no new persistent pool. Raw depth rejection and shared486/512 shader unchanged.
Actual C disk-path test482textures:3706->1063fread requests for identical
14,287,168bytes; exact everyimage, no directGPUreads. These are NOT hardware IPC
counts or timing. Resident482source/GPU images exact,0runtimeSD; corruption suite
1820checks/647rejections passes with ASan/UBSan. Four-target matrix passes.
New exactconsole asset-staging-new clean1101/790rendered/13532GPUbalanced0errors;
normal+stack55,771,296/71,303,168B,linearfree8,736,896/25,165,824B,stack54,840/524288.
Old asset-staging-old clean1101/791rendered/13643GPUbalanced0errors; normal+stack
40,552,944/48,234,496B,linearfree1,343,616/6,291,456B,stack55,132/524288.
Both990screens inspected: repaired cave arch retained. New retainedtexture/scene
runtimeSD0; Old exercised diskpath texture1,287,680B and scene2,199,278B/254reads.
No hardware FPS/stall improvement or whole-game memory acceptance claimed.

Systemic future world reduction guard added to prepare_model_lod.reduce_list by
default. New prepare_world_lod_fidelity retries narrower errors per material;
checks both surface directions, UVs, RGBA and winding. Sampled1worldunit,
1/128UV,1/32colour,20degree normal bounds; not continuous geometry/screen-error
proof. Original source bytes are fallback. Ordered GXstrip encoder preserves
triangle order/winding/all packets, enabling smaller safe patches without
reordering transparent surfaces. 11analytic acceptance fixtures,540strip round
trips and actual cave1093->573unsafe candidate rejected by default pipeline.
Guarded cave reductions save only44triangles (old encoding) or86/13520triangles
(orderedstrips), less than1%; NOTdeployed or described as meaningful GPU win.
Existing non-cave assets not globally repaired yet. New scripts/guard prevent
future unsafe preparations; installed original cave repair remains selected.

Hardware trace214938: retrace10707 1972ms contains311reads/1246ms with zero
disc/movie reads; retrace10109 1003ms contains215reads/912ms. Ret21251 4646ms
has no FS operations observed. Four~1270ms intervals coincide with two fullGCI
writes and metadata; SD wrapper omits some FSUSER operations (rename/delete),
so unaccounted duration is NOT proven CPU time. No blind async success on saves.
Staging addresses prepared-read amplification only; save/other stalls unresolved.
Evidence research/asset-staging/integration.json and world-fidelity/tests.json.
Allowned emulator/build/test jobs terminal. Goal incomplete, keep ACTIVE and
continue implementation; do not stop just because local checks or deployment end.

8 October New73e4838ca819 INSTALLED/readback-verified atnew223:5000
with repaired cave topology dac4acbd6d15, v8scene5bb0ae7d4a8f, cache7b64f69c46bb.
All15files staged and finalreadback verified; backups retained; saves,
disc, texture and movie untouched. Record console-test/20261007-221116.
Package65e1092519a83c313ed631b3ef4391e8f7d2bd40599f5da3e4b5198e211a79fb.
Normal controls traversed the non-ladder passage beneath the platform. Original
cave geometry restores the arch/UVs, plus73e black-object RGBfix. Only this cave's
five world source identities restored; not a global LOD/material fix. Exact-console
790rendered/13529GPUbalanced0errors; added15.54%indices, no physicalFPSclaim.
Surface diagnostic seven analytic fixtures pass. Allowned emulator/FTP jobs exited.
Old17c built/packaged only; installedOld9793 unchanged. Goal incomplete/progress.
Broader fidelity, multi-second stalls, physical30/60FPS and remaining materials open.
Evidence research/hallway-fidelity/integration.json and REPRODUCE.txt.

## Latest cave asset repair

8 October saved-cave passage repair, New binary73e4838ca819, assets pending deployment.
The latest save ab4698fe loads in swapholbot. Ordinary scripted controls descended
from the ladder platform and traversed the passage beneath it into the next
chamber. Reduced geometry created diagonal slabs across the arch and severe UV
stretching. Matching replay with original geometry restores the curved passage.
Only five swapholbot source identities are restored; other banks retain their
existing LOD. No collision, save or progression changes. This is a targeted repair,
not a claim that all decimation/material defects are fixed.

Private topology dac4acbd6d15 (4,426,408B), v8scene5bb0ae7d4a8f (39,953,108B),
cache7b64f69c46bb (15,874,705B disk, unchanged7MiB resident window) must be deployed
together. Existing texture760f3b/movie656ad remain. Original source-position refs,
GPU fencing and bounds refresh preserved. No graphics/OS implementation change
beyond the already-tested73e RGBfix. Pinned Melee ownership/persistent GPU buffer
approach retained; source geometry restoration is Foxhollow asset preparation.

Exact-console hallway-original-console clean1101/790rendered,13529GPU work
submitted/completed,0pending/errors. Matching binary/save/input baseline:
107587->107871draws (+0.26%),22,388,127->25,867,239indices (+15.54%). This fixes
visibility at added geometry cost; no hardware FPS or no-regression claim.
Normal+stackpeak55,771,296/71,303,168B; linearfree minimum8,736,896/25,165,824B;
initializedVRAMfree4,817,920B; measured mainstack26,168/524,288B. Prepared texture
and scene runtime SD reads0, no cachepressure or texture budget evictions.
All53banks coverage0missing, reader2102payloads/46356streamchecks; actual C refresh
1267world packets/808656vertices/2534negative checks and publication ordering pass.

New host surface diagnostic finds up to43.5world units of sampled deformation in
the old cave reduction. UV interpolation can change despite retaining original
surviving vertex records. A global quality guard and other banks remain future
work; this sampling audit is diagnostic, not a proved continuous surface bound.
Physical prior e3 mixed run21.64FPS with8frames>1s; tail60026.49FPS. Not a matched
fidelity/performance acceptance. Both performance target and broader goal remain
incomplete. Old17c built/packaged; installedOld9793 unchanged.
Evidence: artifacts/research/hallway-fidelity/integration.json; before image
hallway-navigation/frame-020.png, after hallway-original-navigation/frame-015.png.
Captures use matching route positions, not exact rendered frame assignments.

# Console validation: Ice Mountain terrain and material colour

New 3DS build `73e4838ca819`; separate experimental Old 3DS build `17c8dc4c4039`.
This is a development build, not a validated steady 30/60 FPS release.

## Changes and limits

Ice Mountain can move world vertices through game objects such as XYZAnimator.
The previous backend assumed world positions were static and rejected the whole
changed block. Version8 geometry records retain original position indices, so
the native renderer can refresh moved terrain and its culling bounds after a
GPU ownership fence. Unchanged blocks retain their GPU buffers. The recorded
Ice Mountain save required one 15,522-byte publication, not a per-frame rebuild.

Unprepared CMPR textures now use RGB5A1 instead of RGBA8: half the storage,
exact binary transparency and maximum RGB channel error 4/255. This is the
authorized reduction in colour precision; no texels or geometry are removed.
Prepared ETC textures, original assets, movie data and saves are unchanged.
Previous HUD, transparency, model-feature and crash fixes remain included.

The normal-save headless replay restores the missing slopes and walls. Previously black
trees, crates and torch posts are additionally corrected by restoring the original
constant material RGB instead of multiplying by raster RGB. The same native
PICA stage handles this without extra draws or passes. This does not fix
all lighting, shadow, face or material defects, or establish a physical FPS gain.
The original scene rejection checks remain for unsupported changes to topology,
UVs or colours; they are not disabled to make the terrain appear.

## Hardware check

After installation is confirmed, exit ftpd and launch Foxhollow normally through
Homebrew Launcher. No reboot is needed. Load the existing Ice Mountain save and
check the previously empty terrain while moving around. Press Select to exit,
allow buffered logs to drain, then reopen ftpd. Saves in
`SD:/3ds/foxhollow/saves/` are preserved; no emulator save is installed.

## Build and private files

The four-target build is reproduced by `tools/build_native_matrix.py` in the
configured Linux SDK environment after rebuilding the game archive with the
current overrides. `tools/package_console.py --native-reconstruction
--direct-efb --reflection-lod` creates the New package; add `--old-3ds` for Old.
Packages contain code/settings/instructions, not game assets or firmware.

App: `SD:/3ds/foxhollow/foxhollow.3dsx`; DSP firmware: `SD:/3ds/dspfirm.cdc`.

| Private file beside the app | SHA256 prefix |
| --- | --- |
| `disc.iso` (PAL revision 1) | `f5e5a9a29147` |
| `native-scene.bin` (version8 world records) | `1a24b85a9c1f` |
| `native-scene.cache` (matching New 7 MiB window) | `97e70004e545` |
| `model-lod.bin` | `be5685634a87` |
| `texture-pack.bin` | `760f3b09cceb` |
| `starfox-pica.thp` | `656ad2a7f8b3` |

Deploy the executable, scene, coverage/manifest and matching cache together
using the verified installer. Preserve the original RVZ. The native port still
reads original assets from the private ISO; native geometry, textures and movie
also use their prepared companions. See `PREPARED_SCENE_CACHE.md` and
`artifacts/research/texture-pressure/integration.json` for exact records.

## Controls

| Input | Action |
| --- | --- |
| Circle Pad | Left stick: movement/aiming |
| New 3DS C-stick | GameCube C-stick: items menu |
| A, B, X, Y | Matching GameCube button |
| L, R | Matching shoulder button, full analog trigger |
| ZL or ZR | GameCube Z |
| Start | Start/pause |
| Select | Exit the port after queued work completes |
| New 3DS D-pad | GameCube D-pad |
| Old 3DS D-pad | C-stick directions for items |
| Old 3DS bottom-screen ITEMS pad | Touch/drag around the center for analog C-stick item-menu directions; lift to release |
| Old: hold touchscreen outside ITEMS pad + D-pad | GameCube D-pad, including world map |

Button actions remain game-dependent. Rumble is unavailable.

## Validation evidence

`ice-world-admission` reproduced the empty terrain from the user's current
GCI save. `ice-live-world` follows the same ordinary load and input sequence:
world block295's 3,196 position rejections disappear, terrain is visibly restored,
and Select shutdown completes with11,049 balanced GPU submissions, zero errors.
The two existing compiled material omissions remain explicit.

Four target builds pass. Host tests cover1,267 world packets/804,317 vertices,
2,534 invalid-input checks and actual publication ordering. UVs, colours and
topology remain byte-identical; only positions change. Every2,102 asset packet
passes the C reader and46,356 stream checks. All53 banks retain complete asset
identity coverage. CMPR checks cover786,432 palette cases plus1,310,976 padded/
unaligned pixels; alpha is exact. New and Old storage/ownership checks pass.

New headless runs peak at56,034,272/71,303,168 normal bytes including reserved
stack; minimum linear free7,399,808/25,165,824; initialized VRAM free4,817,920;
main stack at most26,160/524,288. Existing15% normal and4 MiB linear guards remain.
No additional per-frame vertex work is performed for unchanged world blocks.
Prepared texture runtime SD reads remain zero. The Ice Mountain replay uses
18 prepared-scene reads/445,713 compressed bytes, without cache pressure release.

The matched older-save texture comparison retains essentially the same draw/
index workload, eliminates45 budget evictions and saves1,778,688 upload bytes.
This is memory/work evidence, not a demonstrated CPU or hardware FPS gain.
The console run collected before this fix averaged24.50FPS after frame300 and
22.16FPS over the last600 frames, but had missing terrain and mixed user movement;
it is not a matching-scene performance comparison. Target remains60 rendered FPS,
with steady30 minimum. Physical acceptance, remaining rendering defects, stalls
and full progression/stability validation remain incomplete.

Latest exact-console headless validation: ice-constant-rgb (790 rendered frames)
and menu-constant-rgb (313), clean Select shutdown, balanced GPU queues and
zero GPU errors. Paired screenshot990 shows the restored pine/crates/torch post;
Peppy570 reviewed. 24,576 original-producer cases and 10,240 integrated combiner
cases pass, plus prior alpha/mask/callback regressions. Matching Ice draw/index
work differs by less than0.1%; normal+stack55,710,352 bytes, linearfree8,042,496,
VRAMfree4,817,920, mainstack26,168. No physical FPS/no-regression claim.
See artifacts/research/model-constant-rgb/integration.json. Existing version8
scene/cache remain required; this update does not change private assets.
