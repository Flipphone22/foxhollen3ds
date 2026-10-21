10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native water fog integration (local)

The native splash and droplet batches now retain the scene distance-fog effect.
The original `waterfx_setupSplashDropPointRender` and `setupWaterReflectionTev`
change material/raster state without disabling GX scene fog. The initial native
versions explicitly disabled it. Heavy-fog ambient selection for rings remains
separate; this change does not restore reflection, indirect distortion or wakes.

Both existing vertex entries output linear eye depth through texture coordinate
2. The existing procedural alpha lookup and TEV stage 5 combine fog colour while
preserving the material alpha. No additional geometry pass, CPU vertex work,
worker, image allocation or readback is introduced. The shared program grows
from 503 to 507 instructions, below the PICA limit of 512. Float uniform 94 is
shared with the scene fog owner; both effect entries validate that ABI at startup.
Uniform padding only reserves register numbers and adds no executable work.

The original star entry shares the point shader. Its existing outputs remain
unchanged, and its materials continue to disable the fog combiner. Water callers
invalidate the fog owner after resetting their material stages, then bind fog.
This matters when the previous draw already had fog enabled: retaining its cached
state after erasing stage 5 would silently lose the effect. Scene reentry retains
its existing uniform/material publication. Reviewed pinned Melee `renderer.c`
command ownership and explicit cache publication; the procedural-fog owner is a
Foxhollow adaptation, not a copied Melee lighting implementation. No changes to
resource lifetime, submission fences or the GS handoff fix.

Controls remain SPLASHES, WATER DROPS and DISTANCE FOG; another redundant control
is unnecessary. Both water geometry options remain OFF by default on Old and New
pending visual/hardware acceptance. The fog repair adds no persistent memory.

Validation:

- `tools/test_pica_water_fog_shader.py` runs the exact archived linked shaders
  in Azahar's float24 interpreter. 30,720 droplets, 30,720 ordinary stars and
  32,768 splash vertices retain bit-identical prior position/colour/UV outputs.
  New water fog coordinates agree with linear eye depth. This does not prove
  rasterization, clipping or physical timing.
- `tools/test_pica_fog.py` executes the real fog owner, pinned LUT encoding and
  extracted water stage-reset/owner operations: 400 ON/OFF handoffs, 10,000
  cached draws, 200,020 source-transmission comparisons. A mutant removing water
  invalidation is rejected. Lookup approximation error remains documented.
- Shared replay `58a29afdb3dd` uses the same game archive as `203bd9e04e3e`.
  The six unrelated VRAM candidate files are isolated and restored during builds.
  Matching normal build `56de63e56c34` is archived in `water-fog-normal-v1-builds`.

Both serial exact-CIA-content route checks pass state assertions, save
preservation, drained logs and owned exit. New takes 63.63 host seconds, Old
62.13. New captures 1740/1800 and Old 1800 were inspected: environment, Fox,
HUD and the active New splash ring remain. These nearby effects do not establish
far-distance fog or full droplet visibility acceptance.

| Azahar profile | Before FPS | After FPS | Before / after native submission ms |
|---|---:|---:|---:|
| New | 56.40914 | 55.53631 | 3.62916 / 3.61865 |
| Old | 48.81030 | 50.41687 | 3.60507 / 3.49074 |

The ordinary-input route has timing-dependent animation/audio/effect counts.
Old's higher rate is not a demonstrated speedup; these are not physical forecasts.
Direct active-droplet batch cost changes from about 0.03078 to 0.03219 ms New,
0.03078 to 0.03190 Old. Splash object submission changes from 0.03702 to
0.03802 ms New, 0.03727 to 0.03905 Old, below the provisional 0.1 ms CPU budget.
Physical GPU cost remains unmeasured. The new ordinary build shares the tested
renderer/game archive; unchanged normal menus were not booted again.

Peak heaps are New48,755,408/71,303,168 and Old41,431,848/50,331,648 bytes.
Minimum free linear is18,330,112/25,165,824 and3,654,144/10,485,760;
both VRAM-free4,858,880; measured main stack72,908/reserved524,288.
Process commitment103,149,568/130,023,424 and67,497,984/83,886,080.
Small allocation differences follow the changed workload, not a memory optimization.
Evidence: `water-fog-test/comparison.json`, `water-fog-shader-test/result.json`,
`distance-fog-tests/result.json`, and `artifacts/native/water-fog-v1-on-{new,old}`.

No console upload, physical FPS claim or complete water-fidelity acceptance.
The earlier droplet visibility/occlusion question remains open. Small points and
different animation phases are not accepted solely from nonblank screenshots.
