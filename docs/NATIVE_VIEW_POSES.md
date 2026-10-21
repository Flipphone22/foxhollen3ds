# Camera-space native bone palettes

14 October: local candidate, not deployed or accepted as a frame-time improvement.
It builds on the separately preserved packed-light/normal-demand candidate.

The native actor shader previously skinned into world space and then transformed
both the resulting position and normal through the camera matrix for every
vertex. The candidate concatenates camera and bone once per unique bone during
an actor draw. GPU skinning consumes those camera-space rows directly. The
shader removes three position-path instructions and another three when a normal
is needed. It retains source weights, bind tails, fur pivots, material/lighting
equations, geometry, camera, simulation and draw order.

This is not CPU vertex skinning. A scoped cache contains at most257 matrices,
indexed by original joint ID, with nine32-bit validity words. It uses12,372
stack bytes per actor invocation, resets only those validity words, and is
never retained across objects, frames, camera changes or worker boundaries.
The SDK copies published rows; the GPU never borrows this stack storage.
Unchanged palette slots retain the existing prepared-draw publication rules.

Ordinary, layered and native GPU-fur programs use the new palette convention.
World geometry, UI models, cloud shaders and shadow casters keep their existing
conventions. An actor containing different material families explicitly selects
the correct convention; shader selection already forces the necessary palette
publication. CPU-expanded fur **matrices** (not vertices) remain shell-specific,
and are composed without caching a previous layer. This avoids reintroducing
stale poses or changing any shadow shader while physical flicker is unresolved.

The pinned Melee `port/3ds/vertex.v.pica` likewise separates its supplied model
transform from projection. Foxhollow preserves its own source two-bone, morph
and fur semantics rather than copying Melee's representation. Pinned SDK
uniform-copy/dirty-state and existing native reader ownership remain intact.
No new worker, GPU buffer, texture, draw pass, per-frame heap allocation or file
read is introduced. There is additional CPU matrix work and a stack cost;
both must be measured before promotion.

## Current evidence

`view-pose-owner-test/result.json` executes the actual cache and renderer
publication branch under ASan/UBSan, including640,000 mixed ordinary/cloud/UI/
shadow/fur publications, new-object resets, boundary joint IDs and invalid
slots. It builds10,000 cached poses and reuses50,000;20,000 shell-specific
publications intentionally bypass the cache. Missing reset, cloud/UI/shadow
exclusion and incorrect cached CPU-fur mutants fail. The fur branch harness
substitutes a distinct valid affine shell transform to test ownership/selection;
it is not an original-fur simulation oracle.

`view-pose-shader-test` and `view-pose-linked-test` compare the prototype and
actual linked executable against `light-normals-v1-console-replay-builds`:

-37,888 vertices per ordinary/layered/fur family, including light masks,
 blended/single bones, unlit/world cases, alpha, environment/specular and groom.
-Worst tested screen-position difference0.0000357628pixel, colour difference
 0.000122070312 (fur), other lane absolute difference0.0000610351562.
-2,923 sampled execution traces per family, none longer than the parent.
 Ordinary225,110→213,929 instructions; layered239,725→228,544;
 fur312,710→299,537 across those traces. This is not a scene-weighted GPU time.
-Missing camera translation and rotation mutants fail image-coordinate bounds.
-Linked shader banks445→439 ordinary,238→232 material,150→144 fur.

Azahar's float24 type does not model physical rounding at every ALU operation.
These sampled local error bounds therefore do not establish physical precision,
flicker acceptance, GPU throughput, or an emulator-to-console multiplier.
Nonactor shader outputs remain exact in the tested set.

## Source and build isolation

`tools/build_view_pose.py` selects immutable light-normal sources, changes only
the native actor shader, scene publisher and new helper header, retains the
same game archive, and restores unrelated VRAM work in `finally`. The archived
candidate is `view-pose-v1-console-replay-builds`. Do not blind-build the live
worktree or treat a prototype as the accepted baseline. Old80/New124 CIA
profiles are checked. No console assets, preferences, saves or applications
have been modified.

The earlier actor-cache audit was also revisited. Order-independent shadow-mask
triangle reordering is already implemented. An additional order-preserving
alias audit over the835-model original pack found **zero** identical40-byte
vertices with identical morph position/normal references, so no runtime code
or asset rewrite was added for that route. The larger general index-reordering
numbers must not be applied blindly to transparent/equal-depth materials.
Evidence: `actor-identical-vertices-audit/report.json`.

## Runtime evidence and limits

Shared replay source75e07607e522,3dsx1bcf760e9da8/CIA2b157759b26f is built
locally. Serial New/Old palace runs complete with state assertions, original
saves, drained logs and owned exits. Both1410 images were inspected: Krystal,
purple creatures, world textures, effects and HUD remain present. No normal
variant was built for this unaccepted experiment; the prior light-normal
normal/setup candidate remains unchanged. Nothing was deployed.

The New run builds115,375 camera palettes and reuses119,786; Old111,981 and
117,412. These are whole-run bone counts, not frames or GPU vertex invocations.
Neither invokes the special CPU-fur matrix branch on this route. Both final
timings are excluded because external SMG compilation was observed. No paired
baseline boot was repeated while contended, and no FPS improvement is claimed.

New heap48,598,456/71,303,168,linearfree18,036,224/25,165,824B;
Old41,061,792/50,331,648,linearfree3,356,160/10,485,760B. Both retain
4,723,712VRAM bytes. Measured main stack peaks72,972/72,984 out of524,288B
are unchanged at this sampling resolution; this does **not** mean the new
function has no additional stack cost. Its local cache is12,372B. Process
commitments103,354,368/130,023,424 and67,702,784/83,886,080 remain within
grants. See `view-pose-owner-test/runtime.json` and linked ARM disassembly.

An additional sensitivity test truncates uniform fractions to16bits, retaining
the normal float24 exponent range. It does not model all physical upload or
ALU rounding. In37888vertices per linked family, screen error is at most
0.002218pixel. The initial absolute colour gate failed on a raw fur alpha of
more than1: the homogeneous UV scale occupies tint.w. That failure is retained
as `view-pose-quantized-linked-test/fur-raw-alpha-gate.txt`.

The refined gate still bounds RGB/in-range colours absolutely. For nonnegative
alpha it also checks relative error<=1/8192, giving a conservative
2epsilon/(1-epsilon) bound after interpolation with matching nonnegative weights
and clamping. The largest tested relative alpha error3.58542e-5 yields a
7.1711e-5 bound, below one8-bit level. This conditional bound does not account
for changed raster coverage or prove console precision; raw maximum alpha
error0.00402832 is still reported. Missing rotation/translation mutants fail.
The first sensitivity harness compile attempted a nonexistent `ToRaw` method;
it was replaced with explicit bounded fraction truncation, not a game change.

Full rendering/progression, added CPU cost, physical performance/precision and
the reported rapid whole-shadow flicker remain incomplete requirements.

## First uncontended New-mode comparison, 14 October

`view-pose-v1-clean1-baseline-on-new` and `view-pose-v1-clean1-on-new`
ran serially with no compiler processes observed at either boundary. Both
passed state assertions, save preservation, log draining and owned exit.
Both1410 images were inspected: palace, Krystal, purple creatures, effects and
HUD remain present. This does not make differing animation phases pixel-equal.

The300-frame windows both report59.831223 guest submitted FPS. Exclusive CPU
scene+object+submission rises7.459586 to7.541515ms, a0.081930ms increase;
submission alone rises3.745742 to3.843011ms. Draws51849 to51467 and indices
18809394 to18705198 differ slightly with live phase. GPU pending span falls
0.755343 to0.743465ms, but overlaps CPU and does not measure physical PICA
execution. A single pair is not statistically conclusive. The snapshot's host
status bar reports roughly40–44 host FPS: guest cadence must not be confused
with emulator wall-clock delivery or console performance.

The candidate remains an unaccepted CPU/GPU tradeoff. It shortens vertex
execution, adds measured CPU work, and establishes no FPS gain. No normal
build or deployment follows from this comparison. Preserved prior contended
evidence was not overwritten; `--run-tag` now gives each timing pair a distinct
label. `tools/compare_view_pose.py` verifies matching inputs, assets, settings
and saves and records exclusions/workloads without inferring hardware FPS.
Evidence: `view-pose-owner-test/comparison-clean1-new.json`.
