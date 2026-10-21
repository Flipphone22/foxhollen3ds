# Active-slot particle traversal

The native particle producer previously inspected each of the 25 slots in a
pool. The proposed iterator isolates the lowest live bit at or beyond the
current index and uses ARM11's CLZ instruction to select its slot. It ignores
the seven unused high mask bits. The mask is read again after each processed
slot rather than captured once per pool, preserving changes to later slots.

This changes traversal only. Appearance calculations, original source indices
used for rotation phase, RNG order, simulation, packed particle records,
material boundaries, stream leases and cleanup remain intact. Native GPU
programs and the resident geometry architecture do not change. The pinned
Melee command-cache publication implementation was consulted; its cache and
resource lifetime concerns are handled by the existing, unchanged GPU owners.

The actual generated parent TU has MD5
`e220bcc2b995d0ce5e6cc89e0253b919`. `test_particle_active_draw.py` extracts its
producer and compares the transformed producer using the existing stream
callback fixture. 30,000 comparisons pass under ASan/UBSan: emitted packets,
material metadata, slot contents, RNG state, live masks and cleanup match.
Masks include zero, the last legal bit, unused high bits, dense and sparse
sets. An adversarial RNG stub changes live masks during traversal.

Reverse-order, stale-mask and missing-upper-bound mutants are rejected.
The initial missing-bound mutant did not fail with sanitizer checks alone;
the fixture now explicitly checks every source-slot index before dereferencing
it, and the complete suite was rerun successfully. This is host equivalence
with GPU callbacks stubbed, not GPU raster or performance evidence.

The isolated builder starts from the command-copy parent, requires the exact
tested generated TU hash, builds only the changed game unit, and restores root
WIP and the prior game archive. The candidate must still be assessed for ARM
cost: dense pools can pay extra bit-search instructions. Neither a build nor
host equivalence is sufficient to establish a frame-time improvement.

Reproduction:

```
python3 /work/tools/test_particle_active_draw.py
python3 /work/tools/build_particle_active_draw.py console-setup --revision v1
python3 /work/tools/profile_particle_active_draw.py --system-core --memory-calls
```

This candidate is not deployed. Save-load crash validation remains separate;
the last uploaded normal build is `5052c41d6aa2`, and the retained physical
benchmark is `88915fc75528` at 26.40484 FPS over a short measured window.

## Recorded candidate result

17 October ACTIVE PARTICLE DRAW ITERATOR; goal ACTIVE/INCOMPLETE.
Previous turn PROGRESS(read-only console evidence establishes retained88915 logs,
new5052 CIA not yet evidenced running). This turn PROGRESS(actual source change,
host/ARM producer checks, isolated shared build and one profiled Old palace run).
Local5f03ba1ba7b2 unified-particle-active-draw-v1-console-setup-builds NOTDEPLOYED.
InstalledOld5052 remains; physical26.40484FPS/37.87184ms88915 and save crashOPEN.

Native producer skips inactive slots via guarded low-bit CLZ. Source pool is25
slots, not32; unused maskbits ignored. Re-read live mask each iteration preserves
changes to later slots. Packed data/phaseindices/RNG/simulation/material order/
stream ownership unchanged. PinnedMelee command_cache.c consulted; no GPU owner,
shader, allocation or worker changed. Only expgfx.o+archiveindex payload changed;
all18GPU programs and Old80/New124/recipe/restoration pass. Stableparent7d8e;
GPUclip7072 remains separate experiment, not included here.

Host actualproducer30000cases ASanUBSanPASS, including adversarial mid-loopmask
changes. Reverseorder/stalemask/unbounded mutants rejected. Initial unbounded
mutant escaped sanitizer-only detection; explicit source-index assertion added
to fixture, fullsuite rerun. Actual producer C compiled ARM11,384paired calls:
176355->134113 self-instruction encounters(-23.95%), identicaloutputs/RNG/state.
Syntheticmaskdistribution, calleesexcluded; NOT captured cost/cycles/FPS.

OneOldpalacerun126.02hostsec executionPASS/status1/saves/drain/ownedexit/missing0.
Rawfailed-audio-core-placement preserved; existing profiler SVC11stub, exactELF
execution proves core1 mixer/grant30. Capture1601 inspected Krystal/platform/
sky/HUD, particlesoff-camera; not fullrasterparity. Heap41364432/49283072,
linearfree3591424/12582912,VRAM4723712,mainstack72984/524288,
process70656000/83886080, unchanged sampled memory versusparent.

Actual8frame main16074418->16757793(+4.2513%); drawGlow1036752->1014041.
Candidate has lightning absentparent, camera/simticks/drawcounts differ; no
causal whole-frame gain/regression established. DoNOT subtract lightning/UI to
manufactureFPS. Sourceattribution exactMD5 786296297a00bf2f0200fe604fc51048.
No repeated boot or hardware probe requested. Larger producer/material costs
and combined GPUclip cost still need work; fullvisual/performance goalsOPEN.
Evidence particle-active-draw-test/{result,build-verification,arm-fixture,
arm-comparison,acceptance}.json; docs/NATIVE_PARTICLE_ACTIVE_DRAW.md.
All owned handlesTERMINAL15198(firsttestfailed),76214,41882build,44473runtime,
4170audit,43784source,1882ARM. No FTP or privateasset/save/preferences changes.

