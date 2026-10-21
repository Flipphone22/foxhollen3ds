# Native actor state equality

17 October FIXED-WORD ACTOR STATE COMPARISON; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(NDSP stop-wake/build/exactsave); this turn PROGRESS(native
actor publisher change, bounded/ARM checks, isolated sharedbuild).
Local5d965a29409f unified-actor-state-compare-v1-console-setup-builds NOTDEPLOYED.
Parent04cbdc retainsNDSPstopfix and priorCPUchanges; uploadedOld5052 unchanged.
Physical88915 26.40484FPS/save-load crash stillOPEN, no newwholeframe gain.

Reviewed prior earlyparticlevisibility/GPUclip andworldbatch/posecache evidence;
no repeats/composition of rejected/unproven experiments. Actor publisher's
fixed3/4/12word state equality replacesgenericmemcmp forraster/environment/
UV/scale. Exactbitwise XOR/OR, no floatcomparison, pointerreuse, newcache or
changedvalidation. No GPUcommands/owners/shaders/draws/allocations changed.
Existing pinnedMelee fixedword uniform equality approach informs adaptation.

ASanUBSan6,110,000 exact-allocation equality/singlebit mismatchcasesPASS;
missinglastword mutants3/4/12 rejected. PairedactualSDK-ABI ARMpublisher
expandedfixture2880checksPASS fullcommands/statebytes/capacityreject. Raster,
environment,scale nowvary; repeatedunchangeddraw covered. GCCARM11 instructions
227632->216992(-4.67421%) synthetic measuredcalls inclcallees. Earliernarrow
2304checkfixture-6.9582% supersededbyexpandedcost, NOTwholeframe/cycles/FPS.
Reproduciblefixturegenerator verifiedagainstexecutedstate-stream.c andhashed.

Onlypica_actor_commands.c purepatch; exacttestSHA/sourcearchive verified.
Gamearchive433ed3e4ffb9/all18GPUprograms/setuprecipe unchanged,Old80/New124.
BSS3266864/data178996 unchanged, publisher4068->4332B, totaltext+264B.
No new peakmemory/stack/performance measurement. Root/archive restorationPASS.
No redundantfullboot: pairedoutputbyteproof and prior04cb save-load evidence
reusedwithscopeexplicit, notclaimednewgameplaytest. NoFTP/assets/saveschanged.
Owned90678initialARM/25712expandedARM/90774host/52034build allTERMINAL.
Evidence actor-state-compare-test/{result,words,fixture,build-verification,
acceptance}.json; tools actor_state_compare_patch/test/build/verify variants.
Next pursuelarger measuredCPU costs andbatchcurrentchanges forfullframe evidence;
doNOTmultiply these localpercentages into physicalFPS. FullgoalINCOMPLETE.

