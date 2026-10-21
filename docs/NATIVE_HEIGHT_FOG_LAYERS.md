# Native height fog: two-texture materials and ramp orientation

The new path uses persistent world geometry and the existing immutable 64 KiB
fog atlas. UV0/UV1 retain the two base texture layers; UV2 samples height fog.
The existing fragment-light distance-fog owner supplies transmission without
another texture coordinate. There is no additional geometry pass, CPU vertex
work, image generation, allocation per draw, or worker.

Material admission is compiled at load. Stage 3 must be a true RGB/alpha
passthrough; occupied stages and special material owners remain excluded.
The additional resident CPU state is 536 bytes (128 eligibility words and a
24-byte stage descriptor/counter). No additional linear RAM or VRAM is used.
The height shader grows from 26 to 36 instructions and uses seven output
registers. Height fog remains optional and off in all four presets.

## Pixel validation caught an existing orientation error

The first integrated candidate, `height-fog-layers-v1`, rendered excessive
foreground haze in the cave. A separate original desktop-renderer run with
the same starting save showed fog in the low tunnel but a clear foreground.
The new candidate was not accepted or deployed on the strength of draw counts.

An asset-free test executes the archived production distance-fog and lighting
owners. Across 32 depths, procedural fog, fragment-light fog and disabled fog
match the exponential transmission reference within 0.80 byte levels. This
rules out that particular distance-fog calculation in Azahar; it does not
establish physical GPU equivalence for every material handoff.

A second test executes the real height-fog atlas owner and shader. Both the
existing single-texture path and the new two-texture path inverted the height
ramp relative to the original texel calculation: near the clear boundary the
output was fogged, and near the fogged boundary it was clear. The old shader's
extra `1-v` on height coordinates is wrong for this atlas. Removing it preserves
ordinary material UVs and corrects both height-fog coordinate outputs.

The original failure reaches 148.2 byte levels of colour error. The corrected
GPU test covers 96 tiles, both sampler routes, eight depths and source selector
states 0, 1, 7, 8, 63 and 64. Its worst error is 1.327 byte levels; surface alpha
is retained. The actual float24 shader test also compares 32,768 vertices and
512 original-source matrices. Earlier CPU tests correctly checked atlas bytes
but did not test their GPU sampling orientation; their flipped-V shader oracle
therefore missed this problem.

Evidence:

- `artifacts/diagnostic/fragment-fog-pixels-v1`: actual distance-fog owners.
- `artifacts/diagnostic/height-fog-pixels-v1`: retained failing orientation.
- `artifacts/diagnostic/height-fog-pixels-uv-v1`: isolated coordinate repair.
- `artifacts/diagnostic/height-fog-pixels-v2`: corrected source selector cases.
- `artifacts/native/height-fog-layers-v1-{new,baseline-new}`: first cave pair.
- `artifacts/desktop/height-fog-layers-cave-reference-v1`: original reference.

The GPU tests are silent, isolated, bounded and asset-free. Their readbacks are
diagnostic only. They follow pinned Melee `blend_verify.h` and `early_queue.c`
at c2ebe7c769b3287f2b3748173e8fe5ec24ef408e: publish modified geometry, finish
GPU transfers, invalidate destination cache, and retire before freeing. No raw
depth readback is introduced. Actual SDK lighting, LUT and texture publication
were inspected; no direct-register workaround was added.

## Acceptance limits

The v1 cave run admitted 25,060 additional world draws with zero unsupported
height-fog draws in that route. This is not whole-game coverage. Actor height
fog and indirect distortion remain absent. First-pair timing comparisons are
invalid because unrelated host compilation was active; no hardware speedup or
performance-budget acceptance follows from those runs.

The corrected v2 replay passes New and Old cave runs in 62.52/61.04 host
seconds, including state assertions, unchanged saves, log drain and owned
process exit. Both 1200/1410 captures were inspected. The inverted upper-wall
fog is corrected, but foreground haze still varies across observations and
does not yet establish full original-image parity. Source phase and remaining
material/state interactions need separation before accepting the complete
effect. The pixel-level orientation repair is narrower than that acceptance.

Normal source `c144f5749e8c`, 3dsx `3449072c4f80`, CIA `e6e34d2e73cf`; replay
source `105c70b50873`, 3dsx `762f490a316f`, CIA `cf6c7e0863df`. These local v2
builds have 621 matching source records and 17 matching GPU programs. Each
game archive remains identical to its parent; Old80/New124 CIA grants and
restoration of the unrelated worktree are checked. Evidence is
`height-fog-layers-test/v2-build-verification.json` and `v2-acceptance.json`.
No redundant normal startup was run for the unchanged setup flow.

New sampled heap48,528,040/71,303,168 bytes, linear free18,461,568/25,165,824;
Old heap40,991,400/50,331,648, linear free3,781,504/10,485,760. Both have
4,723,712 bytes VRAM free after renderer initialization and measured main
stack72,984/524,288. Process commits103,419,904/130,023,424 New and
67,768,320/83,886,080 Old. Startup VRAM-zero placeholders are not measurements.
Both runtime timing comparisons remain excluded due unrelated compilation.

The user subsequently confirmed that rapid whole-entity-shadow flicker is
fixed on hardware. That acceptance is separate from this local fog work; see
SHADOW_FLICKER_20261013.md. Console installations, saves, settings and original
assets have not been changed. No new hardware probe was deployed or requested.

## Material ownership repair, 14 October

A separate source audit found that `fh_pica_height_fog_world_bind(0)` cleared
TEV stage 3 even when the current material was rejected for height fog because
it already used that stage. Also, departing a height-fog overlay could leave
it active when the next draw reused a cached material handle.

The new material-entry handoff retires the overlay and invalidates the compiled
material binding exactly once on departure. Disabled/rejected fog no longer
writes stage 3. Consecutive admitted fog draws retain the existing cache.
A colour-only fog invalidation is not mistaken for removal of the overlay.
This adds no buffers, shader instructions, draw passes, CPU vertices or worker.
The remaining CPU work is an ownership check and a compiled-material rebind
when leaving the overlay; its cost has not been isolated on hardware.

This adapts pinned Melee renderer.c's explicit raster-state invalidation across
owner changes (commit c2ebe7c769b3287f2b3748173e8fe5ec24ef408e) to the native
compiled TEV cache. It does not reintroduce GX interpretation or change queue
retirement, SDK cache coherence, or simulation.

`tools/test_height_fog_handoff.py` executes the actual atlas owner and actual
material-binder entry with mocked SDK calls. It checks 10,000 departures,
40,000 retained fog binds, occupied-stage preservation and toggle-off under
ASan/UBSan. Four mutations fail: missing retirement, clearing source stage 3,
confusing invalidation with restoration, and rebinding every fog draw. This is
not execution of the entire material-binding function.

The local asset-free GPU test `artifacts/diagnostic/height-fog-handoff-pixels-v1`
executes the archived production owner for 96 tiles: both fog routes, source
selectors 0/7/8/64, and a material with occupied stage 3. Maximum pixel error
is 1.119 byte levels. It uses a controlled source material; it does not prove
all gameplay materials or physical GPU behaviour.

Same-candidate New cave runs with particles OFF and ON both finish normally
in 61.97/55.05 host seconds, with state assertions, unchanged saves, drained
logs and owned emulator exit. Inspected frame 1410 shows clear foreground with
particles OFF and animated haze over Fox with particles ON; tunnel height fog
remains with particles OFF. This repeats the particle-dependent distinction
recorded on 12 October, rather than proving a new particle bug. Original/native
particle-phase parity remains unresolved; do not change particle alpha or
remove mist to manufacture a match. The route had no rejected fog materials,
so it is not the regression test for the source-stage corruption above.

These runs are correctness evidence only: foreign compiler activity invalidates
timing comparisons. Normal setup behaviour is unchanged. Paired build hashes,
shared GPU programs, memory samples and exact evidence paths are recorded in
`height-fog-handoff-test/v1-build-verification.json`. These builds are local,
not deployed; no console files, saves, settings or private assets changed.

## Separate world post-material stage, 14 October

The `height-fog-post-stage-v1` replacement moves world height fog to stage 4.
Compiled source materials retain stages 0 through 3. World draws do not use
actor colour fade, so these two owners can share stage 4 with explicit mutual
invalidation on publication. Distance fog remains in stage 5. Actors retain
their existing fade; actor height fog is still missing, and cannot be added
using blended bone positions without matching the original PNMTX0 semantics.

This removes the per-material stage-3 eligibility flag (512 static CPU bytes),
the stage-3 setter wrappers and forced material rebinding on fog departure.
There are no added GPU instructions, allocations, passes, workers or per-frame
vertices. CPU work remains the cached stage-4 ownership check and existing
live fog parameter publication. It follows pinned Melee's raster-state cache
invalidation discipline; SDK submission, coherence and lifetimes are unchanged.
Cloud/ring source precedence and rejected sampler families remain explicit.

The actual owners, material admission and world call order pass ASan/UBSan:
30,000 ownership transitions, 40,000 retained fog draws and 512 admission cases.
Four deliberate faults are rejected, including missing reciprocal invalidation
and clearing fog on every draw. The local GPU test covers 96 tiles with stage 3
occupied in every material, both fog samplers, fog-off transitions and preserved
alpha. Maximum error is 0.697 byte levels against the source texel equation;
the captured RGB image was inspected. No additional console probe was requested.

The New-profile cave replay finishes in 68.84 host seconds with state assertions,
unchanged saves, drained logs and owned exit. Captures 1200 and 1410 retain the
cave, Fox, textures and HUD; animated particle mist varies across the captures.
There are 80,749 fogged world draws including 25,025 layered draws and zero
rejected materials. Fog publications remain 29,315 in this route; fade-stage
publications fall from 4,048 to 3,333 versus the preceding handoff run with the
same world draw count. This is a workload-counter reduction, not an FPS gain.
Foreign compiler activity excludes runtime timing comparison. Newly admitted
stage-3-occupied materials are covered by the synthetic GPU check, not this route.

Sampled memory: heap 48,528,040/71,303,168 B, free linear 18,461,568/25,165,824 B,
free VRAM 4,723,712 B, measured main stack 72,984/524,288 B, process commitment
103,419,904/130,023,424 B. Paired normal/replay builds retain 621 matching source
records, 17 unchanged matching GPU programs and Old80/New124 CIA profiles.
Existing normal setup checks are reused because setup code/recipe are unchanged;
no new conversion or complete gameplay acceptance is claimed.

Evidence: `height-fog-post-stage-test/result.json`, `v1-build-verification.json`,
`artifacts/diagnostic/height-fog-post-stage-v1`, and
`artifacts/native/height-fog-post-stage-v1-new`. See the latest deployment record
for installation status; these checks do not establish physical performance.

