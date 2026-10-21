11 October New223 deployment COMPLETE: normal `f50343cd2e56` / CIA
`82d3285f6677`, replay `e8eefd5944cf` / CIA `72ee1193db8b`. Layered
secondary-light materials and reduced native memory-log traffic are installed.
Eight changed application/identity files received full staged byte readback,
rename and final-size verification. Ten unchanged original asset/cache files
reuse prior completed proof plus current descriptors/topology/sizes; they were
not bulk-reread. Saves/preferences preserved. Old229 unchanged. Matching3dsx
ready; CIA requires FBI installation. SPECULAR remains opt-in; existing saved
options are preserved. No physical stall, speed or whole-parity claim.
Evidence: `artifacts/console-settings-update/20261010-202815-223/manifest.json`.

# Layered character-material work, 11 October

## Integrated candidate (not deployed)

Normal `6118a2078f78` / CIA `b996f8fd8862` and replay `ce4a25e5c902` /
CIA `fe53b531eea3` now include the common pre-light two-layer secondary-light
family. The existing SPECULAR option controls it and remains OFF by default.
Both original UV sets, animated frame selections and scroll parameters feed
three independent GPU samplers. Source mode/alpha/mask checks run during
material loading; four compiled texture-combiner stages replace the previously
omitted secondary contribution. Unsupported families remain explicit omissions.

A separate 197-instruction shader retains GPU skinning and vertex lighting,
repurposes uniform c7 for the second base layer, and supplies normalized view
distance to the fragment lighting unit. The existing 511-instruction combined
program and 214-instruction fur program are byte-identical. Program changes
invalidate material state and force camera, format and bone publication.
Ordinary bindings restore texture coordinates, RGB/alpha TEV buffers and fog.
No per-frame vertices, GX interpretation, additional character pass or worker.

The existing fragment-light owner reserves slot7 only when no original light
would be lost. One shared Citro3D environment restores all ordinary light
parameters on handoff. Eight occupied source lights reject the layered route;
this edge case still lacks that secondary contribution, not a silently removed
source light. A 256-entry distance-attenuation curve is prepared at init, then
only changed range parameters are uploaded. Fragment secondary RGB carries fog,
freeing UV2 for the second base image. Previous vertex-fog/W-buffer prototypes
below are historical alternatives, not this implementation.

Half-map storage is now requested at game initialization: 12288B VRAM and92B
linear, even while SPECULAR is OFF. The fragment-fog owner adds1056B of CPU
state. The shader program itself also adds resident code/metadata, accounted
by measured process/heap totals. Half-map GPU draws occur only for updated maps
while the source specular route runs. These are extra auxiliary GPU draws, not
free work; physical cost is unvalidated.

The additional New palace-facing ON/OFF pair also passes state, saves, log
drain and owned exit. Krystal model1 now reports five admitted secondary-light
declarations and zero partial declarations; this is compiler coverage, not
full visual fidelity. ON exercises1541 layered draws, zero unready samplers.
Retrace1410 inspected: world, Krystal, enemies, glow and HUD retained. ON scene/
submit1.824176/4.217762ms versus OFF1.840886/4.046146ms: added0.154907ms CPU,
within the0.20ms gate. Both paced59.83122 Azahar FPS. Heap48470104/71303168,
linear free18703104/25165824, VRAM free4834304, measured stack72796/reserved
524288; process103206912/130023424. Palace images do not expose Krystal's face
well enough to claim eye-image parity. Reports: `layered-palace-runtime-test/`.

The first intended palace check (`layered-material-v1-ordered-on-new`) selected
the cockpit instead because its suite contained both cases. It completed
normally but is excluded from palace/performance acceptance. Corrected helpers
use separate cockpit/palace suites; original evidence is retained.

Actual source/compiler checks retain1671168 single/masked RGB cases and add12
layered descriptor comparisons. Actual animated binding checks cover16 independent
layer-A/layer-B frame pairs and ordinary restoration. The shared fog owner
passes30300 layered binds,300 ordinary/layered/toggle cycles and its existing
200020 source comparisons. Fragment-light ownership passes26700 cached layered
draws,300 switches, range changes, dark slots and eight-light rejection.
The linked 197-instruction blob matches the prior32768-vertex float24 test.
Normal/replay share324 native sources, all three shaders and game archive
`7439906123a225af4a696c711ea2278b8f3c2d43940823d951ec40fbfa0b1b81`.
Shared CIA Old80/New124 profiles are verified.

Four serial cockpit checks pass their state assertions, preserve isolated
saves, drain logs and exit. New/Old ON retrace900 images were inspected: Fox,
his eyes, cockpit, menu and HUD remain visible. ON recorded788/789 layered
draws, with no unready specular samplers. This is live-path correctness coverage,
not full source-image parity or acceptance of every affected model.

| Azahar profile | OFF scene / submit ms | ON scene / submit ms | Added CPU ms | ON FPS |
|---|---:|---:|---:|---:|
| New | 0.717375 / 1.521914 | 0.728026 / 1.647423 | 0.136160 | 59.83193 |
| Old | 0.716761 / 1.521990 | 0.729453 / 1.646667 | 0.137369 | 59.81664 |

These short same-binary ON/OFF runs include all specular work, not just the
new layered portion. Animation phases can differ. They meet the provisional
0.20ms New /0.50ms Old CPU gate, but do not predict physical CPU/GPU throughput.
Measured New heap48110752/71303168, linear free18742016/25165824; Old heap
40574112/50331648, linear free4061952/10485760. VRAM free4834304; measured main
stack72748/reserved524288; process103206912/130023424 New and67555328/83886080 Old.

Build/check/verification helpers and `layered-material-runtime-test/` are under
`artifacts/research/pica-scene-rewrite/`. Builders isolate and restore the six
unrelated VRAM work files. Latest stable candidate snapshot is now
`layered-material-v1-builds/source-snapshot` plus its game archive. Its material
patch is already applied; do not apply `pica_layered_material_patch` twice.
The normal builder printed an inherited `console-replay` label, but build
metadata and verification correctly identify its ordinary `console` variant.

Read-only console collection `console-visual/20261010-200040-223/` is complete;
native log SHAece1ddf2... matches the earlier collection. No new hardware run
or remote modifications. SD stalls, disappearing trees/white Arwing, fur and
broader lighting/shadow issues remain unresolved. No complete rendering claim.

## GPU resource and fragment-fog update

The preferred design now uses **per-fragment fog through a reserved PICA
distance-attenuated light**, instead of vertex-fog interpolation. This remains
an isolated probe and shader prototype, not an enabled gameplay material.
Earlier vertex-fog and W-buffer investigations below are retained as evidence;
they are not the intended integration path.

Original `addSphereMapTexStage`, `addLightTexReg2Stage`,
`addTexModulateReg2Stage`, `addTexLayerStageSwizzled` and `addLitColorStage` now
execute in a host oracle. Their emitted operands, texture indices, swap tables,
register destinations and clamps agree with the derived common-family equation
over131072 stage sequences /524288 RGBA comparisons, maximum2.384e-7.
This still does not model GameCube integer rounding or texture rasterization.

`platform_pica_specular.c` now contains an **opt-in, unrequested** half-map owner:
three32x32RGBA8 VRAM targets (12288B) and one92B immutable linear-memory quad.
The original8852B map mesh is unchanged. Half-map preparation is a separate
load-boundary call; no current game caller uses it. Ordinary owner allocation,
draw counts and reuse checks remain unchanged. This code is ARM-compiled and
GPU-tested in the probe, but no new normal/replay application was built.

Full source light channels render/clamp first. An ordered frame split publishes
them, then one six-index GPU draw per changed map copies RGB at128/255 and
sets alpha128 into a distinct target. A second split publishes those targets
before consumers. Late preparation can copy already-valid full maps without
rerendering light channels; unchanged maps reuse their content. No readback,
CPU pixels, streamed geometry, new worker or private queue in the runtime owner.
Diagnostic readback exists only in the probe. All read shader uniforms are
defined even for a half-only update. Normal shutdown retirement owns deletion.

Actual owner ASan/UBSan checks pass100 lifecycles and100000 unchanged updates,
late preparation, toggles, target separation and retirement. A missing first
publication mutant fails at the dependent draw. The unrequested ordinary-owner
regression also passes100 lifecycles /100000 reuses with no extra allocations.

The silent Old-profile Azahar probe renders asymmetric synthetic maps and
checks all pixels across three slots and changes. Copy error is at most one
byte; full/half captures inspected and orientation preserved. Its first two
runs exposed an incorrect diagnostic readback assumption: despite its name,
`C3D_SyncDisplayTransfer` queues inside a frame. The probe now completes that
frame before CPU invalidation/readback, as the pinned SDK and Melee verification
pattern require. No application readback path was added or changed.

The same probe executes the actual four-stage material descriptors for64
controlled samples. Maximum RGBA differences from the source-derived floating
equation are3,3,2,2 byte levels; RGB RMS1.008,0.944,0.910. This is narrower
than the262144-case software precision model, whose worst RGB9.266 remains
recorded. It is not character image acceptance or a hardware precision claim.
The first material fixture failed because its diagnostic shader placed clip Z
outside the valid interval; fixed explicit Z/W writes, no relaxed pixel gate.

### Fragment fog without a fourth UV

The source-selected diffuse environment uses directional PICA lights. Their
dot products do not depend on the view vector. A specialized vertex program
can therefore output `view=(0,0,1+cameraDepth/R)`, with positive normalization R,
while preserving camera/projected position, skinning, normal quaternion and
all three UVs. A positional light at zero has distance `1+cameraDepth/R`.
Its distance LUT uses scale`R/(end-start)` and bias`-(R+start)/(end-start)`;
the256-entry curve is `exp2(-8*clamp(x,0,1))`. Primary diffuse light remains
available, while a constant specular channel writes fog transmission to
secondary fragment RGB. Final TEV fog can consume that RGB without a sampler
or extra geometry pass. Highlight clamping, specular LUTs and geometric factors
must remain disabled for this synthetic light.

Actual GPU probe planes cross both fog clamp boundaries. Maximum colour errors:
0 levels for1470.893..10999.750;1 for0..10000;1 for500..1000;0 for1500..1510.
Directional diffuse output remains exact in the checked red channel. The
captured fog ramp was inspected. These tests use four specific ranges and an
orthographic fixture; broader float20 range accuracy, perspective scenes,
complete diffuse RGB and real hardware still require validation.

The corresponding separate197-instruction actor shader retains primary alpha
and all other outputs except synthetic view/third UV. Actual float24 testing:
32768 vertices,655360 retained lanes bit-identical, UV error0, normalized
camera-distance error1.192e-7. Main combined511 and fur214 programs are unchanged.
The generated specialization is not yet selected by the game.

Integration must share the existing environment register owner, cache its
LUT/range and explicitly restore light/sampler/program state at handoffs.
It must account for the extra light and reserve a slot without silently
discarding source lights (the API permits8; previous scenes reached2).
Fade/opacity and disabled-fog behavior must remain source-correct. Source
material/animated sampler admission, half-map activation, original character
image comparisons, matching scene timing/memory and physical validation remain.

New evidence: `layered-source-test/result.json`,
`specular-half-owner-test/result.json`, `layered-fragment-shader-test/result.json`
under the research directory; `artifacts/diagnostic/specular-half-v3/`,
`specular-half-material-v2/`, `specular-fragment-fog-v1/` contain GPU runs.
Final probe3dsx SHA`45757f9962898787db48bb142a28cc93f2423923196419404ce863040b6b91cd`;
run completed in2.73 host seconds with owned-process cleanup. This is not FPS.
Build with `tools/build_specular_half_gpu_probe.py`; run via
`tools/run_specular_half_gpu_probe.py --label UNIQUE-LABEL`. No console install,
FTP, game assets or saves were touched this turn.

This is **local, inactive prototype work**. It is not in the normal/replay
applications installed on either console. New223 still has normal
`4ad2bfc39ced` and replay `01d268cc7891`. No new physical FPS or image acceptance.

## Source defect and scope

The native secondary-light compiler rejects layered materials. The original
asset audit contains 61 used two-layer declarations, including Fox465 and
Krystal1 material2 and Fox466 material3. All 61 resolve to the native original
geometry pack; none can share its two base texture coordinate sets without
changing sampled pixels. Fox465/Krystal1 have 38/38 differing UV vertices,
maximum normalized coordinate difference 1.1884765625. Fox466 has 73/73.

File texture indices use `UINT32_MAX` for absence. They are not runtime pointers
or boolean flags. Correcting that interpretation shows all61 lack the auxiliary
bump image; most have effective secondary alpha zero and use base modulation.
The source inventory still lists eight unsupported alternate model instruction
streams. This is not whole-game coverage.

For the audited common family, source `addShaderLayerStages` and the helpers in
`shader_dolphin.c` produce:

```
a = A.alpha * constantOpacity
L = clamp(rasterRGB + diffuseMapRGB * strength)
RGB = clamp((B * (1-a) + A * a) * L + clamp(S * L) * A)
alpha = a * B.alpha
```

Both original layers precede lighting. The interpolation direction matters.
An optional texture field of `0xffffffff` must not admit a nonexistent alpha
map or disable base modulation. The prototype only handles this family, not
every original layer mode, opacity policy or independent secondary mask.

## Native prototypes and measured limits

`fh_pica_layered_specular_plan.h` compiles four PICA combiner stages. It retains
the light value in the delayed RGB buffer and uses independent alpha storage
for intermediate values. It requires a half-intensity secondary-light map with
alpha one-half. Base A and B keep independent samplers/UVs. Fade stage4 and fog
stage5 remain available; the plan neither rebuilds geometry nor interprets GX
each frame.

262144 complete RGBA cases using the actual SDK descriptors match the derived
equation in ideal arithmetic (maximum error2.384e-7). An Azahar-style 8-bit
intermediate model reaches **9.266 RGB and3.898 alpha byte levels** of error.
This is not accepted image fidelity. The reference equations are derived from
the source, not an execution of its complete original GX pipeline. Removing
the delayed-buffer publication produces255 failing witnesses.

Three independent texture coordinates leave none for existing procedural fog.
An unconditional W-buffer/fixed-fog replacement was investigated and rejected:
it would require every depth-sharing pipeline to change together, and the
actual SDK128-entry fog LUT has large errors for narrow ranges. Maximum
transmission errors were1.917 colour levels for1470.893..10999.750,
0.229 for0..10000,39.698 for500..1000, and183.122 for1500..1510.
The source projection/depth derivation itself passes81940 checks. Neither
depth mapping nor fog behavior changed in gameplay.

`pica_layered_shader.py` instead generates a separate201-instruction vertex
program (ordinary actor standalone192; combined scene program remains511).
It keeps22 bones and all position/RGB/normal/view outputs. UV0 samples the
secondary map, UV1 the first base image, UV2 the independent second image.
For constant-opacity materials only, primary alpha carries GPU-computed fog;
TEV still derives opacity from textures and the material constant. No extra
CPU vertices or per-frame allocation. c7 becomes second-image scale/scroll;
c94 carries fog inverse width, offset and exponent. Program selection must
restore those uniforms and sampler routing on every handoff before integration.

Actual Azahar float24 execution passes32768 vertices /753664 bit-identical
retained output lanes. Added UV error0; maximum vertex-fog error2.384e-7.
The unchanged shader negative control fails. Static output/control-flow and
register/storage checks pass (96 float registers,16 booleans,7 outputs).
Initial assembler uniform-operand and duplicate-output-write failures were
fixed; the output validator was not weakened.

Vertex fog is **not per-fragment fog**. `fh_pica_vertex_fog_bound.h` bounds its
interpolation error from live transformed depth limits. Smooth intervals use
the second-derivative bound; clamp crossings use a conservative Lipschitz bound.
A million weighted triangles, including perspective weights and418507 clamp
crossings, pass ASan/UBSan checks. A200-unit depth span inside the recorded broad
Arwing fog range is bounded by0.432 colour levels, but a20-unit span crossing
its start is bounded by1.484. Narrow examples reach9.801 or255 levels. These
are examples, **not measurements of live eye triangles**. Raster quantization
and float24 error need separate allowances. There is no unconditional
admission of this approximation and no runtime fog replacement.

## Hardware practices and next integration

Consulted the pinned Melee renderer's `bind_program`, persistent raster cache
and invalidation on copy/target transitions, plus `shader_verify.h`'s actual
program-route and output comparisons. Adaptation: use an isolated native
specialization with explicit ABI/route checks, not its GX translator, stereo
programs or CPU geometry path. No queues, GPU ownership or lifetime changed
in these prototypes.

Before activation: resolve the 8-bit material error with actual image
comparisons; implement/validate a bounded half-map GPU owner; admit live
geometry only within the fog error budget (and design correct handling for
rejected ranges); test cached sampler/uniform/program handoffs; then compare
matching character scenes, CPU submission, GPU workload and resident memory.
No unchanged full game boots were repeated for this inactive work.

Evidence under `artifacts/research/pica-scene-rewrite/`:

- `layered-specular-audit/result.json`
- `layered-specular-plan-test/result.json`
- `layered-shader-test/result.json`
- `vertex-fog-bound-test/result.json`
- `wbuffer-fog-test/result.json`

Run the corresponding `tools/audit_pica_layered_specular.py` and
`tools/test_pica_{layered_specular_plan,layered_shader,vertex_fog_bound,wbuffer}.py`
using the existing Linux toolchain. No original assets or saves are modified.

The latest bounded read-only FTP collection is
`artifacts/console-visual/20261010-192634-223/manifest.json`. Its full native log
SHA`ece1ddf2fa0d1f00aaff087731a6dce6b53b907f139c9aa35fec525f1887f908`
matches153312 and180740 collections; it is not new post-cache evidence.
No remote files were changed. Physical SD stalls, disappearing trees/white
Arwing, fur sampling and broader lighting/shadow fidelity remain unresolved.
