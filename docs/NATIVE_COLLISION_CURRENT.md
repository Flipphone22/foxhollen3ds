# Source collision shadows on the current renderer

Local integration, not deployed or physically accepted. The source game selects
marked collision surfaces and filters receiver faces by light direction. The
current visible-world shadow projection has a different receiver policy; trimming
its draw workload cannot alone close that fidelity gap.

This candidate combines the existing native point-receiver implementation
(`shadow-points-v7-builds`) with current post-light materials, world culling and
the direct cloud shader (`cloud-visibility-v1-builds`). It consumes prepared
collision geometry, source surface routes and live finite queries. The PICA
point/geometry program performs face selection and emits receiver triangles;
no old GX draw interpreter or per-frame CPU triangle construction is connected.
The prior source, shader and lifecycle evidence remains scoped as recorded in
`NATIVE_SHADOW_TRACK_RECEIVERS.md`; this integration is not full shadow parity.

Source integration preserves 456 tracked files. The one merge conflict was in
shader assembly: the build now retains both the exact 133-instruction collision
program and the 183-instruction cloud program. The original conflict is archived.
Only `shadow_dolphin.o` and `tex_dolphin.o` change in the game archive, for their
native rendering adapters. All other game members remain byte-identical. All
previously linked native shaders retain their bytes; the point program exactly
matches the earlier source-tested candidate. Shared CIA profiles remain
Old 80 MiB/New 124 MiB.

The first link correctly rejected an unresolved world-bounds accessor. The
collision owner had removed metadata that current ordinary world culling uses.
V2 retains those CPU bounds inside the existing bounded world-plan allocation,
including position-mutation refresh. It does not restore the old shadow GPU
vertex/index batches. Both collision points and camera-culling bounds refresh
under the existing reader fence before GPU publication. Source composition checks
verify that ordering and the unchanged bounds accessor. Failure evidence is in
`collision-current-v1-builds`; successful V2 is `collision-current-v2-builds`.

Shared replay 3dsx `56f6a64b82d7`, CIA `d47504aecf89`. Full build/source/archive
proof is `collision-current-test/build-verification.json`. Runtime coverage,
image acceptance, CPU/GPU cost and memory must be recorded separately. Dynamic
object receivers, complete source limits, softness and physical shader behavior
remain explicit open work. No hardware probe is requested while unavailable.

Serial New ON, Old ON and same-binary New OFF cave runs pass replay assertions,
save preservation, log drain and owned exit. Both ON 1410 captures were inspected:
world, player, HUD and ground shadow remain; differing fog/pose phases are not an
image-equality proof. All eleven receiver plans load with no declines, no missing
draw plans, and zero admission-stage rejections. New ON records 715 geometric
casters, 4,290 authored masks, 5,005 receiver draws and 9,493,770 input point
records. A point can emit a triangle; this count is not directly comparable to
the former triangle-index counter. GPU hardware throughput remains unmeasured.

The combined current candidate's New scene/submission times are 1.949274 and
5.365966 ms, 0.388654 ms below the archived visible-mesh baseline in the same
cave recipe. This includes the current shader/renderer integration, so it is
not attribution to one change. Compared with its own shadow-OFF run, the full
effect adds **0.729173 ms**, above the **0.5 ms CPU budget**. It therefore remains
experimental. No more expensive shadow features or default-on policy are accepted
on the strength of draw-count reduction alone. OFF performs zero source-transform
publications, receiver queries, captures or shadow draws; persistent resources
and mutation maintenance remain resident to support toggling.

New heap 48,510,032/71,303,168 bytes, linear free 18,439,168/25,165,824, VRAM free
4,723,712, measured stack 73,008, process 103,325,696/130,023,424. Old heap
40,973,392/50,331,648, linear free 3,759,104/10,485,760, same VRAM/stack, process
67,674,112/83,886,080. Receiver storage is 107,740 normal bytes and 357,248 linear
bytes; maximum observed decoder scratch is 51,424 bytes. Old has no same-binary
OFF cost comparison. Emulator cadence is 59.831 New / 57.226 Old, not physical FPS.
Evidence: `collision-current-test/{runtime,runtime-new,visibility-owner}.json`.


Moving original-geometry Thorntail validation also completes: 25 admitted plans,
zero missing plans/declines, images 1500/1650 inspected. Full evidence and timing
limits are in `collision-current-test/moving.json`. Broad-range statistics include
points later rejected by the texture-footprint test; they need not equal final
submitted point counts outside the fixed cave fixture.

Two further spatial prototypes preserve original query output across all 1,267
blocks and six moving poses. Splitting proven-empty gaps removes another 8.33%
of broad-range points but can add draws; refitting tighter live bounds removes
8.16% but requires mutation maintenance. Neither is integrated or a measured
performance improvement. See `CHECKPOINT_20261012_COLLISION_MOVING.md` and
`collision-current-test/spatial-experiments.json`. No new console probe requested.

17 October COLLISION ADMISSION COST GATE; goal ACTIVE/INCOMPLETE.
Previous goal turn PROGRESS(spawn descriptors/build/runtime/record); this turn
PROGRESS(exactsource attribution, two traversal implementations, correctness and
ARM cost gates). No appbuild/runtime/FTP/deployment or hardware request.
Latestlocal83074988c111 and uploadedOld5052 unchanged; save-load crashOPEN.

Existing current profile trackBuildBlockTriangles587904selfencounters, exact
DWARF MD5 83ad3bbfb06ac37c2e2cf96e8710acc0. Retained candidate list already
admits immutable X/Z topology masks, yet loop repeats masks. Geometry, material,
query bounds and output order remain live. Prior geometry-cache/spatial-index
experiments reviewed; NOT blindly reintroduced. PinnedMelee command-cache
publication consulted; no GPU/cache/fence/OS ownership changes.

V1 per-triangle optional admission PASScorrectness but ARM+3.2847%; REJECTED.
V2 selects cached/uncached traversal per group; removes repeated mask fetches
only in admitted path. No new allocation. Actualcurrentproducer fixture2000
mutablequeries/810603triangles/224capacity cases +3repeats PASS ASanUBSan.
Missingretirement, missinguncached X/Z-axis admissions mutants reject. Initial
V2 mutant touched sourceoracle too; corrected candidate-only and fullrerun.
Mask axes here mean X/Z low/high bytes; geometry/normal math unchanged.

Clang18 ARM11 same16trials/64calls:12645060->12146480selfinstructions(-3.9429%).
External callees excluded, synthetic fixture not linkedgame/cycles/FPS. V1 and
V2 artifacts retained. V2producercode3528->4948B(+1420B); no I-cache cost proof.
NOTPROMOTED; no fullboot or hardwareask justified by this alone. Production
renderer/game unchanged. Do not extrapolate function ratio into frameFPS.
Sourceattribution40023,test88157/25446(failedmutant)/rerun,ARM42148/11117 all
TERMINAL. Save/privateasset/preferences unchanged. Fullgoal stillINCOMPLETE.
Evidence collision-candidate-admission-{v1-test,test}, tools matching patch and
host/ARM tests. Next larger native submission/update cost, or batch this only
if actual compiled/full-workload evidence justifies code footprint.

