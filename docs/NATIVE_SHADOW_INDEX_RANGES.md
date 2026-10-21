# Persistent shadow receiver index ranges

Local candidate, not deployed or accepted for physical performance.

The native projected-shadow pass was redrawing broad resident world batches for
each small shadow footprint. Even the camera-culling candidate submitted about
44 million receiver indices during the cave replay. The new load-time compiler
adds a binary hierarchy over contiguous index ranges, with leaves of at most
64 triangles. Every original triangle, winding, index and position is retained
in order. The existing position-only GPU buffers are unchanged.

At draw time, four conservative footprint planes reject entire subtrees.
Wholly contained nodes stop traversal early. Adjacent retained ranges coalesce
before submission. The PICA still transforms resident vertices; the CPU traverses
bounds and submits index ranges, without expanding, copying or uploading geometry.
This trades extra CPU traversal and potentially more draw calls for fewer GPU
vertices. A source audit alone cannot establish a net speedup.

Trees share their scene owner's normal-memory allocation, existing 64 KiB
aggregate optional receiver-metadata cap and heap reserve checks. GPU/linear
allocation, shader programs, textures, workers and source simulation are
unchanged. If the optional plan cannot fit, the existing complete native world
receiver path retains all shadows. No rendering disappears to meet a budget.

When source world positions move, the existing owner joins GPU readers, refreshes
geometry, rebuilds receiver bounds and this tree, then flushes GPU storage.
Index counts determine topology and allocation, so a position change cannot
overflow the tree or retain stale spatial rejection. Disabled shadow rendering
never traverses it. Trees currently remain resident when shadows are disabled;
that memory cost is explicit.

The four-plane predicate and finite fail-open behavior follow pinned Melee
`port/engine/geometry_bounds.h`; its reader-fence ownership is retained. Pinned
citro3d `drawElements.c` publishes the chosen index pointer relative to the
unchanged buffer base. Every emitted range calls the existing capacity owner
and rebinds after invalidation. No direct GPU register writes were added.

Host evidence: 1,259 complete private world packets; 39,804 synthetic footprints;
2,702,396 rejected triangles checked individually against a double-precision
plane witness. Source order/coordinates are exact and input bytes unchanged.
Synthetic submitted indices decrease 21,302,352 to13,195,164, with draw ranges
increasing 39,804 to47,276. These are coverage measurements, not gameplay timings.
Tree storage across the entire collection is878,092 bytes; only resident world
blocks allocate it. The largest individual packet's host metadata is9,492 bytes.

ASan/UBSan passes256 compact/float lifecycle, capacity, boundary, mutation and
nonfinite traversal cases. The real mutation owner's join/rebuild/publication
order is checked. Evidence is in `shadow-native-ranges-test` and
`shadow-native-ranges-sanitized`. Runtime and hardware acceptance remain separate.

V1 cave replay completed normally, but failed the CPU tradeoff: scene plus
submission increased by1.536632 ms. Receiver indices fell48,461,985 to37,201,554,
while draws rose46,475 to82,194 (caster counts715 vs714). All11 receiver plans
remain present; heap increases22,608 bytes, linear/VRAM/stack unchanged. The1410
image retains cave, player, HUD and shadow, with a different fog phase. V1 is
not promoted and its failure evidence remains archived.

V2 changes traversal to find just the first and last intersecting leaves and
submits their contiguous envelope. Interior gaps deliberately remain drawn.
This guarantees no more than one draw per original receiver, retains source
order and bounds conservatism, and avoids visiting every interior leaf. Source
audit:15,353,427 selected indices vs21,302,352 coarse,39,217 draw ranges vs39,804.
Both source and sanitized mutation/capacity tests pass. Runtime acceptance pending.

V2 runtime also fails the CPU gate: scene plus submission rises0.610121 ms
against the matching accepted baseline. It retains46,475 draws and reduces
receiver indices only48,461,985 to45,985,653 (5.11%). Caster and authored-mask
counts match exactly715/4290. Both runs complete normally with saves preserved
and owned-process cleanup; all11 receiver plans are present. Tree metadata adds
22,592 resident normal bytes; linear, VRAM, stack and process commitment remain
unchanged. The1410 image retains cave, Fox, HUD and ground shadow; fog phases
still differ. No physical speedup is established and V2 is not promoted.

The full-range order has poor spatial locality for these small footprints.
Fine selection raises draw counts; its one-draw envelope retains most interior
geometry. Further work needs a different representation or spatial grouping,
with explicit ordering/image validation, rather than smaller leaves and repeated
boots of this known tradeoff. No Old-profile repetition is justified yet.
