# Direct native particle stream

This experiment replaces the ordinary particle producer's stack instance and
per-particle native submission call with a bounded write lease into the existing
GPU-visible ring. It builds on `unified-world-packet-v1`, not the rejected actor
row cache or earlier unpromoted particle-batch branch.

The producer obtains at most 32 unused records, writes the existing 36-byte
native parameters directly, and commits before changing material, looking up
another texture, emitting a dynamic trail, ending the pool, or retiring assets.
The owner conservatively culls the batch and compacts survivors in place. A
survivor already in its final position needs no second copy. PICA still performs
angle conversion, rotation, quad construction, camera/projection and near fade.
Source simulation, RNG, particle ordering and dynamic corners are unchanged.

The stream borrows only unused space in the current append-only segment. The
same owner acquires segments and waits for prior GPU readers before reuse.
Publication, flush, group termination, pool termination and shutdown reject
an outstanding lease. Commit advances the used range and dirty range together.
The existing cache-flush path publishes those ranges before command execution.
Pinned Melee `command_cache.c` was consulted for explicit streaming-buffer and
command-segment publication; this change retains the existing port's flush and
retirement functions rather than introducing another synchronization system.

There are no new GPU programs, vertex formats, effects, passes or buffer
allocations. The owner adds six unsigned counters/state words (24 bytes); the
producer keeps one 36-byte discard/dynamic record and small lease bookkeeping,
not a second particle array. Partial tail capacity is used before ring rollover.
Invisible render targets return a discard lease while the original producer
still consumes the same RNG. Disabled graphics retain the existing early
visual-work gate.

CPU work that remains: source appearance/RNG evaluation, packing changing
instance parameters, conservative four-plane visibility testing, occasional
in-place compaction, material boundaries and explicit resource publication.
Visibility currently rejects roughly 20% of supplied palace particles; removing
it would increase GPU work and is not assumed to improve total frame time.

`tools/test_particle_stream.py` executes the actual old/new producer bodies in
30,000 randomized source cases with ASan/UBSan, comparing exact RNG, source
slots, packets, ordering, material/depth metadata, dynamic corners and cleanup.
The actual old/new CPU ring algorithms emit the same 145,256 ordered visible
instances under ring rollover, target rejection, effect disable and culling.
GPU flush/retirement are stubs in this host comparison, not hardware proof.
Missing dynamic/final commits and incorrect compaction are rejected mutants.

The full generated candidate source is checked during the isolated ARM build
against the tested source hash. All unchanged linked GPU programs and shared
Old 80 MiB/New 124 MiB profiles are verified by the builder. Initial v1 failed
strict ARM compilation on misleading indentation after adding the pool-end
guard; v2 corrects formatting without weakening the warning or guard. Root and
game archive restoration are recorded for both attempts.

Tools: `particle_stream_patch.py`, `build_particle_stream.py`,
`run_particle_stream.py`, `compare_particle_stream.py` under `tools/`.
Evidence: `artifacts/research/pica-scene-rewrite/particle-stream-test/`.
Performance and release acceptance require actual gameplay results; host checks
alone do not establish a speedup, hardware cache correctness or full parity.

## Actual shared application, v2

Local source `3246aefd08f4` builds both shared containers. Only `expgfx.o`
changes in the game archive; the complete tested generated source hash matches
the build. All 18 GPU programs are unchanged. The earlier actor-cache experiment
is excluded. This build has not been deployed or packaged for installation.

Old palace profiling and New palace draw-layout validation both complete with
unchanged isolated saves/settings, successful state checks and owned-process
cleanup. Captures at 1600/1601 were inspected: world textures, Krystal, purple
creatures, particles and top-screen HUD remain. The New draw audit completes
110,943 draws with zero layout faults. These are bounded correctness checks,
not physical cache/coherence or complete visual acceptance.

The Old run writes 220,530 visible instances directly and compacts 14,177:
93.96% avoid the second copy. There are 19,506 leases, 285,002 supplied instances,
50,295 culled and 234,707 emitted, with no segment rollover or overwrite drain
in this route. Host fixtures separately exercise rollover. The linked ARM
`drawGlow` disassembly confirms direct field stores into the leased pointer.

In the matched eight-frame window, the producer plus ordinary particle
submission functions use 1,577,329 -> 1,453,587 ARM instruction encounters
(-7.85%). Ordinary per-particle submission calls go from 4,139 to zero; the
candidate performs 286 leases/commits. `drawGlow` alone becomes more expensive,
so quoting only the removed submission function would exaggerate the saving.
Pool counts differ 288 -> 290; particle phases are not identical.

The **complete window** goes from 23,408,397 to 24,018,297 encounters (+2.61%).
Audio source segment/curve calls differ substantially (1,406 -> 1,779), so the
whole change cannot be attributed to the stream implementation. No whole-frame
time or FPS improvement is established. Diagnostic DSP LLE/capture runs are
excluded from wall-time comparisons. Keep the local implementation/evidence;
do not promote it as a demonstrated full-frame performance improvement.

Observed memory stays at the parent values. Old: heap 41,734,480/50,331,648,
linear free 1,673,216/10,485,760, process 69,611,520/83,886,080. New: heap
56,611,688/71,303,168, linear free 16,353,280/25,165,824, process
104,751,104/130,023,424. Both: VRAM free 4,723,712 and stack high-water
72,984/524,288 bytes. These are sampled peaks, not final endurance guarantees.

`particle-stream-test/{integration,profile-comparison,component}.json` binds
the results to executable hashes. The last physical Old result remains
13.52 FPS/73.99 ms. Remaining rendering parity, progression/endurance and
performance goals remain incomplete.
