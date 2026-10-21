# Retained PICA sampler descriptors

Local candidate, 12 October; not deployed or physically validated.

The native geometry and textures already stay resident, but Citro3D republishes
each dirty sampler's address, dimensions, wrapping, filtering, border and format.
The new SDK publication path retains an exact copy of each 2D descriptor and
omits those register writes when every descriptor byte is unchanged. It applies
to the native renderer's existing material owners without rebuilding geometry,
interpreting GX state or adding render passes.

The reference is pinned Melee's
`native-3ds/port/3ds/citro3d_fix.c:mp_native_tex_bind`, which copies descriptors
rather than trusting mutable object pointers. Foxhollow deliberately retains at
the common publication point instead of suppressing binds: all texture dirty
bits, unit-enable routing and texture-cache clears still execute. Thus movie
frames, morph-independent dynamic images and generated lighting maps can change
their pixels at an unchanged address. This change does not suppress CPU cache
publication or GPU retirement. No image data is cached by this mechanism.

The existing framebuffer, initialization and APT restore invalidations clear
the sampler cache too. Cube and other non-2D textures always publish; a cube's
face addresses can change behind its descriptor pointer. NULL units retain the
SDK's disable behavior. A source scan found no native code writing the retained
sampler registers outside the SDK. The texture-coordinate routing bit and
procedural texture configuration remain outside this cache.

Additional fixed storage is three ARM 24-byte descriptors, two 64-bit counters
and one 32-bit enable flag (92 bytes). Valid bits reuse the existing raster-cache
word. There are no allocations, workers, shader instructions or new fences.

`tools/test_retained_samplers.py` compiles the actual generated SDK context
publisher and original `C3Di_SetTex` for ARM11. It compares all emulated register
values at 1,800 updates with retention off and on, using descriptor mutations,
unit disables, cube-face mutations, framebuffer changes, APT restores and shader
transitions. Every dirty update must still clear the texture cache. The test
passes 294,248 checks; four mutants that omit invalidation, ignore changed
descriptors, cache mutable cube faces or omit cache clears are rejected.
The existing raster/geometry-boundary test also passes its original assertions
and rejects its three invalidation/boundary mutants.

The synthetic sequence omits 3,800 duplicate sampler publications: exactly
30,400 command words, reducing 234,938 to 204,538 words while retaining the same
1,918 observed cache/framebuffer side effects. This is not a frame-time or FPS
estimate. The first harness attempt had an incorrect relative include; the
second allowed random cube-type bits on a non-cube fixture. Both fixture errors
were corrected before acceptance. Geometry-test setup counters were then
excluded so the reported command-word delta is exactly attributable.

The replay build `bd2960b156dd` isolates three changes over tested V10:
`tools/retained_raster.py`, the SDK build provenance text and joined-shutdown
sampler telemetry. The game archive is identical. Runtime comparisons and
normal-build preparation follow separately. The sky defect remains independent
and unresolved; this optimization is not a sky repair.

The V1 New cave check completed in 47.96 host seconds, including state checks,
save preservation, shutdown drain and owned-process exit. It retained 87,253
of 381,120 sampler publications. However submission averaged 5.761 ms versus
V10's 5.677 ms; scene preparation was 1.857 versus 1.877 ms. Neither that result
nor the paced 59.831 FPS establishes an improvement. The 1410 capture contains
world geometry, Fox, textures and HUD, with more haze than V10's capture. Haze
differences were already reported in earlier identical-input runs; their cause
is still unresolved, and these captures are not pixel-equivalent acceptance.

V2 also changes construction of the remaining 2D sampler packets. It emits the
original eight command words directly into the checked current command buffer,
avoiding a temporary register array and two general command-builder calls.
Cube/other sampler cases retain their original SDK path. This is an adaptation
of Melee's checked command packets, already used for native draw/effect packets.
It does not remove or reorder registers, routing, cache clears or draw work.
Two three-word constant register tables add 24 bytes of read-only storage.

`test_packed_sampler_commands.py` compares 12,288 original/candidate packets
byte-for-byte under ASan/UBSan, including all units, texture types, descriptor
values and offsets. It exercises 4,608 direct packets, exact-capacity success,
insufficient-capacity rejection before any write, canaries and NULL-buffer
rejection. A wrong-dimension mutant fails as intended. Runtime V2 acceptance
is recorded separately; packet correctness alone is not a performance result.

V2 New and Old cave replays both pass the state/save/shutdown/owned-exit checks;
their 1410 images were inspected. V2 New submission averages 5.549 ms and scene
preparation 1.864 ms, versus V10 5.677 / 1.877 ms: a 0.140 ms combined reduction
in these short runs, with equal whole-run world/actor geometry totals. This is
a modest relative emulator observation, not a demonstrated console speedup.
It retains 87,411 of 381,672 descriptor publications. Old V2 submission is
5.567 ms, scene preparation 1.839 ms, retaining 83,497 of 364,799 publications;
there is no paired Old V10 result. Azahar cadence is New 59.831 / Old 57.116 FPS.

Both V2 runs retain the world, Fox, textures and HUD. The New capture again has
more haze than Old; source/phase versus rendering cause remains unresolved.
This is limited regression evidence, not full visual acceptance. New peak heap
48,404,624 / 71,303,168 B, linear minimum free 18,441,856 / 25,165,824 B; Old heap
40,867,984 / 50,331,648 B, linear minimum free 3,761,792 / 10,485,760 B. Both have
4,723,712 B free VRAM and measured main stack 73,008 / 524,288 B. Process commits
are New 103,301,120 / 130,023,424 B and Old 67,649,536 / 83,886,080 B. New process
commit is 4 KiB higher than V10; sampled heap/linear/VRAM usage is unchanged.

Reproducible results live under `sampler-publication-test/`; builders isolate
the stable V10 snapshot and restore unrelated VRAM/shadow WIP in `finally`.
Both requested console normal deployments remain pending the independent sky
repair. No installed files or saves were modified by these tests.

Matching local normal `83c2baee5c9d` / CIA `985147e78937` and replay
`dbfd7dde8c23` / CIA `d76182e3a95c` are built, not deployed. Their 439 source
manifest entries, game archive and active shader match; Old 80 / New 124 MiB
profiles remain intact. The SDK's recorded original/generated source hashes
and compiler commands also match. Its raw archive hashes differ: `ar r` retains
build timestamps in modified members and the symbol table. The verifier records
this explicitly rather than claiming byte-identical SDK archives.

The normal New check also completed 1,201 frames and actual touch toggles
(HUD/video hidden, then restored), 241 movie commits and 1,440 movie draws.
Both 540 and 1080 captures were inspected; the restored movie and title text are
visible. Select exit and log drain pass with no native test hooks. This run took
40.21 host seconds; it is not a matched movie-performance comparison. Heap peak
46,239,632 B, linear minimum free 18,446,976 B, free VRAM 4,723,712 B, measured
stack 72,956 B and process commit 103,264,256 B remain within the New profile.
Source fog, movies and other rendering omissions still have their previously
documented limits; this sampler change does not establish full visual parity.
