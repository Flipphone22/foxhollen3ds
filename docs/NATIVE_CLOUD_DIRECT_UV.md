# Direct cloud UV candidate — hardware confirmation pending

The New 3DS sky probe completed successfully and reproduced the actual visual
fault in `console-sky-probe/results-20261012-010103-223`. The captured cloud
draw using the full scene shader has zero colour variance. The same geometry
using interpreter-produced vertex outputs shows detailed clouds. Disabling
culling/depth, uniformly scaling clip coordinates and using RGBA8 storage do
not restore the detail; ordinary and large-W texture rectangles both work.
This isolates the shader/output path, not the exact failing instruction.

The updated [UV probe](SKY_UV_PROBE.md) is installed on New223 with staged and
final full-byte verification (`console-sky-probe/20261012-010855-223`). Its
binary is `314996a22bed2dbb0ac76ef39bd51cc83ebfe7bcf4e9681681bb78cbd71e5604`.
It tests direct UVs, disabled special-coordinate writes, initialization lifetime,
boolean routing and standalone compilation. Hardware results are pending.
All nine local images were inspected; all variant/control images match the
local baseline within three byte levels. Azahar does not reproduce the fault.

The gameplay candidate routes source cloud callback materials through a native
183-instruction shader using direct vertex UVs. It retains the existing bone
palette, skinning, eight lights, colour/opacity, projection, material/raster state,
and fog outputs. Admission excludes UI and sphere-map materials. World, shadow,
fur and ordinary actor transitions explicitly reset selection through the existing
scene owner. There are no CPU-generated vertices, new geometry passes, texture
copies, workers or game archive changes. The other linked shaders remain unchanged.

The existing scene owner invalidates material/HUD/glow/fog state and republishes
attributes and palettes at program transitions. Initialization and retired
shutdown follow its existing SDK lifecycle. This adapts the pinned Melee program
switch/cache-reset discipline. It does not make another program switch free.

`test_pica_cloud_shader.py` compares 32,768 ordinary world/actor inputs with the
accepted shader using Azahar's actual float24 interpreter. All 917,504 output
lanes match exactly, including lighting and blend endpoints. The test excludes
the fur/specular coordinate modes that are deliberately not admitted here.

Short isolated palace replays complete on both emulated hardware profiles,
preserving saves, checking replay state, draining logs and exiting owned processes.
Both captures1410 were inspected: world, Krystal, EarthWalker, purple creatures,
particles and HUD remain. They are handoff/regression evidence, not an intro-sky
image acceptance. Cloud shader draws: New1340, Old1306. Material-bind counters
are separate from actual shader draw counts and must not be equated.

| Measurement | New profile | Old profile |
|---|---:|---:|
| Added scene + submission CPU versus accepted postlight | +0.0104 ms | +0.0718 ms |
| Normal heap peak / reserve | 48,481,328 / 71,303,168 B | 40,944,688 / 50,331,648 B |
| Linear free / reserve | 18,188,288 / 25,165,824 B | 3,508,224 / 10,485,760 B |
| VRAM free | 4,723,712 B | 4,723,712 B |
| Process commitment / grant | 103,309,312 / 130,023,424 B | 67,657,728 / 83,886,080 B |

These are short emulator comparisons with differing animation phases. Old startup
overlapped completion of lossless compression of older completed evidence; no
other emulator or build overlapped the measured gameplay. No console FPS estimate
is inferred. The candidate adds a few hundred heap bytes and two code pages;
linear/VRAM peaks are unchanged. Physical repair and GPU cost remain unverified.

Shared normal `d55ed2e9b2b0` and replay `a0f1b1655909` are built locally. Their
447 source identities, game archive, exact linked cloud shader, unchanged main
program and Old80/New124 CIA profiles are checked by `verify_cloud_direct_builds.py`.
Normal UI/startup was not rerun: unchanged UI evidence and shared renderer/archive
identity are reused. Neither application is deployed; the requested combined
toggle/sky release remains pending hardware sky acceptance.

Separately, the single-layer shared-specular routing attempt was not accepted:
New CPU changed by only -0.0077 ms while command bytes increased from81,969,456
to85,814,208 and pipeline publications from7835 to9518. It remains an isolated
experiment and is not part of this candidate. Evidence:
`specular-shared-test/runtime-comparison.json`.

## Intro integration and latest hardware collection

The unchanged 64-packet recording prefix through simulation tick5600 completed
in `cloud-direct-intro-v1-new` (135.91 host seconds), with successful replay
status, unchanged save, drained logs and owned-process exit. Captures5280/5400
show cloud detail, rain and ship/characters through the changing camera.
There were6452 actual cloud shader draws. The frame5047..5467 window reports
59.831 Azahar FPS, 0.997ms scene preparation and3.639ms native submission.
There is no matched intro baseline or physical performance inference.
Heap peak48,785,256/71,303,168B; linear free16,868,480/25,165,824B;
VRAM free4,723,712B; process103,309,312/130,023,424B; measured stack72,956B.
See `cloud-direct-shader-test/intro-validation.json`. This is a prefix check,
not complete opening progression or hardware sky acceptance.

After the latest user completion report, the SD still contained the exact
first probe log (source4ed8c2d99fcfff64). Readback confirms the updated executable
314996a22bed is installed, with source3d608c02242b8f60. Collection013225 retained
the mismatch; installation013311 records executable bytes and directory times.
The reason for the unchanged result is unknown. No remote files changed.
The updated UV result and normal deployment remain pending.

Latest collection020255 again contained the first probe result. The verified
UV cases now have a distinct launcher and result folder:
`SD:/3ds/foxhollow-sky-uv/foxhollow-sky-uv.3dsx`, build6e9d1627a598ed11.
Transfer20261012-020556-223 is complete with staged/final byte verification.
Its unchanged GPU fixture/capture identity and visible log-open failure are
recorded in SKY_UV_PROBE.md. Hardware confirmation still pending; the reason
for the unchanged old log has not been established.

## Physical UV result, 12 October 10:52 UTC

The New 3DS completed the correct `SKY UV v2` build
`6e9d1627a598ed11`. Read-only collection
`artifacts/console-sky-probe/results-20261012-105216-223` contains its log,
build record, all nine original captures and the inspected contact sheet.
`verify_physical_sky_uv.py` validates their identities and pixel comparisons.

| Case | Physical result |
|---|---|
| Original, repeated original | Identical flat colour |
| Direct ordinary UV arithmetic | Detailed clouds |
| Specular UV write removed | Identical to direct UV |
| Fur UV write removed | Identical flat failure |
| Precomputed-output reference | Detailed clouds |
| UV initialized near consumers | Identical flat failure |
| Specular/light7 boolean slots swapped | Identical to direct UV |
| Original shader compiled alone | Identical flat failure |

The three successful variants differ from the reference by at most two byte
levels per channel; mean absolute error is 0.004302083 byte, with 819 of 96,000
pixels differing. This supports direct UVs for ordinary cloud materials and
implicates the original specular UV branch/control. It does **not** establish a
universal b15 defect, prove why the branch fails, or justify moving another live
feature into b15. Pinned citro3d `source/uniforms.c` publishes the full 16-bit
boolean mask with `0x7FFF0000`; no SDK publication defect has been established.

The accepted local gameplay candidate already contains a separate direct-UV
cloud shader. This physical fixture validates the repair mechanism, not that
whole application's handoffs, full intro or performance. No further sky probe
is requested for this fault. No executable, asset, preference or save changed
on the console during collection. No new normal build was deployed this turn.

## Confirmed second launcher result, 12 October 13:57 UTC

The user reran the original sky-probe launcher. Collection
`artifacts/console-sky-probe/results-20261012-135716-223` now contains the correct
source `3d608c02242b8f60`, including `direct-UV`, and all nine completed captures.
Their bytes exactly match the already accepted separate SKY UV launcher result.
The contact sheet was inspected and all reference bounds passed again. This
resolves the earlier stale-result ambiguity for the original launcher. No more
sky probes are needed for this fault. No remote files were modified, no new
normal application was deployed, and this fixture does not measure game FPS.

## Follow-up collection, 12 October 16:06 UTC

Following the user's report of running the probe, read-only collection
`artifacts/console-sky-probe/results-20261012-160644-223` retrieved the expected
`3d608c02242b8f60` log and all nine completed cases, including `direct-UV`.
The validator passed and the contact sheet was inspected. Direct UVs restore
cloud detail, within two channel byte levels of the reference. The nine captures
match the previously accepted result byte-for-byte; this deterministic fixture
does not independently timestamp a launch. No further probe is needed for this
fault. No deployment or remote file changes occurred. The whole-game integration,
performance targets and broader active goal remain incomplete.
