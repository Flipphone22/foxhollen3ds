11 October New223 deployment complete: normal `626d217324e9` / CIA
`762ef1732702`, replay `9bcad101a4fe` / CIA `9acc42b6e738`, with matching
original-geometry22-slot pack and lossless cache. All18 changed payloads staged,
byte-readback verified, renamed and final-size checked. Saves/preferences
preserved; Old229 unchanged. CIA requires FBI installation; matching3dsx ready.
Performance-geometry22-slot pack remains local; original-asset installer does
not replace the generic performance pack. Earlier status entries are historical.
See [changes, tests and unresolved issues](RENDERER_UPDATE_20261011.md).

# Source fur layers and native palette regrouping

11 October local work, not deployed. Normal5931e81be1a5/CIA1fffa3f7facf
and replay3ddbd8395a16/CIAdb2816662ef2
retains the exact511-instruction scene shader f670fde7b7fd. The New installation
remains normal70038555df22/replayde1d418ee54c with its original-geometry cache.

The source objRenderFuzz admits distance-dependent counts capped at3,7 or15.
The prior native renderer discarded every intermediate layer. A new immediate,
persistent SOURCE FUR option (page10, bit4194304) restores all admitted indices;
OFF preserves the former at-most-two endpoint layers. FUR remains independently
required and defaults OFF. Replay/record locks and persisted masks cover the new
option. SOURCE FUR describes source layer density only: wind displacement,
projected-light and layered material families still remain incomplete.

The adapter now submits one bounded object descriptor containing the admitted
noise-page IDs. Native actor submission iterates inner-to-outer shells while
sharing camera, lighting, attribute setup and morph view. Bone rows and the noise
sampler remain live per shell. A command-buffer invalidation still republishes
camera/attributes and the complete palette. No sorting across shells/materials,
CPU vertices, new GPU storage, texture conversion, simulation/RNG change or new
worker. Pinned Melee renderer write-site coherence and persistent state ownership
were consulted; their lifetime/fence rules are retained, not an original GX
interpreter. CPU work remaining is live pose composition and SDK draw submission.

The actual native actor loop passes180 source/reduced cases spanning1..15 layers,
forced command invalidation and unsupported material groups. Captured SDK calls
assert original shell order, correct noise-page identity and global bone rows at
every draw. Deliberately reusing noise page0 or old shell poses fails. Actual
source expansion comparison270000rows and2000stream pivots passesASan/UBSan.
Menu persistence/lock/no-idle-flush tests pass, including unknown-bit rejection.
Initial test harness declaration/link errors were fixed before acceptance; no
console crash resulted. These tests are not GPU raster/physical timing evidence.

## First gate: full density is too costly

Same-binary New cockpit source/reduced runs complete state assertions, save
preservation, shutdown/log drain and owned exit. Source capture900 was inspected:
Fox, eyes, fur, cockpit and HUD visible; lighting tint and original-image parity
remain unresolved. Source10213shells/30639draws/34285041indices, reduced1452shells/
4356draws/4874364indices; whole sessions differ by one object observation.

| Azahar cockpit | Reduced | Source |
|---|---:|---:|
| FPS |59.83091|59.81677|
| Scene CPU ms/frame |0.750853|0.765318|
| Native submission CPU ms/frame |1.862972|2.916216|

The combined+1.067709ms exceeds the0.5ms New fur budget. Similar paced FPS does
not erase that cost or establish physical throughput. Both heaps47,855,288/
71,303,168B, linear free18,809,472/25,165,824, VRAM free4,846,592, measured stack
72,740/524,288, process103,186,432/130,023,424. No additional RAM allocation.
This candidate is not accepted for default quality or physical deployment.

## Architectural correction under test

Both current scene manifests still specify11-bone grouping, inherited from the
older position+normal palette split. The resident native shader has22 position
slots at c10..75 and transforms normals from those rows. Its already-validated
float24 corpus addresses all22 slots; its exact bytes have not changed.

Read-only preparation audit finds10 fur-bearing models,67 fur draws mergeable
into20 while preserving material/order and the22-slot bound. Fox465 and Krystal1
have three consecutive compatible fur groups each, mergeable intoone. The new
explicit --model-bones option to rebuild_native_scene_topology.py can recompile
selected --model-ids without changing topology. prepare_original_scene.py exposes
that explicit option too; existing default/prepared assets are not silently
changed. The same representation continues to carry original morph references.

Candidate fur-palette22-v1 rebuilds10 identified models; one is byte-identical.
All other2093 packets are unchanged. compare_native_actor_packets.py verifies
ordered corners including position/normal/colour/bothUV bytes, original morph
references and global bone/weight bindings: exact. The9changed actors preserve
71577indices, reduce all groups150->57, fur groups66->19 and GPU bytes1003898->
932412. The complete pack is42698812B. No added geometry, removed triangles or
lower-resolution textures. Runtime, morph behavior and physical execution still
need acceptance. See fur-source-layers-test/palette22-equivalence.json and
merge-audit.json; do not infer GPU throughput solely from fewer CPU submissions.

## First regrouped runtime result

Same replay binary and cockpit recipe,22-bone candidate pack: source-density
New check completes43.59hostseconds, all state/save/drain/owned-exit checks pass.
Source10199shells now10199draws (one per shell) with34238043indices andzero
unsupported fur. Inspected900: Fox, eyes, tail fur, cockpit and HUD retained.
Single view comparison is not identical animation state:10502/96000pixels
differ, mean absolute RGB1.185/255; do not call this pixel-exact image parity.
The stronger geometry claim is the ordered native corner-byte equivalence.

Scene0.758778/submit2.072228ms versus11-bone source0.765318/2.916216ms: about
0.85053ms less combined CPU rendering. Different pose phases/object counts;
short relative emulator result, no physical FPS prediction. Both around59.83
paced Azahar FPS. This demonstrates a structural draw reduction, not skipped
simulation or fewer rendered triangles.

Same22-pack FUR OFF check completes35.25s, scene0.716289/submit1.499361ms.
Full-source fur adds0.615355ms combined CPU, still above the0.5ms New budget.
Do not label this accepted because it is close, or quote only the incremental
cost relative to reduced fur. Both heaps47,846,248/71,303,168B, linear free
18,742,144/25,165,824;VRAM/stack/process unchanged. Linear residency in this
scene is67,328B higher than11-pack despite smaller aggregate asset bytes; changed
morph/owner granularity matters, so pack size alone is not a runtime RAM claim.

Next native route: Fox/Krystal merged fur palettes use16 bones. A dedicated fur
vertex program can pair48 matrix rows with16 pivot/growth vectors in the space
used by the ordinary program's66 matrix rows. GPU shell expansion could keep
poses resident between layers. It needs exact source/float24 error bounds,
lighting-normal equivalence, publication/handoff tests and real hardware
acceptance. Other audited fur models require19..22 bones and cannot be silently
omitted; preserve their current native path until a bounded solution exists.
A texture atlas alone is not a sound batch solution: independently repeating
64x64 noise pages would bleed between atlas regions without correct fragment
wrapping. No such atlas or new shader is implemented yet.

Shared normal/replay builds verify the same game archive,68renderer sources,
exact previously float24-tested shader and Old80/New124CIA profiles. Normal
builder inherited a console-replay print label; its verified variant is console.
No new normal runtime claim; menu/persistence checks are host-side this turn.
Six unrelated VRAM WIP files are restored after each isolated build.

Old-profile source-density22-pack check also passes all state/save/drain/exit
assertions in42.79hostseconds;900capture inspected with Fox/fur/eyes/cockpit/HUD
and touch item pad. Scene0.760452/submit2.074575ms,59.83122AzaharFPS. This is
not a physical Old60FPS claim and lacks a matched OldOFF budget comparison.
Old heap40,506,216/50,331,648, linear free4,062,080/10,485,760, VRAM4,846,592,
stack72,740/524,288, process67,534,848/83,886,080.

## General actor preparation

`actor-palette22-v1` recompiles835actor packets with22-slot preparation.81
change;2021of2102packets remain byte-identical. For those81, total groups
716->431, fur groups66->19, stored vertices151849->146665 and GPU payload
7073872->6864322B, while all497712indices and ordered source/global-bone/UV/
normal/colour/morph-reference bytes remain exact. Total pack42497636B,
SHA63222ea8734c4140a266575f0b9c78186e4f2e9cb4d1915f16da2ad24f511e84.
This extends the preparation correction to compatible non-fur draws; it does
not restore unimplemented materials or prove physical performance. Original
geometry now has its own22-slot repack, exact comparison, matching compressed
cache and New/Old Arwing runtime checks; see NATIVE_ACTOR_PALETTES.md. Existing
prepared packs are preserved and neither new asset set is deployed.

General actor cave validation passes with a smaller0.174ms combined CPU gain;
see NATIVE_ACTOR_PALETTES.md for scene counts, memory and scope. This does not
remove the failed full-fur CPU gate or establish a hardware FPS improvement.

The separate GPU shell-expansion path is now integrated locally and passes
source/float24, ownership and New/Old cockpit checks. Full source density adds
0.330329ms New rendering CPU versus same-binary OFF, now below0.5gate in this
scene. Four models use it; six stay on the native matrix path with no omissions.
Physical GPU cost, broader gameplay and wind/material fidelity remain open.
See NATIVE_FUR_GPU_EXPANSION.md. No deployment or performance-default change.
