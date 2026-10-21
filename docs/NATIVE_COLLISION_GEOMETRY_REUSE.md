# Collision geometry reuse candidate

The shipping collision producer builds planes and optional edge normals from
live integer triangle coordinates on each query. This candidate retains only
those pure geometric results in 256 fixed slots (18,432 bytes). Query bounds,
surface flags, material selection, output order and capacity checks stay live.
There is no GPU, thread, allocation, renderer or simulation-rate change.

The older content-key cache hashes all nine coordinates. The new candidate
uses a triangle address only to select a slot, then compares all nine current
output coordinates and required edge coverage before reuse. Recycled pointers,
changed vertices, indices, offsets and scratch aliases cannot prove validity.
Unlike Melee's producer-owned snapshot cache, no data is borrowed by another
thread and no pointer identity is accepted as a validity proof. Consulted pinned
`source_snapshot_cache.h` and `rotation_cache.c`. Floating-point control changes
are an additional integration concern: this isolated candidate assumes the
fixture's fixed arithmetic mode and has not established runtime mode invariance.

`test_collision_identity_geometry.py` binds its actual producer body to the
current generated translation unit, DWARF MD5
`83ad3bbfb06ac37c2e2cf96e8710acc0`. The existing independent collision oracle
compares ordered records across 2,000 mutable queries and three repeats each,
810,603 accepted triangles and 224 capacity hits, under ASan/UBSan. Source data,
grid outputs and buffer boundaries are checked. Additional checks require a
plane-only cache result to miss when edge normals become necessary, and reject
changes to each of the nine coordinates. Address-only and ignored-edge mutants
fail. The initial edge mutant survived the repeated-query fixture; this missing
coverage was corrected before acceptance.

An XOR address hash yielded 193,542 hits / 4,042,738 misses. A multiplicative
address hash yielded 190,149 / 4,046,131. Both hit rates are about 4.5%; this is
a synthetic mutable-map workload, not a measurement of gameplay reuse. It does
not justify deployment or an FPS prediction. The address hashes depend on the
fixture's allocation layout; host hit counts do not predict ARM hit counts.

`test_collision_identity_geometry_arm.py` compares 16 identical ARM11 fixture
trials including three repeats, with inclusive math and memory callees. An
earlier reduction to four trials failed a fixture assertion and has no accepted
cost result. The candidate remains disconnected from application builds;
production Old4f58/Newd478, private assets, saves and settings are unchanged.

Evidence lives in `artifacts/research/pica-scene-rewrite/collision-identity-geometry-test/`.
Do not repeat a full emulator boot or request hardware merely for this prototype.

## Cost gate: rejected

The 16-trial ARM run completed all 64 calls and correctness assertions. Inclusive
instruction encounters rose from 13,365,154 to 17,336,309 (**+29.713%**). Producer
self counts rose 12,645,060 to 15,498,906; its code grew 3,528 to 4,056 bytes.
These synthetic counts do not establish a gameplay regression percentage, but
they reject this cache as an integration candidate. No full game build or console
test is warranted. Preserve `arm-fixture.json` and the failure records.

Do not weaken coordinate or edge-coverage checks to recover performance. Any
future retained collision plan needs evidence about actual query reuse and
mutation ownership before adding another lookup/cache in the hot query loop.
