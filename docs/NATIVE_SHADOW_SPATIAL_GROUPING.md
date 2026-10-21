# Spatial shadow receiver grouping

Local experiment, not deployed. The native shadow index-range prototypes V1/V2
retain too much geometry inside their contiguous ranges. This version orders
the shadow-only index triples by Morton codes computed from triangle centroids,
then builds the same conservative bounds hierarchy. Ordinary world indices,
vertices, texture coordinates, collision and simulation are untouched.

No triangle is removed, split, reversed or simplified. Equal Morton codes use
the three original indices as a deterministic tie-breaker. Quantization controls
ordering only; bounds use original positions. Heap sorting uses one fixed
64 KiB normal-RAM key table, with no transient heap allocation. Batches exceeding
16,384 triangles retain their full original order rather than dropping geometry.
All work runs on the existing serial scene owner at load or fenced world-position
mutation; there is no new thread or ordinary per-frame geometry rebuilding.

The envelope selector emits at most one draw per existing receiver batch. It
deliberately retains interior gaps to avoid multiplying submission cost.
The source audit covers1,259 world packets and39,804 footprints: indices fall
21,302,352 to11,108,712; selected ranges38,288 vs39,804. Every oriented index
triple is retained with identical multiplicity and positions.3,397,880 rejected
triangles have individual double-precision plane witnesses. The input assets
remain byte-identical. Sanitizers cover256 compact/float lifetime, capacity,
mutation and nonfinite cases.

Shadow receivers use multiplicative darkening with depth writes disabled.
Reordering therefore needs image acceptance even though geometry is identical:
quantized blending of overlapping surfaces must not be assumed exact globally.
A local replay of the existing captured cave caster and ten receiver draws
changes only the receiver triangle order. All ten output images are byte-exact
against the original ordering. The projected shadow image was also inspected.
This is limited workload evidence, not full gameplay or physical raster proof.
See `artifacts/diagnostic/shadow-spatial-order-images-v1/order-comparison.json`.
No new hardware diagnostic was installed or requested.

The first cave runtime preserves715 casters,4,290 authored masks and46,475 draws,
while receiver indices fall48,461,985 to26,276,574. CPU scene+submission rises
about0.67ms, so this version is not promoted. Its receiver metadata is25,772 bytes
(+22,592); linear/VRAM allocations are unchanged. Fixed sorting scratch adds
64 KiB normal RAM. Process commitment is103,374,848/130,023,424 on the New profile.

The next candidate adds256 exact query-cache entries. Its key includes the node
owner, root/end and all four footprint planes; collisions only replace an entry.
Every successful geometry/tree rebuild clears the cache, including same-address
reload. A result is reused only for an identical query against unchanged geometry.
Changed cameras/footprints miss normally. No approximate matrix key or temporal
shadow skipping is used. The real mutation test rejects a deliberately removed
invalidation. The cached cave run completes with the same 715 casters, 4,290
authored masks, 46,475 draws and 26,276,574 receiver indices. It hits 26,824 of
46,475 queries (57.7%), with 13 invalidations. The cache occupies 22,528 bytes
of normal RAM. Scene plus submission overhead falls to +0.338571 ms versus
the accepted baseline, retaining the 45.78% receiver-index reduction. This is
a candidate GPU/CPU tradeoff, not a measured physical speedup. Wider visual
acceptance and console net cost remain open; no hardware request is pending
while the user is unavailable.

New-profile process commitment is 103,395,328/130,023,424 bytes; heap peak
48,427,232/71,303,168, linear free 18,441,856/25,165,824, VRAM free 4,723,712,
and measured main-stack use 73,008 bytes. All eleven linked shaders and the
game archive match the accepted baseline. This experiment is excluded from
the consolidated normal sky/culling candidate.

Audit bookkeeping correction: derived wrappers originally changed a quoted
directory token that did not match the full base path. Their correct results
overwrote only the base asset-audit files. Those files were archived under
`audit-location-repair/cached-spatial-overwritten`; the wrappers now replace
the directory substring, and each of the four source audits was rerun into
its distinct directory. Runtime records and build verification were unaffected.
See `shadow-spatial-cache-test/{result,runtime,build-verification}.json` and
`shadow-spatial-cache-sanitized/{cache-result,stale-cache-result}.json`.

Pinned Melee geometry-bounds conservatism and reader-retirement rules are kept.
The unchanged citro3d draw-elements owner publishes the selected index pointer;
all native shaders, GPU buffers, materials and fences remain as before.

## V3: combined camera visibility

The spatial hierarchy now composes with the previously checked camera mask.
Offscreen receivers skip footprint queries and draw submission. Position mutation
rebuilds spatial bounds and clears exact query results, then makes registered
camera masks conservative before publishing GPU storage. The composed source
passes 1,000 owner cycles and 5,000 disabled poison-pointer calls; the mutation
negative control still fails. The range compiler/cache is byte-identical to V2.
Six additional sanitizer cases cover 16,383/16,384/16,385 triangles in compact
and float form, exact index multiplicity, and unchanged order above the sort cap.

The New cave replay preserves 715 casters and 4,290 authored masks. Receiver
draws fall 46,475 to 34,320 (-26.15%); indices fall 48,461,985 to 23,400,129
(-51.71%). Added scene+submission CPU is +0.258036 ms versus the accepted
unculled-shadow baseline, down from V2's +0.338571 ms. This is a measured
workload reduction with an added CPU cost, not proof of a console frame-rate win.
The exact query cache hits 20,317/34,320 requests. All 11 receiver plans load
without declines. The 1410 cave image was inspected with world/player/shadow/HUD
present; animation/fog phases differ, so this is not a pixel-equality claim.

The shared binary also completes the Old-profile cave replay: all 11 plans,
zero declines, inspected 1410 image, saves unchanged, shutdown logs drained and
owned exit. Old scene/submission are 1.873229/6.067848 ms and emulator cadence
55.233 FPS, with no matching Old baseline or console forecast claimed. Heap
40,890,592/50,331,648 bytes; linear free 3,761,792/10,485,760; VRAM free 4,723,712;
measured main stack 73,008; process 67,747,840/83,886,080. New process commitment
is 103,399,424/130,023,424; other measured New residency matches V2.

Build `shadow-spatial-v3-builds` has five changed files relative to the accepted
world-visibility base; game archive and all eleven shaders remain identical.
Shared Old80/New124 CIA profiles are verified. Replay 3dsx `36664722d599`,
CIA `af382ab0fff3`. This is still local and separate from the consolidated sky
release. Evidence: `shadow-spatial-visibility-test`, `shadow-spatial-capacity-test`.

The existing original-geometry Thorntail route also completes with shadows ON,
normal movement, unchanged saves and owned shutdown. Both 1500/1650 captures
were inspected: Fox, trees, Arwing, terrain, HUD and ground shadows remain.
All 25 receiver plans load with zero declines. Compared with the archived same
route, indices are 4,706,769 versus 8,144,163 and draws 21,748 versus 24,747;
casters differ (2,158 versus 2,143), so these are not exact fixed-state savings.
Scene+submission rises 0.105927 ms. Azahar cadence 53.759 versus 52.996 FPS
does not establish a speedup. Moving-query cache hits only 2,695/23,777 (11.3%);
do not extrapolate the static cave's cache benefit to moving gameplay.
New heap peak 56,489,088/71,303,168 bytes, linear free 16,185,984/25,165,824,
VRAM free 4,723,712, measured stack 73,068, process 103,399,424/130,023,424.
See `shadow-spatial-visibility-test/moving-runtime.json`.

## V4: shared footprint preparation

The experimental visible-mesh receiver path now prepares the exact footprint key
once per world instance/caster, lazily after finding a candidate draw. A 72-byte
stack context borrows the immutable planes only for that receiver loop; the
bounded cache retains its own complete key. Node/root/end and all sixteen plane
components still participate in identity. Hash collisions only replace entries.
This follows the pinned Melee plane-cache principle of sharing work across
chunks without using approximate camera keys.

All 1,259 asset packets retain the V3 results. Sanitized mutation checks and
32,768 alternating forced-collision queries pass; both missing invalidation and
hash-only identity mutants fail. The New cave completes with the exact same
715 casters, 34,320 receiver draws and 23,400,129 indices as V3. Key preparation
falls to 10,010 calls. Added CPU versus the original baseline is +0.240273 ms,
only 0.017763 ms below V3. This small observation is not a substantial speedup
or reason for a hardware request. No redundant Old/moving runs were repeated
for this isolated key-preparation change. The 1410 capture was inspected.

Heap/linear/VRAM/stack measurements match V3. Process commitment grows one
4 KiB code page to 103,403,520/130,023,424 bytes. Shared 3dsx `c697cced567c`,
CIA `02cee33c073a`; same game archive and eleven shaders. Evidence is in
`shadow-query-context-test` and `shadow-query-context-sanitized`. No normal
build or deployment. This visible-mesh architecture still differs from the
source collision-surface policy; further micro-optimization does not close it.
