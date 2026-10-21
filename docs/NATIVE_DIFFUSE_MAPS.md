# Native diffuse-map reconstruction

Latest local revision: [active-light gates](NATIVE_DIFFUSE_ACTIVE_LIGHTS.md).
The map shader now stores81 instructions and8 Boolean gates, avoiding inactive
light work. Exact linked outputs remain unchanged. The updated full material
family still exceeds its New CPU budget; not deployed or default-enabled.
The73-instruction/no-Boolean description below records the earlier implementation.

Latest extension: [native masked/layered families](NATIVE_DIFFUSE_FAMILIES.md)
now compiles more material families and shares compatible pipelines. V4 is
built locally; New's added CPU.297975ms exceeds the.20ms provisional gate.
No new deployment or physical/complete visual acceptance. The bounded-reuse
V5 experiment was reverted; current map generation remains exact-update V4.

11 October: native maps are integrated locally for single-layer diffuse
materials, including untextured materials. `LIGHT MAPS` is on page10 beside
`SOURCE FUR`, opt-in OFF on both systems. Nothing was deployed. This does not
establish a repair for white Arwing, disappearing trees or complete day/night
lighting/shadow fidelity.

The source adapter snapshots active mode-0 slots before the original group
reset, preserving source light selection, ambient bytes and model-relative
positions. Source simulation/light objects remain unchanged. The first material
route copies existing load-compiled stage descriptions, maps the environment
input to texture0 and base to texture1, and reuses native sphere coordinates.
It adds no actor pass, geometry copy, shader instructions or per-draw material
interpretation. Masks/layered materials and simultaneous C2/specular still use
the existing analytic lighting; they are unfinished, not claimed map parity.
Fur's independent material path is also unchanged.

Replay`f1bb348b9800` passes serial New ON/OFF and Old ON/OFF palace-facing checks.
Normal`56340ab2a26a` is built with the same game archive,329 native sources and
exact511/214/197/73-instruction shader programs. CIA profiles remain Old80/New124.
New ON has1306 GPU map updates/955 reuses and27286 map-lit draws, zero
unready samplers.47 load declarations are admitted and8 partial; these totals
include menu assets and do not mean every draw uses the map when C2 is active.
New ON/OFF scene1.912754/1.857219ms and submit4.077840/4.059842ms give about
.07353ms added CPU, within the provisional.20ms gate. Both~59.83 AzaharFPS.
Animation phases are not identical and console throughput remains unproven.

Old ON/OFF scene1.860813/1.835676ms and submit4.097572/4.084391ms add.038318ms
CPU, below the provisional.50ms gate. Old AzaharFPS58.9635/58.7499 is **not a
speedup**: actor/animation work differs. Old ON records1131 updates/1138 reuses,
28134 map-lit draws and zero unready samplers. New and Old1410 captures were
inspected; world, Krystal, creatures/glows and HUD are present. All four checks
pass state assertions, save preservation, log drain and owned-process cleanup.

Measured memory (New ON / Old ON): normal heap48472720/71303168 and
40936928/50331648B; minimum free linear18396416/25165824 and3688320/10485760B;
minimum free VRAM4822016B; measured stack72796/reserved524288B; process commits
103206912/130023424 and67555328/83886080B. These are this scene's measurements,
not a guarantee for every area. Map allocations are present even OFF.
`diffuse-map-runtime-test/runtime-comparison.json` and`build-verification.json`
record the complete evidence. The new sampler correction is also built, but
the earlier2327-image audit shows it changes no stored texture output.

The matching normal app completes its isolated24.85-second startup/toggle/Select
check,721 retraces, no native test hooks, clean log drain/owned exit. The actual
bottom-screen `LIGHT MAPS` ON/OFF/ON control is readable in inspected540 capture;
saved FHGRAPH4 mask8404993 and checksum are verified. Only this isolated test
profile has the option saved ON. Physical consoles/preferences were untouched.
The normal builder's inherited console-replay print label is cosmetic: the
verified build variant is`console`. No duplicate normal boot was run.

Actual material compiler/SDK tests cover303104 RGBA comparisons including C1
clamp, base intensity and constant opacity; wrong-base-unit mutation fails.
Actual game publisher/source GX comparisons cover12300 light coordinates and
colours, selection masks, disabled options and unchanged original light state.
ASan/UBSan caught two implementation errors before emulator validation: treating
16-bit SDK descriptor fields as32-bit pointers, and `maps+5-i` forming an
out-of-bounds intermediate pointer. Both are corrected. V1 also failed a missing
source-header include; V2 built but was not run after the pointer check; V3 is
the exercised build. Graphics menu/settings tests cover the new bit, lock,
persistence, bounds and unchanged-screen no-flush behavior.

The existing environment path uses analytic PICA fragment lighting in place of
the original 32x32 mode-0 RCP maps. The new path reproduces those maps with the
original immutable 289-vertex/512-triangle hemisphere mesh. It keeps the original
unnormalized border normals, source directional-light positions (including model
view offsets), per-channel accumulation and final saturation. With no selected
lights, the original disabled GX colour channel passes white material, rather
than ambient alone. Input selection remains source mask4, excluding point/spot
lights; arbitrary attenuation is not implemented by this specialized kernel.

`pica_diffuse_map.v.pica` is a separate73-instruction program (18 float registers,
no booleans, two outputs). It does not expand the511-instruction actor program.
`platform_pica_diffuse_map.c` owns three32x32RGBA8 VRAM targets and prepares the
existing8852-byte source mesh once. Actual allocator use is8960 linear bytes and
12288 VRAM bytes, plus owner metadata/shader state. Preparation guards retain15%
linear space and1MiB VRAM. Preparation now occurs at game initialization even
when the option is OFF; this resident cost is explicit. Changed-map rendering
and snapshot calculation occur only when the option and relevant render,
actor, texture and lighting switches permit it.

Updates upload small live light vectors and draw each changed target once.
Unchanged states reuse their existing map. There are no per-update allocations,
CPU vertex reconstruction, CPU image generation, gameplay readbacks or new
workers. The original main auxiliary owner retains command submission, explicit
`C3D_FrameSplit(GX_CMDLIST_FLUSH)` publication and state restoration. Shutdown
requires its completion fence. This follows the pinned Melee persistent-resource
and explicit queue/cache ownership approach and the already tested native
specular target owner. No private command queue or raw texture-copy workaround
was introduced.

Validation:

- `tools/test_pica_diffuse_map_shader.py` executes the actual assembled shader in
  Azahar's float24 interpreter.332928 source-mesh vertices cover0..8 lights,
  actual extracted directional GX publication, moving directions, model/view
  offsets and372404 saturated channels. Maximum float error0.00007385. Missing
  saturation and wrongly normalized border mutants fail. A two-unit depth
  change at100000-unit light distance is beneath the8-bit gate and is explicitly
  **not detected**; the original depth remains implemented.
- `tools/build_diffuse_map_gpu_probe.py` builds the actual ARM resource owner.
  `tools/run_diffuse_map_gpu_probe.py --label <unique-label>` runs it silently in
  isolated Old-mode Azahar with no game assets or saves. Three states across
  three targets produce7 draws/10752 indices,9002 unchanged-state reuses, exact
  resource reclamation after GPU completion and automatic exit. CPU readback
  exists only in this diagnostic application.
- `artifacts/diagnostic/diffuse-map-native-v1/summary.json` retains the initial
  failed pixel oracle: display-transfer rows were assumed bottom-origin. The
  actual transfer is top-origin. `pixel-validation.json` rechecks the same saved
  bytes with corrected orientation, without another emulator run. Nine maps
  pass, maximum0.5011 of an8-bit colour level; all alpha bytes255. Actual and
  corrected expected images were inspected. The owner and GPU code were not
  changed to satisfy the corrected oracle.
- Shader report: `artifacts/research/pica-scene-rewrite/diffuse-map-shader-test/`.
  Probe SHA256 starts`bc4e0f5e1625`. No physical timing or hardware claim.
- A subsequent substantive handoff test adds a real PICA texture consumer in
  the same queue, before any CPU join. `diffuse-map-native-handoff-v2` passes:
  all three changed maps sample byte-identically after target/program changes;
  all nine source-grid pixel comparisons still pass, resource retirement is
  exact and the owned emulator exits. Probe`f8b2ee4c3c41`;3.24 host seconds is
  test duration, not an in-game frame measurement. The scene/material consumer
  remains unconnected; this verifies the isolated queue publication mechanism.

First attempts also caught an illegal MAD operand position and an ARM printf
type mismatch under`-Werror`; both were corrected before the probe build.

Remaining integration: extend mask and layered/specular material families while
retaining fog/transparency; compare original images and measure physical GPU
costs. Native headless captures retain world, Krystal, purple creatures and HUD,
but are not full original-image acceptance. Do not quietly route unsupported
families through an incomplete material or call this whole-game parity.

The first integration cost gates are provisional: incremental CPU preparation
and submission <=0.20ms New /0.50ms Old, separately measured GPU work and command
pressure, explicit linear/normal/VRAM headroom. These are budget allocations,
not measured costs or console FPS estimates. Avoid stacking further effects if
the real-game path exceeds its budget.
