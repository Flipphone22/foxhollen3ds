# Old 3DS audio service placement candidate

The existing Old palace profile places the mixer on core 1 but the NDSP service
thread on core 0. Its auxiliary/reverb processing and buffer publication therefore
still consume the game's core. The service is already concurrent with the game;
this candidate changes affinity rather than introducing a new asynchronous owner.

`audio_service_core_patch.py` shares one verified CPU grant between NDSP service
and the mixer. NDSP starts first. The latched startup choice requests 30%, reads
it back and selects core 1 only on success. New 3DS retains the SDK default
service processor. Core-0 selection and failed grant retain core 0. Failed core-1
thread creation retries on core 0, preserving audio availability and reporting
actual selected affinity. No additional stack, queue or worker is allocated.

Pinned Melee audio.c uses NDSP ownership and publication; it does not provide
this Old-core migration. This is a Foxhollow-specific adaptation of the existing
SDK service thread. Its priority, stack, callbacks, DSP IPC, effects processing,
sleep/reset and shutdown paths remain unchanged. The original mixer queue is
also unchanged. The grant is shared, not an additional 30% per thread.

Host ASan/UBSan executes the actual patched grant and thread-creation paths:
960 checks covering Old/New, selected core, failed set/read, grants 0/29/30/89,
repeated queries and creation failure. This is not scheduling or audio proof.

Build with `tools/build_audio_service_core.py console-setup --revision v1` using
the existing Linux wrapper; it isolates the archived hot-bank build and restores
root work. Profile using `tools/profile_audio_service_core.py --system-core
--memory-calls`, then audit actual mixer/service execution before interpreting
core totals. DSP-LLE/profiling elapsed time is excluded from performance claims.

Promotion requires useful core-0 relief without starvation, corruption or lost
audio. Azahar cannot establish physical core-1 quota headroom. A substantive
candidate may need one matched console comparison; no request is made yet.

## Integrated local evidence

17 October OLD NDSP SERVICE CORE migration; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(nativehotbank). This turn PROGRESS(exactsourceprofile,
actualgrant/creationtests, sharedcandidate and runtime executionmigration proof).
Full objective read. NoFTP/deploy/assets/save/preferenceschanges. OldSDc8acfac0d348
unchanged; physical6d78487still23.0011FPS/43.4762ms shortpalacewindow.

Local unified-audio-service-core-v1-console-setup-builds sourcec4799be03351
NOTDEPLOYED/NOTPROMOTED. Two changedinputs platform_audio_worker.c and
platform_audio_game_sdk.c. SharedOld80/New124/recipe/gamearchive/all20shaders
unchanged versus586b hotbankparent; root/archive restoredPASS. No addedthread/
stack/buffer. OldNDSPservice joinsmixer on verifiedcore1(30%sharedgrant), keeping
existingpriority0x18,stack0x1000,callbacks/DSPIPC/locks/sleep/shutdown. Grant
requestedonce/readbackcached beforeNDSPstarts; Core0selection/failedgrant0,
Newservice remains-2. Failedcore1creation retriescore0/logsselectedaffinity.
PinnedMeleeaudio.c NDSPownership consulted; thisOldmigration islocaladaptation,
notMeleebehaviour. ASanUBSan960actualgrant/creationchecksPASS.

Profileunified-audio-service-core-v1-benchmark-old-dsp-core1-memory-profile-v1
122.78hostsec executionPASS,benchmark1,0missingactors,state/save/drain/ownedexit.
RawSVC11telemetryfailed retained; exactELFprofileprovesmix_once ANDndspThreadMain/
fh_audio_dsp_aux_service/reverb executeonlycore1. ParentthoseDSPfunctions core0.
Maincore17930136->16186986encounters(-9.7219%),core14099930->5585796(+36.2413%).
BothsampledPCMunderruns0,source/callbackfaults0. ThisisoffloadNOTwholeframeFPS;
physical30%quotaheadroomUNPROVEN. Audio/particlephase differs;capture1601shows
palaceworld/purplecreature/HUD butcameramoved/playermostlyoffscreen versusparent,
NOTmatchedimageparity. Important don'tclaimfixedcameraoridenticalworkload.
Memoryheap41363616/49283072,linearfree3529088/12582912,VRAM4723712,
stack72984/524288,process70660096/83886080. Sceneobservationsonly.

Sourceprofileexistinghotbankexpgfx_updateActivePools1462929encounters exactMD5
 a198fd08f70d07b0103654c50ea08506. Distributedlifetime/physics/colour/bounds,
211347line0. DidNOTskip simulation. analyze_arm_source nowacceptsindependentaudio
executionaudit with exactELF/window/capture/save/drain guards, nofakepassfile.
No newunchangedboot forlineanalysis.

Tools audio_service_core_patch/build_audio_service_core/test_audio_service_core/
profile_audio_service_core/compare_audio_service_core; docs/NATIVE_AUDIO_SERVICE_CORE.md.
Evidenceaudio-service-core-test/{result,comparison}. Owned62668sourceanalysis,
71396build,29663run,34489audit allTERMINAL. No hardware request thisturn.
Next isolate service-affinity change against c8ac hardware-baseline renderer
before console comparison; don'tbundleunvalidatedhotbank/lifetimeintoan audio
performanceexperiment. Need physicalquota check eventually; notblockerforlocal
work. Fullrendering/performance/progression/endurancegoalstillINCOMPLETE.


## Isolated console candidate

17 October OLD SERVICE-CORE CONSOLE CANDIDATE DEPLOYED; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(integrated offload). This turn PROGRESS(isolated baseline
build, exactbinary runtime, package and explicitly authorized Old installation).
Fullobjective read. No New223/replay/privateasset/ISO/save/settings changes.

OldSD now9c33896214fc,3dsx0f89f92f6ff5,CIA6a30f59b18df, uploaded/readbackverified
in20261017-170846-old229-direct-install COMPLETE. Sixnamedpayloads staged
fullbyteverified then published withbackups; finalmanifestverified,singlebulk
readback. CIA must be installedviaFBI forHOMEicon; uploadisNOTinstallation.
Remote SD:/3ds/foxhollow/foxhollow.cia andfoxhollow.3dsx. Package75b60ce1261a5ced.
No hardware execution ofnewbuildyet. Latestphysical remains6d78487cce32
23.0011FPS/43.4762ms shortpalacesample; no newconsoleFPSclaim.

Candidate unified-audio-service-console-v1-console-setup-builds uses c8ac
hardware-baseline renderer, notunpromotedhotbank/lifetime. ONLYtwoaudiofiles
changed via same tested audio_service_core_patch.py. Allshaders/gamearchive
324cf41d6468/recipe/Old80New124 unchanged; root/archive restoredPASS.
Actualpatched sourcehashes matchASanUBSan960grant/creationcases. Newservice-2,
Oldrequested/readback30 bothmixer+NDSPcore1, no addedthread/stack/buffer.

Exact Old run unified-audio-service-console-v1-benchmark-old-dsp-core1-memory-
profile-v1 executionPASS120.12hostsec/benchmark1,save/settings/drain/ownedexit,
0missingactors. RawSVC11telemetryfailure preserved; independent exactELF trace
provesmix_once/ndspThreadMain/aux/reverbcore1. Bothbaseline/candidatesampledPCM
underruns0,source/callbackfaults0. Core018206685->16224306(-10.8882%encounters),
core14313269->6193750(+43.5976%). NOTFPS/physicalquotaheadroomproof. Totalwork
is redistributed;30%grantsharednotperthread. Physicalscheduling mayerasegain.
Capture1601inspectedKrystal/floor/sky/HUD; camera pointsawayfrompalace, NOT
matchedimageparity. Particle/audioactivitydiffers; don'tclaimsamenormalframe.
Memoryheap41364432/49283072,linearfree3591424/12582912,VRAM4723712,
stack72984/524288,process70647808/83886080. No worstcaseheadroomclaim.

Limitedhardware-test review retainsRUNTIME_REJECTED/generalreleasefalse; one
freshcore1run plushonestlyinheritedcore0gameplay andexactpatchedhostaffinity
checks. No manufacturedcore0candidateexecution or repeatedunchangedboot.
Userexistingauthorization forOldperfbuilds/hardwaremilestones recorded.
Toolsbuild/profile/compare/review_audio_service_console reproduce; comparison
in audio-service-core-test/console-comparison.json. docs/NATIVE_AUDIO_SERVICE_CORE.md.
One readonlyFTP readinessloginpassed beforewrite. Allowned31237build,5173run,
56257audit,74847transferTERMINAL. No activeownedprocess/blocker.

User next: installCIAwithFBI, existingstartupmenu palacebenchmark CORE1(30%)
default, letfinishautomatic, reopenftpd, reportaudiostutter/hang. Norebootneeded.
Need consoleanswer onsharedquota, notanotheremptyprobe. Continuelocal native
performance/visualworkwhileawaiting; fullgoalstillincomplete.



## 17 October: owned service lifetime

The `unified-audio-service-lifetime-v1` candidate is based on deployed
`88915fc75528`, not the unpromoted enlarged-stack experiment. Only the archived
`platform_audio_game_sdk.c` changes. Renderer/game payloads and Old80/New124
profiles remain unchanged.

The previous service was detached but later joined through its stored pointer.
The pinned SDK can free detached storage before that join. Its `finished` flag
also precedes OS thread retirement, and `threadJoin` returns immediately for
that flag. The exact deployed ARM disassembly confirms this early-return path;
see [libctru v2.7.0 thread implementation](https://github.com/devkitPro/libctru/blob/v2.7.0/libctru/source/thread.c).

The candidate retains an owned thread and caches its live kernel handle. Stop
publication precedes a kernel wait; only successful retirement permits thread
storage release, followed by DSP teardown. A failed wait retains the resources
and reports failure. Start publication now occurs before creation, avoiding a
late-started service overwriting a stop. Failed primary/fallback creation returns
an error rather than the preceding initialization's success result.

This adapts pinned Melee's explicit thread ownership/join/free approach, with a
stronger kernel-retirement wait for this SDK's finished-flag behavior. CPU core,
priority, 4 KiB stack, callbacks, queues and sample processing remain unchanged.
The cached handle costs four static bytes; acquire publication adds a barrier to
the service loop. No measured FPS gain is claimed. No GPU or asset work changes.

Host ASan/UBSan exercises thirteen creation/retirement schedules, including
finished-before-retirement and failed kernel waits. Detached ownership, omitted
retirement and false initialization success mutants all fail. The linked ELF
verifier checks wait -> free -> DSP unhook, archive identity, shader identity,
CIA grants and root restoration. These tests do not emulate the entire DSP.

This is a confirmed lifecycle repair, **not an established explanation or fix
for the physical save-load crash**. The physical fault occurred on a stack push
in `ndspiReadChnState`; no evidence yet shows shutdown was in progress. Keep that
failure open and do not deploy this under a claim that save loading is repaired.

17 October NDSP STOP WAKE repair; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(actor palette run publisher/build); this turn PROGRESS
(service shutdown wait repair, hostfailure/race gates, sharedbuild/exactsave).
Local04cbdc859598 unified-audio-service-stop-v2-console-setup-builds NOTDEPLOYED.
Parent26b954 contains prior local CPU changes. UploadedOld5052 unchanged.
Physical88915 26.40484FPS/save-load crash stillOPEN; NO newFPS or crashfix claim.

Confirmed independent shutdown defect: service waits indefinitely on IRQ/sleep,
owner setsrunfalse thenjoinswithoutwaking. New ownedsticky stop event is first
in service-only waitN; stop also wakes sleepEvent. Sync andloop recheckstop
before consumingstate/callback/publication. Initialization/finalization IRQ
waits retain original singlehandle semantics; MUSTNOT use uncreated/closed
stopevent there. Service handles remainowned throughsuccessfulkerneljoin,
thenfree andeventclose; failedjoin retainsall. No artificialDSP interrupt.
PinnedMelee log_io wake-before-retirement and existingSDK owner consulted.
Core1/priority/4KiBstack/samples/effects unchanged; one additionalkernel event,
4Bhandle +60Blinkerpadding=64B BSS, code+264B. Steady service uses two-handlewait
and stopchecks; kernelcost notmeasured, no zero-overhead claim.

Actualwait/sync/loop ASanUBSan8normal/stop/sleep/init schedulesPASS,3mutantsreject.
Actualcreate/stop owner14failure/schedulecasesPASS,6mutantsreject including
omittedIRQ/sleepwakes andprematureclose. Hostscriptedevents NOT physicalsleep
proof. Initialpatch rejectedambiguousfail3beforechanges, fixedspecificcontext.
Ownerfixture initiallymisroutedoutput intooldlifetime-test: preservedmisrouted
files, correctedpath, regeneratedoriginal13caseproof andverifiedsamearchived
sourceSHA. evidence-repair.json records scope. Firstbuild v1 stoppedbefore
rootmutation onmissingtestpath; successfulv2 restoresroot+gamearchivePASS.

Onlyplatform_audio_game_sdk.c changes; exactpurepatch/testSHA verified,
gamearchive433ed3e4ffb9/all18GPUprograms/setuprecipe unchanged. Old80/New124.
BSS3266864/data178996, onlynewBSSobject ndspStopEvent4B; text3778156.
NormalOld exactprivate-save run113.56hostsecPASS nativeDSP/LLE,save/settings
preserved,Selectdrain/ownedexit,missingactors0. Loaded1601 inspected Fox/
ThornTailHollow/world/textures/HUD. Notplaythrough/fullparity/performanceproof.
Heap40366688/49283072,linearfree2484352/12582912,VRAMfree4723712,
mainstack73044/524288,process70656000/83886080; samplednotworstcase.

Read-only linkedservice prologue inventory audio-service-stack-audit doesNOT
prove4KiBexhaustion or faultcause; unknownallocationbounds/callbacks remain.
No speculative stackincrease. Originalstackpushfault and physicalacceptanceOPEN.
Owned26170audit/49452failedpatch/2196sync/3693+92143+71249owner/19180build/
23359verify/83896runtime allTERMINAL; noFTP/privateasset/savechanges.
Evidence audio-service-stop-test/{result,build-verification,acceptance}.json,
audio-service-stop-owner-test, tools audio_service_stop_patch/test/build/run/
verify. Fullrenderparity/performance/progression/endurance remainINCOMPLETE.

