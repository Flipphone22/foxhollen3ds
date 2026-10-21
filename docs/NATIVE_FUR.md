11 October clarification: SOURCE FUR ON looks underwhelming compared with original,
NOT obviously broken/corrupted. Density explanation is insufficient; do not turn
this into another deformation/crash report. GoalACTIVE/INCOMPLETE/no blocker.
This turnPROGRESS: source direction-map inventory, interpolation quality audit,
standalone native GPU groom kernel. No gameplay/build/console/assets/save changes.
Original fur mask alpha is consumed; authored B/G layer displacement is missing.
Correct model resolver uses textureLoad(-(id|0x8000),1), NOT generic TEXTABLE.
Initial audit placeholder resolved via wrong positive remap was corrected.
Nine unique used MODELS-bank fur declarations/three maps; Fox465/model1 share
64x64RGB5A3. See docs/NATIVE_FUR_GROOM.md, fur-groom-audit/{report,interpolation}.
Fox1119tri p95outerdirectionerror2.186texels;4xtri.949;16xtri.348, max8.138/3.228/
2.119. Level0linear/masked barycentric samples, not visual or maximum proof.
Uniformsubdivision NOTaccepted. Avoid claiming lower coordinateerror=>imagewin.
Separate pica_fur_groom_shader prototype214->216instructions/same96float/16bool/
7outputs. Attribute6signedB/G, existinguniformscalar,2instructions/homogeneousQ.
32768ON/OFF actualfloat24 PASS;851968retainedlanesexact,maxUV5.27407974e-6,
30456missingQcounterexamples. Initial fixture-initializer assertion corrected.
NO runtime stream owner/integration, no physical/perf/image acceptance yet.
Old/Newinstalledunchanged; C-menuaccepted; localshadowcandidateNOTdeployed.
AllownedtoolprocessesTERMINAL(49689,5517,17191,78386,61931). Next boundednative
fur direction representation + sourceimage comparison; wind/projectedlighting
remainmissing. Preserve latestsnapshot/WIP isolation and activegoal.
11 October user report: fur appearance is still wrong WITH SOURCE FUR ON. Full layer density is not a sufficient repair. Remaining wind/displacement/material and lighting fidelity must be compared against original images; previous successful shader/counter tests do not accept visual parity.

11 October New223 fur-projection deployment COMPLETE: normal
`d6068a861f8f` / CIA `238bbeb7a501`; replay `e6178b4e5cd0` / CIA
`b72e38226b55`. Eight changed app/identity files staged and byte-readback
verified, then renamed and sizes checked. Unchanged original asset/cache
payloads retain prior completed byte proof plus current descriptor/topology
and sizes; no fresh bulk asset readback. Saves and preferences preserved.
Old229 unchanged. Matching3dsx ready; install the CIA with FBI to update HOME:
`SD:/3ds/foxhollow/foxhollow.cia` (replay sibling folder/app name).

The normal app also passed its own32.43-second headless check: FUR enabled
through the bottom menu, ordinary Start to cockpit,1140 actual fur draws,
saved option mask1064961 with valid checksum, Select/drain/owned exit;900
capture inspected. New/Old replay checks and measured cost are detailed in
[fur projection](NATIVE_FUR_PROJECTION.md). This repairs one confirmed noise
coordinate error, not all fur effects or other reported visibility/lighting/
shadow defects. Physical projective-sampler performance and appearance still
need validation. FTP collection230702 contains the prior run, not new evidence
for the installed logging changes or a physical stall improvement.
Manifest: `artifacts/console-settings-update/20261010-231226-223/manifest.json`.

11 October fur projection repaired locally. Normal `d6068a861f8f` and replay
`e6178b4e5cd0` are built, NOT DEPLOYED. The old native noise mapping discarded
the original interpolated Q coordinate. Both native shell paths now use PICA
projective sampling; no new pass, texture storage, CPU vertices or shader
instructions. New matched cockpit CPU preparation/submission is2.402663ms
versus2.401824ms before, unchanged within this short emulator comparison.
Both New/Old correctness runs pass; captures inspected. No physical GPU cost
or full-fur fidelity claim. Wind and other reported FS/visibility/lighting/
shadow defects remain open. See [source error and evidence](NATIVE_FUR_PROJECTION.md).
Installed New223 remains normal9eec94e236e3/replay97a579346965 from the
previous completed deployment; no FTP or remote changes in this follow-up.

11 October New223 deployment complete: normal `626d217324e9` / CIA
`762ef1732702`, replay `9bcad101a4fe` / CIA `9acc42b6e738`, with matching
original-geometry22-slot pack and lossless cache. All18 changed payloads staged,
byte-readback verified, renamed and final-size checked. Saves/preferences
preserved; Old229 unchanged. CIA requires FBI installation; matching3dsx ready.
Performance-geometry22-slot pack remains local; original-asset installer does
not replace the generic performance pack. Earlier status entries are historical.
See [changes, tests and unresolved issues](RENDERER_UPDATE_20261011.md).

11 October GPU shell expansion integrated locally: normal626d217324e9 and
replay9bcad101a4fe, NOTdeployed. Full-source cockpit CPU cost now0.330329ms
versus same-binary OFF (below0.5gate), retaining all shell/triangle counts.
No physical GPU budget or full-fur fidelity acceptance; wind/layered/projected
families remain missing. See [GPU shell ownership and evidence](NATIVE_FUR_GPU_EXPANSION.md).

11 October local source-shell density and22-slot asset regrouping are under
validation, not deployed. See [cost gate and exact scope](NATIVE_FUR_SOURCE_LAYERS.md).
Full density still exceeds the New CPU budget; wind/material parity remains open.

10 October local fur coverage now uses the original indirect alpha/intensity mask through a third native sampler; New/Old checks add about0.011/0.010ms renderingCPU and16KiBlinear in the cockpit. Built normalc70bf119b342/replay50e46003c21e, NOTdeployed. Physical flicker, indirect wind and full parity remain open. See [NATIVE_FUR_COVERAGE.md](NATIVE_FUR_COVERAGE.md).

10 October deployment update: New 3DS normal `0b55ba943be8` and replay `bddd446fde84` uploaded and readback-verified at user request. Saves/settings preserved; Old 3DS unchanged. CIA copies require FBI installation. Earlier local-only wording below describes the pre-deployment checks. Physical startup recovery and fur acceptance remain pending.

10 October follow-up: native noise RGB perturbation is restored locally in
normal 0b55ba943be8 / replay bddd446fde84, not deployed. Matched New Azahar
CPU difference +0.00217 ms/frame, unchanged memory/draws; Old correctness/layout
audit passes. Full visual/hardware acceptance remains open. FUR still defaults
OFF and retains original eligibility/distance checks, not a cutscene-only gate.
See [fur colour restoration](NATIVE_FUR_NOISE.md).

10 October follow-up: a combined-effects Thorntail test found that fur can
be the first visible pass, before ordinary pose preparation. The local adapter
now preserves the original first-shell state preparation with no old rendering
interpreter. The repaired route completes; normal eb279defa688 and replay
596b50a968e3 are built, not deployed. See
[the failure and validation record](PICA_LAYOUT_AUDIT_20261010.md).

10 October update: these local features are now included in the New 3DS
normal `0b2c0f722073` and replay `42d91e75aa0f` upload, readback-verified.
CIA installation through FBI is still required for HOME icons. Old installation
is unchanged. Earlier candidate IDs and “not deployed” statements below are
historical. Scope, unfinished fidelity and performance limits still apply.
See [the update summary](RENDERER_UPDATE_20261010.md).

# Native fur candidate

10 October: local replay `cec756f60ebb` and normal `6f1a8da8bfac` are built,
not deployed. Both use the same game archive and exact renderer shader; their
shared CIA profiles remain Old 80 MiB / New 124 MiB. FUR is an immediate,
persistent graphics option on page 9, default OFF on both models. Recording and
replay retain their existing graphics-mask lock. This is partial fur, not parity.

The scene queue now admits the native `objRenderFuzz` replacement. It consumes
the already animated resident mesh, live material/light state, original joint
pivots/radii and original generated noise pages. The original distance admission
and layer-count clamp remain. At most two shells are drawn, at the first and
last admitted source layer; intermediate layers are an explicit detail reduction.
Only source mesh groups carrying the fur flag participate. Their palette-change
masks are prepared at load time for that subsequence, independently of the base
mesh plan. No original GX/TEV interpreter, CPU vertex stream, second animation
update, per-frame texture conversion or new graphics worker is involved.

The necessary CPU work is the source admission calculation, one live parameter
snapshot and composition/publication of the changed bone matrices. PICA performs
skinning and the original normalized noise-coordinate calculation. Static native
combiners implement the ordinary base colour/environment contribution and the
source no-indirect alpha equation `clamp(noiseAlpha + objectAlpha - 1)`. Depth
tests remain enabled, depth writes disabled and alpha blending enabled.

The noise pages are published once at the original generation boundary. Their
bytes and generation/RNG algorithm are unchanged. They borrow the existing
resident image/HUD ownership system, which retires GPU work before release.
They currently remain resident even with FUR OFF. No VRAM placement experiment
is included. The pinned Melee renderer's explicit write-site cache publication
and resource-retirement model were consulted; no new cache or fence policy was
introduced. Audio already occupies New core 2; no extra CPU core is assumed free.

The combined shader occupies 511 of 512 instructions. The fur UV calculation
uses an otherwise unused material-alpha uniform; fur coverage is texture-owned.
Removing a duplicated branch instruction and folding one light multiply/add
keeps the existing linker limit intact. An exact Azahar float24 interpreter
comparison of 32,768 ordinary world/actor vertices found zero output differences;
32,768 fur coordinate comparisons had maximum error 0.000003815. The normal and
tested replay contain that exact shader, verified by SHA256.

Host checks compare 270,000 transformed joint rows against the pinned source
matrix loop (maximum translation error 0.0003052 world units), plus 2,000
CPU-stream pivot cases. The immutable-plan audit covers 835 models and 604 fur
palette slots. The actual native material owner passes 262,144 no-indirect alpha
equations, 1,024 RGB/UV cases and 1,000 forced state handoffs under ASan/UBSan.

## Short cockpit check

These are serial Azahar runs of the same ordinary-input recipe. Animation phase
can differ; these are not physical-console timings or a whole-game gate.

| Emulated profile | FUR | FPS | Scene preparation ms | Native submission ms |
|---|---|---:|---:|---:|
| New | OFF | 59.83122 | 0.718818 | 1.640236 |
| New | ON | 59.83095 | 0.766259 | 1.842362 |
| Old | ON | 59.83165 | 0.764491 | 1.839756 |

The matched New addition is 0.249568 ms across those CPU phases. Both ON runs
submit 1,448 shells / 4,344 fur draws / 4,860,936 indices across the full run,
with no unsupported fur materials in this scene. Direct native shell submission
averages 0.109584 ms New / 0.108375 ms Old in Azahar; that overlaps the submission
column and must not be added again. Approximately six draws / 6,714 indices are
added per cockpit frame. Physical vertex/fill/bandwidth cost remains unmeasured.
Do not infer physical Old 60 FPS from this emulator result.

State checks, save preservation, shutdown/log drain and owned-process exit pass.
Cockpit captures at retrace 900 on both profiles were inspected: Fox, facial
geometry, clothing, cockpit and menu remain visible without distant vertices.
These captures do not establish full original fur appearance, every pose, or
interaction with particles, shadows and cutscenes.

| Budget observation | New | Old |
|---|---:|---:|
| Peak normal heap / reservation | 47,852,816 / 71,303,168 B | 40,512,784 / 50,331,648 B |
| Minimum free linear / reservation | 19,231,872 / 25,165,824 B | 4,551,808 / 10,485,760 B |
| Initialized free VRAM | 4,858,880 B | 4,858,880 B |
| Process commitment / limit | 103,157,760 / 130,023,424 B | 67,506,176 / 83,886,080 B |
| Measured main stack / reservation | 72,788 / 524,288 B | 72,788 / 524,288 B |

## Remaining work and rejected checks

Indirect wind/length masks, dedicated fur-shell/shadow
callbacks and unsupported layered/masked materials are not restored. Base meshes
remain visible for unsupported fur materials; omissions are counted. These are
required parity work. Wider scenes and physical performance still need validation
before changing defaults. Initial CPU feature budgets are 0.5 ms New / 1.5 ms
Old, subject to the whole-frame targets; the cockpit emulator check alone cannot
accept those physical budgets.

The first candidate failed from an old game archive using a 96-byte native draw
descriptor against the new 100-byte descriptor. Rebuilding `main/model.c` fixed
it; runtime ABI and linked-hook guards now reject stale integration. The v7
cockpit runs completed but did not exercise fur because the queue still filtered
that entry. They are explicitly not fur validation. V8 fixes admission, restores
the source maximum-layer clamp and logs real fur work through the file logger.
The planar-shadow counters also now use that logger rather than console printf.

Reproduce with `build-native-fur-v8.py`, `check-native-fur-v8.py new on`,
`new off`, `old on`, `analyze-native-fur.py`, `build-native-fur-normal-v1.py`
and `verify-native-fur-builds.py` under
`artifacts/research/pica-scene-rewrite/`. The build helpers isolate and restore
the unrelated VRAM WIP. Evidence is in `native-fur-test/`,
`native-fur-material-test/` and `native-fur-v7-builds-shader-test/`; runtime
folders are `artifacts/native/native-fur-v8-*`. No console files or saves changed.
