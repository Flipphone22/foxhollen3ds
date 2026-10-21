12 October COMBINED SHADOW SPATIAL/CAMERA progress; goalACTIVE/INCOMPLETE,
no blocker. PreviousgoalturnPROGRESS(spatialcache+combinedsky), thisturnPROGRESS
(composed renderer, actualowner/capacity checks, ARMbuild, New/Old/moving runs).
USER HARDWARE UNAVAILABLE: noFTP/probe requests. Sky physicalrepair unconfirmed.

New purepatch pica_shadow_spatial_visibility_patch composes cached Morton
receiver ranges with conservative perframe camera mask. No removed triangles,
new shaders/GPUstorage/worker. Existing readjoin before geometry mutation,
range-cacheclear and camera fail-open beforeGPUpublication retained. Consulted
pinnedMelee native-3ds/port/engine/geometry_bounds.h andport/3ds/render_worker.c.
Actualowner1000cycles/5000disabledpoisoncallsPASS; missinginvalidationmutantFAIL.
Rangecompiler/cache byteidenticalV2. New ASanUBSan6cases16383/16384/16385triangles
compact+float:exacttriplemultiset,unchangedovercaporder,mutation/lifetimePASS.

Isolated shared replay36664722d599/CIAaf382ab0fff3 inshadow-spatial-v3-builds,
NOTDEPLOYED/no normalbuild. Fivechangedfiles/446sources; gamearchive460df0884c99
andall11linkedshadersUNCHANGED,Old80/New124verified. AllWIP/archive restoredfinally.
NeverblindbuildglobalWIP. tools/build/check_shadow_spatial_v3.py,
test_shadow_spatial_visibility.py,test_shadow_spatial_capacity.py,
analyze_shadow_spatial_visibility.py,verify_shadow_spatial_v3.py.

New cavePASS61.59hostsec/state/saves/drain/ownedexit,1410imageinspected.
Same715casters/4290authoredmasks asbase. Draws46475->34320(-26.15%),
indices48461985->23400129(-51.71%). CPUscene+submitADDS .258036msvsbase,
versusV2+.338571ms. Notphysicalnetwin. Cache20317/34320hits,13invalidations.
11receiverplans/0declines. MemoryNewheap48427232/71303168,linearfree18441856/
25165824,VRAMfree4723712,stack73008,process103399424/130023424.

Old cavePASS59.81hostsec/state/saves/drain/ownedexit,1410imageinspected.
11plans/0declines;658casters/3948masks/31584draws/21534465indices.
Scene1.873229/submit6.067848ms/55.233112AzaharFPS; NOpairedOldbaseline orphysical
forecast. Heap40890592/50331648,linearfree3761792/10485760,VRAM4723712,
stack73008,process67747840/83886080. ExistingOldtouchstickretained.

OriginalgeometryThorntail movingroutePASS76.94hostsec/state/saves/drain/ownedexit.
1500/1650imagesinspectedFox/trees/Arwing/world/HUD/shadows.25plans/0declines.
Sameinputs/assets/optionsasarchivedworldvisibilityroute; differentcasters2158vs2143,
soNOTfixed-stateworkload. Draws24747->21748,indices8144163->4706769;
scene+submit+.105927ms;52.9963->53.7588AzaharFPS NOTprovenspeedup.
Movingcache2695/23777hits(11.3%) limitsstaticcaveextrapolation.
Heap56489088/71303168,linearfree16185984/25165824,VRAM4723712,stack73068,
process103399424/130023424. No regression accepted solelyfromnonblankimage.

Evidence shadow-spatial-visibility-test/{result,missing-mutation-invalidation,
build-verification,runtime,old-runtime,moving-runtime}.json andcapacity-test.
docs/NATIVE_SHADOW_SPATIAL_GROUPING.md updated. FullshadowCPUbudget/softness/
sourcecollisionpolicy andphysicalacceptance remainunmet. Candidate excludedfrom
combinedskyrelease; doesnotfixallremainingvisualsorprovephysical60FPS.
Potentialnext cost: queryhash/key rebuilt perreceiver though planes sharedper
worldinstance/caster; couldprepare once, preserveexactkeysandmutationclear.
Do not stacknewheavyfeaturesbeforebudgeting; continueotherparityworklocally.

AllownedprocessesTERMINAL15463build/34972compression/96968New/98195Old/90957moving.
16completedELFs losslesscompressed359182236B reclaimed, exactSHAproofrecord
completed-run-elf-compression-20261012-113247.json. No filesdeleted/assets/saves
changed/remoteactions. Normalbothstill eb521d9e988c/CIAb9b63688dbd6.
Combinedskynormalb67fde84a7d4/replaye00e8a2c49a6 remainlocalready,physically
unverified; don'tpollFTP orrepeatprobeaskswhileuserunavailable.
