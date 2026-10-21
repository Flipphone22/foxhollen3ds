# Native precipitation

11 October later integration and New223 normal deployment: `lightning-v1-builds`
include this weather path in both normal and replay applications, alongside
the current shadow/fur fixes. Rain is visibly present in the opening flight
captures. See [NATIVE_LIGHTNING.md](NATIVE_LIGHTNING.md) for exact builds, tests,
memory and remaining limitations. WEATHER still defaults OFF on both models.
Normal `86b336c4bfa5` is deployed; Old and replay installations are unchanged.
The following records the earlier standalone weather milestone.

11 October: replay `194b2b6ce0fa…` restores source precipitation through a new
PICA vertex path. **No normal weather build or physical acceptance yet.** The
weather setting is opt-in/OFF on both models while budget/visibility validation
continues. This does not fix the C-stick background or finite shadow receivers.

The earlier adapter ran `snowPrintSnowCloud` motion but omitted its triangles
under the stripped rendering path. `tools/pica_weather_override.py` now supplies
the original centers, scales, twenty triangle templates, texture selection,
UVs and tint to a bounded native owner. Motion, spawn, drift and RNG remain the
original game work even with WEATHER off. No GX callback interpreter is restored.

`platform_pica_weather.c` uses a 512-byte immutable linear vertex-ID buffer and
live uniforms. PICA constructs and transforms the three corners. The general
26-instruction shader admits eleven flakes per batch. When all twenty source
templates are byte-identical, an exact specialization admits eighty flakes in
23 instructions. Template equality is checked once per cloud; source order and
texture-page boundaries are preserved. No CPU-expanded triangles, geometry
shader, additional render targets or worker threads are used. Existing resident
HUD image ownership supplies the source texture; ordinary distance fog is kept.

The main 511-instruction program is unchanged. Both separate weather programs
use 96 float registers and zero booleans. Unrelated native-image VRAM WIP is
isolated by `build-weather-v3.py` and restored in `finally`, as is the game
archive. Never blind-build those six WIP files. Latest checked weather source
snapshot/archive: `artifacts/research/pica-scene-rewrite/weather-v3-builds`.
Shared CIA Old 80/New 124 MiB metadata is verified; Old runtime is separate.

The pinned Melee persistent-binding invalidation and SDK uniform/command
publication informed ownership: each shader/command-buffer handoff republishes
its state, the ID buffer is flushed once, and the parent retires GPU work before
freeing it. Disabled, empty and entirely clipped pools do not invalidate the
scene. Remaining CPU work is unchanged flake motion, source cloud template
preparation, bounded admission, page batching and uniform submission.

Evidence under `artifacts/research/pica-scene-rewrite/`:

- `weather-source-test`: 90,000 exact source corner coordinates, 10,000 unchanged
  motion states; three original simulation functions byte-identical; wrong-height
  mutant rejected, ASan/UBSan clean.
- `weather-owner-test`: actual owner, 100 lifecycles, 10,000 clouds and 3,187,800
  vertices; generic/shared batches, toggles, command splits, no per-draw allocations,
  publication and retirement. Missing-flush mutant rejected, ASan/UBSan clean.
- `weather-shader-test-v3` / `weather-shared-shader-test`: actual Azahar float24
  interpreter, 33,000 / 240,000 vertices; maximum sampled coordinate/output
  errors `1.19e-7` / `5.96e-8`. Wrong shape-selection mutants rejected.
- `weather-runtime-test/build-verification.json`: actual linked general/shared
  shader bytes match those tests; main shader unchanged; archive and CIA grants
  metadata checked. Game archive `856637469f53…`.
- Graphics option tests pass persistence, locking, touch bounds and idle behavior.

Initial v1 compile errors (missing image header and misleading indentation) were
corrected without weakening compiler checks. V2 restored precipitation but made
20,070 draws in the short palace route. V3 emits **3,345 draws for the same
214,749 triangles**, with all 1,338 clouds eligible for the exact shared shape.
Both ON captures at retrace 1410 were inspected: rain, world, Krystal, enemies
and HUD remain visible. Old V3 ON also passes state/save/drain/owned-exit checks
(45.80 host seconds), with image 1410 inspected and precipitation/world/actors/
HUD present. Full snow/heavy weather/source-image acceptance is open.

V2 ON/OFF combined scene+submission CPU delta was approximately +0.628 ms in
Azahar. V3 ON reduces that to approximately +0.239 ms against the earlier V2
OFF; **this is not a same-binary comparison** because disabled invalidation was
also removed. The matching V3 New OFF/ON pair measures scene 1.868667/1.970857
ms and submission 3.993986/4.131750 ms, **+0.239954 ms combined CPU**. Both
approximately 59.831 Azahar FPS, state/save/drain/owned-exit checks pass. Source
animation phases are not instruction-identical. These figures are not hardware
estimates. Provisional added CPU
gates are 0.25 ms New / 0.50 ms Old, with command/GPU work assessed separately.

V3 New ON sampled heap peak 48,472,048 / 71,303,168 B; minimum free linear
18,369,664 / 25,165,824 B; free VRAM 4,822,016 B; measured stack 72,996 /
524,288 B; process 103,239,680 / 130,023,424 B. Old ON: 58.469 Azahar FPS,
scene 1.945600 ms, submit 4.149956 ms; heap 40,936,376 / 50,331,648 B,
minimum free linear 3,661,568 / 10,485,760 B; free VRAM 4,822,016 B;
stack 73,008 / 524,288 B; process 67,588,096 / 83,886,080 B. There is no
paired Old OFF run, so no accepted Old added-cost gate or physical FPS claim.
No console files have changed.
Rendering parity, physical cost, snow coverage, and the overall gameplay/performance
goal remain incomplete.
