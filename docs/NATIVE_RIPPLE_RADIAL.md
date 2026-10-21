# Load-prepared radial ripple candidate

14 October: implemented and tested locally, **not connected to gameplay or
deployed**. This is a candidate mesh level for small ripples, not a blanket
replacement of the current mesh. The previous turn completed resident uniform
publication and its GPU checks; this turn investigates the larger triangle cost.
The renderer/performance goal remains active and incomplete.

The current ripple plan submits 3,054 triangles per pass, twice per instance,
even when small on screen. The original procedural texture describes a radial
field, so the new load compiler places samples on concentric rings and angular
rays. It samples the user's original generated RGB5A3 A/B offsets; the original
G channel still supplies coverage. Texture generation, simulation, spawn state,
poses, opacity and both reflection passes are untouched. No game-derived mesh
or texture is embedded in source.

The bounded compiler in `pica_water_grid.c` supports 24..64 angular segments
(multiples of eight) and 12..32 rings (multiples of four). Explicit square-corner
rays and clipped outer rings preserve the complete source domain. Vertex
deduplication runs only at loading. The existing SHORT4/full-fetch-padded 16-byte
format and native shader consume it directly; no per-frame geometry generation,
CPU rasterization, extra pass, thread, readback or new GPU state mechanism.

Pinned Melee `early_queue.c` retirement/publication rules remain unchanged in
the isolated owner. The replacement compiler allocates nothing itself; the
owner uses its existing checked preparation allocation, linear admission,
cache flush and joined release. SDK bindings and queue handoff restoration
remain those already checked by the water batch/uniform work.

| Mesh | Triangles per pass | Prepared bytes | Aligned linear bytes |
|---|---:|---:|---:|
| Current reduced square grid | 3,054 | 44,820 | 44,928 |
| Candidate 32 segments / 16 rings | 1,078 | 15,380 | 15,488 |

This is 64.7% fewer submitted triangles for that effect. It is **not** a measured
FPS or GPU-time improvement. The full-square radial topology can rasterize more
zero-alpha pixels than the coverage-pruned grid. The candidate's CPU preparation
reservation is 17,152 bytes versus 1,415,271 for the prior compiler/reducer path;
it bypasses that simplifier. Its ARM function-local stack is 2,576 bytes plus
callees. Retaining both levels for gameplay would retain both meshes: do not
claim the single-plan fixture's linear saving for a future two-level owner.

## Image limits

The independent software experiment uses the actual C mesh and source offsets,
a private scene capture and native screen patch dimensions. At 128x64 pixels
with varying projective Q, mean/p95 colour errors against per-pixel source
offsets increase from 1.410/7.947 to 3.357/19.801 byte levels. The contact sheet
was inspected; large rings lose detail. This is unsuitable as an unconditional
replacement. At 32x16 the respective errors are 0.950/5.157 and 2.071/10.706;
these measurements alone do not establish a safe transition threshold.

Remaining admission work includes subpixel-motion/temporal comparisons,
projected-size selection without repeated transforms, both-level lifetime and
RAM accounting, gameplay coverage and CPU/GPU cost. No LOD threshold or graphical
preset is silently changed. Both water effects stay OFF in all current presets.
Original filtered five-bit bump alpha and full source-game image parity remain
separate unresolved requirements.

## Evidence

- `test_ripple_radial.py`: all 36 configurations pass ASan/UBSan capacity checks,
  all positive triangles, exact square area, paired interior edges and boundary
  coverage. Original source sampling differs by at most 1/32 byte level (the
  existing Q4 rounding). Strict ARM compilation passes. Failed arguments leave
  caller buffers/result unchanged; unused allocation tails are checked.
- `regular-regression.json`: original procedural pixels and all four existing
  square meshes remain byte-identical after adding the new compiler.
- `test_ripple_radial_owner.py`: actual owner, 16 two-plan lifecycles, 6,178
  draws, four allocation/admission failures and no/every/mixed queue reset
  patterns pass ASan/UBSan. Eleven broken retirement, reset, pose, fog, mask,
  UV, strength and SDK uniform-publication variants are rejected. Reload and
  shutdown preserve pending readers; draw paths allocate nothing. Combined
  wake/radial fixture peak is 52,480 linear bytes. Modeled service boundaries
  do not replace physical lifetime acceptance.
- `audit_ripple_radial_mesh.py --compiled`: actual C geometry rather than only
  the Python proposal; five patch sizes/two Q fields and coverage assertions.
  Private outputs reside in `ripple-radial-compiled-pixels`. The initial audit
  accidentally selected reduced0 rather than reduced2; its 3,054-triangle guard
  stopped it before accepting a comparison. The corrected audit uses reduced2.
- `artifacts/native-ripple-radial-device/v1`: silent, isolated GPU fixture,
  eight cases, 48 draws, 28 complete-state and 20 retained-state publications.
  Source mask/mix, independent reflection channels, projective Q, opacity,
  foreground occlusion and queue handoffs pass. Max mean/p99/worst pixel errors
  against the radial-mesh oracle are 0.4251/1.5876/2.2789 byte levels. Actual
  mixed-pass image was inspected. Owned emulator exits in 11.72 host seconds.
  These are shader/output checks, not acceptance of lower mesh detail or timing.

The GPU fixture is not a new normal/replay game build. No hardware probe,
deployment, save, user preference, ISO or prepared installation asset changed.
Foreign compilation is still active; no contended game timing run is treated
as evidence of a speedup. Installed shadow-depth repair remains separately
unconfirmed on physical hardware.
# Subsequent integration

The radial candidate is now available in a local, explicit RIPPLE FAST option;
see [NATIVE_RIPPLE_DETAIL.md](NATIVE_RIPPLE_DETAIL.md). Both mesh levels remain
resident. The historical compiler-only results below are not net memory savings
for that combined implementation. Distance-only automatic selection was rejected
after motion/stress-image comparisons. No physical speedup is established.
