12 October SHADOW PREFLIGHT LOCAL progress; goalACTIVE/INCOMPLETE.
Hardware unavailable; no FTP, new consoleprobe, hardware question or deployment.
Both installednormal eb521d9e988c/CIA b9b63688dbd6 and saves unchanged.
PreviousgoalturnPROGRESS(rigidvisibility);thisturnPROGRESS(receiverpreflight
implemented, actual selection/sanitizer checks, sharedbuild, boundedmovingrun).

New experimental replay0a3e70fd1617/CIA2b8fcf1a3341 in shadow-preflight-v1-builds.
Based on collision-current-v2, NOT the unpromoted rigid candidate. One source
changed: platform_pica_shadow_project.c. Prepares finite receiver draws before
geometric capture; empty receivers skip target/caster work. Authored masks skip
empty auxiliary submissions. Same source order, queries, points, matrices and
all13 GPUprograms; gamearchive03989fabb2f7 unchanged; sharedOld80/New124verified.
CPU-only128-entry bounded synchronous command scratch10752B; no newGPUbuffers,
worker, shader, texture or per-frame geometry. Existing fence/target sequence
unchanged for nonempty draws. PinnedMelee/SDK ownership consulted. Builder
restores absent/replaced/shelvedWIP andarchivefinally; neverblindbuildglobalWIP.

12000 actualold/new selection comparisons PASSASanUBSan:8349empty,capacity128
covered, bothroutes/masks/blockorigins/querybounds; missingmatrixmutantFAIL.
One New originalgeometry movingThorntail run PASS73.057hostsec/state/saves/
logdrain/ownedexit;1500/1650imagesinspectedworld/Fox/Arwing/HUD/groundsilhouette.
25collisionplans,zero declines/missingplans. Skipped378empty masks butZERO
geometric captures;2128captures/7023receiverdraws/1369857inputpoints.
Scene+submit CPU -.004807ms vsbaseline, effectivelyunchanged; differingpose/
workloadcounts. Azahar52.413vs53.332FPS, no physicalprediction orspeedupclaim.
NOTPROMOTED; doNOTrequesthardware/anotherOldboot for this candidate.
Newheap56566256/71303168,linearfree16041472/25165824,VRAM4723712,
stack73068/524288;process103337984/130023424 (+12288committed versusbaseline).
Do notdescribe varyingheap/linear peaks as allocation savings. No Oldruntime.
ShadowCPUgate remainsfailed(.729ms vs.5); receiverlimits/dynamicobjects/softness
and fullrenderer/physical acceptance remainOPEN. This does notfixthosefeatures.

Evidence tools/pica_shadow_preflight_patch.py,build/check/verify/analyze
shadow_preflight_v1 scripts,test_shadow_preflight.py; docs/NATIVE_SHADOW_PREFLIGHT.md;
shadow-preflight-test/{selection,build-verification,runtime}.json. Windowsverify
initiallymissingelftools; Linuxverifiedsuccessfully. AllownedhandlesTERMINAL
81759test,74804build,85112replay; no concurrentheavyhostchecks/emulator timing.
Currentrecommended collisionbaseline remainscollision-current-v2. Avoidmore
smallshadowquery tweaks withoutlarger evidence. Goalcontinues,no blocker.
