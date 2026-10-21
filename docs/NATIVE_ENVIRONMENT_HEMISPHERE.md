11 October New223 deployment COMPLETE: normal `4ad2bfc39ced` / CIA
`b93ef5c89545`, replay `01d268cc7891` / CIA `d5fcb77b8a07`. New texture
page caching plus the accumulated environment-normal/unlit-material corrections
are installed. Eight changed application/identity files received full staged
byte readback, rename and final-size checks. Ten original asset/cache files
reuse prior completed byte proof plus current descriptor/topology bytes and
sizes; large asset bytes were not freshly reread. Saves/preferences preserved.
Old229 is unchanged. Matching3dsx ready; CIA requires FBI installation.
Physical stalls, fur and reported visibility/lighting acceptance remain open.
Evidence: `artifacts/console-settings-update/20261010-190758-223/manifest.json`.

# Native environment normal correction

11 October 2026: local, not deployed. Replay `bc8b58203277` / CIA
`96979c091437`; normal `dd1ac4d7ac4c` / CIA `b62c3cc9425a`. The installed
New console still has the GPU-fur/original-palette update described in
[the deployment record](RENDERER_UPDATE_20261011.md).

The analytic environment shader reconstructed a normal quaternion from a
view-space normal's X/Y components. Fur expansion and bone blending can make
those components longer than a unit-circle radius. The old calculation then
produced a non-unit quaternion; PICA's fragment-stage normalization could turn
the resulting normal behind the light-map hemisphere. The original
`Rcp_DrawWarpDistortionMesh` uses nonnegative Z, and its texture lookup clamps
the axes. A backwards-facing reconstructed normal is not that behavior.

The new shader clamps each lookup axis, normalizes X/Y when outside the unit
circle, and constructs a unit quaternion in the front hemisphere. The original
normal remains available to vertex lighting and specular coordinates. Four
existing light accumulation pairs use MAD, and a redundant zero assignment is
removed, keeping the combined program at 511 instructions. The separate GPU
fur program remains 214 instructions. There are no extra draws, buffers, CPU
vertex operations, workers, or uniform/Boolean ABI changes.

This repairs the existing analytic approximation. It does not reproduce the
original 32x32 light-map rasterization, its corner normal amplitudes, or all
lighting/material families. The cockpit captures still show green illumination
and incomplete fur. Subsequent inspection of the archived desktop reference
confirms green cockpit illumination is intentional; that tint alone is not
evidence of a native bug. See [the source material follow-up](NATIVE_UNLIT_TEXTURES.md).
This change must not be reported as fixing all RGB,
day/night, eye, shadow, or physical-console rendering issues.

## Verification

`tools/test_pica_environment_hemisphere.py` executes the actual linked shaders
in Azahar's float24 interpreter. Across 16,641 synthetic lookup coordinates,
the old shader produces 15,844 backwards normals; the repaired shader produces
none. These are deliberately broad synthetic inputs, not counts of affected
game vertices. Maximum normal-vector error against the intended analytic
front-hemisphere oracle is 1.021e-7, versus 0.9394 previously. Another 4,096
interpolations remain in front. The old shader is rejected as a negative control.

65,536 combinations cover ordinary/world/fur outputs, vertex and alpha
lighting, morph inputs, UV/fog, and specular coordinates. Non-quaternion outputs
retain their checked values; sampled color changes are zero. The original
joint-loop comparison also passes 61,440 GPU-fur cases, including authored
pivots, all 16 slots and 15 shells. Maximum clip-position error is 0.000732422;
normal-quaternion error is 5.067e-7. These tests do not establish raster or
physical GPU correctness.

Serial isolated cockpit replays complete with state assertions, save
preservation, drained logs, and owned-process exit. Both retrace-900 captures
were inspected: Fox, eyes, cockpit, menu and HUD remain present. Old also
retains its touch controls. Matching ordinary-app code is verified by identical
game archive, all 317 native source files, and both actual linked shader blobs;
unchanged UI evidence is reused rather than claiming a new normal-app run.

| Azahar configuration | Previous scene / submission ms | Candidate scene / submission ms | Candidate FPS |
|---|---:|---:|---:|
| New | 0.757768 / 1.795900 | 0.758775 / 1.796348 | 59.8312 |
| Old | 0.762240 / 1.795885 | 0.757804 / 1.796599 | 59.8265 |

These are short paced runs with slightly different animation phases and
draw counts, not a speedup or physical-FPS claim. A brief host shader check
may overlap part of the New run; do not use noise-scale differences as a
precise performance estimate. New rendering CPU differs by about +0.00145 ms.

New peak heap is 47,848,144 / 71,303,168 bytes; Old is
40,508,112 / 50,331,648. Minimum free linear RAM is respectively
18,742,144 / 25,165,824 and 4,062,080 / 10,485,760. Both retain 4,846,592
free VRAM bytes and measure 72,740 stack bytes within a 524,288 reservation.
Process commitments remain 103,198,720 / 130,023,424 New and
67,547,136 / 83,886,080 Old. Shared CIA profiles remain Old80/New124.

Source snapshots, builds and reports are under
`artifacts/research/pica-scene-rewrite/environment-hemisphere-*`.
`verify-environment-hemisphere-builds.py` confirms archive, sources, linked
shader bytes and memory profiles. Six unrelated VRAM work-in-progress files
were isolated and restored; never build them accidentally. Pinned Melee
renderer/lighting uniform ownership and pinned Azahar quaternion normalization
were consulted. No SDK ownership or GPU resource lifetime was changed.

## Fur wind investigation

`tools/audit_pica_fur_wind.py` evaluates a prospective per-vertex approximation
of the source caustic wind on all ten prepared fur models, 16 scroll phases,
and four sample positions per triangle. This is an offline source-formula
audit; NumPy/libm transcendental rounding is not proven identical to the
original game library. No assets or gameplay renderer were changed by it.

The coarse approximation is visibly too inaccurate by the chosen numeric
criterion: Fox/Krystal outer-shell offsets differ by up to 9.84 noise texels,
and another model reaches 44.19. Even 16-fold triangle subdivision leaves
maximum errors of 1.31 to 6.23 texels across these examples, before considering
the separate material-RGB indirect offset. That expensive approximation was
not integrated. Faithful caustic/material offsets, layered/projected fur,
original-image acceptance and physical performance remain open. Evidence:
`fur-wind-audit/report.json`.
