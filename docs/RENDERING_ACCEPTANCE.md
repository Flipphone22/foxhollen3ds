14 October current status: rapid whole-shadow flicker is user-confirmed FIXED.
Normal source06685e13fd29 (height-fog-post-stage-v1) deployed/readback-verified
New223; matching replay remains local. Saves and graphics settings unchanged.
World fog now follows all four material stages with explicit actor-fade handoff.
CPU/GPU pixel tests and New cave replay pass; timing comparison excluded for
foreign compilation. No new physical FPS/full-image parity acceptance.
See latest IMPLEMENTATION.md and NATIVE_HEIGHT_FOG_LAYERS.md.

14 October REPRODUCIBLE BUILD IDENTITY progress; goal ACTIVE/INCOMPLETE.
Previous turn NO PROGRESS (shadow reinspection/FTP timeout). This turn PROGRESS:
confirmed default GNU ar timestamp drift; stable content identities, rcsD game/
support archives, single-CXI packaging, repeated real app builds and byte proofs.
Latest LOCAL archive-identity-v4-{console-setup,console-replay}-builds:
normal993afab9e971 /3dsx5b88bdcf19d7 /CIA2ddcc802a140;
replay177c80515b9c /3dsx8f7e693f17cd /CIAfbaf9da52d8c. NOTDEPLOYED.
620 source records match; each archived game library is byte-identical to its
uniform-packet-v1 parent. Pair payload identity08329688c63b; raw ar timestamps
still differ in these preserved parent inputs. New archivers explicitly use D.
Full ELF/3dsx/CXI repeat exactly after archive metadata changes, both variants.
Every loaded section matches its parent except ONE build-tag string; all17GPU
programs unchanged. SharedOld80/New124 and installation recipes verified.
No extra emulator boots/perf claims: runtime code is unchanged. No new RAM cost.
CIA content ID fixed0; CXI compressed only once then wrapped. Test-ticket ID,
unused unencrypted key and signature remain randomized, so rawCIA is NOT byte
reproducible. Read-only comparator excludes ONLY those ticket fields; all other
package bytes must match; raw hashes retained and no package postprocessing.
ARM archive/link tests +16metadata/8payload-name/order/11malformed mutations;
CIA13protected-byte mutants/encrypted rejection pass. Evidencearchive-identity-
test/{result,cia-envelope,build-verification}.json and each/reproducibility.json.
Pure archive_build_patch.py applied to stable snapshot and root selected tools.
Failurehistory: v1 rootWIPsupport mismatch caught water shader guard; corrected
isolated patching. v2 CIA repeat failed (ELF/3dsx passed); v3 correctly detected
randomcontent IDs; v4 explicitmakeromID resolves that metadata variation.
All restoration proofs PASS. Owned82712/59385/61909/2303/26235 TERMINAL.
No FTP attempt/deployment this goal turn; previous034547 timeout unchanged.
Physical whole-shadow flicker UNRESOLVED. Specular/swizzle unsupported combination
exists but current reviewed logs report missing0; not a repair/cause claim.
No additional render features this turn. Remaining visual/parity/performance
and physical/endurance/progression goals still incomplete. Continue local visual
implementation; avoid another unchanged shadow audit or packaging repeat.
New stable source snapshot is archive-identity-v4, not unrelated root VRAM WIP.

14 October FUSED UNIFORM PACKETS progress; goal ACTIVE/INCOMPLETE, no blocker.
Full goal attachment read. Previous turn PROGRESS(particle CPU retirement);
this turn PROGRESS(native command/cache writer, ARM/host checks, shared builds).
Latest LOCAL uniform-packet-v1-{console-setup,console-replay}-builds:
normal bf6c7238e439 /3dsx aae48bcd1e29 /CIA 83ab23ca0f9d;
replay fad3e79c9119 /3dsx 66893e4cc6cf /CIA c1f960df6b67. NOTDEPLOYED.
New installed f2c4 unchanged. Whole-shadow flicker remains UNRESOLVED.

retained_uniforms.py now loads each changed float32 word once, writing both
immutable SDK commands and the existing reuse cache. Exact SDK CONFIG/DATA
sequence, 256-word chunks, padding/order, float24 invalidation and context
restoration preserved. No extra command, allocation, shader, draw, worker or
GPU wait. Pinned Melee uniform_upload.h/checked packet approach consulted;
actual pinned libctru GPUCMD_Add disassembly and ARM object used as oracle.
652 ARM11 cases wire/cache/dirty/counters exact; 9 capacity/null/cursor rejects
before packet/cache mutation, exact-fit cases1/3/63/64/65/96 rows PASS.
Synthetic instructions903927->738388(-18.31%);551 cases improve,101 regress
by at most1 instruction. NOT captured-workload cycles or FPS prediction.
Host10020 logical-upload/state checks ASanUBSanPASS; missing constant/context
invalidation mutants FAIL expected. Host decoder merges SDK chunks into the
logical oracle; raw byte equality is independently checked against linked
libctru on ARM, not claimed from that host abstraction. Result SHA eb681ccc8d54.
Initial ARM test missed prefixing private cache symbols; corrected, full rerun.

New Thorntail exactbinary run PASS70.45hostsec/status1/saves/drain/ownedexit;
1770 image inspected scene/Fox/HUD/effects retained. 2423391 uniform rows
uploaded,1426330 retained. No compiler activity observed at boundary samples;
prior baseline was contended, so NO FPS improvement established. No extra Old
boot or normal startup/conversion: unchanged setup/OS evidence reused honestly.
New sampled heap49230472/71303168,linearfree16629120/25165824,VRAM4690944,
stack73044/524288,process103419904/130023424. No allocation added by writer.
619 sources/17 GPU programs and each parent gamearchive unchanged except
1SDK generator. Shared source identity/Old80New124/recipe/restoration verified.
Pair game payloads identical, inherited ar header timestamps differ; not SHAequal.
Evidence uniform-packet-test/{result,host/host,build-verification,runtime}.json;
docs/NATIVE_CPU_AFFINE.md follow-up. Latest stable snapshot is uniform-packet-v1,
not root's unrelated VRAM WIP. New normal does not include unconnected bump tests.
Read-only FTP223 collection20261014-033413 timed out before login; no remote
changes, new hardware evidence or probe request. Owned18030/20274/13269/1631
all TERMINAL. No private assets/saves/preferences altered. Next continue native
performance and remaining visual parity; physical shadow check still open.

14 October bump lighting substitution gate:209496conditional source-equation
samples rejectunconditionalnormal-mapintegration; ArwingpositiveZRGBerrorworse
thanomittingbump. Someothermaterials improve; notcapturedscene/PICAacceptance.
Noapp/deploychange. NATIVE_ENVIRONMENT_BUMP.md recordsfullscope/limitations.
Hostcompilecontentionstilllive; no wastedtimingreplay. Shadowflickerunresolved;
fullgoalACTIVE/INCOMPLETE,no blocker.

14 October native fragment-bump field/RGB8 compiler locally implemented,
NOTgameplayintegrated. SourceA/B detailretained in conditionalnormalfield;
1Mbytepair/25Mpackedtexel checks +139264actualfiltered samplesPASS; strictARM11.
No liveallocation/newapp/deploy/FPSclaim. Basis/hemisphere/materialconsumers still
missing. NATIVE_ENVIRONMENT_BUMP.md recordsprecision/storage/limitations.
Whole-shadowflicker remainsUNRESOLVED; fullgoalACTIVE/INCOMPLETE,no blocker.

14 October native bump input compiler locally validated; gameplay bump still
MISSING. Exact17materials/17458references and73280original-parameter cases,
strictARM11compile; no runtimeallocations or app/deploy changes. Static vertex
bump interpolation rejected: even Arwing64xtriangles misses provisionalerrorgate.
See NATIVE_ENVIRONMENT_BUMP.md for conditional source/error measurements.
Whole-shadowflicker UNRESOLVED. GoalACTIVE/INCOMPLETE, no external blocker.

14 October shared uniform-retire local normalbc8802c1cd9b/replayf0c98aeb6c24,
NOTDEPLOYED. PackedSDKflagretirement keeps commandstream exact:2x10020host
checks +246ARM11cases.6.51%syntheticinstructionreduction is NOTgameFPS.
NewThorntail/Oldpalace checksPASS,capturesinspected;bothcostcomparisons excluded
foreigncompilation.619sources/17unchangedprograms/archive/Old80New124 verified.
No freshnormalboot/conversion. Exactinstalledf2c4movingcave154uniqueimages
inspected,shadowpresent;hardwarewhole-shadowflicker remainsUNRESOLVED.
FTP223013905timedout;no remotechanges. GoalACTIVE/INCOMPLETE,no blocker.
See NATIVE_CPU_AFFINE.md follow-up and SHADOW_FLICKER_20261013.md moving coverage.

14 October nativeaffine normalabce232a3f0d/replay348537b85014 LOCALONLY.
ARM11kernel2048casesbitexact/alias/ABI/mutantsPASS;NewThorntail/Oldpalace state/
save/capturechecksPASS.619sources/17unchangedGPUprograms/archive/80New124pair
verified. Bothruntimecostcomparisons excludedcompilercontention;nofreshnormal
boot/conversion,physicalFPS/shadowstability/fullparityclaims. NATIVE_CPU_AFFINE.md.

14 October four-entrynative bank normal96435b5e1914/replaya25121517e0e LOCALONLY.
305storedinstructions,exactfloat24/SDKhandoffs;NewThorntail/Oldpalace/Newitemmenu
state/save/capturechecksPASS. Shared618sources/17programs/archive/Old80New124.
Actualprogramtraffic/draw -4.43%; netFPS gateOPEN,controltimingscontended and
CALLcostunvalidatedonhardware. No physicalshadowfix/fullparity/progressionclaim.
No freshnormalboot/conversion/deployment. NATIVE_SCENE_BANK.md givesbounds.

14 October shared native shader bank locally built: normalbb94b5b34f6d/replaya62474102735,
NOTDEPLOYED/NOTpromoted. Ordinary192+ambient58 share250instructions; SDK tests
prove within-bank switches avoid code-upload flags and preserve state. Both
entries float24exact; NewThorntail/Oldpalace state/save/capture checksPASS. Shared
Old80/New124,616sources/19programs/archiveverified. Newcontroltimingscontended:
no netFPS orphysicalclaim. Shadowflicker remainsUNCONFIRMED;FTPtimeout,nochanges.
See NATIVE_AMBIENT_SHADER.md. Goal ACTIVE/INCOMPLETE.

14 October ambient-only GPU specialization: exactfloat24 output/admission and
NewThorntail/Oldpalace state/save/imagechecks PASS. Localnormal56292372736a/
replayc2c0f7150692 NOTDEPLOYED orpromoted. Eligible shaderinstructioncount -36.8%,
but pipelinepublicationsincrease; netcostgateOPEN, timingscontended. No physical
performance/flicker/fidelity acceptance. NATIVE_AMBIENT_SHADER.md recordslimits.

14 October consolidated native effects: Oldpalace/NewThorntail correctness PASS,
including specular+FASTripples together onNew. Normal6f0292b8f0e3/replay664d6f45567b
sharedsources/archive/18GPUprograms/Old80New124 verified, NOTDEPLOYED. Actual
copycounts lower than eager rule, NOT totalFPSacceptance. Both timings excluded
due compilation. Source-image waterparity, physicalshadowstability and sustained
30/60FPS stillOPEN. See NATIVE_EFFECTS_CONSOLIDATED.md. No remote/savechanges.

14 October RIPPLE FAST optional game integration: actual compiler, lifetime,
UI/settings and GPU offset/image checks PASS; New/Old Thorntail routes PASS
with world/Fox/HUD/water captures inspected. Shared normal1c13d3cffe56 and
replay6ac366c43e31 NOTDEPLOYED. DefaultOFF; source-image fidelity and physical
cost remain OPEN. Both timing comparisons excluded by foreign compilation.
No net FPS claim. Dual meshes add15488Blinear; Old sampled free1,952,128B.
See NATIVE_RIPPLE_DETAIL.md. Physical shadow flicker unconfirmed, no fresh
console evidence; latest FTP connection timed out before login.

14 October radial ripple mesh preparation implemented and checked locally.
Candidate1078 vs3054tri/pass (64.7%cut),15KiBlinear and17KiBtemporaryreservation;
no per-frame geometry work.36compilerconfigurations, existingregularoutput
identity, actualowner lifetime/failure tests and8GPUcases pass. This is NOT
connected to gameplay or deployed: larger images lose detail, projected-size/
temporal selection and physical/clean performance gates remain open. No net FPS
claim or fullrendering acceptance. See NATIVE_RIPPLE_RADIAL.md. Installed shadow
flicker remains unconfirmed; no remote/save changes. GoalACTIVE/INCOMPLETE.

14 October experimental water uniforms now remain resident across native batches.
Shared normal6525adee8679/replay3a090e405173 are built, NOT deployed. 30-instance
float setters368->135, but shader adds one executed instruction/vertex; no net
speedup established. Actual SDK boolean publication error caught by GPU fixture,
fixed with real SDK regression. Corrected eight GPU captures pixel-identical to
prior batch;65,536 interpreted vertices bit-identical. Archive/615sources/18GPU
program identity, Old80/New124 and restoration pass. No fresh game replay or
normal boot claimed; no uncontended timing available. See NATIVE_WATER_UNIFORMS.md.
Physical whole-shadow flicker remains unconfirmed on installedf2c4; FTP preflight
timed out before login. No console/assets/save changes. Goal ACTIVE/INCOMPLETE.

14 October native water submission now batches source instances instead of
rebuilding common GPU state twice per ripple. New/Old routes retain state on
91.08%/90.21% of draws; state reset, lifetime, live pose, source-order and actual
GPU pixel checks pass. Shared normal26df937cf7fb/replaye4e84ba77eaf built locally,
not deployed;615sources/archive/18programs match,Old80/New124 preserved.
Memory remains within budgets. Timing acceptance remains OPEN: the earlier
clean visibility ON/OFF failed+.7343ms CPU, and new batch runs overlap unrelated
compilation, so no FPS improvement is claimed. RIPPLES OFF all presets.
Original-image parity/filtered GX alpha and physical shadow-flicker acceptance
remain incomplete. See NATIVE_WATER_BATCHES.md for evidence and failure history.
No FTP, private assets, saves or preferences changed. Goal ACTIVE/INCOMPLETE.

14 October ripple native gameplay path is PARTIAL, local replay13e85d7dc927.
Source pool/pose/material, immutable reduced plan and independent toggle work;
New/Old route state/save/shutdown and memory checks pass. Corrected shared
capture no longer requires WAKES. Full original-image/filtered-alpha/physical
cost and performance gates OPEN; timings excluded due host compilation.
RIPPLES OFF all presets. No deployment or physical shadow-flicker acceptance.
See NATIVE_RIPPLE_GRID.md for costs, limits and failure history.

14 October reducedripple candidate cuts6854→3054tri/pass,55.4%; this is NOT a
GPUtime/FPSgain or gameplayintegration. Sourceboundary/area andsurvivingbytes
exact; sampledimageerror risesmean1.014→1.410,p956.196→7.947. Actualnative
load-only adapter passesASan/allocationfailures/SDKlink; two-planowner only
strictARMcompileaccepted. Lifetime/adapter/toggle/replay/GPU/perfgatesopen;
no newapp ordeployment. NATIVE_RIPPLE_GRID.md records source/render/costlimits.

14 October candidatewakegrid nowpassesactualGPU8pixelcases plusNew/Oldgameplay
routes; replayf342a6e016d6 NOTDEPLOYED.2projectivedraws/wake,2376tri/pass,
36992Blinear. NoCPUvertexrebuild; originalsimulation/archive preserved. All
resources retainmeasuredheadroom, savesisolation/shutdownPASS. Performance
gate remainsOPEN(hostcompilation), originalappearance/physicalcost/unfiltered
bumpalpha stillpartial; OFFallpresets. SeeNATIVE_WATER_GRID.md forfailurehistory,
pixelmutantchecks andmemory. Correction:ripplemap isinit-generatedstatic,
notadynamicproducer; no ripplepathimplementedyet. Fullgoal remainsINCOMPLETE.

14 October water distortion remains UNACCEPTED. Load-prepared grid/compiler and
31instruction shader pass source/ASan/ARM/VM checks. Coarse grids sometimes
increase pixel error; finest candidate improves it, with zero-alpha trimming
reducing4290 to2376triangles. This is software comparison and vertex arithmetic,
not GPU raster/performance or source-game parity. Not gameplay-linked/deployed.
SeeNATIVE_WATER_GRID.md; lateststableapplication unchangedwater-capture-v1.

14 October water pixel/failure-path progress: actual production owner, SDK,
material and shader pass8 captured-pixel cases including forced state handoffs.
Max3.75/255 RGB error; missing projective-Q mutant fails73.22/255. This fixture
does NOT validate indirect distortion, dynamic ripples, fragment distance fog,
original gameplay appearance or hardware cost. WAKES remains OFF allpresets.
Optional shared capture allocator is now transactional, tested on all3failure
stages; real-game OFF run confirms zero wake draws. Performance attempt was
contaminated midrun by another project compiling, so no cost gate passed.
Shared replaya7a1605be736/normal14dd9962313d built NOT deployed, same game and
18shaderbytes aswaterv3, Old80/New124. Installed shadowf2c4 stillawaitsphysical
confirmation; fullgoal incomplete. See NATIVE_WATER_SHEETS.md.

14 October wakeintegrationprogress: nativeimmutablewaterquad+twoGPUprojective
passesnowconsumeoriginalwaterDLLstate. WAKESdefaultOFFallpresets. Shared
replay9904bce048d7/normal00889a33880e builtNOTDEPLOYED; onlygame19.ochanged.
New/OldrealThorntailroutesPASSwith440/376wakedraws,shutdown/saves/memorygates.
V2shutdownimageleakfixedV3beforepoolteardown. Imagesretainworld/Fox/HUD;
affectedpixels/sourceparitystillOPEN. Both57.28/51.28guestFPScountersEXCLUDED
comparisonunderhostcompilation;noCPUbudget/physicalacceptance. Indirect
distortion,filteredbumpalpha,dynamicripples remainMISSING. SeeNATIVE_WATER_SHEETS.
Shadowwhole-shadowflicker stillawaitsphysicalconfirmationofinstalledf2c4fix.

14 October water sheet compiler/kernel progress: see NATIVE_WATER_SHEETS.md.
Two native passes solve the simultaneousprojectivesampler limitation locally;
not integrated, not visible gameplay, not completewaterparity. Source/SDK/VM
tests pass. Simplevertex-offsetgrid has unacceptable sampledcoordinateerror
even4096triangles; do not blindly add that cost. Indirectdistortion,filtered
bumpalpha,sourceimages and physical/performance gates remainOPEN. No deployment.

14 October CAMERA-SPACE PALETTE EXPERIMENT progress; goal ACTIVE/INCOMPLETE.
PreviousgoalturnPROGRESS: packedlights/normaldemand. ThisturnPROGRESS: real
shader/palettecacheimplementation, exactlinked prototypes, mixedownerchecks,
sharedreplay andserialNew/Old correctness. No blocker, no performanceacceptance.
Fullgoalattachmentread. Readcurrentevidence; don'trepeat rejectedexperiments.

Sourcecamera*bone composedonceperuniquejoint peractorinvocation. GPUstillskins
vertices; ordinary/layered/nativefur omit3position+3needednormalinstructions.
World/UI/cloud/shadowprograms retainoriginalspace; CPUfur matrices remainper-
shellanduncached. Cache257x48B +36Bvalid=12372STACKbytes, noheap/linear/texture/
worker/geometrypasses. SDKcopiesrows; noGPUborrowedstack. Actorinvocation resets
validbits so nooldobject/camera/pose reuse. PinnedMeleevertex.v.pica +SDKuniform
ownershipconsulted. RemainingCPUcache+concatenationcost needsmeasurement.

Actualpublication+cache640000mixedcases ASanUBSanPASS; noreset/missingcloud/UI/
shadow/cachedCPUfur mutantsFAIL. Actuallinked3families37888verticesEACH local
maxscreenerror3.57628e-5pixel,colour1.220703e-4;2923traces/familynoinstruction
regression. Sharedbank445->439,material238->232,fur150->144;NOTFPSpercentages.
Nativefloat24typeusesfloat32ALU internally; cannotcallthesephysicalerrorbounds.
Extra16fractionbituniformtruncationsensitivityPASS maxpixel.002218; furrawalpha
absolutegateinitiallyFAILED. It exceeds1 duehomogeneousUV/tint.w ABI. Retained
failure; subsequentrelativepositivealphagate+conditionalinterpolate/clampbound
<=7.1711e-5(<1/255) passes, rawalphaerror.00402832stillreported. No gamecode
relaxed tohideit. Rotation/translationmutantsFAIL. FullphysicalprecisionOPEN.
Firstquantizedharnesscompile usednonexistentToRaw; correctedexplicitfraction
truncation, notALUemulation. Accidentalrecursivegrepfromshellquotingstoppedowned
session47555; subsequentsearchusedboundedPython. No externalprocess touched.

Replay75e07607e522/3dsx1bcf760e9da8/CIA2b157759b26f BUILTNOTDEPLOYED,
view-pose-v1-console-replay-builds. Only3sourceschangefromlight-normals-v1;
samegame9d521d51f66e/Old80New124,17programswith3changedbanks. Source/archive
restoredfinally. No normalvariantbuild forunacceptedexperiment. Latestnormal
candidate remains898b0229c65f. NEVERblindbuildVRAMWIP; useimmutablearchives.
build.json genericshader_budget445 isnotactive; active_native_shader439 and
actualELFbytesverified. DoNOTcitegenericunusedbudgetasactiveinstructioncount.

SerialNew/Oldpalace status1/state/saves/drain/ownedexitPASS;1410imagesbothseen.
Newposesbuilt115375/reused119786,Old111981/117412; uncached0both (noCPUfurroute
runtimecoverage). Whole-run counts notperframe. BOTHtimingsexcluded: external
SMGcompilationobserved. Nopairedbaselineboot whilecontended/noFPSwinclaimed.
Newheap48598456/71303168,linearfree18036224/25165824;Old41061792/50331648,
linearfree3356160/10485760. VRAMfree4723712both. Measuredstack72972/72984 vs
524288; unchangedsamplemaxdoesNOTerase12372addedfunctionlocalstackcost.
Process103354368/130023424 and67702784/83886080. Sparse,notcontinuouspeaks.

Additionalorder-preservingactoraliasaudit835originalmodels315419vertices/
988530indices foundZEROidentical40B+sameposition/normalref aliases. No pointless
runtimeoptimizeradded. Generictriangleordering maychangeblends/equal-depth;
mask-onlycacheordering ALREADYimplemented. Don'trepeatthatshadowwork.
Evidence actor-identical-vertices-audit,view-pose-owner-test,
view-pose-linked-test,view-pose-quantized-linked-test; docsNATIVE_VIEW_POSES.md.
host_timing_context.py gainedreadonlyCLI; allownedhandlesTERMINAL92136,16421,
27052,24588,11002,1027,47555,27358. NoFTP/remotechanges/probe/deployment.
InstalledNewf2c4fafba819+rapidwhole-shadowflicker stillPHYSICALOPEN; thischange
doesnotfixorsubstituteitsacceptance. Lastconsolelogs predateshadowcorrection.

Next quantifyCPU/GPUtradeoff oncomparableuncontended/hardwareworkload before
promotingviewposes; continueparityindependentlyifhostbusy. Do notstackexpensive
featuresonassumedwin. Nativeheat/lightnormals/prefetch referencespreserved.
Newcore2audio,not2freegraphicscores. Fullplayableparity/performancegoalcontinues.

14 October PACKED GPU LIGHT LOOP + NORMAL DEMAND LOCAL progress; goal ACTIVE/INCOMPLETE.
Previous goal turn PROGRESS: specular prefetch tested/sharedbuilds. This turn
PROGRESS: native shader architecture+publication, actuallinked output/traces,
ASanUBSan ownership/contracts, sharedbuilds, serialreplay correctness. No blocker.
Read full goal attachment before continuing. Physical targets/parity incomplete.

Historical correction: directional and signed-colour normal-demand experiments
already rejected Oct12. Did NOTrepeat them. Replaced8unrolled lightblocks with
one bounded GPU loop: source-slot order/point+directional math/clamps EXACT.
Packonceperlightsnapshot; zero-count guard;i0(count-1,0,1,0) AFTERprogrambind.
Normal-demand usesfreedbool5 WITHOUTsignedmaterialencoding; skipnormalmath only
when noactive lights/environment/specular. Cloud keepsoldlight1 ABI/restored.
NoCPUvertices/newworker/allocations/texture/geometrycopy. Four-bytecountstate;
normal/lightpackingCPUcostgate OPEN. SDKdirtybitretained;newpublicationforced.
PinnedMelee citro3d_fix cacheownership +pinnedSDKuniforms/types consulted.
Picasso normalNOPpadding retained;LOOPrelocation added;checker onlyarithmetic
loopbody/nooutputs/MOVA/control/IFescape;badtarget/output/controlmutantsfail.

Actualpack256000snapshots +1024000integerhandoffsPASSASanUBSan, missingpack/
handoff/framepublicationmutantsFAILexpected. Actualnormalflag524288cases incl
cloud +missingmap/cloudmutantsPASS. ActualLINKEDordinary/layered/fur131072
verticesEACH all28outputlanesEXACT. 10240sampledtraces/familyNOinstruction
regression. Badloopstep/zeroguard/missingmapnormals rejected. Staticbank511->445,
material304->238,fur216->150. These are NOTcycles/FPS. Looponly intermediate
443/236/148 +NewpalacePASS; don'tconfusewithfinalcombinedshader.

Replay7a4abce630df/3dsx18eebdc729f1/CIAd7a32678ba84,
normalsetup898b0229c65f/3dsx38d061f05ed9/CIA0416a9f7cf63 BUILTNOTDEPLOYED.
light-normals-v1-{console-replay,console-setup}-builds immutablecandidate;
samegame9d521d51f66e,597sources/17shaders,Old80/New124,recipesunchanged.
Only5sourcefiles differfromprefetch;3shaderbankschanged. BuildersrestoredWIP/
archivefinally. NEVERblindbuildworktree: VRAMWIPremains; loop/normalpatch is
inarchivedsource snapshots andpurepatch/buildhelpers,notpromotedoverVRAMWIP.

SerialNew/Oldpalace finalstatus1/saves/drain/ownedexitPASS;1410imagesinspected
bothworld/character/enemies/effects/HUD. Bothfinaltimingsexcludedwhenexternal
SMGcompilationobserved; don'trunpairedunchangedbootswhilecontended. NoFPSwin
claim. ActualGPUcycles andphysicaldynamic-loopacceptanceOPEN. New heap48598456/71303168,linearfree18036224/25165824,VRAMfree4723712,stack72972/524288,process103354368/130023424.
Old heap41061816/50331648,linearfree3356160/10485760,VRAMfree4723712,stack72984/524288,process67702784/83886080. Samplesnotcontinuouspeaks. No newnormalboot/conversionrun;
normal/archive/shaderssameascheckedreplay,notfullopeningvalidation.

Evidence light-loop-test,light-loop-linked-test,light-normals-test,
light-normals-linked-test. NATIVE_LIGHT_LOOP.md describeslimits andreproduction.
NoFTP/remotechanges. InstalledNewf2c4fafba819 unchanged; rapidwhole-shadowflicker
stillPHYSICALOPEN; don'tclaimthislightingworkfixesit. Lastcollected13Oct175106
logs predatetheshadowcorrection. NoOlddeploy/noasset/settings/savechanges.

Next validate combinedcandidateCPUcost whenuncontended/physicalmilestone, then
continue remainingrendererparity. Sharedbankremoved66staticinstructions, not66
instructionslesspervertex; strictcodeSize<512guardretained. Looponly saves5lit/7empty;combinedlitneedsnormal
net3fewer,ambientonlyalsoomitsnormaltransforms;unlitchangesnone.
Normalcount currently packloop plusoldcountroutine; mayconsolidateifCPUcostneeds
it, don'tfabricatecostwin. Newcore2audio,not2freegraphicscores. Goalcontinues.

14 October SPECULAR PREFETCH LOCAL progress; goal ACTIVE/INCOMPLETE.
Previous goal turn PROGRESS: demand experiment+evidence rejecting CPUwin. This
turn PROGRESS: revised scheduling, actual tests, shared builds, serial replay
checks and read-only hardware-log collection. No blocker/no physicalFPSclaim.
Read full goal attachment before continuing; preserve scope and performance gates.

Source-boundary full maps unchanged. Loadcompiled halfmask predicts next copy
batch; immediate late repair BEFOREactor uniforms/camera/pose. Per-slot expiry,
group isolation, fullwrite invalidateshalf. Getters sideeffectfree. No shader,
geometry,textureallocation,CPUvertices,worker/readback/frameallocation changes.
Adds4B/material+smallmask/counters; noVRAM/linearallocation win. PinnedMelee
gpu_efb_copy/citro3d_fix/early_queue retained publication/cache/retirement rules.

ActualASanUBsan100lifecycles/100000predictedupdates PASS; missingpublication,
stalehalf,noexpiry,grouploss,missingrepair mutants rejected. Actualcompiler36
aggregate/preflightcases PASS. Priorhalfowner100lifecycles and1671168actualsource
RGBtests PASS; fixturepostlightenum/header updated, no productionequationchange.
Older demandtests now read immutableheatbase so remain reproducible separately.

Replayb9cf14dd9c44/3dsx410d9f67bd6c/CIA98d05310a10e, normalsetup5a16f2fa68f0/
3dsx5575157b6eda/CIA2bd4b6f67991 BUILTNOTDEPLOYED. Candidate snapshots are
specular-prefetch-v1-{console-replay,console-setup}-builds. Same596sources,
game9d521d51f66e and17linkedshaders both variants; only6nativefileschanged vs
native-heat-v3. Old80/New124/recipes unchanged; worktree/archive restored.
Workingtree carriesprefetchpatch but unrelatedVRAMWIP stillpresent: NEVERblind
build. Useisolatedbuilder. Heatv3 preserved PERFORMANCEreference; prefetchCPU/GPU
costgate stillOPEN, notacceptedphysicalspeedup. Nofreshnormalboot/conversion.

SerialNew/Oldpalace status1/saves/drain/ownedexitPASS;1410imagesbothinspected
world/Krystal/enemies/HUD. Newfull806/half553,requesthits3086/misses4,consumer
24017/unready0. Old792/542,3029/4,23158/0. Avoids253/250half copies vs eager
ruleforSAMEfullupdates, NOTthatfractionofwholescene/FPS. Bothruns TIMINGEXCLUDED
externalSMGcompilation atboundaries. Don'trerun unchangedboots whilecontended.
LinkedARMhitpath hasnoGPU/FS/alloccalls; staticnotphysicalcycles/instructiontrace.

Newheap48598456/71303168,linearfree18036224/25165824;Old41061792/50331648,
linearfree3356160/10485760;bothVRAMfree4723712; measuredstack72972/72984 vs524288.
Process103354368/130023424 and67702784/83886080. Sparse,notcontinuouspeaks.
specular-prefetch-test/{owner,material,integration}.json +linkedARMdisassembly.
DocsNATIVE_SPECULAR_PREFETCH.md. Builders/test/analyzer scripts in tools.

FTPNew223 readonly20261013-175106-223 COMPLETE: everycollectedhashidenticalto
163615. Installed f2c4fafba819 unchanged; runlogstillold. Rapidwhole-shadowflicker
PHYSICALOPEN; nofixclaim/noextraProbe/newdeployment. NoOldFTP/save/settings/assets
changes. Allownedhandles TERMINAL92024,35348,96900,85970,6808,29940.

Next substantive GPU lead identified (NOTIMPLEMENTED): actualstable actor shader
normalizes directional light vector pervertex even when light_position.w==0.
Eight unrolled light blocks repeat normalization; verify live lightkind frequency,
float24 error and pipelinecost before specializing/packing. Native loop orseparate
shader could free512instructionbudget but mustretainpointlights/alpha/source
order andallotherentryABI; don'tspeculativelyreplace normalization. Current
native-heat-v3 remains availablecostreference. No new light optimization yet.
Continue remainingparity/performance; Newcore2audio,not2freegraphicscores.

14 October SPECULAR DEMAND EXPERIMENT progress; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS: native heat. This turn PROGRESS: implemented demand
compiler/owner, actual C tests, immutable replay build and matched runtime checks.
NO speedup accepted, no blocker. Read full goal attachment before continuing.

New223 remains f2c4fafba819; no FTP or remote changes this turn. Rapid WHOLE
shadow dropout remains PHYSICAL OPEN. Do not infer a new run from clarification.
No further probe requested. Last console logs still older; depth repair unconfirmed.

Demand candidate3fab2370a7e7/3dsx2cd1bdfb0bd3/CIA2d770793de6c BUILTNOTDEPLOYED,
specular-demand-v1-console-replay-builds. Only6nativefiles differ; gamearchive
9d521d51f66e andall17shaders identical toheatv3. Old80/New124. Builder restored
worktree/archive. NO normal candidate build. LatestSTABLE remains native-heat-v3
console-replay+console-setup snapshots. NEVERblindbuild unrelatedVRAMWIP.

tools/pica_specular_demand_patch.py: source groups snapshot104B/slot; loadcompiled
full/half aggregate mask; constanttimeactorpreflight BEFOREuniforms/camera/pose.
Sideeffectfree samplers; pendingstates; stalehalf invalidation; sameordered
GPUtargets/splits/retirement. NoCPUvertices/newworker/readback/frameallocation.
Adds312Bpending +smallcounters+4B/material; targets/linearunchanged,NO VRAMsaving.
PinnedMelee gpu_efb_copy+SDKbinding/cacheadaptation consulted.

ActualASanUBsan owner100lifecycles/100000invisiblesnapshots PASS; missingflush,
stalehalf,eagerhalf,stalefull mutants rejected. Actualcompiler36aggregate cases
PASS/missingdemand mutant rejected,preflightorderchecked. Firstfixture lacked
ADD_MULTIPLY enum; harnessfixed. No shader/lighting equation changes.

SerialNew/Oldcandidate+baseline palace allPASS/state/saves/drain/ownedexit.
Newcandidate/baseline+Oldcandidate1410images inspected world/Krystal/enemies/HUD.
New300frames16.713682->16.711522ms,renderCPU INCREASED .054360ms/frame.
Whole-run full796->790,half796->574,consumer23137->23270/unready0.
NoFPSwin established. Latefull/halfproduction adds target/state switches.
OldtimingsEXCLUDED due externalSMGcompiler atbothrunboundaries; correctnessonly,
candidate23137consumer/unready0. No physicalclaims/repeatedtest request.
Physicalprior specmapspan44.74sec isWHOLErun nestedelapsed inclpossiblequeuewaits,
notpureCPU/GPU/additiveframecost. DoNOTclaimdominantcostproven.

CandidateNewheap48599424/71303168,linearfree18008192/25165824;Oldheap41061792/
50331648,linearfree3356160/10485760;bothVRAMfree4723712. Measuredstack72972/
72984 vs524288; excludeunmeasured startupreservation. Process103354368/130023424
and67702784/83886080. Samples notcontinuous highwater.

EXPERIMENTHELD: patch+immutablebuild+experimental-worktree.zip retained; only
itscheckedhunks reversed fromsharedworktree. transform(restored)==candidate
verified,unrelatedVRAMWIPpreserved. Existingtestmods reverted; experimenttests
transforminmemory. DoNOTdefaultbuild thisasaccepted. No normalbuild/deploy.
DocsNATIVE_SPECULAR_DEMAND.md; specular-demand-test/{owner,material,runtime-
comparison,experimental-worktree-retained}.json. Runtime scripts/analyzer retain
timing exclusions. AllownedhandlesTERMINAL incl48338,15798,72285,27655,16668,
1905,27395. Next redesignscene-level demand batching before further map work;
continue remaining native parity/performance. Newcore2audio,not2freegfxcores.

14 October NATIVE HEAT LOCAL progress; goal ACTIVE/INCOMPLETE, no blocker.
Previous turn PROGRESS: combined GPU capture. This turn PROGRESS: native heat
source/shader/material/owner integration, optional toggle, shared builds/tests.
Read full goal attachment before continuing. No full parity/performance claim.

New223 remains f2c4fafba819. Read-only FTP console-visual/20261013-163615-223
COMPLETE; all collected hashes unchanged from155001. No console writes/probe
request. Rapid whole-shadow flicker remains PHYSICAL OPEN; latest clarification
is NOT evidence of a new f2c4 run. Prior emulator cave tests did not reproduce
it even before the installed depth repair. Do not mark physically fixed.

Native heat: immutable80x224grid/18225vertices/35840tri/433792linear inclpadding,
separate54instruction/9float/noGS program. Original scroll/pitch/intensity/Kalpha/
HUDprojection/depthGREATER; onlylightmap.o game member changed. Analytic caustic
approximation vs source IA8:4,608,000samples48phases max.249808nativepixels,
p99.165295. Coarsegridsrejected; finiteaudit notuniversalerror/mathbitidentity.
No CPUperframevertices/GX/readback/newworker. SharedRGBAcolour/depthcapture;
separated captureprogram/quads fromblur scratch soheat-alone needsNOscratch.
HEAT HAZE bit1<<30,VALID0x7fffffff,OFFall4presets,persistent/recordlocked.
Actualreserve denial marksunavailable/replayfailure.1MiBVRAM/15%linear guards.
Residentstorage helduntiljoinedshutdown afterOFF, no repeatedallocations.

Actualfloat24VM291600vertices/all16lanes/pitch/PAL+NTSCviewport PASSmax1.57e-7.
Source/nativeTEV589824ASanUBsan casesPASS. GPU8casesOFF/stale/reseed/foreground/
realreserve-denial+retryPASS,flatmax.377/255; no blur scratch. Spatial6936edge
pixels nativeerror1.692/255,source28.065/255 atsharpedge(subpixelapproximation),
wrongdirectionmutant187 rejected. Synthetic,NOTaffectedgameplay/physical.
Separablebasisinit preservespackedbytesexactly (heat-basis-reuse-test),1224B
stacktemporaries. Deviceprepare55.13->2.20ms, subsequentnestedCPU.0401ms/call;
NOTFPS/physicalGPUclaim. TargetsCPU.20New/.50Old,GPU1.5New; GPUgateOPEN.

Sharedmotion/blur ASanUBsan35600cases100lifecyclesPASS,missing split/OFFmutants
rejected. RealGPU VRAM/lineardenial+temporal PASSmax1byte. InitialOFFmutant
failedto match3-optiongate; updatedstrictmatch. Initialdenialfixtureexpected
sharedcapturestoragefreed; nowasserts retained512B/program+no failedscratch.
No guardweakened. Firstbuildfailed SDKmaterialnamedpica_ selectedgameclang;
renamedplatform_pica_heat_material.c. Allfailurebuild/device dirsretained.
Menu bounds/lock/idle/presets/replaymaskPASS inclHEAT+reject1<<31.

Replayc06e17e142a8/3dsx15826f54d486/CIAed257f3c6bcb; normalsetup266ed5530385/
3dsx819b17ce46f3/CIA808b3789ec56 BUILTNOTDEPLOYED. Lateststable snapshots:
native-heat-v3-{console-replay,console-setup}-builds.596sources identical;
gamearchive9d521d51f66e sharedexact;17linkedshaders incltestedheatfcbdf66e9dc0,
prior16unchanged. Old80/New124,normalrecipesunchanged,worktree/archive restored.
NEVERblindbuildVRAMWIP. tools/build_native_heat.py isolates/pica_heat_patch.py
idempotent. Normal --game-from reuses exactreplayarchive; successivearheaders
vary but all794v2/v3memberpayloads identical. No fresh normalruntime/conversion.

Serialnative-heat-v3-new-dialogue and-on-old-new-dialogue PASS73.64/62.56hostsec,
state/status1/saves/drain/ownedexit.1800imagesinspectedworld/Fox/Tricky/dialogue.
Existingblur160/142requests320/284draws,unseeded0; HEATZERO. Sharedcapture/game
regression ONLY; no fabricatedheat activation/progression. External smg3ds
compilation excludes timingattribution; originalsummary.runner retained.
host_timing_context.py qualifiesfuture runs. Do not claimtimingimprovements.
Newlastheap49264080/71303168,linearfree16731904/25165824;Old41727416/50331648,
linearfree2060032/10485760;bothVRAMfree3675136,stack73044/524288;process103354368/
130023424 and67702784/83886080. Sparse,notcontinuous; inactiveheat excludesmesh.
Oldactive headroomnarrow; sourceactivationVFP/Drakor/EnvFx, noaffectedrouteyet.

DocsNATIVE_HEAT.md andnative-heat-test/{build-verification,runtime-regression}.json.
ActualGPU native-heat-device/native-heat-v{1,2-admission,3-prepared}; shared
native-motion-blur-device/heat-shared-capture-regression-v2. Allownedhandles
TERMINAL incl13608FTP,26588fail,99667,77864,57479,77617fail/retrypass,39998fail,
74301,29465,86315,6064,84371,17762. No saves/preferences/assetsmodifiedremote.
Nextcontinue native parity/performance; physicalshadow andsourceheat fidelity/
GPUbudgetstillopen. Newcore2already audio. No blocker/statuschange; continue.

14 October COMBINED GPU POST CAPTURE progress; goal ACTIVE/INCOMPLETE, no blocker.
Previous turn PROGRESS: temporal shadow evidence + ordinary waypoint replay.
This turn PROGRESS: replaced separate colour/depth fullscreen captures with one
GPU draw, tested actual pixels/lifetimes/dialogue, built matching normal/replay.
No FTP or console writes/probe requests. New223 remains f2c4fafba819, physical
whole-shadow flicker OPEN. Do not claim latest clarification retested f2c4.

platform_pica_motion_blur.h history nowRGBA8 RGBcolour/Adepth; blur owner borrows
nearest history descriptor, retains separateRGBA8partial target. Original capture
timing/stalehistory/motion-before-capture preserved. One less draw/two fewer
targettransitions per capture. +262144VRAM (also motion-only), no extra normal/
linear/process allocations/CPUvertices/pixels/workers/shaderinstructions.
Combined1MiBVRAM+1KiBlinear;1MiBVRAM/15%linear admission reserves preserved.
OFFall4presets; toggles retain live storage until retired shutdown. New8-bit
colour vsold565 is not bit-identical quantization. PinnedMelee gpu_efb_copy and
citro3d_fix consulted; no raw depth readback/SDK bypass. DocsNATIVE_POST_DEPTH.

Devicev1 FAILED wrongUV: oldUI used1-v; new affine initially omitted finalflip.
V2 fixes coordinates;8GPUcases maxflatRGB1/255,3216spatialedge max1.999/255,
wrongasymmetric-kernel mutant68.1 rejected. V3admission includes onecapturedraw
assertion, realVRAM/linear denial/toggles/staledepth PASS. Motion-only actual
normalshader8cases max.3765/255 PASS. OwnerASanUBSan35600cases/100lifecycles,
12allocationfailures/12000deniedretries PASS; missingresetmutantFAILexpected.
Alldeviceartifactspreserved under native-motion-blur-device/combined-post-*.

Shared replay79da13900cc0/3dsxb94636c124e8/CIA5459467841ed and normalsetup
9d99a0d60bb2/3dsx7a60cd736c5b/CIA7c8e47eca438 BUILTNOTDEPLOYED.
combined-post-v1-{console-replay,console-setup}-builds source snapshots/libgame
are new stable combined bases. 588sources identical, same16linkedshaders/game
3001dd7502b2/Old80New124, unchangednormalinstallrecipes, restorationtrue.
DoNOTblindbuildVRAMWIP. tools/build_combined_post.py isolates both variants;
run_combined_post.py reuses successful ordinary waypoint+A route unchanged.

New/OldONdialogue status1/saves/drain/ownedexitPASS63.09/59.35hostsec,1800images
inspected. New171requests342draws,Old142/284,unseeded0;1280/1156capture draws
removed. Newframe18.7146->17.9156ms,Old21.9161->21.5881 AzaharONLY.
Newsubmit4.82965->4.42079,Old4.61677->4.41067; animation/audio phases differ,
doNOTattributewholeframe delta solely tochange. DirectnestedcaptureCPU/capture
.198754->.120888New,.199105->.121197Old. Don'taddnestedspans tosubmission.
No samecandidateOFFrun (unchangedinactive behavior/deviceOFF tested); no physical
FPS/60FPS/fullparity acceptance. combined-post-test/{new,old}-comparison.json.

LastNewheap49264056/71303168,linearfree16740096/25165824;Old41727440/50331648,
linearfree2051840/10485760. BothVRAMfree3675136,stack73044/524288.
Process103350272/130023424 and67698688/83886080. Sparse,notcontinuouspeaks.
No normalmenu boot/wholeISO conversion rerun forowner-onlychange; oldevidence
scope preserved. build-verification.json +scope files record source equivalence.
AllownedprocessesTERMINAL5260failedpixel,90491,24544,43028devices;28992/48378
builds;63376/64063gamechecks. No activeemulator/build/FTP. Fullgoal remains
incomplete. Next rendererparity/performance (heatdistortion stillmissing), shadow
physicalacceptance pending; avoidrepeatunchangedcaveboots/probes.

13 October TEMPORAL SHADOW + LIVE DIALOGUE progress; goal ACTIVE/INCOMPLETE.
Previous continuation: SDK shadow command-order regression. This continuation:
bounded temporal observations, ordinary-input waypoint replay implementation,
three successful source-triggered dialogue runs; no blocker or full parity claim.
New223 still f2c4fafba819. Read-only console-visual/20261013-155001-223 collection
finds gameplay logs/captures identical to142848, only manifest newer. No FTP writes,
Old/replay deploys, or new probe request. Physical whole-shadow flicker OPEN.
Cave before/after-temporal-v2 both pass480observations/8s,230/228unique images;
neither reproduces rapid disappearance, including OLD baseline. Images inspected.
Not physical acceptance or timing comparison. First temporal-v1 failed missing
ffmpeg; replaced with owned hidden-X11 libX11 capture, retained failure.
shadow-temporal-capture-test/result.json and SHADOW_FLICKER_20261013.md record
limits. Command rollover audit found acquire invalidates; capture rebinds target
and projection republishes after reserve. No speculative waits or renderer edits.

WALK_TO appended to actual replay interpreter: ordinary on-foot camera-relative
stick, live 3D arrival, state gating, bounded timeout, no game/camera writes or
collision/simulation bypass. Approach unmeasured. Existing tapes unchanged.
V1 title-ready gate wrongly blocked gameplay; V2 weak approach input stopped
short; both failures retained. V3 constant strength24..56 solves route.
ASan/UBSan4608sourceangle/matrix directions +cadence/state/deadline PASS;
root-motion plant idealized, not animation/collision model. Existing replay tests
rerun PASS. Build d6a95f83d4f7/3dsxbb5dab2a9092/CIA9a6323e00e16 LOCALNOTDEPLOYED,
replay-walk-v3-console-replay-builds. Only depthheader/replay.c/fh_replay.h differ
from base; same16shaders/game3001dd7502b2/Old80New124, restorationtrue.

Serial NewON/OFF,OldON normal waypoint+A dialogue all status1/savepreserved/
drain/ownedexitPASS64.22/61.58/60.03hostsec. ONNew163requests326draws,
Old140/280, unseeded0; OFFcapture/draw0. NewON1800/1920,OFF1800,OldON1800
images inspected: sharp foreground, blurred ON background, real dialogue.
New161measuredframes18.7146/18.7159ms; renderingCPU+.43775ms ON, audio differs.
Old137frames21.9161ms,noOFFpair. AzaharONLY, no physical timing/image parity.
Local phase reports replay-walk-test/{dialogue-comparison,old-dialogue}.json.
OptionalBLUR_FILTER staysOFFallpresets. Newlinearfree16740096/25165824;
Old2051840/10485760,VRAM3937280,mainstack73044/524288; sparse samples.
NATIVE_POST_DEPTH.md records full memory/evidence. No hardware cost claim.

All owned processes TERMINAL incl80142OFF/2485Old; no active emulator/build/FTP.
Next continue actual renderer parity/performance and physical shadow stability;
do not repeat unchanged cave boots or declare entire-shadow issue fixed.
No goal status change. Keep originalassets/saves private/intact; no blindbuild.

13 October WHOLE-SHADOW FLICKER clarified; goal ACTIVE/INCOMPLETE, no blocker.
User says whole entity shadows disappear/return at high frequency, not speckles.
This is clarification, NOT confirmation that deployed f2c4 was retested. New223
still f2c4fafba819, Old/replay unchanged; no FTP writes this continuation.
Audited exact deployed snapshot, pinned Melee early_queue and SDK publication.
SDK emits framebuffer flush/invalidate and texture cache clear; retained sampler
filter skips only pure descriptors. Do not add blind waits/flushes or claim
missing cache command. New tests/arm_shadow_handoff.c and tools/
test_shadow_sdk_handoff.py execute deployed image begin/end/rebind with actual
ARM retained SDK/framebuffer/texture publishers; 1024 three-slot capture/sample/
reuse cycles, 66558 checks PASS; missing framebuffer flush/cache clear mutants
FAIL expected. This checks command ORDER, not async GPU execution/pixels/physical
flicker. No new renderer change or FPS claim. Existing depth source correction
physical acceptance remains OPEN. Early harness failures were test integration
(missing libctru float symbols, duplicate wrappers, omitted existing NULL sampler
patch), NOT game regressions; final harness uses matching deployed adapters.

Other continuation work: blur-filter-v6-on-new-dialogue-v4 guest exits normally
but coverage assertion FAIL (zero blur draws/requests). Read-only observer now
captures actual NPC volume centre(-5516.215,-785.437,-2960.276), radius48;
Fox(-5530.556,-791.300,-2878.377) ~83 away, outside, despite body proximity.
V5 route FAIL status2 step8 before observer thresholds, not GPU hang. Preserve
failures; inspect recipe before any further blind boot. All run/observer handles
TERMINAL. No new blur build/deployment or affected-gameplay acceptance.
Observer timing exclusion fixed: capture_blur_dialogue writes live marker first;
run_console_headless and analyze_blur_filter reject active/completed observer
runs for timing. test_observer_timing 8 cases PASS. V5 started before runner fix;
its marker excludes timing retrospectively despite old summary valid flag.
Memory panel correction: values USED6795/24576KiB linear,1531/6144KiB VRAM;
match telemetry free17780/4613KiB. Previous missed-allocation claim was a reading
error, corrected docs/AGENTS; memory-panel-reconciliation/result.json records
actual archived owner + telemetry. Sparse samples not continuous highwater.
Current work remains native visual parity/perf, shadow temporal acceptance and
source-triggered depth-blur coverage. New core2 already audio. Never blind-build
six unrelated VRAM WIP files; use snapshot builders. No new hardware probe asked.

13 October SHADOW FLICKER repair DEPLOYED New223; goalACTIVE/INCOMPLETE.
User reports frequent entity-shadow flicker. Read-only console-visual/
20261013-142848-223 confirms installed912b6c79f861, normal shutdown5872,
59868GPUjobs complete/0pending/0failed;5840projectedcasters/0rejected/0empty.
Finalimage inspected Fox+Tricky ThornTailHollow. No temporal proof from logs.
Found concrete source mismatch: original Camera_ApplyDecalViewport near=-.05/
far1 before projected shadows; native receiver had fixed4D24quantum epsilon.
Restored reversedZ scale=-1.05/offset0, same as existinggrounddecals. GEQUAL,
no depthwrites, unchangedfinitecollisiontriangles/shaders/queue/resource life.
No passes/allocations/CPUvertices added. Plausible flicker cause, physical
acceptance OPEN; not a claim every shadow issue is fixed. PinnedMelee
native-3ds/port/3ds/early_queue.c consulted; no relaxedretirement/reset.
Actualoriginalcamera oracle393216casesASanUBSanPASS;old-epsilon/equality/
depthwrite mutants rejected. shadow-decal-depth-test/result.json.
Normalf2c4fafba819/3dsx57d6c0e23c62/CIA456fd01ab0e3 built from installedsnapshot;
sole source delta fh_pica_shadow_depth.h, ALLshaders/gamearchive64298a7423a1
unchanged;Old80/New124,recipesunchanged. shadow-decal-v1-console-setup-builds,
restoredtrue. Futurebuild_blur_filter carries header intoBOTHvariants; existing
localblurpair NOTrebuilt. Neverblindbuild unrelatedVRAMWIP.
Headless exactnormal shadow-decal-after-save-new-v1 PASS157.15hostsec;1600/1750
imagesinspected cave/Fox/HUD/shadowvisible.167936auditeddrawlowerbound/0fault;
Select/drain/ownedexit/saves/assetsPASS. No FPScomparison:diagnostic+external
hostbuild. CORRECTION: panel showsUSED6795/24576KiB linear and1531/6144KiB VRAM;
free17780/4613KiB agrees withfinal telemetry, not missed lowerfree samples. Test is staticcave coverage, not hardwareflickerproof.
New223 app-only deployCOMPLETE console-test/20261013-145001-new223.
All6payloads staged/fullbyte-readback/rename/finalsize. Saves/settings/ISO/
preparedassetsuntouched. HBL SD:/3ds/foxhollow/foxhollow.3dsx;matchingCIAneedsFBI.
NoOld/replayupdate. docs/SHADOW_FLICKER_20261013.md. AllownedprocessesTERMINAL
87105,41755,55182,92803. CurrentturnPROGRESS;no blocker/fullparityclaim.
Blurworkpausedforusersteering: read-only observerbase-variablebugfixed,
target-audit PASS132.13s/status1/drain/ownedexit;actualThorntailinteraction
centre z=-45/radius48 (bodyproximity insufficient). Shortdiagnostictimeout
retained. Revisedinputroutev4 writtenNOTRUN; no sourcefiltercoverageclaimed.
Continue remainingparity/perf afterthis targetedrepair; no newprobe requested.

13 October SHARED POST BUILDS local progress; goalACTIVE/INCOMPLETE,no blocker.
Normal69cb54b9394c/3dsx7bfbcfd406ab/CIA3beda0187483 and replay6e52c12afb98/
3dsx19a96af57a89/CIA47a55a1ba14e built NOTDEPLOYED. Normal/replay now use SAME
motion-v6-setup base; all588 source hashes,111renderer/loader sources,16linked
shaders and archive3001dd7502b2 match. Old80/New124 CIA verified, snapshots and
archive restored true. First comparison correctly rejected replay's old ETC-only
movie/format14-only texture loader; fixed builder, not weakened comparisons.
post-admission-build-verification.json records proof. No physicalFPS claim.
Optional pressure handling/UI/replayfailure evidence follows in prior checkpoint.
New dialogue-v2 TIMEOUT155.56hostsec; observedNPCtail but unfinished, retained.
Shortenedv3 completed179.46hostsec/status1/saves/drain/ownedexitPASS, but added
coverage assertion correctly FAILS:1482captures/ZEROrequests/ZEROfilterdraws.
Four read-only observations1843/1845/1960/2108 show currentTarget0; NPCroot56.65
units away, interaction not activated. Capture1950 inspected world/Fox/NPC/HUD;
flower occludes foreground. Don't rerun guessed routes or claim blur accepted.
Source camcontrol_findBestTarget uses actual hit-volume centre/range/height,
flags, playerCanUseCombatTargeting and optional visibility test; root distance
alone proves no targeting condition. Next locate actual interaction volume from
source/live read-only data or use a known eligible scene, without guest writes.
Observer means NO timing comparison; analyzer now rejects such comparisons.
Lastsampleheap49194280/71303168,linearfree16767744/25165824,VRAMfree3937280,
process103350272/130023424,mainstack73044/524288; sampled,notcontinuouspeaks.
post-admission-runtime.json records scope. Shared normal has no new runtime
startup test; renderer proof and replay evidence don't imply setupUI acceptance.
ALL owned handlesTERMINAL:43494,25102(rejectedoldpair),1382,98411,39816(timeout),
55887,79474(coveragefailed),87109,32566,58669. No FTP or remote/assets/savechanges.
New223 installed912b6c79f861 shadowfetchfix unchanged/physicalacceptancePENDING.
Full visual parity/performance/progression remain unmet. Continue implementation;
no goal complete/blocked status. Never blind-build unrelated VRAM WIP.

13 October optional depth blur: packed GPU target implemented locally, shared replay
built and serial New/Old capture checks pass; see NATIVE_POST_DEPTH.md. Source
filter requests remain unobserved in game; component/spatial checks are not
full visual acceptance. No deployment or physical FPS claim.

13 October motion blur: native temporal owner integrated behind page12 toggle,
OFFall4presets. Exactgame-shader GPU pixel cases and source/order/lifetime/UI
checks pass. NewON/OFF57.1357/57.1404AzaharFPS, +.12229ms rendering CPU;
captures1617/0, zero requestedblurdraws onroute. Gamevisibleblur, Oldcost and
physicalGPUcost remainOPEN. Normal11b2c703d94d/replay4f50326fd425 built;
userrequestsfastNewdeployment, no furtherlongchecks. NATIVE_MOTION_BLUR.md.

13 October v6: splash material descriptions are now load-compiled (360 CPU
bytes), eliminating per-ring construction and overwritten ordinary setup.
All3modes match archived SDK state in3072cases; source/material/lifetime checks
pass. SerialNew/Oldreplays pass; captures inspected. 57.4515/50.5784AzaharFPS,
renderCPUchange+.10781/+.00512ms vs v5: NO speedup established, workloads vary.
Normalbef4c5151310/replay108888816502 builtNOTDEPLOYED; allshaders/game unchanged.
SPLASH REFL remainsOFF4presets; optional integration authorized despite incomplete
distortion/imageparity. FullgoalACTIVE/INCOMPLETE; no physicalperformance claim.
See NATIVE_WATER_REFLECTION.md for budgets/evidence and unresolved work.

13 October09:30UTC: v5 adds per-fragment fog to reflected splashes using the
existing fog-only lighting owner. Noadditional pass/texture/CPUvertices. Shared
normal4f473946578a/replay5d39b1e5e544 builtNOTDEPLOYED. Source/linkedshader/state
checks and New/Oldreplays pass; no measuredbudget breach. Original-imageparity,
distortion and physicalperformance remainopen; seeNATIVE_WATER_REFLECTION.md.

13 October: SPLASH REFL is now an optional native gameplay stage, OFF in all four
presets per user authorization. Source colour/alpha/foam equations and persistent
geometry are checked; indirect distortion, reflected-ring distance fog and
original-image acceptance remain incomplete. NewON/OFF and OldON replays pass;
OFF skips captures and extra capture storage. No physical performance claim.
See NATIVE_WATER_REFLECTION.md. Latest local normal55d2fba567ee and replay
aeb5b937222c are built, not deployed; full renderer/performance goal remains open.

13 October 08:17 UTC: actual cave screenshots rejected the first index-cache
build despite passing counters. Restored lost capture raster branch; New/Old
shared replay images now show silhouette, counters/saves/retirement pass.
Exact archived normal/replay rebuilds change only capture header; converter,
recipe/game/shaders unchanged. New local ISO setup4e24995bc96d, package
0996053d1fec9b0f. Earlier191072361db3f64b ZIP WITHDRAWN; not deployed.
Conversion62m58s under70 acceptance preserved for unchangedconverter, no rerun.
Full renderer/performance goal stillACTIVE/INCOMPLETE. No physical speedup.
See NATIVE_SHADOW_INDEX_CACHE.md and AGENTS.md for evidence/budgets/limitations.

13 October deployment update: normal ISO-setup build87ca5200d925 (source17e602b1c3a8) including shadow vertex sharing was deployed/readback-verified to New223 at explicit user request. Replay/Old builds were not updated. Complete actual ARM original conversion is verified; remaining conversion and physical renderer acceptance are pending. See ISO_SETUP_VALIDATION.md and AGENTS.md. Later local installer-only progress/I/O candidates remain undeployed.

13 October: exact native shadow vertex sharing implemented locally. See NATIVE_SHADOW_VERTEX_SHARING.md. Full ordered geometry/bone equivalence, lifetime and serial New/Old cave checks pass. 35% fewer collection shadow vertices; +144384B minimum linear headroom in cave. CPU submission unchanged, hardware FPS benefit unproven. Initial shared replay and normal setup builds were not deployed at this checkpoint; see later deployment update above. Full parity/performance acceptance remains open.

12 October: direct native actor material publication passes actual prior/new state comparisons, shared builds and serial New/Old replays. Combined rendering CPU improves about0.10/0.09ms locally with no new GPU storage; no physical FPS claim. Source-preserving index-order audit finds a larger modeled GPU workload opportunity but is not enabled. See NATIVE_ACTOR_LIVE_STATE.md and NATIVE_VERTEX_CACHE_AUDIT.md.

12 October: rigid actor visibility candidate removes under1% actor index work on moving Thorntail, but adds .042ms rendering CPU after refinement. Source/owner/linked-shader checks and runtime pass; not promoted or physically accepted. See NATIVE_RIGID_VISIBILITY.md. No hardware request while unavailable.

12 October: native source-collision shadows integrated with current materials/cloud renderer. New/Old cave coverage passes, images inspected, shared build/archive/shaders verified. New combined scene/submission -.389ms versus visible-mesh baseline, but full effect +.729ms vs same-binary OFF still exceeds .5ms budget. Local only; source limits/dynamic receivers/softness/physical cost remain open. See NATIVE_COLLISION_CURRENT.md.

12 October: combined spatial/camera shadow candidate locally reduces cave receiver indices51.71% and draws26.15%, with +0.258ms emulator CPU. New/Old cave and original-geometry moving route pass with inspected images and no declined plans. Not deployed or physically accepted; full shadow cost/parity remains open. See NATIVE_SHADOW_SPATIAL_GROUPING.md.

12 October: persistent shadow index-range hierarchy prototypes V1/V2 built and tested locally, neither promoted. V1 saves23% receiver indices but adds1.54ms CPU; V2 bounds draw count but saves only5.1% indices and adds0.61ms. Original triangles/shaders retained; source/sanitizer checks and cave completion pass. No deployment or physical performance claim. See NATIVE_SHADOW_INDEX_RANGES.md.

12 October: post-light specular family restored locally in four native TEV stages. Source/owner checks and serial New/Old palace comparisons pass; CPU +0.157/+0.116ms, unchanged New memory, increased command traffic. Shared normal/replay not deployed, physical cost and sky repair still unverified. See NATIVE_POSTLIGHT_SPECULAR.md and current AGENTS.md.

12 October: ordinary world-material specialization passes exact SDK-publication comparisons and shared New/Old cave checks. Measured emulator scene+submission improves 0.115/0.130 ms with unchanged memory; modest local gain, not hardware FPS. Normal startup/toggles pass. Latest local world-material-v1 builds remain undeployed pending sky repair. See NATIVE_WORLD_MATERIALS.md.

12 October: V7 direct block transforms remove the camera-inverse round trip. New ON/OFF and Old ON replay/receiver coverage pass, but added shadow CPU remains 0.7195 ms versus the 0.5 ms budget. Experimental only; no physical claim or deployment. Both toggle+sky normal releases still await sky repair.

12 October: V5 exact shadow-range reuse removes most added indexing CPU cost. Its first Old-profile run failed visual coverage (8 missing plans); V6 uses measured decoder scratch and restores all 11 receiver plans with zero missing draws under existing caps. Native shadow coverage now rejects that recorded failure. Candidate-only, not deployed or physically accepted; full-shadow CPU budget still unmet.

12 October: experimental point-receiver V4 trims 18.779% of cave shadow input points with unchanged draws, but adds 0.05386ms emulator CPU; the shadow feature budget still fails. Not accepted/deployed. Exact source and ownership checks pass; physical execution and throughput unvalidated. See current AGENTS.md and NATIVE_SHADOW_TRACK_RECEIVERS.md.

12 October: prepared native buffer layouts replace per-draw descriptor construction/scanning. ARM source/state equivalence and serial Old/New cave plus normal startup/toggles pass. Measured emulator rendering CPU improves ~0.26?0.28 ms; physical acceptance and sky repair remain pending. See NATIVE_PREPARED_BUFFERS.md. No deployment.

12 October: buffer publication candidate reduces descriptor commands ~94.1% but raises measured rendering CPU ~0.10 ms in both emulator profiles; not promoted or deployed. See NATIVE_BUFFER_PUBLICATION.md. Stable toggle release and pending sky acceptance are unchanged.

12 October current local boundary: shared normal30a24c54674a/replayc47c685d5c2e
replace per-publication attribute construction with ten prepared SDK layouts.
New/Old cave checks pass; measured rendering CPU sum improves about0.071/0.063ms
in short Azahar comparisons. This is modest, not physical target acceptance.
No console deployment; skybox repair still pending. See NATIVE_VERTEX_LAYOUTS.md
and the current AGENTS.md for authoritative build/evidence boundaries.

12 October current local boundary: particle distance fog restored in native GS,
shared normal7d9acccf1435/replay6ee2e2f05bec NOT DEPLOYED. Source/linked shader/
handoff checks and serial New/Old cave replays pass; incremental emulator CPU
+0.022/+0.023ms, not hardware performance. Distant particle image acceptance
remains open. See docs/NATIVE_PARTICLE_FOG.md and current AGENTS.md.
Requested both-console deployments await the unresolved intro skybox repair.
Earlier chronological shadow notes below are historical; do not use them as
the current installed-build boundary (see AGENTS.md).

11 October latest: V3 passes all15 physical mask/depth/fog controls (152157-results). Missing cave shadows remain UNFIXED. Installedgame unchanged; actual cave GPU commands now captured (shadow-live-draws-v1-new), no physical diagnosis yet. Local source collision receiver compiler/oracles pass but NOTintegrated or budget-accepted.

11 October latest: V2 shadow probe PASSES all9 physical pixel controls (20261011-150409-results); game cave shadows remain unfixed. V3 actual-fog15case probe3d55adefeeb5 passes Azahar and is deployed/readback-verified New223 (151303); hardware fog acceptance pending. No main/replay/Old/save changes. See SHADOW_GPU_PROBE.md.

11 October shadow diagnosticV1 hardware collection completed but its32px DisplayTransfer is invalid (minimum64). Do not infer a gameplay shader/depth failure from its images. Corrected64px probe183528216b18 deployed and exact-binary Azahar pixel checks pass; hardwareV2 pending. Normalgame shadows remain unfixed.

11 October: depth candidate4ff085bfe304 also FAILS hardware shadow visibility (console-visual/20261011-142505-223). Automatic9case shadow probe35a36a6387a0 deployed New223, exact-binary Azahar passes; physical probe pending. Game builds unchanged. See SHADOW_GPU_PROBE.md.

11 October: projected-shadow v5 rejected on hardware (SHADOW SHAPE hides cave shadows). New depth-state candidate 4ff085bfe304 normal / b9c3cfa271d3 replay deployed and readback-verified; physical acceptance pending. Azahar cave walk passes, no physical fix/performance claim. See docs/NATIVE_PROJECTED_SHADOWS.md.

11 October native projected-shadow cave candidate integrated/deployed New normal
1a1a16f3e6cd, physical acceptance pending. Actual finite world surfaces replace
the single-plane band; source authored masks avoid unnecessary capture. New
Azahar cave idle59.83FPS with +~1.02ms auxiliary CPU cost, movement still dips.
Old candidate54.11FPS is emulator-only, not a console forecast. Partial receiver
policy/softness/self-shadow parity remains; see [current scope](NATIVE_PROJECTED_SHADOWS.md).

11 October C-menu direct-UV PHYSICAL SUCCESS in probe; normal repair DEPLOYED.
New223 console-cmenu-probe/20261011-091330-results COMPLETE/source d4dde6ecc398cd96.
All8images inspected. Full/shared/standalone andnoQoutput only256pixels/topstrip.
Case6direct ordinaryUV restores2800pixels, BYTEIDENTICAL minimalGPUskin(case7);
oracle2801/IoU.999643. Thus workingUVarithmetic provenonhardware, NOTmissing
assets/culling/linkage/Qrouting. Exactsharedtemporary/controlflowfailure still
unknown; doNOTapplydirectUVgloballyoverfur/specular coordinatecalculations.

Normal59d8e6f354e8/CIAea7b4911b58a DEPLOYEDNew223, console-settings-update/
20261011-092846-223 COMPLETE; changedpayloadsstaged/fullbyte-readbackverified.
Saveshashesunchanged/preferencespreserved. Originalassets reusedwithprior
completedbyteproof+currentdescriptors/sizes, notfresh51MBreadback. Old/replay
unchanged. HBL SD:/3ds/foxhollow/foxhollow.3dsx ready; CIA samefolderneedsFBI.
Physicalnormalmenu acceptance PENDING; next runnormal/openCmenu/Select/ftpd.

Newnative pica_ui_model.v.pica43instructions, c0..95/same7outputs. All prepared
unlitUI materialkinds selectpipeline5, NOTmodelIDspecialcase. Reusesresident
vertices/indices/bones/poses, GPUskin/directUV/tint/opacity/fog outputs; noCPU
vertices/extrapasses/worker/texturestorage. Source-selectedfur/shadow/world clear
UIowner; existinginvalidation/retirement handleshandoff. Existing3shaderbinaries
andgamearchive BYTEIDENTICAL. 3owners/selectorheaderchangedplusnewshader.
PinnedMelee completeoutput/ownership consulted. BuildUIprogramseparate43vs
shared511, no relaxationof512limit. Fullrendererparity/performanceINCOMPLETE.

Actualfloat24test32768unlitposes/22bones/blends/UV/colours:917504lanesEXACT.
Linkedshaderhasha71888170432 matchestestedbinary. Exactnormalheadlesscmenu-ui-new
PASS95.123hostsec/2160retraces/Select/logdrain/ownedexit; localsavehashunchanged.
3060UIdraws, all3panels510each/pipeline5. Images2010/2130inspectedfullredbacking,
world/Fox/HUDintact. World/UIinterleaveeachframe; releaseCstickdidNOTclose
latchedmenu, so no separateclose-menu acceptance. No redundantboot repeated.
Matched300framewindow emulatorreportedsubmit.165580->.165653ms,GPU.677980->
.677780ms, interval16.71372->16.71377ms; minorworkloaddifference, NOTtotalCPUor
physicalGPUperformance. Normalheap53887088/71303168(+288observed), linearfree
18656128/25165824,VRAMfree4822016,process102694912/130023424,stack73008/524288.
No newphysicalFPSclaim. docs/NATIVE_UI_MODEL_SHADER.md records scope/evidence.

Buildcmenu-ui-v3 isolatesexactcmenu-depthsnapshot/libgame, restoresWIPfinally.
v1patchCRLFassertionandv2ARMprintfu32Werrorcaughtpredeployment; checksretained.
pica_ui_shader_patch ALREADYappliedtoWIP; doNOTdoubleapply. Newbuildergenerates
UIshaderforallsharedvariants. AllownedjobsTERMINAL. Nextcollectphysicalnormal
menuacceptance, thenremainingrenderingissues; no complete/blockedclaim.

11 October PHYSICAL C-menu isolation completed; missing background reproduced.
New223 collection console-cmenu-probe/20261011-090209-results COMPLETE/source
65aba52ff2049afc; corrected9663c9b7de4e completes8cases andexits normally.
All8images inspected: shared scene shader only256pixels/topstrip; no-cull,
floatattrs anduniformclipscale also256. Solidgeometry2975pixels/complete;
texture rectanglescorrect; oracle2801/minimalGPUskin2800 withIoU.999643.
Thus texture/fullshader path narrowed, NOT geometry/assetabsence orculling.
No established UVmath versusoutputmapping cause yet. Azahar rendersfullpanels
forallsamecases; thisisnowexactsmallphysicalreproducer. Olddump193 unchanged,
notnewcrash. ProbeNULLsampler fixphysicallyaccepted for thisrun. Gameunfixed.

NewUVprobe 1e6dff12b342/sourced4dde6ecc398cd96 DEPLOYEDNew223 verified:
artifacts/console-cmenu-probe/20261011-090845/transfer.json COMPLETE/staged+final bytes.
Same remote SD:/3ds/foxhollow-cmenu-probe/foxhollow-cmenu-probe.3dsx, automatic.
Case1standalone fullsceneprogram;case4onlydropTEXCOORD0Woutputsemantic;
case6directv2/constant ordinaryUVbypassesfur/specular temporary; othercontrols
unchanged. All8archivedshaderentries reconstructinstalled511program EXACTLY
beforevariants. NoQkeepscode/uniformsidentical. DirectUVdiagnosticonly, NOTfur
replacement. PinnedMelee outputinitialization/SDKbindings consulted; SDKoutmap
generatorand3dbrewregisterdocreviewed. No rawregisterwrites/CPUrender path.
18linkedARMnullsamplercasesPASS; 4.882s exactbinary headlessPASS, imagesinspected,
source/bounds/coverage/drain/ownedexit. HardwareUVprobe results PENDING.
Crude initialCindentation Werror stoppedfirstbuild; fixed,guardretained.
Main1433/replay/Old229/assets/saves/preferencesunchanged. No gameplayFPSclaim.
Originalprobe ELF/3dsx/fixture/main/builder archivedcmenu-probe-hardware-v6.
AllownedjobsTERMINAL. Rendering goalINCOMPLETE; next readphysicalUVcases.
NeverblindbuildsixVRAMWIPfiles; productionweather-v3 stillneedsUIdepthpatch.

11 October C-menu PROBE crash identified/repaired; GAME MENU still unresolved.
User OS crash collected read-only console-cmenu-probe/20261011-084149-results.
Luma dump193: CPU data abort PC0x103ecc/FAR0xC, C3D_TexBind(1,NULL), main.c73.
Dump instruction bytes match archived crashed probe21b3b7eb3717. Stock SDK
texture binding dereferenced NULL before checking texture type. Probe omitted
the known fix already linked by game/morph probe; this was a probe regression,
not evidence of GPU failure. Pinned Melee citro3d_fix.c/SDK consulted again.
Corrected probe links narrowly patched texture.o via citro3d_null_texture.py.
Build now gates actual linked ARM binder:18 cases/pagezero unmapped. Dedicated
test_cmenu_null_texture.py reproduces both old faults/exact firstPC+address,
checks dump bytes and passes corrected probe/current game18cases each.
Azahar alone missed nullread; preserve ARM fault gate, not another blind rerun.

Probe9663c9b7de4e/source65aba52ff2049afc DEPLOYED New223/readback-verified:
console-cmenu-probe/20261011-084714 COMPLETE, staged+final bytes all3payloads.
Remote SD:/3ds/foxhollow-cmenu-probe/foxhollow-cmenu-probe.3dsx automatic8cases.
Exactbinary cmenu-isolation-safe-sampler-new PASS5.407hostseconds, all8cases,
sourceID/bounds/coverage/ownedexit; contact sheet inspected, panels/textures
present. Physical rerun pending; no physical crash-fix acceptance yet.
Main1433c8ace4cf/replay/Old229/assets/saves/settings unchanged. Probecollector
handles ftpd450 missingfiles and fetches bounded latest Luma dump. Crashed ELF,
fixture/main/builder preserved under cmenu-probe-crashed. All ownedjobs terminal.
Fullrendering goal incomplete, no performance/menu parity claim. Next collect
corrected probe hardware outputs and inspect before choosing game correction.
Never blindbuild six VRAM WIPfiles; productionweather-v3 needs priorUIdepthpatch.

11 October C-menu depth candidate FAILED physical acceptance: user still sees
missing backing panels. Read-only console-visual/20261011-081146-223 COMPLETE;
LCD/log4884 matches, three captured CPU publication records match emulator
exactly, activeuniformmaxdiff0. Prior081108 FTP read timedout; one retryworked.
Do NOT claim depth ALWAYS fixes this. It remains source-state correction only.

New isolated CMENU GPU probe21b3b7eb3717/source3c0990dd67a16b5d DEPLOYEDNew223:
console-cmenu-probe/20261011-083451 COMPLETE, staged+final full byte readback.
Remote /3ds/foxhollow-cmenu-probe/foxhollow-cmenu-probe.3dsx runs automatically;
eight image cases then exits. No FUR setting/save load/input needed. Reopenftpd.
NORMAL1433c8ace4cf/replay/Old229/assets/saves/settings unchanged this turn.
Probe only: compact/cull, no-cull, solid, oracle-passthrough, float-input,
texture-rectangle, homogeneous-scale, minimal-GPU-skin. Private original images
and compact/index fingerprints match physical; earlier emulator open-menu pose
used explicitly, not physical shutdown pose/complete prior GPU state. Same
511instr production scene shader. Offline oracle/float fixture preparation is
NOT a CPU gameplay renderer. No raw GPU regs/depth readback/new game worker.
Pinned Melee explicit flush/fence lifetime and SDK queue/uniform APIs consulted.

Final exactbinary Azahar pass6.891hostsec; all eight images inspected through
byte-identical V5 captures; ordinary/reference coverage IoU1, no-cull/float/
scaled/minimalskin meanRGBerror.0004583byte per channel across whole image.
Culling changes overlapcolour, wholeimageMAE.40143 vsno-cull, not missing pixels.
Not physical correctness/performance evidence. Tests check clearcolour/geometry
bounds, queuecompletion/sourceID/ownedexit; probe reserves8MiBheap/4MiBlinear/
512KiBstack,256KiBcommands,768000BVRAMtarget,288000BCPUstaging;8RGBfiles2.304MB.

Failure trail preserved: strict uniform equality failed because captured open
pose differs from shutdown records (not evidence of gameplay mismatch); V1
probe used boolean bit instead of SDK0x68+bit, corrected+asserted. V2/V3/V4
automatic relative tests missed distorted captures. Initial edge-on-pose
explanation was WRONG: copying retired linear GPU output into ordinary CPU
heap before fileIPC fixes images. RGB conversion alone did not. Added absolute
image bounds/clearcolour gates. Precise emulator/IPC cause not established;
do not call that a game fix. V2 briefly staged082211, superseded by final083451.
All owned jobs terminal; rendering goal incomplete. Next collect currentprobe
with tools/collect_cmenu_gpu_probe.py, inspect all images before choosing fix.
Never blindbuild VRAMWIP; productionweather-v3 stillneeds prior UIdepthpatch.

11 October New223 CMENU depth candidate DEPLOYED/readback-verified.
Normal1433c8ace4cf/CIAa77459962fe1; sharedOld80/New124. Old/replay unchanged.
console-settings-update/20261011-075946-223 COMPLETE, saves/preferences preserved.
Usable physical074852 capture still missing panels; all three captured menu
active uniforms, texture/geometry fingerprints and recorded draw state EXACT
emulator match. CPU observations do not prove GPU contents/full fragment state.
Candidate forwards original disabled-depth ALWAYS instead of GEQUAL; source
216064states/1024viewport/12221depth PASS, previous-comparison mutant rejected.
Hardware cause and repair UNCONFIRMED. No new shader/draw/geometry/allocation.
Exact normal cmenu-depth-new PASS101.143hostsec, Select/drain/ownedexit/save
checks, complete red panel image2010 inspected; three publication records match.
Same game archive and all three shader binaries, scene still511 instructions.
New finalheap53886800/71303168,linearfree18656128/25165824,VRAMfree4822016,
process102694912/130023424,mainstack73008/524288. Not physical FPS evidence.
HBL /3ds/foxhollow/foxhollow.3dsx ready; CIA needs FBI installation to update icon.
Next physical check: FUR OFF, load save/open C-menu, Select then ftpd without
relaunch. Production weather-v3 snapshot needs pica_ui_depth_patch once; WIP
already corrected. Diagnostic snapshot is not the next production baseline.
All owned build/test/emulator/FTP processes terminal. Rendering goal incomplete.

11 October New223 CMENU diagnostic DEPLOYED/readback-verified, NOT A FIX.
Normal4da2defd6ea8/CIA8b2b24d3128b, sharedOld80/New124; Old229/replay unchanged.
console-settings-update/20261011-071349-223 COMPLETE; saves/settings preserved.
HBL /3ds/foxhollow/foxhollow.3dsx ready; CIA requires FBI installation.
Archived deployed-renderer/game/shaders preserved; only two owners gain bounded
menu CPU publication observation. 4824B BSS +sampler pointer, first texture/
geometry FNV fingerprints and latest95 uniforms/booleans/clip/draw metadata
for models1212..1214. No SDK setter/getter, GPU readback, allocation or SD write
in draw hook. Logging at shutdown. Does NOT measure actual GPU consumption.
tools/cmenu_capture_patch.py injected only by isolated cmenu-capture-v3 builder;
live production owners/WIP/gamearchive restored. Never use this diagnostic
snapshot as next feature baseline: production work remains weather-v3-builds.
V1 invalid --console option; V2 newerweather/oldheader ABI; V3 isolates newly
added translation units, unchanged archived renderer. Failures retained.

Exact normal cmenu-publication-new PASS92.54hostsec/Select/drain/ownedexit;
three records decoded, each510draws/12vertices/36indices/2bones/bools1;
2010 image inspected with complete red background. Current save unchanged.
Main511/fur/material shader bytes and gamearchive identical to installedprior.
New finalheap53886800/71303168,linearfree18656128/25165824,VRAMfree4822016,
stack73008/524288,process102694912/130023424. Diagnostic, not FPS comparison.
Actual emulator draw observer cmenu-draw-capture-new also captures three full
draws; float24 VM finite/fullalpha/exactUV, image2010 complete.123.39hostsec,
not timing evidence. Observer source restored/baseline preserved.

User FUR OFF stillmissing; fresh SD geometry match proof and matching physical
LCD failure documented above/below. NEXT hardware ask uses NEW diagnostic:
launch HBL normal, load save/open C-menu briefly, Select, reopen ftpd. Collect
and compare with cmenu-capture-v3-builds/emulator-publication.json using
tools/analyze_cmenu_capture.py. Do NOT ask same old photo/FUR test again.
Menu rootcause/fix and remaining visual parity/performance still OPEN.
All owned build/emulator/FTP handles TERMINAL; no goal status mutation.

11 October C-menu HARDWARE failure confirmed, still UNFIXED.
Matching physical LCD/log collected read-only: console-visual/20261011-063846-223,
retrace1507/stamp4000718191372. Icons/top labels present; background absent.
324 ring submissions/zero hidden do NOT prove visible output. User's FUR OFF
check did NOT restore background; do not repeat capture/FUR questions.
Fresh bounded SD directory+four model packets1212..1215 match local exact bytes:
console-visual/cmenu-geometry-20261011-145833. No remote writes or save changes.
25%-CPU replay diagnostic cmenu-slow-new still shows complete red/blue panels;
103.68 hostsec/status1/saves/drain/ownedexit PASS, not performance evidence.
No menu production repair/deployment yet. Continue investigating hardware draw
state against captured failure; do not call emulator success hardware acceptance.

11 October local native precipitation and C-stick background investigation.
Replay194b2b6ce0fa BUILT/TESTED NOTDEPLOYED; no new normal build. Source rain/snow
now has immutable PICA corner IDs + live cloud uniforms; exact shared-shape
batching cuts draws20070->3345 for214749 triangles. Actual source/owner/shader
tests and linked-byte check pass. SamebinaryNewON adds~.240ms combined rendering
CPU in Azahar; OldON passes with image inspected, no pairedOldOFF or hardware
claim. WEATHER optional/OFFboth. See [scope/cost/memory](NATIVE_WEATHER.md).

User's missing C-stick background is NOTFIXED. Exact deployed normal5af250cf5d6d
3dsx/current console save/settings/original cache still shows complete panel in
Azahar, as do replay variants and desktop reference. Console's available LCD
capture is stale relative to its log. Requested Select while broken menu remains
open, then ftpd; installedapp already saves image. No console changes this turn.
See [reproduction evidence and limits](CMENU_BACKGROUND_INVESTIGATION.md).

11 October opacity-mask texture colour selection implemented locally, NOT
DEPLOYED. Normal5af250cf5d6d/CIA02d7224e3855; replay45d6a97b1d65/CIAbabceefa15d9.
Native RGB reconstruction now preserves separate mask alpha, including LIGHT
MAPS routing. No extra passes/images/shaders/CPU vertices; one4Bdraw counter.
1,376,256source-equation cases and200,000owner bindings pass. Isolated96-tile
Azahar GPU test passes within0.53/255 per channel; actual RGB/alpha inspected.
New cave regression passes, +0.013970ms combined CPU/unchanged measured RAM,
but masked draw countZERO; live affected-material acceptance is OPEN. Its image
also shows less haze than the preceding run, an unresolved recurring difference.
Do not call this a fix for the reported whiteArwing/trees/fur/SDpauses or a clean
visual regression. Shared344sources/archive/511+214+304shaders/CIA80+124verified.
See [implementation, failed probe and limits](NATIVE_TEXTURE_OPACITY_CHANNELS.md).

11 October texture-channel repair now composes with LIGHT MAPS; local only,
NOT DEPLOYED. Normalb7edeed89c4c/CIA4b3d3af928ca, replay8d9a6fb8396f/CIA44e86b268a93.
Same344 native sources/archive, unchanged511/214/304 shaders, Old80/New124CIA.
SerialNewbefore/afterON andOldON pass state/saves/drain/ownedexit; both1410
captures inspected. New+0.011046ms combined renderingCPU;59.8312AzaharFPS,
Old56.2349 with no paired priorON. Physical timing and broader visual parity
unvalidated. New2145/Old2013 repaired mapped draws,0missing,0environment-family
draws. No extra draws/images/shaders;16Badditional metadata/material,Newheap+4016B.
LIGHT MAPS whole-feature budget still exceeds its gate; defaultOFF unchanged.
See [scope and evidence](NATIVE_TEXTURE_CHANNELS.md). Reported SDstalls, white
Arwing, disappearing trees, fur and day/night/shadow acceptance remain open.

11 October native texture-channel selection implemented and checked locally,
NOT DEPLOYED. Normalb73739b78bc8/replay960273bc8d06 share344native sources,
game archive, unchanged main511/fur214/material304 shaders andOld80/New124CIAs.
Final New/Old cave replays pass with2145/2058 repaired draws and no unsupported
nonzero selections in that scene. New combined rendering CPU +0.074058ms;
Azahar59.8312/57.5109FPS does not establish console performance. Heap+6056B
New, no additional linear/VRAM allocations. Partial material families and all
reported Arwing/tree/fur/SD-stall acceptance remain open. No console files changed.
See [implementation, failures and evidence](NATIVE_TEXTURE_CHANNELS.md).

11 October local height-fog publication cache validated, NOT DEPLOYED.
Shared normala76161780652/replay1db501a91022; native sources/archive/main511/fur214/
material304 andOld80/New124CIA verified. SerialNewbefore/afterON/NewOFF/OldON
passed state/saves/drain/ownedexit; images inspected. SmallNewcombinedCPUchange
-0.0883ms; ONstill+.2405msabove0.20gate. HEIGHTFOGremainsOFFdefault. No physical
speedup or reported visual-defect cure claimed. BoundedGPUstate observations
also pass; v2 atlas snapshots exact14perrun andLUTsequal, butbothv2captureslack
priorhaze variation,so rootcause remainsopen. GS particles are not observed.
See [tests, budgets, failures and observation limits](NATIVE_HEIGHT_FOG.md).

11 October New223 normal5dd6a33aec44/CIA bd33b0dc9555 DEPLOYED and readbackverified,
manifest031944-223 COMPLETE; saves/configs preserved, Old andpendingVRAMreplay
unchanged. Clean Select exit captures completedLCD400x240 for diagnosis, with
zero recurring screenshot work. ActualNew/Old normal checks and image inspection
PASS; shared343sources/archive/exactshaders verified. Capture may be one frame
older than scanout, does not capture crashes and is not a visual/FPS repair.
Last read-only console collection031739 contains prior logs, no new physical
evidence. Active-light optimization now included opt-in, still above CPU gate.
See [capture acceptance and limitations](NATIVE_LAST_FRAME_CAPTURE.md).

11 October active-light map optimization locally checked, NOT DEPLOYED.
Normal43049ce63b9b/replay418b3032dc93; linked float24 outputs identical332928
vertices, source/owner/lifetime checks pass. NewON/OFF and OldON serial palace
checks pass; inspected1410 images retain world/actors/glow/HUD. No physical claim.
New feature CPU delta+.334588ms >.20ms gate: still not accepted within budget.
LIGHT MAPS remains opt-inOFF, ordinary511/fur214 programs unchanged. Map81,
material304/512. Full visual acceptance and user's Thorntail defects remain open.
See [active-light evidence](NATIVE_DIFFUSE_ACTIVE_LIGHTS.md); installedappsunchanged.

11 October New223 normal deployment COMPLETE: 46baff465715/CIAced987c20ce7,
console-settings-update/20261011-023844-223. Four changed application/identity
payloads staged, byte-readback verified, renamed and final-size checked. Original
geometry/cache reused through prior complete byte proof plus current descriptors/
topology/sizes; no fresh bulk readback. Saves/settings preserved. Matching3dsx
ready at SD:/3ds/foxhollow/foxhollow.3dsx; CIA requires FBI installation at
SD:/3ds/foxhollow/foxhollow.cia. PendingVRAMreplay8e4bbc1c9563 andOld229 unchanged.

This normal update includes reduced duplicate memory reporting plus the previously
local opt-in HEIGHT FOG world family/immutable atlas. HEIGHT FOG stays OFF by
default; source camera-intensity motion and physical GPU cost remain unaccepted.
No claim that SD stalls, white Arwing, invisible trees or full fur/shadow/lighting
parity are fixed. Physical targets and overall goal remain INCOMPLETE.

FinalnormalNewheadless check memory-cadence-normal-v2-new PASS38.947hostsec,
startup/Start/cockpit/Select/drain/ownedexit, no native test hooks,900image inspected.
44memorysamples->14reports/28983B; finalstack watermark retained. Heap46277472/
71303168,linearfree18518656/25165824,VRAM4822016,process103186432/130023424,
stack72956/524288. Shared341native sources/archive/shader identity verified,
Old80/New124CIA. Native source changes vs latestatlasbaseline only platform_3ds.c
and fh_memory_cadence.h; game/renderer unchanged versus that LOCAL baseline.

Follow-up moving visibility route completed in61.79hostsec with actual last console
mask3866625, originalgeometry/textures and unchanged e520binary. Five stable source/native
material captures; tree images remain present. Inspected1200/1350/1650/1800; Arwingis
only distant background, not the earlier close composition. No reproduction/fix of the
physical white-Arwing/disappearing-tree fault; timings excluded (observer, differing path).
No console/saves changed. See native-observation-layout-test/visibility-findings.json.

11 October visibility investigation: fixed read-only CIA executable layout accounting in
inspect_native_objects (pinned Azahar adds separately aligned initialized data and BSS;
3DSX aligns their sum). Actual replay differs4096B; normal/earlier replay differ0.
ELF/CXI metadata/hash checks and all code/counter/heap/canary guards retained. Corrupt
hash/pages/BSS/address tests rejected. Three diagnostic New runs terminal; v1/v2 material
capture failed, v3 captures five stable Arwing/fern native/source material records.
Selected-owner checks replace checking unrelated whole-array per-frame timestamps;
no guest writes. Existing e52070361090 binary/geometry/sourceassets unchanged.

All sampled material images ready, alpha255, frames unchanged. Arwing body512x512,
canopy16x16; fern196x512 in256x512 padded storage, scale .765625x1. This is resident
state, NOT proof of visible draw in each sample. Inspected v3 1410 image has Fox at
cave exit and trees; Arwing outside view. Prior sd-bursts-v2-new1650 has textured
Arwing. Physical defect NOT reproduced/fixed. Swaphol texture metadata:203/592 records
request mipmaps; fern does NOT. Native mip omission is a broader gap, not fern cause.
Actual source bump family still unsupported on Arwing body; not established sole white
cause. No new performance claim or deployment; saves preserved. Evidence
native-observation-layout-test/{result,visibility-findings}.json and run visibility-materials.json.

11 October local height-fog atlas revision: source/filter/lifetime/SDK checks pass,
Old ladder correctness and no material CPU regression observed. One initial atlas
preparation ~7.01 emulator ms and +53,248 B linear retained explicitly. Camera stays
above fog range; repeated live intensity and physical GPU view sampling unvalidated.
No console deploy; no general visibility/fur/stutter fix claimed. See NATIVE_HEIGHT_FOG.md.

11 October local native height-fog world consumer built/tested, NOT DEPLOYED.
Normala33c15095766/replay8d85e463f74d share source/game/shaders and Old80/New124 grants.
The first version added ~2ms rendering CPU; keeping one world program bound reduced
that to ~.343ms New/.330ms Old in serial Azahar cave comparisons. Optional/defaultOFF;
masked/layered/actor/indirect families and moving-camera update cost remain incomplete.
No new hardware result or general fix for FS stalls/missing surfaces/fur. See
[NATIVE_HEIGHT_FOG.md](NATIVE_HEIGHT_FOG.md) for scope, costs, memory and evidence.

11 October bounded file/thread SD burst recorder implemented and New223 normal
fc3841012fb3/CIAa5507299e029 DEPLOYED. Four changed app/identity files staged,
byte-readback verified, renamed and final sizes checked; original assets reused
with prior completed byte proof/current descriptors/sizes, not freshly reread.
Saves/preferences preserved. Replay8e4bbc1c9563 and Old229 remain unchanged.
CIA needs FBI installation at SD:/3ds/foxhollow/foxhollow.cia; matching3dsx ready.

This adds4520B fixed RAM and no gameplay trace writes/allocations/worker. It
groups completed calls by file, operation and launch-local thread over <=1s,
retaining32 bursts with >=2calls and >=100ms call spans.8pending slots and
evictions are explicit. This is IPC including scheduling/waits, not CPU time
or an exact main-thread stall partition; overlaps are not additive. An incomplete
marker prevents old completed logs masquerading as new after a crash. Existing
slow-call trace remains. Collector and replay archival include sd-bursts.csv.

Actual wrapper test1000x2ms reads->2 exact file/thread bursts; sanitizer tests
cover concurrency, handles, errors, gaps, wrap/pressure. Analyzer rejects
incomplete/truncated/invalid traces.601-row observation test remains passing.
First ARM build caught uint32_t printf mismatch; corrected before runtime.
Shared replayd9614c762951/CIA89141e2a6e58 built locally, NOTDEPLOYED;336native
sources/game archive/main511+fur214+material270shader identity verified,
CIAOld80/New124. New Arwing route66.12hostsec passes state/saves/drain/owned
exit;1650 image inspected and replay archival byte-exact.25burst records name
disc/cache/movie/texturepack callers. No new normal boot; unchanged normal
UI evidence and shared-owner/shader proof reused. Azahar54.402FPS vs54.536
prior route, differing animation/workloads, no physical prediction or speedup.
Newheap56421728/71303168,linearfree16495744/25165824,VRAM4822016,
stack73068/524288,process103215104/130023424. No new Old runtime.

Read-only FTP collection003756 is byte-identical old evidence; no new physical
result. This deployment includes prior local no-light environment correction
and exact diffuse-slot reuse, not a claim to fix all missing surfaces/stalls/fur.
See [SD burst trace](NATIVE_SD_BURSTS.md).

11 October resident source-identity cache experiment SHELVED after measured
loading checks. New texture-pack reads311->304; Old530->530. Most repeat
identity verification was already served by the existing page cache, so the
extra128/32KiB working set is not justified by this route. First v2 candidate
also discarded the page cache prematurely (344 reads); corrected and caught
by a material-pressure regression before v3. No installed files changed.

Actual image owner/real pack tests:936 entries,8424 repeated acquisitions,
exact byte/quality/dimension identity, LRU/pressure/retirement and two mutants.
Serial v3 New/Old Arwing replays pass state/saves/drain/owned exit;1650 images
inspected. Game/shader bytes unchanged; Old80/New124 verified. Physical stalls
are NOT fixed. Active sources restored with checked inverse preserving VRAM
WIP; experimental replaye590099d5acb/CIA85e05e369698 retained privately,
no normal build. Latest stable remains diffuse-slots-v1-builds.
See [experiment, limits and measurements](NATIVE_IMAGE_IDENTITY.md).

11 October exact diffuse light-map sharing implemented locally. Normal
c8feed02b443/CIAe1033ea24e0b and replay9462cc49cea0/CIAe4f2247a4a49 are
built, NOT DEPLOYED. Actual readonly palace capture found identical logical
slots0/2 in30 observations. Exact assignment now shares their resident GPU
result, reserving all still-needed targets before any overwrite. Captured
state replay needs39 map draws versus66, with identical shader inputs. No
approximation, atlas, CPU geometry or new worker;20B extra static CPU state,
unchanged8852B linear mesh/12288B VRAM. Actual owner sanitizer tests cover
301290 logical samples/100 lifecycles; publication/overwrite mutants rejected.

Serial NewON/OFF and OldON palace checks pass saves/state/drain/owned exit;
New/Old1410 images inspected. NewON59.87616/OFF59.82397 AzaharFPS; combined
scene/submission incremental CPU0.310547ms still EXCEEDS0.20ms gate, so LIGHT
MAPS remains optional/OFF. OldON57.48930 has no matchedOFF; no console speed
claim.335 normal/replay sources, unchanged game archive and exact511/214/270
shader programs verified; sharedCIAOld80/New124. No new normal UI run since
UI is unchanged. Details and memory: [exact slot sharing](NATIVE_DIFFUSE_SLOT_SHARING.md).

No FTP or remote changes. Installed New normald6068a861f8f and experimental
VRAM replay8e4bbc1c9563 remain unchanged; Old unchanged. This does not fix or
establish acceptance for FS stalls, disappearing objects, full fur or shadows.

11 October experimental native-image VRAM replay deployed New223:
replay8e4bbc1c9563/CIAae3682970035, menu sourcead2f22e49522. Four app/identity
files staged and byte-readback verified, renamed and final sizes checked.
Five unchanged original asset/cache payloads reuse prior completed byte proof
plus current descriptors/topology/sizes, not fresh bulk readback. Saves and
preferences preserved. Normald6068a861f8f and Old229 unchanged. CIA requires
FBI at SD:/3ds/foxhollow-replay/foxhollow-replay.cia; matching3dsx ready.

This is optional immutable native image placement, defaultOFF, not a proven
physical speedup or visibility fix. No runtime promotion/eviction; original
linear source retained until existing owner joins copies/draws. Cap3MiB with
1MiB VRAM reserve, L4/A4 stay linear. Actual owner/SDK sanitizer and mutation
checks pass, shared builds verified, serial NewON/OFF and OldON original-Arwing
runs pass state/saves/drain/ownedexit;1650 captures inspected. New54.09/54.66
AzaharFPS with differing animation/workloads, no established speedup; OldON
46.95FPS has no matched OFF. Physical bandwidth/safety remain unvalidated.
See [implementation, evidence and budgets](NATIVE_IMAGE_VRAM.md).
Deployment manifest: artifacts/console-settings-update/20261010-234303-223/manifest.json.

11 October local no-light environment correction. Normalf75b075a8585 /
replay7295f9362325 BUILT NOT DEPLOYED. Actual source GX disables lighting
when its mode-0 environment map has no selected lights, passing white material;
the default native approximation wrongly applied ambient. Corrected SDK state
publication, preserving dark fog-only state. No new GPU calls/passes/allocations
or shader instructions. Original-control/native-owner sanitizer tests and
old-behaviour/fog mutants pass. Serial New/Old Arwing-route state/save/drain/
owned-exit checks pass;1650 captures inspected. Unpaired Azahar54.54/47.34FPS
does not establish console performance or fix all reported visibility/lighting.
InstalledNew remains d6068a861f8f/e6178b4e5cd0; no FTP/remote changes this turn.
See [scope, evidence and memory](NATIVE_ENVIRONMENT_UNLIT.md).

11 October New223 fur-projection deployment COMPLETE: normal
`d6068a861f8f` / CIA `238bbeb7a501`; replay `e6178b4e5cd0` / CIA
`b72e38226b55`. Eight changed app/identity files staged and byte-readback
verified, then renamed and sizes checked. Unchanged original asset/cache
payloads retain prior completed byte proof plus current descriptor/topology
and sizes; no fresh bulk asset readback. Saves and preferences preserved.
Old229 unchanged. Matching3dsx ready; install the CIA with FBI to update HOME:
`SD:/3ds/foxhollow/foxhollow.cia` (replay sibling folder/app name).

The normal app also passed its own32.43-second headless check: FUR enabled
through the bottom menu, ordinary Start to cockpit,1140 actual fur draws,
saved option mask1064961 with valid checksum, Select/drain/owned exit;900
capture inspected. New/Old replay checks and measured cost are detailed in
[fur projection](NATIVE_FUR_PROJECTION.md). This repairs one confirmed noise
coordinate error, not all fur effects or other reported visibility/lighting/
shadow defects. Physical projective-sampler performance and appearance still
need validation. FTP collection230702 contains the prior run, not new evidence
for the installed logging changes or a physical stall improvement.
Manifest: `artifacts/console-settings-update/20261010-231226-223/manifest.json`.

11 October fur projection repaired locally. Normal `d6068a861f8f` and replay
`e6178b4e5cd0` are built, NOT DEPLOYED. The old native noise mapping discarded
the original interpolated Q coordinate. Both native shell paths now use PICA
projective sampling; no new pass, texture storage, CPU vertices or shader
instructions. New matched cockpit CPU preparation/submission is2.402663ms
versus2.401824ms before, unchanged within this short emulator comparison.
Both New/Old correctness runs pass; captures inspected. No physical GPU cost
or full-fur fidelity claim. Wind and other reported FS/visibility/lighting/
shadow defects remain open. See [source error and evidence](NATIVE_FUR_PROJECTION.md).
Installed New223 remains normal9eec94e236e3/replay97a579346965 from the
previous completed deployment; no FTP or remote changes in this follow-up.

11 October New223 update COMPLETE: normal `9eec94e236e3` / CIA
`208ed7ac4914`, replay `97a579346965` / CIA `e0949e10fa82`. All eight changed
app/identity files staged and byte-readback verified, renamed and final sizes
checked. Ten unchanged original asset/cache files reuse prior completed byte
proof plus current descriptors/topology/sizes. Saves and preferences preserved;
Old229 unchanged. Matching3dsx ready; CIA requires FBI installation at
`SD:/3ds/foxhollow/foxhollow.cia` (replay sibling folder/app name).

Normal rendering telemetry now summarizes every frame at about1Hz, preserving
timestamped >100ms stalls and final totals; replay remains per-frame. Host
serializer test reduces this log's bytes27x, not a claim that total SD traffic
or physical stalls improve by27x. Shared270-instruction material bank avoids
shader-code uploads on eligible entry switches. Optional native diffuse maps,
masks/layers and compact shadow preparation are accumulated local work now in
these apps; LIGHT MAPS and shadows stay OFF by default. Added light-map CPU
cost still exceeds its provisional gate, so no full-feature performance
acceptance. No established physical speedup or fix for disappearing trees,
white Arwing, full fur/day-night/shadow fidelity or all filesystem stalls.

Normal startup/toggle/persistence/Select and palace replay checks pass; captures
inspected. Latest read-only console collection222419 is unchanged from211130,
not new hardware evidence. Detailed scope and validation:
[render logging](NATIVE_FRAME_METRICS.md),
[material shader ownership](NATIVE_MATERIAL_SHADER_BANK.md).
Deployment evidence: `artifacts/console-settings-update/20261010-223515-223/manifest.json`.

11 October native diffuse masks/layers implemented, but New CPU budget NOT
accepted: exact V4 ON/OFF adds.297975ms against.20ms. Normal077590850267/
replay28948d103c2a are built, not deployed. New/Old captures and replay checks
pass; masks still have ZERO palace runtime draws. Normal startup/toggle/save-
preferences/Select check passes. No physical performance or whole-game visual
acceptance. Bounded map reuse V5 is experimental only; current owner restored
to exact V4 after no established gain. [Scope and evidence](NATIVE_DIFFUSE_FAMILIES.md).

11 October native diffuse material integration accepted for local budget checks,
not full visual parity or hardware performance. Normal56340ab2a26a/replay
f1bb348b9800 are built, not deployed. LIGHT MAPS OFF default; native single-layer
and untextured families connected, layered/masked/C2/fur still incomplete.
New/Old paired palace checks and normal startup/toggle/persistence/exit pass;
captures inspected. Added local CPU.07353/.03832ms passes.20/.50ms gates. Old
linear headroom3,688,320B and normal heap40,936,928/50,331,648B in tested scene.
No claim that white Arwing, missing trees, FS stalls or complete lighting/shadow/
fur fidelity is fixed. [Full scope](NATIVE_DIFFUSE_MAPS.md).

11 October native diffuse-map prototype: actual ARM GPU owner and separate73-
instruction kernel pass source-grid pixels and dependent texture sampling in
isolated Azahar.332928 shader vertices;9maps within.5011 colour levels;3same-
queue consumer samples byte-identical;8960linear/12288VRAM bytes reclaimed after
completion. First probe's wrong Y-origin oracle remains recorded separately.
No gameplay material integration, physical cost or visible repair accepted.
White Arwing, missing trees, FS stalls, lighting/shadows and fur remain open.
Sampler correction affects none of2327 inventoried stored textures, so is not
evidence of a fix for those reports. No deployments or save changes this turn.
See [diffuse-map evidence and remaining gates](NATIVE_DIFFUSE_MAPS.md).

11 October v2 local shadow checks complete: normal007333766cbd/replayc602175f7873,
not deployed. Inactive morph-capable models now use compact base plans; active
morphs retain native live lanes. Shadow draws15015->6435 at unchanged8790210
indices; New same-binary whole-shadow CPU cost .462081ms passes the provisional
.5ms cave gate. OldON replay and memory check passes with3,899,648B linear free.
NewON/OFF and OldON images inspected, saves/results/drain/ownedexit pass. No
physical GPU/FPS or full shadow-fidelity acceptance. Defaults remain OFF.
Two ENOSPC host failures remain excluded; fresh-label replacements passed.

11 October v1 compact shadow palette publication updated locally, not deployed.
Normal ca2122c8c415/replay55832803ca68 preserve geometry and shader code.
Source/data/ownership checks pass, but the serial cave comparison shows no
useful gain: its compact casters already use one draw. The remaining casters
were excluded by a blanket morph-capability check; this run reports no active
morph updates and does not identify the caster from counters alone.
See [evidence and limits](NATIVE_SHADOW_PALETTES.md).

11 October New223 deployment COMPLETE: normal `f50343cd2e56` / CIA
`82d3285f6677`, replay `e8eefd5944cf` / CIA `72ee1193db8b`. Layered
secondary-light materials and reduced native memory-log traffic are installed.
Eight changed application/identity files received full staged byte readback,
rename and final-size verification. Ten unchanged original asset/cache files
reuse prior completed proof plus current descriptors/topology/sizes; they were
not bulk-reread. Saves/preferences preserved. Old229 unchanged. Matching3dsx
ready; CIA requires FBI installation. SPECULAR remains opt-in; existing saved
options are preserved. No physical stall, speed or whole-parity claim.
Evidence: `artifacts/console-settings-update/20261010-202815-223/manifest.json`.

11 October native memory telemetry simplified locally, not deployed.
Normal f50343cd2e56 / replay e8eefd5944cf retain the tested game/shader code.
Removing retired-renderer counters cuts the memory log from331KB to106KB at
the same53 samples. Actual serializer sanitizer checks and one palace replay
pass; memory/stack values retained, image inspected, no physical stall-fix claim.
See [scope, measured costs and remaining issues](NATIVE_MEMORY_TELEMETRY.md).

11 October layered character materials integrated locally, not deployed.
Normal6118a2078f78 / replayce4a25e5c902 share checked game code, native sources,
511/214/197-instruction programs and Old80/New124 CIA profiles. Independent
animated UVs, a half-map GPU owner and shared fragment-fog lighting restore the
common two-layer secondary-light family without CPU vertices or another actor
pass. New/Old cockpit and New palace ON/OFF checks pass; added CPU rendering
work ~0.14/~0.14/~0.15ms, within provisional gates. Images inspected, but no
physical performance or full visual-parity claim. SPECULAR remains opt-in OFF.
Console logs collected read-only are unchanged; no deployment this turn.
SD stalls, disappearing trees/white Arwing, fur and broader lighting/shadow
issues remain unresolved. See [details and evidence](NATIVE_LAYERED_SPECULAR.md).

11 October layered-material resource and fragment-fog probes pass locally.
An optional GPU half-map owner is implemented (12KiB VRAM/92B linear, no game
caller yet). Actual four-stage GPU samples differ by at most3 colour levels;
PICA distance-attenuated fragment lighting reproduces four fog ramps within1
level and frees the third UV without vertex-fog approximation. The197-instruction
specialized actor shader preserves the other tested outputs. No normal/replay
build or deployment, no physical performance or character-image acceptance.
See [implementation, failed probes and remaining integration](NATIVE_LAYERED_SPECULAR.md).

11 October layered eye-material work is local/inactive: source audit confirms
61 layered secondary-light declarations need distinct UVs. A four-stage
combiner and separate201-instruction three-UV shader are tested; 8-bit material
error and fog interpolation still prevent visual acceptance. No runtime or
console build changed, no FPS claim. The new full FTP collection matches the
previous log exactly. See [prototype evidence and integration gates](NATIVE_LAYERED_SPECULAR.md).

11 October New223 deployment COMPLETE: normal `4ad2bfc39ced` / CIA
`b93ef5c89545`, replay `01d268cc7891` / CIA `d5fcb77b8a07`. New texture
page caching plus the accumulated environment-normal/unlit-material corrections
are installed. Eight changed application/identity files received full staged
byte readback, rename and final-size checks. Ten original asset/cache files
reuse prior completed byte proof plus current descriptor/topology bytes and
sizes; large asset bytes were not freshly reread. Saves/preferences preserved.
Old229 is unchanged. Matching3dsx ready; CIA requires FBI installation.
Physical stalls, fur and reported visibility/lighting acceptance remain open.
Evidence: `artifacts/console-settings-update/20261010-190758-223/manifest.json`.

11 October texture reader cache LOCAL: normal `4ad2bfc39ced` / CIA
`b93ef5c89545`, replay `01d268cc7891` / CIA `d5fcb77b8a07`, not deployed.
Native materials now attach the existing staged page reader: 256KiB New /
64KiB Old normal RAM, reclaimable under heap pressure. No GPU allocations,
per-frame geometry work, new worker or changed shaders. Full pack/lifetime/
corruption tests and serial original-Arwing New/Old replays pass; images
inspected. Pack file reads 15.10->6.88MB New, 15.10->13.32MB Old, identical
bytes returned to the renderer. Rendering cost effectively unchanged;
Azahar55.04/47.37FPS is not physical performance. Physical stalls and reported
visibility/fur/lighting defects remain open. See [evidence](NATIVE_TEXTURE_WINDOW.md).

11 October local unlit texture source rule repaired for the audited37 material
sections; numeric source/SDK comparisons and serial New/Old cockpit runs pass.
Newnormal08e74edfe122/replay9f352cf3933d NOTDEPLOYED; no physical/full-image
acceptance, unchanged CPU/memory budgets. Neither reported tree/Arwing family
is repaired by this rule. The archived desktop image confirms green cockpit
illumination is intentional; earlier tint observations are not defect proof.
See [unlit source evidence](NATIVE_UNLIT_TEXTURES.md).

11 October local environment-normal correction passes actual linked shader,
source-pose and serial New/Old cockpit checks with unchanged shader/memory
budgets. It repairs backwards hemisphere reconstruction, not full lighting
or original-image parity. Green illumination and incomplete fur remain visible.
Normal `dd1ac4d7ac4c` and replay `bc8b58203277` are not deployed; console
installations below remain current. See [evidence and limits](NATIVE_ENVIRONMENT_HEMISPHERE.md).

11 October New223 deployment complete: normal `626d217324e9` / CIA
`762ef1732702`, replay `9bcad101a4fe` / CIA `9acc42b6e738`, with matching
original-geometry22-slot pack and lossless cache. All18 changed payloads staged,
byte-readback verified, renamed and final-size checked. Saves/preferences
preserved; Old229 unchanged. CIA requires FBI installation; matching3dsx ready.
Performance-geometry22-slot pack remains local; original-asset installer does
not replace the generic performance pack. Earlier status entries are historical.
See [changes, tests and unresolved issues](RENDERER_UPDATE_20261011.md).

11 October GPU shell expansion integrated and checked locally. Full-source fur
passes the New cockpit CPU gate (+0.330329ms versus same-binary OFF), preserving
shell/triangle counts. New/Old runs and shader/ownership tests pass; no physical
GPU, complete fur fidelity or deployment acceptance. See
[integrated GPU fur evidence](NATIVE_FUR_GPU_EXPANSION.md).

11 October original-geometry22-slot counterpart now passes independent corner
equivalence, New/Old runtime/residency and matching-cache checks. Not deployed.
GPU shell-expansion214instruction prototype passes source/float24 tests but has
no gameplay integration, switching-cost or physical acceptance. See
[GPU prototype scope](NATIVE_FUR_GPU_EXPANSION.md) and
[original geometry evidence](NATIVE_ACTOR_PALETTES.md).

11 October local source-layer/22-slot regrouping work: source fur density now
selectable, but whole-effect New CPU gate remains0.615ms>0.5ms and wind/material
parity remains absent. No new deployment. Ordered native geometry/global bones/
morph references exact across81changed actors; gameplay cost validation ongoing.
See [evidence and limitations](NATIVE_FUR_SOURCE_LAYERS.md).

10 October: New223 normal3ddc672fddee/replaya05052e6976b deployed and readback-verified. Includes alpha-source correction, fur coverage and prepared-file stdio fix. New/Old Azahar non-disc/movie read calls562 to302 with unchanged steady rendering cost. Physical pauses and reported visibility/lighting/fur defects remain unaccepted; user reports morphs repaired. See [NATIVE_ASSET_IO.md](NATIVE_ASSET_IO.md). Earlier deployment/status notes below are historical.

10 October local fur coverage now uses the original indirect alpha/intensity mask through a third native sampler; New/Old checks add about0.011/0.010ms renderingCPU and16KiBlinear in the cockpit. Built normalc70bf119b342/replay50e46003c21e, NOTdeployed. Physical flicker, indirect wind and full parity remain open. See [NATIVE_FUR_COVERAGE.md](NATIVE_FUR_COVERAGE.md).

10 October: physical probe v3 completed, rejecting the padded morph loader in all12 comparisons; packed attributes matched all12. New223 normal913f01dfaa2a/replayd5f2e028e258 deployed/readback-verified with packed static lane; gameplay visual confirmation remains pending. See [MORPH_PACKED_FIX_20261010.md](MORPH_PACKED_FIX_20261010.md). Earlier padding acceptance statements below are superseded.

10 October V3 probe8ab51c5ec8a2 deployed New223/readbackverified after identifying and reproducing null C3D_TexBind CPU data abort. This repairs the diagnostic; game already has guard. Physical morph/fur defects remain OPEN; no main deployment. See MORPH_GPU_PROBE.md.

10 October probe251564 OS-crashed before first reference result. V2 diagnostic83adb34a03fb installed/readbackverified New223, not an established fix. Both emulator modes pass; physical morph/fur rejection remains. See MORPH_GPU_PROBE.md.

10 October hardware rejection: user retried normal70b95ca077de; morph distortion and fur flicker persist. Candidate NOT accepted. New automatic morph-input probe251564459bc4 is installed separately on New223; normal gameplay build unchanged. See docs/MORPH_GPU_PROBE.md.

10 October: hardware reports widespread morph distortion (MORPHS OFF removes it), fur flicker and eye problems. Repair candidate normal70b95ca077de/replayb6a6e70f48b5 deployed New223; hardware visual acceptance remains OPEN. See MORPH_FUR_REPAIR_20261010.md.

10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

10 October LOCAL shadow submission follow-up: replayf06f317f3d25/normal96e47d68053e,
not deployed. Load-prepared24B shadow vertices/palette-compatible material batches
retain exact topology, weights and animation; morphs use their live native lanes.
New cave shadow draws23595->15015, identical8790210indices. Submission saves
only.059ms; total shadow CPU increment.553ms still exceeds.5msbudget. Keep OFF
by default on both models. Cache cap512KiBlinear/64KiBCPU, peak250KiBlinear in
checked scene; New/Old memory reserves retained. Same shader511/512, no worker.
All835asset equivalence and owner/handoff tests pass; serialNew/Old captures
inspected. Softness/finite receivers/self-shadow and physical performance remain
unaccepted. See NATIVE_PLANAR_SHADOWS.md. This does not close full shadow parity.

10 October LOCAL water-fog follow-up: replay58a29afdb3dd/normal56de63e56c34
retain scene distance fog on native splash/droplet batches without another draw
pass or allocation. Shared shader507/512; exact linked shader and state-owner
tests pass, serial New/Old route captures inspected. Submission remains about
3.62/3.49ms, but different animation workloads preclude a speedup claim. Full
water fidelity, distant-effect visibility and physical performance remain open.
No deployment. See NATIVE_WATER_FOG.md.

10 October additional LOCAL work: source-selected actor alpha lighting is now
implemented in replay f8a1635f6bd1, under the existing LIGHTING toggle. Source
equations pass 9,792 comparisons; short New/Old unaffected-scene checks show no
material regression and unchanged memory. They contain zero alpha-lit actors;
affected-scene and physical validation remain open. This does not close reported
face/RGB problems or full diffuse-lighting parity. NATIVE_ALPHA_LIGHTING.md
records scope and actual linked shader budget. No console installation changed.

10 October current state: New normal e783636ad1e8/replay b49219fa4c26 were
uploaded and readback-verified (console-settings-update/20261010-070609-223).
Old remains normal697b6b1d9794/replay3c0f3e2d7aa7. New includes native distance
fog and opt-in circular ground shadows; the latter remain OFF by default because
their cave CPU budget failed. CIA copies require FBI installation for HOME icons.
The local planar silhouette candidate reuses resident GPU-skinned geometry and
has a separate opt-in SHADOW SHAPE control. It is not deployed and does not close
projected-shadow parity. See NATIVE_PLANAR_SHADOWS.md and NATIVE_SHADOW_RECEIVERS.md.

Implemented means a native path exists and has been exercised, not exact
original-game parity in every scene. Partial includes unsupported cases and
visible or validation defects. Missing means no general native replacement in
the deployed build. Local candidates are labelled explicitly. This inventory
is not exhaustive visual acceptance of every asset/effect through a playthrough.

| Status | Feature | Scope and remaining work |
|---|---|---|
| Implemented | Top-screen output | Native 400x240 monoscopic output; runtime Old/New selection. |
| Implemented | Camera and viewport | Live view/projection, clipping/scissor and UI depth ranges. |
| Implemented | Base world geometry | Persistent native terrain/room draw plans, including original repaired cave geometry. |
| Implemented | Base actor meshes and skeletal animation | Persistent actor meshes, live bone palettes and GPU skinning. Separate morph limitation below. |
| Implemented | Texture storage and sampling | Prepared resident PICA textures, compressed source-pack loading, frame selection, UV scrolling and supported wrap/filter state. |
| Implemented | Basic vertex/material colours | Vertex colours, live object alpha, and the source solid-colour material family fixed in 6cd59125022e. |
| Partial, local only | Animated object colour fades | Native TEV interpolation now consumes original damage/object tint state without another pass. Source equations and New/Old checks pass; a palace capture exercises changing Krystal tint. Whole-game/hardware acceptance and heavy-fog ordering remain open. See NATIVE_COLOR_FADE.md. |
| Implemented | Core 2D HUD and text | Fonts, rectangles, loaded HUD images, health/staff/compass/inventory displays and loading/title images. |
| Implemented | Sky backdrop | Time-of-day texture blending and environment-transition snapshots. |
| Implemented | Stars | Native star batches with original placement, colour/size updates and RNG ordering. |
| Implemented | Cloud, sun and moon base paths | Native cloud models/depth states, lightning glare, celestial placement and moon phase. Broader weather parity remains partial. |
| Implemented | Movie presentation | Prepared PAL movie playback, including mapped cockpit screens; deployed asset is reduced resolution. |
| Implemented | Bottom-screen interface | Old touch item stick, New recording controls, graphics controls (nine pages in the local droplet candidate), optional FPS/RAM/audio/SD/GPU panels. Settings persist on clean Select exit; independent original geometry/texture choices require restart. |
| Partial | Whole-game geometry/UV fidelity | Shared bone-boundary/shape/UV repairs include 37 audited bone-boundary errors. An optional original-topology pack removes decimation from actors and environments, with increased geometry cost. Prepared texture resolution and native material limitations remain; every model, pose, room and texture is not visually accepted. |
| Partial | Actor material layering | Base textures, opacity plane, dual UV and supported two-layer combinations exist. Arbitrary layer combinations and opacity-plus-extra-layer cases are not complete. |
| Partial | Diffuse lighting | GPU vertex lighting and original object-light selection exist. Spot and per-vertex distance attenuation are incomplete; reported colour errors are not all resolved. |
| Partial | Environment illumination | Native analytic hemisphere/fragment-light approximation replaces diffuse sphere maps. It is not the complete original environment/specular treatment. |
| Partial | Transparency, alpha cutouts and depth | Blending and several special callback states exist. Additional source alpha/depth-rule repair is deployed on New, not the older Old installation. Whole-game acceptance remains open. |
| Partial | Eyes, facial appearance and animation | Eye geometry/UV and layer-lighting repairs exist. Local native morph position lanes now consume original animation results, with GPU skinning and a MORPHS toggle. New/Old cave and New cockpit checks exercise live updates; all blink/speech states, lighting and hardware acceptance remain open. See NATIVE_MORPHS.md. |
| Partial | Particles, fireballs, glows and trails | Native geometry-shader template/dynamic paths exist; user confirmed recent particle handoff repair. Reflected/unsupported paths and all effect variants are not complete. |
| Partial | Weather | Sky/cloud/lightning and supported particles exist; every rain/snow/storm variant and its interaction with surfaces is not verified. |
| Partial | Animated terrain | GroundAnima and XYZAnimator producers publish native position changes. Other deformation families need coverage. |
| Partial | Pause/communicator holograms | Native projected mesh, texture and opacity-mask path exists; source distortion is only handled for the supported zero-indirect case. |
| Partial | C-stick inventory | Native ring, selected icons, unavailable-item dimming and labels exist; complete layout, transition and item-state visual acceptance remains open. |
| Partial | Contextual A and lock/enemy indicators | Native live state adapters exist; normal gameplay image coverage for all interaction and damage transitions remains incomplete. |
| Partial | Scene-specific render callbacks | Several UI, movie, moon and additive/post-state callbacks have native replacements. Unknown callbacks are counted and omitted, not sent to the old renderer. |
| Partial | Player/object ground and projected shadows | New has opt-in circular contact footprints and simple decals. Local animated planar silhouettes reuse GPU skinning; original soft projection, finite/multiple/wall/dynamic receivers and self-shadowing remain missing. Neither approximate path has passed the overall feature performance gate. |
| Partial, local only | Fur/fuzz rendering | Native first/last shell LOD reuses resident meshes and GPU skinning behind FUR, default OFF. Short New/Old cockpit checks exercise it; matched New Azahar CPU addition is 0.250 ms. Noise-colour perturbation is restored in the local fur-noise candidate (+0.00217 ms/frame New Azahar CPU, unchanged draws/memory). Indirect masks/wind, special shell/shadow callbacks, broader materials and physical acceptance remain missing. See NATIVE_FUR.md. |
| Missing | Dynamic reflected-scene passes | Reflection texture generation/update is skipped. Diffuse environment illumination is not a dynamic scene reflection. |
| Partial, local only | Specular/highlight materials | Local d8f70d951f45 generates the three source light maps on PICA and consumes them in a first single-texture material family. SPECULAR page9 defaults OFF. New ON/OFF 59.83 AzaharFPS with +.054ms rendering CPU; Old 57.39/58.60 with +.031ms rendering CPU and an unresolved larger `other` delta. Masks/layers/bump families, original-image acceptance and hardware costs remain open. See NATIVE_SPECULAR.md. |
| Missing | General projected-light/shadow texture chains | Original arbitrary projected-light material stages are not implemented by the native actor compiler. |
| Partial | Distance fog | New uses native linear eye-distance plus a procedural alpha LUT and source range/colour, after material colour fades as in fixed GX fog. Narrow-range accuracy, every material and physical validation remain open. Old installation predates the feature. |
| Missing | Height-dependent heavy fog | The original height/texture/warped-noise material fog is separate from distance fog, occurs before object colour fades, and is not restored. |
| Missing | Screen-space heat/distortion | Original heat effect and general distortion/render-target paths remain skipped. |
| Partial | Motion blur and blur filters | Native temporal blur is integrated behind an OFF-by-default toggle; GPU pixel/order tests pass, visible gameplay and physical cost remain unvalidated. The separate blur filters remain missing. See NATIVE_MOTION_BLUR.md. |
| Missing | Water secondary passes and caustic effects | Some base water-related surfaces may render, but dedicated secondary world/reflection/distortion/caustic passes are not restored. |
| Partial, local only | Dedicated splash geometry | Persistent engine-19 rings and GPU band transforms behind SPLASHES; Thorntail movement exercises the path on both emulated profiles. WATER DROPS now submits live droplets as one immutable-template batch, but clear affected-pixel/original-image acceptance remains open. Original reflected material, ripple/wake geometry and the separate lightmap quad family remain absent. Deployed builds omit these additions. See NATIVE_SPLASHES.md and NATIVE_WATER_DROPS.md. |
| Missing | General warped-noise/decal materials | True decal-layer materials still lack a general native combiner replacement. |
| Partial, local only | Rendered morph/vertex-animation parity | Bounded position lanes now consume original morph outputs, with live cave/cockpit coverage and a MORPHS toggle. Hologram consumer is wired but its affected scene remains unvalidated. Skeletal skinning is separate. See NATIVE_MORPHS.md. |
| Missing | Captured menu backdrops and slot shadows | Generated framebuffer-backdrop and pause-slot-shadow paths are not generally restored. |
| Missing | Other secondary world passes | Non-base world pass requests are counted and omitted; base geometry coverage does not prove these effects exist. |

Evidence anchors: tools/pica_only_override.py (removed callback tail and secondary world passes), tools/minimal_render_override.py (remaining skipped shadow/reflection/postprocess calls, with restored sky/cloud/particle overrides applied later), tools/pica_actor_material_override.py, tools/pica_actor_lighting_override.py, tools/pica_environment_override.py, tools/pica_{hud,ui_model,glow,sky,stars,cloud,world_positions}_override.py and native-3ds/source/platform_pica_*.c. Some comments in older modules describe an earlier implementation stage; the active dispatch and replacement code take precedence. 6cd exact normal/Old/New and palace checks are recorded in artifacts/research/pica-scene-rewrite/granular-checks/validation.json; no physical FPS improvement or full visual parity established.

10 October candidate update: actor alpha/depth plans match 1280 original-GX branch combinations and 327680 alpha samples under ASan/UBSan; the former zero cutout threshold mutant fails. Four serial console-binary palace checks passed with captures inspected. Native submission in Azahar changed from 3.74360 to 3.74078 ms (New) and 3.75328 to 3.75858 ms (Old), with unchanged linear/VRAM use. It does not add shadows or restore other missing passes. New texture/performance candidate 596131750fcd/e414ce885bd6 includes this repair; its deployment status is recorded separately.

# Rendering and performance acceptance

10 October implementation direction: the user authorizes continued restoration behind graphics controls, including New 3DS worker processing for effects the PICA cannot express. This does not authorize a general software-rendering fallback or remove the performance gates. Prefer native raster, texture-combiner and vertex work. Any CPU effect path must have bounded buffers, explicit ownership, a measured preparation/copy/upload/wait budget, and a runtime Old/New policy. The existing New audio worker occupies core 2; additional jobs must not assume a free core or starve audio. Account for memory contention as well as worker execution time. An OFF switch is useful for diagnosis and user choice, but does not establish parity for an unimplemented or unusably slow effect.

All issues below are required work, not optional polish. A code change or a complete
asset inventory does not close an item: capture the relevant changing game state,
inspect it against the original, check workload/memory, and distinguish emulator
evidence from physical-console acceptance. User permits reduced detail while
preserving gameplay and essential cues. Target60 FPS; steady30 is the minimum.

Current deployment is recorded at the top of this document; the issue evidence
below includes earlier stages. No steady30/60 physical performance acceptance.

| User-reported issue | Current evidence | Still required |
|---|---|---|
| Incorrect shadows, geometry and lighting | Several native material, alpha and world-light corrections; incomplete coverage | Scene-by-scene original-reference comparison, including moving shadows and lighting changes |
| Missing Fox/Krystal eyes | Wrong offline flag rejection fixed; Fox cockpit eye visibly restored on both emulated targets with existing native material | Capture Krystal and blink/animation states; console confirmation and performance accounting |
| Malformed Peppy face | Face/glasses restored in inspected console-menu-feature-coverage capture570 | Confirm on console with acceptable performance; retain regression |
| Triangular WarpStone head | Isolated original model220 restores all five surfaces exactly. Camera-asserted native before/after captures now show repaired mouth/neck and upper-head detail; static comparison also passes | Physical confirmation, dialogue/warp animation coverage, and acceptable whole-game performance; restoration adds1230triangles and costs~2.2% in this emulator view |
| Distorted walls/floors from decimation | Original swapholbot cave restores traversable arch; future world reductions have sampled geometry/UV/color/winding guard | Repair existing affected banks beyond that cave, verify exits and UV interpolation without unacceptable GPU cost |
| Frozen contextual A indication | Original callback decisions and native combiner corrected; differential state tests pass | In-game image evidence of unavailable/available transitions at interaction range |
| Frozen enemy health indication | Original health-driven callback alpha published; differential state tests pass | Normal combat captures showing changing damage/health states |
| Pause/hologram transparency | Inspected map communicator hologram and Save carousel repairs; no added geometry pass | All relevant pause and cutscene variants, reference comparison and console confirmation |
| Broken C-stick inventory | Live selected icons/labels/masks repaired; New PDA and Old fuel-cell captures | All usable item states, disabled items, selection/use/cancel and complete layout/opacity acceptance |
| Framerate and no regression from fidelity fixes | No verified steady30/60 physical acceptance; some topology restoration adds vertices | Matched-scene physical CPU/GPU/workload evidence, gameplay lows and extended play; offset added fidelity work |
| Multi-second runtime stalls | Bounded texture fallback and expanded save metadata timing implemented/tested locally; physical benefit unverified | Eliminate runtime stalls; loading-only allowance applies at startup/initial loading, not ordinary movement |

Relevant records: `artifacts/research/native-callbacks/integration.json`,
`lod-features/integration.json`, `native-hologram/integration.json`,
`pause-materials/integration.json`, `model-constant-rgb/integration.json`,
`hallway-fidelity/integration.json`, `file-texture-hints/integration.json`.
These are separate partial results, not evidence of full-list completion.

Shared CPU-skin audit: `lod-features/cpu-skin/integration.json` and `distortion.json`.
37changed lists across33models;14now exact original surfaces, but8other lists
worsen at least one sampled shape/UV metric by over1%. The global candidate is
not accepted for deployment. Finite static bind-pose measures do not prove
animated visual fidelity or console performance. Both must be checked.

Keep this table current alongside IMPLEMENTATION.md. Closing every visual row does
not close the overall goal: normal progression, controls, meaningful save/reload,
three cold boots, repeated transitions, extended gameplay, memory headroom and
physical performance acceptance are also required.

8 October follow-up: refined CPU-skin candidate462b65e7740a has no measured
bidirectional shape/UV max/RMS/p95/p99 regression above1e-5;21lists are exact
source. This removes the first candidate's8static metric regressions, not the
need for animated visual acceptance or performance accounting. WarpStone
replays diverged; whole-head acceptance remains open. Candidate not deployed.
The new save-isolated console replay menu is validated on both emulated targets
and can collect physical timings; it does not close any rendering/FPS defect.

8 October surface follow-up: topology71d63f3b113f restores all original WarpStone
model bytes, including list4(16->52triangles). Scene1349c950a429/cache824991978610
are local only. Full-face capture remains incomplete, and cameras differ across
the successful replay windows: no matched speed/physical acceptance. General
model-surface guard is now enabled, including cluster publication.
The omitted Krystal1/list15 and Fox465/list16 require the animated warped-noise
decal combiner. A missing-eyes causal link and its native repair are still open.

8 October continued: missing-eye compiler fix; goal ACTIVE.
CORRECTION to earlier omitted-draw audit: model.h defines DECAL_LAYER=0x100000,
not0x20000. The latter is WATER_CAUSTIC. Models1/list15 and465/list16 already
fit the existing native two-layer material. prepare_scene_geometry rejected
these52-triangle eye lists using the WRONG flag. It now rejects the actual
unsupported decal bit; runtime material checks are unchanged. Actual-source
regression admits both eye lists and rejects otherwise-identical true decals.
Do NOT implement warped-noise rendering for these eye draws. A procedural PICA
prototype passed an isolated blend test but was NEVER exercised by these eyes;
it was removed from production and retained only in decal/prototype/status.json.

Final scene6879c8991bb6/cache3f18387ef113 with topology71d63f3b113f restores
2eye packets, preserves2100others including all1267world packets. Scene+6784B.
Corrected final compiler reproduces the tested packets byte-identically.
Creader2102payloads/46356checks pass. Files remain LOCAL, not deployed.
Fox cockpit before/after captures750 show the missing eye opening repaired.
New menu before/after each298submitted frames in10.0116s; this is emulator
cadence, NOT physicalFPS, and animation/window workload counts vary slightly.
New restored initializedheap54716896/71303168B,linearfree9702656/25165824B,
VRAMfree4822016B,stack55208/524288B. GPU6566submitted/completed,0errors.
New capture used prototype binary9298 with decal draw counter0; restored eyes
used only the existing material path. Final ordinary executables reproduce
previous binaries byte-for-byte: Newfb1fc8a560ab/Oldcabe83636cb7. No runtime
shader/CPU/PICA/memory policy change adopted. Final replayNewf02fcc684f94,
Old7ffb681509e6 have refreshed source tags; prior packages remain archived.
Old final menu replay completes; capture720 inspected and Fox eye present.
Krystal final replay1261retraces/889render frames passes load/movement gates;
GPU9007submitted/completed,0errors. Capture1230 inspected: face small/dark,
so detailed eye/blink/lighting acceptance remains OPEN. Heap55287784/71303168B,
linearfree9090816/25165824B,VRAMfree4813824B,stack54392/524288B. Measured
5s sky-facing scene51.82emulator submissionFPS, NOT physical performance.
First Krystal recipe correctly failed a wrong spawn gate; saved semantic record
confirms12379.543,211,89.164 and the corrected test passes without save edits.
All owned builds/tests/emulators are terminal; no deployment this turn.
Installed New73e4838ca819/Old9793, SDassets and normal saves unchanged.
Evidence: artifacts/research/lod-features/decal/{integration,final-assets,
admission-regression}.json. No physicalFPS improvement established.


8 October world repair follow-up: legacy world reductions were audited across all
banks. 2931/2945 changed lists fail sampled shape/UV/baked-color/winding bounds.
Eight-bank candidate repairs575lists (542original,33validated reductions), with
all model patches preserved. Scene rebuild now preserves v8live position refs.
Topology1fa8270e61c3 / scene6d8c85f07f0d / cache6aab55239a9d remain LOCAL.
Palace-facing matched actual-console-binary runs show restored wall detail,
+11.58%submitted indices and +0.597ms sampled emulated submission cost; this is
not physical performance acceptance. Room/interior/shop/Ice Mountain visual
acceptance,2356other failing lists and all model/material issues remain open.
Do not deploy this candidate as a performance-safe fix without further evidence.
Details and memory/SD limits: artifacts/research/world-repair/integration.json.

8 October isolated eye candidate: scene73cb9f7530cf/cachee3e6d9d20ea7 built
on the DEPLOYED topologydac4acbd6d15. Only Krystal1 andFox465 packets change,
+52triangles each/+6784B total; all2100otherpackets/1267worldpackets unchanged.
All bank inventories pass. Samebinary60cee cockpit pair on installed topology
shows missingeye before andvisibleeye after (capture750; animations differ).
Both298submittedframes/10.0116s (~29.765emulatorFPS); no measured cadence loss
established in this short pair. Mean draw counts87.35->88.23, indices23167->23345;
counts also include animation variation, not exact fixed-frame cost. Heappeak
+11584B, linearfree -26624B, VRAM unchanged. GPUwork balances, no scene runtimeSD.
This package is LOCAL pending completion of hardwareVRAMcomparison. Installation
preconditions/identity payload in decal/installed-eyes/install-candidate.json.
Krystal detailed eye/blink states and console acceptance remain OPEN.

8 October isolated WarpStone candidate: installed-warpstone restores only model220 on deployed topology plus isolated eyes. Static exact-source arrays and inspected before/after shape prove the head/body distortion repair at bind pose. +1230triangles, +56216GPU bytes; all53bank coverage passes. Same-binary stationary headless runs complete but camera faces away; emulator15.600->15.057FPS is no performance acceptance. Full animated face and console cost remain open. Candidate is local only.

8 October animated bounds: actual-capture proof of offscreen WarpStone submissions led to an ownership-tested conservative candidate. Same-binary emulator OFF16.300/ON14.908FPS despite4.44% fewer indices; defaultOFF, no physical speedup/deployment claim. Optional normal boundspeak67032B, no added linear/VRAM pool. See lod-features/animated-bounds/integration.json. Full animated WarpStone view still open; captures prove it was behind the camera, so future visual testing must correct the viewing direction.

8 October physical VRAM comparison: palace baseline28.225FPS; experimental run corrupted/froze before its measured window. No VRAM FPS gain established. Captured L4/VRAM/TEXUNIT1 hazard guarded; replaya78ee deployed after headlesspalace and production-policy regression tests. Console safety/performance remains pending. All visual and finalprogression gates above remain open.

8 October: physical mask-guard replay completed in MAIN/VRAMON28.9188FPS and
SHARED/ON29.0210FPS, without the prior freeze in these short runs. Broad stability
and steady30/60 remain open. New7fc replay adds complete wall-time accounting;
physical results pending. Global world repair2931exact original lists is compiled
and statically verified, but added geometry cost and runtime memory remain
unaccepted. No repaired assets deployed. Full visual acceptance remains open.
