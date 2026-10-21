# Keeping collision vector calculations in the query

This candidate inlines the existing Aurora `C_VECSubtract`, `C_VECScale`,
`C_VECCrossProduct` and `C_VECDotProduct` expressions into
`trackBuildBlockTriangles`. It preserves arithmetic order and the cross-product
temporary. Magnitude evaluation, division, threshold tests, live coordinates,
topology admission, query order and capacity handling remain unchanged.

Unlike the rejected geometry-cache candidates, this introduces no retained
geometry, invalidation mechanism, allocation or cache lookup. These calculations
remain CPU work because the game needs the resulting collision records before
simulation continues. No rendering, simulation or collision checks are omitted.
The pinned Melee exact-math approach in `rotation_cache.c` was consulted; this
does not copy its cache or change floating-point mode.

`tools/collision_inline_vectors.py` extracts the four source function bodies and
restricts the consumer replacement to the gameplay collision function. It does
not replace shadow receivers or the global SDK functions. SDK debug assertions
are excluded as they are in production builds; all call sites use local records.

Initial evidence under `collision-inline-vectors-test`:

- Host ASan/UBSan: 2,000 mutable queries, 810,603 ordered triangles and 224
  capacity hits match the existing reference. The fixture now uses the actual
  four vector expressions instead of its former simplified versions. Magnitude
  remains a fixture sqrt model, so this is not complete production math execution.
- An incorrect cross-product expression is rejected.
- Complete production translation units compile with identical ARM options.
  The target function grows from 5,312 to 5,360 bytes. Relocations verify that
  exactly the four vector callees disappear and all other called symbols remain.
- Production ARM prologues reserve 440 -> 368 bytes including saved integer
  and VFP registers: 72 bytes less per active invocation. This is the function's
  own frame, not a new measured application stack watermark.
- Source SHA-256 is `7d168e8358fb5a4113f3715e35b26c79e00e5fcb3e93cf699b8a12a456e4824d`;
  candidate SHA-256 is `9f483aa2aea8eba618f1e8296077104e579a1b4a26c102b438af355c6e6cbee9`.

The inclusive ARM fixture test counts callees as well as the query body. It uses
out-of-line baseline vector functions to represent the SDK compilation boundary.
Its result belongs in `arm-fixture.json`; it is not a whole-frame or hardware
performance measurement. No performance conclusion follows from code size alone.

The completed 16-trial ARM fixture executes 64 query calls per implementation.
Inclusive instruction encounters decrease from 18,357,430 to 13,365,154
(27.195%). Query-body encounters are 13,685,044 -> 12,645,060. This includes
the effect of making intermediate vectors visible to the compiler, not merely
removing call/return instructions. The executable fixture's producer layouts
(3,748 -> 3,528 bytes) differ from the complete production TU sizes above;
do not conflate those binaries or extrapolate a console FPS percentage.

`build_collision_inline_vectors.py` requires the correctness, call-retirement
and favorable inclusive ARM results before building over local `df5668002cdc`.
It isolates archived sources, changes only the adapter and `track_dolphin.o`,
verifies the generated TU, and restores root/archive state. A build is not a
deployment or gameplay acceptance; require runtime evidence before promotion.

## Integrated local result

Shared normal build `7b980b145315` includes this change atop the guarded particle
update candidate. Only `track_dolphin.o` changed; its target section is byte-equal
to the independently compiled complete production TU. All shaders, memory
profiles and setup recipes are unchanged, and root/archive restoration passed.

The exact CXI completed the Old palace replay: state, saves/settings, log drain
and owned exit pass. The frame-1600 image was inspected. Heap peak 41,366,136 /
49,283,072, linear minimum free 3,667,840 / 12,582,912, VRAM free 4,723,712,
stack 73,008 / 524,288, process 70,713,344 / 83,886,080 bytes match the parent.
These remain sampled scene budgets, not whole-game worst-case measurements.

Eight-update main instruction encounters are 14,803,579 versus shipping
15,243,960 (-2.889%) and immediate parent 14,846,949 (-0.292%). Major game entry
counts match, but particle/transparency activity and presentation count differ.
No causal whole-frame gain or hardware FPS estimate is established. The 154.70
host seconds are excluded from performance comparisons because of instrumentation
and foreign activity. This is a bounded local improvement, not target completion.
Neither console has been updated with this build.
