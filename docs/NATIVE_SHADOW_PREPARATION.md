# Single-pass compact shadow preparation

This local change removes repeated work at asset admission. It does not change
per-frame rendering, reduce triangle counts or establish a frame-rate gain.

The previous compact-shadow owner asked the compiler for an exact allocation
size, then compiled into that allocation. Those calls performed three complete
deduplication passes: one to measure, another to recompute the compact layout,
and a third to emit it. The owner now prepares one temporary immutable mapping,
allocates the exact persistent buffers, and emits vertices/indices through that
mapping. Emission has no allocations or hash lookups. Scratch is freed before
the GPU buffer is published.

The borrowed source asset and model plan must remain immutable until preparation
is closed. Admission already owns that interval; there is no new worker or
cross-frame cache. Every allocation/budget exit releases the prepared mapping.
The existing scene-reader retirement fence, one-time cache publication, 512 KiB
linear/64 KiB resident-CPU budgets and normal-memory reserve remain. The second
reserve check now charges only the future resident CPU allocation because the
prepared scratch is already included in current heap use.

The scratch query includes the temporary header, draw layout, one reusable hash
table and all source-to-output indices. The checked 835-model collection peaks
at 157,666 bytes on the host ABI, versus 157,634 previously. Artificial maximum
32,768-input-vertex batches use 327,792 bytes. Larger multi-batch inputs are
charged for their full mapping before allocation; the owner declines an optional
compact plan if it cannot preserve the normal-memory reserve. Ordinary native
shadow geometry remains available under the existing policy. No old GX renderer
is reintroduced.

Pinned Melee `port/3ds/command_cache.c` and the current scene storage owner were
reviewed for cache publication and retirement. This change needs no SDK, cache
coherence or command-submission modification.

`tools/test_shadow_prepared.py` compiles the archived prior compiler and current
compiler, then compares their actual outputs over all 835 prepared actor models.
All 6,959,312 bytes of GPU storage and all draw descriptors are identical,
including vertex order, indices, padding, bone palettes and palette-change bits.
There are 1,670 rejected output-capacity cases with unchanged destinations.

Counted load-time work over the collection:

| Operation | Previous owner sequence | Prepared owner sequence |
| --- | ---: | ---: |
| Deduplication passes | 3,120 | 1,040 |
| Vertex-key construction, including emission | 1,442,616 | 688,486 |

These are counts from instrumented actual compiler bodies, not elapsed CPU time,
GPU throughput or a scene-frequency-weighted estimate. No physical improvement
is established. The independent ordered-corner test also retains all 328,012
triangles, 207,614 vertices, 1,040 draws and 2,505 malformed/capacity rejections.
Maximum-batch ASan/UBSan, weights/tails, allocation failures and the rejected
position-only weld mutant pass. Actual owner tests pass 100 delayed-reader
load/retirement cycles; removing the fence is still rejected. ARM compilation
passes with warnings treated as errors.

Evidence: `artifacts/research/pica-scene-rewrite/shadow-prepared-test/`.
Shared replay `a5dde74e3cbc`/CIA `c982d8b68566` and normal setup
`5731f4a18936`/CIA `65f2e0859c49` are built, not deployed.
`build-verification.json` confirms both link the prepared owner, dead-strip the
old compile entry, retain the game archive and shaders, and preserve Old80/New124
CIA profiles. Both builders restored the worktree. No console files have changed. Runtime admission counters, actual load-time savings,
hardware validation and full shadow/rendering parity remain open. Existing
image evidence is reusable only for the byte-identical geometry and unchanged
GPU path; it does not prove this new admission lifetime executed in gameplay.
