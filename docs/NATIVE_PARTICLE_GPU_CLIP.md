# Static particle clipping on PICA

Experimental, not deployed. Parent is88915fc75528, not the rejected direct
member-packing candidate. The native static particle stream no longer evaluates
four CPU planes or compacts surviving instances. It publishes all supplied
static particles in original order to the unchanged VSH/GS; PICA performs its
normal homogeneous clipping. Dynamic trail bounds/culling remain unchanged.

This is a CPU/GPU workload tradeoff, not a free optimization. Source particle
simulation, random-number calls and producer values are unchanged. Previously
CPU-rejected particles add GPU work, command pressure and streaming traffic.
No shader instructions, render passes, allocations or threads are added. Ring
capacity, acquire/wait, dirty ranges, flush, retirement and outstanding-lease
guards remain. Shape validation remains in commit. Pinned Melee command_cache.c
explicit publication ownership was consulted; no cache-coherence change.

The host actual-commit test compares against full-particle reference publication
with always-visible admission. Both emit396954 particles with identical ordered
digest through ring rollover, disabling and material/flush boundaries, under
ASan/UBSan. This is not an old-CPU-cull output-equality claim: those removed
records are intentionally submitted now. Raster correctness and physical GPU
cost need separate evidence. Initial test scaffolding lacked its archived header
and still required nonzero CPU culls; corrected before a successful result.

Reproduce with test_particle_gpu_clip.py, build_particle_gpu_clip.py,
profile_particle_gpu_clip.py and compare_particle_gpu_clip.py. Only the archived
native owner is changed; game archive, shaders and shared Old80/New124 profiles
must remain unchanged. Existing gameplay timing and parity targets remain open.

## Runtime tradeoff

Shared66bc68be06b6 completes Old palace with state/save/drain/owned exit and
zero missing actors. Capture1600 inspected; camera differs from parent, so not
full image parity. Actual execution confirms core1 audio despite unsupported
core-ID telemetry. Commit encounters412378->50624, while totalmain16017300->
16313503. Candidate lightning activity differs; no causal full-frame timing.
Whole-run particle quads231072->284837, draws6256->7351; culls/compaction now0.
No ring wraps/overwrite drains. GPU cost remains unvalidated physically.
Retain locally; not deployed or promoted as an FPS improvement.

## Profile-window scene qualification

profile_scene_alignment.py now checks actual replay.csv state at frames1301..1308,
not the later diagnostic screenshot. Palette->GPUclip differs by up to4.66275
simulation ticks; camera axis differences are0.05/0.0092/0.00135 worldunits,
yaw6/pitch1 encodedunits. Inputs agree; draw/indices differ by up to8/1020 per
frame. These are recorded differences, not a cause-of-divergence proof. Existing
CPU counts remain valid observations but not matched-workload FPS evidence.
The shared ARM comparator includes this qualification; matching recorded fields
alone cannot prove complete RNG/particle/audio/animation equivalence. No new
emulator run or change to game RNG/simulation was used.


## Combined candidate

17 October COMBINED COPY / GPU PARTICLE CLIP; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(fullwindow copy comparison); this turn PROGRESS(compose
tested particle owner with current copy/lifetime/lightning repairs, build/runtime).
Local 7072670f11e2 unified-command-particle-clip-v1-console-setup-builds NOTDEPLOYED,
experimental. Stable localcopy7d8e and installedOld5052 remain separate; noFTP.
Physical latest88915 26.40484FPS/37.87184ms; save-load crash stillOPEN.

GPUclip owner is byte-identical to alreadytested66bc candidate; original7d8e
particleowner byte-identical to88915. Reused ASan396954orderedpublications;
no redundant host rerun. Native static particles now allpublished in sourceorder,
removing CPUfour-plane tests andcompaction; unchanged PICA VSH/GS clips them.
Dynamictrail culling, shapechecks, leasebounds/dirtyranges/cache/fences retained.
No newshader/worker/allocation, no sourceRNG/simulation change. PinnedMelee
command_cache.c explicit publication reviewed. Copyoptimization retained.
Gamearchivea336c7fd3bd1/allGPUprograms/setuprecipe/Old80New124 unchanged;
onlyplatform_pica_glow.c deltafrom7d8e. Source/archive restorationPASS.

EarlierGPUclip profilecommit412378->50624 butwholemain+1.85% withlightning/phase
differences; NOTrejected ascausallyslower. ExtraGPUquads/draws remaintradeoff.
Current Oldcorrected-emulator runPASS91.68hostsec,
core1audio/grant30/layoutfault0/missingactors0/saves/settings/drain/ownedexit.
Capture showsKrystal/platform/sky/HUD; particlesoff-camera, NOTparticlevisual
acceptance. No pairedsame-phase image/fullparity orphysicalcost claim.
Detailed particlecounters andsampledmemory: command-particle-clip-test/result.json.
No extra fullprofile runs: existing unchanged commit code evidence reused.

Physical reference's133measuredsamples/132intervals GPUcounter14.17460ms/frame;
overlapsCPU andincludesqueue/timestamp limits. NOTaGPUlinear costmodel, notnew
measurement, andnot permissiontoassumeNew60FPS headroom. LocalGPUclip remains
experimentaluntilvisual/performanceacceptance. No consoleprobe requested.
Allowned99547build/84023runtimeTERMINAL. Next evaluatecombinedcost andremaining
largerparticleproducer costs, without another unchangedprofile. Fullgoalopen.



## Linked geometry exclusion check

17 October PARTICLE GPU CLIP GEOMETRY gate; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(crash tooling); this turn PROGRESS(linkedshader geometric
admission check). No new productionbuild/deployment or physicalFPS claim.
Reviewed latest5d965a actor lighting: already prepared state; no speculative
pointer cache/extra selector cache added. Revisited meaningful CPU-to-PICA
static-particle clipping experiment707267, not another unchanged runtime boot.
New test_particle_clip_geometry.py runs exact707267 linkedVSH4/GS5 against
originalC geometry using capturedcave camera/templates and systematic translations,
all4shapes/2templates. Native conservative-plane/bounds expressions guarded
against source drift. 448syntheticextensions/2688emittedcorners PASS;239CPUrejects
all have entire emitted geometry outside a common GPU sideplane. Position-shift
mutant rejected. No game state/input/RNG mutation, no emulator game restart.
This proves sampled geometric exclusion only; no raster, physicalprecision,
throughput or universal clipping proof. ExtraGPUdraw/vertex tradeoff staysOPEN.
Initialv1 failed source-vsVMalpha by2bytelevels for synthetic behindcamera case33.
Preservedfailure; v2/v3 explicitly geometry-only, NOTclaimed alpha acceptance.
V3output says alpha_not_checked; doesnot report zeroerror as alpha validation.
Existing actualcaptured8particlealpha evidence unchanged. No alpha clamp/fix.
Evidence artifacts/native/particle-cave-state-v5-new/clip-geometry-v{1,2,3}/.
GPUclip remains experimentalNOTDEPLOYED; latestnormal5d965a unchanged.
Physicalsave-load crash OPEN; no newFTP evidence/retry. Owned70886 terminalfailed,
v2/v3commands completed. Fullparity/performance/progressiongoal INCOMPLETE.
Next actual raster/performance acceptance before composing GPUclip with latest
particle lifetime owner; shaders/ABI differ across experiments, neverblindmerge.



## Current application composition

17 October CURRENT GPU PARTICLE CLIP composition; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(geometric gate); this turn PROGRESS(shared app composition,
verified archived delta/build, exact Old229 save runtime/capture). Localcandidate
 e2640b69be75 unified-current-particle-clip-v1-console-setup-builds NOTDEPLOYED.
Parent5d965a retains latestCPU/NDSPstop fixes. Sourceonlyplatform_pica_glow.c:
staticparticles submit without CPUplanes/compaction; dynamictrail unchanged.
Correct previous caution: actualcomparison proves707267/currentparticle ABI and
VSH/GS sources BYTEIDENTICAL. Lifetime experiment was separate/unpromoted;
do not assume every newer archive contains it. Testedowner also byteidentical.
Reused actualowner ASan ordered-publication test and448case geometry gate.
Fullsource purepatch exact,gamearchive433ed3e4ffb9/18GPUprograms unchanged;
Old80/New124/setuprecipe/restorationPASS. Text3778420->3777988(-432B),
data178996/BSS3266864 unchanged. No newallocation/shader/worker.
Oldexactsave nativeDSP/LLE boundedrunPASS105.46hostsec, no missingactors/layout
faults, preservedprivatefixture/save, Selectdrained/ownedexit. Loaded-later1750
inspectedFox/ThornTailtrees/dinosaur/Arwingedge/HUD, sparklesleft. NOTsamephase
raster acceptance or hardwarecrashrepair. Static150342particles direct,
compacted0,wraps0,overwritedrains0; peak246/frame,4978draws. GPUcoststillOPEN.
Heap40366688/49283072,linearfree2484352/12582912,VRAM4723712,
mainstack73044/524288,process70656000/83886080 (same prior exactsave samples).
DSP LLE/diagnosticrun NOTFPS comparison; no newperformanceclaim. NoFTP/request.
Evidence current-particle-clip-test/{build-verification,runtime}.json;
tools build/verify/run/record_current_particle_clip.py. Build39625/runtime13749
and verifierTERMINAL. Root/archive restored. Fullgoal and physicalsavecrashOPEN.
Next actualsamephase raster/GPUcost gate; do not repeat unchanged profileloops.



## Same-input raster comparison

17 October SAME-INPUT PARTICLE RASTER gate; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(current sharedbuild/exactsave); this turn PROGRESS(local
actual linked GPU raster comparison and missingdraw negativecontrol). No new
productionbuild/deployment/FTP or hardware request; locale2640b69be75 unchanged.
New build/run_particle_clip_pixels.py +particle_clip_pixels.c draw currentELF
particleVSH4/GS5 with8capturedcamera/templates and448synthetictranslated/shape
variants, proceduraltexture and3blend modes. CPUcull vsallsubmitted paths use
identical inputs/order. All24 400x240 RGBA comparisons BYTEIDENTICAL;239CPUrejects,
24nonblankcases. Modes0/2 visuallyinspected: texture/alpha layers visible.
Droppedallcandidate-draw mutantFAIL96000pixels; expectedfailure accepted only
for thatexplicit mutant. Normal v2 rebuilt after mutant-switch addition PASS
2.96hostsec; v1PASS11.75hostsec. Theseare diagnostic startup times, NOTGPUcost.
V2images byteidentical toinspectedV1. Failurehistory: firststrictARMcompile caught
readback pointertype; corrected alignedu32 cast, no warning suppression.
Original private captured camera data stayslocal; no assets shipped/probe upload.
Diagnostic readback only afterretirement; no rawdepthreadback/gamepath change.
Evidence artifacts/diagnostic/particle-clip-pixels-{v1,v2,drop-v1};
current-particle-clip-test/raster.json records exactsource/ELF/shader hashes.
This supplies sampled same-input raster equivalence previouslymissing. It does
NOT prove originalgamefidelity, allrenderstates, physicalGPUcost or crashfix.
CPUadmission equations copied from guardedsource, not entireproductionowner;
existingactualownerpublication tests remain separate. No timings combined.
Allowned13824failedcompile/78559v1run terminal, finalbuild/mutantrun/v2run complete.
Physicalsave-load crash OPEN; fullparity/performance/progressiongoal INCOMPLETE.
Next measurecombinedwholeframe tradeoff or substantivehardwarecandidate when
available; don'trepeat unchanged geometric/raster tests orclaimFPS fromcounts.

