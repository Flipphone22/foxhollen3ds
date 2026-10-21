# Explicit native ripple detail

14 October local integration. No console deployment or physical performance
acceptance. Automatic distance-based selection was not adopted: subpixel-motion
tests show that a small footprint alone does not bound reflected colour error.
The renderer's overall parity/performance goal remains incomplete.

`RIPPLE FAST` is a separate graphics preference beside `RIPPLES`. It selects the
1,078-triangle radial mesh instead of the 3,054-triangle reduced square mesh,
for each of the same two ordered passes. Original simulation, source texture,
live pose/opacity, material, capture, fog and shadow behavior are unchanged.
This is an authorized visual-detail reduction, not original-image parity.
The normal mesh remains the default. All four presets leave both ripple effects
and this preference off; Custom preserves the user's selection.

Both meshes are prepared at load and stored in one owned linear allocation.
Changing the preference only selects the resident vertex/index offsets once
per batch. It performs no SD reads, allocation, CPU geometry rebuild, additional
pass or shader change. Both levels retain the same source coverage mask. Large
reflections visibly lose detail with FAST; it is an explicit tradeoff rather
than silent camera-dependent switching.

The second effects-word bit is `FH_EFFECT_RIPPLE_FAST=2`, independent of
`FH_EFFECT_RIPPLES=1`; earlier bit assignments remain intact. Existing FHGRAPH5
settings and recording/replay snapshots retain the full word. Old settings and
recipes default the new preference off. Recording locks the preference as it
does other appearance options. Unknown bits continue to fail validation.

## Memory and failure behavior

The normal ripple allocation grows from 44,928 to 60,416 bytes when both levels
are admitted: **15,488 additional linear bytes**, not a net memory saving.
Combined with the unchanged wake mesh, plan storage is 97,408 bytes. The
existing temporary peak remains 1,415,271 bytes because the normal plan's
reducer still runs. Metadata grows by three unsigned fields per sheet, plus
small counters. Both levels are resident even when FAST or RIPPLES is off.

The existing 15% linear reserve is enforced before admission. If the radial
preparation allocation, extra linear capacity or larger allocation fails, the
normal plan remains available. Requesting unavailable FAST reports it through
the existing effects-unavailable mechanism, and replay validation cannot call
that requested configuration a success. No per-frame allocation retry occurs.
The `fast_loaded` diagnostic counts prepared plans, including attempts that
later fail image admission; it is not the current resident-plan count.

Pinned Melee ordered publication/retirement remains unchanged. One flush covers
the newly prepared allocation; source image ownership is still shared with the
normal plan. Reload and shutdown retire all readers before release. There is
no speculative GPU fence removal or resource overwrite.

## Validation so far

- Motion experiment: 512 combinations of four footprints, two positive-Q fields,
  eight subpixel phases, four opacities and two backgrounds. At opacity48 the
  scene's worst per-case FAST mean/p95 errors are 0.523/3.147 byte levels,
  but the high-contrast stress image reaches 6.229/28.347. Mesh-switch differences
  and temporal residuals are separately recorded. The moving scene contact
  sheet was inspected. This supports rejecting automatic distance-only
  admission; it does not establish complete source-game or hardware fidelity.
- Actual owner: 16 lifecycles, 9,064 draws, normal/fast switching, no/every/mixed
  command resets, five full-plan failures and three optional-plan failures pass
  ASan/UBSan. Existing broken retirement/state/uniform mutants are rejected.
  Both detail counters must match their actual 9,162/3,234 indices per draw.
  The first harness counted new allocations but not `free(radial)`; its accounting
  assertion failed. The wrapper now tracks that actual free too; failure files
  are retained. Production allocation logic was not changed for that assertion.
- Actual menu/settings/parser tests pass independent toggling, recording locks,
  all four presets, Custom preservation, EFFECTS2/3, unknown-bit rejection,
  v2-v5 migration and round-trip/no-change-no-write behavior. An initial test
  executable path collided with its settings directory; corrected in the tool.
- Replay build v1 caught a missing direct graphics-options include. The isolated
  builder restored the worktree/archive; no failing game was run. V2 includes
  the declaration and retains all 18 GPU programs and the original game archive.
- Dual-resident GPU fixture: eight cases, 48 draws, full/retained state resets,
  capture, masks, Q, opacity and depth occlusion pass. All eight images are
  pixel-identical to the previously inspected single radial-plan fixture.
  Owned emulator exits in 13.75 host seconds; this is not a throughput benchmark.

Evidence is under `ripple-lod-motion`, `ripple-detail-owner-test`,
`ripple-detail-options`, and `artifacts/native-ripple-detail-device/v1`.
Replay v2 source `6ac366c43e31`, 3dsx `9da9750ee0e2`, CIA `58955d0396a4` is built
locally. Shared Old80/New124 profiles and restoration pass. Runtime and matching
normal-build results are recorded separately as they complete.

## Game integration and shared build

The New-profile real Thorntail route completes in 82.78 host seconds, status1,
with all state/save/log-drain/owned-exit assertions passing. It submits 5,178
ripple instances / 10,356 fast draws, with zero unavailable effects or declined
fast plans. Relative to the normal topology for those **same** instances, this
avoids 20,463,456 submitted triangles across the run; it is not a matched total
frame-performance comparison. Capture1740 was inspected: Fox, world, HUD and
water remain present. Full water/source-image acceptance is still open.

Sparse New memory samples: normal heap49,228,408/71,303,168 bytes, linear
free16,631,168/25,165,824, VRAMfree4,690,944, measured main-stack73,044/524,288;
process commitment103,411,712/130,023,424. Samples do not establish every peak.
The host activity classifier explicitly excludes this run's timing comparison
because unrelated compilation was observed. No FPS improvement is claimed.

Matching normal setup source `1c13d3cffe56`, 3dsx `53d57047def1`, CIA
`486d1ca53a39` is now built locally, **not deployed**. Normal/replay share all615
inventory entries, game archive `c996af790c5a` and18 linked GPU programs.
Old80/New124 profiles, installation recipes and worktree/archive restoration
pass. No fresh normal-app startup or conversion run is claimed; the menu change
has actual C UI/settings coverage and the unchanged startup evidence is reused.
`water-sheet-runtime/detail-build-verification.json` proves static identity only.

The Old-profile route also completes, in 66.37 host seconds, with status1,
state/save/log-drain/owned-exit checks passing. It submits9,622 fast draws /
31,117,548 indices, no normal ripple draws and no declined fast plan. Capture1740
was inspected: Fox, terrain, HUD and water remain visible. This does not establish
full water fidelity. Sparse memory: heap41,691,832/50,331,648 bytes, linear free
1,952,128/10,485,760, VRAM free4,690,944, main-stack73,044/524,288 and process
67,760,128/83,886,080. Old linear headroom is only about1.86MiB in these samples;
do not treat it as permission for unbounded optional allocations.

Both runs are recorded in `ripple-detail-runtime/integration.json`. Both timing
comparisons are excluded due concurrent foreign compilation. All owned builds,
tests and emulator runs are terminal; no new console deployment or performance
acceptance follows. No fresh normal startup/conversion run was performed.
