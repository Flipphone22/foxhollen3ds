10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native shadow receiver query

Local implementation under validation. The installed e783636ad1e8 build still
uses the previous main-thread collision lookup. No separate-core shadow worker
has been implemented, and ordinary contact shadows remain opt-in.

The former contact shadow called `trackGetNearestGroundOffsetAndNormal` with
queryMask zero. This constructs the shared collision triangle buffer, constructs
edge normals, scans the triangles again, reconstructs edge equations, stores
ground hits and sorts them. Its static-world query adds substantial CPU work in
the cave. The query does not include dynamic-object descriptors with mask zero;
moving-object receivers were already absent and remain a visual gap.

The replacement streams the original map/group/cell/triangle spatial predicates
into a dedicated plane query. It uses private stack scratch, never changes the
game's collision buffers, and never constructs a full triangle or ground-hit
array. Gameplay collision functions remain unchanged. The source's 1,200
accepted-triangle and 35-hit limits are retained, as are upward-face selection,
0.2 edge tolerance, nearest absolute height, and descending-height tie breaking.

The bounded cache stores plane and edge calculations keyed by the
exact nine signed 16-bit coordinates used by the original collision builder.
There are no retained source pointers. A map replacement, vertex deformation,
collision height offset or coordinate-origin change cannot reuse a plane unless
its numerical geometry is identical. Live coordinates still have to be read:
collision data is mutable, and there is no established mutation-generation
contract that would permit blindly trusting a pointer. This is necessary CPU
receiver work, not per-frame rebuilding or uploading of the rendered quad.

The first 256-entry direct-mapped cache occupied 21,544 bytes but achieved only
32% hits in the real cave workload. Its scene preparation was 3.813610 ms versus
4.369852 ms for the previous receiver, still above the feature budget. The next
candidate mixes high hash bits and uses 512 entries in four-way sets; it occupies
45,224 bytes including counters and replacement cursors. It measured 77.3% cache
hits and 3.475290 ms scene preparation in the Old-profile cave replay. A third,
90,408-byte candidate reached 93.3% hits but only 3.470629 ms: doubling storage
did not resolve the remaining traversal cost. The working source therefore
returns to the smaller 512-entry cache, retaining query timing counters.
These are Azahar measurements, not console timings or a passed feature budget.
The third candidate's whole-run query mean was 234.019 us, maximum 442.543 us.
Query scratch is
bounded; GPU/linear/VRAM storage and draw count are unchanged. Counters report
queries, lookups, hits, valid triangles and accepted receivers on shutdown, with
`worker=0` to make the current ownership explicit. The GPU continues to transform
and blend the existing resident two-triangle shadow quad.

The pinned Melee `native-3ds/port/3ds/render_worker.c` requires immutable borrowed
sources and a CPU consumption fence before owner reuse. This implementation
currently consumes the map data synchronously and needs no new thread, service,
GPU fence or cache flush. A future New3DS worker must establish equivalent source
lifetime and mutation fences; private query scratch alone does not make map
access safe concurrently. The existing audio worker already runs on core 2 on
New3DS, so moving work there cannot be assumed to be free.

`tools/test_ground_receiver.py` extracts the complete original world triangle
builder, ground hit collector, height sorting and nearest-hit selection. Its
32,000 synthetic queries vary live vertices, groups, flags, cell masks, map
origins, collision offsets, source addresses and repeated queries. It checks
identical hit counts, heights and normals and unchanged shared scratch. Current
host ASan/UBSan result: zero height/normal difference, 14,958 successful receiver
queries, 2,562 source-limit cases. It inherits the original collision-view test's
two documented source-UB exclusions (edge-array subobjects and signed cell-bit
shift). This does not establish physical performance or full shadow fidelity.

## Load-time collision mask index candidate

The next candidate prepares 16 packed triangle bitsets per map block: eight X
cell masks and eight Z masks. Their intersection enumerates only source-admitted
triangles in original order. It retains live group filters/bounds, vertex data,
collision height offsets, the source candidate limits and exact plane testing.
It caches no query result, world height or borrowed vertex pointer. Source cell
masks are immutable after unpacking; the original terrain animation changes
vertices and group bounds instead. The caller still checks the live mask too.

The index is built at the end of `MapBlock_init`, after pointer relocation and
endian conversion. It is invalidated through the same real `mm.c` release
boundary as native scene resources, before the game allocation can be reused.
Explicit same-address reload replaces the index. At most 128 owners and 128 KiB
of normal-heap bitsets are resident; each triangle costs about two bytes plus
word padding. Failure to allocate or a missing index preserves the complete
streaming scan rather than dropping receivers. Shutdown retires these CPU-only
indexes after game use ends. No GPU memory, cache flush, service or new worker.
This adapts the pinned Melee immutable-data/owner-lifetime rule to collision
metadata, without copying its render queues or reintroducing GX interpretation.

Pinned source audit: `mapBlockUnpackPolygons` is the only cell-mask writer;
`mapBlockGetPolygon` consumers are the collision builder, GroundAnima and
XYZAnimator. The two animator consumers read vertex indices and mutate vertices
and group bounds, not the masks. Group ranges/flags remain live in the query.
If the source contract changes, add a publication/invalidation boundary before
allowing mutation; pointer equality alone is not enough.

`tools/test_ground_mask.py` compares 24,738,537 ordered selections with the source
mask predicate under ASan/UBSan, including group slices, same-address reload,
capacity exhaustion and full-scan behavior. The complete source-query test now
mutates vertices, collision offsets and group flags between queries, and tests
both indexed and missing-index paths. All 32,000 queries still match exactly:
14,946 receiver hits and 2,534 source-limit cases, zero height/normal difference.
The changed totals follow the added mutations, not relaxed comparisons.
Shared replay77ebb7ed13b1 passes serial exact-CIA-content New/Old cave checks,
including state assertions, save preservation, shutdown/log drain and owned
exit. Both retrace1410 captures were inspected; silhouettes, room geometry,
Fox, HUD and particle mist remain. No measured interval exceeds250ms.

| Azahar cave with silhouette shadows | Scene preparation ms | Native submission ms | FPS |
|---|---:|---:|---:|
| New, before mask index | 3.592915 | 5.524021 | 59.83184 |
| New, mask index | 3.003793 | 5.525455 | 59.83122 |
| Old, mask index | 2.951334 | 5.538215 | 53.46049 |

The earlier Old planar-v1 check was52.003FPS/scene3.532577ms and predates the
shadow-fog correction; do not attribute that entire difference solely to the
index. The New pair uses the same fog behavior. Whole-run New query mean drops
from241.033 to156.329us, with5005queries/218790accepted triangles unchanged.
Both profiles prepared11indexes, released1 during startup, used30,912bitset
bytes plus2,588metadata bytes, with no declined allocation or query-index miss.

Peak heap New48,109,904/71,303,168 and Old40,769,872/50,331,648bytes;
free linear19,474,048/25,165,824 and4,793,984/10,485,760, VRAMfree4,858,880.
Process commitments103,124,992/130,023,424 and67,473,408/83,886,080;
measured main stack72,836/524,288. Memory headroom remains above15%.
This is emulator-only evidence, not physical FPS or a passed overall shadow
budget: New scene overhead versus shadowsOFF remains about1.2ms. Keep opt-in.
Evidence ground-mask-v1-builds, ground-mask-test, and analyses under
planar-shadow-test/ground-mask-v1-cave-on-{new,old}.json. Not deployed.

## Exact stationary-query reuse (local)

Replay ffb3de564ac3 adds a bounded 64-entry result store (16,944 bytes on ARM).
Its key includes exact position, current map-cell membership and grid locations,
world origin and collision-mutation generation. It stores the entire query result,
including a missing receiver. It never reuses results just because an actor has
not moved. Changed vertices/bounds publish through the GroundAnima/XYZAnimator
native position boundary; HitAnimator_applyBlockState, blasted_activateMapLayer
and dimlavasmash_setBlockSurfaceFlags publish collision-group edits. Map load and
allocation release invalidate results, including same-address reuse. The live
cell membership/origin key covers remapping without assuming static maps.

These are main-thread publications. No source data crosses threads and no GPU
resources are added. Generation wrap clears the bounded store. Global invalidation
on ordinary allocation releases is conservative; no stale data is accepted to
improve the cache hit rate. The source query remains the miss path.

ASan/UBSan compares 48,000 original/replacement queries, including exact reuse,
vertex/offset/group mutations and missing mask indexes: 22,419 receiver results,
3,801 source-limit cases, zero height/normal error. Separate tests execute the
actual generated group-writer bodies and terrain-publication function, including
early return and disabled/unattached renderer cases. Result-store tests cover map
membership/origin, zero-hit caching, wrap and eviction.

| Serial Azahar cave, exact same replay binary | Scene preparation ms | Native submission ms | FPS |
|---|---:|---:|---:|
| New, silhouettes ON | 1.995498 | 5.517504 | 59.83122 |
| New, all ground shadows OFF | 1.812184 | 5.073268 | 59.83122 |
| Old, silhouettes ON | 1.958338 | 5.544498 | 56.26819 |

All three pass state checks, save preservation, shutdown/log drain and owned exit.
ON captures1410 were inspected on both profiles. New reused4977/5005 queries and
Old4669/4697; both had28 misses,442 invalidations and1224 triangle tests. Mean query
cost is14.08/14.12us, including hits, compared with156us before result reuse.
The stationary replay intentionally benefits most; this is not a measurement of
moving casters or terrain. Mutation correctness is separately source-tested.

The matched New addition is0.183314ms scene +0.444236ms native submission,
about0.628ms CPU, still above the0.5ms feature budget. Old improved from53.46
to56.27 AzaharFPS versus the previous mask-only candidate. These are not console
FPS predictions and do not prove the physical GPU cost of the extra draw pass.
Defaults remain OFF on both hardware models.

Peak heaps remain48,109,904/71,303,168 New and40,769,872/50,331,648 Old;
free linear19,474,048/4,793,984 and initialized free VRAM4,858,880 are unchanged.
Process commitments103,141,376/130,023,424 and67,489,792/83,886,080 retain headroom;
measured main stack72,836/524,288. Analyses: planar-shadow-test/ground-result-v1-*.
Archive: ground-result-v1-builds. Not deployed.
