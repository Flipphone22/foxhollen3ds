# Ordered native particle submission

16 October 2026. Local candidate; not deployed. DSP remains the default audio
backend. This change does not complete rendering parity or the frame-rate goal.

The native particle geometry shader already emits corners from compact instance
records. The remaining producer called the scalar submission API for each
particle, repeating frame, ring and material checks. The new producer collects
consecutive compatible records in a bounded 32-record stack array, then submits
them together. It flushes before a dynamic shape, material change or pool end;
it never sorts transparent particles. The direct-packing refinement writes into
that array without copying a temporary static record.

CPU work retained: original simulation, random calls, lifetime, motion, source
colour/alpha and scale calculation, conservative side-plane culling, and dynamic
trail corners. These supply live game state or avoid unnecessary GPU work. No
near/far culling, alpha clamp, simulation shortcut or shader change was added.

The consumer handles frame acquisition and material handoff once per visible
batch, retaining per-instance culling, append-only ring writes and capacity
rollover. Existing dirty-range publication, cache flushing, fences and resource
retirement are unchanged. Pinned Melee `command_cache.c` and `early_queue.c`
informed these ownership requirements; batching does not permit early reuse.

Resource change: a 1152-byte stack array, replacing per-record API traffic;
no additional static/heap buffers, linear memory, VRAM, GPU programs or worker.
The existing temporary dynamic record remains. Shared Old 80/New 124 MiB CIA
profiles and the converter recipe are unchanged.

Reproduce the isolated candidate:

```
python3 /work/tools/test_particle_batch_direct.py
python3 /work/tools/build_particle_batch_direct.py console-setup --revision v1
python3 /work/tools/verify_particle_batch_direct.py
python3 /work/tools/run_unified_startup.py benchmark --old --family unified-audio --revision particle-direct-v1 --profile
python3 /work/tools/analyze_arm_blocks.py /work/artifacts/native/unified-audio-particle-direct-v1-benchmark-old-dsp-profile-v2
python3 /work/tools/compare_particle_batch_direct.py
```

Do not build the unrelated root work in progress. The builder restores selected
root sources and the game archive. Only `expgfx.o` changes in the game archive;
the generated simulation outside `drawGlow` remains byte-identical. All 18 GPU
programs are unchanged.

Host checks execute 30,000 original/candidate producer comparisons, including
packet order, material parameters, dynamic corners, RNG, slots and cleanup.
Actual scalar/batch consumer checks emit identical 145,256 records with 6,317
flushes, 162 acquisitions and 108 ring wraps. ASan/UBSan pass; removing the
static-to-dynamic handoff fails. The test mocks fence owners; it does not prove
physical GPU coherence. Immutable owner code is checked separately.

The first batching candidate completed the Old palace run, but its sampled
window contained 112 lightning calls versus zero in the parent. Particle
draw/submission encounters fell 5.26%, while total encounters rose 4.90%.
That total is confounded by different scene work and establishes neither a
batching regression nor a full-frame speedup. The report now explicitly records
particle/lightning entry counts. Do not rerun until a favourable percentage
appears or convert instruction encounters into console FPS.

Detailed records live in `particle-batch-test/`, `particle-batch-direct-test/`
and the corresponding isolated `unified-audio-particle-*-builds` directories
under `artifacts/research/pica-scene-rewrite/`.

The direct-packing candidate, source `f84c482f009a`, also completed the bounded
Old palace run: status 1, 307 rows, unchanged saves/settings, drained shutdown
and owned emulator exit. The captured palace image was inspected: Krystal,
world textures, enemies and HUD remain visible. This is not full visual parity.
Audio source/callback faults and curve shortages were zero; no new PCM review.

Its sampled particle draw/submission group was 1,503,924 encounters versus
1,615,952 (-6.93%). Total was 23,841,407 versus 23,937,467 (-0.40%). DrawGlow
entries differed (312 versus 286), and lightning calls were 19 versus zero.
Neither percentage is a full-frame hardware speedup. The 155.40-second host
runtime includes DSP LLE and profiling and is excluded from timing comparisons.

Observed Old budgets matched the parent: heap 41,717,672/50,331,648 bytes;
linear minimum free 1,755,776/10,485,760; VRAM free 4,723,712; measured stack
upper bound 72,984/524,288; process 69,582,848/83,886,080. These are sampled
palace bounds, not area-wide headroom guarantees. No fresh New run, conversion,
moving-route/endurance or hardware validation was performed. Candidate remains
local and unpromoted pending stronger cost evidence; the DSP-default/collision
package `6eed681ce57c90a7` remains the prepared deployment candidate.
