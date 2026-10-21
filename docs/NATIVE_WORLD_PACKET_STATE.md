Native world packet ownership, 17 October 2026

The safe inline transport introduced after the hardware GPU-jump failure copies
compiled templates into the SDK command buffer. Only the destination is consumed
by PICA. The old template cache flushes survived that transition unnecessarily.
The cpu-packets candidate removes those two flush sites; the GX submission wrapper
still publishes the destination and all required index/particle resources.
Pinned Melee command_cache.c was consulted for that ownership distinction.

The following single-state candidate removes another remnant: a full 96-byte live
state copy beside each template. Scalar values already reside in their exact
command positions. Updates now compare against those positions and write only
changed words. Three immutable raster/sampler pointer keys remain. This removes
84 bytes per prepared draw; it does not add an allocation, GPU pass, shader,
worker, command, source-data interpretation or per-frame geometry preparation.
Resource retirement and whole-group visibility/admission remain unchanged.

CPU work retained: validate the live packet contract before mutation, compare
live parameters, update changed command words, and copy the compiled payload
into the SDK destination. That last copy is retained because the alternative
GPU-jump transport failed on hardware. It is not permissible to restore it on
the strength of emulator-only success.

The initial single-state implementation passed correctness but increased ARM
instructions in the update fixture by 1.90%. Its artifacts are preserved under
world-packet-single-state-test. Revision 2 splits the fixed uniform-header gap
from the contiguous data loop. The same 64-case ARM fixture drops from 294456 to
265464 instructions (-9.85%); every case improves. Both implementations produce
the same per-draw register states across 1024 draws, 5312 dirty-word writes and
15 unchanged updates. Five stale-state/address/preflight mutants are rejected.
The actual allocation/admission/epoch/release owner passes 157 checks.

These are component results. In the fresh eight-frame Old palace profile,
packet_update was only 0.71% of instruction encounters. Its improvement cannot
be presented as a substantial whole-frame speedup. The main value of compacting
this state is removing redundant ownership and recovering normal RAM near the
Old 3DS admission ceiling. No new physical FPS claim is established.

The scalar comparisons now read existing template storage in linear RAM rather
than its former normal-RAM mirror. The ARM fixture does not model hardware cache
or CPU/GPU memory contention; fewer instructions do not settle that tradeoff.
Keep this candidate unaccepted for physical performance until a substantive
hardware benchmark includes it. The shader and GPU command workload are unchanged.

Evidence and reproducible commands:

- tools/test_world_packet_single_state.py (default revision v2)
- tools/test_world_packet_single_owner.py
- tools/build_world_packet_single_state.py console-setup --revision v2
- artifacts/research/pica-scene-rewrite/world-packet-single-state-v2-test/result.json
- artifacts/research/pica-scene-rewrite/world-packet-single-owner-v2-test/result.json
- artifacts/native/unified-save-heap-v1-benchmark-old-dsp-profile-v2/

The standalone cpu-packets build passed the exact Old229 ThornTail save-load
regression with native DSP enabled, unchanged save/settings, clean shutdown and
zero vertex-layout faults. The loaded-later capture was inspected: Fox, world
textures, trees, NPC, HUD and touch controls are present. Its elapsed time is
excluded from performance claims (DSP LLE, capture and concurrent local tests).
The combined v2 single-state app FAILED its palace integration check at retrace
825 with `Native morph resident budget`. Its evidence remains in
artifacts/native/unified-world-state-v2-benchmark-old-dsp-profile-v2; it is not an
accepted build. Normal usage was 41365352/49283072 bytes with 524288 bytes reserved
for the main stack. The normal-reserve ceiling, rather than total free linear
memory, was approached.

The smaller metadata changes when allocation pressure is encountered. Material
and scene loading already reclaim the optional CPU texture-read cache before
required allocation; morph loading did not. Revision 3 adds a reusable pressure
entry point and calls it before the unchanged morph admission checks. The cache
is detached from the reader before being freed, is never GPU-visible, and is
not recreated during draws. No reserve, morph quality or storage-lifetime rule
is relaxed. The v3 game run failed again at retrace 833 with identical observed
normal usage and no reclaim log. The read-cache hypothesis was not supported;
v3 is rejected and its change is excluded from v4. Keep this failure visible.

The actual cache-owner host test passes 31818 ASan/UBSan boundary/teardown checks;
no-reclaim and weakened-reserve mutants fail. Evidence is in
morph-cache-pressure-test/result.json. Build with
tools/build_world_packet_pressure.py console-setup --revision v3, then run
tools/run_world_packet_single_state.py benchmark --family unified-world-state
--revision v3 --old --profile. These commands reproduce the rejected candidate.

Revision 4 places the compact packet metadata and its existing command template
in one bounded linear allocation. Required morph data no longer competes with
these optional packet records for normal RAM. Both portions count against the
128 KiB packet-linear cap and the existing 15% linear reserve; the 24 KiB metadata
cap also remains. Commands remain 16-byte aligned. The template payload and GPU
destination publication are unchanged. The actual ARM owner passes 137 checks,
including zero normal allocations, combined bounds/alignment, epoch rejection,
visibility rejection, limits and complete release. This is a placement change,
not a larger application memory grant or relaxed reserve. It also adds complete
morph budget diagnostics. V4 FAILED the same load and is rejected for packaging:
asset 11 needs 2492 normal bytes, with cpu=5740, gpu_request=127232,
gpu_used=303744, heap=41365760, stack=524288, grant=49283072, linear_free=5220608.
Only 563.2 normal bytes remain under the ceiling. Neither morph's component caps
nor available linear RAM caused this failure. The packet-only move was insufficient.

Build: tools/build_world_packet_storage.py console-setup --revision v4.
Run: tools/run_world_packet_single_state.py benchmark --family unified-world-state
--revision v4 --old --profile. Test: tools/test_world_packet_storage.py.

V5 additionally places the existing compiled world/actor command-plan arenas in
linear RAM. The native renderer retains every plan, command, effect and geometry;
it does not replace them with a slower renderer or reduce rendering. Admission
retains the 192 KiB per-plan and 512 KiB total material-metadata caps and checks
15% linear headroom before AND after allocation. The allocator zeros each plan
once, matching the former calloc contract, and release uses the matching heap.
No new worker or per-frame allocation is added. Existing observed plan peaks are
62896+8920 bytes in palace, 173396+12880 in the ThornTail fixture; linear placement
therefore also needs validation in ThornTail, whose free linear margin is smaller.
Material CPU-byte counters still describe CPU metadata, including these linear
allocations; actual normal/linear pool samples are the memory-budget authority.
Packet-group metadata has its separate 24 KiB cap and is included in the packet
linear extent; it no longer contributes to the material allocator's normal-heap
accounting. Do not add those counters as though all were normal-RAM allocations.

The actual helper passes 10650 sanitizer checks covering reserve boundaries,
component rejection, allocation failure, zero initialization, post-allocation
pressure rollback and release. This does not establish GPU or whole-frame timing.
Build v5 with tools/build_command_plan_storage.py console-setup --revision v5.
The rejected v2/v3/v4 archives contain RUNTIME_REJECTED.json. The normal setup
packager now refuses matching or stale rejection records before reading payloads.

17 October V5 execution and coverage correction

V5 a5b4ab2a8866 completes both the palace replay and exact Old229 ThornTail save
with unchanged saves/settings and clean teardown. The save capture was inspected
(Fox, world, trees, NPC, HUD), with 167936 audited draws and zero layout faults.
Normal peak is 40378352/49283072, linear minimum free 2496768/12582912, VRAM free
4723712, stack 73044/524288, process 70643712/83886080. Palace normal peak is
41361072 and linear minimum free 4808832. This is not an endurance guarantee.

Important: the palace execution pass did NOT establish rendering correctness.
Its log contains 661 attempted missing actor draws, versus 5653 in the baseline.
The candidate admits 12 extra scene assets; draw work consequently differs.
Source IDs 1,135,1256,1257 still reject despite exact entries in a prepared pack.
Source 801 lacks a matching pack entry, but a load rejection alone is not proof
that it is rendered. The ThornTail save run has no recorded coverage failures.
V5 is now marked RUNTIME_REJECTED for packaging. Original runtime summaries are
preserved; separate admission reports qualify their earlier execution-only pass.

The integration verifier originally compared the run's CXI hash against the
3dsx hash and failed correctly. It now verifies CXI against cia.json and the
actual CXI file, ELF against the linked CIA identity, and records all four
binary hashes separately. V5 3dsx is 8a63bbb98ef0; CXI e3a84819bf4d is NOT its
3dsx identity. No console was updated.

V6 adds bounded CPU scene-metadata placement when the normal pool cannot admit
it. The original normal allocation is retained where it fits. Linear placement
keeps the scene metadata cap, GPU byte cap, 15% linear reserve, and existing
target-specific reserve; preflight includes upcoming GPU storage and alignment,
and a second check can roll back after allocation. Per-slot ownership selects
the matching deallocator on load failure, ordinary release and shutdown. No GPU
references are added to this CPU storage; GPU vertex/index publication and
resource fences remain unchanged. No new per-frame processing or worker is added.
This follows pinned Melee linear_lock.c allocator serialization, using the
existing wrapped SDK allocators; texture storage and GPU ownership are unchanged.
256 bytes of per-slot ownership flags plus two counters are added statically.

The exact allocator passes 3354 ASan/UBSan checks; weakened reserve, absent
post-allocation check and incorrect owner-reset mutants are rejected. Evidence:
scene-metadata-storage-test/result.json. Build with
tools/build_scene_metadata_storage.py console-setup --revision v6. This placement
change addresses required asset admission, not a claimed performance improvement.

V6 0ef57b30aa59 failed the Old palace load at asset 1's morph plan: 3308 normal
bytes requested, heap 41363312 plus reserved stack 524288 of 49283072; linear
free 3838848. It is rejected for packaging. Admitting additional scene assets
exposed the remaining normal-only morph metadata owner. Do not weaken normal
reserve or omit that model to make the benchmark complete.

V7 combines each morph's small CPU plan and existing GPU lanes into one aligned
linear allocation. CPU metadata precedes the GPU lanes at a 128-byte boundary;
all GPU lane strides, initialization, shaders, position publication and fences
are unchanged. Release frees the owning base once after the existing storage
join. The 256 KiB CPU-plan and 1 MiB GPU-lane limits remain; total allocation
(including metadata alignment) is checked against actual linear capacity and
15% free space before and after allocation. There is no morph malloc on the
normal heap. Existing cpu_bytes describes CPU plan bytes despite their linear
placement; existing linear_bytes describes GPU lanes, not total pool usage.
Use sampled linearSpaceFree for the physical pool budget including alignment.

The exact new allocator passes 1717 ASan/UBSan checks; weakened reserve,
missing rollback and misaligned GPU base mutants are rejected. Evidence:
morph-metadata-storage-test/result.json. This is memory admission work, not a
new rendering approximation or performance claim. Build with
tools/build_morph_metadata_storage.py console-setup --revision v7.

V7 local integration results: source 5ab4bfcb3e60, 3dsx 56fe6aa96459,
CIA 1fd6012338bd, CXI 306b94acb1b9. All 18 GPU programs and the game archive
remain unchanged; shared Old80/New124 and root/archive restoration pass.
Old palace completes with 65 admitted assets, zero matching-source rejections
and zero site-36 missing draws (baseline 49 assets / 5653 missing attempts).
Only resource 801 has no matching inventory entry; no attempted missing draw
is recorded for it. The capture now visibly contains Krystal and the purple
actors. Scene metadata in linear RAM peaks at 324772 bytes. Palace normal peak
41364432/49283072 and linear free minimum 3591424/12582912; main stack 72984.

Exact Old ThornTail save completes with 167936 layout-audited draws, zero layout
faults and zero recorded missing actor draws. Normal peak 40366688, linear free
minimum 2484352, stack 73044. Both runs preserve saves/settings, drain logs and
exit owned processes; VRAM free minimum 4723712, process 70643712/83886080.
No conversion rerun, private asset change, hardware request or deployment.

The corrected eight-frame Old profile totals 21864989 ARM encounters, versus
20086230 in the incomplete baseline. This is NOT a speedup or physical FPS
prediction. Largest functions include memcpy 1447336, particle update 1405863,
drawGlow 973587, collision triangle preparation 587904, uniform update 519272,
DSP control 513179 and actor command append 477944. Focus subsequent work on
Old Fast whole-frame costs, not further tuning the <1% packet-update kernel.

Visual review also found a separate unresolved issue: both ThornTail save
captures lack the static bottom GRAPHICS button and Items pad, while dynamic
performance text remains. Palace bottom controls are visible. The cause is
unproven; no speculation about memory corruption or another unchanged replay.
V7 is blocked from deployment by RUNTIME_REJECTED.json pending that repair.
The integration-v7.json pass covers allocator/source identity, game rendering
coverage and memory/lifecycle assertions, not this bottom-screen visual check
or complete rendering parity. See the archived visual-review.json.
