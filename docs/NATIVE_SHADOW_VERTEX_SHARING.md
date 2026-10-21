# Exact native shadow vertex sharing

13 October local implementation. The full rendering/performance goal remains
incomplete; this is not a claim of physical 60 FPS or complete shadow parity.

`pica_shadow_plan.c` now shares byte-identical 24-byte shadow vertices during
asset admission. Normal, colour and UV seams in the ordinary mesh need not
duplicate position-and-skin shader inputs in the shadow mesh. Both skin slots
are first rebased into the batch palette; position, both weights, bind-tail
bytes and all remaining shadow attributes must match exactly. Triangles,
their winding and their order are unchanged. This avoids the alpha/equal-depth
ordering risk of general vertex-cache triangle sorting.

The shader, per-frame submission, simulation, asset bytes and GPU lifetime are
unchanged. Active morphs and source modifications retain the existing owner's
selection/invalidation rules. No CPU vertex processing is added to a frame.
Sharing applies only to the compact native shadow plan; ordinary actors, world
geometry, fur and projected receiver geometry are not optimized by this change.

The compiler uses an ordinal hash table and source-to-output map, reconstructing
collision keys from immutable source vertices rather than keeping another full
vertex stream. Temporary normal-memory usage is queried before compilation and
checked against the existing reserve, including the subsequent resident CPU
allocation. Worst-case scratch is 327,680 bytes plus at most 256 draw records
(20,480 bytes). The checked collection peaks at 157,634 bytes. This is temporary
load-time memory, not per-frame or resident memory. Allocation failure declines
the unpublished optional plan and retains ordinary native shadow geometry.
The existing 512 KiB linear/64 KiB CPU plan limits, one-time cache flush and
parent GPU retirement fence remain in force. Pinned Melee source publication
and scene-lifetime rules were reviewed; no SDK/OS/register change was needed.

The actual compiler was checked across all 835 prepared actor models:

| Work/storage | Before sharing | After sharing |
|---|---:|---:|
| Shadow vertices | 319,752 | 207,614 |
| Ordered triangles | 328,012 | 328,012 |
| Compact draw groups | 1,040 | 1,040 |
| Whole-collection GPU storage | 9,651,104 B | 6,959,312 B |

These are collection totals, not a simultaneously resident allocation or a
scene-frequency-weighted workload. Every expanded corner matches the original
position, global bone IDs, weights and offsets exactly. The 2,505 malformed and
capacity cases retain output atomicity. ASan/UBSan checks cover maximum-size
32,768-vertex batches, reversed palette slots, coincident positions with
different weights/tails, no-merge batches, allocation failure and invalid source
data. A position-only-sharing mutant fails the same oracle. The actual owner
passes 100 delayed-reader load/retire cycles and rejects a missing-fence mutant.
The ARM compiler passes with warnings treated as errors.

FIFO cache models within the **same merged draw boundaries** predict 2.5%,
5.0%, 7.6%, 10.8% and 15.1% fewer vertex invocations for 4/8/16/32/64 slots.
No physical cache policy or elapsed GPU improvement is established. The
29–33% estimate in `NATIVE_VERTEX_CACHE_AUDIT.md` describes a different,
unintegrated triangle-order experiment and must not be attributed to sharing.

The shared replay build is in
`artifacts/research/pica-scene-rewrite/shadow-share-v1-builds`; only the compiler,
its header and the owning preparation header differ from the stable actor-live
baseline. The game archive and every embedded shader are identical. Shared
Old 80 MiB/New 124 MiB CIA profiles are retained. Workspace restoration passed.
No console deployment has occurred. Runtime comparisons are recorded separately
under `shadow-share-test/runtime.json` when complete.

Evidence: `shadow-share-test/{compiler,share,share-position-only,lifetime,
lifetime-mutant,arm}.json`. The first runtime attempt stopped at disk preflight
before launching Azahar. Verified duplicate prepared payloads were retired from
two completed setup tests; their original evidence and canonical files remain.

## Cave integration

Serial candidate New, fresh baseline New and candidate Old runs all completed
the replay, state/save assertions, native receiver coverage and owned-process
shutdown. Captures at 1200/1410 were inspected on New and 1410 on Old: cave,
Fox, HUD and ground silhouettes remain present. Screenshot pixels are not
identical (different observed animation/fog phases); no exact pixel-equivalence
claim is based on these captures. Expanded shader-input equivalence is covered
by the independent ordered-corner check above.

| Azahar guest profile | Baseline FPS | Candidate FPS | Baseline submit | Candidate submit |
|---|---:|---:|---:|---:|
| New, fresh matched pair | 59.832152 | 59.832153 | 5.368916 ms | 5.369108 ms |
| Old, retained prior baseline | 57.432254 | 57.415478 | See runtime.json | Baseline +0.000046 ms |

These are paced guest frame/tick measurements, not host-real-time or physical
console performance. Host New durations were 64.18/64.40 seconds. Old candidate
took 68.53 seconds against a historical 53.57-second run; that is not a
contemporaneous host comparison, and the host-duration increase is not explained
by the near-identical guest component counters. No host speedup is established.

Both profiles have 144,384 more free linear bytes at their minimum. Optional
shadow-plan peak falls from 469,824 to 324,688 bytes; resident-at-exit storage
falls from 321,504 to 253,904 bytes. All 30 plan builds succeed with zero declines.
New whole-run mask updates/draws/indices remain 715/2860/7,353,060, and receiver
draws/indices remain 5005/9,493,770. Old corresponding counts also match its
prior baseline. The reduction did not omit casters, triangles or receivers.

Candidate New heap peak is 48,510,056/71,303,168 B; Old is
40,973,416/50,331,648 B. Minimum free linear is 18,583,552/25,165,824 B and
3,903,488/10,485,760 B respectively. Both report 4,723,712 B free VRAM and
73,008 B measured main-stack usage of 524,288 B reserved. Process commitments
are 103,325,696/130,023,424 B and 67,674,112/83,886,080 B. Sampled heap peaks
must not be treated as a measurement of every transient compiler allocation;
the explicit scratch bounds and reserve checks cover that additional demand.

The measurable result is reduced resident geometry storage with unchanged
submission cost. Hardware throughput improvement, shadow fidelity and the
overall frame-rate targets remain unproven.
