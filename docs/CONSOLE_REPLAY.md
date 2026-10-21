10 October visual handoff candidate: replay dd579215456e / menu source 050351e3f97d, with unchanged repaired topology9a795b4ddda2/nativef94831959008. Consult eye-uniform-rejoin-builds/deployment.json for actual transfer status; a prepared package alone is not installation. The candidate restores vertex uniforms after geometry-shader use and defers inactive geometry-uniform uploads. Local palace (Old/New), opening fight (New), normal menus (Old/New), and bounded-memory checks pass. Physical streak/RGB repair remains unconfirmed. Install SD:/3ds/foxhollow-replay/foxhollow-replay.cia in FBI to update the HOME icon, then select palace-facing / PICA ACTORS+HUD / MAIN / VRAM OFF / GPU profiling OFF, particles ON, and press A once. Inspect the purple creatures and surfaces until autoexit, then reopen ftpd. The long recording is unnecessary for this visual check. Determinism work is paused at the user's request.

10 October systemic actor asset repair: replay executable 7b66dd61aca1 / menu source 70544bedbac0, paired topology 9a795b4ddda2 and native scene f94831959008. Deployment status is recorded under systemic-actor-deployment; preparation alone does not mean installed. This supersedes the purple-head-only asset update below. The executable retains the tested eye-layer lighting fix.

All 37 audited bone-blend-boundary failures and the audited actor shape/UV failures are repaired. Of 625 remaining reduced sections, 555 use their original source and 70 retain independently checked reductions. This restores substantial geometry: 61,083 -> 193,510 triangles across these asset sections (original 195,008). It is a fidelity repair, not a speedup. World topology and texture image bytes are unchanged. Palette/compact format, ordered materials and both UV references are verified across all 835 prepared actors. Finite bind-pose checks do not prove every animation or GPU effect correct.

Short New/Old headless checks pass. Matched Old-profile palace/cave runs remain around 59.8 emulator FPS with virtually unchanged CPU submission, but submitted indices increase 8.7%/12.6%. Emulator GPU time does not prove unchanged hardware performance. Hardware validation: install SD:/3ds/foxhollow-replay/foxhollow-replay.cia through FBI if needed, run palace-facing then cave-idle with PICA ACTORS + HUD, MAIN, VRAM OFF and GPU profiling OFF. Both auto-exit; reopen ftpd for collection. The matching .3dsx is also supplied. If this same executable was already installed, this asset-only update does not require reinstalling it.

The separate long FHTAPE5 recording still has a state mismatch. Do not repeat 00-intro-v5 as a validated benchmark or make another long recording yet. No recording inputs, RNG seeds, assertions or saves were changed. Red/green lighting artifacts, other effect compatibility and performance targets remain separate outstanding work.

Earlier records below are historical.

Latest collected hardware results: Old exits cleanly with state mismatch at 415.418 simulation seconds; New at 810.028 seconds. New matches all 807 emulator game observations. Diagnostic same-binary 100%/25% speed checks preserve input/clock order but first diverge in RNG snapshots around 305.64 seconds and draw workload around 305 seconds. Audio status already differs earlier; its causal role in the actual failure is still being traced. No assertions relaxed, seeds injected or input edits applied.

Historical evidence below is retained; earlier LIVE/pending notes are superseded by these collected failures.

10 October FULL REPLAY VALIDATION FAILED after candidate deployment. New87822 andOld92381 BOTH TERMINAL cleanexit/status2 (state mismatch), nothang. SameCLOCK45799/PAD46786/IO1126, tick48601.689874625 (810.028s /13m30s), retrace45865; all807 measuredgame rows identical NewvsOld acrosssimtick,renderframe,draws,indices,UI/player/position/sequence/buttons/sticks. Evidence long-v5/cross-profile-failure.json (zero differingrows). Actualplayer teleport(13440.08,-206,-13233.81), expected(12101.8887,42.06,-14475.3203) atTestofObservation shrine. Screenshot45240 showscuptrial. Source399_ECSH_Shrine randomizescup andshuffle andteleportsoutonfailure; randomizationdivergence is HYPOTHESIS, notconfirmedrootcause. sfaRand globalLCG startsstatic0; sfaSrand callsinengine7 and14; no obviouswallclockseed. Do NOT forcecupoutcome,editrecordinginputs,relaxpositioncheck,injectguessedseed,orclaimfulltracevalid. No new recording requested. Alloriginalprivatebytes retained.

Candidate5153eb92f4c0 alreadybyteverifieddeployedBOTH atuserrequest, see160255/160256. User immediatelytoldlocalfailure afterdiscovery, warnednotvalidatedfullbenchmark; finalhardwaretestuseisdiagnosticwhetherphysicalconsolesreproducemismatch. They mayberunningnow; NO remote writes untilftpdback. Source3c5182327229/case00-intro-v5 andCIA SD:/3ds/foxhollow-replay/foxhollow-replay.cia,FBIinstallneeded. Normalapp/savesunchanged. Both fullchecksfinalmemorywithinbudget: Newheap48,567,192/71,303,168,linearfree17,931,264/25,165,824,process103,043,072/130,023,424; Oldheap41,227,160/50,331,648,linearfree3,251,200/10,485,760,process67,391,488/83,886,080. Both initializedVRAMfree4,858,880,measuredmainstack72,836/reserved524,288. No FPSclaim from concurrentruns. Allownedbuild/FTP/emulatorprocesses TERMINAL.

Next: diagnose originalhardware-vs-replay state divergence systematically; comparehardwarecandidate result whenavailable, targetedRNG/phase instrumentation orcapturedcheckpoint ratherthanrepeated14minblindboots. Add neededrecordedstate onlyafter rootcauseevidence; preservecurrenttrace forregression andvalidaterecordingchangeslocallybeforeaskinganotherlongsession. Ground-up renderer goalACTIVE/INCOMPLETE, no externalblock yet. Allprevious "runsLIVE" and "validationpending" notes below superseded bythisfailure.

10 October FULL RECORDING REPLAY CANDIDATE DEPLOYED to BOTH at user status request. Old229 console-test-apps/20261009-160255 and New223/160256 COMPLETE,8changedpayloads each fullystaged/readbackverified thenpublished/finalsizes; normal savehashesbefore=after. Shared5153eb92f4c0/CIA d10f86b5c32a2/source3c5182327229; full unchanged24m23s recording c6e866... case00-intro-v5,wallLIMIT7200. Privateassets reused onlyfrompreviousbyteproof+currentmanifest/sizes (Old004316/New110016),notfalselyfreshreadback. Package SHA4167f70c47fda24c3c5687c586e00bc17ec58f722cc98a88566efca54c54552f. User told ready: FBIinstall SD:/3ds/foxhollow-replay/foxhollow-replay.cia BOTH;case00-intro-v5, PICA ACTORS+HUD MAIN VRAMOFF/profilingOFF, Aonce,autoexitthenftpd. NewHBLmatching3dsxsupported. Normalapp/savesunchanged. Do NOT perform further remote writes while usertesting.

Candidate deployment explicitly communicated FULLSEQUENCEVALIDATIONPENDING; notclaimend-to-endpass. Old66.7simsecondprefixpasses; Newfull~46kframes andOld~30k ongoing, sameownedNew87822/Old92381 sessions. Continue pollingthese. User's latestrequest isdeploymentstatus; answeredready incommentary, continuefullchecks. Original validrecord preserved. GoalACTIVE/INCOMPLETE.

9 October full hardware-recording replay candidate (deployment pending)

Shared replay 5153eb92f4c0, CIA d10f86b5c32a2, menu source 3c5182327229. The private case 00-intro-v5 contains the user's unchanged complete 24m23s FHTAPE5 recording and original starting-card snapshot. Its two-hour wall timeout accommodates slower playback; simulation clocks and every recorded input/checkpoint remain unchanged. It exits automatically at the recorded endpoint or on a detected failure. Normal saves stay isolated.

Once deployment is confirmed: install SD:/3ds/foxhollow-replay/foxhollow-replay.cia through FBI on BOTH consoles to update the HOME icon. Launch Foxhollow Replay, choose 00-intro-v5, keep PICA ACTORS + HUD / MAIN / VRAM OFF / GPU profiling OFF, press A once and leave controls untouched. Reopen ftpd after automatic exit. Old 3DS playback can take considerably longer than the original session. A clean early exit does not mean completion; result.json must report status1 and all81606 clock steps. New 3DS can instead launch the matching SD:/3ds/foxhollow-replay/foxhollow-replay.3dsx in Homebrew Launcher. Use the CIA on Old 3DS for its authorized80MiB grant.

Playback now completes a due real disc read at the DVD pump before returning to game initialization, rather than waiting until a later PAD/CLOCK observation. Loading-loop extra PAD polls reuse the published input without consuming another game step. Exact read identity, callback outcome, order, CRC and state checks remain enabled. Each real-read timeout has its own start/reset; no timer carries over to later requests. Pinned Melee supplies the bounded FIFO/owned-read pattern; recorded callback publication is a Foxhollow adaptation. No renderer or gameplay shortcut is part of this change.

Local evidence: the unchanged deployedb508 trace failed during startup; diagnostic and partial fixes are preserved under long-v5. Current5153 completed the first66.67 simulation seconds on the Old80MiB emulator profile, all3916CLOCK/4910PAD/1105IO observations, proper shutdown and unchanged saves. Full New/Old runs remain in progress. Concurrent runs check correctness, not comparative FPS. Hardware recording had no overflow (90/512 peak); one write call took17.041s wall time, including FS/scheduling waits, not a measured main-thread freeze. Original recording and saves are preserved.

9 October physical short replay passed on both consoles; recording-pause candidate remains LOCAL.

The deployed recorder is normal75d0ec992dc1. The user reports intermittent freezing while recording; the latest hardware recording/logs have not yet been collected. Local normal347d40d8903f keeps the same FHTAPE5 format and renderer, reserves64MiB file capacity at recording startup, uses64KiB stdio blocks, and expands the bounded ring to4096samples (327,704bytes). Total additional RAM336KiB; stack unchanged32KiB. Startup/end may wait; normal enqueue still never waits for disk. Final close truncates to exact CRC-terminated length, publishing scenario.fhr only after success. This is a candidate mitigation, not a confirmed fix for the user's pauses. Host ASan tests and normal NewCIA recording pass; no console upload yet.

9 October short replay repair validation

After deployment is confirmed, install SD:/3ds/foxhollow-replay/foxhollow-replay.cia through FBI on each console, then launch Foxhollow Replay. Installing is required to update the HOME icon; merely copying a CIA is insufficient. The matching New 3DS HBL executable is SD:/3ds/foxhollow-replay/foxhollow-replay.3dsx.

Expected replay source ID: 49ceb70ba956 (3DSX b508e3536324). Select 00-clock-check, PICA ACTORS + HUD, MAIN, VRAM OFF, GPU profiling OFF. Press A once and leave controls alone until automatic exit, then reopen ftpd. This plays the intro/title, enters the name AAA and ends on Game Select. It starts from an isolated empty card and does not change your ordinary save. Allow several minutes on slower hardware. The unusable old opening recording has been deleted from both consoles at the user's request; its original bytes are retained locally for regression evidence.

The New and Old Azahar profiles pass this short case with all 2,996 recorded clock steps, 4,359 PAD states and 1,468 real DVD completions. Captures show the intended title/name/Game Select flow. Both reserve more than 15% of normal/linear/process memory, including the full main-stack reservation. This is not full opening progression, a physical fix claim, or an FPS improvement. The deliberately slower-CPU check is recorded separately and is never performance evidence.

9 October ordered-recording implementation details

The physical FHTAPE2 opening recording is preserved but does not replay reliably: Old 3DS fails a menu checkpoint and Azahar diverges in name entry. Name-entry scroll velocity advances per update, not by elapsed simulation time. Input timestamps cannot reconstruct omitted game updates. Do not suppress these failures or report their timings as a completed gameplay benchmark.

The new FHTAPE5 candidate records every game clock step, distinct controller state/checkpoint, DVD completion publication and both attract-movie scheduling time observations. Identical extra controller reads within one step reuse published input. The captured delta is applied before reciprocal delta and frame-step/remainder computation. Normal play returns all original delta/time values unchanged; recording only observes them. Playback performs real disc reads, validates file/offset/size/result, and delays completion publication when necessary. It never substitutes asset bytes or callback outcomes. Real read waits remain in wall timings and separate replay I/O counters. The movie deadline observations preserve its decoded-frame schedule without virtualizing SDK timing, GPU fences or audio service clocks.

This is captured-workload throughput, not proof of normal-play FPS, audiovisual synchronization across arbitrarily different playback speeds, or universal determinism. Earlier local FHTAPE3 capture replay diverged at startup; FHTAPE4 passed that boundary but hit a ten-second failure gate when the wall-clock movie decoder requested a frame early. FHTAPE5 now completes the short menu trace on New/Old profiles and a deliberately 25%-CPU Old profile; full recorded gameplay remains unvalidated. All state/order/CRC checks remain enabled. The original user trace cannot be upgraded with clock and completion observations that were never recorded.

On the candidate normal New 3DS app, RECORD arms the next launch and changes to RESTART. Exit and relaunch to capture from the first observed clock/input/I/O event. STOP or Select commits the trace and publishes scenario.fhr last. The starting card snapshot is private and immutable. Do not request another long hardware recording until a short cross-system trace passes.

Bounded resources: 512 x 80-byte in-memory samples plus a 24-byte ring header (40,984 bytes), 8 KiB FILE buffer and 32 KiB worker stack. On-disk packets remain 64 bytes. No growing history or per-frame allocation. The worker follows pinned Melee file_io.c/log_io.c: caller priority minus one, default app affinity, event notification, owned join before free. Failed CRC, full/starved queue, wrong event order, mismatched read identity/result, or state divergence invalidates the run. Replay release/dependency waits have a ten-second failure bound; actual worker ownership still must be joined before freeing buffers.

Replay preparation renames previous closed logs into its unique run directory instead of copying many megabytes before leaving the menu; final live-log archival retains copying. The actual archival function passes a host sanitizer regression for inode-preserving rename, live-log copy and collision preservation. Physical fix validation remains pending.

The instructions below concern previously deployed builds, not the unvalidated recording candidate.

# Current hardware check: particle-to-HUD hang repair

Replay **b87a39838629**, CIA **d3487ea70a2b**, menu source **b19679fb2f27**.
One shared Old80/New124 pair. The Old3DS isolated v3 probe passed all stages,
including eight repeated switches; the former same-list sequence hung.
The game now queues a command-list boundary before disabling geometry shading.
No CPU particle geometry or forced resource retirement is added. This is a
stability candidate, not an FPS gain: local palace submission added0.39ms and
still exceeds the particle-stage budget. Full physical game safety is unproven.

After upload is confirmed, install through FBI:
**SD:/3ds/foxhollow-replay/foxhollow-replay.cia**
Copying the CIA does not update the installed HOME icon. Launch Foxhollow Replay
and check source **b19679fb2f27**. Run **palace-facing** first, then **cave-idle**,
relaunching between them. Choose **PICA ACTORS + HUD**, **MAIN**, **VRAM OFF**,
**GPU profiling OFF**; keep particles **ON**. Press A and allow automatic exit.
Then reopen ftpd. No additional isolated probe is needed if both finish.
If a GPU timeout appears, reboot before another GPU app.

Local Old palace/cave and New recorded opening/combat prefix complete with
images inspected, logs drained and normal saves unchanged. Includes corrected
particle static-data ownership and source-owned dynamic local-corner support;
physical dynamic-particle coverage and full visual parity remain unproven.
No ordinary game or normal save replacement. No new hardware FPS claim.

## Historical instructions

# Previous hardware check: native particles and accumulated renderer parity

Replay 0d171fc383c8, CIA 8b94e6fb4e92, menu source **fed29d6d7a95**.
This is the preserved, emulator-tested glow-groups build. Unfinished dynamic
trail/rotating-particle source changes are excluded. Both consoles use the same
files; the CIA requests Old 80 MiB / New 124 MiB.

After upload is confirmed, install **SD:/3ds/foxhollow-replay/foxhollow-replay.cia**
through FBI on each console. Copying the file alone does not update the HOME
application. Launch Foxhollow Replay and check source **fed29d6d7a95**.

Run **palace-facing**, then **cave-idle**, relaunching between tests.
Select **PICA ACTORS + HUD**, **MAIN**, **VRAM OFF**, **GPU profiling OFF**.
Press A once per test and let it exit automatically. Leave particles enabled
(the fixture selects them). Then reopen ftpd. No reboot needed.

Watch for missing/flickering particles, geometry or HUD, freezes and audio gaps.
Palace purple glow/flames and the cave torch should be visible. This checks
physical cost and correctness of native HUD, hologram, stars and ordinary
particles added since the previous actor build. Reflections, dynamic particle
shapes and broader rendering/progression parity remain incomplete. New 60 FPS
and Old ~20 FPS targets are not yet verified for this build.

Local matched windows exclude VBlank: palace 13.086 ms, cave 15.042 ms;
these are Azahar measurements, not console predictions. Normal saves and the
ordinary game installation are preserved. Private replay assets are isolated.
Upload status is recorded in artifacts/console-test-apps/*/transfer.json.

## Superseded test instructions

# Current replacement replay test: deformation stage

Candidate 1a433cbe6d29, CIA fdbfaa84c5c2, menu source **e47d4f0443e4**.
One shared Old/New pair requests Old 80 MiB / New 124 MiB in CIA format.
Deployment completion must be checked in the transfer record.

Install **SD:/3ds/foxhollow-replay/foxhollow-replay.cia** through FBI, then
launch **Foxhollow Replay** from HOME. Copying the CIA does not update the
installed HOME application. The first bottom-screen line must show
`Foxhollow replay e47d4f0443e4`.

Run **palace-facing**, then **cave-idle**: **PICA ACTOR BASE**, **MAIN**,
**VRAM OFF**, **GPU profiling OFF**. Press A for each and allow automatic exit.
Reopen ftpd afterward. No reboot is needed. Ordinary saves remain isolated.

This is the latest accepted local replay candidate: compact 40-byte actor
vertices, supported dual-UV texture layers, and corrected GPU skinning for
original CPU-stream models. Paired private FHNSCN0B assets are required.
Palace/cave exact-CIA Old80 headless checks completed, captures inspected;
57.84/56.94 emulator FPS, not physical predictions. Lighting, morph rendering,
callbacks, HUD and essential effects remain incomplete. This is not a playable
release. The newer lighting experiment failed visual inspection and is excluded.

## Previous deployment and historical procedures (superseded)

# Current replacement replay tests

Candidate dd1771fc4f6f is the native actor-base stage. Its replay menu identifies
source **6d5ed388699f**. One shared `.3dsx` and `.cia` select Old/New hardware at
runtime; CIA profiles remain Old 80 MiB / New 124 MiB. Deployment status is in
AGENTS.md and the recorded transfer manifest; this document alone is not proof
that upload completed.

After upload, install **SD:/3ds/foxhollow-replay/foxhollow-replay.cia** in FBI,
then launch **Foxhollow Replay** from HOME. Replacing a CIA file on SD does not
update the installed HOME application. Check the source ID on the replay menu.

For the requested hardware check:

1. Select **palace-facing**, leave **PICA ACTOR BASE**, **MAIN**, **VRAM OFF**,
   and **GPU profiling OFF**, then press **A**. Let it exit automatically.
2. Relaunch and run **cave-idle** with the same settings. Let it exit automatically.
3. Reopen ftpd. Report any visible corruption, freeze, crash or audio gap.

No reboot is required. Normal saves and ordinary game settings are preserved.
The package's `visuals.txt` applies only to the replay application. It defaults
to actor-base mode; L cycles native geometry, world textures and actor base.
Geometry and world-texture modes remain useful controlled comparisons.

This is an incomplete rendering benchmark: native world/actor base images and
GPU skinning are present, but layered lighting, morph rendering, callbacks, HUD,
fog and essential effects are not complete. It is not a playable release. The
palace and cave images were inspected headlessly with isolated saves. Earlier
5fa0014ca1d5 local checks measured 57.88/56.94 submitted FPS respectively, with
3.44/5.35 ms native submission and at least 17.9% normal-RAM reserve. These are
Azahar observations, NOT console FPS predictions. The current candidate changes
replay-menu selection and identification; game/renderer code is unchanged.

The preserved physical legacy baseline is 9.91 FPS on Old 80 MiB CIA. An older
run selected legacy WORLD TEXTURES and measured 9.94 FPS. Neither measures this
replacement. The 9 October collection `20261008-230025` still contains only four previously
archived runs. A direct current-log read (`20261008-230059`) exactly matches the
older `palace-facing-000000015b466084-0` log. No new physical replacement result
is established. Check the menu source ID before requesting another replay.

See [UNIFIED_BUILDS.md](UNIFIED_BUILDS.md) and
[PICA_RENDERER_REPLACEMENT.md](PICA_RENDERER_REPLACEMENT.md).

## Historical measurements and procedures

The following records describe previous builds, not the current selection.

The preceding physical Old 3DSX run measured **9.19 FPS**, up from **1.04 FPS**.
Texture resolution fell from809.35 to2.31ms/frame; native submit58.81ms is now
the largest bucket. No pauses over250ms in the short measured window. Still
unplayably slow, with world/HUD only; models and effects are incomplete.
Headless CIA-content tests passed Old96MiB and New124MiB grants. Physical HOME
installation, launch, memory grant and CIA performance still require this run.
Normal game saves and the ordinary game installation are preserved.

# Historical instructions (superseded)

# Old 3DS texture residency fix (9 October)

Replay **661f6a8e6626** keeps the same world-texture stage and assets. It moves
CPU-only texture comparison snapshots into spare Old linear RAM while retaining
normal and linear reserves. The previous physical run measured 1.04 FPS with
809 ms/frame in texture resolution and thousands of budget evictions.

Run **palace-facing**, press **L** until **WORLD TEXTURES**, leave GPU profiling
**OFF**, press **A**, and let the sequence exit automatically. Then reopen ftpd.
No reboot is needed. This is still incomplete rendering, not ordinary gameplay.
Normal saves stay isolated. Old emulator texture resolution fell from235.55 to
0.678ms/frame and cadence rose3.758 to28.921FPS; physical improvement is unproven.

# Historical instructions (superseded)

# Old 3DS unified replay test (9 October)

The unified replay selects Old/New hardware at runtime. On Old 3DS it uses
46 MiB normal RAM, 6 MiB linear RAM, the smaller game arena, and no core-2
render worker. The private replay assets are independent of ordinary gameplay.

Choose **palace-facing**, press **L** until **WORLD TEXTURES**, leave GPU
profiling **OFF**, and press **A**. Let the sequence exit automatically, then
reopen ftpd. No reboot is needed. This tests incomplete textured world/HUD
rendering; characters, dynamic lights, water and effects are still absent.
Normal saves are preserved. Other fixtures are available but are not needed
for the first Old hardware measurement.

The New 3DS texture run has now been collected: 59.63 rendered submissions/s,
16.77 ms/frame, 13.99 ms main work and 2.78 ms VBlank in the short palace window.
This is not full-game rendering parity or a prediction for Old 3DS.

# Historical records and instructions (superseded)

# Current test: world textures (9 October)

Replay **d023694300ce** restores base/layered world textures, animated image
selection, scrolling and ambient colour. Characters, dynamic lights, water and
effects remain deliberately absent in this reconstruction tier. The normal game
and private assets/saves are unchanged. This section supersedes older requests.

1. Launch **foxhollow-replay** and choose **palace-facing**.
2. Press **L** until **WORLD TEXTURES**. Leave **GPU profiling OFF**.
3. Press **A** and let it finish and exit automatically, then reopen ftpd.

No reboot required. MAIN and VRAM OFF are enforced for this mode. Note any
flicker, missing surfaces or corruption. The question is whether adding these
textures retains the physical refresh-rate budget. There is no need to repeat
FULL or blank tests. WORLD ONLY remains available for a same-build flat comparison;
Select toggles optional GPU profiling in the menu, but that extra run is not needed
initially. Gameplay Select cancels. All runs use isolated save copies.

Azahar checks complete: textures and flat both exit cleanly, native world failures0,
normal saves unchanged, no runtime scene/texture-pack SD reads. The texture stage
has not yet been benchmarked on hardware; emulator timings are not a forecast.

# Historical records and instructions (superseded)

# Collected physical world result (9 October)

World + HUD reached 59.63 rendered submissions/s (299 frames/5.014s), with
12.75ms main work and 4.02ms VBlank. Profiling ON: ten valid samples,
1.04ms total IPC overhead. Main p95 13.62ms; worst frame 30.38ms.
This is incomplete flat geometry, without characters/textures/effects.
Evidence: artifacts/research/render-budget-reset/physical-world-summary.json.
No additional run requested now. The test instructions below are historical.

# Current test: first world restoration (9 October)

Replay build **b1dce300bb23** adds **WORLD ONLY** and optional GPU hotspot profiling.
This section supersedes all historical instructions below. Normal game and saves
are unchanged. This is incomplete flat world geometry plus HUD/movie output;
characters, world textures, water and effects are deliberately absent. It is not
intended for ordinary play. STRIPPED remains the blank CPU-budget comparison.

1. Launch **foxhollow-replay**, choose **palace-facing**.
2. Press L until **WORLD ONLY**. Leave **GPU profiling OFF**; press A and allow automatic exit.
3. Relaunch, choose **palace-facing**, L to **WORLD ONLY**, then Select to set
   **GPU profiling ON**. Press A and allow automatic exit.
4. Reopen ftpd. No reboot or further FULL/STRIPPED baseline is needed.

WORLD ONLY forces MAIN / VRAM OFF. Select toggles profiling only in the replay
menu; during playback it still cancels. Profiling is per launch and off by default.
The first run answers whether the restored world fits the physical frame budget.
The second samples eight tentative GPU stages in bounded 15.64ms windows, with
sampling overhead recorded; it is not an unperturbed FPS benchmark. Unsupported
registers/counter totals are reported invalid and profiling stops automatically.
The previous hardware stripped result was59.83updates/s with6.55ms main work.
World restoration is about18.8ms main work/29.92renderedFPS in Azahar, not a console
forecast. No textures or models will be restored until this cost is understood.

# Historical instructions (superseded)

Current result: both requested runs collected successfully. No further hardware
run is requested now. Full27.52FPS; stripped59.83updates/s with6.55ms main work,
10.17msVBlank. This is a blank CPU-budget result, not rendered60FPS. Evidence:
artifacts/research/render-budget-reset/physical-comparison.json.

# Current test: rendering budget reset (9 October)

New 3DS replay build **66e42bd8fe25** adds an L visual-mode selector and
sliced audio synthesis. This section supersedes the historical instructions below.
The normal game executable, configuration, assets and saves remain unchanged.

1. Launch **foxhollow-replay**, choose **palace-facing**.
2. Press L until **STRIPPED (blank)**, then press A.
3. Leave controls alone until automatic exit, then reopen ftpd.

No repeat FULL baseline is needed for this CPU-budget check. The saved physical
MAIN / VRAM OFF palace baseline completed at 28.225 submissions/sec (142 frames
in 5.031 seconds), recorded in native-vram-mask-guard/physical.json. It is an older
build, so comparison is historical, not a same-build controlled A/B. The immediate
question is whether stripped game updates fit the hardware refresh budget.

No reboot is required. The stripped run is deliberately blank, including loading;
input still runs automatically and audio remains enabled. This measures game-update
cadence and CPU headroom, NOT rendered FPS or playability. It may take a minute or
two including loading/log archival; recipe deadline is 150 seconds, cooperative.
Both modes use isolated copies of the same save and the same scripted sequence.
STRIPPED forces renderer MAIN/VRAM OFF internally. L also offers DRAW OFF (blank),
the older diagnostic that retains more visual preparation; it is not needed now.
The L/Y/R selections apply only to this replay launch, never ordinary gameplay.

Stripped source removes visual preparation for world/model draws, sky, reflections,
fur, shadow meshes, procedural effect triangles and precipitation geometry while
retaining audited game callbacks, animation/hit spheres, weather-state updates,
input, audio and loading. Broad equivalence across the whole game is not proven.
Audio worker releases its lock after each 160-sample slice; complete 640-frame
stereo output packets and callback handoffs are retained. Actual-code sanitizer
tests cover byte-identical output, delayed schedules, partial handoffs and stops.

Prior same-code stripped replay: 59.63 game iterations/sec in Azahar with 9.12ms
average non-VBlank main work. One missed refresh in its short five-second window;
NOT proof of hardware 60 FPS. Full graphics still need substantial reconstruction.
Final menu binary passes autonomous palace replay, save/state checks, log drain,
menu mode cycling and owned emulator cleanup. Old build is compiled separately;
no new Old hardware result is claimed. Evidence: artifacts/research/render-budget-reset.

# Historical test instructions (superseded)

# VRAM mask correction candidate - 8 October

Replay build `a78ee5049347` keeps uncompressed L4/A4 images in linear memory.
Other stable textures can still use VRAM. The prior `a80894c6a4d2` VRAM ON
palace attempt corrupted graphics and froze at rendered frame380, before its
measured palace window. Its baseline completed at28.225 FPS. No physical FPS
benefit or hardware fix is established yet. Do not repeat the old VRAM ON build.

The captured command stream has eight enabled TEXUNIT1 draws sampling L4 from
VRAM, matching the reported PICA corruption/hang hazard. The new policy applies
to every cached texture throughout its lifetime, including future sampler use;
it does not drop masks, change their texels or replace them with untextured draws.
A4 is excluded as the analogous unpacked four-bit format, not a separately
confirmed fault. ETC1 block compression remains eligible. Original linear images
are retained for cold demotion, bounded by the existing4MiB New/2MiB Old converted
texture budget. Additional VRAM copies remain capped at3MiB with1MiB reserve.

For physical validation of this candidate, choose `palace-facing`, renderer
**MAIN** with Y, **VRAM ON** with R, then A. Allow automatic exit, then reopen
ftpd. This tests the specific failure and obtains the previously missing palace
measurement. Replay defaults to OFF if no configuration file overrides it.
If recovering from the previous GPU corruption, reboot once first: the reported
hardware fault can persist across applications. No repeat baseline is required
for the first safety check; a matched pair follows only if this one completes.

The app, previous logs and normal saves have separate paths. Replay-only
installation preserves the ordinary game binary, assets, cases and normal saves.
Headless checks cannot establish physical safety, speed or sustained stability.
See `artifacts/research/native-vram-mask-guard/physical.json` and
[the original hardware report](https://www.3dbrew.org/wiki/GPU/Pitfalls#TEXUNIT1_Corruption_and_Instability).

# Console input replay

This separate development build runs the normal game with scripted controller
input. It is not a performance fix. Use the New or Old 3DS package matching the
console. Extract its `3ds` directory onto an existing Foxhollow SD installation.
Launch `foxhollow-replay` in Homebrew Launcher. The ordinary game executable and
normal saves are not replaced. Existing private assets and runtime settings in
`/3ds/foxhollow` are shared; record their hashes/configuration when comparing runs.

Use Up/Down to choose a case, **X** to validate its recipe and save, **A** to
validate and run, or **B** to exit. During playback, leave controls untouched;
**Select** cancels. A completed or failed test exits automatically through normal
shutdown. Initial asset loading and final log archival can take time. The wall
deadline is cooperative: it cannot recover a hung GPU, OS or game loop.

New replay build `60ceeab254d7` also supports **Y** to choose **MAIN (baseline)**
or **SHARED (experimental)** rendering before starting. This changes only the
current launch; it does not write ordinary game settings. The menu initially
reads `native_worker.txt` if present. Old 3DS stays on MAIN and has no Y toggle.
Results distinguish the requested mode from actual worker execution.

This build adds conservative propagation of clip-plane tests through world
bounds. Its cave replay reduced visibility CPU instructions, but overall emulator
timing changed only from 49.43 to 49.30 ms/frame. It is not a substantial FPS
fix. Old replay `7a37717085cd` is built with the same change and MAIN fallback;
it has no new runtime validation. Prior same-binary audio/menu checks used
`fe10ec27c4c1`; their record remains separate. See
`artifacts/research/world-plane-masks/integration.json` and
`artifacts/research/native-prepared-draws/audio-integration.json`.

For the controlled New 3DS comparison, choose `palace-facing`, select MAIN and
press A. Let it load, turn the camera and finish automatically. Relaunch the
same app, choose the same case, select SHARED with Y and press A. Leave controls
untouched in both runs. No reboot or manual progression is required. The two
private run directories are the evidence; keep the same assets/configuration
installed between runs. This tests whether the shared worker improves physical
performance. No speedup or 30/60 FPS result has been established for it.

Cases live in `/3ds/foxhollow/replays/cases/<name>/`. Each contains `scenario.fhr`
and an immutable `fixture.gci`. Before starting, the build checks its size, GCI
header and CRC32, then copies it to a new uniquely named run directory. Game
saves during the test affect only that copy. Results, replay inputs, state
observations and ordinary performance/SD logs are archived under
`/3ds/foxhollow/replays/runs/<name>-<id>/`. Logs from the preceding ordinary run
are preserved there with a `previous-` prefix before root logs are reused.
These directories contain private save data; do not publish them.

The refreshed private packages contain four cases:

| Case | Sequence and measured window |
| --- | --- |
| `cave-idle` | Load the genuine collected cave save through ordinary menus, check Fox's position, settle for 120 simulation ticks, measure 600 ticks (nominal ten seconds), then check position again. |
| `menu-eyes` | Wait for the title, press Start, settle for 180 ticks, and measure 600 ticks of the cockpit. This case has no camera checkpoint; inspect the view before comparing results. |
| `krystal-eyes` | Load the genuine early palace save, check Krystal's position, settle, hold the main stick down briefly, then measure 300 ticks and check position again. |
| `palace-facing` | Load that same palace save, turn Krystal, use L to face the camera toward the palace, settle, measure 300 ticks, and check position. This includes the walls, ground, rain and torches rather than mostly sky. |

These are short workload and replay checks, not progression or full rendering
acceptance. No diagnostic position-modified save is included. Earlier headless
evidence covers the cave case on both models, Old build `7ffb681509e6` at the
cockpit, and New build `f02fcc684f94` in Krystal's opening. The hierarchy update
has a completed cave replay on New build `b85a9e779573`. The subsequent uniform
upload change completed the same cave recipe on New build `7e4377c08b3c`.
The CPU-view update then completed the cave recipe on New build `911955735aa9`;
Old build `0c4a057ade94` is built but has not had a new runtime check. Exact records are in
`artifacts/research/native-world-culling/hierarchy-integration.json` and
`artifacts/research/native-uniform-batches/integration.json`, alongside
`artifacts/research/native-cpu-views/integration.json` and
`artifacts/research/console-replay/package-refresh.json`. This does not establish
every case on every build or physical-console performance. Packages share installed assets;
they do not install the separate experimental geometry/texture replacements.

The subsequent world-source scope experiment was rejected and removed. Refreshed
packages use New `86d395723c77` and Old `9063564aa1ed`: binary comparison proves
that only the 64-byte build-tag string differs from `911955735aa9` and
`0c4a057ade94`, respectively. Runtime evidence is reused accordingly; this is
not a new performance improvement. See
`artifacts/research/world-read-scope/restored-builds.json`.

The command-packet update now supplies New `d104e788a6f6` and Old
`833fa0ebe59b` packages. New completed the cave recipe with drained GPU work;
Old is built but has no new runtime check. Same-window main-thread instruction
encounters fell about 3%; this is not a physical FPS measurement. Exact stream
tests and matched-run records are in `artifacts/research/native-command-packets/`.
Neither package has been FTP-deployed or physically validated.

The subsequent world/collision broadphase update supplies New
`cbdf58b88dbb` and Old `bd4a34fb1e8b` packages. New completed the
cave recipe; Old is built only. Exact geometry-selection/collision checks and
same-window main instruction reduction of 4.635% are recorded under
`artifacts/research/native-broadphase/`. No physical FPS claim or FTP deployment.

## Timing and comparison

Holds use the game's elapsed simulation time, expressed in nominal 60 Hz ticks,
not rendered-frame counts. The original timestep, game work and renderer remain
active. Each press/release is delivered at least once; overshoot is recorded.
Overshoot is not carried into later holds. Different frame rates can still
change trajectories, camera state and menu timing. Position/UI gates reject
known divergence but do not prove identical scenes or deterministic execution.
Use short movement segments with frequent checkpoints, inspect screenshots, and
compare matching draw/index counts and game state before interpreting timings.
Replay candidates built after the 8 October camera update also support the
viewfinder angle gates below. Older installed replay builds do not understand
those commands. A general-purpose input recorder is still not implemented.

The current replay log reserves 491,520 bytes of ordinary RAM for 4096 observations plus
the bounded recipe. It adds no linear/VRAM pool and buffers rows during playback;
CSV writing and archival happen after the run. Sampling and the extra memory
still have overhead. Existing ordinary telemetry remains enabled.

Retrieve the run directory, then run:

```powershell
py -3.13 tools/analyze_console_replay.py path/to/run --environment console --output report.json
```

Use `--environment azahar` for emulator runs. Reports identify measured steps,
submitted frames per elapsed second, workload counts, observation latency and
input lateness. Submission cadence is not display scanout. Multi-frame sample
intervals cannot identify individual frame latencies. Emulator CPU/GPU timing
does not establish console performance. `accepted` means the recipe completed
and logs were written, not that the target FPS or graphics requirements passed.
The result's `source_tag` must match the packaged build manifest; preserve the
package hash, case hashes, runtime settings and assets for a reproducible result.
The analyzer also reports the recipe hash, save CRC and requested/executed worker
mode. Matching these does not verify that installed asset files are identical;
preserve the deployment's asset hashes as well.

Status values: 1 completed, 2 checkpoint failure, 3 deadline exceeded, 4 invalid
clock, 5 log capacity exhausted, 6 canceled. Partial/missing results are failures.

## Creating sequences

Prepare a private GCI and idle recipe using `tools/make_replay_case.py --help`.
Names contain only letters, digits, `_` and `-`, at most 31 characters. The menu
lists at most 16 cases. Edit the recipe to add controls and checkpoints:

```text
FHREPLAY 1
SAVE 24640 eb804909
LIMIT 300
LOAD_PLAYER -6619 -1036 -1662 100 240
HOLD 120 0 0 0 0 0 0 0 0
CHECK_PLAYER -6619 -1036 -1662 100
HOLD 600 0 0 0 0 0 0 0 1
CHECK_PLAYER -6619 -1036 -1662 100
END
```

`SAVE` is byte length and hex CRC32; it must match the fixture. `LIMIT` is a wall
deadline (maximum 600 seconds). At most 128 steps are accepted.

- `LOAD_PLAYER x y z radius timeout`: ordinary Start/A pulses through initial
  menus, then wait for a controllable player at that position. It selects the
  default profile; use suitable single-profile fixtures. No state is teleported.
- `WAIT_PLAYER x y z radius timeout`: wait without pressing buttons.
- `CHECK_PLAYER x y z radius`: fail immediately unless the player is ready there.
- `WAIT_TITLE timeout` and `WAIT_UI numeric_id timeout`: neutral state gates.
- `AIM_VIEW yaw pitch tolerance settle_ticks timeout`: use ordinary main-stick
  inputs to aim an already active first-person view, then require continuous
  agreement for the requested simulation ticks. Angles use the game's signed
  16-bit units (65536 per turn), yaw -32768..32767, pitch -8960..8960, tolerance
  128..1024, settle 1..600 ticks, timeout 1..120 wall seconds. The gate waits with
  neutral input through loading, cutscenes and viewfinder entry; it does not
  enter first-person mode, move the player, set camera values or change timing.
  Enter with a separate Z press. Yaw inertia must settle as well as the angle.
- `CHECK_VIEW yaw pitch tolerance`: fail unless the active viewfinder is at
  those angles with settled yaw. The limits match `AIM_VIEW`. Pitch is controlled
  by held stick displacement in this game, so a following measured `HOLD` must
  keep the appropriate stick Y: for example -115 for pitch -8192, -65 for -4096.
  Releasing the stick levels the camera. Diagonal clamping can alter the tilt;
  the aim controller turns first and then tilts. These are game controls, not
  C-stick camera controls; C-stick retains its item-menu behavior.
- `HOLD ticks buttons x y cx cy L R measured`: GC button hex mask, main/C-stick
  axes -127..127, analog triggers 0..255; final 1 records a measured window, 0
  settles/navigates. Maximum hold is 3600 ticks. Separate presses with neutral
  holds. GC buttons: A=100, B=200, X=400, Y=800, Start=1000, Z=10, R=20, L=40;
  D-pad left/right/down/up=1/2/4/8. The C-stick is the game's item menu.
- `END`: clean completion.

This format scripts GC-level inputs after normal 3DS input translation, so the
same sequence can run on both models without requiring a physical C-stick.

## Rebuilding

Use the existing Linux build environment:

```text
python3 /work/tools/build_native.py --replay-console --shared-worker-console
python3 /work/tools/build_native.py --replay-console --shared-worker-console --old-3ds
```

The pinned Melee platform/GPU ownership rules remain in force. This Foxhollow
adaptation adds a libctru startup menu, isolated memory-card directory, read-only
game-state observations and a `waitNextFrame` wrapper that accumulates the real
game delta. It introduces no GPU/graphics pipeline or worker-thread changes.
The menu restores stdout/stderr device ownership before the game initializes.

The camera update adds read-only camera position, yaw, pitch, viewfinder phase
and yaw-speed observations to the replay CSV. It reads the typed game camera
interface and only accepts the active viewfinder targeting the player. Adaptive
aim observations are logged but unmeasured. It adds 128 KiB to the replay log
allocation, no GPU/linear pool, and no ordinary-build runtime path. Host tests
cover uneven timesteps, angle wrap, drift, interrupted settling, deadlines and
invalid states; integrated before/after mesh runs remain necessary. Angle checks
do not prove an identical pose, camera position, visibility or game workload.

The particle-pool view experiment was rejected after a near-flat whole-frame
profile. Current local New replay `fe52d1a5525a` restores the previous code;
binary comparison proves only the source-tag string differs from `cbdf58b88dbb`.
Old replay `bd4a34fb1e8b` remains unchanged. Existing runtime evidence is reused.
See `artifacts/research/native-particle-views/restored.json`. No FTP deployment
or new physical performance result.

The world packet architecture experiment was archived without enabling a worker.
Current restored New replay `9d26f0375013` differs from `fe52d1a5525a` only
in its provenance tag; previous runtime evidence is reused. Old replay is
unchanged. See `artifacts/research/native-world-packets/restored.json`.
No new physical performance result or FTP deployment.

Current comparison candidate: New `fe10ec27c4c1`, with MAIN/SHARED menu selection.
Both modes completed the same palace-facing recipe in silent headless Azahar.
DSP output was captured in the host emulator, without capture code or extra I/O
in the console executable. The inspected trailing eight seconds of each output
had no exact silence gaps, clipped samples or half-scale adjacent jumps. This is
signal inspection, not proof of speech fidelity, every audio deadline or console
behavior. Evidence: `artifacts/native/palace-replay-main-audio/` and
`artifacts/native/palace-replay-shared-audio/`. Older four-route worker evidence
remains in `artifacts/research/native-prepared-draws/worker-integration.json`.
The package is a private comparison candidate, not performance-accepted gameplay.

The analyzer now reports `gpu_work_windows` when at least two valid accumulated
GPU-work snapshots lie entirely inside the measured replay interval. This covers
early queue prefixes and copies that the final `gpu_ms` segment excludes.
Pending-command time includes GSP/IRQ scheduling and overlaps CPU work; it is
not a hardware utilization counter. Missing/interrupted/invalid sample windows
remain explicitly unavailable. Physical cave baseline frames600..720 show
31.138ms pending work per50.139ms frame, not the ~1.4ms final-tail GPU figure.
The five-second palace tests contain only one interior snapshot, so no such
whole-window estimate is reported for them.

8 October hardware timing schema1 (replay7fc3171d24b0): each replay observation now
has a matching row in frame-timing.csv, stored in the unique run directory after
playback.27 exclusive main-thread phases sum exactly to the observation interval;
OTHER is explicitly unclassified. GPU snapshots are separate overlapping pending
command time, not exclusive GPU execution or utilization. Main GPU_WAIT covers
FrameBegin acquisition; any other blocking operation remains inside its named
caller phase. Worker CPU execution is not additive main wall time. The analyzer
rejects incomplete rows, mismatched frames, clock errors and any missing or
double-counted tick. Large OTHER requires finer instrumentation before attribution.
Run: python tools/analyze_console_replay.py RUN --environment console.
Old profiles are no longer archived as fresh native replay metrics.

Collection is now incremental. Completed run markers and inventories are checked
against SHA256-verified local archives. Unchanged files are referenced via the
transfer manifest (local paths may point into older collections). Current crashed
run logs still refresh; immutable prior logs/fixtures do not. --refresh disables
cache reuse. New files are collected first. FTP timestamps are metadata checks,
not remote cryptographic comparisons. No files are deleted from SD.

Installed New replay7fc3171d24b0 passed both MAIN/ON and SHARED/ON headlessly.
Requested physical run: palace-facing, SHARED, VRAM ON, automatic exit, then ftpd.
This is a measurement correction, not an FPS fix. Existing save fixtures unchanged.
