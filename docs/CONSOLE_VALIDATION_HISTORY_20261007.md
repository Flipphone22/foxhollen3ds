8 October Singapore / 7 October UTC: New candidate93cc5fc33fe7 fixes missing
prepared shop/swapholbot geometry in v14 and adds startup-resident prepared
textures alongside the geometry cache. Olda32a79ef08b8 built, not deployed.
Physical latest2c36 store run collected20261007-153442:19.248FPS mixed, last600
16.672FPS; not target. Backup of real save20261007-153732 is read-only.

Corrected isolated shop visual pair console-shop-relocated-v13/v14 reproduces
missing floor/walls with v13 and restores textured room/doorway with v14.
Synthetic LOCAL diagnostic position only: never install this card or claim
progression. Initial globalmap-position fixture was INVALID; game relocates
shop to X[-11,-9],Z[-6,-4],layer1. Read-only runtime map inspection supplies
correct bounds. Normal unmodified save tests reload in hut, not shop.

New exact-console tests pass clean Select/shutdown. v14 shop GPU11140balanced,
0rejects; normal+reservedstack56,070,928/71,303,168B (21.36%free),linear19,826,304/
25,165,824B (21.22%free), mainstack26,104/524288. Both prepared caches: ZERO
runtime SD reads and ZERO pressure releases in tested routes. Geometry RAM
7,281,366B /7MiB cap; texture CPU staging10,617,152B /11MiB cap in linear pool.
Live GPU images retain separate ownership; release optional staging before
4MiB linear reserve is exhausted. Old uses disk reads. No GPU pass/shader change.
Disc streaming/game preparation remain possible stalls; no physicalFPSgain yet.

New immutable package93cc SHA bf895a2d2d3e, Olda32a SHA d42e92b83697. v14 scene
SHA5f93b7de4ca1,cache729b7b666790. Latest IMPLEMENTATION and PREPARED_SCENE_CACHE
have reproduction/evidence. Deployment20261007-160037 is IN PROGRESS; inspect
transfer.json and owned session45850 before claiming installation or restarting.
InstalledNew remains2c36 until verified commit; Old9793 unchanged. Preserve all
saves/RVZ. Active goal incomplete; current turn made substantive implementation,
physical evidence and verified rendering progress. Do not request another probe.

7 October latest priority: multi-second stalls are acceptable during startup /
initial loading only; eliminate gameplay stalls first, then sustained FPS.
Prepared geometry RAM cache implemented and tested. New ddab6748b5cb and Old
1ff80862ebe4 built; New cache6,820,890B normal RAM, cap7MiB,15% global reserve.
Old retains disk reads. Pressure release preserves packet cursor and yields
optional cache to live assets. New4MiB texture snapshot cap also included.
Actual C ASAN/UBSAN cached/uncached/corrupt/pressure stream tests pass;92malformed
fixtures rejected; New/Old texture1095/879joins. Final exact-console moving
ThornTail1400/Select1401 clean:857cache reads,2,633,536B,109decodes,ZERO runtime
sceneSDreads/bytes/ticks,0pressure releases,0budgetevictions,123sceneassets/0rejects.
GPU21354submitted/completed,0pending/errors. Normal+reservedstack56,427,328/
71,303,168B (20.86%free),linear9,070,464/25,165,824;measuredmainstack26,444/524,288.
Image1260 inspected; foliage/lighting defects remain. No physical speedup or
all-stalls-fixed claim. Original disc/texturepack and game loading still remain.

New package foxhollow-validation-ddab6748b5cb.zip SHA9e5e8cda7601 prepared,
Old foxhollow-o3ds-validation-1ff80862ebe4.zip SHA8b4c13dfccb3 prepared. Neither
deployed: FTP New223:5000 refused beforelogin at20261007-152905; zero remote
files changed. InstalledNew remains2c36 andOld9793. Do not restart identical
emulator boot tests. Await reachableFTP for substantive cache deployment, then
measure gameplay stall tails, scene cache/counter use and total SD/CPU costs.
Use --native-scene-cache artifacts/research/prepared-ram-cache/native-scene.cache
with selected New package; original scene manifest/source verified. Keep saves.
Instructions docs/PREPARED_SCENE_CACHE.md, details latestIMPLEMENTATION section.
All owned jobs terminal; active goal incomplete. This turn made implementation
and integrated progress; FTP refusal does not block further local work.

7 October latest physical result: New2c36d8e60387 run completed without crash;
user reports final1-2minutes were RUNNING AROUND THORNTAIL HOLLOW. Collection
artifacts/console-test/20261007-144723/ complete; executable readback matches2c36,
all four gameplay streams changed since installation. Shutdown15885; GPU289376
submitted/completed,0pending/errors, normal palettes505868/fallback0. Evidence
supports this run's stability only, not all crashes solved. Physical15627frames
(afterstartup34) /577.864s =27.043FPS; final1800frames/95.508s =18.847FPS,
median49.230ms,p9585.012ms. NOT steady30/60. Current active performance baseline
is moving ThornTail gameplay, not idle cockpit and not historical6FPS Peppy.
User reiterates PICA-friendly lighting/texture/rendering fixes; preserve cues,
avoid costly added passes/CPU fallback without measured budget evidence.

Textures459budget evictions; source snapshots peak2,097,056B (2MiB cap), GPUpeak
3,328,512B. Physicalnormal49,002,376B +524288reservedstack; linearpeak8,313,344B;
VRAMafterinit1,477,632B; measuredmainstack33,680B. SDreads97,580,434B/53.597s,
assets80,355,872B/14.661s,max874.684ms. Worker I/O overlaps frames; not additive
mainCPU cost. Writes7,442,549B/3.855s,max203.870ms,0read/write errors. Detailed
performance-analysis.json in collection, including frame windows and caveats.

Local candidateNew820ca1c3863e increases CPU texture snapshot cap2->4MiB only;
keeps exact bytes,15% normal reserve,GPU/linear limits. Old7a079d1b972f unchanged.
Rejected snapshotless experiment caused6359uploads byframe417 and boundedtimeout:
reverted, never deployed. Repeated-publication host tests now catch that failure.
New/Old tests pass. Exact-console1400-retrace moving checks both Select1401 clean:
baseline37evictions, candidate0; peakcandidate normal+stack49,601,216B,
linear9,061,248B. Sameinput/save but poses/camera diverged, NOT scene-matched FPS
or pixel-equivalence evidence. Images1260 inspected; visual issues remain.
No new hardware build/probe deployed this turn. InstalledNew2c36 andOld9793.
Do not ask for another console test for this modest cache-cap change alone;
continue substantial PICA-friendly performance/visual work. All owned jobs ended.
Goal incomplete; current turn progressed through new physical evidence and code.

7 October deployment confirmed: New 3DS build 2c36d8e60387 installed and readback-verified at new223:5000. Transaction artifacts/console-test/20261007-142246/transfer.json completed all eight files. Previous executable/config and failure captures retained as remote backups; assets and saves preserved. No new probe required. Launch normal Foxhollow for previous Pepper/Fox landing cutscenes, then Select and reopen ftpd. Physical stability and CPU cost remain unvalidated; no FPS gain claimed. Old 7a079d1b972f package built, not deployed; Old installed9793 unchanged. Install process completed; no owned live jobs. Goal incomplete.

7 October physical replay v3 isolates a normal-transform shader path failure:
console-test/20261007-141026 confirms installedde82. Control2061, baked3061 and
CPU normal palette1061 complete; original GPU inverse61 times out. This is now
valid relocated input evidence. Precise hardware instruction/root cause remains
unproven; do not claim every game hang fixed. No further probe rerun requested.

Candidate gameNew2c36d8e60387 / Old7a079d1b972f, emulator112a2cf9fa3e/
20f29bd17267 BUILT. CPU normal helper resolves finite ill-conditioned/small
matrices on ARM, preserving positions and bone blend ratios; 935 current model
groups all fit11bones. No model-ID exceptions or game/progression changes.
3000numeric cases and captured matrix pass; actual-C palette shader comparison
586vertices has exactpositions/UV and colourdelta2.38e-7. Generic>11/invalid input
still rejects; current package contains none. Shared queue no-panic wait included.
ExactNew headless console-fox-arm-normal-fix1100/Select1101 clean,17797GPUcommands
complete, normal_palette34262/fallback0. Image inspected; knownblackpalms remain.
Normal+reservedstack49,257,000/71,303,168B;linear8,817,664/25,165,824;
VRAMafterinit1,477,632/6,291,456; observedmainstack26,404/524,288. No hardwareFPSgain
or gameplay stability acceptance yet. Main console deployment PENDING; Newstill80e
andOld9793. Preserve saves/assets. No live owned jobs. Goal active.

7 October latest: probe v3 de82bf85838b INSTALLED/readback-verified on New223.
Hardware v2 log collected135053: GPU fill completed, then draw1061 timeout.
IMPORTANT: v1/v2 replay relocation was WRONG (offsets retained old addresses),
and INPUTBUFFER_CONFIG was omitted. Do not use their timeout or rendered images
as evidence that captured game geometry/shader fails or passes. Small-matrix
inverse hypothesis remains unproven; earlier inference from v2 is withdrawn.
v3 relocates effective vertex/index addresses and restores paired input count.
Host2418address checks/62draws/3heap destinations pass; old indices reach65535
instead of292. v3 adds control2061, expected baked varyings3061 before1061/full
original captures. Headless landing-gpu-relocated-baked all8draws complete13.70s,
image inspected (control triangle). Separate private baked.bin25,394B installed;
original25MBfixture, gameNew80e/Old9793 and saves unchanged. Next physical run:
foxhollow-gpu-probe once, recover immediately onTIMEOUT, reopenftpd. No gameplay.
Game local matrix nowNew19165929b436/Old72c1d44c9d43, emulator8a1e7c63d92a/
e477563cd085: shared diagnostic waits replace surface3-second panic. Host268
waitcases and2048queue schedules pass, rawdepth rejection preserved. NOT deployed;
no new game integration run after this edit. No physical stability/FPS claim.
All owned processes terminal; goal active. Current turn progress: valid replay
contracts and retained-resource queue behavior; root game freeze still unresolved.

7 October probe update: standalone 7b35714b9c92 is INSTALLED and readback-verified
on New new223:5000. Prior probe7c30 OS-crashed BEFORE captured draws:
Luma dump190 code matches archived ELF, PC00100d8c STR to VRAM1f000000,
DFSR080d. CPU VRAM initialization was invalid on this hardware; Azahar missed it.
Replaced direct stores with ordered GX memory fill for both owned banks.
Offline read-only mapping test rejects old CPU stores and missing completion;
headless landing-gpu-vram-fill completes all6draws in13.14s with byte-identical
captured color output. This fixes the identified PROBE startup fault, not the
unresolved game landing/cutscene hang. Hardware probe result still pending.
User should run foxhollow-gpu-probe once, then reopen ftpd; no gameplay replay.
GameNew80e and Old9793, fixture and saves unchanged. Backup of old probe retained.
Evidence console-test/20261007-133921/probe-crash-analysis.json and
research/gpu-capture-replay/deployment-v2.json. No live owned processes.
Ownership review also found a remaining 3-second panic in native_surface_split;
not edited yet. Follow up with shared queue diagnostics, not unsafe reset/reuse.
Goal remains active; this turn made progress through physical evidence and fix.

7 October deployment confirmation: the separate foxhollow-gpu-probe app (7c30f83aed71), replay fixture (8abe20da46fc), and README are installed on New 3DS new223:5000 at /3ds/foxhollow-gpu-probe; all three files were readback-verified. See artifacts/research/gpu-capture-replay/deployment.json. Ready for one automatic hardware probe run; no gameplay replay requested. No physical probe result yet. Main game and saves unchanged.

7 October: user requires a SYSTEMIC renderer fix; do not present another
tiny-model culling change as crash recovery. New game80e7dc934d53 remains installed;
it froze during Fox landing. Old9793 unchanged. Fresh complete capture125148.
Queue count13,last10,current11 => DRAW batch10 outstanding; copy11 unsubmitted.
Initial commentary incorrectly named the copy; corrected after queue audit.
Physical mixed11010loggedframes29.346FPS,last30021.281FPS; buffered tail missing.
Root cause UNPROVEN. Complete62draw capture: layouts/bounds/sampler checks pass;
lastbatch58..61 expected shader replay2132vertices finite. Draw61 model459 is
collapsed and offscreen; lastbatch also has model1152 draws58..60. No claim which
draw stalled. Local generic side-plane cull fixes missed rejection, NOT hardware
stability proof. User-directed pivot: isolated GPU reproduction/systemic contracts.
Local Newcc5dd1595394/Old164f83a1c20d, emulator da43604a8732/ba0ab69dacd7.
Includes uniform batch reuse and offscreen bound. NOT deployed. Normal emulator
flight/landing/Fox/save completes,110817GPUcommandsbalanced; image inspected,
known black foliage and opaque hologram remain. No 30/60hardware acceptance.
Separate diagnostic artifacts/research/gpu-capture-replay: final7c30f83aed71,
fixture8abe20da46fc. Headless13.1s passes6draw submissions, captured output inspected.
Draw1061 = captured61 with CPU normal palette, then original61, then58..61.
CPU comparison preserves positions/UV exactly,colourmaxdelta2.38e-7; hypothesis,
not proven cause. Live immutable buffers, per-draw completion; no GPU reset.
VRAMreflection substitute cleared, fog/fragmentlighting disabled; NOT full-frame
reproduction and a passing probe cannot establish game ownership/cache correctness.
Check gpu-capture-replay/deployment.json for install status. Game/saves untouched.
Next: retrieve probe evidence when user reports run. Do not repeat full cutscene
replay or deploy culling alone as a fix. Continue renderer-level investigation.

7 October New 80e7dc934d53 INSTALLED and readback-verified at
new223:5000. Transaction console-test/20261007-123332 completed12files,
including v13scene019ccb06814a and482texturepack760f3b09cceb. Saves/ISO untouched;
previous files and partial4628GPUdiagnostics backed up. Old9793 unchanged;
localOld84533cbc5d65 package ready, NOT deployed. All owned jobs terminal.
Candidate subpixelLOD fix needs physicalPepper validation. No hardware30/60claim.
Latest actualphysical4628 mixedrun25.832FPS, still froze; partialcapture122039.
Next: retrieve new80e run only afteruserreportsit, continue larger CPU/render
preparation work meanwhile. Do not ask forunchanged menu tests. Goalactive.

7 October current New80e7dc934d53 / Old84533cbc5d65, deployment pending.
Fresh physical4628 freeze captured122039: GPU timeout dump exists, but native
log stopped at64KiB and input snapshot incomplete11.600MB; final queue unknown.
User DID run4628 now. Do not reuse the earlier stale-log conclusion.
Candidate extends collapsed-model detail policy: inflated projectedbounds<.25
pixel on both axes permits LOD even nearpixelcenter; not zero-coverage proof.
Actual recovered model457draw109 numericalfixture nowLOD1 (priorretained).
Missing queue status means this is NOT a proven culprit or hardwarefix.
Flushqueue log BEFORE bigdump. Preserve GPU ownership waits; no forcedreset/exit.
6000bounds cases/3capture fixtures and266waitcases pass. ExactNew Palace2100/
Select2101clean,28482GPUcommands complete,zero rejected; image inspected.
LivePalacetest hasZERO subpixelLODadmissions; fixture tests exercise change.
All4builds current, includes priortexture/constantlighting/modelview changes.
Physical4628 recordedmixed5031frames25.832FPS,last30026.136FPS; steady30unmet.
Finalfreeze frame/tail unavailable. Evidenceconsole-test/20261007-122039 and
research/console-4628-pepper-hang. User requests deployment; stage/readbackcheck
newbuild+v13scene+482texturepack, preserveallsaves/originalRVZ. Goalactive.

7 October local New7c8b0e595080 / Old89307dc2fbbb, NOT DEPLOYED.
File-loaded CMPR textures use bounded lifetime/visibility tokens; dynamic and
unknown sources still compare. New/Old host ASAN/UBSAN pass. ExactNew checked
Fox run verifies97815reuses,19409GPUcommands complete; fast98166reuses/19349
complete. Select1101, zero rejected, images inspected (known black foliage).
Same assets/checkpoint/input; fastframe650..780 median83.504ms/29.974submit,
previousmodelviews100.334/38.431. Checked116.680ms is verification overhead,
NOT the performance baseline. Different animated workloads; no consoleFPSclaim.
ARM900..909 main164421081->141882884 across accumulatedchanges; comparator
32150504->14976305. Draw3510->3415,indices933858->884916: not exactworkloadequal.
4builds current, gamearchive76e625219826. Registry20KiB static, poolcapsunchanged.
New sampled normal+stack49.257/71.303MB,linear8.818/25.166MB,VRAM1.478/6.291MB.
InstalledNew4628/Old9793 untouched. User may not yet have tested4628; latest
FTP120249 is stale d78 log, NOT a new4628failure. Do not repeatlaunchquestions.
Next larger remainingwork: mutablemodel comparison/conversion, native command
submission and audio; no more unchanged texture replays. No physical30/60proof.
Previousgoalturn PROGRESS (retrieval clarification+archive); currentPROGRESS
(integrated/tested texture lifetime path). All owned jobs terminal; goalactive.
Evidence artifacts/research/native-texture-assets/audit.json, latestIMPLEMENTATION.

7 October local New 14cdb2c821ac / Old 45fecfff81d9; NOT DEPLOYED.
Added bounded independent model GPU views keyed by live source arrays. Pointer
identity selects storage only; exact source comparisons and mutation fences stay.
Same-binary exactNew firstFox on/off1100/Select1101 passes:1724 same-frame model
drains ->0; uploaded bytes160716978->156135080.17views/1547502extraGPUbytes, no
fallback. This removes overwrite serialization, NOT most real animation copies.
Frame650..780 median interval100.334ms on /100.260off; NO whole-frame speedup
established. Both hid1020 images inspected, known black foliage persists.
Host New/Old ASAN/UBSAN mutation/alias/all15attribute-mask/fallback/release tests
pass; deliberately removing write fence fails queued-reader byte oracle.
New extra-view cap2MiB,Old512KiB, within unchanged6MiB scene/linear/headroom caps.
Measured New normal+stack49.255/71.303MB,linear8.814/25.166MB,VRAM1.478/6.291MB.
All4builds current. No new Oldruntime or physical FPS/Pepper acceptance. Installed
New4628/Old9793 unchanged. Latest FTP112302 timed out; no retry this turn.
Prior goal turn PROGRESS (constant path); this turn PROGRESS (integrated storage
ownership). Goal remains incomplete. All owned jobs terminal. No unchanged rerun
needed for this path. Next: dominant remaining draw/texture preparation; fresh
physical logs when reachable. Source-switch cache alone is not a60/30FPS solution.
Evidence: artifacts/research/native-model-views/audit.json and IMPLEMENTATION.

7 October local New d0429612b212 / Old 503551623439; NOT DEPLOYED.
Constant model lighting now avoids normal transforms/inverses/normalization and
per-vertex lights when packed channels prove position/normal independence.
Sphere UV generation, directional/complex lighting keep full path. Same assets,
game archive and memory caps. On/off exact-console runs completed1100/Select1101,
19901/19814 GPU commands balanced; candidate26504draws/5255856indices eligible.
Same-binary frame650..780 median intervals100.250/100.086ms, submit38.555/38.723ms:
NO established whole-frame speedup. No physical FPS claim. Both images inspected;
known black foliage persists. Do not spend another cycle polishing this alone.
Initial8cb6 shader candidate overwrote normal blend weight; REJECTED, corrected
before final runs. Host math114688cases and12288rejections pass; packed lighting
4800cases pass. Shader branch/output/MOVA contracts pass; not hardware validation.
Read-only FTP collection20261007-112302 timed out before login; remote unchanged.
Pepper freeze UNRESOLVED; installedNew4628/Old9793 remain unchanged. Next: retrieve
fresh crash logs when reachable and address dominant model/draw CPU preparation.
No more cold Fox replays just to prove the already-tested constant path.
Evidence: artifacts/research/native-constant-light/audit.json; latest IMPLEMENTATION.

7 October local New d15b5a48a3b0 / Old c9b44505c77e; NOT DEPLOYED.
Callback-free model loops now reuse exact texture validation after animation;
callbacks remain unscoped, writes/palette changes invalidate, every next object
starts with full comparison. Constant-time validated scene-handle lookup added.
Same assets/checkpoint as below; game archive9912c80dadc6. Four builds current.
Checked live Fox run:49332 exact reuse comparisons, normal Fox control, clean
GPU shutdown. Actual-C texture tests New/Old and256-slot handle bounds pass.
ARM window900..909 main187227332->164421081 encounters (-12.18%), comparator
47558386->32150504; similar animated draw counts. Not hardware cycles/FPS.
ExactNew cold Fox/Select1101 clean,19712GPU commands balanced, zero rejected.
Matched old/current exact frame650..780 medians100.305/98.947ms; submit47.337/
38.156ms; draws354/344 and indices93435/88719 differ. NO substantial whole-frame
or physical speedup established. Do not present the12% instruction reduction
as12% console FPS. Still far short of30/60; Pepper freeze remains UNRESOLVED.
Installed New4628/Old9793 unchanged, pending reachable FTP for latest failure.
Do not ask for another unchanged console run. No assets/saves/RVZ changed.
Rejected experiment: world validation scope had ZERO reuse; removed. Do not
repeat that direction. Optional memory observer in checked runs caught process
teardown after its valid normalFox snapshot; guest shutdown passed, not an
observer pass. All owned runs terminal; exactconsole raw captures archived.
Next: larger rendering/vertex/submission architecture, not another extended
cache-only cycle. Physical measurements require the pending new crash logs.
Evidence: artifacts/research/native-fox-render/audit.json and latest IMPLEMENTATION.

7 October local New3593313a1b5c / Olda383dc7545ae; NOT DEPLOYED.
Current private texture pack is texture-pack-surface-fox-dir-128.bin under
assets/GSAP01-rev1/prepared/ (SHA760f3b09cceb92a03d98a29a9e15dce2dda4b1ab15dbbc4855c964437dc37671),
installed as3ds/foxhollow/texture-pack.bin. It includes firstFox/flight and raw
DIRn textures omitted before. Samev13 native-scene.bin, alpha-coverage topology
and prepared movie. Original assets/RVZ and existing console saves stay private.
Packages exclude assets. New exactbinary coldFox/Select test passed; normalFox
save/reload/movement previously verified in matching gamecode with finalassets.
Old is rebuilt/packaged but no new Oldruntime verification. No physicalFPS gain
or Pepper hang fix established. The larger linear cache candidatee4ef was slower;
final indexed lookup restores approximately the old emulator cadence, not30/60FPS.
InstalledNew4628/Old9793 remain unchanged. Preserve latest crash logs before any
replacement. No additional hardware playthrough is requested without capturing them.

7 October local candidates Newd7cc9cf64e0d / Oldae6d65b0d9a2; NOT DEPLOYED.
These include original background clear handling and all earlier source fixes.
Use private artifacts/research/minimal-opening-flight-v13/native-scene.bin as
3ds/foxhollow/native-scene.bin for restored worldmap/opening flight geometry;
same model-lod-alpha-coverage.bin, texture-pack-surface-128.bin and prepared
movie as before. Packages exclude private assets. Preserve installed saves.
New exact-binary headless worldmap/Select passed; Old built but this change
was not newly runtime-tested there. Flight launches normally; automated
flight survival/first Fox gameplay not passed. Physical Pepper hang remains
unresolved; this is NOT a promised hang fix or established FPS improvement.
InstalledNew4628/Old9793 remain unchanged; no new console run requested.

7 October update: user reports another freeze in the same Pepper cutscene.
The last candidate has NOT established hardware recovery. Fresh logs could
not be retrieved: both latest New FTP attempts timed out before login.
Last verified installed New4628/Old9793; no replacement or remote changes.
Preserve the new crash evidence before another launch. Local3cf16/56cb and
v12 are not deployed or validated as fixes for this repeated hang.

7 October local New3cf16b8552cc / Old56cbdcfe4d04 built; NOT DEPLOYED.
Installed New4628bb7f6389 / Old9793b9823cba unchanged; await physical Pepper
candidate result. No FTP poll/replacement during pending test.
Restored missing world-map asset set: v12 private native-scene.bin at
artifacts/research/minimal-opening-worldmap-v12/, SHA32ed57156235017e2515c68c348b193acbffa18327dd17ef575cd08ca2f0d7a6.
38 packets added,558 existing packets byte-identical,+505628bytes. Same topology/
textures/movie. Planet/islands now visible; missing counter stops growing.
Sky gaps/opaque hologram remain WRONG. Alternate local-joint object transform
also corrected;4800 actual-C cases pass, no extra GPU/RAM allocations. It does
not explain world-map omissions (that alternate model is rigid).
ExactNew1500/Select1501 clean,20422GPU commands balanced. Normal+reserved stack
47.379/71.303MB,linear7.064/25.166MB,VRAM1.469/6.291MB. Old rendered map but
TIMEOUT180s at1131 before Select; NOT passed/playable, do not repeat long test
just for Select. Complete Old rows normal+stack40.028/48.234MB,linear4.895/6.291MB.
All owned sessions terminal. Assets/saves remain private. Existing post-Pepper
checkpoint f16204fe2daf reloads directly into map; no Palace replay needed.
Next: sky/hologram, prepare Arwing route assets and normal flight/Fox progression,
plus dominant CPU costs. No physical FPS gain or Pepper hang recovery claim.
Evidence native-alt-pose/audit.json; latest IMPLEMENTATION has reproduction.

7 October local New696d771a3d57 / Old7867d2d59509 built; NOT DEPLOYED.
Installed New4628bb7f6389/Old9793b9823cba unchanged. Pepper hang recovery
on4628 still awaits user hardware result; do not poll/replace console mid-test.
Narrow audited world texture reuse is integrated: 13845 checked hits, zero
mismatches; exact console binary mode0/1/2 Palace2100 and Select2101 clean.
Matched room render submit15.214->14.900ms (~2.06% emulator CPU reduction),
texture compared bytes -3.80%. Small contribution, NOT a major/physical FPS gain;
do not spend another continuation polishing this same scope or request hardware
for it alone. Next work still needs broader CPU cost/progression evidence.
All four variants built, originalv11 assets retained, screenshots inspected.
Host disk filled during one test; verified lossless raw archives reclaimed space.
Successful run_console_headless runs now archive raw XWD duplicates automatically.
Sources/saves/private assets unchanged on SD. All owned sessions terminal.
See artifacts/research/native-world-texture-scope/{audit,comparison}.json.

7 October New4628bb7f6389 DEPLOYED/readback-verified atnew223:5000.
Transaction20261007-065141 complete,8runtime/settings files. Saves/private
assets unchanged; collected d78 Pepper evidence preserved, snapshots archived.
Candidate excludes actual-current collapsed meshes with no pixel-center coverage;
both repeated Pepper captures' model457 qualify, while prior broad bound kept
them.6000random/6guard/2capture cases and prior5000bounds cases pass. ExactNew
Palace2100/Select2101 clean;28469GPU commands balanced, image1440 inspected.
This removes one shared suspected trigger; hardware hang recovery remains TO
TEST, and no physical FPS gain is claimed. Continue the existing save to the
same Pepper briefing, Select if responsive, reopen ftpd; no routine reboot.
Old7f2353e335e0 built/packaged locally, installedOld9793 unchanged. Originalv11
geometry retained; world proxy lighting regression isolated and excluded.
All owned builds/emulators/FTP terminal. Goal60/steady30 and stable normal
progression remain incomplete. See latest IMPLEMENTATION/native-mesh-visibility.

7 October recurring Pepper freeze on diagnostic Newd78 CONFIRMED.
Read-only collection20261007-063619 complete; saves again copied20261007-064725
and unchanged. At retrace5468, GPU submitted89481/completed89480/pending1,
failed/unmatched/unsupported0; queue16entries,15completed,16submitted. No proven
submission rejection or unmatched IRQ. Shared outstanding batch contains
model457/group0:690indices/168vertices/8bones at tiny projected scale. Expected
shader probes (ordinary and two approximate reduced-precision modes) are finite;
this is NOT proof of physical GPU arithmetic or the exact hanging draw.

Local New4628bb7f6389 / Old7f2353e335e0 BUILT. Adds conservative actual-current-
mesh bounds for collapsed models that the old full-signed16-domain bound retains.
Both Pepper captures' model457 draws now fail pixel-center coverage with the
existing arithmetic margins and1/16pixel raster allowance. No size-only discard,
assetID exception, animation/callback skip or changed game state. Ordinary scale,
near-camera, pixel-center and nonfinite cases retain rendering.6000random blended
cases/6guards plus2physical captures pass; existing5000case bounds suite passes.
Exact New2100/Select2101 headless Palace clean;28469commands completed, no errors.
Image1440 inspected against prior original pack. Hardware hang recovery UNPROVEN.
No physical FPS claim. Pending explicit completed deployment record, installed
Newd78/Old9793 remain unchanged. All owned builds/tests/collection now terminal.
Previous goal turn PROGRESS: verified diagnostic deployment; this turn PROGRESS:
new physical capture narrows next action and integrated conservative culling.
Goal remains incomplete: Pepper stability,60/steady30 and full progression.

7 October user-requested diagnostic build d78e9461d06b DEPLOYED and
readback-verified to New 3DSnew223:5000. Transaction20261007-062609
complete:8runtime/settings files. Saves/private assets untouched. Previous
Pepper crash evidence collected, failure snapshots archived remotely.
This adds verified GPU submission/completion diagnostics; Pepper hang remains
UNRESOLVED and no FPS improvement is established. Exact console-binary headless
Palace2100/Select2101 passed,28439commands balanced, image inspected. The newly
linked observer passed the actual IRQ-call guard and host failure tests.
Short-run normal+stack47864024/71303168,linear6253056/25165824,
VRAM1473536/6291456,mainstack25916/524288; emulator evidence only.
Old8b49435b98d5 built/packaged locally; Old installed9793 unchanged.
User can launch normally/resume existing save; no mandated replay/reboot.
All owned build/emulator/transfer processes terminal. Target60/steady30,
Pepper stability and extended progression remain unmet. See native-gpu-work.

7 October GPU observer build READY for user-requested New 3DS deployment:
New d78e9461d06b; Old 8b49435b98d5 built locally. General Pepper hang remains
UNRESOLVED; this is diagnostic coverage, not a hang fix or an FPS gain.
Existing rendering/assets retained; reduced topology candidates remain excluded.
Exact New console binary passed isolated headless Palace save load, 2100 retraces,
Select at2101, clean shutdown in88.130s. Image1440 inspected. All28439 observed
GPU commands completed; pending/failed/unmatched/unsupported all0 across16
samples. This is not Pepper reproduction, physical stability or GPU timing proof.
All4 variants built/verified. Host overlap/failure controls pass with ASAN/UBSAN.
Build guard checks actual IRQ thread calls observer; ordinary --wrap did NOT
redirect libctru's local weak reference. Isolated pinned archive symbol adapter
fixes observation, without changing queue code or original SDK. Saves unchanged.
See artifacts/research/native-gpu-work. Deployment requires subsequent complete
transfer record; until then installed New4d04/Old9793 remain unchanged.

7 October New4d04d721ede6 DEPLOYED and readback-verified atnew223:5000.
Transaction20261007-041913 complete,8runtime/settings files installed. Saves
and private assets untouched; prior diagnostics collected and archived remotely.
Includes explicit water/world padding hardware fix plus ordered index gathering
and finer world bounds. Both prior hang captures contain the bad water layout
at the start of the first unfinished list. Offline actual-source regression,
32water pixel checks and exactNew2100-retrace Palace run pass, queue balanced.
Physical camera-at-door/central-room freeze recovery remains TO TEST; no console
FPS gain claimed. Launch normally, view the prior failure area, Select if still
responsive, then reopen ftpd. No reboot needed unless recovering from a hang.
Oldcdf35e4f5c17 package ready locally; installed Old9793 unchanged. Target60FPS/
steady30 and full playability remain unmet. All owned emulators/builds/FTP ended.

7 October native vertex-padding fix READY LOCAL: New4d04d721ede6,
Oldcdf35e4f5c17; all four variants rebuilt and verified.
Physical12a capture queue interpretation CORRECTED: lastEntry15 counts completed
entries; curEntry16 is first unsubmitted. Entry15 is submitted/unfinished and
starts with water draws98/99:20-byte stride but only16-byte fetch extent.
Earlier decab hang entry0 starts with the SAME mismatch. PICA requires explicit
padding for unused trailing fields. Water packed/float and world float bindings
now describe the full stride, following pinned Melee's padding mechanism.
Four actual source layouts pass;3old-layout negative controls and both physical
captures are rejected. GPU fixture passes32pixel checks for4water draws, with
reflection/alpha/caustics; exactNew Palace2100/Select2101 clean, image inspected.
Integrated route has0water draws, so fixture coverage is explicitly separate.
Includes ordered index gathering/finer world bounds from prior offline tests.
No new cost/allocation for padding; gathering uses optional64KiB linear stream.
Physical freeze recovery and FPS gain are UNPROVEN. Last physical mixed-window
25.967FPS remains below30floor/60target. Installed New12a/Old9793 unchanged until
later complete deployment record. No save/asset changes; no further old-build
test needed. Evidence artifacts/research/native-vertex-padding/ and completed
console-native-vertex-padding-palace. This supersedes batch16 fault speculation.

7 October current: physical12a82a5b76b4 freeze RECORDED at retrace3439.
User corrected trigger: camera looking at Palace door/big central room, NOT
switch activation. Read-only collection20261007-035441 COMPLETE (16 files),
including verified24MiB CPU input capture842343b5fe82. Installed New12a and
Old9793 unchanged; no saves/assets changed. Queue18/last15/current16; batch16
contains draws104..111.117draws decoded;40985float inputs, uniforms and observed
fixed attributes finite. Approximate clip transforms plausible; exact failing
draw, shader execution, texture/coherency and hardware cause remain UNPROVEN.
Physical persisted1800frames25.967FPS; last300=27.079FPS (not matched speedup,
not final freeze frames). Target60/steady30 floor and hang-free play NOT met.

Local ordered-world-gather implementation built/verified: New6e9dcbd2a38d,
Oldd079bb315895, emulator49f9c25f58a6/55c031d3dd25. NOT deployed/packaged.
Four-triangle bounds + optional64KiB immutable linear index stream preserve
triangle/material order and merge visible spans to one draw. Normal metadata
uses existing admission budget; no extra VRAM.100000publication/20000retirement
host cases,100000corner checks,1700command capacity and266GPU wait cases pass.
ExactNew stationary Palace1500/Select1501 clean; matched frame1000..1189
indices3407460->3181740,draws11778->8929; screenshots1440 inspected, same scene
with animation/rain differences. Work counts, not a physical FPS forecast.
Candidate8359gathers,0capacityfallbacks,10000byte stream peak,65536reserved.
New normal+stack47905496/71303168,linear5860736/25165824,VRAM1477632/6291456.
ExactOld startup350/Select351 clean19.447s; no Old Palace acceptance claim.
All owned builds/emulators/FTP terminal. No further unfixed console run needed.
Next diagnosis uses captured late batch; preserve useful cumulative render work.
This status supersedes earlier awaiting-test/local-only deployment descriptions.

7 October guarded world-culling New build DEPLOYED and readback verified:
12a82a5b76b4 atnew223:5000. Transaction20261007-034229 complete;
eight runtime/settings files verified, saves and private assets untouched.
Includes shared shaders, early GPU submission, conservative world chunk
culling with per-span capacity guard, and failure-only GPU input capture.
Physical FPS gain and Palace door-switch hang fix remain UNPROVEN.
Exact-New startup passed; prior unchanged culling Palace evidence retained
with moving-camera comparison limitation. Ready for one physical measurement.
Old remains9793b9823cba; guarded8c124ca32e1c package ready locally only.
No emulator/build/uploader remains running when this completed record is made.
Goal60FPS/steady30 minimum remains incomplete; modest gains retained alongside
larger renderer work. This status supersedes earlier local-only entries.

7 October guarded world-culling candidate READY LOCAL: New12a82a5b76b4,
Old8c124ca32e1c, emulator886d9a72bdf1/05fd8ef1eea7. Matrix rebuilt/verified.
1700actual span-loop capacity cases pass; old unguarded loop fails negative
control. ExactNew350/Select351 startup19.054s clean, image330 inspected.
Earlier f511Palace/17aOldstartup evidence reused for unchanged visibility work;
only per-span capacity retirement added. Do not use old f511package.
033309installation cancelled before commit, partial staging file only;
installed Newdecab/Old9793 until a later complete transfer record says otherwise.
All emulators/builds terminal. No physical FPS gain or Palace hang fix claimed.
User correction retained: useful small gains accumulate alongside larger work.

7 October local performance candidate f5116b9c4c48 (New),17a8375f3863 (Old).
Includes shared PICA shader code, earlier GPU submission, static world chunk
culling, and failure-only CPU input capture. Modest gains are retained as
cumulative work; physical performance improvement is NOT yet established.
New exact-binary Palace and Old startup runs exit cleanly in headless Azahar.
Stationary Palace view checked; later moving views differ in camera position,
so full moving-image equivalence remains unverified. Door-switch GPU hang
is unresolved. Failure-only capture may add SD time after a3second GPU stall.
Original scenev11/assets/saves are retained; no new asset conversion required.
If installed for hardware comparison, use the same Palace rooftop save and
stationary camera for about30seconds before moving, then Select and ftpd.
No reboot required. Installation status requires the latest transfer record;
a local package does not mean either console was updated.

Old 3DS control candidate 9793b9823cba: D-pad sends C-stick directions for the
items menu. Hold the touchscreen while using D-pad to send the original D-pad
instead. Existing Circle Pad, ABXY/L/R/Start/Select controls are unchanged.
New 3DS binaries are unchanged. This is a controls fix, not a performance gain.
Deployment status is recorded in latest IMPLEMENTATION; do not assume installed
from this candidate documentation alone.

7 October candidate decab42518c1 (New) / 558d2c80b3ca (Old): nonfatal native
GPU wait. At three seconds record the queue once and continue waiting with GPU
resources retained. Select during the wait is deferred until actual completion
and normal frame input; permanently stuck GPU still cannot safely recover/exit.
This is NOT a proven Palace hardware hang fix or an FPS improvement. Test-only
4second completion-delay binary is excluded from these packages. Includes
locally checked native world foreground transparency and point-light adaptation;
physical visuals and natural world-pointlight coverage remain unverified.
Assets/saves are unchanged. Latest IMPLEMENTATION has exact evidence.

7 October candidate4c2e2bc5b014 + private scenev11 fd2af3aa6952.
This tests a replacement for the22bone model draw that stalled physicalbc9:
smaller groups use precomputed normal matrices; original triangles preserved.
It is a startup compatibility candidate, NOT a confirmed crash fix/FPS build.
Launch normally once (no reboot prerequisite), press Start if opening appears,
then Select and reopen ftpd. If it crashes, recover and reopen ftpd.
No long run needed. Saves are unchanged. Old90934841fede remains experimental.

7 October candidate bc9afc3da5af: STARTUP DRAW DIAGNOSTIC, NOT AN FPS BUILD.
The preceding 9ec run confirms a rendering command-list timeout before screen
presentation. This build serializes only the first3 rendered frames and records
each completed draw to identify the failing component. All original game work
and draws remain; newer world point-light/fade additions are excluded. The crash
is not yet declared fixed. Launch normally once. If it reaches the opening,
press Select; if it crashes, recover. Reopen ftpd for the log/queue snapshot.
No reboot prerequisite or long performance run. Saves/private assets unchanged.
Old3DS build9d4f9704b6db is experimental and not installed on .229.

7 October candidate 9ecbdff1b01c: targeted GPU-timeout diagnosis following
bd92 startup failure. Crash fix is NOT confirmed. Launch normally once; if it
fails, recover and reopen ftpd so native.log, panic.txt and gpu-timeout.bin can
be collected. No long FPS test is needed. Save files remain untouched. The
snapshot stays private. Old build5b281c905837 remains experimental.

7 October current builds: New932b90811cbc/5f15d8ce549a, Olde47a703f21fb/6ed3bc3a24cc.
Four targets verified, same game96fda3ef. Full normal helper fixed3x3 expressions:
ARM11 eleven-bone4274->1787instructions;12000differential cases/ARM hashes exact.
GPU12cases/36captures pass byte-identical; shader367 unchanged. No full game
boot repeated: prior New/Old Palace evidence reused for unchanged paths, not
new exact-build integrated acceptance. Own group stack1648B,512KiBreserve unchanged.
Keep v10 default. v11 groups+25.49%, normal uniform rows3.44x, helper work3.31x
(static census), extra GPU coverage unproven as a whole-frame win. No FPS claim.
Records artifacts/research/native-normal-cost. No more normal-helper microtuning
without new evidence; return to integrated progression/visual/performance gaps.
NoFTP: New.223 pending1d/v6, do not poll/overwrite; Old.229 empty/notready.
Keep four builds current; physical60/steady30 and full playable opening unresolved.

7 October current paired-normal builds: New console d7685c808c5e/emulator955773597f2a;
Old console ea658d4ccc0b/emulatorc3e79adf31c9. Four targets verified, game96fda3ef unchanged.
Full CPU inverse normals per bone for <=11 bones, existing66uniform rows/16booleans;
GPU inverse fallback retained. GPU12cases byte-identical; host3000/ABI4800 pass.
Candidate private v11 fd2af3aa6952 regroups models; all triangles/draws preserved,
world205 packets identical; more submissions/5% more stored model vertices.
Exact New/Old console Palace Select2101 clean83.675/91.752hostseconds. Every lit
model draw admitted; images inspected, all owned runs terminal/raw archived.
New normal+stack47902512,linear6172032; Old40392144,linear5188992,VRAM1477632.
Old linear headroom1102464B only53888B above guard. Both remain unvalidated on
physical hardware. Cameras/work differ: NO FPS gain claim; extra submissions
need controlled assessment. v10 remains compatible; v11 candidate, not default.
Records artifacts/research/native-normal-palette; latest IMPLEMENTATION section.
No new progression/save/Shrine. New.223 pending1d/v6 untouched: do not poll or
replace pending test. Old.229 empty/not ready. Keep four targets current;
full playable opening and physical60target/steady30minimum remain incomplete.

7 October current native skinning builds: New console0c8250217ed1/emulator2a386475288f;
Old consolee323d3b4cf11/emulator4bbde1450a9a. All four targets built sequentially
and verified; game96fda3ef0983 and privatev10/alphaLOD/surface128/movie unchanged.
Matching bones transform once; sphere generation skips unused inverse normals.
Full distinct-bone/nonuniform/shear lighting remains. GPU fixture ten cases
byte-identical baseline/current; independent oracle <=0.501/255. Shader357ins,
five outputs, unchanged ABI/storage. Source-prefix counts69->44 shared bones,
66->19 sphere,69->73 distinct bones; NOT physical cycles or FPS.
Exact New console Palace76.077s/Select2101 clean; image inspected,67raw archived.
Lit references20,241,165; single-bone subset2,289,144/18,670draws; sphereupdates3366.
Normal+stack47,898,136,linear6,126,592,VRAM1,477,632,stack25,668; budgets pass.
No new wall/save/Shrine or natural water-effect coverage. Old current compile
validated, previous Old Palace evidence reused for unchanged platform paths.
Records artifacts/research/native-shared-bone. Use build_native_matrix.py;
regenerate minimal.v.pica after generator edits (build guard rejects stale code).
No FTP: New.223 pending1d/v6 report; do not poll/overwrite pending test.
Old.229 still empty/not ready. Keep separate Old builds experimental. Physical
60target/steady30minimum and full progression remain unproven; goal incomplete.

7 October native splash rings: Newconsole571631170347/emud75193a09e49;
Oldconsolee06d9d1c9067/emu80809b8b9504; game96fda3ef0983. Fourtargets rebuilt
sequentially/verified. Original8band motion/lifetime/210triangles retained, two
cull passes; one reflected image replaces3distorted samples, original weights/
foam12xU/mask/alpha preserved. Shader25instructions/6outputs; no newpersistent
GPU allocation. Linked splash stack31,040B within512KiB reservation; active
whole-call-chain measurement missing. Droplet points still missing.
Host630vertices/ASan and simulation-tail identity pass; prior36ripple adapter
checks pass. GPU19441864spatial checks passes; failed194302foam expectation
retained (test U basis wrong, same production binary passes). Images inspected.
Exact Newconsole Palace74.504s/Select2101 clean, hid1800visible,67rawarchived.
SPLASH/WATER/EFFECT counters0: no natural feature coverage or FPSgainclaim.
Newnormal+stack47906992,linear6183936,VRAM1477632,stack25668. Oldnewcompileonly,
priorboundedPalace reused for unchangedpaths. Records native-splash. Previously
collected console save79ede57 is new-file/playTime0, no latercheckpoint.
Privatev10/alphaLOD/surface128/movie unchanged. NoFTP: New.223pending1d/v6,
Old.229empty/notready. Next normal effect/progression coverage, remainingnative
materials/droplets and whole-frameperformance. Goal incomplete; physical60/30
stillunproven. Use build_native_matrix.py for native changes.

Latest7October native ripples/wakes: Newconsole d1d602d2df4b/emu796ca10202af;
Oldconsole095c669d81d1/emu7aa06c961523. Game8acf6671175c; all4built sequentially.
Originalengine19simulation/geometry/lifetime/masks retained; native single
reflection replaces2distorted samples; BUMPN quantization approximate(max7/255).
Existing21instructionshader/60KiBstream, no extra pool/VRAM. Splashbands/drops
stillmissing. GPU20261006-18532248spatialchecks pass, mask/tint/fog/lifetime,
world9/11inspected; hostASan36vertices+badindex pass, simulationunchanged.
NormalPalace southern/fullstickattempt106.620s clean cancellation at2832;
door/pickup/southerntraverse pass, barrel lost atenemyencounter near12228,369,
oneHagabonhealth0 observed. NOwall/newsave/shrine; next handlecombat, notroute
microtweaks. Automaticallystop failedcontrollers; no idledeathmenuruns.
Newnormal+stack48096912,linear6034560,VRAM1477632,stack26620. Watereffects0 AND
water0: fixturecoverage only, notgameplayacceptance. Records native-water-effects.
Allownedprocesses terminal/rawarchived. Oldcurrentcompile, priorPalace reused.
Privatev10/alphaLOD/surface128/movie unchanged; surface64experimental only.
NoFTP: .2231d/v6pendingreport, .229empty/notready. No poll/overwritependingtest.
Goalactive, physical60target/30minimum/fullprogression unresolved.

Latest 7 October: native GPU reflection and reduced-detail deferred map water.
New console530d2701ca25/emulator513bd71e7e6f; Old consolef7e66a964d63/
emulatorf9120cbd2772. Gamearchivec944b4415100; private v10/alphaLOD/surface128/
movie assets unchanged. Padded texture-backed framebuffer retains400x240 output;
128x128RGB565 reflection, no CPU readback. Measured extraVRAM697344B/linear240B.
21instruction water shader preserves geometry/vertexalpha/tint/animation; one
projective reflection+caustic shimmer replaces two indirect distortions.
Engine19 dynamic water remains missing. GPU fixture181409:32spatial checks,
unequal W, transparency, heavyfog tint and caustic modulation pass. Surface
180405 crop and180514 mirror failed and retained;180637 corrected/inspected.
Host worldmovie16cases+negative pass. Exact New/Old Palace Select2101 clean,
81.524/90.668hostseconds; reflectioncopies1488/1489 BUT waterdraws0 on both.
Do not claim in-game water coverage. hid1800both inspected; floor/clouds/rain/HUD
visible, different camera positions. Worldmovie209/208 now exercised.
New normal+stack47894208,linear6097920; Old40371248,linear5239296; VRAM1477632
both. Old linear reserve1052160 (only3584above1MiB), region0spare489152. SD full
records native-water/integration.json; uploadsNew858/Old2040. No FPSgainclaim.
All owned processes terminal, raw captures archived/readback verified.
No FTP: New .223still1d/v6 pending report, Old .229empty/notready. Do not poll
or overwrite pending console test. Keep4targets current. Next: actual water
route coverage/normal progression, engine19 water/other essential materials,
Old texture residency and physical60target/steady30minimum. Goal incomplete.

Latest 7 October: fixed packed-world movie routing before deployment. World
callbacks now use the20byte world fetch, not40byte character fetch. Newconsole
be67118740ab/emulator3f0c8b211dfa; Oldconsole9b9761ad626d/emulatorc42783425d05.
Game2de61dc5a47e and private v10/alphaLOD/surface128/movie assets unchanged.
Host actual-C16cases +removed-route negative pass. GPU fixture175002 uses actual
world submission: packed/float RGB identical,16spatial checks,2world-movie draws.
Earlier raw fixture captures174153/174242/174653 striped: failed_visual_validation;
174839 stale initial frontend failed. Final uses distinct presented-frame markers,
no GPU readback. No new readback in production. Short New/Old console runsSelect601
27.263/29.351s clean; ordinary movie visible, BUT world-movie counter0, so these
are not fixed-branch coverage. Prior normal Palace evidence reused. All owned
processes terminal, game raw captures archived. Records world-movie/build-matrix,
integration and IMPLEMENTATIONtail. Prior6a52/74c2 packages superseded, not deployed.
No FTP: .223still1d/v6 pending report; .229empty/notready. Keep4builds current.
Next: deferred water/material restoration and full progression. Water stream2 is
lightmap_draw.c mapBlockRenderWater -> setupWaterCausticTev; engine19's water is
separate setupWaterReflectionTev. Do not just shade these as generic opaque meshes.
No physical FPSgain/console acceptance/full progression; goal remains active.

Latest7October packed world: four current builds Newconsole6a52a1379dc6/
emulator177adbb3fad5; Oldconsole74c2f43bc81c/emulator61c7864d5228. Game2de61dc5a47e.
Static world64->20bytes/vertex, unchanged shader; private v10scene f7677f1cc940
(12,660,120B) requires alphaLOD82412755d509. Surface128texture57ddd5296e3c unchanged.
205world packets/225666vertices/579378indices exact;353model packets byte-identical.
Reader558/9500stream checks,1435corruption rejects pass. Old exactPalace Select2101/
80.029s clean; same-binary v9control90.696s. Uploads2216->851, SD10.942->9.871s;
resident scene GPU1,763,564->1,121,692B. Earlier11081upload comparison NOTisolatedgain.
Different late draw/index counts and camera offsets; no FPSspeedup/physicalclaim.
New exactPalace Select2101/73.603s clean; floor/rain/clouds/HUD inspected. Oldnormal
+stack40,378,784/48,234,496;linear5,236,480/6,291,456;region0spare489152. Newnormal
+stack47,874,576;linear6,057,728;bothVRAM780288. Allprocesses terminal/rawarchived.
Records compact-world/build-matrix.json,integration.json and IMPLEMENTATIONtail.
Old .229empty/notready; New .223still1d/v6 pendinguserreport, noFTPpoll/overwrite.
Goalactive: remaining native materials/essentialeffects, fullprogression and
physical60target/30minimum unresolved. Keep separate4builds current. Sevenold
instruction-overflow assets have0streams/lists/shaders; fivewarp/decaldraw omissions
remain. Do not pair new private scene with installed1d; explicit native mode only.

Latest7October native distance fog: Newconsole006b505b0e74/emulator51bb73daef0b;
Oldconsolea30677ba7e56/emulator774fbad50d21 all compiled, game2de61dc5a47e.
Actual PICA512byte CPU fogLUT, no added shader/linear/VRAM work. Generated
GXSetFog native-mode discard fixed; negativecontrol rejects former behavior.
Host2560curve checks/cache330joins pass; GPU28RGBA checks pass at
20261006-165923, rawdepthnegative passes170156. First console-native-fog-palace
4e699 run hadZERO fogdraws: keep failed featurecoverage, not fogvalidation.
Corrected console-native-fog-wired-palace Select2101/76.037s clean;fog67095draws,
2rebuilds/0reject. hid1800inspected, Palace floor/rain/HUD visible. Samplednormal
+stack47,889,056;linear6,764,160;VRAM780288;mainstack25668. Allruns terminal/raw
archived. See IMPLEMENTATIONtail and native-fog/build-matrix.json. Old current
compile-only; prior Palace/residency evidence reused. Old .229empty/notready.
New .223still1d/v6 awaitinguserreport; noFTPpoll/upload. Private v9scene+alphaLOD+
surface128texture unchanged. Heavyfog/particlefog/rangeadjustment, water/lighting,
physicalFPS/fullprogression remain open. Goalactive; continue substantive native
material restoration/wholeframe work and Oldresidency, keep4builds current.

Latest7October surface variants: Newconsolec30a34eb7800/emulator60b486687284;
Oldconsole78ac16ddc582/emulatord9d47f4e0040. All4compiled; game2de61dc5a47e.
Private optional texturev2 57ddd5296e3c (277entries) halves eligible opaque
GPUstorage; fullresHUD/alpha retained. Scenev9adcbe1b8d3d2 +LOD82412755d509
unchanged requiredpair. Reader1583ASan checks, native cache330joins pass.
Old exactPalace Select2101/130.748s now passes140bound;uploads11081/SD38.132s
still excessive, not playable/.229notready. New exactPalace2101/73.989s clean;
linear6,821,504,normal+stack47,909,928. No physicalFPSgain/shrineacceptance.
New/Oldhid1800inspected; floors/rain/HUD visible, visual gaps remain. Fullmetrics
IMPLEMENTATIONtail +surface-texture-budget/integration.json/build-matrix.json.
Allruns terminal/rawarchived; noFTP. .223still1d/v6 awaitinguserreport; do not
poll/overwrite. Old .229empty. Melee VRAMpromotion rejected: pinnedhardware
pathdisabledafterGPUhangs. Next: wholeframe/remainingessentialvisuals, Old
working-set pressure. Preserve alpha/fullsourcechecks and keep4targets current.

Latest local candidate6October: New3DS8b44eb64a793, same private scenev8
8984522138d4dddbde220589cdd3b554a3c6379d837b45edb60c3d6f0ee3bfa1.
Rain now visibly renders in opening/Palace; original object color fades added.
Exactconsole saved Palace movement/Select passes. Black sky, lighting and other
material gaps remain; physical performance and shrine acceptance unverified.
Old0fa2e9871f49 compiles only, startup RAM failure unresolved, .229 NOT ready.
No deployment. New .223 remains1d/v6 awaiting user report. Older notes below
are historical local candidates; preserve saves and original assets.

Latest local candidate6October: New3DS1e7d686e3d81, same private scenev8
8984522138d4dddbde220589cdd3b554a3c6379d837b45edb60c3d6f0ee3bfa1.
Adds original single-UV model animation/scrolling/material opacity. Actual
console binary passes headless saved Palace movement/Select; separate ordinary
new-game opening test passes. Captures still show sky/lighting defects; no
physical FPS or shrine acceptance. Oldbd4bb099b8cf compiles only, startup RAM
failure unresolved, .229 NOT ready. New .223 remains1d/v6 awaiting user report.
No deployment in this continuation. Older local candidate notes below are history.

Latest local candidate6October: New3DSad86c1f71965, requires private scenev8
8984522138d4dddbde220589cdd3b554a3c6379d837b45edb60c3d6f0ee3bfa1.
Adds deferred transparent world draws in original queue order. Exactconsole
headless saved Palace/Select passes,821deferred draws. Water/lighting/sky and
individual visual equivalence remain unresolved. Not deployed; no physicalFPS
claim. Oldee456b04fc3d compiles only; startup RAM remains unresolved. Installed
New .223 remains1d/v6; Old .229 empty. Older candidate notes below are history.

Latest local candidate6October: New3DS1cbbb3fac2cb, same private scenev7.
Adds native ordered particle billboard batches; exact-console headless Palace
load/movement/Select passes. Individual effects still need visual comparison;
world/cloud/light/fog/distortion coverage remains incomplete. Not deployed;
physical performance unverified. Old6d16b4af8c2b compiles but startup RAM remains
unresolved. Both targets have independent manifests; never install New on Old.
The following3c7 note is historical local evidence, not current deployment.

Current local candidate6October: New3DS3c7f6fcde19f with private scenev7
6f2ef629fb53e732349e6083e4130b670d6ee5581ed13a357ed71026f69f53e5.
Restores common twoUV character layers and shared audio-bank metadata savings.
Exact console binary passed headless saved Palace load/Select; eyes/face visible
in separate opening capture. Lighting/sky/effect gaps remain. No physical FPS
claim or shrine acceptance. Candidate NOT deployed; installed .223 remains1d/v6.
Do not put this scene on the older executable. Old3DSa0c4b2a87280 only compiles;
startup RAM failure remains unresolved and .229 is not ready. Preserve saves.

# New 3DS console validation

Local follow-up89e11ddd192c is built/packaged but **not deployed**. It shares
the55SFX groups' converted metadata and uses compact validation bookkeeping.
Exact-console headless Palace load/movement/Select passes; peak normal heap
falls56,192,344 ->47,849,264bytes. This is a memory result, not physical FPS
validation. Package: build/console-validation/foxhollow-validation-89e11ddd192c.zip.
Keep the installed candidate below until the pending shrine test is reported.
Old3DS d743fcee10f8 also compiles, but startup still fails the40MiB normal-heap
budget in its headless check. Original3DS .229 remains uninstalled/not ready.

## Current candidate 1d9794cc1261: world materials and shrine assets

For New 3DS atnew223:5000. Requires scene packagev6
SHA b55e2d28c22e129e6768011855adf737c50bd58f3a0e4c5b59aa758ae4df0de1,
20,273,136bytes,550payloads. It now includes all six shrine banks, absent from
the previously installed package, and native two-UV world materials with
scrolling/animation. Existing private disc/movie/topology/texture files remain.
World shader:20/512instructions,11/96float registers,4ordinary attributes.
Runtime scene cache caps unchanged. Shader startup/movie hang fixes retained.

Exact console binary completed2101retraces in isolated silent Azahar, ordinary
Palace movement and Select exit,1071layered world draws/0world failures.
Inspected hid-1800. This is NOT shrine visual acceptance or physical FPS proof.
Your latest backed-up save reloads the opening; a shrine checkpoint was not
available. Essential model/material/weather gaps and performance work remain.

Launch normally, no reboot required. The useful hardware check is whether the
formerly blank shrine now has navigable geometry. Select exits and flushes logs;
reopen ftpd on the New3DS afterward. No saves are replaced by installation.

Old3DS atold229:5000 is a separate new target, NOT ready for this build.
Separate console-o3ds/emulator-o3ds binaries now compile, but the first bounded
Old3DS emulator test fails allocating ARAM pages. No Foxhollow files have been
installed on that console. Do not treat its emulator's reported96MiB process
limit as proof of the physical launch entitlement or increase heaps to fit it.
The older validation entries below are historical.

## Movie-read hang repair fe04c6b97408 (6 October)

This supersedes the earlier pending startup test: de277b89ff97 did display the
movie on hardware, then panicked on a background read after468retraces.
The replacement returns BUSY while movie SD reads are pending, keeps completion
on main, and removes the fatal three-second latency deadline. Its actual-C
regression survives an injected six-second read and validates cancellation and
bytes. The exact console binary passes1200retraces of headless movie/menu and
Select exit; screenshots inspected. Hardware confirmation is still needed.

Launch normally; no reboot required. Let the cockpit movie play past the
previous hang for about90seconds, then press Start and check the menu. Press
Select to exit and reopen ftpd. Report a persistent hang, crash, long pauses or
audio failure. SD stalls may still pause playback; this change does not make
SD faster. Saves and prepared assets remain unchanged. This is an incomplete
native renderer development build, not a playable or60FPS release. Existing
lighting/sky/effect omissions remain; unfinished sphere generation is excluded.


## Startup repair candidate de277b89ff97 (6 October)

The preceding installed8ca9d257a673 failed physically before the first image:
Minimal renderer GPU timeout, after the first two UI draws. Evidence is in
artifacts/console-test/20261006-110937. Do not treat its emulator pass as
hardware success. This candidate separates UI into an11-instruction shader
with three ordinary attributes, initializes all fixed model attributes, and
fills all output-register lanes following pinned Melee's shader convention.
These remove uninitialized GPU state; the exact physical cause is unproven.

The exact console binary passes a600-retrace isolated headless normal-HID
movie/Start/cockpit/Select run in29.16host seconds, with inspected images and
drained shutdown (native/console-minimal-startup-fix). It includes the ordered
material/movie restoration and asynchronous native CSV logging. Sphere-map
sampling is preliminary; generated lighting maps and essential effects remain
incomplete. Neither a hardware crash fix nor a playable FPS gain is confirmed.

Use the same private native scene, movie and other assets/settings listed
below. Saves must be preserved. For this hardware check, launch normally and
confirm whether the first image appears; if it does, press Start, leave the
cockpit visible for about20seconds, press Select, then reopen ftpd. No long
playthrough is needed to answer this startup question. Latest installation
status is in artifacts/console-test/*/transfer.json.

## Native PICA reconstruction candidate 8ca9d257a673 (6 October)

Installed and readback-verified atnew223:5000; record
artifacts/console-test/20261006-103559/transfer.json. Saves preserved.
User-requested hardware test of the renderer rewrite; not a playable release.
Includes the pinned Melee fixed-attribute hardware mapping fix (slots8..11).
The staged66b3d53ce35f upload was cancelled before any installed file changed.
This replaces the generic GX rendering path with native 400x240 geometry,
base textures, UI and GPU vertex lighting. Original game simulation, controls
and saves remain. Sky, two-UV materials, projected lighting, fur and several
essential effects are still incomplete. The resident package covers the tested
opening banks, not validated Palace/Fox exploration coverage. Do not use this
candidate as evidence that missing effects or progression have been accepted.

Requires the matching private native-scene.bin (SHA89196fd43b2b01cc04ce0245af0e9dbb803f28d432e63e703ecb6667503b77b8),
model-lod-world-seams-joints.bin, texture-pack-opening.bin, and the verified
starfox-pica-half-lz4.thp installed as starfox-pica.thp. Settings:
render_disabled.txt=2, audio_pipeline.txt=1, movie_prefetch.txt=1.
The prepared files supplement disc.iso; ARM game code runs from the .3dsx.
The original RVZ/disc and console saves are preserved.

Launch normally from Homebrew Launcher, with no reboot required. Press Start
for the cockpit menu. Check responsiveness, lighting, audio and any long pauses;
Select exits and flushes the buffered measurement logs. Reopen ftpd afterward.
The current opening evidence is emulator-only and still about33.4ms/frame in
late windows. It does NOT establish30/60FPS or hardware correctness. Latest
installation/readback status is recorded separately in artifacts/console-test.


## Local candidate: geometry key storage 4772572ccda4

Not installed. The SD card remains on68a491febe29 with reflection_lod1.
This candidate stores each variable geometry key with its cached GPU payload
inside the existing6MiB linear cache budget and4MiB linear reserve. It removes
a separate normal-heap allocation that can prevent new cached geometry near
the ordinary-RAM reserve. It does not increase memory pools or relax source
validation, GPU fences, draw ordering or the raw-depth rejection regression.

Allocator ownership/alignment/budget/failure tests pass with ASan/UBSan.
Headless testing of this actual console binary loads the ordinary Palace
checkpoint and exits via Select:2234/2234 GPU queues completed, zero log errors,
58.843s; image630 inspected. Evidence: native/console-linear-geometry-key.
Sampled normal+stack59954712/71303168,linear17099296/25165824,
VRAM4971520/6291456,main stack84872/524288. The corresponding instrumented
build completes the outdoor EarthWalker dialogue with console-matched private
assets; normal/linear/VRAM headroom there is15.61%/23.87%/20.98%.

No new physical performance evidence or matched-scene speedup is established.
The outdoor NPC dialogue is not the reported central-room cutscene. An earlier
diagnostic used a different hybrid movie asset and must not be used as a
deployed-build timing or memory baseline. No30/60FPS or playable-release claim.

## Single-pass reflections: configuration for 68a491febe29

The new validation package enables `reflection_lod.txt=1`. The executable is
byte-identical to68a491febe29 below; this is a runtime configuration change.
Installed and readback-verified: `artifacts/console-test/20261005-215335/transfer.json`.
Package `foxhollow-validation-68a491febe29-cda33319d07f.zip` retains the same
executable and enables the setting. Assets and saves are unchanged.

A captured lower-courtyard floor batch uses1GPU draw/264vertices instead of
275draws/825vertices. Eight repetitions take43.008ms versus179.537ms in the
emulator, including initial texture preparation. These are isolated replay
times, not console timings or full-scene FPS estimates. Both reflection layers
and their live colors remain, but their spatial detail and brightness change.
Captured geometry coverage and alpha match exactly; maximum RGB difference is
67/255. The brighter floor remains readable in the inspected image. Wider
visual/gameplay validation and physical performance remain unproven.

Actual console binary with the setting passes the normal-HID exterior load
and exit (2227/2227GPU completions, zero log errors,59.232s). That exterior
contains no qualifying reflections; the captured floor supplies the direct
effect evidence. Evidence: draw-replay/20261005-214400 and214801,
native/console-reflection-native-detail. Set the file to0 to restore full
reflection detail. This remains a development candidate, not a playable release.

## Native effect detail candidate: 68a491febe29

Installed and readback-verified atnew223:5000; transfer record
artifacts/console-test/20261005-212107/transfer.json. Previous4138788f4af8
binary/configs have dated backups. Assets and saves are unchanged.
This includes the strided framebuffer work below and reduces subdivision of
recognized, linearly filtered chained effects from a 1/32 to a 1/8 texture-texel
coordinate bound. Effect geometry, transparency, timing and gameplay remain.
Preparation batches reuse the existing 384-vertex scratch space. No new pool,
shader, texture pack or save format is required.

Three private captured draws pass the unchanged strict image checks against
full software shading: exact visible coverage, maximum RGBA differences of
(4,4,2,1), (4,3,3,1), and (2,3,3,1). Generated vertices fall from
3858/5715/2130 to984/1374/561. The earlier one-pixel failure is absent in the
tested capture; this does not prove subdivision seams impossible elsewhere.
These captures are NOT the central-room EarthWalker cutscene.

Exact console binary passed normal-HID Palace exterior loading and Select
exit in headless Azahar:2236/2236GPU completions, zero log errors,58.398s.
Evidence: artifacts/native/console-chained-native-detail, plus draw-replay
20261005-211648,211748,211913. Isolated emulator draw times improve, but no
whole-scene or physical FPS gain is established. This is not a playable release.
Existing assets and saves should be retained; normal launch needs no reboot.

## Strided framebuffer candidate: 4138788f4af8

Installed and readback-verified atnew223:5000. Transfer record:
`artifacts/console-test/20261005-204638/transfer.json`. Previous9e562 binary and
metadata/configs have dated backups. Saves and private assets were untouched.

This package adds direct CPU conversion from the padded EFB readback. It
removes421344bytes of redundant row copying and a cache flush per acquisition,
while retaining GPU completion and cache invalidation. It includes the earlier
chained-coordinate GPU changes and their known one-pixel artifact described
below. Existing saves and asset packs are compatible and should be retained.

Exact console binary passed an isolated normal-HID Palace exterior load and
Select exit:2227/2227GPU completions,logerrors0,65.599s. Color/depth conversion
fixtures pass exact comparisons, and the old raw-depth-copy path is rejected.
Memory evidence is in artifacts/native/console-strided-readback/memory-audit.json.

**No whole-game or physical FPS improvement is established.** Emulator copy time
fell11.300->4.676ms, but total CPU time in the same requested menu window rose
103.721->124.629ms with different geometry rebuilding and animation observations.
This is not an equivalent-work whole-frame comparison. Do not predict console
FPS from it. Krystal meeting the EarthWalker in the central room remains untested
with these changes; the exterior smoke test does not cover that failure.

This is an optional development validation candidate, not a playable release.
Check the latest installation record before assuming it is on the SD card.

## Local chained-coordinate candidate: 388f2c8cd244 (not installed)

This historical local build was superseded by **4138788f4af8** above. The
candidate tightens the projective-coordinate subdivision bound and retries
oversized valid triangles in up to four bounded GPU chunks. It preserves the
existing texture-coordinate accuracy limit and 384-vertex scratch buffer.
Two captured effects eliminate CPU pixel shading, but the second strict image
comparison fails at one translucent subdivision-edge pixel (up to45/255 RGB,
30/255 alpha); the remaining pixels differ by at most5/255. That known image
limitation is recorded in `artifacts/draw-replay/20261005-201257/summary.json`.
Do not mistake the local package for a deployed build or a full correctness pass.
Normal-HID headless smoke: Select650 observed651,2236/2236GPU completions,
logerrors0,owned exit61.646s; captured Palace exterior inspected.
No measured physical FPS improvement, central-room EarthWalker validation,
or playable release claim accompanies this candidate.

## Layered fades and EFB sample centers: 9e562af89aee (previous installed candidate)

Installed/readback-verified atnew223:5000; record
`artifacts/console-test/20261005-193834/transfer.json`. Saves and asset packs
preserved. Prior61b3 executable retained with the dated backup suffix.

This is a development build, not a playable release. It adds PICA paths for
layered material tint/opacity combinations that previously shaded pixels on
the CPU, and moves aligned half-size EFB color/depth copies to exact texel
centers. It does not establish a fix for the reported central-room EarthWalker
cutscene or steady30/60FPS. No new asset pack is required: retain1246077f6f9b
meshes,8bc55417d89e textures,original ISO/movie,and existing saves.

Two private slow-draw replays eliminate CPU fragment samples, with native-grid
image differences<=2/255. The expanded material fixture covers81920channels;
new six-stage fades have observed max4/255, while earlier cases retain<=3.
Depth/color conversion checks remain exact and the physical depth admission
guard still requires exact agreement. A float24 coordinate regression catches
the old epsilon formula; this is a suspected hardware issue, not yet a proven
explanation for the console's rejected depth path.

Exact console binary reached the saved Palace outdoor area in isolated silent
Azahar and exited with Select at651;2237/2237GPU queues completed, log errors0.
The invocation accidentally requested660 with a script ending650, causing a
harness bound failure. Original report is retained. Read-only audit against
the actual recorded650input verifies clean shutdown without rerunning the boot:
`artifacts/native/console-layer-fade-centers/reviewed-shutdown.json`.
Image630 inspected. This is loading/rendering/shutdown smoke evidence only.

No long Palace replay is requested solely on this evidence. The outstanding
hardware question is whether the revised sample centers pass GPU depth
admission in a varied scene, followed by actual copy/frame timing measurements.
Previous physical collapse was7.784seconds software shading in one frame;
even complete removal of depth-copy overhead cannot solve that by itself.

## Fenced texture-backed framebuffer candidate: 61b3a9587092

Development validation package; not a playable-performance release. Target
60 FPS, steady 30 minimum, remains unmet. This candidate replaces reflection
row-DMA capture with a texture-backed framebuffer sampled directly after
producer completion. It keeps consumer completion before overwrite, private
depth snapshots, GPU depth admission, and the raw-readback rejection guard.
This is a different storage/copy architecture, not an unfenced transfer mode.

Companions: `efb_contiguous.txt` containing `2`, private joint-safe reduced
mesh pack `1246077f6f9b`, and optional native compressed texture pack
`8bc55417d89e`. The texture pack reduces GPU storage, preserves alpha, and
falls back on absent or invalid data. See `PREPARED_TEXTURES.md`. Original
ISO, prepared movie and saves stay intact. The reduced-fur-surface experiment
remains disabled because it regressed offline performance.

Exact console binary passed silent headless HID testing through a saved
Palace checkpoint, Select shutdown, 2236/2236 completed GPU queues and owned
process cleanup (`artifacts/native/console-direct-efb-textures`). Inspected
630 capture shows Krystal, rain, lightning-lit platforms, shadow and HUD.
Sampled normal plus reserved main stack 60,074,496/71,303,168 bytes; linear
17,111,264/25,165,824; VRAM 4,971,520/6,291,456, each with at least15% reserve.
These are sampled emulator measurements, not physical peak guarantees.

Launch normally; no reboot required. A useful physical check is Fox's cockpit
menu followed by ordinary Palace play, then Select and ftpd. It should answer
whether the direct framebuffer path is stable and reduces the previously
expensive copy stage; logs also measure additional texture-pack SD overhead.
There is no established physical FPS improvement. Emulator frame timings did
not improve substantially, and cannot predict this console copy-path cost.
Rollback uses the prior backed-up executable, `efb_contiguous.txt` set to0,
and removal/rename of the optional texture pack. Do not delete saves.

Package preparation and installation state are recorded in IMPLEMENTATION.md
and the dated transfer record; this document alone is not proof of deployment.

## Reduced meshes and fragment batching: 81618a4a2487

Installed and readback-verified; see transfer record below. Includes the previous shadow and
startup fixes, native400x240/full-width camera, three-shell fur, host-prepared
model and environment meshes, and ordered CPU-fragment submission batching.
Private mesh pack bb9e6d26eaa9 is an optional companion to the original ISO.
Collision, animation arrays, progression, saves and game timing remain unchanged.

The reduced pack removes about63% of triangles from its changed render lists;
this is not a whole-scene FPS gain. A focused headless fixture compares2,457,600
bytes exactly and reduces5216 separate fragment submissions to265 while retaining
all shading, ordering, depth/alpha behavior and atlas-reuse fences. Neither
result establishes physical30/60FPS. This remains an unproven development build.

Launch normally from Homebrew Launcher; no reboot is required. Circle Pad moves,
C-stick operates the original inventory interface, ABXY/L/R use their existing mappings; Select exits and
flushes buffered logs. Keep saves. Check normal menus and Palace play for missing
surfaces, animation distortion, visual effects and long pauses, then reopen ftpd.
Test duration is optional; this is a combined candidate, not another profiling-only
build. The existing original RVZ, ISO, prepared movie and saves are preserved.

See docs/MODEL_LOD.md for preparation, scope, limitations and rollback by removing
the optional model-lod.bin. Exact-console test console-reduced-world completed650 normal HID retraces,
Select shutdown,2235/2235GPU completions and owned cleanup. Inspected630 Palace
image retains Krystal,rain,lightning,platform/HUD;42 geometry assets applied,
0rejected. Normal+reserved stack60073016/71303168,linear20588320/25165824,
initialized VRAM3096576/6291456,main stack84824/524288. This checkpoint view has
zero software-fragment triangles; batching correctness is covered by the focused
fixture, not this camera. Physical performance remains to be measured.
Final installation/readback evidence is recorded below.

## Full-width / reduced-fur candidate: f1f539ebba91

Installed and full readback-verified atnew223:5000. Evidence:
artifacts/console-test/20261005-152049/transfer.json. Previous executable and
metadata retain .backup-20261005-152049; saves and private assets are untouched.
Renders across the native 400x240 top screen with a 5:3 gameplay camera and
no port-added side bars. Retains PAL offscreen rows used for texture copies.
Ordinary fur uses three layers spanning the original depth. Gameplay, timing,
controls, saves and essential effects are unchanged by these visual adaptations.
The previous shadow alignment fix remains included. Existing saves are retained.

This is still a development build, not a playable-performance release. Matched
emulator menu comparisons reduce Fox vertices about38% and Peppy about16%, but
whole-frame reductions are much smaller and do not establish console FPS.
The full-width change adds horizontal fill work. No 30/60FPS claim is made.
Optional fur_shells.txt: 0 restores original fur, 2..16 sets the visual shell
budget; without that file the console uses3. Existing original assets stay private.

Headless exact-console run console-full-width-three loads the private Palace
checkpoint through normal HID menus, completes650 retraces, drains2565/2565GPU
queues and exits with Select. Inspected630 image shows full-width Palace,
Krystal, rain and HUD. This is not a new-game/full-progression validation.
Sampled normal+stack<=59950760/71303168, linear<=20608128/25165824,
VRAM<=3096576/6291456, main stack84824/524288. Physical correctness and sustained
performance remain to be checked; no reboot is required for ordinary launch.

The preceding3a3 build now has a longer physical run:13,169render rows and
about57minutes after startup, clean logged Select shutdown, no recorded panic.
User reached Krazoa Palace and reports unacceptable scene-dependent slowdown.
Mixed-scene average3.83FPS is not a matched-scene benchmark; some1000-frame
windows average2.25FPS and the late170-frame window1.35FPS. Do not call this
playable. Evidence artifacts/console-test/20261005-151750/performance.json.


## Installed shadow alignment fix: 3a3a3f6a77c5

Installed and readback-verified atnew223:5000, /3ds/foxhollow.
artifacts/console-test/20261005-140048/transfer.json records the executable,
manifest, README and attribution16 verification. Previous c83d files have
.backup-20261005-140048 suffixes. Saves and assets are untouched. The immutable
package retains its pre-install documentation; this record supersedes that status.

The longer physical run of c83d crashed during the Krystal cutscene, as reported
by the user. Collection artifacts/console-test/20261005-135104 includes Luma
dump185: core0 data abort, alignment fault at objShadowRender+0x5d0, loading a
float from gShadowDrawScratch at 0x00831f25. All 96 dumped instruction bytes
match the installed c83d ELF. This is a confirmed failure, despite the earlier
short menu run exiting cleanly. Application logs are buffered/truncated; the
leftover log-io-summary.json does not establish a clean exit of this run.

The native generated sources now preserve all six ALIGN_DECL declarations
across shadow, object-matrix and distortion units; the desktop global.h had
erased them. No shadow or game work is removed. The linked build checker rejects
misaligned buffers, including the previously fixed collision-grid origin.
The strict ARM regression reproduces the old exact fault, and the corrected
binary completes 3,600 shadow float reads across all 1,200 scratch records.
Evidence: artifacts/shadow-alignment/arm-probe.json.

Exact console binary 3a3a3f6a77c5d6b08dd8782ecca0e78224b99fd13d77124c84973f0fb98436d3
passes artifacts/native/console-shadow-alignment: 650 scripted HID retraces,
3352/3352 GPU completions, drained Select exit and owned-process cleanup.
The inspected retrace630 image shows loaded Krazoa Palace, Krystal and her
shadow. This checkpoint load is not a replay of the reported opening cutscene.
Sampled normal+stack60019048/71303168, linear20704224/25165824,
VRAM3096576/6291456; main stack84824/524288, worker69984/262144.
All sampled pool reserves exceed15%; these are not full-game peaks.

This is a specific crash fix, with no FPS improvement claim. Physical validation
of the fix is pending. The port remains below the steady30FPS minimum. Saves
and private assets must remain unchanged during installation; do not request
another menu FPS test for this build. Any subsequent hardware check should
answer whether normal progression passes the formerly crashing cutscene.

## Previous installed development build: c83dfe72923b

Physical run collected in artifacts/console-test/20261005-133343 confirms
unplayable menu performance: Fox render273-337 averages6.328FPS, median
CPU121.784ms; Peppy417-465 averages8.446FPS, CPU91.620ms. Scene labels are inferred
from recorded workload, not scripted physical input; no exact prior-run speedup
claim. Changed logs, sampled timing marker and verified executable identify the
new run. Normal Select shutdown481,4226/4226GPU completions, no panic.
SD reads consumed422.394ms/10.095s in the Fox retrace window and341.879ms/5.699s
in Peppy's; Peppy also wrote64KiB in29.088ms. No SD errors. CPU/render preparation
remains the main limitation. Normal+stack60025248/71303168,linear20420416/25165824,
VRAM2965504/6291456; sampled15%pool reserves pass. This menu run establishes
neither full-game memory peaks nor playable30/60FPS. No follow-up build requested.

Installed and readback-verified atnew223:5000 under/3ds/foxhollow;
artifacts/console-test/20261005-131528/transfer.json records all four verified
files. Previous74fb executable/configuration backed up with suffix
.backup-20261005-131528. Assets and saves unchanged. Immutable package
foxhollow-validation-c83dfe72923b.zip records the pre-install documentation;
its SHA256 is02e05616abf052faae68f2aa694a8ef769662d1943406bc2592d73197a2b8c46.

Prepared at the user's explicit request for a new console build. This retains
74fb's prepared renderer and idle bookkeeping fixes, and samples detailed
render component clocks once per 16 complete frames. Every-frame FPS/workload,
SD measurements, audio service and watchdog clocks remain enabled. Sparse
render-profile.csv rows are intentional; do not treat absent rows as zero cost.
Melee's renderer.c native_detail_begin/end is the reference for avoiding
production per-draw profiling overhead; our whole-frame samples retain complete
component spans. No game, shader, draw-order or buffer-lifetime change.

Exact console binary c83dfe72923bec656b3432ff040710a4ce95e9a1eea0a54843c1fa7b6ff5611b
passed artifacts/native/console-component-sampling: 330 scripted HID retraces,
3295/3295 GPU completions, drained Select shutdown and owned-process cleanup.
Fox180 and Peppy300 captures inspected. Matching script/save/emulator comparison
against74fb: Fox CPU124.032->120.259ms, FPS6.708->6.934; Peppy CPU114.058->109.248ms,
FPS7.242->7.404. One run each; this small emulator gain is not a console forecast
or playable performance. No new audio-equivalence claim. Sampled memory remains
normal+stack60025144/71303168, linear20442208/25165824, VRAM2965504/6291456;
all sampled15% reserves pass. SD read timing remains recorded, with no writes
or errors in the compared menu windows.

Launch normally; no reboot required. For a comparable console measurement,
press Start, leave Fox's cockpit visible about25seconds, pan to Peppy for about
25seconds, press Select, and reopen ftpd. Report any crash or visible corruption.
Assets and saves are preserved. See the collected physical result above; this is a
development build and does not meet the steady30FPS minimum or60FPS target.
Installation status and readback hashes are recorded in artifacts/console-test.

## Installed follow-up candidate: 74fbafa03760

Installed at the user's request and readback-verified in
artifacts/console-test/20261005-114957/transfer.json. Previous28e460145e8b
is backed up on SD; assets and saves are preserved. Collected logs are unchanged
from before28e installation, so they provide no physical measurement of28e.
Physical validation of74fb is pending. Fixes empty source-lease retirement walking128 cache
slots twice per GX barrier even with copied sources disabled. Also avoids
worker wakeups when an acquire read proves the CPU queue empty, outside any
open/deferred scope and without an allocated source pool. Active source and
GPU completion fences remain intact. No game work or effects are removed.

ASan/UBSan source lifetime and dispatcher tests pass, including delayed readers,
invalid leases and negative controls. The exact console binary completes330
headless HID retraces,3294/3294 GPU submissions and Select shutdown. Inspected
Fox/Peppy captures; no pixel-exact or audio-equivalence claim. Same script and
emulator as28e460: Fox renderer CPU164.159->124.032ms, submission5.350->6.708FPS;
Peppy168.837->114.058ms,5.121->7.242FPS. This repairs a regression and removes
idle bookkeeping. It is still insufficient for the requested30/60FPS and is
not a hardware prediction. Console improvement remains unmeasured.

Sampled normal+stack60025144/71303168,linear20442208/25165824,
VRAM2965504/6291456; mainstack84824/524288,shared worker87664/262144.
All sampled15%reserves pass; no new memory pool. The same Fox25s/Peppy25s
normal-launch check below applies after installation and notification. Keep
render attribution16. Evidence:artifacts/native/console-idle-dispatch-fix,
with reproducible comparison from artifacts/research/compare_idle_dispatch.py.

## Previous combined preparation candidate: 28e460145e8b

Experimental physical validation build, not a playable release. It enables
producer-side GX state patches, batch consumption, matrix blocks, compact
geometry keys, synchronous model source scopes, active matrix palettes,
material scope, fur post/coefficients and order-preserving movie batching.
Source-reader joins, GPU-copy fences, memory pools, audio, clocks and normal
game behavior remain in place. Resource-generation shortcuts are OFF.
`render_preparation.txt=0` selects the older preparation path in the same binary.

The exact console binary passed isolated silent headless Azahar Fox/Peppy
menu and Select-exit checks (330 retraces,3302/3302 GPU completions). Both
views were inspected. A same-binary legacy comparison also passed. Candidate
versus legacy Fox150-180: CPU211.242->164.159ms,4.244->5.350FPS; Peppy270-300:
CPU183.519->168.837ms,4.730->5.121FPS. These are emulator results, not physical
estimates. Compared instead with the older fc6 archive, Fox is similar and
Peppy slower (5.937->5.121 emulator FPS). Do not conceal this mixed result or
claim a console gain before a physical run. User explicitly requested a new
candidate after the profiling run; installation does not establish acceptance.

Sampled normal use including main-stack reservation60015632/71303168,
linear20440832/25165824,VRAM2965504/6291456. Separate15%reserves pass for this
menu sample. No gameplay peak or physical-console validation is inferred.
Main shader433/512instructions,93/96floats,16/16booleans: boolean registers
are fully allocated, with no headroom for additional booleans.

Launch normally; no reboot needed. Press Start, leave Fox's cockpit visible
for about25seconds, then pan to Peppy/Options for about25seconds. Press Select
and reopen ftpd. Note corruption, crashes, long pauses or audio gaps. The
purpose is to establish the combined changes' actual hardware cost, not to
claim30FPS. Keep attribution sampling at16 for comparison. Existing saves
and private game assets are preserved.

Latest collected physical run fc6edfbba82c: about5.96FPS Fox and7.23FPS Peppy,
normal shutdown with no recorded panic. View labels are inferred from workload;
this is not a matched gain over the prior6.12FPS baseline. Evidence:
artifacts/console-test/20261005-112032. Exact candidate evidence:
artifacts/native/console-prepared-model-gx and its matched-legacy-comparison.json.

## Previous profiling candidate: fc6edfbba82c

This is a diagnostic build, not a performance fix or a playable release.
The last established physical baseline remains b52697da501a at about6.12FPS.
Its broad CPU timing does not identify whether the dominant physical cost is
GX preparation/identity work or preparing/submitting resident GPU geometry.
Emulator timing cannot answer that hardware question reliably.

The package enables render_attribution.txt with period16. One complete frame
in sixteen records raw GX and backend spans in render-attribution.jsonl.
Initialization is excluded. The existing buffered logging policy applies;
no new worker, renderer allocation, shader, GPU transfer or service is added.
A64KiB normal-heap stdio buffer is allocated only when sampling is enabled.
GX/shading/backend spans overlap: do not add them into a CPU total or convert
Azahar timings with a hardware multiplier. Timer frequency comes from the
pinned SDK and is included in each row. Profiled-frame overhead is unresolved
on physical hardware; compare sampled frames with adjacent ordinary frames.

For this specific measurement: launch normally, press Start to reach Fox's
cockpit, leave Fox visible for about30seconds, then pan to Peppy/Options and
leave him visible for about30seconds. Press Select and reopen ftpd. No reboot
is needed. This gathers enough separated samples in both views without
requiring new-game progression or assessing a claimed FPS improvement.

Exact-console-binary headless check: artifacts/native/console-sampled-render-attribution,
330retraces,63.935s, normal emulator HID input and Select exit,3301/3301 GPU
submissions drained, zero log errors/backpressure, owned emulator exited.
Fox and Peppy captures inspected; scene and menus remain visible. No new
pixel-equivalence or audio listening claim. Sampled normal heap plus stack
60037592/71303168B, linear20442880/25165824B, VRAM2965504/6291456B;
main stack upper bound84640/524288B. Separate15% reserves pass in this menu
sample; these are emulator observations, not console memory certification.
Current sources also contain the previously tested Peppy GPU shell support;
this build is not a byte-identical instrumentation patch of archived b526.

Analyze downloaded spans with tools/analyze_render_attribution.py and explicit
--evidence-kind physical_console, matching --first-frame/--last-frame to the
scene window in render.csv. Keep the raw frame samples and SD/audio logs.
Delete render_attribution.txt or set it to0 to disable this extra profiling.
Private assets and saves are not part of the package and must be preserved.

## Previous candidate: b52697da501a (Peppy menu fur fallback)

Physical follow-up: artifacts/console-test/20261005-010727 confirms zero CPU
fragment pixels and clean shutdown, but Peppy400-549 is only6.12FPS. User
reports the best performance is still unplayable. This closes the severe
fallback reproduction only. Do not request another physical run for a small
gain or isolated stall fix; establish substantial normal-menu improvement first.

Development validation build, **not a playable release**. Physical e00 logs
in artifacts/console-test/20261005-003901 measure about6.02FPS at Fox's cockpit,
then0.318FPS after panning toward Peppy. About2945ms/frame was CPU fragment
rendering; SD reads account for0.20% in that slow interval. Select shutdown
completed. The shared renderer has not established a useful physical gain.

This candidate recognizes the second fur alpha equation used in that menu
view and sends it through the existing GPU fur path. Both indirect samples,
RGB shading, alpha addition, depth and blending remain. It allocates no new
render target or worker. Other optional cache/matrix/texgen experiments remain
off. Source provenance and detailed comparisons are in IMPLEMENTATION.md and
licenses/ADAPTATIONS.md.

Headless normal menu pan reproduced the previous collapse and worker timeout.
With the change, render270-275 rises from0.336 to4.380FPS in Azahar and the run
completes330retraces with drained GPU/log shutdown. CPU pixel fallback becomes
zero. The earlier stationary Fox window is unchanged; this is a specific
severe-stall fix, not a claim of playable menu performance. Scalar equations,
PICA combiner tests and captured-draw comparisons pass. Fur coordinate sampling
retains the existing approximation; inspected close-up has mean RGB differences
below2/255 but some edge/sampling differences up to35/255.

Emulator sampled normal heap plus main stack60190952/71303168B,
linear20438048/25165824B, VRAM2965504/6291456B, main stack78384/524288B.
Physical RAM peaks, timing and the new Peppy appearance remain to be measured.
The exact console executable also completes the normal HID menu pan in
artifacts/native/console-hid-unmasked-peppy:331retraces,65.28seconds, clean
Select exit,3266/3266GPU queues, no log errors. Peppy295-322 measures4.98FPS
in Azahar with zero CPU fragment pixels. Its sampled normal+stack59971728B,
linear20430048B and VRAM2965504B meet the reserved pool headroom requirement.
Inspected the captured Peppy view. A second build reproduces identical bytes.
Shader storage remains433/512instructions,93/96float and16/16boolean registers;
these are capacity counts, not executed GPU costs.

Build: `python3 tools/build_native.py --shared-worker-console` in the pinned
Linux environment. SHA256:
`b52697da501a4ff4666249f5efeba7609c0322821ec9d4f9afac38a9c033a387`.
Use existing private assets and saves; no asset/save conversion needed.

Physical check after installation: launch normally, no reboot required. Press
Start, pan right from Fox to Peppy/Options, and leave Peppy visible about20seconds.
Then press Select and reopen ftpd. Record whether the severe stall recurs,
and any visible fur/transparency corruption or audio gap. This checks the
specific reproduced failure; no gameplay/profile loading is needed.

## Earlier candidate: e00fdfdc9fb6 (shared audio and GX core-2 owner)

Development measurement build, **not a playable release**. Installed baseline
dbbcba23f7d2 measures6.33FPS in the cockpit, which is not playable. This candidate
moves leased fur translation onto the existing audio worker. It adds no thread;
that worker's stack grows96->256KiB, with a256KiB command ring. Audio synthesis
and bounded GX slices share the owner, with nonblocking synth acquisition,
separate FP controls, source-retirement barriers and joined shutdown. Game,
movie and DVD callbacks stay on main. Buffered logging uses one64KiB batch
instead of four16KiB batches; SD counters remain enabled. Fur-post, movie-batch,
uniform-row and TEV-filter experiments remain OFF to isolate the worker change.

The hardware question is whether physical core-2 translation reduces frame
time, and how much time main still waits for it. Azahar's virtual cores execute
sequentially in host time. Same-binary emulator menu comparison3959974cdd3f,
menu-shared-console-reference/options, render150-187, measures5.28064->4.77476FPS
and CPU median158.8115->179.734ms. **No emulator speed improvement is claimed.**
Draws/vertices/command workload match at the medians. Captures inspected; animated
movie/highlight/prompt phases differ, so this is not pixel-exact equivalence.
Audio120/180 is numerically nonzero without zero runs, not a listening verdict.
Actual shared-worker fixturec23650dbd7e4 checks concurrent payload ownership,
synth-lock contention, FP isolation, attach/detach/restart and suspend callbacks;
its intentionally broken FP variant is rejected. Physical cache behavior,
suspend, audio scheduling and FPS remain unverified on this candidate.

palace-shared-console-options completes660retraces/82.81host seconds through
normal menu/save selection into Palace, with drained owned exit. Frame600
inspected: Krystal, platform, rain and HUD render. State records show loaded
map11, gameplay UI, no pending map and player sequence-1. The separate live
semantic reload checker failed to verify its emulator memory mapping; do NOT
count that checker as passing or claim full save/control validation here.
Existing unchanged save support has earlier independent evidence.

Sampled menu normal heap plus reserved main stack60204136/71303168B;
Palace maximum60209952B (15.56% reserve), linear20678304/25165824B (17.83%),
VRAM3096576/6291456B (50.78%). Main stack78216/524288B; worker70184/262144B.
Shader budgets are unchanged:433/512instructions,93/96float and16/16boolean
registers. These are emulator samples, not complete gameplay high-water marks
or proof of physical RAM entitlement. A repeated console build produced the
identical .3dsx SHA256e00fdfdc9fb64d1f166759cc8e6c9c2a1ff45a911cc46f26822f73dc94817663.

Build with `python3 tools/build_native.py --shared-worker-console` in the pinned
Linux environment. Ordinary console builds keep the new executor disabled.
An explicit `0` in `sdmc:/3ds/foxhollow/shared_worker.txt` disables GX offload
in this same binary; it retains the established audio worker. The executor also
requires the existing synth-job option. No asset or save conversion is needed.

Next physical measurement: launch normally (no reboot needed), press Start to
reach Fox's cockpit menu, leave it untouched about25seconds, press Select,
and reopen ftpd. Report a crash, obvious corruption or audio gaps. No new profile
or gameplay progression is requested. Exit may pause while buffered logs drain.
Collect worker busy/wait, frame, audio, SD and memory counters before changing
the executable or its mode. Preserve installed backups and existing saves.

## Earlier candidate: dbbcba23f7d2 (pipelined music during movie/menu rendering)

Development validation build, **not a playable release**. This build moves
native MusyX blocks to the existing core-2 audio worker while leaving movie
callbacks, cooperative queues and game updates on main. It retains all samples,
uses one pending job, joins before synth mutation or ring reuse, and refills
synchronously if fewer than seven packets are buffered. Six complete packets
beyond the playing packet provide reserve for producer stalls. The NDSP copy
and cache flush are unchanged. Owned CPU-only audio cache notifications avoid
unrelated GX cache scans. The earlier batched DMA, live plane hints and ARM
collision-grid alignment fix are included.

Matched headless menu comparison on emulator binary83fadaa1697d, window130-167:
mean main/render preparation215.821 to148.391ms, interval252.928 to168.660ms.
That four-packet prototype had two additional transition underruns and was
superseded. The seven-packet candidate bf5c2adb73c2 measures149.899ms preparation
and173.083ms interval; its underrun counts match the reference at every sampled
retrace (1,1,2,2,3,3), and captured audio120/180 has no silent frames. The reserve
threshold is the only active emulator-path change from the matched comparison.
Screenshots show the same menu; movie phase and blinking text differ with elapsed
audio time. This is roughly1.5x emulator cadence, **not fivefold improvement or a
physical FPS prediction**. Physical startup, worker behavior and FPS still need
measurement on this binary.

Host checks compare exact PCM and callback/mutation order under sixteen delayed
schedules and urgent refills. Actual headless worker/NDSP checks cover suspend,
resume, stop, restart, joined shutdown and captured stereo tones. Sampled emulator
normal heap plus reserved stack60009896/71303168B; linear19411328/25165824B;
VRAM2965504/6291456B; process commit104030208/130023424B. Main stack bound76760B,
audio stack bound4096/98304B. Extra packet/control storage is8224B BSS; no new
thread, linear buffer or shader. These are menu samples, not gameplay peaks.

For the next measurement: launch normally, press Start to reach Fox's cockpit
main menu, leave it untouched for about25seconds, press Select, and reopen ftpd.
No reboot or profile creation is needed. Report crashes, obvious corruption or
audio gaps. Existing assets and saves are preserved. Exit can take longer while
buffered logs are written. The shorter menu measurement isolates this CPU change:
the previous extended console run shows large late SD waits alongside telemetry
writes, which remain an open issue rather than evidence of steady game cost.

Both audio changes default on in this console candidate. An explicit `0` in
`sdmc:/3ds/foxhollow/audio_synth_jobs.txt` or `audio_cpu_cache.txt` disables that
individual change for fault isolation. Other renderer experiments remain off.

## Earlier candidate: 3a4b3c616890 (collision alignment, batched DMA and live plane hints)

Development build, **not a playable release**. This supersedes the uninstalled
349ecec6e83e package. Main-menu performance remains the priority. Physical
5bf7b640444d still measures about7.87FPS in the matching opening window.

The gameplay-entry crash after profile naming is an ARM VFP alignment fault:
collision code reads 32-bit coordinates from byte-aligned gTrackGridOrigin.
This candidate explicitly aligns that260-byte buffer to4bytes; no collision
arithmetic or gameplay state is changed. The PC regression executes the actual
linked ARM entry with strict VFP access checking: old build faults, fixed build
reads both coordinates. Physical confirmation remains pending. This crash fix
is not a performance claim.

This build also includes the previously tested batched50-row DMA retirement
and live clipping-plane hints described below. The GPU producer/consumer fences
and bounded waits remain. Compact attributes, render LTO, model scope, post-only
UV transforms and shared paired-bounds experiments remain disabled. There is
no overclock beyond supported New3DS804MHz mode and no skipped game work.

Launch normally; no reboot is needed. Performance measurements should focus
on the opening and main menu. Preserve existing saves. Select exits. Reopen ftpd
after the run for log collection. Do not infer complete gameplay compatibility
or playable FPS from this package. Both currently linked ELF and the original
faulting instruction regression are recorded in artifacts/collision-grid-alignment.

## Earlier candidate: 349ecec6e83e (batched DMA and live plane hints)

Development validation build, **not a playable release**. The working physical
reference5bf7b640444d measured7.84FPS in the opening. This candidate keeps the
successful DMA mechanism, but retires the50disjoint capture rows in batches
instead of waiting after every row. Producer and consumer fences, bounded
capacity waits, callback joining and queue reservation remain. It also retests
previously useful clipping planes first, always with current bounds/matrices;
no visibility decision, fur layer or gameplay work is reused or omitted.

Headless exact device fixture:1536000color/1689600depth checks pass. Host queue
checks cover1760 completion/stall schedules and missing-fence/capacity negatives.
CPU hint checks:123200host comparisons and10431in-game checked decisions agree
with the original calculation. Fast opening capture is byte-identical, with
unchanged draws/vertices. Matched emulator CPU104.947ms versus108.559ms for the
physical reference's emulator counterpart (3.3% reduction); presentation interval
is unchanged. This does not establish physical FPS. The specific console question
is how much of the measured25.3mscopy/queue waits survives batched retirement,
and whether DMA completion remains stable. No large speedup is promised.

Sampled initialized emulator RAM including reserved main stack59901392of
71303168normal-heap bytes; linear13743904of25165824; VRAM2965504of6291456.
Process commit104009728of130023424. The hint table adds1024B; rejected larger
cache experiments are removed. These are title samples, not gameplay peaks.

Launch normally from the Homebrew Launcher; no reboot is required. Leave the
opening untouched for about60seconds, then press Select and reopen ftpd.
Report any crash, long pause or visible corruption. Assets and saves are preserved.
Physical performance, complete gameplay and extended stability remain unvalidated.

## Previous working reference: 5bf7b640444d

Development validation build, **not a playable release**. Physical startup now
works on5bf7b640444d. Collection console-test/20261004-130659 measures7.84FPS
over render frames34-71,median94.7msCPU/render preparation (including25.3mscopy
queue waits),and5.1msremaining GPU tail. The GPU tail is not total GPU work.
No panic,196render rows and938/938prefix retirements. SD reads occupy5.03%of a
separate steady retrace window; logging adds no writes/flushes in that window.
This improves on the earlier4.60FPSworking build but remains unplayable.
9561746b785b,313849a90958 and9a614627c455 failed on hardware before displaying
the game. Each stops at the same queue tail at retrace16/renderframe8. Corrected
gap flags and Melee's sampled-linear capture allocation did not resolve it.
The subsequent947aeefb9197 run isolated the transfer: five producer draw lists
completed, then the single padded color copy stalled before any depth copy or
consumer draw. Evidence: console-test/20261004-121755. The blue bottom-screen
bar was accompanied by "GPU copy 9 transfer timeout", not successful startup.

2f02676c2b22 also failed on its first contiguous raw row. Replacing the sampled
capture with DMA in485f0ff97597 passed that physical operation, but then stalled
on the separate raw VRAM depth snapshot. Evidence: console-test/20261004-124759.
This candidate extends DMA to that owned snapshot, preserving all copy-time
depth bytes. Bounded producer/transfer waits remain; no game work is omitted.

Focused tests pass1536000color/1689600depth exact checks, including nonblank GPU
depth admission and later framebuffer overwrites. A new negative control rejects
the physically failing raw depth snapshot. The original raw CPU depth rejection
and sampled-capture regression remain. The emulator's approximate floating-point
depth aliasing was independently shown to corrupt packed bytes; a custom build
uses its existing packed-byte cache fallback. No game checks were relaxed and no
emulated clocks changed. Its host timings are not console performance evidence.

The80retrace title run completes with318/318prefixes retired and a byte-identical
frame60image. Sampled initialized memory: normal+mainstack59901392/71303168bytes,
linear13743904/25165824,VRAM2965504/6291456. The later physical run above
establishes startup recovery; playable FPS remains unmet. Fused vertex packing
and rejected cache experiments are absent/disabled in that reference.

The short hardware startup check passed; another identical run is not needed.
That build remains the measured physical reference; the new candidate above
is a separate, unmeasured hardware test. The prepared PAL movie is installed; saves and assets
are preserved. Longer gameplay and final hardware validation remain open.

This candidate includes retained indexed geometry and fur, audited attribute
reuse, GPU material/reflection paths, buffered diagnostics, background mixing,
and prepared-movie support. New changes target framebuffer copies and repeated
CPU vertex processing:

- Animated meshes that repeatedly miss the geometry cache periodically retry
  recording instead of filling it with single-use packets. Every vertex still
  uses current data and renders normally. In the matched title window, cached
  GPU geometry falls from6.17MBto2.00MB; arena caps stay unchanged.

- Single-matrix visibility stops at the first rejecting plane while preserving
  the existing GX/PICA margins and mixed-matrix behavior. No new cache is used.
- Register-only and texture-times-register materials bind changing colours
  without the general symbolic compilation. More than111,000accepted plans
  matched the full compiler byte-for-byte; other states retain that compiler.
  Their cache keys now contain only the28bytes that determine the complete plan.
- The point geometry shader rejects fully off-screen expanded sprites. Wide
  points crossing any screen edge remain visible in96CPU/GPU comparison cases.
  Shader storage is433/512instructions; console GPU benefit is unmeasured.

- Animated GPU-lit meshes use a prepared decoder with the original scalar
  fallback. Emulator validation compared134,608title and143,050Palace vertices
  byte-for-byte against the original decoder with no mismatch.
- Melee's exact alpha-predicate cache avoids repeating the256-value reduction
  at every draw. Alpha state is reconsidered on every relevant raster change;
  collisions recompute the complete predicate. The cache uses1,280normal bytes.

- Early GPU command submission overlaps translation with completed immutable
  prefixes. It includes Melee's callback-publication, join and queue-capacity
  fixes. Pending compatible draws remain batched. With this enabled, `gpu_ms`
  reports remaining queue tails, **not total GPU work**.
- Vertex/index cache flushes cover only appended bytes, including a newly dirty
  partial cache line. A deliberately noncoherent host test detects stale data.
- CPU-only geometry keys move to normal RAM; common GPU keys omit unused scalar
  material fields. Each optional allocation preserves normal-heap headroom.
- GPU depth sampling compares its actual image against safe CPU readback before
  admission. Blank images cannot qualify; failure retains CPU conversion.

- R4 shadows use a bounded GPU cache instead of CPU conversion and re-upload.
  All 256 intensity values are checked on the actual rendering backend before
  selecting direct or compensated quantization. If neither is exact, the
  original CPU path remains active. No shadows or game work are omitted.
- The CPU Z8 reflection conversion processes complete texture tiles. A bounded
  ARM11 comparison drops from 2,070,005 to 322,536 instructions with all 76,800
  bytes identical. This is instruction work, not measured console time.
- Fragmented texture allocation retries after safely retiring cached geometry
  or unpinned textures. A real-fragmentation fixture and its negative control
  cover this failure. CPU read-visibility requests preserve authoritative GPU
  images; CPU writes supersede them.
- Indexed retention admits up to 2,048 unique ordinary vertices, preserving
  the 1,024-vertex fur limit and existing cache caps. The measured large scenery
  list fell from 7.316 to 0.448 ms per retrace in Azahar.
- A separate PICA vertex program evaluates both distortion-lighting channels
  and all six lights. Unsupported states retain the CPU path. The measured
  distortion list fell from 7.527 to 0.782 ms per retrace in Azahar. This adds
  real GPU work whose physical cost remains unmeasured.
- I8 depth-image uploads repack pairs of rows with aligned word transfers.
  A 320x240-to-512x256 ARM11 comparison falls from 1,715,812 to 523,403
  instructions with all output bytes identical. No extra heap allocation.

Matched OpenGL Palace frames 640-660 show median CPU preparation falling from
99.610 to 68.488 ms and copy time from 33.889 to 8.891 ms. These runs use the
same save, inputs and stationary viewpoint. Weather and animation phases differ
because the game's normal elapsed-time updates remain active; gameplay images
are not pixel-identical. Captures were inspected. Exact conversion/lifetime
checks use focused fixtures. **These numbers do not predict console FPS.**
See `artifacts/native/palace-shadow-depth-tiles/window-comparison.json` and
`docs/IMPLEMENTATION.md` for build hashes, counter differences and limitations.

After indexed retention, a same-binary distortion-shader comparison over
renderer frames 632-652 (retraces 640-660) measured 61.573 versus 51.785 ms CPU
preparation. Median draws were 97 versus 95 and vertices 41,325 versus 40,779;
weather and animation remain a comparison limitation. Evidence is in
`artifacts/native/palace-sphere-gpu/window-comparison.json`. The current candidate
also fixes a subsequent shader-state leak into framebuffer clears: a targeted
negative control fails before the fix and passes afterwards. No console FPS
improvement is inferred from these emulator timings.

Checks include 638,912 exact GPU shadow comparisons with eviction, overwrites
and interior-range CPU reads; 1,459,200 depth and 1,536,000 color comparisons;
14,029,744 host EFB samples under sanitizers; 92,101 ARM GX assertions; and the
full graphics fixture. The former raw depth-to-CPU transfer is still rejected
by its negative control. Source reuse checks remain byte-audited in emulator
integration. These passes do not establish physical cache/driver correctness.
Additional checks cover 62,130 large-indexed ARM assertions, 7,168 exact GPU
channels, 32,701 ARM sphere-lighting assertions, and 163,840 rendered lighting
channels including independent RGB/alpha variation and shader-clear lifetime.
I8 upload checks add 19,252,736 exact pixels under address/undefined-behavior
sanitizers. In the matched emulator window, the changing depth texture retained
21 uploads and identical hashed-byte counts while inclusive load time fell from
9.097 to 4.421 ms/call. Whole CPU time was 58.807 versus 52.388 ms with slightly
different weather/animation vertices. See `palace-i8-barrel/window-comparison.json`.
That longer run reached its host deadline at retrace 3,456, drained and exited.
It exercised 210 successful allocation retries (190 geometry/20 texture
evictions) without a panic. Sampled normal RAM plus stack peaked at 60,240,264
bytes, linear at 20,971,296 bytes, VRAM at 3,620,864 bytes. These are sampled
moving-scene maxima, not full-game guarantees or console performance evidence.

Latest opening check `title-compact-material-key` uses emulatorda0d791e0f82,
completes80retraces and drains shutdown. Image60 was inspected and is
byte-identical to the preceding material candidate. It remains slow:
matching renderer34-71 median CPU101.822ms versus135.9255ms before these
decoder/alpha/visibility/material/admission changes, with169draws/39547vertices. These are
custom-emulator timings, **not console FPS estimates**. The checked Palace
decoder run reached2204retraces before its380-second bound, drained shutdown,
and matched143,050vertices. It carried and normally respawned a barrel but
did not clear a wall. Sampled normal RAM plus reserved main stack peaked at
60,275,832/71,303,168bytes; linear20,970,432/25,165,824;
VRAM4,145,152/6,291,456. This leaves about15.5% normal headroom in that run;
later areas and peaks between samples are still unvalidated.
The broad queue/geometry fixture passes4025prefixes and retirements plus its
existing exact pixel/mutation/buffer-reuse assertions. Hardware speed and
queue/cache behavior remain unvalidated for this candidate.

## Memory and remaining hardware questions

Normal RAM reservation is 68 MiB, linear RAM 24 MiB, and main stack 512 KiB.
Admission includes reserved stack space and retains at least 15% normal RAM
headroom. Linear allocation keeps 4 MiB free. CPU retained geometry uses a
4 MiB normal arena; native GPU geometry has a separate 6 MiB linear cap.
Texture storage has a 10 MiB cache limit inside its 12 MiB budget.

The new shadow cache has eight slots, a 512 KiB VRAM cap and a 1 MiB free-VRAM
reserve. The game's eight persistent R4 images need 483,328 bytes when all are
resident. The matched Palace scene used 131,072 bytes. Calibration temporarily
uses another 8 KiB linear and 4 KiB VRAM, checked before allocation. Latest
sampled normal RAM plus stack: 59,898,112 / 71,303,168 bytes; linear:
19,789,152 / 25,165,824; VRAM: 3,620,864 / 6,291,456. Longer previous gameplay
samples reached about 60.1 million bytes of normal RAM plus stack and
20.97 million linear bytes. Samples are not whole-game high-water guarantees.
The ordinary shader uses 425/512 instructions, 93/96 float uniforms and 16/16
boolean uniforms. The separately bound distortion program uses 210/512
instructions, 89/96 float uniforms and no boolean uniforms. The programs are
not concatenated into one instruction store. Larger indexed staging adds
151,256 bytes of normal memory, included in admission accounting.

GPU depth sampling requires an exact runtime admission check on console.
The original OpenGL emulator depth-copy rectangle bug is reproduced and
rejected; an isolated, recorded two-coordinate emulator fix agrees with the
independent Vulkan depth fixture. Shadow quantization remains separately
calibrated. CPU depth reads retain the proven opaque snapshot and flags-zero
DisplayTransfer. Bottom LCD blanking is intentional. Physical CPU/GPU timings, texture cache visibility, worker affinity,
DSP sound, SD service latency, launch entitlement and suspend/resume need checks.

Normal progression has reached the Palace and saved gate/crate progress.
Barrel pickup and travel down the ramp have been observed, but the cracked wall,
Krazoa spirit and first Fox area remain unverified. Playability, three final cold
boots, repeated transitions and an extended gameplay run remain outstanding.

## Prepared movie and next console measurement

The private `SD:/3ds/foxhollow/starfox-pica.thp` replaces runtime JPEG decoding
with the prepared 256x192 ETC1 version. It retains all 1,797 frames and original
audio payloads, with image-compression loss. Size: 33,206,080 bytes. SHA256:
`d1ebce5e78e095414f31df9083215142f2ebc1618a47f7474d74a49a29124ef1`.
See `PREPARED_MOVIE.md`. Missing/rejected data falls back to the original movie.
The native executable reads the private ISO through its filesystem adapter;
Azahar emulates that 3DS executable. Original RVZ, assets and saves are preserved.

After startup recovery has been established, launch normally after
exiting ftpd; a reboot is unnecessary for this focused measurement. Leave the
opening untouched for about 60 seconds, note pauses or visible/audio corruption,
press Select, then reopen ftpd. Exit drains buffered logs. There is no need to
reach Fox or assess fur in gameplay for this opening check. Compare matching
opening frames, SD timings and component counters against the collected console
baseline. The last physical asset-read span was 486.4 ms (about 5.9% of that
window); actual latency for prepared playback remains unmeasured.

A file containing `0` named `early_queue.txt`, `efb_depth_gpu.txt`, `gpu_sphere.txt`, `efb_shadow.txt`, `audio_worker.txt`, `efb_color.txt`
or `efb_depth.txt` disables that feature for fault isolation. The shadow
calibration mode is also recorded in `native.log` and `memory.jsonl`.
Installation must preserve saves and collect previous logs first.

## Build and prepare the SD card

From the configured workspace, build the physical-console variant:

```powershell
wsl -d Ubuntu -u root --exec sh <repo>/tools/linux_exec.sh python3 /work/tools/build_native.py
py -3.13 tools/package_console.py
```

The package is written under `build/console-validation/`. Extract its `3ds`
directory to the SD root. It contains `3ds/foxhollow/foxhollow.3dsx`. The console
build uses real NDSP initialization, physical controls and the GPU fur candidate;
emulator replay and the test DSP component are absent.

Copy the already verified private `assets/GSAP01-rev1/disc.iso` to
`SD:/3ds/foxhollow/disc.iso`. Its size is 1,459,978,240 bytes and SHA256 is
`f5e5a9a29147f80648dbef00ed68cd84d0f08c7120645e6f653333296944facf`.
The selected disc is Europe/PAL GSAP01 revision 1. No extracted asset directory
is needed at runtime. Preserve the original RVZ. Conversion evidence and the
original image hash are in [DISC_BASELINE.md](DISC_BASELINE.md).

NDSP requires the console's normal `SD:/3ds/dspfirm.cdc` setup. Neither firmware
nor game data is included in this package. Start `foxhollow.3dsx` from the
Homebrew Launcher. The port requests New 3DS fast mode and, where supported,
records Luma clock/max/L2 readback in its memory telemetry. Valid slow readback
triggers one bounded configuration retry. Unknown readback remains explicitly
unverified. Clock behavior and launch-mode memory entitlement still require
physical validation.

## Controls and saves

| New 3DS input | GameCube input |
| --- | --- |
| Circle Pad | Left analog stick: movement/aiming |
| C-stick (New 3DS) | GameCube C-stick: items menu |
| A, B, X, Y | Matching face button |
| L, R | Matching shoulder button with full analog trigger |
| ZL or ZR | Z |
| Start | Start/pause |
| D-pad (New 3DS) | Matching GameCube D-pad direction |
| D-pad (Old build 9793b9823cba onward) | C-stick directions for the items menu |
| Hold touchscreen + D-pad (Old) | Original GameCube D-pad, including world-map controls |

The analog deadzone and octagonal clamp follow the desktop Foxhollow input
contract. Button actions remain game-dependent. Rumble is unavailable. Gameplay
usability of the small C-stick and the digital shoulder-trigger mapping remains
to be checked during normal play.

GCI saves live in `SD:/3ds/foxhollow/saves/`. The game's save identity is GSAE/01
even with PAL assets. Back up that directory before a development test. A raw
GameCube memory-card image is not accepted by the native adapter. Component
tests cover replacement/backup recovery, but a title-menu slot is not evidence
of meaningful gameplay persistence.

## Record the physical result

Keep the package manifest and record console model, system/homebrew environment,
SD card, launch route and clock settings. Follow the candidate-specific launch
instructions above; cold boots remain part of final acceptance. Check the
screen, controls, intelligible audio, and whether the title/new-game sequence
continues. Current rendering is very slow and unresolved effects can still stop
execution. Copy `native.log`, `game.log`, `memory.jsonl`, `render.csv`, `render-profile.csv`, `sd-io.csv`, `audio.csv` and any
`panic.txt` from `SD:/3ds/foxhollow/` after the run. These files are overwritten
by subsequent boots; keep each run in a separate host directory.

`render.csv` includes `interval_ms`, elapsed time between rendered-frame
submission endpoints. Exclude the first zero interval and calculate aggregate
FPS as 1000 times the number of intervals divided by their sum. This includes
game work and waits between rendered frames. The older `wall_ms` field measures
renderer work only; its reciprocal is not the full-frame rate. GPU timing is
retired after the associated queue completes. With early prefixes enabled,
`gpu_ms` is the remaining queue tail, not total GPU execution. The console
analysis tool detects and labels this mode from `native.log`. Logging uses six 64 KiB ordinary
RAM buffers plus a 64 KiB worker queue and some SD I/O; measurements include
that instrumentation overhead.

Physical acceptance remains open for GPU/cache correctness, SD latency, DSP
stability, available RAM, verified CPU speed and sleep/resume. Once the opening
is playable, validate three cold boots, movement/camera/combat/interactions,
inventory, essential cutscenes, repeated area transitions, a meaningful save
followed by full exit/reload, and an extended gameplay session. Record actual
frame pacing separately from emulator timings. Sleep/resume and interrupted
save behavior need separate checks with backed-up saves.

The renderer now uses a 64 KiB linear index buffer with retained GX vertex
identities. Physical validation must include sustained geometry and buffer
rollover, checking for missing triangles or stale vertex data after transitions.
Azahar pixel comparisons cover the indexed path, but cannot establish physical
GPU cache visibility.

The fixed reservations are 68 MiB normal RAM, 24 MiB linear RAM and a 512 KiB
main stack. Texture storage has a 12 MiB budget with a 10 MiB cache limit. The
6 MiB VRAM pool must retain headroom. Runtime logs report sampled use; they do
not prove peaks between samples or a sustainable CPU/GPU workload. Do not treat
an emulator pass, an overclock or a menu boot as physical gameplay acceptance.


Installed and final readback-verified 81618a4a2487 atnew223:5000,
/3ds/foxhollow. Evidence artifacts/console-test/20261005-161721/transfer.json.
Six payloads verified:3612484-byte3dsx,build manifest,README,attribution16,
4549144-byte private model-lod.bin(bb9e6d26eaa9),680184-byte private mesh manifest.
Previous f1f executable/metadata/config retain .backup-20261005-161721 suffix.
Saves and original RVZ/ISO/movie untouched; no saves removed. Asset-free immutable
package foxhollow-validation-81618a4a2487.zip SHA256
cc75f5f0c1b1f1d7cedf7caf72210fd067347852d88b4aa943cf8f9368755f5e.
Physical performance and busy Palace material workload remain to be measured;
steady30/60FPS and complete opening progression goal remain unmet.

6 October candidate33020f1a3886: native world texture pointer repair, alternate
model coverage, diffuse light maps/sky and model clipping. Requires private
native-scene.bin v4 SHA fca644fa0614132d4dc723178f82f5d47f9bbbca66445d8e5620e218de8109b2.
This is an incomplete graphics development build. Exact console executable
passed silent Azahar normal saved Palace load and Select exit; textured floor
captures inspected. Layered/two-UV surfaces, weather and secondary effects remain
incomplete. Do not use missing-geometry FPS as playable performance evidence.
Launch normally, no reboot required; preserve saves. The next hardware check is
whether the formerly white/untextured Palace surfaces are now visible, and
whether restored work introduces stalls/crashes. It is not a final gameplay gate.
Installed and final-readback verified33020f1a3886: console-test/20261006-123934,
10files includingv4native-scene.bin. Saves preserved. Physical check pending.


7 October local candidate: console8b44eb64a793 now tested with paired scenev9
adcbe1b8d3d2 and model-lod-alpha-coverage82412755d509. Install those private files
as native-scene.bin and model-lod.bin together; keep original texture/movie data.
Cloud coverage was incorrectly removed by the former simplifier; headless opening
and actual-console-binary Palace captures now show clouds and rain. Asset-free
package does not contain these private assets. This is not shrine acceptance or
physical FPS evidence. Oldconsole0fa2e9871f49 still cannot fit startup40MiBnormal
heap and must not be installed as a working Old port. .229 not ready. .223 remains
installed1d/v6 awaiting the user's previous check; no upload/poll in this turn.


7October region-budget builds: New3b6bfc67be9d passes exact-console short movie/
Select regression. Oldb73015db9526 uses19MiBgamearena and46/6MiBheaps, boots in
Old-mode Azahar and loads Palace, but severe texture reloading misses the bounded
Palace test deadline. It remains unsuitable for gameplay deployment; physical
Old loader entitlement is unverified. No .229 setup or .223 changes this turn.
package_console.py --old-3ds creates a separately named asset-free development
package; it does not mean Old playability or installation readiness. Both need
paired topology82412755d509 and scenev9adcbe1b8d3d2. Details IMPLEMENTATIONtail.
