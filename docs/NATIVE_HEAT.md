# Native heat haze, 14 October

Local implementation, not deployed or accepted as whole-game/physical parity.
The normal goal remains incomplete. The rapid whole-shadow flicker reported
by the user is a separate unresolved physical issue; the current SD logs
still predate the installed f2c4 shadow decal-depth repair.

## Source and architecture

`track/intersect_render.c::doHeatEffect` samples the source-timed colour/depth
reflection, animates the 64x64 caustic lookup with the original scroll values,
uses the original camera-pitch alpha gradient and three-stage depth mask, and
tests its z=-8 HUD quad with LESS, without depth writes. The native adapter
consumes these parameters at the same `main/lightmap.c` boundary. Simulation,
fade updates, saves, RNG and progression are unchanged. Native GREATER with
reversed depth and the live HUD projection preserve the source depth test.

The PICA path uses a resident 80x224 grid: 18,225 compact 12-byte vertices and
107,520 u16 indices. Two aligned indexed draws cover the screen. A separate
54-instruction vertex program animates a prepared sine/cosine basis from
four live parameter vectors and the viewport. No geometry shader, per-frame
CPU vertices, GX interpreter, readback, new worker or actor-program growth.
Preparing the basis evaluates each of the 81/225 axis positions once; the
1,224-byte temporary stack arrays produce exactly the former mesh bytes.

Colour and depth reuse the existing ordered GPU capture. Heat alone allocates
no blur-filter scratch target. The shared capture program/quads now have
independent lifetime from that optional scratch target. Pinned Melee
`citro3d_fix.c` descriptor-content caching and ordered EFB copy/SDK framebuffer
publication remain in use. Texture descriptors remain valid until joined
shutdown; switching OFF avoids work but does not free GPU-owned allocations.

`HEAT HAZE` is a persistent, recording-locked graphics option, off in all four
presets. Actual allocation denial marks the option unavailable and invalidates
a replay that requested the missing workload. There is no CPU fallback.

## Fidelity limits

This is an explicit approximation of the source quantized/bilinear IA8
caustic lookup, using its analytic wave with prepared quantized basis values.
It does not claim original math-library bit identity. Coarse meshes failed
the quarter-native-pixel displacement gate; the selected grid passes
4,608,000 sampled pixels over 48 scroll phases, including near-wrap values:
maximum 0.249808 pixels, p99 0.165295. This is a finite audit, not a universal
error bound. Geometry/reference bytes remain private; no game assets changed.

Source activation includes Volcano Force Point and Boss Drakor. An unrelated
cave/Thorntail run with zero heat requests is only regression coverage.
Original-game heat images, affected gameplay and physical GPU cost remain
unvalidated. Do not equate an enabled toggle or a nonblank frame with coverage.

## Checks and budgets

- Actual Azahar float24 shader: 291,600 prepared vertices, all 16 output lanes,
  pitch quadrants and both HUD viewport shapes. Maximum arithmetic difference
  1.57e-7; existing scene shaders unchanged.
- Actual original/native material equations: 589,824 threshold/depth/alpha
  cases under ASan/UBSan; floating error below 4e-15. Estimated fixed-point
  rounding is separate from GPU pixel evidence.
- Actual native GPU owners: eight cases covering OFF, stale capture,
  OFF/ON reseeding, foreground occlusion, sampler/target handoffs and a real
  low-linear-memory rejection followed by retry. No blur scratch allocated
  for heat. Flat colour errors below 0.38/255.
- Spatial GPU capture: 6,936 edge pixels, maximum native interpolation error
  1.70/255; wrong displacement direction fails at 187/255. Relative to the
  source per-pixel lookup, the high-contrast synthetic edge can differ by
  28.07/255 due to the documented subpixel approximation.
- Shared motion/blur owners: actual GPU VRAM/linear denial and temporal
  regressions pass; host ASan/UBSan 35,600 cases/100 lifecycles pass, missing
  split and disabled-work mutants fail.
- All presets keep heat OFF; menu bounds, toggles, recording lock, replay
  masks and unchanged-panel no-flush checks pass.

Incremental heat storage is 433,792 linear bytes including alignment; history
adds 524,288 VRAM bytes and shared quads 1,024 linear bytes if not already
resident. The blur scratch is an additional 524,288 VRAM bytes only when blur
is enabled. Keep the existing 1 MiB VRAM and 15% linear admission reserves.
Budget heat submission at <=0.20 ms New / <=0.50 ms Old, and GPU at <=1.5 ms
New as an initial restoration gate. Those are targets, not hardware results.
The isolated emulator's nested CPU submission is about 0.04 ms/effect call;
one-time preparation fell from 55.13 to 2.20 ms with byte-identical output.
Preparation can occur on first use after enabling; no physical stall bound
has been established. Full-frame and real GPU budgets remain open.

Evidence: `heat-{grid,plan,shader,material}-test`, `heat-basis-reuse-test`,
`artifacts/native-heat-device/native-heat-v{1,2-admission,3-prepared}`, and
`native-motion-blur-device/heat-shared-capture-regression-v2`.

Failure history: the first full build named the SDK material as a generic
`pica_` C unit, causing the clang game compiler to miss `citro3d.h`; renaming
it `platform_pica_heat_material.c` selects the established platform compiler.
The first regression rerun exposed stale test assumptions: an OFF mutant
matched the old two-option gate, and a blur-denial check expected shared
capture storage to be freed. Both checks now explicitly assert the new
gate/layout; production guards were not weakened.

## Integrated builds and runtime

`native-heat-v3-console-replay-builds`: source c06e17e142a8,
3dsx15826f54d486 / CIAed257f3c6bcb.
`native-heat-v3-console-setup-builds`: source266ed5530385,
3dsx819b17ce46f3 / CIA808b3789ec56. Both are local, **not deployed**.
The 596 source inventories and game archive9d521d51f66e are identical;
the exact VM-tested heat shader is fcbdf66e9dc0 (54 instructions).
All previous linked shader bytes and Old80/New124 profiles are preserved.
Installation recipes are unchanged; no new full conversion or normal-app
menu runtime is claimed for this binary. Snapshot/archive restoration passed.

The ordinary-input save-load/walk/dialogue route passed serially on New and
Old in 73.64/62.56 host seconds. Both status1, logs drained, owned emulator
exited, copied saves unchanged, screenshots1800 inspected. These routes
request the existing depth blur (160/142 requests), **zero heat draws**.
Unrelated host compilation excludes speed attribution; original runner
summaries are retained before the explicit timing qualification. No console
FPS or thermal-scene acceptance follows from these runs.

Last sampled memory, not whole-game continuous peaks:

| Resource | New | Old |
| --- | ---: | ---: |
| Normal heap used / reserved | 49,264,080 / 71,303,168 | 41,727,416 / 50,331,648 |
| Linear free / reserved | 16,731,904 / 25,165,824 | 2,060,032 / 10,485,760 |
| VRAM free | 3,675,136 | 3,675,136 |
| Main stack measured / reserved | 73,044 / 524,288 | 73,044 / 524,288 |
| Process committed / granted | 103,354,368 / 130,023,424 | 67,702,784 / 83,886,080 |

Heat mesh storage was not allocated in these inactive-effect routes. Its
433,792-byte residency and actual reserve denial are separately tested by
the synthetic GPU fixture; do not present these game figures as its active
whole-scene memory peak. Old headroom is narrow and later-area admission/
transitions need affected-scene validation before default enablement.

Reproduction helpers: `build_native_heat.py` isolates archived sources;
normal `--game-from` reuses the exact tested replay game archive after
matching headers/adapters/generated scene source. `run_native_heat.py`
reuses ordinary waypoints and records host compiler observations.
`native-heat-test/{build-verification,runtime-regression}.json` records scope.
