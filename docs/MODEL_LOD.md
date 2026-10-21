# Prepared render meshes

The user authorized stronger visual reductions on 5 October 2026. The current
implementation prepares a **static reduced-detail model pack**, not dynamic
distance LOD. Original disc assets remain intact. Collision, animation, joints,
hit volumes, material instructions and vertex attribute arrays are unchanged.
Only model display-list topology and its byte counts are replaced, before
relocation or renderer publication. Surviving vertices retain original attribute
packets. This reduces render work, not the original model allocation or animation
update work. Environment block render lists are also supported with `--include-blocks`; their collision polygons, bounds and render instruction streams remain unchanged.

Prepare locally in the existing Linux build environment:

```sh
python3 /work/tools/prepare_model_lod.py --output /work/assets/GSAP01-rev1/prepared/model-lod.bin
python3 /work/tools/test_model_lod.py --pack /work/assets/GSAP01-rev1/prepared/model-lod.bin
```

Preparation uses pinned meshoptimizer v1.0, commit
`73583c335e541c139821d0de2bf5f12960a04941`, under its MIT license in
`reference/meshoptimizer/LICENSE.md`. The established preparation command runs
on the host. A bounded native startup converter is also under development; its
current reductions are **not** performance-equivalent to deployed assets (see
[the native preparation audit](INSTALL_GEOMETRY_PERFORMANCE.md)). Default
target is 20% of original triangles with a 3.5% relative simplification-error
limit, attribute error metrics, material borders and matrix-boundary locks.
The error limit is in model space, **not a guaranteed screen-pixel error**.
The simplifier may stop above the requested ratio. Unsupported instruction
streams and lists that cannot fit a smaller replacement remain original.
It never removes disconnected components as a separate optimization.

The private JSON report records each changed model's source hash and list counts.
The pack identifies each model by ID, original length and CRC32. Payloads and
the directory have additional CRC checks. The host manifest uses SHA256.
These CRC checks detect mismatches; they are not a security signature.
The runtime validates the entire patch before modifying the model, checks
destination bounds and overlap, and retains original assets on rejection.

For isolated emulator testing, pass `--model-lod PATH` to `tools/run_native.py`.
The runtime reads `sdmc:/3ds/foxhollow/model-lod.bin` if present. It retains only
the small directory and file handle; each payload is read at model load and
freed immediately. Allocations preserve the existing 15% normal-heap reserve,
including the reserved main stack. There is no per-frame simplification or SD
lookup, and no additional linear RAM or VRAM allocation. Memory logs report
applied/rejected counts. Shutdown closes the file and frees the directory.
Removing the optional pack restores original mesh detail on the next launch.
Do not overwrite a pack hard-linked into a recorded test profile; use a new
output filename for a different candidate.

Initial pack `4eb40ca6ef2239010cb9b372092051543359cfe8e89eaf25e481516b43ba508c`
is 1,452,888 bytes: 317 changed model variants, 435 lists, 167,412 to 67,294
triangles in those lists. This is not a whole-scene or FPS reduction ratio.
All replacements pass the runtime patch code on the host; non-render bytes
and original attribute packet membership are checked. ASan/UBSan tests cover
truncation, mismatched identity, overlapping storage and transactional rejection.
Animation quality, transparency, silhouettes and essential visual cues still
require representative captured gameplay and console validation.

The pinned Melee renderer's ownership/cache rules remain intact. In particular,
prepared geometry is installed before consumers can retain it; there is no
in-flight source replacement or relaxation of GPU fences. Melee does not supply
this SFA-specific simplification policy. Its `port/engine/geometry_dirty.h` and
`port/3ds/renderer.c` were consulted for visibility and retained-source ownership.


The stronger candidate uses `--ratio 0.1 --error 0.075 --include-blocks`.
Private pack `bb9e6d26eaa9595ecd3e9383b0b2f9b8d7252a2d4ee78d95fd1e9b56f9a02ccd`
is 4,549,144 bytes: 1,397 model/block variants, 3,263 changed lists,
747,079 to 273,303 triangles in changed lists. This is deliberately reduced
visual detail, not a 10% whole-scene workload guarantee. All 1,397 replacements
pass the runtime validator and unchanged-data checks. Model IDs with bit31 set
identify environment blocks; their raw header is164 bytes, display-list table
at0x68 and count at0xa1. The host reads indexed PAL mod*.zlb.bin variants;
unindexed root copies are not assigned guessed IDs. The directory occupies
33,528 normal-heap bytes; payload allocation remains transient and reserve-gated.
Original vertex arrays and collision remain allocated, so this is a rendering
work reduction, not a claim of equivalent asset-RAM savings.

Install the private pack alongside the tested executable as
`/3ds/foxhollow/model-lod.bin`. The transfer tool's explicit `--model-lod` option
validates the local manifest, stages the file, verifies it, backs up any previous
pack and verifies the final SD copy. It never replaces disc.iso or saves.
The public validation ZIP stays asset-free; this pack remains a private input.

The experimental `--permissive` option allows normal/UV/color seams to collapse
within the same error budget. Material borders, triangles crossing matrix
palettes, and coincident positions with different matrix packets remain locked.
It still retains original attribute packets and all non-render data. Candidate
`model-lod-world-seams-joints.bin` has SHA256
`1246077f6f9b19daccdb2db1ed592ed718499590627cb228bf13574102f7b172`:
1,558 variants, 3,915 changed lists, 870,018 to 288,247 triangles, 4,815,592 bytes.
This candidate was installed with console61b3a9587092. An earlier seam-collapse candidate reduced
menu vertex counts by about 18–20% but did not substantially improve emulator
frame times. Do not infer an FPS gain from these asset counts.

Optional `--permissive --joint-weighted` relaxes mixed-joint triangle locks
using ten one-hot joint attributes in the simplification error metric. It
retains coincident matrix seams, independent texture-matrix boundaries and
every surviving original attribute packet. This is an animation-detail
approximation; it does not rewrite bone, collision or morph data. Candidate
`model-lod-joint-weighted.bin` (3defd31ceb6b) remains experimental and uninstalled.
Its menu vertices fall another10–11%, with only a small Peppy frame-time change
and no Fox improvement. Animated gameplay quality remains unvalidated.

Experimental whole-model clustering (`--cluster-cell .125`, 6 October) shares
position representatives across material lists before the ordinary simplifier.
This differs from the default: position indices can change while every other
packet attribute remains original. Open boundaries, incompatible resolved bone
bindings, reversed faces, disappearing complete material lists, and baked map
visibility boxes constrain the reduction. It does not guarantee preservation of
every small connected component, silhouette, morph, or animated surface.

Reproduction of the evaluated private candidate:

```sh
python3 /work/tests/model_cluster.py
python3 /work/tools/prepare_model_lod.py --output /work/assets/GSAP01-rev1/prepared/model-lod-cluster125-bindings.bin --ratio .05 --error .075 --include-blocks --permissive --joint-weighted --cluster-cell .125
python3 /work/tools/test_model_lod.py --pack /work/assets/GSAP01-rev1/prepared/model-lod-cluster125-bindings.bin --clustered
```

Candidate SHA256 `1b53cd18a57bf580999581df6d303820e78c9d1127285c577314d06197f3e681`
passes transactional runtime loading for 1,840 model variants. **Rejected for
deployment:** cockpit vertices fall 19–21%, but emulator median CPU time only
about 5%, with visibly harsher character geometry. It is not a solution to the
physical 6 FPS baseline. Do not expand this into further ratio/grid tuning or
request a console run on this evidence. Default preparation and the installed
pack remain unchanged. See the 6 October whole-model clustering record in
`IMPLEMENTATION.md` for exact comparison windows and limitations.


7 October correction: the earlier simplifier interpreted vertex RGBA4444 as
RGB565 and could remove all opaque cloud coverage. Use model-lod-alpha-coverage.bin
(82412755d509) with scenev9(adcbe1b8d3d2); preparation and tests are recorded in
IMPLEMENTATION7October. Cloud masks now retain opaque interiors. Alpha gradients
are locked and missing coverage levels reject simplification. Position-only
clustering protects varying-alpha lists. This restores render work, not an FPS win.


8 October local feature repair: `repair_model_lod_fidelity.py` preserves source
morph triangles and prevents complete deletion of disconnected pieces. Candidate
`model-lod-feature-coverage.bin` (be5685634a87) pairs only with complete scene
bfb5e2626f9f and cache d83cf39ce262. Peppy face/glasses improve in headless imagery,
but cockpit index references rise16.5%; no physical FPS or deployment approval
claim. It remains local pending performance work. Full reproduction/measurements
are in the latest IMPLEMENTATION section. Default future simplification also uses
correct VAT5UV units and morph/component-loss safeguards. Explicit component-error
mode is experimental and cannot combine with positional clustering. Component
survival does not itself bound texture interpolation or silhouette distortion.


## 8 October: saved cave passage fidelity

8 October saved-cave passage repair, New binary73e4838ca819, assets pending deployment.
The latest save ab4698fe loads in swapholbot. Ordinary scripted controls descended
from the ladder platform and traversed the passage beneath it into the next
chamber. Reduced geometry created diagonal slabs across the arch and severe UV
stretching. Matching replay with original geometry restores the curved passage.
Only five swapholbot source identities are restored; other banks retain their
existing LOD. No collision, save or progression changes. This is a targeted repair,
not a claim that all decimation/material defects are fixed.

Private topology dac4acbd6d15 (4,426,408B), v8scene5bb0ae7d4a8f (39,953,108B),
cache7b64f69c46bb (15,874,705B disk, unchanged7MiB resident window) must be deployed
together. Existing texture760f3b/movie656ad remain. Original source-position refs,
GPU fencing and bounds refresh preserved. No graphics/OS implementation change
beyond the already-tested73e RGBfix. Pinned Melee ownership/persistent GPU buffer
approach retained; source geometry restoration is Foxhollow asset preparation.

Exact-console hallway-original-console clean1101/790rendered,13529GPU work
submitted/completed,0pending/errors. Matching binary/save/input baseline:
107587->107871draws (+0.26%),22,388,127->25,867,239indices (+15.54%). This fixes
visibility at added geometry cost; no hardware FPS or no-regression claim.
Normal+stackpeak55,771,296/71,303,168B; linearfree minimum8,736,896/25,165,824B;
initializedVRAMfree4,817,920B; measured mainstack26,168/524,288B. Prepared texture
and scene runtime SD reads0, no cachepressure or texture budget evictions.
All53banks coverage0missing, reader2102payloads/46356streamchecks; actual C refresh
1267world packets/808656vertices/2534negative checks and publication ordering pass.

New host surface diagnostic finds up to43.5world units of sampled deformation in
the old cave reduction. UV interpolation can change despite retaining original
surviving vertex records. A global quality guard and other banks remain future
work; this sampling audit is diagnostic, not a proved continuous surface bound.
Physical prior e3 mixed run21.64FPS with8frames>1s; tail60026.49FPS. Not a matched
fidelity/performance acceptance. Both performance target and broader goal remain
incomplete. Old17c built/packaged; installedOld9793 unchanged.
Evidence: artifacts/research/hallway-fidelity/integration.json; before image
hallway-navigation/frame-020.png, after hallway-original-navigation/frame-015.png.
Captures use matching route positions, not exact rendered frame assignments.


## Default world-surface acceptance

World reductions now pass `world_lod_fidelity.accept` before publication. It checks finite samples in both directions against original positions, all UV layers, RGBA and winding. `prepare_world_lod_fidelity.py` retries smaller simplifier error bounds and otherwise retains original material lists. This prevents the reproduced cave distortion, but is not a continuous error bound or a global repair of existing assets. Ordered strip encoding preserves the full ordered oriented-face sequence. The cave trial saved less than 1% and is not selected for deployment. See `artifacts/research/world-fidelity/tests.json`.

## Default model-surface acceptance

`model_lod_fidelity.accept_list` now gates ordinary model reduction and final
legacy clustered payloads. Original palettes resolve bind-space positions; every
material context must pass sampled shape, all UV layers, RGBA and winding in
both directions. Limits are0.5bind unit,1/128UV,1/32RGBA and20degrees. Morph and
CPU-skin protections remain separate. Failure retains source geometry. This
adds offline preparation cost only, and does not repair already prepared packs.
Finite sampling is not an animated or continuous error bound.

The actual WarpStone52-to16detail reduction is rejected by this default path;
`artifacts/research/lod-features/model-surface-guard.json` records the test.
Candidate71d63f3b113f restores the entire model's original bytes; its scene and
cache passed native-reader, inventory and live admission checks. The full face
capture and physical workload acceptance remain incomplete. See
`artifacts/research/lod-features/cpu-skin/refined/surface-repaired/integration.json`.
