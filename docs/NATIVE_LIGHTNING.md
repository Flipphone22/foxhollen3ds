# Native lightning and opening-cutscene repair

11 October, New223 normal deployment COMPLETE, physical validation pending. Normal
`86b336c4bfa5` / CIA `01f3aa780117`; replay `e1a844c319b4` / CIA `02a897f695ad`.
Transfer `console-settings-update/20261011-114316-223` staged and byte-readback
verified changed payloads before rename/final sizes. Saves and preferences are
unchanged; original geometry retains prior completed byte proof plus current
metadata/sizes, rather than another full readback. Old and replay installations
are unchanged. New `.3dsx` is ready at `SD:/3ds/foxhollow/foxhollow.3dsx`; the
sibling `foxhollow.cia` requires FBI installation to update the HOME icon.
Enable WEATHER for rain; PARTICLES enables bolts. Both settings remain user-owned.

The user clarified that missing “lighting” in Krystal's opening means lightning
flashes and bolts. Ordinary lighting and fur remain separate outstanding work.

The original engine-7 lightning code generated endpoints and branches, but its
GX line submissions had no native consumer. `pica_lightning_override.py` replaces
only rendering setup and endpoint emission. The original branch recursion,
jitter, RNG restoration, timing, colour and fade remain. Existing cloud glare
still consumes the same source lightning state.

The new owner keeps 1,152 aligned linear bytes of immutable vertex IDs and
indices, with forty line segments per batch. Live uniforms supply endpoints,
width, view/projection and colour. A separate 66-instruction vertex shader
clips against the near plane, computes screen-space width and expands corners
on PICA. The original generated I8 beam texture supplies coverage; blending is
source-alpha plus destination, with depth testing and no depth writes. PARTICLES
controls rendering while the original game state continues. No CPU corner
expansion, geometry shader, extra target, new worker or per-draw allocation.

The pinned Melee `native-3ds/port/3ds/citro3d_fix.c` binding discipline and existing
SDK ownership informed this integration. The immutable buffer is flushed once,
shader changes invalidate shared bindings, command splits cause full publication,
and parent GPU retirement precedes freeing resources. Existing verified SDK
submission is reused; no private GPU queue or direct register writer was added.

Remaining CPU work is original endpoint/branch generation, one combined matrix
per bolt, bounded endpoint copies and uniform submission. Provisional added CPU
budgets are 0.20 ms New / 0.50 ms Old per active bolt call; worst concurrent bolts,
GPU time and physical performance require separate measurement. Shader storage
limits are not runtime instruction counts or a hardware speed prediction.

Evidence under `artifacts/research/pica-scene-rewrite/`:

- `lightning-source-test`: actual generated adapters preserve source recursion,
  lifetime, creation, endpoint arithmetic, RNG and generated texture data.
- `lightning-owner-test`: actual owner, ASan/UBSan, 100 lifecycles and 10,000 bolts,
  15,206,400 submitted indices; batching, strand boundaries, immutable buffers,
  toggles, command-split publication and retirement. Missing flush/retirement
  mutants fail.
- `lightning-shader-test`: 48,000 vertices, including 3,640 clipped segments,
  all forty uniform slots, degenerate lines, width, RGBA and UV; maximum sampled
  error 3.81469727e-6. A wrong-width mutant fails. Azahar's interpreter uses float32
  inside its float24 representation: this is not physical precision proof.
- `lightning-v1-builds/verification.json`: normal/replay native sources, linked
  shader bytes and game archive match; existing shader blobs are unchanged;
  shared CIA metadata requests Old80/New124 MiB.

The New-profile opening/combat-prefix replay completed normally at retrace8587:
state checks, unchanged saves, drained logs and owned-process cleanup passed.
1,060 bolt calls / 5,076 batches / 181,824 segments / zero invalid endpoints.
Direct owner submission took112.40 ms across the entire run, about0.106 ms per
bolt call. This overlaps native submission telemetry and must not be added again.
The prior rectangle/rain build took225.60 host seconds; this run226.83 seconds,
but animation workloads and capture cadence differ. Neither establishes a
performance regression or speedup. Physical performance remains unmeasured.
Rain, cloud background, Krystal/Kyte and the ship fight are visible in inspected
New captures3840/5640/7440. Subsequent targeted Old captures2501/2747/3201 show
branching blue-white bolts, bright cloud glare, rain and Krystal/Kyte. This
establishes that the missing bolts are visible in the emulator; original-image
fidelity and physical GPU acceptance remain separate requirements.

New-profile sampled peak normal heap48,808,376/71,303,168 B; minimum free linear
16,807,168/25,165,824 B; free VRAM4,822,016 B; measured stack72,956/524,288 B;
process103,260,160/130,023,424 B. The fixed buffer is not the entire program's
memory delta: linked code, BSS and shader resources also count.

Old80-profile opening also passed state/save/drain/owned-exit checks at retrace8572.
970 bolt calls / 4,478 batches / 160,426 segments, zero invalid endpoints.
Heap41,271,760/50,331,648 B; minimum free linear2,127,104/10,485,760 B; free
VRAM4,822,016 B; measured stack72,956/524,288 B; process67,608,576/83,886,080 B.
`capture_native_lightning.py` read only the verified owned guest process and
captured eight real strikes, without writes, injected effects, pause or altered
inputs. Those observations perturb host timing: exclude this Old run from timing
comparisons despite the runner's base summary not knowing about the sidecar.
`runtime.json` records this exclusion and measured memory extrema. Stack reporting
explicitly excludes unmeasured startup reservations from measured usage.

## Top-left rectangle

Original `track/intersect_render.c::hudDrawRect` emits coordinates multiplied by
four. Its native adapter omitted that conversion even though the native HUD
shader consumes quarter-pixel coordinates. A full-screen transition therefore
covered only the top-left quarter of each dimension. `pica_hud_override.py` now
performs the original signed16 wrapping conversion without signed-shift UB.
There are no extra draws, allocations or shader instructions.

`hud-rect-source-test` compares36,867 cases against the actual original position
statements, including full-screen, clipped negative and wrapping coordinates,
alpha and scissor. ASan/UBSan pass; the former unscaled implementation fails.
`weather-rect-v1-opening-new` completes with state/save/drain checks. The exact
reported transition has not yet received a matched hardware image check.

Rain is integrated from the previously checked native weather owner. WEATHER
remains opt-in/OFF on both models. The cloud background's full resemblance,
lightning glare/bolt appearance, fur wind/projected lighting and wider rendering
parity remain open; this document does not claim all visual bugs are fixed.


## 17 October discrete phase rotation

`lightning_phase_patch.py` replaces the three source rotation calls in bolt and
strand generation with a lookup for the original 1,001 integer phases. The table
contains 8,008 bytes of pure math constants generated using ARM libm; it contains
no game assets. It retains the exact normalization and matrix arithmetic. Signed
out-of-range phases retain the original function as a guarded fallback.

`test_lightning_phase.py` compiles the source matrix/normalization and candidate
with the ARM game compiler settings, then executes both on ARM11 emulation.
All 8,024 matrices match bit-for-bit (1,001 phases plus two fallback phases,
eight axes). A table sine-sign corruption is rejected at the expected phase.
The synthetic one-axis full-phase sweep counts 164,330 versus 84,084 instructions
inside the rotation calls. This is not an FPS, cycle or whole-frame claim.

The native-adapter source proof restores exactly the three call replacements
and reproduces the parent source modulo whitespace. RNG calls, recursive branch
logic, endpoints outside the rotation and render-state handoffs are preserved.
Do not simply skip bolt generation when particles are disabled: the original
strand code updates the saved RNG seed through its pointer, which affects the
state restored by the caller. Such a separate optimization needs a proof that
preserves that state transition.

The isolated `unified-lightning-phase-v1` build is based on the NDSP lifecycle
repair `3e9816956759`. Of 796 ordered archive members, only `7.o` and the archive
symbol index change. All GPU programs are unchanged; Old80/New124 profiles and
root/archive restoration pass. The exact linked table is read-only and matches
the ARM-generated bytes. Runtime acceptance is recorded separately under
`lightning-phase-matrix-test`; do not infer a hardware speedup or audio crash fix.
