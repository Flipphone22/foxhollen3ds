# Conservative wake admission

14 October local candidate; not deployed or performance-accepted. This addresses
the prepared wake mesh's real vertex workload before adding another effect.
It does not change game simulation or replace the source wake pool.

The source quad is prepared as24 bytes of conservative bounds at load time.
At draw time, `fh_water_bounds_outside` transforms interval bounds through the
live model-view and viewport-folded projection. It only rejects sheets wholly
beyond a screen side plane with positive W. Camera-plane crossings, uncertain
values and near/far-depth questions remain on the ordinary GPU path. No
subpixel LOD, minimum opacity, temporal visibility cache, vertex scan or GPU
readback is used.

This adapts the pinned Melee `port/engine/geometry_bounds.h` side-plane approach
and existing world-renderer margins. The water helper preserves separate eye
and clip intervals so matrix cancellation does not silently discard uncertainty.
The source position construction, view and clip intervals are inflated by0.2%
of their arithmetic magnitudes, with an additional1/16-pixel side-plane margin.
Texture displacement changes UV only; the bounds enclose all prepared geometry.
This is deliberately conservative and may retain invisible sheets.

The check happens before resource binding and the two mesh submissions. It
does not yet avoid the earlier shared scene capture or source matrix preparation.
HUD flushing and target admission retain their previous ordering. No SDK, GPU
command, shader, geometry or allocation behavior changes for retained draws.
Counters record checks, rejected sheets and bounds time. Bounds time overlaps
visible draw-owner time and must not be added to it as a separate frame cost.

The actual helper passes200000 independent double-precision quad-oracle cases,
with80486 safe rejections, plus explicit camera-plane/NaN cases. The pinned PICA
interpreter then checks131072 corner/interior vertices and7536 rejected boxes
without a rejected sampled GPU-visible hull. This second test checks admission,
not a newly tightened transform accuracy guarantee; its observed coordinate
arithmetic error is8.84e-5. The existing shader bytes are unchanged.

Actual-owner ASan/UBSan tests pass128 lifecycles,2562 retained draws and128
off-screen rejections, with retirement/cleanup/uniform/visible-cull mutants
rejected. Existing resource failure paths remain covered. The helper's local
ARM stack is64bytes, excluding callers. Evidence is in
`water-bounds-test`, `water-bounds-shader-test` and `water-bounds-owner-test`.

Replay source `63fefecbd9e8` builds from the checked water-grid snapshot in
`water-visibility-v1-console-replay-builds`. Only the new header and water owner
change; game archive and all18 shader programs remain identical. Shared CIA
Old80/New124 and restoration checks pass. Runtime benefit must be established
from recorded rejections and timing, not assumed from off-screen unit tests.

The real New route `water-visibility-v1-on-new` completed in82.25 host seconds,
preserving saves, draining logs and exiting its owned emulator. Capture1740
was inspected: world, Fox and HUD remain present. All225 admitted sheets
remained visible to the conservative check:225 checks, zero rejections,
450 draws. Bounds instrumentation recorded260094 ticks over the whole run,
nested within visible-owner work. Concurrent unrelated compilation invalidates
timing attribution. This route establishes no saved geometry or performance
benefit. The candidate remains experimental; no normal build or deployment is
justified by this result. The previous water-grid pair remains the local base.

## Two-plan ripple candidate, 14 October

The same conservative helper is now prepared for both resident water plans.
Actual two-plan owner tests retain the earlier lifetime/material checks and
exercise off-screen rejection for both families. No shader or game archive
changes were needed for `ripple-visibility-v1-console-replay-builds`.

The New Thorntail route submitted6,737 ripple candidates and rejected1,236
(18.35%), retaining5,501 instances/11,002 draws. This avoids2,472 draws and
7,549,488 prepared triangles over that run. Bounds cost27.69 ms over the
whole run, nested in owner work, or4.11 microseconds/check. Capture1740 was
inspected; replay assertions, unchanged saves, drain and owned exit pass.

The matching OFF binary performs zero bounds checks, capture work and ripple
draws. ON/OFF rendering CPU totals are8.2761/7.5418 ms, exceeding the0.20 ms
New-profile effect allowance. The fresh unbounded baseline has different
source ripple counts, so neither the18.35% saved work nor its FPS establishes
an overall speedup. All three runs have no unrelated compiler at their boundary
snapshots; that is not continuous host-utilization proof. Evidence:
`ripple-runtime/visibility-pair.json`. The response to this failed cost gate is
the scoped submission redesign in `NATIVE_WATER_BATCHES.md`, not another
effect layered onto an over-budget path.
