12 October DIRECT ACTOR MATERIAL UPDATES LOCAL progress; goalACTIVE/INCOMPLETE.
PreviousgoalturnPROGRESS(shadowpreflight);thisturnPROGRESS(single native material
publication, source/lifecyclechecks, sharedbuilds, New/Oldruntime, index-orderaudit).
Hardware unavailable: no FTP, consoleprobe, question or deployment. Installed
normalboth eb521d9e988c/CIA b9b63688dbd6 and allassets/saves/settings unchanged.

Replayb4caad32747a/CIAff4672b4ac22 (actor-live-v2-builds), matchingnormal659f3ca49785/
CIA34b92e950a9c (actor-live-normal-v1-builds), BUILTNOTDEPLOYED. Based directly on
collision-current-v2, excludes unpromoted rigid andpreflight experiments.
New fh_pica_actor_live.h +pica_actor_live_patch.py resolvesone preparedmaterial
then publishes livevalues together, instead of six/seven repeated lookups and
setters. Same source textureanimation/scroll, alpha, channel swizzle, environment,
cloud/postcallback states and UI/movie/moon resets. Synchronousborrow only;
no cachedsourcepointer, newworker, allocations, GPUstate or perframegeometry.
Newcounter4B; temporarylivepackets only. PinnedMelee material_program.h consulted;
its CPUcolourevaluator NOTintroduced. Newcore2 stillaudio.

Actualold/new gameadapters andnativefunctions comparecompleteMaterialbytes under
ASanUBSan:20000actorupdates/100allocationlifecycles/512000publicationsPASS;
lookups4058944->640000. Stalecallback andswizzlemutants rejected. Firstharness
compile extern/staticcollisionfixed; firstmutantselector matchedold+new strings,
restrictedtoactualcandidate. V1builderfailedCRLF assertionBEFOREcompile; fixed
newlinehandling,V2built. Onlyobjprint_dolphin.o gamearchivechanges. Archive
0f1582d04548a49a4ad2c3b7b042ece1c5deda9e22bcf489f3f0ca09d7e6f64a.
All457sources+archive+13linkedGPUprograms SAME normal/replay; Old80/New124verified.
Buildersrestoresnapshot-onlyabsent/replaced/shelvedfiles andarchivefinally.
NeverblindbuildglobalWIP; stablecandidate snapshots alreadycontainactorlivepatch.

SerialNewmovingoriginalgeometryPASS74.9216hostsec,OldcaveONPASS53.5688s;
state/saves/drain/ownedexitPASS. New1500/1650 andOld1410imagesinspectedworld,
player/HUD/groundshadow; NewArwing andOldtouchstick present. Notpixel-equivalence.
Newobjectrender -.084300ms; scene+submit+objectrender -.104173ms. Oldobjectrender
-.105329ms; combined -.093627ms. Modestlocal CPU improvement, notphysicaltarget.
New53.589vs53.332AzaharFPS;Old57.432vs57.226; workloads/posephases differ.
New58641directmaterialupdates,25collisionplans/0declines/0missingplans.
Newheap56566256/71303168,linearfree16033280/25165824,VRAM4723712,stack73068;
process103321600/130023424. Oldheap40973392/50331648,linearfree3759104/10485760,
VRAM4723712,stack73008;process67670016/83886080. Stackreserve524288.
No newnormalboot: normalUIunchanged, priorUIevidencereused; no claimnewphysical.
Retain this directupdate aslocal improvement; do notrequesthardware for .1ms.
FullshadowCPUgate(.729ms vs.5), missingreceivers/softness/visualparity remainOPEN.

Larger nextGPUcandidate: tools/audit_actor_vertex_cache.py, offlineonly. All835
originalmodels/1609groups/988530indices; meshoptimizer general order preserves
EXACT orientedtriangle multiset and unchanged vertices/UV/bones. FIFO16/32/64
modelmisses497907/473742/442099 ->332681/324463/320153 (-33.18/-31.51/-27.58%).
FIFO4only-1.49%. NO hardwarecachepolicy proof, scene weighting, actualGPUtiming
orFPSgain. Azahar softwarecode64entryFIFO notsameasaccelerated/physicalrenderer.
DoNOTblindlyreorderall meshes: blending/equaldepth/sourceorder matters. Need
selective nativeindex path with originalorder fororder-sensitive materials,
boundedadditionalindexmemory andmorph/retirement handling, thenimages/timings.
No optimizedpackhasbeen written/deployed orselectedbygameplay. SourcepackSHA
c5ab2aaa1cfdbda2e375eac856b67afdf4d088da07bd957f1a1338415350b19a unchanged.
Alsoobservedgenericdirectionallight shader repeatsnormalization pervertex;
NOTimplemented; evaluateworkload before further specialization/programswitches.

DocsNATIVE_ACTOR_LIVE_STATE.md,NATIVE_VERTEX_CACHE_AUDIT.md; actor-live-test/
source/build-verification/normal-verification/runtime/old-runtime.json;
actor-vertex-cache-audit/report.json. Reproduciblebuilders/check/analyzers under
actor_live names. Compression16completedELFs reclaimed360269968B, hashesretained
completed-run-elf-compression-20261012-125637.json; none duringemulatortiming.
AllownedhandlesTERMINAL3897build64732verify64558compress50645New12774Old60119normal.
Goalcontinues; no blocker/completionclaim. No moreunchangedboots for this update.
