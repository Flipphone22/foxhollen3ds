# Native GPU shell expansion

11 October: deployed on New223 in replay9bcad101a4fe and normal626d217324e9;
see [the verified deployment](RENDERER_UPDATE_20261011.md). Old229 was not
updated. The later [environment-normal correction](NATIVE_ENVIRONMENT_HEMISPHERE.md)
is local only. The same-
binary cockpit comparison now meets the New CPU gate for full source density
(0.330329ms versus fur OFF). This is one emulator scene, not complete fur
fidelity or physical GPU acceptance. Wind/projected/layered materials remain
missing. Matching normal build/check details are recorded below.

The previous native CPU path composed and republished each selected shell's bone
matrices. `pica_fur_gpu_shader.py` derives a dedicated native program from the
current actor shader, replacing 22 matrix slots with 16 base matrices, 16
pivot/growth vectors and two shell/decode vectors. The rest of the native
lighting, texture-coordinate and fog ABI remains unchanged. No geometry stage,
CPU-expanded vertex stream, extra texture or original GX interpreter is involved.

For ordinary skeletal fur, with source joint matrix J, tail T, pivot P and shell
scale s, the current CPU palette produces `J * (s*(position-T)+P*(1-s))`.
The GPU path retains the base palette `J * translate(-T)` and supplies the GPU
point `s*position+(P+T)*(1-s)`. It also scales each transformed normal by s before
bone blending and lighting. Omitting that normal scaling is a visible error.

The shape vector carries P+T and log2 of the source growth increment; PICA EX2
evaluates the selected layer. Root slots have zero growth. Source step values
and distance-admitted layer count remain game-owned. The scene load owner now
prepares immutable growth for steps1/2 once from the original big-endian model
source and native tail table. There are no per-frame CPU logarithms.

The standalone program assembles to 214 instructions, 96 float and 16 Boolean
registers. The existing combined scene program remains 511 instructions and
cannot simply absorb this kernel. Switching separate programs has real command
and GPU costs. The CPU comparison below includes runtime switching; physical
GPU cost remains unmeasured. The fur wrapper invalidates scene ownership both
on entry and exit. The next ordinary draw republishes its shader/palette/material
state; real frame/command-list invalidation also forces full publication.
Pinned Melee `renderer.c` program ownership and `command_cache.c` write-site
publication, plus pinned SDK `C3D_BindProgram`/`C3Di_LoadShaderUniforms`, were
consulted. Binding uses the SDK; no register bypass or new worker.

`audit_pica_fur_shapes.py` finds 10 fur-bearing source models. Referenced
skeletal radii range from 0.454455525 to 18.1518803 and are finite/positive.
Several unused joints contain zero radius; those are not evidence of a rendered
failure. Stream-expanded model245 uses a different equation. Some other fur
groups require19..22 slots. They retain the existing native shell path until
separately supported; no model, draw or shell disappears due to this admission.

`test_pica_fur_gpu_shader.py` extracts the original source matrix loop and runs
both native programs in Azahar's actual float24 interpreter. 61,440 vertices
cover all16 local slots, blended bones, an unexpanded root, both growth steps,
15layers, varying light flags and both synthetic and authored pivot/radius data.
Maximum sampled differences: clip position0.000732422, view position0.000488281,
colour0.000366211, normal quaternion0.000003815, fog0.0000000187; both UV outputs
are identical. Missing-normal-scale and missing-pivot mutants are rejected.
Complete-output/IF-stack/MOVA-separation checks pass. This does not validate
rasterization, near-plane pixel error, shader handoffs or physical throughput.

Development failures retained in the record: an indexed uniform was initially
placed in PICA's restricted second operand; operand publication corrected that.
The harness initially assumed the wrong output-register ordering, then used the
assembled DVLE mapping (position0, colour1, UV0/1 at2/3, view4, quaternion5,
fog6). The gameplay shader was not changed during those failures.

Evidence: `artifacts/research/pica-scene-rewrite/fur-gpu-shader-test/`.

## Load owner and runtime validation

`pica_fur_plan.c` admits a complete fur object only if its fur groups all fit16
bones and use the supported skeletal equation. The source joint count does not
normally include an additional root: an initial overly strict joint+1 assertion
was caught by the private-source test before runtime and corrected. Candidatev1
e580abca1869 was built but never runtime-accepted or deployed; v2 is9bcad101a4fe.

Four source models1/465/466/449 are admitted, using728/728/768/768CPUbytes.
Six other fur models stay on the native matrix path. The optional allocation has
a32KiB global cap and the existing normal-heap reserve. It uses no linear RAM or
VRAM and is retained even with fur OFF. Preparation refusal preserves rendering.
The scene record owns its lifetime and releases it at the existing retirement
boundary; commands copy parameter values into SDK uniform storage.

A load-compiled last-to-first palette-change mask permits base matrices to stay
resident across shell wraps. Changed palettes and command-buffer invalidation
still republish every required row. Camera/light/morph state retains the existing
shared per-object preparation. No CPU vertices or extra shader geometry stage.

Actual-private-source compiler checks cover all10fur models and12invalid
capacity/source/radius cases. ASan/UBSan owner tests pass100load/retire cycles,
single/multiple palette wraps and allocation/normal/global-budget refusal.
The actual draw loop passes360source/reduced/CPU/GPU/forced-invalidation/skipped-
material cases. A wrong shell-wrap palette mutant fails. Actual binder/wrapper
tests pass200ordinary/fur/retirement transitions; missing exit invalidation fails.
These SDK-stub ownership checks complement the numerical interpreter and runtime
checks; they do not emulate hardware GPU caches.

Serial v2New SOURCE/OFF and Old SOURCE cockpit runs pass state/save/drain/owned-
exit assertions in37.04/35.20/36.39hostseconds. New/Old900captures were inspected:
Fox/eyes/fur/cockpit/HUD and Old item pad remain visible. Green lighting and fur
wind fidelity remain unresolved; no full visual parity or flicker-free hardware
claim. All~59.83122paced AzaharFPS, not console throughput.

| Profile/mode | Scene CPU ms | Submission CPU ms |
|---|---:|---:|
| Previous native CPU shells, New SOURCE |0.758778|2.072228|
| GPU shells, New SOURCE |0.757768|1.795900|
| Same binary, New OFF |0.714337|1.509003|
| GPU shells, Old SOURCE |0.762240|1.795885|

New combined CPU reduction0.277337ms versus the prior full-density path;
whole-effect cost0.330329ms versus same-binary OFF, below0.5ms New CPU gate.
ON comparisons retain10199shells/draws and34238043indices,0unsupported.
724objects use GPU expansion and publish46336matrix/shape rows. All actor
matrix rows fall823086->368286; the extra shape rows are counted separately in
the GPU-fur telemetry. Direct fur submission falls0.041607->0.019979ms/shell.
Animation observations can differ; this is not an exact frame/state A/B.
No matched OldOFF or physical GPU cost is claimed.

Runtime fur parameter allocation1496B,2plans,0declines. New heap47,848,120/
71,303,168B; Old40,508,088/50,331,648. Linear free18,742,144/25,165,824 and
4,062,080/10,485,760; VRAM4,846,592; measured stack72,740/524,288. Process
New103,198,720/130,023,424 andOld67,547,136/83,886,080. Normal/linear/VRAM
headroom remains; this cockpit is not the tighter original-geometry Old route.

Additional evidence: fur-gpu-plan-test/{result,owner}.json,
fur-gpu-handoff-test/, fur-source-layers-test/{result,mutant-wrap/result}.json,
fur-gpu-runtime-test/runtime-comparison.json. Next: broader affected gameplay,
original-geometry memory/cost checks, remaining fur material/wind fidelity and
physical shader-transition/GPU-budget validation. Defaults remain unchanged.

Normal626d217324e9/CIA762ef1732702 and replay9bcad101a4fe/CIA9acc42b6e738
verify identical game archive and317native source files. Both embed the exact
214instruction float24-tested fur program and unchanged511instruction ordinary
scene program; sharedOld80/New124CIA profiles remain intact. Six unrelated VRAM
WIP files were isolated/restored by the builders. The normal builder's inherited
console-replay printed label is cosmetic; its recorded variant is console.

Normal startup/touch-toggle/Select run passes in25.01hostseconds with no test
hooks, saved options5259265 (SOURCE FUR+FUR+defaultmask), valid FHGRAPH4 FNV
checksum and clean owned shutdown. The540image was inspected: readable SOURCE
FUR row, opening movie and record control retained. Settings are isolated test
preferences, not changes to console defaults or the user's SD. There are no
new console files or deployment claims in this update.

## Broader original-asset check, 11 October

The same v2 binary now also completes the original-geometry/original-texture
Arwing route on both profiles with the user's effects and source density enabled.
New/Old Azahar cadence is 54.31/47.59 FPS; scene CPU 2.001/1.974 ms, submission
3.743/3.719 ms. The earlier ordinary Arwing runs used different effect settings;
these numbers are not a speedup comparison. Both 1650 images were inspected.
New renders 5,295 admitted source shells and Old 5,492, zero unsupported;
animation phase differs. New scene SD reads after startup remain zero.

Original-asset heap peaks are 56,156,328 / 71,303,168 B New and
41,475,704 / 50,331,648 B Old. Linear free is 16,660,224 / 25,165,824 B and
1,980,160 / 10,485,760 B respectively. VRAM free 4,846,592 B, measured stack
72,860 / 524,288 B, fur parameter peak 1,496 B. See
[combined update and limits](RENDERER_UPDATE_20261011.md) and
`fur-gpu-original-test/runtime-comparison.json`. Installation status is recorded
separately; this test is not physical GPU acceptance.

Source follow-up confirms that `objFuzzRenderCb` still differs materially from
the native sampler: layer/32 scales two indirect matrices (both exponent -1).
The first reads the scrolling generated caustic texture at base UV plus half
the live scroll offsets. The second reads the material's indirect texture;
its RGB channels alter the noise coordinates while S supplies coverage alpha.
Those offsets affect fragment sampling, not just shell positions. A generic
vertex wobble would not restore that behavior. Native coverage and source
density are implemented, but neither restores these texture-driven offsets.
No speculative CPU vertex update or unmeasured extra geometry pass was added.
